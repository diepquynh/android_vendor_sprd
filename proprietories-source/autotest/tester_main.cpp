// 
// Spreadtrum Auto Tester
//
// anli   2012-11-09
//
#include "type.h"
#include "tester.h"
#include "ver.h"
#include "autotest.h"

#include <sys/poll.h>
#include <cutils/sockets.h>

//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------

#define AUTOTEST_HANDSHAKE   "ImAutotest"

//------------------------------------------------------------------------------
static uchar * sBuffer = NULL;
//------------------------------------------------------------------------------
static int  asynRead(int fd, uchar * buf, int size, int timeout);
static int  asynWrite(int fd, const uchar * buf, int cnt, int timeout);
static void signalProcess( int signo );

int autotest_main( int argc, char *argv[] )
{
	DBGMSG("........ autotest_main enter ........\n");

	if( test_Init() < 0 ) {
		test_Deinit();
		return -1;
	}

	#define RCVSIZE (2 * 1024)
	#define SNDSIZE (2* 1024)

    sBuffer = new uchar[RCVSIZE + SNDSIZE];
    if( NULL == sBuffer ) {
		test_Deinit();
        return -1;
    }

	uchar * rcvBuf = sBuffer;
	uchar * sndBuf = sBuffer + RCVSIZE;

	while( 1 ) {
		const char * hs = AUTOTEST_HANDSHAKE;

		int sck = socket_local_client("eng_autotst_sck",
						ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
		if( sck < 0 || write(sck, hs, strlen(hs)) < 0 ) {
			ERRMSG("error: sck = %d, errno = %d\n", sck, errno);
			if(sck >= 0) {
				close(sck);
			}
			usleep(1000 * 1000);
			continue;
		}

		DBGMSG("aha, im connect to server.\n");
		while( 1 ) {
			int rcv_cnt = asynRead(sck, rcvBuf, RCVSIZE, 30000);
			DBGMSG("autotest_main rcv_cnt=%d,errno=%d\n",rcv_cnt,errno);
			if( 0 == rcv_cnt ) {
				continue;
			} else if( rcv_cnt < 0 ) {
				break;
			}
			DBGMSG("rcv cnt = %d\n", rcv_cnt);

			int snd_cnt = test_DoTest(rcvBuf, rcv_cnt, sndBuf, SNDSIZE);
			asynWrite(sck, (const uchar *)&snd_cnt, sizeof(snd_cnt), 3000);
			if( snd_cnt > 0 ) {
				asynWrite(sck, sndBuf, snd_cnt, 4000);
			}
		}
		close(sck);
    }
	delete [] sBuffer;
	sBuffer = NULL;

	test_Deinit();

	DBGMSG("........ autotest_main exit ........\n");
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int asynRead(int fd, uchar * buf, int size, int timeout)
{
	int ret = 0;

    struct pollfd pfd;
    do {
        pfd.fd     = fd;
        pfd.events = POLLIN;
		pfd.revents= 0;
        errno = 0;
        ret   = poll(&pfd, 1, timeout);
        if (ret < 0) {
            ERRMSG("poll() error: %s\n", strerror(errno));
            break;
        } else if( 0 == ret ) {
            INFMSG("poll() timeout: %d ms\n", timeout);
            break;
        }

        if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
            ERRMSG("poll() returned  success (%d), "
                 "but with an unexpected revents bitmask: %#x\n", ret, pfd.revents);
            ret = -1;
            break;
        }
        //DBGMSG("after poll\n");

		do {
			ret = read(fd, buf, size);
		} while (ret < 0 && errno == EINTR);

        DBGMSG("read %d bytes\n", ret);
    } while( 0 );

    //DBGMSG(" %s exit\n", __FUNCTION__);
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int asynWrite(int fd, const uchar * buf, int cnt, int timeout)
{
	int ret = 0;

    struct pollfd pfd;
    do {
        pfd.fd     = fd;
        pfd.events = POLLOUT;
		pfd.revents= 0;
        errno = 0;
        ret   = poll(&pfd, 1, timeout);
        if (ret < 0) {
            ERRMSG("poll() error: %s\n", strerror(errno));
            break;
        } else if( 0 == ret ) {
            INFMSG("poll() timeout: %d ms\n", timeout);
            break;
        }

        if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
            ERRMSG("poll() returned  success (%d), "
                 "but with an unexpected revents bitmask: %#x\n", ret, pfd.revents);
            ret = -1;
            break;
        }
        //DBGMSG("after poll\n");

		do {
			ret = write(fd, buf, cnt);
		} while (ret < 0 && errno == EINTR);

        DBGMSG("write %d bytes\n", ret);
    } while( 0 );

    //DBGMSG(" %s exit\n", __FUNCTION__);
    return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
