#ifndef ANDROID_HARDWARE_CAMERA_PARAMETERS2_EX_H
#define ANDROID_HARDWARE_CAMERA_PARAMETERS2_EX_H

#include <camera/CameraParameters2.h>

#ifdef SPRD_FEATURE_BRIGHTNESS
int getSprdBrightNess() const;
void setSprdBrightNess(int brightness);
#endif /* SPRD_FEATURE_BRIGHTNESS */
#ifdef SPRD_FEATURE_3GVT
int getSprdSensorOrient() const;
int getSprdSensorRot() const;
#endif // #ifdef SPRD_FEATURE_3GVT
#ifdef SPRD_FEATURE_BEAUTY
int getSprdPerfectSkinLevel() const;
void setSprdPerfectSkinLevel(int skinlevel);
#endif // #ifdef SPRD_FEATURE_BEAUTY

#ifdef SPRD_FEATURE_ISO
int getSprdISO() const;
void setSprdISO(int iso);
#endif /* SPRD_FEATURE_ISO */

#ifdef SPRD_FEATURE_CONTRAST
int getSprdContrast() const;
void setSprdContrast(int contrast);
#endif

#ifdef SPRD_FEATURE_SATURATION
int getSprdSaturation() const;
void setSprdSaturation(int saturation);
#endif

#ifdef SPRD_FEATURE_METERING_MODE
int getSprdMeteringMode() const;
void setSprdMeteringMode(int metermode);
#endif

#ifdef SPRD_FEATURE_SLOW_MOTION
int getSprdSlowMotion() const;
void setSprdSlowMotion(int slowmotion);
#endif

#ifdef SPRD_FEATURE_EIS
const char *getSprdEOIS() const;
void setSprdEOIS(const char *eois);
#endif

#endif /* ANDROID_HARDWARE_CAMERA_PARAMETERS2_EX_H */
