#ifdef SPRD_FRAMEWORKS_CAMERA_EX
#define MAX_LEN 32
#include <camera/CameraParameters2.h>
#include <stdlib.h>
namespace android{

#ifdef SPRD_FEATURE_BRIGHTNESS
int CameraParameters2::getSprdBrightNess() const
{
    return getInt(CameraParameters::KEY_BRIGHTNESS);
}
void CameraParameters2::setSprdBrightNess(int brightness)
{
    char str[MAX_LEN];
    snprintf(str , MAX_LEN , "%d" , brightness);
    set(CameraParameters::KEY_BRIGHTNESS, str);
}
#endif
#ifdef SPRD_FEATURE_3GVT
int CameraParameters2::getSprdSensorOrient() const
{
    return getInt(CameraParameters::KEY_SENSOR_ORIENT);
}
int CameraParameters2::getSprdSensorRot() const
{
    return getInt(CameraParameters::KEY_SENSOR_ROT);
}
#endif // #ifdef SPRD_FEATURE_3GVT

#ifdef SPRD_FEATURE_ISO
int CameraParameters2::getSprdISO() const
{
    const char *value = get(CameraParameters::KEY_ISO);
    int i = 0, ret = 0;
    if (value == 0)
        return -1;
    if (!strcmp(value, "auto")) {
        return 0;
    }
    else
    {
        ret = strtol(value, 0, 0) / 100;
        for (; i < 10; i++)
        {
            if (ret == (1 << i))
                return i + 1;
        }
        ALOGI("para error iso=%d", ret);
    }
    return -1;
}
void CameraParameters2::setSprdISO(int iso)
{
    char str[MAX_LEN];
    if (iso == 0)
        snprintf(str , MAX_LEN , "auto");
    else
        snprintf(str , MAX_LEN , "%d", (1 << (iso - 1)) * 100);
    set(CameraParameters::KEY_ISO, str);
}
#endif

#ifdef SPRD_FEATURE_CONTRAST
int CameraParameters2::getSprdContrast() const
{
    return getInt(CameraParameters::KEY_CONTRAST);
}
void CameraParameters2::setSprdContrast(int contrast)
{
    char str[MAX_LEN];
    snprintf(str , MAX_LEN , "%d" , contrast);
    set(CameraParameters::KEY_CONTRAST, str);
}
#endif

#ifdef SPRD_FEATURE_SATURATION
int CameraParameters2::getSprdSaturation() const
{
    return getInt(CameraParameters::KEY_SATURATION);
}
void CameraParameters2::setSprdSaturation(int saturation)
{
    char str[MAX_LEN];
    snprintf(str , MAX_LEN , "%d" , saturation);
    set(CameraParameters::KEY_SATURATION, str);
}
#endif

#ifdef SPRD_FEATURE_METERING_MODE
int CameraParameters2::getSprdMeteringMode() const
{
    const char *value = get(CameraParameters::KEY_METERING_MODE);
    if (value == 0)
        return -1;
    if (!strcmp(value, "frame-average"))
        return 0;
    if (!strcmp(value, "center-weighted"))
        return 1;
    if (!strcmp(value, "spot-metering"))
        return 2;
    ALOGI("para error metermode=%s", value);
    return -1;
}
void CameraParameters2::setSprdMeteringMode(int metermode)
{
    const char *str[4] = {
        "frame-average",
        "center-weighted",
        "spot-metering",
        NULL
    };
    if (metermode < 0 || metermode > 2)
        ALOGI("para error metermode=%d", metermode);
    else
        set(CameraParameters::KEY_METERING_MODE , str[metermode]);
}
#endif

#ifdef SPRD_FEATURE_SLOW_MOTION
int CameraParameters2::getSprdSlowMotion() const
{
    return getInt(CameraParameters::KEY_SLOWMOTION);
}
void CameraParameters2::setSprdSlowMotion(int slowmotion)
{
    char str[MAX_LEN];
    snprintf(str , MAX_LEN , "%d" , slowmotion);
    set(CameraParameters::KEY_SLOWMOTION, str);
}
#endif

#ifdef SPRD_FEATURE_BEAUTY
int CameraParameters2::getSprdPerfectSkinLevel() const
{
    return getInt(CameraParameters::KEY_PERFECT_SKIN_LEVEL);
}
void CameraParameters2::setSprdPerfectSkinLevel(int skinlevel)
{
    char str[MAX_LEN];
    snprintf(str , MAX_LEN , "%d" , skinlevel);
    set(CameraParameters::KEY_PERFECT_SKIN_LEVEL, str);
}
#endif // #ifdef SPRD_FEATURE_BEAUTY

#ifdef SPRD_FEATURE_EIS
const char *CameraParameters2::getSprdEOIS() const
{
    return get(CameraParameters::KEY_EOIS);
}
void CameraParameters2::setSprdEOIS(const char *eois)
{
    set(CameraParameters::KEY_EOIS, eois);
}
#endif
}; // namespace android

#endif // SPRD_FRAMEWORKS_CAMERA_EX
