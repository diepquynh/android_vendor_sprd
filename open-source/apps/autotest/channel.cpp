// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#include <fcntl.h>
#include <sys/poll.h>

#include "type.h"
#include "channel.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_chl {
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int chlOpen( const char * name )
{
    int fd = open(name, O_RDWR);
	if( fd < 0 ) {
		ERRMSG("open '%s' error: %s(%d)\n", name, strerror(errno), errno);
		return -1;
	}
    
/*  // driver not support  
    int flags = fcntl(fd, F_GETFL, 0);
    int ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if( ret < 0 ) {
        ERRMSG("f_setfl error: %s\n", strerror(errno));
    }
*/    
    DBGMSG(" %s exit\n", __FUNCTION__);
    return fd;
}

int chlRead( int fd, uchar * buf, int size, int timeout )
{
    if( fd < 0 ) {
        return -1;
    }
    DBGMSG(" %s enter\n", __FUNCTION__);
    
    AT_ASSERT( buf != NULL && size > 0 );
    
    int ret = 0;
    
    struct pollfd pfd;
    do {        
/*      // !! driver not support these mechenism !!
        pfd.fd     = fd;
        pfd.events = POLLIN;
        errno = 0;
        ret   = poll(&pfd, 1, timeout);
        if (ret < 0) {
            ERRMSG("poll() error: %s\n", strerror(errno));
            break;
        } else if( 0 == ret ) {
            CHL_INF("poll() timeout: %d ms\n", timeout);
            break;
        }
        
        if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
            ERRMSG("poll() returned  success (%d), "
                 "but with an unexpected revents bitmask: %#x\n", ret, pfd.revents);
            ret = -1;
            break;
        }
        DBGMSG("after poll\n");
*/        
        ret = read(fd, buf, size);
        //DBGMSG("read %d bytes\n", ret);
    } while( 0 );
        
    DBGMSG(" %s exit\n", __FUNCTION__);
    return ret;
}

int chlWrite( int fd, uchar * buf, int len )
{
    if( fd < 0 ) {
        return -1;
    }
    
    int cnt = 0;
    uchar * pwb = buf;
    do{ 
        int wrt = write(fd, pwb, len); 
        if( wrt < 0 ) {
            ERRMSG("write error: %s(%d), written = %d\n", strerror(errno), errno, cnt);
            break;
        }
        cnt += wrt;
        pwb += wrt;
        len -= wrt;
    } while( len );
    
    return cnt;
}

void chlClose( int fd )
{
    DBGMSG(" %s enter\n", __FUNCTION__);
    if( fd >= 0 ) {
        close(fd);
    }
    DBGMSG(" %s exit\n", __FUNCTION__);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
