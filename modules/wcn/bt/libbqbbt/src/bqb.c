/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 **/

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "libbt-bqb"
#else
#define LOG_TAG "libbt-bqb"
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <cutils/sockets.h>
#include <sys/un.h>
#include <poll.h>
#include <signal.h>
#include <termios.h>
#include <cutils/str_parms.h>
#include <cutils/sockets.h>

#include "bqb.h"

#define UNUSED(expr) do { (void)(expr); } while (0)
#define ENG_BUFFER_SIZE 2048
#define ENG_CMDLINE_LEN 1024
static int bt_fd = -1;
static pthread_t ntid_bqb = (pthread_t)(-1);
int at_fd = -1;

controller2uplayer_t controller2uplayer = NULL;

static void thread_exit_handler(int sig) {
    ALOGD("receive signal %d , eng_receive_data_thread exit\n", sig);
    pthread_exit(0);
}

/*
** Function: eng_receive_data_thread
**
** Description:
**      Receive the data from controller, and send the data to the tester
**
** Arguments:
**     void
**
** Returns:
**
*/
void  eng_receive_data_thread(void) {
    int nRead = 0;
    char buf[1030] = {0};
    struct sigaction actions;

    ALOGD("eng_receive_data_thread");

    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = thread_exit_handler;
    sigaction(SIGUSR2, &actions, NULL);
    while (1) {
       memset(buf, 0x0, sizeof(buf));
       nRead = read(bt_fd, buf, 1000);

        ALOGD("receive data from controller: %d", nRead);
        if (nRead > 0 && controller2uplayer != NULL) {
             controller2uplayer(buf, nRead);
        }
    }
}

