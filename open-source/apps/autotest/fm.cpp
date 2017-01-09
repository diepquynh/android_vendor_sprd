// 
// Spreadtrum Auto Tester
//
// anli   2012-11-29
//
#include <dlfcn.h>
#include <fcntl.h>
#include <string.h>

#include <hardware/fm.h>
//#include <media/AudioSystem.h>

#include <media/AudioRecord.h>
#include <media/AudioSystem.h>
#include <media/AudioTrack.h>
#include <media/mediarecorder.h>
#include <system/audio.h>
#include <system/audio_policy.h>

#include "type.h"
#include "fm.h"
#include "perm.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_fm {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
using namespace android;
using namespace at_perm;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
#define FM_DEV_NAME "/dev/fm"

/*  seek direction */
#define FM_SEEK_UP 0
#define FM_SEEK_DOWN 1
#define FM_RSSI_MIN 105

extern int SendAudioTestCmd(const uchar * cmd,int bytes);
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
static hw_module_t * s_hwModule = NULL;
static fm_device_t * s_hwDev    = NULL;

int fd = -1;

struct fm_tune_parm {
    uchar err;
    uchar band;
    uchar space;
    uchar hilo;
    uint freq;
};

struct fm_seek_parm {
    uchar err;
    uchar band;
    uchar space;
    uchar hilo;
    uchar seekdir;
    uchar seekth;
    ushort freq;
};

struct fm_check_status {
    uchar status;
    int rssi;
    uint freq;
};

#define FM_IOC_MAGIC        0xf5
#define FM_IOCTL_POWERUP       _IOWR(FM_IOC_MAGIC, 0, struct fm_tune_parm*)
#define FM_IOCTL_POWERDOWN     _IOWR(FM_IOC_MAGIC, 1, long*)
#define FM_IOCTL_TUNE          _IOWR(FM_IOC_MAGIC, 2, struct fm_tune_parm*)
#define FM_IOCTL_SEEK          _IOWR(FM_IOC_MAGIC, 3, struct fm_seek_parm*)
#define FM_IOCTL_GETRSSI       _IOWR(FM_IOC_MAGIC, 7, long*)
#define FM_IOCTL_CHECK_STATUS  _IOWR(FM_IOC_MAGIC, 71, struct fm_check_status*)

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int fmOpen( void )
{
	if( NULL != s_hwDev ) {
		WRNMSG("already opened.\n");
		return 0;
	}

    int err = hw_get_module(FM_HARDWARE_MODULE_ID, (hw_module_t const**)&s_hwModule);
    if( err || NULL == s_hwModule ) {
		ERRMSG("hw_get_module: err = %d\n", err);
		return ((err > 0) ? -err : err);
	}

	err = s_hwModule->methods->open(s_hwModule, FM_HARDWARE_MODULE_ID,
					(hw_device_t**)&s_hwDev);
    if( err || NULL == s_hwDev ) {
		ERRMSG("open err = %d!\n", err);
		return ((err > 0) ? -err : err);
	}

    {
        char  cmd_buf[100] ={0};
        sprintf(cmd_buf, "autotest_fmtest=1");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
    }
    return 0;
}

//
#define V4L2_CID_PRIVATE_BASE           0x8000000
#define V4L2_CID_PRIVATE_TAVARUA_STATE  (V4L2_CID_PRIVATE_BASE + 4)

#define V4L2_CTRL_CLASS_USER            0x980000
#define V4L2_CID_BASE                   (V4L2_CTRL_CLASS_USER | 0x900)
#define V4L2_CID_AUDIO_VOLUME           (V4L2_CID_BASE + 5)
#define V4L2_CID_AUDIO_MUTE             (V4L2_CID_BASE + 9)

//------------------------------------------------------------------------------
int fmPlay( uint freq )
{
    if( NULL == s_hwDev ) {
        ERRMSG("not opened!\n");
        return -1;
    }

	status_t   status;
    String8 fm_mute("FM_Volume=0");
    String8 fm_volume("FM_Volume=11");
#if 0
	AudioTrack atrk;

	atrk.set(AUDIO_STREAM_FM, 44100, AUDIO_FORMAT_PCM_16_BIT, AUDIO_CHANNEL_OUT_STEREO);
	atrk.start();

	AudioTrack::Buffer buffer;
	buffer.frameCount = 64;
	status = atrk.obtainBuffer(&buffer, 1);
    if (status == NO_ERROR) {
		memset(buffer.i8, 0, buffer.size);
		atrk.releaseBuffer(&buffer);
	} else if (status != TIMED_OUT && status != WOULD_BLOCK) {
		ERRMSG("cannot write to AudioTrack: status = %d\n", status);
	}
	atrk.stop();
#endif
	int ret = s_hwDev->setControl(s_hwDev, V4L2_CID_PRIVATE_TAVARUA_STATE, 1);
	//DBGMSG("setControl:  TAVARUA = %d\n", ret);
	usleep(20 * 1000);

    uint volume = 12; // max  [0 - 15]
    ret = s_hwDev->setControl(s_hwDev, V4L2_CID_AUDIO_VOLUME, volume);
	//DBGMSG("setControl:  VOLUME = %d\n", ret);

	ret = s_hwDev->setFreq(s_hwDev, freq);
    if( ret < 0 ) {
        ERRMSG("ioctl error: %s\n", strerror(errno));
        return ret;
    }
    {
        char  cmd_buf[100] ={0};
        sprintf(cmd_buf, "autotest_fmtest=2");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
    }

    return 0;
}

