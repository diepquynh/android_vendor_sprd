#ifdef SPRD_FRAMEWORKS_CAMERA_EX
#include "api1/Camera2ClientEx.h"
namespace android {
namespace camera2 {
    void playSound(Parameters &params,sp<CameraService>& mCameraService,bool start) {
    ALOGD("%s: Camera2Client  %d",  __FUNCTION__, params.playShutterSound);
    if (params.playShutterSound) {
        if(start)
            mCameraService->playSound(CameraService::SOUND_RECORDING_START);
        else
            mCameraService->playSound(CameraService::SOUND_RECORDING_STOP);
    }
}
};
};
#endif //#ifdef SPRD_FRAMEWORKS_CAMERA_EX
