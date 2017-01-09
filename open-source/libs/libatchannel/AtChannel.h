#ifndef ANDROID_ATCHANNEL_H
#define ANDROID_ATCHANNEL_H

#ifdef __cplusplus
extern "C" {
#endif

const char* sendAt(int modemId, int simId, const char* atCmd);

#ifdef __cplusplus
}
#endif


#endif // ANDROID_ATCHANNEL_H
