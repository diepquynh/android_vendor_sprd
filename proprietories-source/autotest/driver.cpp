// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
// add key info 2013-01-22 anli.wei
//
#include <dlfcn.h>
#include <fcntl.h>
#include <cutils/misc.h>
#include <string.h>
#include <cstdlib>

#include "type.h"
#include "driver.h"
#include "autotstdrv.h"

extern "C" int init_module(void *, unsigned long, const char *);
extern "C" int delete_module(const char *, unsigned int);
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_drv {
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
#define DRIVER_PATH  "/lib/modules/autotst.ko"
#define DRIVER_NAME  "autotst"
#define DEV_PATH     "/dev/autotst"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int s_fd      = -1;
static int s_opencnt = 0;
//------------------------------------------------------------------------------
static int insmod(const char *filename, const char *args);
static int rmmod(const char *modname);
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int drvOpen( void )
{
    char cmd[128] = {0};
    if( s_fd >= 0 ) {
	s_opencnt++;
	DBGMSG("already opened: cnt = %d\n", s_opencnt);
	return 0;
    }

    if(0 != access(DRIVER_PATH, F_OK)){
	DBGMSG("the path of autotest drv can`t find %d\n",DRIVER_PATH);
	return -1;
    }
    sprintf(cmd, "insmod %s",DRIVER_PATH);
    system(cmd);
    usleep(100 * 1000);

    s_fd = open(DEV_PATH, O_RDWR);
    if( s_fd < 0 ) {
        ERRMSG("open '%s' error: %s\n", DEV_PATH, strerror(errno));
		rmmod(DRIVER_NAME);
        return -2;
    }

    s_opencnt = 1;
    return 0;
}

//------------------------------------------------------------------------------
int drvI2CRead( uchar bus, uchar addr, uchar reg, uchar *value )
{
    if( s_fd < 0 ) {
        ERRMSG("uninialize!\n");
        return -1;
    }
    
    int        ret = 0;
    autotst_i2c_info_t iit;
    iit.bus      = bus;
    iit.addr     = addr;
    
    iit.reg      = reg;
    iit.regBits  = sizeof(reg) * 8;
    
    iit.data_len = sizeof(value);
    iit.data[0]  = 0;
    iit.data[1]  = 0;
    
    ret = ioctl(s_fd, AUTOTST_IOCTL_I2C_READ, &iit);
    if( ret < 0 ) {
        ERRMSG("ioctl error: %s\n", strerror(errno));
    }
    
    if( 2 == sizeof(value) ) {
        *value = ((iit.data[0] << 8) | iit.data[1]);
    } else {
        *value = iit.data[0];
    }
    DBGMSG("data = 0x%x, 0x%x\n", iit.data[0], iit.data[1]);

    return ret;
}

//------------------------------------------------------------------------------
int drvGIODir( ushort gpio, ushort pull, uchar dir )
{
    if( s_fd < 0 ) {
        ERRMSG("uninialize!\n");
        return -1;
    }
    
    struct autotst_gpio_info_t git;
    
    git.gpio = gpio;
    git.dir  = dir;
    git.val  = 0;
    git.pup_enb  = 0;
    git.pdwn_enb = 0;

      if( pull == 0x00)
	{
	     git.pup_enb  = 0;
	     git.pdwn_enb = 1;
	 }else if(pull == 0x01){
	      git.pup_enb  = 1;
	      git.pdwn_enb = 0;
       }else if(pull == 0x02){
             git.pup_enb  = 0;
	      git.pdwn_enb = 0;
	}
    
    int ret = ioctl(s_fd, AUTOTST_IOCTL_GPIO_INIT, &git);
    if( ret < 0 ) {
        ERRMSG("ioctl error: %s\n", strerror(errno));
    }
    
    return ret;
}

//------------------------------------------------------------------------------
int drvGIOGet( ushort gpio, uchar *val )
{
    if( s_fd < 0 ) {
        ERRMSG("uninialize!\n");
        return -1;
    }
    
    struct autotst_gpio_info_t git;
    
    git.gpio = gpio;
       
    int ret = ioctl(s_fd, AUTOTST_IOCTL_GPIO_GET, &git);
    if( ret < 0 ) {
        ERRMSG("ioctl error: %s\n", strerror(errno));
    }
    *val = git.val;
    
    return ret;
}

//------------------------------------------------------------------------------
int drvGIOSet( ushort gpio, uchar val )
{
    if( s_fd < 0 ) {
        ERRMSG("uninialize!\n");
        return -1;
    }
    
    struct autotst_gpio_info_t git;
    
    git.gpio = gpio;
    git.val  = val;
    
    int ret = ioctl(s_fd, AUTOTST_IOCTL_GPIO_SET, &git);
    if( ret < 0 ) {
        ERRMSG("ioctl error: %s\n", strerror(errno));
    }
        
    return ret;
}


//----------------------------------------------------------------
int drvGIOClose( ushort gpio )
{
    if( s_fd < 0 ) {
        ERRMSG("uninialize!\n");
        return -1;
    }

    struct autotst_gpio_info_t git;

    git.gpio = gpio;

    int ret = ioctl(s_fd, AUTOTST_IOCTL_GPIO_CLOSE, &git);
    if( ret < 0 ) {
        ERRMSG("ioctl error: %s\n", strerror(errno));
    }

    return ret;
}

//------------------------------------------------------------------------------
int drvLcdSendData( uint data )
{
    if( s_fd < 0 ) {
        ERRMSG("uninialize!\n");
        return -1;
    }
    
    int ret = ioctl(s_fd, AUTOTST_IOCTL_LCD_DATA, data);
    if( ret < 0 ) {
        ERRMSG("ioctl error: %s\n", strerror(errno));
    }
        
    return ret;
}
//------------------------------------------------------------------------------
int drvLcd_mipi_on( void )
{
    if( s_fd < 0 ) {
        ERRMSG("uninialize!\n");
        return -1;
    }
    
    int ret = ioctl(s_fd, AUTOTST_IOCTL_LCD_MIPI_ON, 0);
    if( ret < 0 ) {
        ERRMSG("ioctl error: %s\n", strerror(errno));
    }
        
    return ret;
}
//------------------------------------------------------------------------------
int drvLcd_mipi_off( void )
{
    if( s_fd < 0 ) {
        ERRMSG("uninialize!\n");
        return -1;
    }
    
    int ret = ioctl(s_fd, AUTOTST_IOCTL_LCD_MIPI_OFF, 0);
    if( ret < 0 ) {
        ERRMSG("ioctl error: %s\n", strerror(errno));
    }
        
    return ret;
}
//------------------------------------------------------------------------------
int drvGetKpdInfo( ushort val, ushort * row, ushort * col, ushort * gio )
{
	if( s_fd < 0 ) {
        ERRMSG("uninialize!\n");
        return -1;
    }
    
    struct autotst_key_info_t kit;

    kit.val = val;
	kit.row = 0;
	kit.col = 0;
	kit.gio = 0;
    int ret = ioctl(s_fd, AUTOTST_IOCTL_GET_KEYINFO, &kit);
    if( ret < 0 ) {
        ERRMSG("ioctl error: %s\n", strerror(errno));
    }
	if( NULL != row ) {
		//*row = kit.row;
		*row = 0;
		if(val == 115){
			*row = 1;
		}
	}
	if( NULL != col ) {
		//*col = kit.col;
		*col = 0;
	}
	if( NULL != gio ) {
		*gio = kit.gio;
	}
    
    return ret;
}

//------------------------------------------------------------------------------
int drvClose( void )
{
	FUN_ENTER;
	char cmd[128] = {0};
	DBGMSG("open count = %d\n", s_opencnt);

	if( (--s_opencnt) >= 1 ) {
		FUN_EXIT;
		return 0;
	}

    if( s_fd >= 0 ) {
        close(s_fd);
        s_fd = -1;
        
        usleep(100 * 1000);
    }
    if(0 != access(DRIVER_PATH, F_OK)){
	DBGMSG("the path of autotest drv can`t find %d\n",DRIVER_PATH);
	return -1;
    }
    sprintf(cmd, "rmmod %s",DRIVER_NAME);
    system(cmd);
    usleep(100 * 1000);

    //rmmod(DRIVER_NAME);

    FUN_EXIT;
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int insmod(const char *filename, const char *args)
{
    void *module;
    unsigned int size;
    int ret;

    module = load_file(filename, &size);
    if (!module) {
        ERRMSG("load file '%s' fail: %s\n", filename, strerror(errno));
        return -1;
    }

    ret = init_module(module, size, args);
    if( ret < 0 ) {
        ERRMSG("init_module '%s' fail: %s\n", filename, strerror(errno));
        return -2;
    }

    free(module);

    return ret;
}

//------------------------------------------------------------------------------
static int rmmod(const char *modname)
{
    int ret = -1;
    int maxtry = 10;

    while (maxtry-- > 0) {
        ret = delete_module(modname, O_NONBLOCK | O_EXCL);
        if (ret < 0 && errno == EAGAIN)
            usleep(500 * 1000);
        else
            break;
    }

    if (ret != 0)
        ERRMSG("Unable to unload driver module \"%s\": %s\n", modname, strerror(errno));
    return ret;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
