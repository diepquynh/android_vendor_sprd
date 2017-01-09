#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <cutils/sockets.h>
#include <poll.h>
#include <sys/sysinfo.h>
#include "eng_diag.h"
#include "calibration.h"
#include "eng_pcclient.h"
#include "engopt.h"
#include "vlog.h"
#include "eng_ap_modem_time_sync.h"

TIME_SYNC_T g_time_sync = {0};
extern pthread_mutex_t g_time_sync_lock;
static enum CPEVENT_E parse_cp_event_notify(const char* buf);
static void fds_init(struct pollfd *fds, int len);
static void fds_collate(struct pollfd *fds, int len,
                        struct SocketConnection* modemd_sock,
                        struct SocketConnection* refnotify_sock);
static int try_connect(struct SocketConnection* sock, struct pollfd* poll_array,
                       int len, int* poll_num, const char* server_name);
static int add_connection(struct pollfd* poll_array, int len,
                          int* poll_num, int fd);
static int cp_event_notify_rsp_handle(CP_EVENT_NOTIFY_T *cp_event);
static int time_sync_rsp_handle(MODEM_TIMESTAMP_T *ts);

int get_timezone() {
  time_t t1;
  struct tm* pg;
  struct tm gm_tm;

  t1 = time(0);
  if ((time_t)(-1) == t1) {
    return 0;
  }
  pg = gmtime_r(&t1, &gm_tm);
  if (!pg) {
    return 0;
  }

  return (int)(difftime(t1, mktime(pg)));
}

static void fds_init(struct pollfd *fds, int len) {
  int i;
  for (i = 0; i < len; i++) {
    fds[i].fd = -1;
    fds[i].events = POLLIN;
    fds[i].revents = 0;
  }
  return;
}

static void fds_collate(struct pollfd *fds,
                        int len,
                        struct SocketConnection* modemd_sock,
                        struct SocketConnection* refnotify_sock) {
  int i;
  int j;

  // Search for the first hole in the array
  for (i = 0; i < len; ++i) {
   if (fds[i].fd < 0) {
     break;
   }
  }

  if (i >= len -1) {
    // No hole
    return;
  }

  // Move all non-hole to i and onwards
  for (j = i + 1; j < len; j++) {
    int fd = fds[j].fd;
    if (fd >= 0) {
      fds[i].fd = fd;
      if (fd == modemd_sock->fd) {
        modemd_sock->index = i;
      } else if (fd == refnotify_sock->fd) {
        refnotify_sock->index = i;
      }
    }
    ++i;
  }

  return;
}

static int try_connect(struct SocketConnection* sock, struct pollfd* poll_array,
                       int len, int* poll_num, const char* server_name) {
  int ret = -1;

  //max_retry:max number of connection trials, 0 for no limit
  if(sock->max_retry && sock->try_num > sock->max_retry)
    return ret;

  sock->fd = socket_local_client(server_name,
      ANDROID_SOCKET_NAMESPACE_ABSTRACT,
      SOCK_STREAM);
  if (sock->fd >= 0) {
    // Add the connection to the polling array
    sock->index = add_connection(poll_array, len, poll_num, sock->fd);
    ENG_LOG("%s: connect %s socket poll_num=%d, sock->fd=%d, sock->index=%d\n",
        __FUNCTION__, server_name, *poll_num, sock->fd, sock->index);
    // Reset the connection count
    sock->try_num = 0;
    ret = sock->index;
  } else { // Connection failed
    ++sock->try_num;
  }

  if (ret < 0) {
    ENG_LOG("%s: connect %s socket error(%s), try_num=%d\n", __FUNCTION__,
        server_name, strerror(errno), sock->try_num);
  } else {
    ENG_LOG("%s: connect %s socket success, try_num=%d\n", __FUNCTION__,
        server_name, sock->try_num);
  }
  return ret;
}

static int add_connection(struct pollfd* poll_array, int len,
                          int* poll_num, int fd) {
  // Search for the first available position
  int i;

  for (i = 0; i < len; ++i) {
    if (poll_array[i].fd < 0) {
      poll_array[i].fd = fd;
      poll_array[i].events = POLLIN;
      poll_array[i].revents = 0;
      ++*poll_num;
      break;
    }
  }
  if (i >= len) {
    i = -1;
  }
  // Return the index in the array
  return i;
}


static enum CPEVENT_E parse_cp_event_notify(const char* buf) {

  if(strstr(buf, "P-ARM Modem Assert") != NULL) {
    return CE_ASSERT;
  }
  if(strstr(buf, "TD Modem Assert") != NULL) {
    return CE_ASSERT;
  }
  if(strstr(buf, "Modem Assert") != NULL) {
    return CE_ASSERT;
  }
  if(strstr(buf, "Modem Reset") != NULL) {
    return CE_RESET;
  }
  if(strstr(buf, "Modem Alive") != NULL) {
    return CE_ALIVE;
  }
  if(strstr(buf, "Modem Blocked") != NULL) {
    return CE_BLOCKED;
  }
  return CE_NONE;
}

