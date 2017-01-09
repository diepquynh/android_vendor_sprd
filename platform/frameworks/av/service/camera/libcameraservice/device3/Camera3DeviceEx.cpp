#ifdef SPRD_FRAMEWORKS_CAMERA_EX
#include "device3/Camera3Device.h"
namespace android {
int Camera3Device::cancelPicture() {
    int cancelCount;
    ALOGV("canclePicture Camera3Device begin .. ");
    cancelCount = mRequestThread->removemRequestQueue();
    ALOGV("canclePicture Camera3Device end");
    return cancelCount;
}
int Camera3Device::RequestThread::removemRequestQueue() {
    ALOGV("RequestThread::%s:", __FUNCTION__);
    int resetCaptureRequestCount = 0;
    int lastid = 0;
    for (RequestList::iterator it = mRequestQueue.begin();
        it != mRequestQueue.end(); ++it) {
            ALOGV("removemRequestQueue::%d:",(*it)->mResultExtras.requestId);
            if(lastid == (*it)->mResultExtras.requestId) {
                resetCaptureRequestCount++;
            }
            lastid = (*it)->mResultExtras.requestId;
    }
    resetCaptureRequestCount++;
    ALOGD("resetCaptureRequestCount = %d",resetCaptureRequestCount);
    mRequestQueue.clear();
    return resetCaptureRequestCount;
}
} // namespace android
#endif
