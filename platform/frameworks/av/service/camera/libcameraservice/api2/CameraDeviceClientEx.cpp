#ifdef SPRD_FRAMEWORKS_CAMERA_EX
#include "api2/CameraDeviceClient.h"
#include "device3/Camera3Device.h"
namespace android {
binder::Status CameraDeviceClient::cancelPicture(/*out*/int* pictureNum){
    binder::Status res;
    *pictureNum = mDevice->cancelPicture();
    return res;
}
} // namespace android
#endif
