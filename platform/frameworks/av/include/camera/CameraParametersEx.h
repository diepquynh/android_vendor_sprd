#ifndef ANDROID_HARDWARE_CAMERA_PARAMETERS_EX_H
#define ANDROID_HARDWARE_CAMERA_PARAMETERS_EX_H

#ifdef SPRD_FEATURE_BRIGHTNESS
// The camera string.
// Example value: "1". Read/write.
static const char KEY_BRIGHTNESS[];
// Supported the camera strings.
// Example value: "0,1,2,3,4,5,6". Read only.
static const char KEY_SUPPORTED_BRIGHTNESS[];
#endif /* SPRD_FEATURE_BRIGHTNESS */
#ifdef SPRD_FEATURE_3GVT
static const char KEY_SENSOR_ORIENT[];
static const char KEY_SENSOR_ROT[];
#endif // #ifdef SPRD_FEATURE_3GVT
#ifdef SPRD_FEATURE_BEAUTY
int getSprdPerfectSkinLevel() const;
void setSprdPerfectSkinLevel(int skinlevel);
static const char KEY_PERFECT_SKIN_LEVEL[];
#endif // #ifdef SPRD_FEATURE_BEAUTY

#ifdef SPRD_FEATURE_ISO
// Current iso settings.
static const char KEY_ISO[];
static const char KEY_SUPPORTED_ISO[];
#endif

#ifdef SPRD_FEATURE_CONTRAST
static const char KEY_CONTRAST[];
static const char KEY_SUPPORTED_CONTRAST[];
#endif

#ifdef SPRD_FEATURE_SATURATION
// Current saturation settings.
static const char KEY_SATURATION[];
static const char KEY_SUPPORTED_SATURATION[];
#endif

#ifdef SPRD_FEATURE_METERING_MODE
static const char KEY_METERING_MODE[];
static const char KEY_SUPPORTED_METERING_MODE[];
#endif

#ifdef SPRD_FEATURE_SLOW_MOTION
static const char KEY_SLOWMOTION[];
static const char KEY_SUPPORTED_SLOWMOTION[];
#endif

#ifdef SPRD_FEATURE_EIS
static const char KEY_EOIS[];
#endif

#endif /* ANDROID_HARDWARE_CAMERA_PARAMETERS_EX_H */
