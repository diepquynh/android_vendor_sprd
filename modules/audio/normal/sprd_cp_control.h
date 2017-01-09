#ifndef __SPRD_CP_CONTROL__
#define  __SPRD_CP_CONTROL__

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>


#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

#include <expat.h>

#include <tinyalsa/asoundlib.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

struct cp_control_res
{
    pthread_t thread;
    sem_t   sem;
    sem_t   sem_end;
    struct pcm_config config;
    struct mixer *cp_mixer;
    int init;
    int enable;
    int waiting_for_end;
    int fd;
    int card;
    int cb_func;
    pthread_mutex_t lock;
};

static const unsigned char s_pcm_mono[] = {
    0x92,0x02,0xcb,0x0b,0xd0,0x14,0x1d,0x1d,0xfc,0x24,0x17,0x2c,0x4a,0x32,0x69,0x37,
    0x92,0x3b,0x4e,0x3e,0x22,0x40,0x56,0x40,0x92,0x3f,0x12,0x3d,0x88,0x39,0x10,0x35,
    0xf0,0x2e,0x51,0x28,0xce,0x20,0x7f,0x18,0xd5,0x0f,0xda,0x06,0xdf,0xfd,0xa4,0xf4,
    0xa2,0xeb,0x39,0xe3,0x57,0xdb,0x3d,0xd4,0x1f,0xce,0xe2,0xc8,0xb1,0xc4,0xc0,0xc1,
    0xec,0xbf,0xc1,0xbf,0xa4,0xc0,0xf2,0xc2,0x18,0xc6,0xc2,0xca,0xc8,0xd0,0x36,0xd7,
    0xbb,0xde,0xe6,0xe6,0xa5,0xef,0xa6,0xf8,
};

#define AUDIO_MIXER_TO_CP_PCM_FILE_ROOT "/etc/record_tone.pcm"
#define AUDIO_MIXER_TO_CP_PCM_FILE "/data/local/media/record_tone.pcm"

int cp_mixer_enable(struct cp_control_res * record_tone_info, struct mixer *cp_mixer, int card);
int cp_mixer_disable(struct cp_control_res * record_tone_info, struct mixer *cp_mixer);
struct cp_control_res * cp_mixer_open(void);
int cp_mixer_close(struct cp_control_res * record_tone_info);
int cp_mixer_is_enable(struct cp_control_res * record_tone_info);

#endif