//------------------------------------------------------------------------------
int fmStop( void )
{
    if( NULL != s_hwDev ) {
        char  cmd_buf[100] ={0};
        sprintf(cmd_buf, "autotest_fmtest=0");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
        s_hwDev->setControl(s_hwDev, V4L2_CID_PRIVATE_TAVARUA_STATE, 0);
    }

    return 0;
}

int fmClose( void )
{
	if( NULL != s_hwDev && NULL != s_hwDev->common.close ) {
		s_hwDev->common.close( &(s_hwDev->common) );
	}
	s_hwDev = NULL;

    if( NULL != s_hwModule ) {
        dlclose(s_hwModule->dso);
		s_hwModule = NULL;
    }

	return 0;
}

#if 0
int fmCheckStatus(uchar *fm_status )
{
    if( NULL == s_hwDev ) {
	    ERRMSG("not opened!\n");
	    return -1;
    }

	int ret = s_hwDev->setControl(s_hwDev, V4L2_CID_PRIVATE_TAVARUA_STATE, 1);
	//DBGMSG("setControl:  TAVARUA = %d\n", ret);
	usleep(20 * 1000);

	ret = s_hwDev->checkStatus(s_hwDev, fm_status);

    if( ret < 0 ) {
        ERRMSG("ioctl error: %s  ret %d  fm_status %d\n", strerror(errno), ret, fm_status);
        return ret;
    }

	return 0;
}
#endif

/* Below Radio_ functions is for google fm apk in android 6.0 and upon */

int Radio_Open( uint freq )
{
    int ret = -1;
    struct fm_tune_parm parm = {0};

    fd = open(FM_DEV_NAME, O_RDWR);
    if (fd < 0) {
        ERRMSG("open err! fd = %d\n", fd);
        return -1;
    }

    if ( freq < 875 || freq > 1080 ) {
        ERRMSG("invalid freq! freq = %d\n", freq);
        return -1;
    } else {
        parm.freq = freq;
    }

    ret = ioctl(fd, FM_IOCTL_POWERUP, &parm);
    if (ret) {
        ERRMSG("power up err! ret = %d\n", ret);
        return ret;
    }

    {
        char  cmd_buf[100] ={0};
        sprintf(cmd_buf, "test_stream_route=4");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
    }

    return ret;
}

int Radio_Play( uint freq )
{
    int ret = -1;
    struct fm_tune_parm parm = {0};

    parm.freq = freq;
    ret = ioctl(fd, FM_IOCTL_TUNE, &parm);
    if (ret) {
        ERRMSG("tune err! freq = %d, ret = %d\n", parm.freq, ret);
        return ret;
    }

    {
        char  cmd_buf[100] ={0};
        sprintf(cmd_buf, "handleFm=1");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
        sprintf(cmd_buf, "connect=16777216;FM_Volume=11");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
    }

    return ret;
}

int Radio_Tune( uint freq )
{
    int ret = -1;
    struct fm_tune_parm parm = {0};

    parm.freq = freq;
    ret = ioctl(fd, FM_IOCTL_TUNE, &parm);
    if (ret) {
        ERRMSG("tune err! freq = %d, ret = %d\n", parm.freq, ret);
        return ret;
    }

    return ret;
}

int Radio_Seek( uint *freq )
{
    int ret = -1;
    struct fm_seek_parm parm = {0};

    parm.freq = *freq;
    //seekdir = 1 mean seek from 875 to 1080
    parm.seekdir = FM_SEEK_DOWN;
    ret = ioctl(fd, FM_IOCTL_SEEK, &parm);
    if (ret) {
        ERRMSG("tune err! freq = %d, ret = %d\n", parm.freq, ret);
        return ret;
    } else {
        *freq = parm.freq;
        return ret;
    }
}

