#define LOG_NDEBUG 0
#define LOG_TAG "BootAnimation"

#include "BootAnimationExt.h"
// SPRD: add shutdown animation
#include <system/audio.h>

namespace android {

BootAnimationExt *BootAnimationExt::sInstance = NULL;

BootAnimationExt *BootAnimationExt::Instance() {
    if (!sInstance)
        sInstance = new BootAnimationExt();
    return sInstance;
}

BootAnimationExt::BootAnimationExt() {
    mfd = -1;
}

BootAnimationExt::~BootAnimationExt() {
    // SPRD: add shutdown animation
    if(mfd != -1){ close(mfd); }
}

/* SPRD: add shutdown animation @{ */
bool BootAnimationExt::soundplay()
{
    mp = NULL;

    if(soundpath.length() == 0){
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "sound resource is not right.");
        return false;
    }

    mfd = open(soundpath.string(), O_RDONLY);

    if(mfd == -1){
        __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "boot animation play default source.");
        mfd = open(sound_default_path.string(),O_RDONLY);

        if(mfd == -1){
           __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "can not find bootanimation sound resource....");
           return false;
        }
    }

    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "22222path = %s mfd = %d", soundpath.string(), mfd);
    //AudioSystem::setForceUse((audio_policy_force_use_t)1,(audio_policy_forced_cfg_t)1);// for media,force speaker.

     mp = new MediaPlayer();
     mp->setDataSource(mfd, 0, 0x7ffffffffffffffLL);
     mp->setAudioStreamType(/*AUDIO_STREAM_MUSIC*/AUDIO_STREAM_SYSTEM);
     mp->prepare();
     mp->start();
     return false;
}

bool BootAnimationExt::soundstop()
{
    if (soundpath.length() == 0) {
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "no sound resource ");
    return false;
    }

    if (mp != NULL)mp->stop();
    return false;
}


void BootAnimationExt::setAnimSource(uint32_t paramLen) {

    /* SPRD: add shutdonw animation @{ */
    char srcpath[2][BOOTANIMATION_PATHSET_MAX];

    memset(srcpath[0],0,BOOTANIMATION_PATHSET_MAX);
    memset(srcpath[1],0,BOOTANIMATION_PATHSET_MAX);

    //__android_log_print(ANDROID_LOG_INFO,"BootAnimation", "argc : %d : argc: %s", argc, argv[0]);

    if (paramLen<2){ /* SPRD: if no param ,exe bootanimation, else exe shutdown animation*/
        strncpy(srcpath[0],BOOTANIMATION_BOOT_FILM_PATH_USER,BOOTANIMATION_PATHSET_MAX);
        strncpy(srcpath[1],BOOTANIMATION_BOOT_SOUND_PATH_USER,BOOTANIMATION_PATHSET_MAX);
    } else {
        strncpy(srcpath[0],BOOTANIMATION_SHUTDOWN_FILM_PATH_USER,BOOTANIMATION_PATHSET_MAX);
        strncpy(srcpath[1],BOOTANIMATION_SHUTDOWN_SOUND_PATH_USER,BOOTANIMATION_PATHSET_MAX);
    }

    __android_log_print(ANDROID_LOG_INFO,"BootAnimation", "begin bootanimation!");


    // SPRD: create the boot animation object
    BootAnimationExt *bootaExt = BootAnimationExt::Instance();

    //sp<BootAnimation> boota = new BootAnimation();
    String8 descname("desc.txt");

    if (paramLen<2){
        String8 mpath_default(BOOTANIMATION_BOOT_FILM_PATH_DEFAULT);
        String8 spath_default(BOOTANIMATION_BOOT_SOUND_PATH_DEFAULT);
        bootaExt->setmoviepath_default(mpath_default);
        bootaExt->setsoundpath_default(spath_default);
        bootaExt->setShutdownAnimation(false);
    } else {
        String8 mpath_default(BOOTANIMATION_SHUTDOWN_FILM_PATH_DEFAULT);
        String8 spath_default(BOOTANIMATION_SHUTDOWN_SOUND_PATH_DEFAULT);
        bootaExt->setmoviepath_default(mpath_default);
        bootaExt->setsoundpath_default(spath_default);
        bootaExt->setShutdownAnimation(true);
        __android_log_print(ANDROID_LOG_INFO,"BootAnimation","shutdown exe bootanimation!");
    }

    String8 mpath(srcpath[0]);
    String8 spath(srcpath[1]);

    bootaExt->setmoviepath(mpath);
    bootaExt->setsoundpath(spath);
    bootaExt->setdescname(descname);

    __android_log_print(ANDROID_LOG_INFO,"BootAnimation","%s", mpath.string());

}
bool BootAnimationExt::setsoundpath(String8 path)
{
    //__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "path = %s", path.string());
    soundpath = path;
    return false;
}

bool BootAnimationExt::setmoviepath(String8 path)
{
    //__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "moviepath = %s", path.string());
    moviepath = path;
    return false;
}

bool BootAnimationExt::setdescname(String8 path)
{
    //__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "descname = %s", path.string());
    descname = path;
    return false;
}

bool BootAnimationExt::setsoundpath_default(String8 path)
{
    sound_default_path  = path;
    return false;
}

bool BootAnimationExt::setmoviepath_default(String8 path)
{
    movie_default_path = path;
    return false;
}

bool BootAnimationExt::setdescname_default(String8 path)
{
    descname_default = path;
    return false;
}

void BootAnimationExt::setShutdownAnimation(bool isShutdownAnimation)
{
    mShutdownAnimation = isShutdownAnimation;
}
/* @} */

// ---------------------------------------------------------------------------

};
// namespace android
