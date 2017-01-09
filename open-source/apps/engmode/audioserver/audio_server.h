#ifndef _AUDIO_SERVER_
#define _AUDIO_SERVER_


#ifdef __cplusplus
extern "C" {
#endif

int setDeviceConnectionState(int device, int state);
int setForceUse(int usage, int config);
int setParameters();
int setVolume(int volume);

#ifdef __cplusplus
}
#endif

#endif