int Radio_Close( void )
{
    int ret = -1;
    int type = 0;

    {
        char  cmd_buf[100] ={0};
        sprintf(cmd_buf, "handleFm=0");
        SendAudioTestCmd((const uchar*)cmd_buf,sizeof(cmd_buf));
    }

    ret = ioctl(fd, FM_IOCTL_POWERDOWN, &type);
    if (ret) {
        ERRMSG("power down err! ret = %d\n", ret);
        return ret;
    }

    ret = close(fd);
    if (ret) {
        ERRMSG("close err! ret = %d\n", ret);
        return ret;
    }

    return ret;
}

int Radio_CheckStatus(uchar *fm_status, uint *fm_freq, int *fm_rssi)
{
    int ret = -1;
    struct fm_check_status parm = {0};

    if (fd < 0) {
        ERRMSG("open err! fd = %d\n", fd);
        return -1;
    }
    ret = ioctl(fd, FM_IOCTL_CHECK_STATUS, &parm);
    if (ret) {
        ERRMSG("get status err! ret = %d\n", ret);
        return ret;
    }

    *fm_status = parm.status;
    *fm_rssi = parm.rssi;
    *fm_freq = (unsigned int)parm.freq;

    return ret;
}

int Radio_GetRssi(int *fm_rssi)
{
    int ret = -1;

    if (fd < 0) {
        ERRMSG("open err! fd = %d\n", fd);
        return -1;
    }
    ret = ioctl(fd, FM_IOCTL_GETRSSI, fm_rssi);
    if (ret) {
        ERRMSG("get rssi err! ret = %d\n", ret);
        return ret;
    }

    return ret;
}

int Radio_Seek_Channels(uchar *fm_status, uint *start_freq, int *fm_rssi)
{
    int ret = 0, Num = 0;
    struct fm_check_status cur_freq;
    unsigned int LastValidFreq = 0;

    memset(&cur_freq, 0, sizeof(fm_check_status));
    cur_freq.freq = *start_freq;

    while(LastValidFreq < cur_freq.freq) {
        LastValidFreq = cur_freq.freq;
        cur_freq.freq += 1;

        ret = Radio_Seek(&cur_freq.freq);
        LOGI("seek channels, ret: %d, current freq: %d, last freq: %d\n",
            ret, cur_freq.freq, LastValidFreq);
        if (0 != ret || LastValidFreq >= cur_freq.freq) {
            LOGE("seek channels stop: [%d]\n", ret);
            continue;
        }

        Radio_Tune(cur_freq.freq);
#ifndef SPRD_WCNBT_MARLIN //for 2351
        ret = Radio_CheckStatus(&cur_freq.status, &cur_freq.freq, &cur_freq.rssi);
#else //for marlin
        ret = Radio_GetRssi(&cur_freq.rssi);
#endif
        if (!ret) {
            if(cur_freq.rssi <= FM_RSSI_MIN) {
                LOGI("seek channels success: [%d]: freq:%d rssi:%d\n", Num++, cur_freq.freq, cur_freq.rssi);
                cur_freq.status = 0;
                break;
            } else {
                LOGI("Num++: [%d]: freq:%d rssi:%d\n", Num++, cur_freq.freq, cur_freq.rssi);
            }
        }
    }

    *fm_status = cur_freq.status;
    *start_freq = cur_freq.freq;
    *fm_rssi = cur_freq.rssi;

    if ((Num == 0) || (cur_freq.rssi > FM_RSSI_MIN)) {
        *fm_status = 1;
        LOGI("status: %d freq: %d rssi: %d\n", cur_freq.status, cur_freq.freq, cur_freq.rssi);
        return -1;
    }

    return 0;
}

int Radio_SKD_Test(uchar *fm_status, uint *fm_freq, int *fm_rssi)
{
    int ret = 0;

#ifdef GOOGLE_FM_INCLUDED //for android 6 and upon platform Using google original apk
    if( Radio_Open(*fm_freq) < 0 || Radio_Tune(*fm_freq) < 0 ) {
        goto FAIL;
    } else {
        ret = Radio_Seek_Channels(fm_status, fm_freq, fm_rssi);
        if (!ret) {
            ret = Radio_Play(*fm_freq);
            *fm_status = 0;
        } else {
            goto FAIL;
        }
    }
    if( Radio_Close() < 0 ) {
        goto FAIL;
    }
#else
#if 0
#ifndef SPRD_WCNBT_MARLIN
    if( fmOpen() < 0 || fmPlay(freq) < 0 ) {
        ret = -1;
    }
    fmCheckStatus(&skd_fm_status_r);
    rsp[0]= skd_fm_status_r;
    fmStop();
    fmClose();
#endif
#endif
#endif

    return ret;

FAIL:

    *fm_status = 1;
    return -1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
