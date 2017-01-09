#ifndef _AUDIO_LOCAL_SOCKET_H_
#define _AUDIO_LOCAL_SOCKET_H_
#define AUDIO_SOCKET_NAME "audio_local_socket"
#define LOCAL_SOCKET_BUFFER_SIZE  4096
#define LOCAL_SOCKET_CLIENT_MAX_NUMBER 8
#ifdef LOCAL_SOCKET_SERVER
#include "audio_hw.h"
void start_audio_local_server(struct tiny_audio_device *adev);
#endif

#ifdef LOCAL_SOCKET_CLIENT
#endif

#endif
