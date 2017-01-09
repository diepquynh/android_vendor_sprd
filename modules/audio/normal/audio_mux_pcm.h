#ifndef __AUDIO_MUX_PCM_H
#define __AUDIO_MUX_PCM_H
#include <stdlib.h>
#include <tinyalsa/asoundlib.h>



#define SND_CARD_VOICE_TG 1
#define SND_CARD_VOIP_TG    2
#define SND_CARD_MUX_MAX   3


struct pcm * mux_pcm_open(unsigned int card, unsigned int device,
                     unsigned int flags, struct pcm_config *config);

int mux_pcm_write(struct pcm *pcm_in, void *data, unsigned int count);

int mux_pcm_read(struct pcm *pcm_in, void *data, unsigned int count);

int mux_pcm_close(struct pcm *pcm_in);



#endif
