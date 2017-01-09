/* alsa_pcm_inf.c
**
** Copyright 2011, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of The Android Open Source Project nor the names of
**       its contributors may be used to endorse or promote products derived
**       from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY The Android Open Source Project ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL The Android Open Source Project BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <limits.h>

#include <linux/ioctl.h>
#include <tinyalsa/asoundlib.h>
#include <sound/asound.h>

#define PCM_ERROR_MAX 128

struct pcm {
  int fd;
  unsigned int flags;
  int running : 1;
  int prepared : 1;
  int underruns;
  unsigned int buffer_size;
  unsigned int boundary;
  char error[PCM_ERROR_MAX];
  struct pcm_config config;
  struct snd_pcm_mmap_status *mmap_status;
  struct snd_pcm_mmap_control *mmap_control;
  struct snd_pcm_sync_ptr *sync_ptr;
  void *mmap_buffer;
  unsigned int noirq_frames_per_msec;
  int wait_for_avail_min;
};

static unsigned int pcm_format_to_alsa(enum pcm_format format) {
  switch (format) {
    case PCM_FORMAT_S32_LE:
      return SNDRV_PCM_FORMAT_S32_LE;
    case PCM_FORMAT_S8:
      return SNDRV_PCM_FORMAT_S8;
    case PCM_FORMAT_S24_3LE:
      return SNDRV_PCM_FORMAT_S24_3LE;
    case PCM_FORMAT_S24_LE:
      return SNDRV_PCM_FORMAT_S24_LE;
    default:
    case PCM_FORMAT_S16_LE:
      return SNDRV_PCM_FORMAT_S16_LE;
  }
}

static inline int param_is_interval(int p) {
  return (p >= SNDRV_PCM_HW_PARAM_FIRST_INTERVAL) &&
         (p <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL);
}

static inline struct snd_interval *param_to_interval(
    struct snd_pcm_hw_params *p, int n) {
  return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

static void param_set_min(struct snd_pcm_hw_params *p, int n,
                          unsigned int val) {
  if (param_is_interval(n)) {
    struct snd_interval *i = param_to_interval(p, n);
    i->min = val;
  }
}

static inline struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p,
                                             int n) {
  return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

static inline int param_is_mask(int p) {
  return (p >= SNDRV_PCM_HW_PARAM_FIRST_MASK) &&
         (p <= SNDRV_PCM_HW_PARAM_LAST_MASK);
}

void param_set_mask(struct snd_pcm_hw_params *p, int n, unsigned int bit) {
  if (bit >= SNDRV_MASK_MAX) return;
  if (param_is_mask(n)) {
    struct snd_mask *m = param_to_mask(p, n);
    m->bits[0] = 0;
    m->bits[1] = 0;
    m->bits[bit >> 5] |= (1 << (bit & 31));
  }
}

void param_init(struct snd_pcm_hw_params *p) {
  int n;

  memset(p, 0, sizeof(*p));
  for (n = SNDRV_PCM_HW_PARAM_FIRST_MASK; n <= SNDRV_PCM_HW_PARAM_LAST_MASK;
       n++) {
    struct snd_mask *m = param_to_mask(p, n);
    m->bits[0] = ~0;
    m->bits[1] = ~0;
  }
  for (n = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
       n <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; n++) {
    struct snd_interval *i = param_to_interval(p, n);
    i->min = 0;
    i->max = ~0;
  }
  p->rmask = ~0U;
  p->cmask = 0;
  p->info = ~0U;
}

void param_set_int(struct snd_pcm_hw_params *p, int n, unsigned int val) {
  if (param_is_interval(n)) {
    struct snd_interval *i = param_to_interval(p, n);
    i->min = val;
    i->max = val;
    i->integer = 1;
  }
}

int pcm_set_samplerate(struct pcm *pcm, unsigned int flags,
                       struct pcm_config *config, unsigned short samplerate) {
  struct snd_pcm_hw_params params;

  if (pcm->fd < 0) {
    fprintf(stderr, "%s, error pcm_fd (%d) ", __func__, pcm->fd);
    return -1;
  }
  if (config == NULL) {
    fprintf(stderr, "%s, error pcm config ", __func__);
    return -1;
  }

  param_init(&params);
  param_set_mask(&params, SNDRV_PCM_HW_PARAM_FORMAT,
                 pcm_format_to_alsa(config->format));
  param_set_mask(&params, SNDRV_PCM_HW_PARAM_SUBFORMAT,
                 SNDRV_PCM_SUBFORMAT_STD);
  param_set_min(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, config->period_size);
  param_set_int(&params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS,
                pcm_format_to_bits(config->format));
  param_set_int(&params, SNDRV_PCM_HW_PARAM_FRAME_BITS,
                pcm_format_to_bits(config->format) * config->channels);
  param_set_int(&params, SNDRV_PCM_HW_PARAM_CHANNELS, config->channels);
  param_set_int(&params, SNDRV_PCM_HW_PARAM_PERIODS, config->period_count);
  param_set_int(&params, SNDRV_PCM_HW_PARAM_RATE, samplerate);

  if (flags & PCM_NOIRQ) {
    if (!(flags & PCM_MMAP)) {
      fprintf(stderr, "%s, noirq only currently supported with mmap(). ",
              __func__);
      return -1;
    }
    params.flags |= SNDRV_PCM_HW_PARAMS_NO_PERIOD_WAKEUP;
  }
  if (flags & PCM_MMAP)
    param_set_mask(&params, SNDRV_PCM_HW_PARAM_ACCESS,
                   SNDRV_PCM_ACCESS_MMAP_INTERLEAVED);
  else
    param_set_mask(&params, SNDRV_PCM_HW_PARAM_ACCESS,
                   SNDRV_PCM_ACCESS_RW_INTERLEAVED);

  if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_PARAMS, &params)) {
    fprintf(stderr, "%s, SNDRV_PCM_IOCTL_HW_PARAMS failed (%s) ", __func__,
            strerror(errno));
    return -1;
  }
  //    ALOGW("%s, out,samplerate (%d) ",__func__,samplerate);
  return 0;
}
