//
// for Spreadtrum Auto Test
//
// anli   2013-02-20
//
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/poll.h>
#include "engopt.h"
#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>

#include "eng_autotest.h"

//------------------------------------------------------------------------------
#define AUTOTEST_SOCK_NAME "eng_autotst_sck"
#define AUTOTEST_SERVICE "autotest"
#define AUTOTEST_HANDSHAKE "ImAutotest"

#define WRITE_TO 4000  // ms
#define READ_TO 4000   // ms
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define LTAG "eng_autotest"

#if 1
#define EAT_DBG(fmt, arg...) \
  android_printLog(ANDROID_LOG_DEBUG, LTAG, fmt, ##arg)
#define EAT_INF(fmt, arg...) \
  android_printLog(ANDROID_LOG_INFO, LTAG, fmt, ##arg)
#define EAT_WRN(fmt, arg...) \
  android_printLog(ANDROID_LOG_WARN, LTAG, fmt, ##arg)
#define EAT_ERR(fmt, arg...) \
  android_printLog(ANDROID_LOG_ERROR, LTAG, fmt, ##arg)
#else
#define EAT_DBG(fmt, arg...) \
  do {                       \
  } while (0)
#define EAT_INF(fmt, arg...) \
  do {                       \
  } while (0)
#define EAT_WRN(fmt, arg...) \
  do {                       \
  } while (0)
#define EAT_ERR(fmt, arg...) \
  do {                       \
  } while (0)
#endif

//------------------------------------------------------------------------------
static int s_sckAccept = -1;
static int s_running = 0;
static int s_sckServer = -1;
int g_autotest_flag = 0;
;
static int test_buff[3] = {0x5a, 0x5a, 0x5a};

static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;

//------------------------------------------------------------------------------
static void *acceptThread(void *param);
static int asynRead(int fd, uchar *buf, int size, int timeout);
static int asynWrite(int fd, const uchar *buf, int cnt, int timeout);
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
int eng_autotestStart(void) {
  char status[PROPERTY_VALUE_MAX];
  int num, ret;

  if (s_sckServer >= 0) {
    return 0;
  }
  ENG_LOG("eng_autotestStart : errno = %d \n", errno);
  s_sckServer = socket_local_server(
      AUTOTEST_SOCK_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
  ENG_LOG("eng_autotestStart : s_sckServer = %d, errno = %d \n", s_sckServer,
          errno);
  if (s_sckServer < 0) {
    ENG_LOG("socket_local_server error: sock = %d, errno = %d\n", s_sckServer,
            errno);
    return -1;
  }

  s_running = 1;

  pthread_t ptid;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&ptid, &attr, acceptThread, NULL);
  usleep(500 * 1000);  // 500 * 1000
  ENG_LOG(
      "eng_autotestStart: s_sckServer = %d, s_sckAccept = %d, s_running = %d "
      "\n",
      s_sckServer, s_sckAccept, s_running);
  num = 6;
  while (num--) {
    ret = -2;
    status[0] = 0;

    property_set("ctl.start", "autotest");
    usleep(100 * 1000);

    property_get("init.svc.autotest", status, NULL);
    ENG_LOG("svc autotest status = %s\n", status);

    if (0 == strcmp(status, "running")) {
      ret = 0;
      g_autotest_flag = 1;
      break;
    }
  }

  return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
int eng_autotestIsConnect(void) {
  ENG_LOG("eng_autotestIsConnect: s_sckAccept = %d  \n", s_sckAccept);
  return (s_sckAccept >= 0) ? 1 : -1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
int eng_autotestDoTest(const uchar *req, int req_len, uchar *rsp,
                       int rsp_size) {
  int ret = 0;
  ENG_LOG("eng_autotestDoTest %02x %02x %02x %02x %02x; len = %d\n", req[4],
          req[5], req[6], req[7], req[8], req_len);
  pthread_mutex_lock(&s_mutex);

  if (s_sckAccept < 0) {
    ENG_LOG("client not connect!!!!\n");
    pthread_mutex_unlock(&s_mutex);
    return -1;
  }

  if (asynWrite(s_sckAccept, req, req_len, WRITE_TO) <= 0) {
    ENG_LOG("write error: errno = %d\n", errno);
    pthread_mutex_unlock(&s_mutex);
    return -2;
  }

  if (asynRead(s_sckAccept, (uchar *)&ret, sizeof(ret), 20 * 1000) < 0) {
    ENG_LOG("read error: errno = %d\n", errno);
    pthread_mutex_unlock(&s_mutex);
    return -3;
  }
  if (ret > 0) {
    if (ret > rsp_size) {
      ENG_LOG("!!!! buffer to small, dst = %d, need = %d !!!!\n", rsp_size,
              ret);
      ret = rsp_size;
    }

    if (asynRead(s_sckAccept, rsp, ret, READ_TO) <= 0) {
      ENG_LOG("read error 2: errno = %d\n", errno);
      pthread_mutex_unlock(&s_mutex);
      return -4;
    }
  }
  pthread_mutex_unlock(&s_mutex);
  EAT_DBG("ret = %d\n", ret);
  return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
/*int eng_autotestStop( void )
{
        s_running = 0;

        if( s_sckServer >= 0 ) {
                close(s_sckServer);
        }

        if( s_sckAccept >= 0 ) {
                close(s_sckAccept);
        }
        sleep(1);

        s_sckServer = -1;
        s_sckAccept = -1;
       ENG_LOG("eng_autotestStop: s_sckAccept = %d \n", s_sckAccept);
        return 0;
}*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void *acceptThread(void *param) {
  char buffer[32];
  int flags;
  struct sockaddr addr;
  socklen_t alen = sizeof(addr);
  const char *hs = AUTOTEST_HANDSHAKE;
  ENG_LOG("acceptThread: s_sckServer = %d \n", s_sckServer);
  if (s_sckServer < 0) {
    return NULL;
  }

  while (s_running) {
    ENG_LOG("acceptThread: s_sckServer = %d, s_running = %d \n", s_sckServer,
            s_running);
    int sck = accept(s_sckServer, &addr, &alen);
    ENG_LOG("accept error: sck = %d, errno = %d\n", sck, errno);
    if (sck < 0) {
      ENG_LOG("accept error: sck = %d, errno = %d\n", sck, errno);
      continue;
    }

    ENG_LOG("accept sock = %d\n", sck);
    flags = fcntl(sck, F_GETFL);
    if (flags < 0) {
      ENG_LOG("get sck flags error with fcntl\n");
      close(sck);
      continue;
    }
    if (fcntl(sck, F_SETFL, flags | O_NONBLOCK) < 0) {
      ENG_LOG("set sck flags error with fcntl\n");
      close(sck);
      continue;
    }

    if (asynRead(sck, (uchar *)buffer, sizeof(buffer), 1000) > 0 &&
        0 == strncmp(buffer, hs, strlen(hs))) {
      EAT_DBG("ok, welcome autotest client.\n");

      pthread_mutex_lock(&s_mutex);
      if (s_sckAccept >= 0) {
        close(s_sckAccept);
      }

      s_sckAccept = sck;
      test_buff[1] = s_sckAccept;
      ENG_LOG("acceptThread s_sckAccept  = %d  \n", s_sckAccept);
      pthread_mutex_unlock(&s_mutex);

      continue;
    }

    ENG_LOG("invalid connect!\n");
    close(sck);
  }

  EAT_DBG("acceptThread exit.\n");
  return NULL;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int asynRead(int fd, uchar *buf, int size, int timeout) {
  int ret = 0;

  struct pollfd pfd;
  do {
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    errno = 0;
    ret = poll(&pfd, 1, timeout);
    if (ret < 0) {
      ENG_LOG("poll() error: %s\n", strerror(errno));
      break;
    } else if (0 == ret) {
      ENG_LOG("poll() timeout: %d ms\n", timeout);
      break;
    }

    if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
      ENG_LOG(
          "poll() returned  success (%d), "
          "but with an unexpected revents bitmask: %#x\n",
          ret, pfd.revents);
      ret = -1;
      break;
    }
    // EAT_DBG("after poll\n");

    do {
      ret = read(fd, buf, size);
    } while (ret < 0 && errno == EINTR);

    ENG_LOG("read %d bytes\n", ret);
  } while (0);

  // EAT_DBG(" %s exit\n", __FUNCTION__);
  return ret;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int asynWrite(int fd, const uchar *buf, int cnt, int timeout) {
  int ret = 0;

  struct pollfd pfd;
  do {
    pfd.fd = fd;
    pfd.events = POLLOUT;
    pfd.revents = 0;
    errno = 0;
    ret = poll(&pfd, 1, timeout);
    if (ret < 0) {
      ENG_LOG("poll() error: %s\n", strerror(errno));
      break;
    } else if (0 == ret) {
      ENG_LOG("poll() timeout: %d ms\n", timeout);
      break;
    }

    if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
      ENG_LOG(
          "poll() returned  success (%d), "
          "but with an unexpected revents bitmask: %#x\n",
          ret, pfd.revents);
      ret = -1;
      break;
    }
    // EAT_DBG("after poll\n");

    do {
      ret = write(fd, buf, cnt);
    } while (ret < 0 && errno == EINTR);

    ENG_LOG("write %d bytes\n", ret);
  } while (0);

  // EAT_DBG(" %s exit\n", __FUNCTION__);
  return ret;
}
