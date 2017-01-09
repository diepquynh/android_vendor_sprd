#define LOG_NDEBUG 0
#define LOG_TAG "mplayer"
#include <media/mediaplayer.h>
#include <media/MediaPlayerInterface.h>
#include <cutils/properties.h> // for property_get
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <utils/threads.h>
#include <utils/Log.h>
#include <unistd.h>
#include <time.h>

using namespace android;

#define DEFAULT_PATH_SOUND   "/system/media/bootsound.mp3"

#define CTL_USE_FILE_EX_NAME (1 << 0)

static int binder_ipc(void *cookie)
{
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
    return 0;
}

static void usage(void)
{
    printf("\n"
"mplayer [-l:] [-s:] [-v:] [-e] music_file\n"
"\n"
" -l second (play continued seconds)\n"
" -s msecond (seek to msecond position to play)\n"
" -v volume (range [0-100]\n"
" -e (use the file type extention to decide file type)\n"
"\n"
" setprop sys.mplayer.path music_file_path for music file path\n"
" setprop sys.mplayer.seconds for play continued seconds\n"
" setprop sys.mplayer.msecond for seeking to msecond position to play\n"
" setprop sys.mplayer.volume for set volume\n"
"\n"
);
}

int main(int argc, char *argv[])
{
    int arg;
    time_t stime;
    int ctl = 0;
    int seconds_max = 0;
    int msecond = 0;
    float volume = 100.0f;
    int fd = 0;
    // static struct timeval tmv;/* = {.tv_sec = 0, .tv_usec = 500*1000}; [luther.gliethttp] */
    static char path[512];

    while ((arg = getopt(argc, argv, "l:s:v:e")) != EOF) {
        switch (arg) {
            case 'l': seconds_max = strtol(optarg, NULL, 0); break;
            case 's': msecond = strtol(optarg, NULL, 0); break;
            case 'v': volume = strtol(optarg, NULL, 0); break;
            case 'e': ctl |= CTL_USE_FILE_EX_NAME; break;
            default: usage(); return -1; break;
        }
    }

    property_get("sys.mplayer.seconds", path, "0");
    arg = strtol(path, NULL, 0);
    if (arg != 0 && seconds_max == 0) seconds_max = arg;

    property_get("sys.mplayer.volume", path, "100");
    arg = strtol(path, NULL, 0);
    if (arg != 100 && volume == 100) volume = arg;

    property_get("sys.mplayer.msecond", path, "0");
    arg = strtol(path, NULL, 0);
    if (arg != 0 && msecond == 0) msecond = arg;

    if (argv[optind] != NULL)
        strncpy(path, argv[optind], sizeof path);
    else property_get("sys.mplayer.path", path, DEFAULT_PATH_SOUND);

    if (seconds_max == 0)
        seconds_max = INT_MAX - 50;
    if (volume > 100) volume = 100;
    volume /= 100.0f;

    if ((ctl & CTL_USE_FILE_EX_NAME) == 0) {
        fd = open(path, O_RDONLY);
        if (fd < 0) {
            ALOGE("File <%s> is not found.\n", path);
            return -1;
        }
    }

    sp<MediaPlayer> mp = new MediaPlayer();
    createThreadEtc(binder_ipc, &mp, "mplayer binder ipc");
    mp->reset();
    if (ctl & CTL_USE_FILE_EX_NAME) {
        mp->setDataSource(path, NULL);
    } else {
        mp->setDataSource(fd, 0, INT_MAX);
        close(fd);
    }

    mp->setAudioStreamType(AUDIO_STREAM_ALARM );
    mp->prepare();
    if (msecond) mp->seekTo(msecond);
    mp->setVolume(volume, volume);
    mp->start();
    stime = time(NULL);
    // tmv.tv_sec = 0; tmv.tv_usec = 500*1000;
    while (mp->isPlaying()) {
        sleep(1); // select(0, 0, 0, 0, &tmv);
        if (difftime(time(NULL), stime) > seconds_max)
            break;
    }
    mp->stop();
    mp->disconnect();

    return 0;
}
