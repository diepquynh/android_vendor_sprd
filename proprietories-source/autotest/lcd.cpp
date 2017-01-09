// 
// Spreadtrum Auto Tester
//
// anli   2012-11-26
//
#include <fcntl.h>
#include <linux/fb.h>
#include <hardware_legacy/power.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "type.h"
#include "lcd.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_lcd {
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
static const char FB_DEV_PATH[] = "/dev/graphics/fb0";
static int        sFBFD         = -1;
static void     * sLCDBuffer    = NULL;
static uint       sLCDBufLen    = 0;
static void     * sFrameBuf[2];
//------------------------------------------------------------------------------
inline uint roundUpToPageSize(uint x) {
        return (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int lcdOpen( void )
{
    if( sFBFD >= 0 ) {
        WRNMSG("already opened!\n");
        return 0;
    }
    
    sFBFD = open(FB_DEV_PATH, O_RDWR);
    if( sFBFD < 0 ) {
        ERRMSG("open '%s' error: %s\n", FB_DEV_PATH, strerror(errno));
        return sFBFD;
    }
    int ret = 0;
    struct fb_var_screeninfo info;
    struct fb_fix_screeninfo finfo;
    do {
        if( ioctl(sFBFD, FBIOGET_VSCREENINFO, &info) < 0 ) {
            ERRMSG("Failed to get screen info: %s\n", strerror(errno));
            ret = -1;
            break;
        }
        DBGMSG("Screen: x = %d, xo = %d, xv = %d\n", info.xres,
            info.xoffset, info.xres_virtual );
        DBGMSG("Screen: y = %d, yo = %d, yv = %d\n", info.yres,
            info.yoffset, info.yres_virtual );            
        DBGMSG("Screen: bpp = %d\n", info.bits_per_pixel);
        
        // map buffer
        if( ioctl(sFBFD, FBIOGET_FSCREENINFO, &finfo) < 0 ) {
            ERRMSG("Failed to get screen info: %s\n", strerror(errno));
            ret = -2;
            break;
        }
        
        sLCDBufLen = roundUpToPageSize(finfo.smem_len);
        sLCDBuffer = mmap(0, sLCDBufLen, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, sFBFD, 0);
        if( sLCDBuffer == MAP_FAILED ) {
            ERRMSG("Failed to map screen\n");
            ret = -3;
            break;
        }

        sFrameBuf[0] = sLCDBuffer;
        sFrameBuf[1] = ((uchar *)sLCDBuffer) + info.xres * info.yres * (info.bits_per_pixel >> 3);

        DBGMSG("LCD Buffer Len = %d\n", sLCDBufLen);
        DBGMSG("FB0 = %p, FB1 = %p\n", sFrameBuf[0], sFrameBuf[1]);
    } while( 0 );
    
    return ret;
}

int lcdDrawBackground( uint rgbValue )
{
    struct fb_var_screeninfo info;
    
    if( ioctl(sFBFD, FBIOGET_VSCREENINFO, &info) < 0 ) {
        ERRMSG("get vscreeninfo fail: %s\n", strerror(errno));
        return -1;
    }
    
    DBGMSG("Screen: x = %d, xo = %d, xv = %d\n", info.xres,
            info.xoffset, info.xres_virtual );
    DBGMSG("Screen: y = %d, yo = %d, yv = %d\n", info.yres,
            info.yoffset, info.yres_virtual );  
    
    void * curFB;
    if( 0 == info.yoffset ) {
        info.yoffset      = info.yres;
        //info.yres_virtual = info.yres * 2;
        
        curFB = sFrameBuf[1];
    } else {
        info.yoffset = 0;
        //info.yres_virtual = info.yres;
        curFB = sFrameBuf[0];
    }
   
    info.xoffset      = 0;
    info.yoffset      = 0;
    //memset(curFB, (uchar)rgbValue, info.xres * info.yres * info.bits_per_pixel / 8);
    memset(sLCDBuffer, (uchar)rgbValue, sLCDBufLen);
    
    info.activate = FB_ACTIVATE_FORCE;

    if (ioctl(sFBFD, FBIOPUT_VSCREENINFO, &info) < 0) {
        ERRMSG("set vscreeninfo fail: %s\n", strerror(errno));
        //return -2;
    }
    
    return 0;
}

int lcdClose( void )
{
    if( NULL != sLCDBuffer ) {
        munmap(sLCDBuffer, sLCDBufLen);
        sLCDBuffer = NULL;
    }
    if( sFBFD >= 0 ) {
        close(sFBFD);
        sFBFD = -1;
    }
    return 0;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