static int cp_event_notify_rsp_handle(CP_EVENT_NOTIFY_T *cp_event) {
  int ret = -1;
  char tmp[128] = {0};
  int tmp_len;
  char rsp[256] = {0};
  int rsp_len;
  int fd = get_ser_diag_fd();
  MSG_HEAD_T head;
  head.len = sizeof(MSG_HEAD_T) + sizeof(CP_EVENT_NOTIFY_T);
  head.seq_num = 0;
  head.type = 0x33;
  head.subtype = 0xff;
  memcpy(tmp, &head, sizeof(MSG_HEAD_T));
  memcpy(tmp + sizeof(MSG_HEAD_T), cp_event, sizeof(CP_EVENT_NOTIFY_T));

  rsp_len = translate_packet(rsp, (unsigned char*)tmp, head.len);

  ret = eng_diag_write2pc(rsp, rsp_len, fd);
  if (ret <= 0) {
    ENG_LOG("%s: eng_diag_write2pc ret=%d !\n", __FUNCTION__, ret);
    return -1;
  }
  return 0;
}

static int time_sync_rsp_handle(MODEM_TIMESTAMP_T *ts) {
  int ret = -1;
  char tmp[128] = {0};
  int tmp_len;
  char rsp[256] = {0};
  int rsp_len;
  int fd = get_ser_diag_fd();
  MSG_HEAD_T head;
  TOOLS_DIAG_AP_CNF_T ap_cnf;
  head.len = sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CNF_T) + sizeof(MODEM_TIMESTAMP_T);
  head.seq_num = 0;
  head.type = 0x5;
  head.subtype = 0x11;
  memcpy(tmp, &head, sizeof(MSG_HEAD_T));
  ap_cnf.status = 0;
  ap_cnf.length = sizeof(MODEM_TIMESTAMP_T);
  memcpy(tmp + sizeof(MSG_HEAD_T), &ap_cnf, sizeof(TOOLS_DIAG_AP_CNF_T));
  memcpy(tmp + sizeof(MSG_HEAD_T) + sizeof(TOOLS_DIAG_AP_CNF_T), ts, sizeof(MODEM_TIMESTAMP_T));

  rsp_len = translate_packet(rsp, (unsigned char*)tmp, head.len);

  ret = eng_diag_write2pc(rsp, rsp_len, fd);
  if (ret <= 0) {
    ENG_LOG("%s: eng_diag_write2pc ret=%d !\n", __FUNCTION__, ret);
    return -1;
  }
  return 0;

}

void current_ap_time_stamp_handle(TIME_SYNC_T *time_sync, MODEM_TIMESTAMP_T *time_stamp) {
  struct timeval time_now;
  struct sysinfo sinfo;
  struct tm *tmp;
  char strTime[32] = {0};

  gettimeofday(&time_now, 0);
  sysinfo(&sinfo);
  time_stamp->sys_cnt = time_sync->sys_cnt + (sinfo.uptime - time_sync->uptime) * 1000;
  time_stamp->tv_sec = (uint32_t)(time_now.tv_sec);
  time_stamp->tv_usec = (uint32_t)(time_now.tv_usec);

  //add timezone
  time_stamp->tv_sec += get_timezone();
  ENG_LOG("%s: [time_sync]:sys_cnt=%d, uptime=%d; "
      "[time_stamp]:tv_sec=%d, tv_usec=%d, sys_cnt=%d\n",
      __FUNCTION__, time_sync->sys_cnt, time_sync->uptime,
      time_stamp->tv_sec, time_stamp->tv_usec, time_stamp->sys_cnt);

  tmp = gmtime(&time_stamp->tv_sec);
  strftime(strTime, 32, "%F %T", tmp);
  ENG_LOG("%s: AP Time %s %d Micorseconds\n", __FUNCTION__, strTime, time_stamp->tv_usec);

  return;
}

