#ifdef SPRD_FRAMEWORKS_CAMERA_EX
#include "api1/client2/Parameters.h"
#include <SprdCamera3Tags.h>

namespace android{
namespace camera2{

status_t initializeEx(Parameters *p)
{
    if (NULL == p)
        return BAD_VALUE;
#ifdef SPRD_FEATURE_BRIGHTNESS
    camera_metadata_ro_entry_t availableBrightNess = p->staticInfo(ANDROID_SPRD_AVAILABLE_BRIGHTNESS , 0 , 0 , false);
    if (!availableBrightNess.count)
        return NO_INIT;

    p->brightness = availableBrightNess.data.u8[availableBrightNess.count / 2];
    p->params.setSprdBrightNess(p->brightness);
    String8 supportedBrightness;
    for (size_t i=0; i < availableBrightNess.count; i++)
    {
        if (i != 0) supportedBrightness += ",";
        supportedBrightness += String8::format("%d" , availableBrightNess.data.u8[i]);
    }
    p->params.set(CameraParameters::KEY_SUPPORTED_BRIGHTNESS, supportedBrightness);
#endif // #ifdef SPRD_FEATURE_BRIGHTNESS

#ifdef SPRD_FEATURE_ISO
    String8 supportedISO;
    camera_metadata_ro_entry_t availableISO = p->staticInfo(ANDROID_SPRD_AVAILABLE_ISO , 0 , 0 , false);
    if (availableISO.count)
    {
        /*1 means "auto"*/
        p->iso = availableISO.data.u8[0];
        p->params.setSprdISO(p->iso);
        for (size_t i=0; i < availableISO.count; i++)
        {
            if(i == 0)
                supportedISO += "auto";
            else
            {
                supportedISO += ",";
                supportedISO += String8::format("%d", (1 << (availableISO.data.u8[i] - 1)) * 100);
            }
        }
        p->params.set(CameraParameters::KEY_SUPPORTED_ISO, supportedISO);
    }
#endif // #ifdef SPRD_FEATURE_ISO

#ifdef SPRD_FEATURE_CONTRAST
    String8 supportedContrast;
    camera_metadata_ro_entry_t availableContrast = p->staticInfo(ANDROID_SPRD_AVAILABLE_CONTRAST , 0 , 0 , false);
    if (!availableContrast.count)
        return NO_INIT;
    p->contrast = availableContrast.data.u8[availableContrast.count / 2];
    p->params.setSprdContrast(p->contrast);
    for(size_t i = 0 ; i < availableContrast.count ; i++)
    {
        if(i != 0)
            supportedContrast += ",";
        supportedContrast += String8::format("%d", availableContrast.data.u8[i]);
    }
    p->params.set(CameraParameters::KEY_SUPPORTED_CONTRAST, supportedContrast);
#endif // #ifdef SPRD_FEATURE_CONTRAST

#ifdef SPRD_FEATURE_SATURATION
    String8 supportedSaturation;
    camera_metadata_ro_entry_t availableSaturation = p->staticInfo(ANDROID_SPRD_AVAILABLE_SATURATION , 0 , 0 , false);
    if(!availableSaturation.count)
        return NO_INIT;
    p->saturation = availableSaturation.data.u8[availableSaturation.count / 2];
    p->params.setSprdSaturation(p->saturation);

    for (size_t i=0; i < availableSaturation.count; i++)
    {
        if (i != 0)
            supportedSaturation += ",";
        supportedSaturation += String8::format("%d", availableSaturation.data.u8[i]);
    }
    p->params.set(CameraParameters::KEY_SUPPORTED_SATURATION, supportedSaturation);
#endif // #ifdef SPRD_FEATURE_SATURATION

#ifdef SPRD_FEATURE_METERING_MODE
    String8 supportedMeterMode;
    const char *str[4] = {"frame-average" , "center-weighted" , "spot-metering" , NULL};
    camera_metadata_ro_entry_t availableMeterMode = p->staticInfo(ANDROID_SPRD_AVAILABLE_METERING_MODE , 0 , 0 , false);
    if(!availableMeterMode.count)
        return NO_INIT;
    p->meteringmode = availableMeterMode.data.u8[0];
    p->params.setSprdMeteringMode(p->meteringmode);

    for (size_t i=0; i < availableMeterMode.count; i++)
    {
        if (i != 0)
            supportedMeterMode += ",";
        supportedMeterMode += String8::format("%s", str[availableMeterMode.data.u8[i]]);
    }
    p->params.set(CameraParameters::KEY_SUPPORTED_METERING_MODE, supportedMeterMode);
#endif // #ifdef SPRD_FEATURE_METERING_MODE

#ifdef SPRD_FEATURE_SLOW_MOTION
    String8 supportedSlowMotion;
    camera_metadata_ro_entry_t availableSlowMotion = p->staticInfo(ANDROID_SPRD_AVAILABLE_SLOW_MOTION , 0 , 0 , false);
    if(!availableSlowMotion.count)
        return NO_INIT;
    p->slowmotion = availableSlowMotion.data.u8[0];
    p->params.setSprdSlowMotion(p->slowmotion);

    for (size_t i=0; i < availableSlowMotion.count; i++)
    {
        if (i != 0)
            supportedSlowMotion += ",";
        supportedSlowMotion += String8::format("%d", availableSlowMotion.data.u8[i]);
    }
    p->params.set(CameraParameters::KEY_SUPPORTED_SLOWMOTION, supportedSlowMotion);
#endif // #ifdef SPRD_FEATURE_SLOW_MOTION

    return OK;
}

status_t setEx(Parameters *p , Parameters *validatedParams , CameraParameters2 *newParams)
{
    if (NULL == p || NULL == validatedParams || NULL == newParams)
        return BAD_VALUE;
#ifdef SPRD_FEATURE_BRIGHTNESS
    size_t i = 0;
    validatedParams->brightness = newParams->getSprdBrightNess();
    if(validatedParams->brightness != p->brightness)
    {
        camera_metadata_ro_entry_t availableBrightness = p->staticInfo(ANDROID_SPRD_AVAILABLE_BRIGHTNESS);
        for (i = 0; i < availableBrightness.count; i++)
        {
            if (availableBrightness.data.u8[i] == validatedParams->brightness)
                break;
        }
        if (i == availableBrightness.count) {
            ALOGE("%s: Requested brightness %d is not supported" , __FUNCTION__ , validatedParams->brightness);
            return BAD_VALUE;
        }
    }
#endif // #ifdef SPRD_FEATURE_BRIGHTNESS

#ifdef SPRD_FEATURE_ISO
    validatedParams->iso = newParams->getSprdISO();
    if(validatedParams->iso != p->iso)
    {
        camera_metadata_ro_entry_t availableISO = p->staticInfo(ANDROID_SPRD_AVAILABLE_ISO , 0 , 0 , false);
        for(i = 0 ; i < availableISO.count ; i++)
        {
            if(availableISO.data.u8[i] == validatedParams->iso)
                break;
        }
    }
#endif // #ifdef SPRD_FEATURE_ISO

#ifdef SPRD_FEATURE_CONTRAST
    validatedParams->contrast = newParams->getSprdContrast();
    if(validatedParams->contrast != p->contrast)
    {
        camera_metadata_ro_entry_t availableContrast = p->staticInfo(ANDROID_SPRD_AVAILABLE_CONTRAST);
        for(i = 0 ; i < availableContrast.count ; i++)
        {
            if(availableContrast.data.u8[i] == validatedParams->contrast)
                break;
        }
        if(i == availableContrast.count)
        {
            ALOGE("%s: Requested contrast %d is not supported", __FUNCTION__, validatedParams->contrast);
            return BAD_VALUE;
        }
    }
#endif // #ifdef SPRD_FEATURE_CONTRAST

#ifdef SPRD_FEATURE_SATURATION
    validatedParams->saturation = newParams->getSprdSaturation();
    if(validatedParams->saturation != p->saturation)
    {
        camera_metadata_ro_entry_t availableSaturation = p->staticInfo(ANDROID_SPRD_AVAILABLE_SATURATION);
        for(i = 0; i < availableSaturation.count; i++)
        {
            if(availableSaturation.data.u8[i] == validatedParams->saturation)
                break;
        }
        if(i == availableSaturation.count)
        {
            ALOGE("%s: Requested saturation %d is not supported", __FUNCTION__, validatedParams->saturation);
            return BAD_VALUE;
        }
    }
#endif // #ifdef SPRD_FEATURE_SATURATION

#ifdef SPRD_FEATURE_METERING_MODE
    validatedParams->meteringmode = newParams->getSprdMeteringMode();
    if(validatedParams->meteringmode != p->meteringmode)
    {
        camera_metadata_ro_entry_t availableMeterMode = p->staticInfo(ANDROID_SPRD_AVAILABLE_METERING_MODE);
        for(i = 0; i < availableMeterMode.count; i++)
        {
            if(availableMeterMode.data.u8[i] == validatedParams->meteringmode)
                break;
        }
        if(i == availableMeterMode.count)
        {
            ALOGE("%s: Requested meteringmode %d is not supported", __FUNCTION__, validatedParams->meteringmode);
            return BAD_VALUE;
        }
    }
#endif // #ifdef SPRD_FEATURE_METERING_MODE

#ifdef SPRD_FEATURE_SLOW_MOTION
    validatedParams->slowmotion = newParams->getSprdSlowMotion();
    if(validatedParams->slowmotion != p->slowmotion)
    {
        camera_metadata_ro_entry_t availableSlowmotion = p->staticInfo(ANDROID_SPRD_AVAILABLE_SLOW_MOTION);
        for(i = 0; i < availableSlowmotion.count; i++)
        {
            if(availableSlowmotion.data.u8[i] == validatedParams->slowmotion)
                break;
        }
        if(i == availableSlowmotion.count)
        {
            ALOGE("%s: Requested slowmotion %d is not supported", __FUNCTION__, validatedParams->slowmotion);
            return BAD_VALUE;
        }
    }
#endif // #ifdef SPRD_FEATURE_SLOW_MOTION

#ifdef SPRD_FEATURE_3GVT
    validatedParams->sensorRot = newParams->getSprdSensorRot();
    validatedParams->sensorOrient = newParams->getSprdSensorOrient();
#endif // #ifdef SPRD_FEATURE_3GVT
#ifdef SPRD_FEATURE_Beauty
    validatedParams->perfectSkinlevel = newParams->getSprdPerfectSkinLevel();
#endif // #ifdef SPRD_FEATURE_Beauty
    return OK;
}

status_t updateRequestEx(const Parameters *p , CameraMetadata *request)
{
    if (NULL == p || NULL == request)
        return BAD_VALUE;
    status_t res = OK;
#ifdef SPRD_FEATURE_BRIGHTNESS
    uint8_t reqBrightness = 0;
    if (p->brightness >= 0 && p->brightness < 256)
    {
        reqBrightness = p->brightness;
        res = request->update(ANDROID_SPRD_BRIGHTNESS , &reqBrightness , 1);
        if (res != OK)
            return res;
    }
    else
    {
        ALOGE("updatereq bright too big=%d" , p->brightness);
        return BAD_VALUE;
    }
#endif // #ifdef SPRD_FEATURE_BRIGHTNESS

#ifdef SPRD_FEATURE_ISO
    uint8_t reqISO = 0;
    if(p->iso >= 0 && p->iso < 10)
    {
        reqISO = p->iso;
        res = request->update(ANDROID_SPRD_ISO , &reqISO , 1);
        if(res != OK)
            return res;
    }
#endif // #ifdef SPRD_FEATURE_ISO

#ifdef SPRD_FEATURE_CONTRAST
    uint8_t reqContrast = 0;
    if(p->contrast > -10 && p->contrast < 10)
    {
        reqContrast = p->contrast;
        res = request->update(ANDROID_SPRD_CONTRAST , &reqContrast , 1);
        if(res != OK)
            return res;
    }
    else
    {
        ALOGE("updatereq contrast too big=%d", p->contrast);
        return BAD_VALUE;
    }
#endif // #ifdef SPRD_FEATURE_CONTRAST

#ifdef SPRD_FEATURE_SATURATION
    uint8_t reqSaturation = 0;
    if(p->saturation > -10 && p->saturation < 10)
    {
        reqSaturation = p->saturation;
        res = request->update(ANDROID_SPRD_SATURATION , &reqSaturation , 1);
        if(res != OK)
            return res;
    }
    else
    {
        ALOGE("updatereq saturation too big=%d", p->saturation);
        return BAD_VALUE;
    }
#endif // #ifdef SPRD_FEATURE_SATURATION

#ifdef SPRD_FEATURE_METERING_MODE
    uint8_t reqMeterMode = 0;
    if(p->meteringmode >= 0 && p->meteringmode < 10)
    {
        reqMeterMode = p->meteringmode;
        res = request->update(ANDROID_SPRD_METERING_MODE, &reqMeterMode , 1);
        if(res != OK)
            return res;
    }
    else
    {
        ALOGE("updatereq meteringmode too big=%d", p->meteringmode);
        return BAD_VALUE;
    }
#endif // #ifdef SPRD_FEATURE_METERING_MODE

#ifdef SPRD_FEATURE_SLOW_MOTION
    uint8_t reqSlowmotion = 1;
    if(p->slowmotion >= 0 && p->slowmotion < 10)
    {
        reqSlowmotion = p->slowmotion;
        res = request->update(ANDROID_SPRD_SLOW_MOTION, &reqSlowmotion , 1);
        if(res != OK)
            return res;
    }
    else
    {
        ALOGE("updatereq slowmotion too big=%d", p->slowmotion);
        return BAD_VALUE;
    }
#endif // #ifdef SPRD_FEATURE_SLOW_MOTION

#ifdef SPRD_FEATURE_3GVT
    int32_t reqSensorRot = p->sensorRot;
    //reqSensorRot = 270;

    ALOGI("sensor rot=%d", reqSensorRot);
    res = request->update(ANDROID_SPRD_SENSOR_ROTATION,
        &reqSensorRot, 1);
    if (res != OK) return res;
    uint8_t reqSensorOrient = 0;
    if (p->sensorOrient > -10 && p->sensorOrient < 10) {
        reqSensorOrient = p->sensorOrient;
        //if (reqSensorOrient == 255)
        //reqSensorOrient = 1;
        res = request->update(ANDROID_SPRD_SENSOR_ORIENTATION,
                &reqSensorOrient, 1);
        if (res != OK) return res;
    } else {
        ALOGE("updatereq sensorOrient too big=%d", p->sensorOrient);
       // return BAD_VALUE;
    }
#endif // #ifdef SPRD_FEATURE_3GVT
#ifdef SPRD_FEATURE_BEAUTY
    {
        int32_t reqPerfectskinlevel = p->perfectSkinlevel;
        ALOGI("perfect skin level = %d", reqPerfectskinlevel);
        res = request->update(ANDROID_SPRD_UCAM_SKIN_LEVEL,
        &reqPerfectskinlevel, 1);
        if (res!=OK) return res;
    }
#endif // #ifdef SPRD_FEATURE_BEAUTY
    return res;
}

};//namespace camera2
};//namespace android

#endif //
