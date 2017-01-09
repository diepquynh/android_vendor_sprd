#ifdef SPRD_FRAMEWORKS_CAMERA_EX
#define MAX_LEN 32
#include <camera/CameraParameters.h>
#include"CameraParametersEx.h"

namespace android {

#ifdef SPRD_FEATURE_BRIGHTNESS
// Values for brightness setting
const char CameraParameters::KEY_BRIGHTNESS[] = "brightness";
const char CameraParameters::KEY_SUPPORTED_BRIGHTNESS[] = "brightness-values";
#endif
#ifdef SPRD_FEATURE_3GVT
const char CameraParameters::KEY_SENSOR_ORIENT[] = "sensor-orient";
const char CameraParameters::KEY_SENSOR_ROT[] = "sensor-rot";
#endif // #ifdef SPRD_FEATURE_3GVT

#ifdef SPRD_FEATURE_ISO
//Values for iso settings
const char CameraParameters::KEY_ISO[] = "iso";
const char CameraParameters::KEY_SUPPORTED_ISO[] = "iso-values";
#endif

#ifdef SPRD_FEATURE_CONTRAST
// Values for contrast settings.
const char CameraParameters::KEY_CONTRAST[] = "contrast";
const char CameraParameters::KEY_SUPPORTED_CONTRAST[] = "contrast-values";
#endif

#ifdef SPRD_FEATURE_SATURATION
//Values for saturation settings.
const char CameraParameters::KEY_SATURATION[] = "saturation";
const char CameraParameters::KEY_SUPPORTED_SATURATION[] = "saturation-values";
#endif

#ifdef SPRD_FEATURE_METERING_MODE
//Values for metering mode settings.
const char CameraParameters::KEY_METERING_MODE[] = "metering-mode";
const char CameraParameters::KEY_SUPPORTED_METERING_MODE[] = "metering-mode-values";
#endif

#ifdef SPRD_FEATURE_SLOW_MOTION
const char CameraParameters::KEY_SLOWMOTION[] = "slow-motion";
const char CameraParameters::KEY_SUPPORTED_SLOWMOTION[] = "slow-motion-values";
#endif

#ifdef SPRD_FEATURE_BEAUTY
const char CameraParameters::KEY_PERFECT_SKIN_LEVEL[] = "perfect-skin-level";
int CameraParameters::getSprdPerfectSkinLevel() const
{
    return getInt(KEY_PERFECT_SKIN_LEVEL);
}
void CameraParameters::setSprdPerfectSkinLevel(int skinlevel)
{
    char str[MAX_LEN];
    snprintf(str , MAX_LEN , "%d" , skinlevel);
    set(KEY_PERFECT_SKIN_LEVEL, str);
}
#endif // #ifdef SPRD_FEATURE_BEAUTY

#ifdef SPRD_FEATURE_EIS
const char CameraParameters::KEY_EOIS[] = "EOIS";
#endif
}; // namespace android
#endif // SPRD_FRAMEWORKS_CAMERA_EX
