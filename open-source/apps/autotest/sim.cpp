// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#include <fcntl.h>
#include <sys/poll.h>

#include "type.h"
#include "sim.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_sim {
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
#define MODEM_DEV_BASE_NUM  13
#define MODEM_DEV_PREFIX_T    "/dev/CHNPTYT"
#define MODEM_DEV_PREFIX_W    "/dev/CHNPTYW"
#define MODEM_DEV_PREFIX_TL    "/dev/CHNPTYTL"
#define MODEM_DEV_PREFIX_LF    "/dev/CHNPTYLF"
#define MODEM_DEV_PREFIX_L     "/dev/CHNPTYL"

#define SIM_CHECK_AT        "AT+EUICC?\r\n";

//------------------------------------------------------------------------------
int simOpen( void )
{
	return 0;
}

//------------------------------------------------------------------------------
int simCheck( int index )
{   
    int fd, ret = 0;
    char pty[32];
    char resp[64];
    const char * at = SIM_CHECK_AT;

    snprintf(pty, 32, "%s%d", MODEM_DEV_PREFIX_W, MODEM_DEV_BASE_NUM + index);
    fd = open(pty, O_RDWR);
    if( fd < 0 ) {
        ERRMSG("open w %s fail: %s\n", pty, strerror(errno));
        ret = fd;

        //if "CHNPTYW" not find, try  "CHNPTYT"
        memset(pty,0,sizeof(pty));
        snprintf(pty, 32, "%s%d", MODEM_DEV_PREFIX_T, MODEM_DEV_BASE_NUM + index);
        fd = open(pty, O_RDWR);

		if( fd < 0 ) {
			ERRMSG("open t %s fail: %s\n", pty, strerror(errno));
			ret = fd;

			//if "CHNPTYT" not find, try  "CHNPTYTL"
			memset(pty,0,sizeof(pty));
			snprintf(pty, 32, "%s%d", MODEM_DEV_PREFIX_TL, MODEM_DEV_BASE_NUM + index);
			fd = open(pty, O_RDWR);
			if( fd < 0 ) {
				ERRMSG("open tl %s fail: %s\n", pty, strerror(errno));
				ret = fd;

				//if "CHNPTYTL" not find, try  "CHNPTYLF"
				memset(pty,0,sizeof(pty));
				snprintf(pty, 32, "%s%d", MODEM_DEV_PREFIX_LF, MODEM_DEV_BASE_NUM + index);
				fd = open(pty, O_RDWR);
				if( fd < 0 ) {
					ERRMSG("open lf %s fail: %s\n", pty, strerror(errno));
					ret = fd;
					//if "CHNPTYTLF" not find, try  "CHNPTYL"
					memset(pty,0,sizeof(pty));
					snprintf(pty, 32, "%s%d", MODEM_DEV_PREFIX_L, MODEM_DEV_BASE_NUM + index);
					fd = open(pty, O_RDWR);
					if(fd < 0){
						ERRMSG("open l %s fail: %s\n", pty, strerror(errno));
						ret = fd;
					}
				}
			}
		}
	}

    while( fd >= 0 ) {
        
        if( write(fd, at, strlen(at)) < 0 ) {
            ERRMSG("write fail: %s\n", strerror(errno));
			close(fd);
            ret = -1;
            break;
        }
        resp[0] = 0;
        resp[sizeof(resp) - 1] = 0;
        
        struct pollfd pfd;
        int timeout = 1000; // ms
        
        pfd.fd     = fd;
        pfd.events = POLLIN;
        errno = 0;
        ret   = poll(&pfd, 1, timeout);
        if (ret < 0) {
            ERRMSG("poll() error: %s\n", strerror(errno));
            break;
        } else if( 0 == ret ) {
            ret = -1;
            WRNMSG("poll() timeout: %d ms\n", timeout);
            break;
        }
        
        if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
            ERRMSG("poll() returned  success (%d), "
                 "but with an unexpected revents bitmask: %#x\n", ret, pfd.revents);
            ret = -2;
            break;
        }
        
        if( read(fd, resp, sizeof(resp) - 1) < 0 ) {
            ERRMSG("read fail: %s\n", strerror(errno));
            ret = -3;
        } else {
            // +EUICC: 2,1,1
            char * pval = strstr(resp, "EUICC");
            DBGMSG("read OK: %s\n", resp);
            if( NULL != pval ) {
                pval += 5;
                while( *pval && (*pval < '0' || *pval > '9') ) {
                    pval++;
                }
                DBGMSG("%s\n", pval);
                ret = ('0' == *pval || '1' == *pval) ? 0 : -4;
            } else {
                ret = -5;
            }
        }

        close(fd);
        break;
    } 
            
    DBGMSG("simCheck: ret = %d\n", ret);
    return ret;
}

//------------------------------------------------------------------------------
int simClose( void )
{
	return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