/*
** Function: eng_controller_bqb_start
**
** Description:
**      open the ttys0 and create a thread, download bt controller code
**
** Arguments:
**     void
**
** Returns:
**      0
**    -1 failed
*/
int eng_controller_bqb_start(controller2uplayer_t func, bt_bqb_mode_t mode) {
    pthread_t thread;
    int ret = -1;
    uint8_t local_bdaddr[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    ALOGD("test eng_controller_bqb_start");
    controller2uplayer = func;
    ALOGD("bt_on");
    bt_fd = bt_on(mode);
    ALOGD("bt_fd = %d", bt_fd);

    ret = pthread_create(&ntid_bqb, NULL, (void *)eng_receive_data_thread, NULL);   /*create thread*/
    if (ret== -1) {
        ALOGD("create thread failed");
    }

    return ret;
}

int eng_controller_bqb_stop(void) {
    ALOGD("stop BQB test receive data thread");

    if (pthread_kill(ntid_bqb, SIGUSR2) != 0) {
        ALOGD("pthread_kill BQB test receive data thread error\n");
        return -1;
    }
    pthread_join(ntid_bqb, NULL);
    bt_off();
    return 0;
}

/*
** Function: eng_send_data
**
** Description:
**      Recieve the data from tester and send the data to controller
**
** Arguments:
**     data  from the tester
**   the data length
**
** Returns:
**      0  success
**    -1 failed
*/
void eng_send_data(char * data, int data_len) {
    int nWritten = 0;
    int count = data_len;
    char * data_ptr = data;

    ALOGD("eng_send_data, fd=%d, len=%d", bt_fd, count);
    lmp_assert();
    while (count) {
        nWritten = write(bt_fd, data, data_len);
        count -= nWritten;
        data_ptr  += nWritten;
    }
    lmp_deassert();
    ALOGD("eng_send_data nWritten %d ", nWritten);
}



static inline int create_server_socket(const char* name) {
  int s = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (s < 0) {
        ALOGD("socket(AF_LOCAL, SOCK_STREAM, 0) failed \n");
        return -1;
    }
  ALOGD("bqb covert name to android abstract name:%s", name);
  if (socket_local_server_bind(s, name, ANDROID_SOCKET_NAMESPACE_ABSTRACT) >=
      0) {
    if (listen(s, 5) == 0) {
      ALOGD("bqb listen to local socket:%s, fd:%d", name, s);
      return s;
    } else {
      ALOGD("bqb listen to local socket:%s, fd:%d failed, errno:%d", name, s,
              errno);
    }
  } else {
    ALOGD("bqb create local socket:%s fd:%d, failed, errno:%d", name, s,
            errno);
  }
  close(s);
  return -1;
}

static int accept_server_socket(int sfd) {
  struct sockaddr_un remote;
  struct pollfd pfd;
  int fd;
  socklen_t len = sizeof(struct sockaddr_un);

  // ENG_LOG("accept fd %d", sfd);

  /* make sure there is data to process */
  pfd.fd = sfd;
  pfd.events = POLLIN;

  if (poll(&pfd, 1, 0) == 0) {
    ALOGD("accept poll timeout");
    return -1;
  }

  // ENG_LOG("poll revents 0x%x", pfd.revents);

  if ((fd = accept(sfd, (struct sockaddr*)&remote, &len)) == -1) {
    ALOGD("sock accept failed (%s)", strerror(errno));
    return -1;
  }

  // ENG_LOG("new fd %d", fd);

  return fd;
}


int eng_controller2tester(char * controller_buf, unsigned int data_len) {
    int len = 0;
    len = write(at_fd,controller_buf, data_len);
    ALOGD("bqb test eng_controller2tester %d", len);
    return len;
}


static void bqb_service_enable(int pc_fd, bt_bqb_mode_t mode) {
  struct termios ser_settings;
  int ret = 0;
  ALOGD("bqb bqb_service_enable");
  tcgetattr(pc_fd, &ser_settings);
  cfmakeraw(&ser_settings);
  ser_settings.c_lflag = 0;
  tcsetattr(pc_fd, TCSANOW, &ser_settings);
  ret = eng_controller_bqb_start(eng_controller2tester, mode);
}

static void bqb_service_disable(int pc_fd) {
  struct termios ser_settings;
  int ret = 0;
  ALOGD("bqb bqb_service_disable");
  tcgetattr(pc_fd, &ser_settings);
  cfmakeraw(&ser_settings);
  ser_settings.c_lflag |= (ECHO | ECHONL);
  ser_settings.c_lflag &= ~ECHOCTL;
  tcsetattr(pc_fd, TCSANOW, &ser_settings);
  ret = eng_controller_bqb_stop();
}



int check_received_str(int fd, char* engbuf, int len) {
    int is_bqb_cmd = 0;
    ALOGD("pc got: %s: %d", engbuf, len);
      if (strstr(engbuf, ENABLE_BQB_TEST)) {
          is_bqb_cmd = 1;
        if (current_bqb_state == BQB_CLOSED) {
                    bqb_service_enable(at_fd, BQB_NORMAL);
          write(fd, NOTIFY_BQB_ENABLE, strlen(NOTIFY_BQB_ENABLE));
        } else {
          write(fd, NOTIFY_BQB_ENABLE, strlen(NOTIFY_BQB_ENABLE));
        }
      } else if (strstr(engbuf, DISABLE_BQB_TEST)) {
          is_bqb_cmd = 1;
        if (current_bqb_state != BQB_CLOSED) {
          bqb_service_disable(at_fd);
          write(fd, NOTIFY_BQB_DISABLE, strlen(NOTIFY_BQB_DISABLE));
        } else {
          write(fd, NOTIFY_BQB_DISABLE, strlen(NOTIFY_BQB_DISABLE));
        }
      } else if (strstr(engbuf, TRIGGER_BQB_TEST)) {
          is_bqb_cmd = 1;
        if (current_bqb_state == BQB_OPENED) {
          write(fd, TRIGGER_BQB_ENABLE, strlen(TRIGGER_BQB_ENABLE));
        } else if (current_bqb_state == BQB_CLOSED) {
          write(fd, TRIGGER_BQB_DISABLE, strlen(TRIGGER_BQB_DISABLE));
        }
      }
      return is_bqb_cmd;
}


static void* eng_read_socket_thread(void* par) {
    UNUSED(par);
    int max_fd, ret, bqb_socket_fd, fd;
    int len;
    char engbuf[ENG_BUFFER_SIZE];

    fd_set read_set;
    bqb_socket_fd = create_server_socket(BQB_CTRL_PATH);
    if (bqb_socket_fd < 0) {
        ALOGD("creat bqb server socket error(%s)", strerror(errno));
        return NULL;
      } else {
        max_fd = bqb_socket_fd;
      }
  for (;;) {
        FD_ZERO(&read_set);
        if (bqb_socket_fd > 0) {
          FD_SET(bqb_socket_fd, &read_set);
        }
        ret = select(max_fd + 1, &read_set, NULL, NULL, NULL);
        if (ret == 0) {
          ALOGD("select timeout");
          continue;
        } else if (ret < 0) {
          ALOGD("select failed %s", strerror(errno));
          continue;
        }

        if (FD_ISSET(bqb_socket_fd, &read_set)) {
          ALOGD("bqb_socket_fd got");
          fd = accept_server_socket(bqb_socket_fd);
            if (fd < 0) {
                ALOGD("bqb get service socket fail");
                sleep(1);
                continue;
        }


      memset(engbuf, 0, ENG_BUFFER_SIZE);
      len = read(fd, engbuf, ENG_BUFFER_SIZE);
      ALOGD("bqb control: %s: len: %d", engbuf, len);
      check_received_str(fd, engbuf, len);
      close(fd);
      continue;
    }
  }

  return NULL;
}

void start_socket_thread() {
    pthread_t t1;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    // set thread name
    ALOGD("sensen start eng_read_socket_thread");
    pthread_create(&t1, &attr, eng_read_socket_thread, (void*)NULL);
}

void set_bqb_state(int state) {
    ALOGD("current_bqb_state = %d, next state=%d", current_bqb_state, state);
    current_bqb_state = state;
}


static void bqb_init(void) {
    ALOGD("%s", __func__);

    set_bqb_state(BQB_CLOSED);
    start_socket_thread();
}

static void bqb_exit(void) {
    ALOGD("%s", __func__);
// kill thread
    set_bqb_state(BQB_CLOSED);
}

void set_at_fd(int fd) {
    at_fd = fd;        // at interface fd
}

int get_bqb_state() {
    ALOGD("current_bqb_state = %d", current_bqb_state);
    return current_bqb_state;
}

const bt_bqb_interface_t BLUETOOTH_BQB_INTERFACE = {
    sizeof(bt_bqb_interface_t),
    bqb_init,
    bqb_exit,
    set_at_fd,
    get_bqb_state,
    check_received_str,
    eng_send_data
};