void* eng_timesync_thread(void* x) {
  int ser_fd = -1;
  int mod_cnt = -1;
  int ref_cnt = -1;
  struct pollfd fds[FDS_SIZE];
  int modem_reset_flag = 0;
  int err_counter = 0;
  struct timespec ts = {1, 0};
  struct SocketConnection modemd_conn = { -1, -1, 0, 0 };
  struct SocketConnection refnotify_conn = { -1, -1, 0, 0 };
  int total_conn = 0;
  int fds_collate_flag = 0;
  fds_init(fds, FDS_SIZE);

  ENG_LOG("eng_timesync thread start\n");
  while (1) {
    MODEM_TIMESTAMP_T time_stamp;
    TIME_SYNC_T time_sync;
    struct timeval time_now;
    struct sysinfo sinfo;
    char notify_buf[256] = {0};
    CP_EVENT_NOTIFY_T cp_event;
    int ret = -1;
    int i = 0;
    int poll_ret = -1;
    int timeout = -1;

    if (fds_collate_flag) {
      fds_collate(fds, FDS_SIZE, &modemd_conn, &refnotify_conn);
      fds_collate_flag = 0;
    }

    if (modemd_conn.fd < 0) {
      try_connect(&modemd_conn, fds, FDS_SIZE, &total_conn, MODEM_SOCKET_NAME);
    }
    if (refnotify_conn.fd < 0) {
      try_connect(&refnotify_conn, fds, FDS_SIZE, &total_conn, TIME_SERVER_SOCK_NAME);
    }

    ENG_LOG("%s: total_conn=%d\n", __FUNCTION__, total_conn);
    // If not all connections are established, set a timeout.
    if (!total_conn) { // All connections failed
      // Sleep for a while and retry
      nanosleep(&ts, NULL);
      continue;
    } else if (total_conn < FDS_SIZE) { // Some connection failed
      // Set a timeout
      timeout = 1000;
    } else { // All connections are established
      // Wait for all connections
      timeout = -1;
    }
    poll_ret = poll(fds, total_conn, timeout);
    if (poll_ret > 0) {
      for (i = 0; i < total_conn; ++i) {
        if (fds[i].revents & POLLIN) {
          if (i == modemd_conn.index) {
            mod_cnt = read(fds[i].fd, notify_buf, sizeof(notify_buf) - 1);
            if (mod_cnt < 0) {
              ENG_LOG("%s: read error[%s], can't read time info\n", __FUNCTION__,
                  strerror(errno));
            } else if (mod_cnt == 0) {
              modemd_conn.fd = -1;
              fds[i].fd = -1;
              --total_conn;
              fds_collate_flag = 1;
              ENG_LOG("%s: modemd socket disconnect\n", __FUNCTION__);
            } else {
              notify_buf[mod_cnt] = '\0';
              ENG_LOG("%s: notify_buf=%s\n", __FUNCTION__, notify_buf);
              if(parse_cp_event_notify(notify_buf) == CE_RESET) {
                modem_reset_flag = 1;
                cp_event.subsys = SS_MODEM;
                cp_event.event = CE_RESET;
                ret = cp_event_notify_rsp_handle(&cp_event);
                if(ret == -1) {
                  ENG_LOG("%s: Modem Reset, notice pc tool Error[%s]\n", __FUNCTION__,
                      strerror(errno));
                } else {
                  ENG_LOG("%s: Modem Reset, notice pc tool success\n", __FUNCTION__);
                }
              }
            }

          } else if (i == refnotify_conn.index) {
            ref_cnt = read(fds[i].fd, &time_sync, sizeof(TIME_SYNC_T));
            if (ref_cnt < 0) {
              ENG_LOG("%s: read error[%s], can't read time info\n", __FUNCTION__,
                  strerror(errno));
              continue;
            } else if (ref_cnt == 0) {
              refnotify_conn.fd = -1;
              fds[i].fd = -1;
              --total_conn;
              fds_collate_flag = 1;
              ENG_LOG("%s: refnotify socket disconnect\n", __FUNCTION__);
              continue;
            } else if (ref_cnt != sizeof(TIME_SYNC_T)) {
              ENG_LOG("%s: read %d bytes, can't read time info\n", __FUNCTION__, ref_cnt);
              continue;
            } else {
              ENG_LOG("%s: receive time sync data from refnotify\n", __FUNCTION__);

              pthread_mutex_lock(&g_time_sync_lock);
              memcpy(&g_time_sync, &time_sync, sizeof(TIME_SYNC_T));
              pthread_mutex_unlock(&g_time_sync_lock);

              current_ap_time_stamp_handle(&time_sync, &time_stamp);
              if (modem_reset_flag) {
                modem_reset_flag = 0;
                ret = time_sync_rsp_handle(&time_stamp);
                if(ret == -1) {
                  ENG_LOG("%s: Time sync, notice pc tool Error[%s]\n", __FUNCTION__,
                      strerror(errno));
                } else {
                  ENG_LOG("%s: Time sync, notice pc tool success\n", __FUNCTION__);
                }
              }
            }
          }
        }
      }
    }
  }

out:
  ENG_LOG("eng_timesync thread end\n");
  if (modemd_conn.fd >= 0) close(modemd_conn.fd);
  if (refnotify_conn.fd >= 0) close(refnotify_conn.fd);
  pthread_mutex_destroy(&g_time_sync_lock);
  return 0;
}
