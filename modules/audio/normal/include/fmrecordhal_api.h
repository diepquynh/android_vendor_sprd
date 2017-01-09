
#ifndef __FM_RECORD_HAL_API_H__
#define __FM_RECORD_HAL_API_H__

typedef void* FMPcmHandler;
extern FMPcmHandler fm_pcm_open(int samplerate,int channels,int bytes,int required_channel);
extern int fm_pcm_read(FMPcmHandler pcm,void* buf,int bytes,int waitflag,int channel);
extern void fm_pcm_close(FMPcmHandler pcm);

#endif

