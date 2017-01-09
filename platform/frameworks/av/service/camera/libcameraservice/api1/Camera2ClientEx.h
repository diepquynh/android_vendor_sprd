#ifndef ANDROID_SERVERS_CAMERA_CAMERA2CLIENT_EX_H
#define ANDROID_SERVERS_CAMERA_CAMERA2CLIENT_EX_H
#include "api1/client2/Parameters.h"
#include "CameraService.h"
namespace android {
namespace camera2 {
    void playSound(Parameters &params,sp<CameraService>& mCameraService,bool start) ;
};
};
#endif
