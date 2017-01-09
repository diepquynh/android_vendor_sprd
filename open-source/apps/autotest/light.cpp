// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#include <dlfcn.h>
#include <fcntl.h>
#include <stdlib.h>

#include <hardware/hardware.h>
#include <hardware/lights.h>
#include <hardware_legacy/power.h>

#include <sys/stat.h>

#include "type.h"
#include "light.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_light {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
typedef enum LIGHT_IDX_E {
    LIGHT_INDEX_BACKLIGHT = 0,
    LIGHT_INDEX_KEYBOARD,
    LIGHT_INDEX_BUTTONS,
    LIGHT_INDEX_BATTERY,
    LIGHT_INDEX_NOTIFICATIONS = 4,
    LIGHT_INDEX_ATTENTION,
    LIGHT_INDEX_BLUETOOTH,
    LIGHT_INDEX_WIFI,
    LIGHT_COUNT = 8,
} LIGHT_IDX;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static const hw_module_t   * s_hwModule = NULL;
static const light_device_t* s_lightDevs[LIGHT_COUNT];

//------------------------------------------------------------------------------
static light_device_t* get_device(const char * name);
static int             set_light(int idx, int brightness);
extern int set_screen_state(int on);
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int lightOpen( void )
{   
    if( NULL != s_hwModule ) {
        WRNMSG("ligth alread opened!\n");
        return 0;
    }
    
    memset(s_lightDevs, 0, sizeof(s_lightDevs));
    
    int err = hw_get_module(LIGHTS_HARDWARE_MODULE_ID, &s_hwModule);
    if (err == 0) {
        s_lightDevs[LIGHT_INDEX_BACKLIGHT]     = get_device(LIGHT_ID_BACKLIGHT);
        s_lightDevs[LIGHT_INDEX_KEYBOARD]      = get_device(LIGHT_ID_KEYBOARD);
        s_lightDevs[LIGHT_INDEX_BUTTONS]       = get_device(LIGHT_ID_BUTTONS);
        //s_lightDevs[LIGHT_INDEX_BATTERY]       = get_device(LIGHT_ID_BATTERY);
        //s_lightDevs[LIGHT_INDEX_NOTIFICATIONS] = get_device(LIGHT_ID_NOTIFICATIONS);
        //s_lightDevs[LIGHT_INDEX_ATTENTION]     = get_device(LIGHT_ID_ATTENTION);
        //s_lightDevs[LIGHT_INDEX_BLUETOOTH]     = get_device(LIGHT_ID_BLUETOOTH);
        //s_lightDevs[LIGHT_INDEX_WIFI]          = get_device(LIGHT_ID_WIFI);
    } else {
        ERRMSG("hw_get_module('%s') error: %d\n", LIGHTS_HARDWARE_MODULE_ID, err);
        return -1;
    }

	set_screen_state(1);
    
    return 0;
}

int lightSetLCD( int brightness )
{
    if( NULL == s_lightDevs[LIGHT_INDEX_BACKLIGHT] ) {
        INFMSG("No LCD light device\n");
        return -1;
    }
    
    int ret = set_light(LIGHT_INDEX_BACKLIGHT, brightness);
    if( ret ) {
        WRNMSG("set LCD backlight( %d ) Fail(%d)!\n", brightness, ret);
    } else {
        INFMSG("set LCD backlight( %d ) OK.\n", brightness);
    }
    return ret;
}

int lightSetKeyBoard( int brightness )
{
    if( NULL == s_lightDevs[LIGHT_INDEX_KEYBOARD] ) {
        INFMSG("No Keyboard light device\n");
        return -1;
    }
    
    int ret = set_light(LIGHT_INDEX_KEYBOARD, brightness);
    if( ret ) {
        WRNMSG("set Keyborad backlight( %d ) Fail(%d)!\n", brightness, ret);
    } else {
        INFMSG("set Keyborad backlight( %d ) OK.\n", brightness);
    }
    return ret;
}

int lightSetButtons( int brightness )
{
	if( NULL == s_lightDevs[LIGHT_INDEX_BUTTONS] ) {
        INFMSG("No buttons light device\n");
        return -1;
    }
    
    int ret = set_light(LIGHT_INDEX_BUTTONS, brightness);
    if( ret ) {
        WRNMSG("set buttons backlight( %d ) Fail(%d)!\n", brightness, ret);
    } else {
        INFMSG("set buttons backlight( %d ) OK.\n", brightness);
    }
    return ret;
}

int lightSetKeypad( int brightness )
{
	struct stat stt;
	if( stat("/sys/class/leds/keyboard-backlight", &stt) == 0 ) {
		return lightSetButtons(brightness);
	} else {
		return lightSetButtons(brightness);
	}
}

int lightSetRgb(int index , int brightness)
{
	struct stat stt;

	if( !(stat("/sys/class/leds/red", &stt) == 0) ) {
		return -1;
	}
	if( !(stat("/sys/class/leds/green", &stt) == 0) ) {
                return -1;
        }
	if( !(stat("/sys/class/leds/blue", &stt) == 0) ) {
                return -1;
        }

        INFMSG("lightSetRgb index =%d, brightness = %d\n", index, brightness);

	if(index == 0x00){
		if(brightness == 1)
			system("echo 1 > /sys/class/leds/red/on_off");
		else
			system("echo 0 > /sys/class/leds/red/on_off");
	}
	if(index == 0x01){
                if(brightness == 1)
                        system("echo 1 > /sys/class/leds/green/on_off");
                else
                        system("echo 0 > /sys/class/leds/green/on_off");
        }
	if(index == 0x02){
                if(brightness == 1)
                        system("echo 1 > /sys/class/leds/blue/on_off");
                else
                        system("echo 0 > /sys/class/leds/blue/on_off");
        }
        return 0;
}

int lightClose( void )
{
    if( NULL != s_hwModule ) {
        dlclose(s_hwModule->dso);
    }
    s_hwModule = NULL;

    return 0;
}

//==============================================================================
//==============================================================================
light_device_t* get_device(char const* name)
{
    int err;
    hw_device_t* device;
    err = s_hwModule->methods->open(s_hwModule, name, &device);
    if (err == 0) {
        return (light_device_t*)device;
    } else {
        return NULL;
    }
}

int set_light(int idx, int brightness)
{
    if( NULL == s_hwModule ) {
        WRNMSG("light not opened!\n");
        return -1;
    }
    AT_ASSERT( idx < LIGHT_COUNT );
    AT_ASSERT( NULL != s_lightDevs[idx] );
    
	//set_screen_state(brightness ? 1 : 0);

	if(idx == LIGHT_INDEX_BACKLIGHT){
		system("echo 0  > sys/class/leds/keyboard-backlight/brightness");
        WRNMSG("####LIGHT_INDEX_BACKLIGHT off led");
	}else if(idx == LIGHT_INDEX_BUTTONS){
		//system("echo 0  > sys/class/backlight/sprd_backlight/brightness");
        WRNMSG("####LIGHT_INDEX_BUTTONS off lcd");
	}

    int color = brightness & 0x000000ff;
    
    light_state_t state;
    
    state.color      = 0xff000000 | (color << 16) | (color << 8) | color;;
    state.flashMode  = LIGHT_FLASH_NONE;
    state.flashOnMS  = 0;
    state.flashOffMS = 0;
    state.brightnessMode = BRIGHTNESS_MODE_USER;

    return s_lightDevs[idx]->set_light((light_device_t*)s_lightDevs[idx], &state);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
