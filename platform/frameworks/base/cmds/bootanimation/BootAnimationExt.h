#ifndef ANDROID_BOOTANIMATIONEXT_H
#define ANDROID_BOOTANIMATIONEXT_H

/* SPRD: added boot and shutdown animation @{ */
#include <fcntl.h>
#include <media/mediaplayer.h>
/* @} */

namespace android {
/* SPRD: added boot and shutdown animation ,define  path here @{ */
#define BOOTANIMATION_BOOT_FILM_PATH_DEFAULT    "/system/media/bootanimation.zip"
#define BOOTANIMATION_SHUTDOWN_FILM_PATH_DEFAULT    "/system/media/shutdownanimation.zip"

#define BOOTANIMATION_BOOT_SOUND_PATH_DEFAULT	    "/system/media/bootsound.mp3"
#define BOOTANIMATION_SHUTDOWN_SOUND_PATH_DEFAULT    "/system/media/shutdownsound.mp3"

/* SPRD:added boot and shutdown animation,user path for bug243780 */
#define BOOTANIMATION_BOOT_FILM_PATH_USER       "/data/theme/overlay/bootanimation.zip"
#define BOOTANIMATION_SHUTDOWN_FILM_PATH_USER   "/data/theme/overlay/shutdownanimation.zip"

#define BOOTANIMATION_BOOT_SOUND_PATH_USER      "/data/theme/overlay/bootsound.mp3"
#define BOOTANIMATION_SHUTDOWN_SOUND_PATH_USER  "/data/theme/overlay/shutdownsound.mp3"
#define BOOTANIMATION_PATHSET_MAX    100
/* @} */


// ---------------------------------------------------------------------------

class BootAnimationExt
{
public:
                BootAnimationExt();
    virtual     ~BootAnimationExt();

    /* SPRD: add shutdown animation. @{ */
    bool setsoundpath(String8 path);
    bool setmoviepath(String8 path);
    bool setdescname(String8 path);

    bool setsoundpath_default(String8 path);
    bool setmoviepath_default(String8 path);
    bool setdescname_default(String8 path);
    // SPRD: add for bug 279818, only draw black frame in shutdown animation
    void setShutdownAnimation(bool isShutdownAnimation);
    /* @} */

    void setAnimSource(uint32_t paramLen);
    static BootAnimationExt *sInstance;
    static BootAnimationExt *Instance();

    /* SPRD: added boot and shutdown animation ,next function and param is metioned @{ */
    bool soundplay();
    bool soundstop();
    sp<MediaPlayer> mp;
    String8    soundpath;
    String8    moviepath;
    String8    descname;
    String8    movie_default_path;
    String8    sound_default_path;
    String8    descname_default;
    /* @} */
    // SPRD: add shutdown sound
    int         mfd;
    // SPRD: add for bug 279818, only draw black frame in shutdown animation
    bool        mShutdownAnimation;
};

// ---------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_BOOTANIMATION_H
