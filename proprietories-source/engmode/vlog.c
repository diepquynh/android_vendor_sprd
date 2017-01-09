#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <termios.h>
#include <semaphore.h>
#include <cutils/sockets.h>
#include <poll.h>
#include "engopt.h"
#include "cutils/properties.h"
#include "eng_pcclient.h"
#include "vlog.h"
#include "eng_util.h"
#ifndef CONFIG_MINIENGPC
#include "gps_pc_mode.h"
#endif
#include "eng_diag.h"

#define DATA_BUF_SIZE (4096 * 64)
#define MAX_OPEN_TIMES 100
#define GPS_DATA_BUF_SIZE 4096
#define MAX_NAME_LEN 4096
#define DATA_EXT_LOG_SIZE (4096 * 64 * 2)

#define AUDIO_DSP_LOG 0
#define AUDIO_DSP_PCM 1
#define AUDIO_DSP_MEM 2
static uint32_t m_sn;

extern sem_t g_armlog_sem;
extern int g_autotest_flag;
extern int g_armlog_enable;
extern int g_ass_start;
extern sem_t g_gps_sem;
extern int g_gps_log_enable;
extern int g_ap_cali_flag;
static char log_data[DATA_BUF_SIZE];
static char diag_data[DATA_BUF_SIZE];
static char gps_log_data[GPS_DATA_BUF_SIZE] = {0};
static int s_ser_diag_fd = 0;
static eng_dev_info_t* s_dev_info;
static char ext_log_data_buf[DATA_EXT_LOG_SIZE];
static int ext_log_buf_len;
char external_path[MAX_NAME_LEN];
char top_logdir[MAX_NAME_LEN];

extern void eng_usb_enable(void);
extern void eng_usb_maximum_speed(USB_DEVICE_SPEED_ENUM speed);
extern void set_raw_data_speed(int fd, int speed);

static void dump_mem_len_print(int r_cnt, int* dumplen, char *data) {
  unsigned int head, len, tail;

  if (r_cnt == 12) {
    head = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) |
           data[0];
    len = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) |
          data[4];
    tail = (data[11] << 24) | (data[10] << 16) | (data[9] << 8) |
           data[8];

    ENG_LOG("eng_vlog: get 12 bytes, let's check if dump finished.\n");

    if (tail == (len ^ head)) {
      ENG_LOG("eng_vlog: cp dump memory len: %d, ap dump memory len: %d\n", len,
              *dumplen);
      *dumplen = 0;

      g_ass_start = 0;
    }
  }

  if (g_ass_start) {
    *dumplen += r_cnt;
  }
}

int get_ser_diag_fd(void) { return s_ser_diag_fd; }

void update_ser_diag_fd(int fd) { s_ser_diag_fd = fd; }

char* get_ser_diag_path(void) { return s_dev_info->host_int.dev_diag; }

int restart_gser(int* fd, char* dev_path) {
  struct termios ser_settings;

  if (*fd > 0) {
    ENG_LOG("%s: eng_vlog close usb serial:%d\n", __FUNCTION__, *fd);
    close(*fd);
  }

  *fd = eng_open_dev(dev_path, O_WRONLY);
  if (*fd < 0) {
    ENG_LOG("%s: eng_vlog cannot open general serial, ERROR:%s\n", __FUNCTION__, strerror(errno));
    return -1;
  }

  ENG_LOG("%s: eng_vlog reopen usb serial:%d\n", __FUNCTION__, *fd);
  return 0;
}

static void eng_get_agdsp_devpath(char* log_chan, char* pcm_chan, char* mem_chan)
{
    char property_name[32] = {0};

    sprintf(property_name,"%s","ro.modem.ag.log");
    property_get(property_name, log_chan, "not_find");
    ENG_LOG("%s %s log_chan:%s", __FUNCTION__,property_name,log_chan);

    sprintf(property_name,"%s","ro.modem.ag.pcm");
    property_get(property_name, pcm_chan, "not_find");
    ENG_LOG("%s %s pcm_chan:%s", __FUNCTION__,property_name,pcm_chan);

    sprintf(property_name,"%s","ro.modem.ag.mem");
    property_get(property_name, mem_chan, "not_find");
    ENG_LOG("%s %s mem_chan:%s", __FUNCTION__,property_name,mem_chan);
}

// Add SMP and message header
static void ag_dsplog_add_headers(uint8_t* buf, size_t len, unsigned type) {
    // SMP header first

    // FLAGS
    buf[0] = 0x7e;
    buf[1] = 0x7e;
    buf[2] = 0x7e;
    buf[3] = 0x7e;
    // LEN (length excluding FLAGS in little Endian)
    size_t pkt_len = len + 8 + 8;
    buf[4] = (uint8_t)(pkt_len);
    buf[5] = (uint8_t)(pkt_len >> 8);
    // CHANNEL
    buf[6] = 0;
    // TYPE
    buf[7] = 0;
    // RESERVED
    buf[8] = 0x5a;
    buf[9] = 0x5a;
    // Checksum (only cover LEN, CHANNEL, TYPE and RESERVED)
    uint32_t n = (uint32_t)(pkt_len) + 0x5a5a;
    n = (n & 0xffff) + (n >> 16);
    n = ~n;
    buf[10] = (uint8_t)(n);
    buf[11] = (uint8_t)(n >> 8);

    // MSG_HEAD_T

    // SN
    buf[12] = (uint8_t)(m_sn);
    buf[13] = (uint8_t)(m_sn >> 8);
    buf[14] = (uint8_t)(m_sn >> 16);
    buf[15] = (uint8_t)(m_sn >> 24);
    ++m_sn;

    // length
    pkt_len = len + 8;
    buf[16] = (uint8_t)(pkt_len);
    buf[17] = (uint8_t)(pkt_len >> 8);
    // type
    buf[18] = 0x9d;
    // subtype: AG-DSP
    buf[19] = (uint8_t)(0x40 | type);
}

static int send_data_to_host(int *fd_ptr, int r_cnt, eng_dev_info_t *dev_info, char *data)
{
    int offset = 0;  // reset the offset
    int w_cnt;
    int retry_num;
    int split_flag = 0;

    if (((r_cnt % 64) == 0) && dev_info->host_int.cali_flag &&
            (dev_info->host_int.dev_type == CONNECT_USB))
        split_flag = 1;
    do {
        if (split_flag)
            w_cnt = write(*fd_ptr, data + offset, r_cnt - 32);
        else
            w_cnt = write(*fd_ptr, data + offset, r_cnt);
        if (w_cnt < 0) {
            if (errno == EBUSY) {
                usleep(59000);
            } else {
                ENG_LOG("eng_vlog no log data write:%d ,%s\n", w_cnt,
                        strerror(errno));

                // FIX ME: retry to open
                retry_num = 0;  // reset the try number.
                while (-1 == restart_gser(fd_ptr, dev_info->host_int.dev_log)) {
                    ENG_LOG("eng_vlog open gser port failed\n");
                    sleep(1);
                    retry_num++;
                    if (retry_num > MAX_OPEN_TIMES) {
                        ENG_LOG("eng_vlog: vlog thread stop for gser error !\n");
                        sem_post(&g_armlog_sem);
                        return -1;
                    }
                }
            }
        } else {
            r_cnt -= w_cnt;
            offset += w_cnt;
            split_flag = 0;
            // ENG_LOG("eng_vlog: r_cnt: %d, w_cnt: %d, offset: %d\n", r_cnt, w_cnt,
            // offset);
        }
    } while (r_cnt > 0);
    return offset;
}

static int eng_retry_open_device(char *path, int mode) {
  int retry_num = 0;
  int fd;

  do {
      fd = open(path, mode);
      if (fd < 0) {
          ENG_LOG("eng_vlog cannot open %s, times:%d, error: %s\n",
                  path, retry_num, strerror(errno));

          if ((++retry_num) >= MAX_OPEN_TIMES) {
              ENG_LOG(
                      "eng_vlog SIPC open times exceed the max times, vlog thread "
                      "stopped.\n");
              return -1;
          }
          usleep(200*1000);
      }
  } while (fd < 0);
  return fd;

}

inline size_t agdsp_get_diag_data_len(size_t len) {
  size_t diag_len;

  // The max diag frame length is 65535
  if (len > 65535) {
      diag_len = 65535 - 20;
  } else {
      diag_len = len - 20;
  }

  return diag_len;
}

void* eng_agdsp_log_thread(void* x) {
  int ser_fd;
  int r_cnt = -1;
  int r_log_cnt = -1;
  int r_pcm_cnt = -1;
  int r_mem_cnt = -1;
  char ag_log_chan[ENG_DEV_PATH_LEN] = {0};
  char ag_pcm_chan[ENG_DEV_PATH_LEN] = {0};
  char ag_mem_chan[ENG_DEV_PATH_LEN] = {0};
  struct pollfd fds[3];
  int log_fd = -1;
  int pcm_fd = -1;
  int mem_fd = -1;
  int rbuf_size = 0;
  int poll_flag = 1; // 0:Start dump memory, poll ag_mem_chan one time; 1:Non dump state, loop poll
  int poll_num = 3;
  int timeout = -1;
  int ret = -1;

  eng_dev_info_t* dev_info = (eng_dev_info_t*)x;

  ENG_LOG("eng_agdsp_log thread start\n");

  /*open usb/uart*/
  ENG_LOG("eng_agdsp_log open serial...\n");
  ser_fd = eng_open_dev(dev_info->host_int.dev_log, O_WRONLY);
  if (ser_fd < 0) {
    ENG_LOG("eng_agdsp_log open serial failed, error: %s\n", strerror(errno));
    return NULL;
  }

  eng_get_agdsp_devpath(ag_log_chan, ag_pcm_chan, ag_mem_chan);

  /*open vbpipe/spipe*/
  ENG_LOG("eng_agdsp_log open SIPC channel...\n");
  log_fd = eng_retry_open_device(ag_log_chan, O_RDONLY);
  if (log_fd < 0) {
      goto out;
  }

  pcm_fd = eng_retry_open_device(ag_pcm_chan, O_RDONLY);
  if (pcm_fd < 0) {
      goto out;
  }

  mem_fd = eng_retry_open_device(ag_mem_chan, O_RDONLY);
  if (mem_fd < 0) {
      goto out;
  }

  fds[0].fd = log_fd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  fds[1].fd = pcm_fd;
  fds[1].events = POLLIN;
  fds[1].revents = 0;

  fds[2].fd = mem_fd;
  fds[2].events = POLLIN;
  fds[2].revents = 0;

  ENG_LOG("eng_agdsp_log put log data from SIPC to serial\n");
  while (1) {
    int r_offset = 0;
    int remain_len = DATA_BUF_SIZE;

    if (g_armlog_enable) {
        sem_post(&g_armlog_sem);
    }
    sem_wait(&g_armlog_sem);

    if(poll_flag) { // Normal case
        poll_num = 3;
        timeout = -1;
    } else { // AG-DSP asserted
        poll_num = 2;
        timeout = 0;
    }

    ret = poll(fds, poll_num, timeout);
    if (ret < 0 || (ret == 0 && timeout == -1)) {
        ENG_LOG("%s: ERROR: poll error(%s)\n", __FUNCTION__, strerror(errno));
        goto out;
    }

    if (fds[0].revents & POLLIN) {
        rbuf_size = agdsp_get_diag_data_len(remain_len);
        r_log_cnt = read(fds[0].fd, log_data + 20, rbuf_size);
        if (r_log_cnt > 0) {
            ag_dsplog_add_headers((uint8_t *)log_data, r_log_cnt, AUDIO_DSP_LOG);
            r_offset += r_log_cnt + 20;
            remain_len = DATA_BUF_SIZE - r_offset;
        } else {
            if (r_log_cnt < 0) {
                if (EAGAIN != errno && EINTR != errno)
                    ENG_LOG("%s: ERROR: read ag_log_chan error(%s)\n", __FUNCTION__, strerror(errno));
            } else {
                ENG_LOG("%s: ERROR: read ag_log_chan r_log_cnt=0\n", __FUNCTION__);
            }
        }
    }

    if (remain_len >= 2068 && (fds[1].revents & POLLIN)) {
        rbuf_size = agdsp_get_diag_data_len(remain_len);
        r_pcm_cnt = read(fds[1].fd, log_data + 20 + r_offset, rbuf_size);
        if (r_pcm_cnt > 0) {
            ag_dsplog_add_headers((uint8_t *)log_data + r_offset, r_pcm_cnt, AUDIO_DSP_PCM);
            r_offset += r_pcm_cnt + 20;
            remain_len = DATA_BUF_SIZE - r_offset;
        } else {
            if (r_pcm_cnt < 0) {
                if (EAGAIN != errno && EINTR != errno)
                    ENG_LOG("%s: ERROR: read ag_pcm_chan error(%s)\n", __FUNCTION__, strerror(errno));
            } else {
                ENG_LOG("%s: ERROR: read ag_pcm_chan r_pcm_cnt=0\n", __FUNCTION__);
            }
        }
    }

    if (remain_len >= 2068 && (!poll_flag || (fds[2].revents & POLLIN))) {
        rbuf_size = agdsp_get_diag_data_len(remain_len);
        r_mem_cnt = read(fds[2].fd, log_data + 20 + r_offset, rbuf_size);
        if(r_mem_cnt > 0) {
            ag_dsplog_add_headers((uint8_t *)log_data + r_offset, r_mem_cnt, AUDIO_DSP_MEM);
            r_offset += r_mem_cnt + 20;
            poll_flag = 0;
        } else {
            if (r_mem_cnt < 0) {
                poll_flag = 0;
                if (EAGAIN != errno && EINTR != errno) {
                    ENG_LOG("%s: ERROR: read ag_mem_chan error(%s)\n", __FUNCTION__, strerror(errno));
                    sleep(1);
                }
            } else {
                // Dump finished
                ioctl(fds[2].fd, DSPLOG_CMD_DSPASSERT, 0);
                poll_flag = 1;
                ENG_LOG("%s: AG-DSP memory dump finished\n", __FUNCTION__);
            }
        }
    }

    r_cnt = r_offset;

    //ENG_LOG("%s:%d r_cnt=%d\n", __FUNCTION__, __LINE__, r_cnt);

    if (r_cnt <= 0) {
        ENG_LOG("eng_agdsp_log read no log data : r_cnt=%d, %s\n", r_cnt,
                strerror(errno));
        continue;
    }

    if (send_data_to_host(&ser_fd, r_cnt, dev_info, log_data) < 0) {
        goto out;
    }
  }

out:
  ENG_LOG("eng_agdsp_log thread end\n");
  if (log_fd >= 0) close(log_fd);
  if (pcm_fd >= 0) close(pcm_fd);
  if (mem_fd >= 0) close(mem_fd);
  close(ser_fd);

  return 0;
}

void* eng_vlog_thread(void* x) {
  int ser_fd;
  int modem_fd;
  int r_cnt = -1;
  int dumpmemlen = 0;

  eng_dev_info_t* dev_info = (eng_dev_info_t*)x;

  ENG_LOG("eng_vlog thread start\n");

  /*open usb/uart*/
  ENG_LOG("eng_vlog open serial...\n");
  ser_fd = eng_open_dev(dev_info->host_int.dev_log, O_WRONLY);
  if (ser_fd < 0) {
    ENG_LOG("eng_vlog open serial failed, error: %s\n", strerror(errno));
    return NULL;
  }

  /*open vbpipe/spipe*/
  ENG_LOG("eng_vlog open SIPC channel...\n");
  modem_fd = eng_retry_open_device(dev_info->modem_int.log_chan, O_RDONLY);
  if (modem_fd < 0) {
    goto out;
  }

  ENG_LOG("eng_vlog put log data from SIPC to serial\n");
  while (1) {

    if (g_armlog_enable) {
      sem_post(&g_armlog_sem);
    }
    sem_wait(&g_armlog_sem);

    r_cnt = read(modem_fd, log_data, DATA_BUF_SIZE);

    if (r_cnt <= 0) {
      ENG_LOG("eng_vlog read no log data : r_cnt=%d, %s\n", r_cnt,
              strerror(errno));
      continue;
    }

    // printf dump memory len
    dump_mem_len_print(r_cnt, &dumpmemlen, log_data);

    if (send_data_to_host(&ser_fd, r_cnt, dev_info, log_data) < 0) {
        goto out;
    }

  }

out:
  ENG_LOG("eng_vlog thread end\n");
  if (modem_fd >= 0) close(modem_fd);
  close(ser_fd);

  return 0;
}

void* eng_vdiag_rthread(void* x) {
  int ser_fd, modem_fd, test_fd = -1;
  int r_cnt, w_cnt, offset;
  int retry_num = 0;
  int dumpmemlen = 0;
  int ret = -1;
  int flag = 0;
  s_dev_info = (eng_dev_info_t*)x;
  char get_propvalue[PROPERTY_VALUE_MAX] = {0};

  ENG_LOG("eng_vdiag_r thread start\n");

  /*open usb/uart*/
  ENG_LOG("eng_vdiag_r open serial...\n");
  ENG_LOG("eng_vdiag_r s_dev_info->host_int.dev_diag=%s\n",
          s_dev_info->host_int.dev_diag);
  ser_fd = eng_open_dev(s_dev_info->host_int.dev_diag, O_WRONLY);
  if (ser_fd < 0) {
    ENG_LOG("eng_vdiag_r open serial failed, error: %s\n", strerror(errno));
    return NULL;
  }

  if (s_dev_info->host_int.dev_type == CONNECT_UART) {
    set_raw_data_speed(ser_fd, 115200);
  }

  s_ser_diag_fd = ser_fd;

  /*open vbpipe/spipe*/
  ENG_LOG("eng_vdiag_r open SIPC channel...\n");
  do {
    modem_fd = open(s_dev_info->modem_int.diag_chan, O_RDONLY);
    if (modem_fd < 0) {
      if(0 == retry_num)
      ENG_LOG("eng_vdiag_r cannot open %s, error: %s\n",
              s_dev_info->modem_int.diag_chan, strerror(errno));

      if ((++retry_num) >= MAX_OPEN_TIMES) {
          ENG_LOG(
                  "eng_vdiag_r SIPC open times exceed the max times, vlog thread "
                  "stopped.\n");
          goto out;
      }
      usleep(200*1000);
    }
  } while (modem_fd < 0);
  ENG_LOG("eng_vdiag_r open %s success, the first %d times fail\n", s_dev_info->modem_int.diag_chan, retry_num);

  if (s_dev_info->host_int.cali_flag &&
      (s_dev_info->host_int.dev_type == CONNECT_USB) && !g_ap_cali_flag) {
    USB_DEVICE_SPEED_ENUM usb_device_speed = USB_SPEED_FULL;
    eng_usb_maximum_speed(usb_device_speed);
    eng_usb_enable();
  }

  ENG_LOG("eng_vdiag_r put log data from SIPC to serial\n");
  while (1) {
    r_cnt = read(modem_fd, diag_data, DATA_BUF_SIZE);
    if (r_cnt <= 0) {
      ENG_LOG("eng_vdiag_r read no log data : r_cnt=%d, %s\n", r_cnt,
              strerror(errno));
      continue;
    }
    if (CONNECT_UART == s_dev_info->host_int.dev_type) {
      property_set("sys.config.engcplog.enable", "1");
    }
    property_get("sys.config.engcplog.enable", get_propvalue, "not_find");
    if ((0 == strcmp(get_propvalue, "1")) &&
        (1 == s_dev_info->host_int.cali_flag)) {
      ENG_LOG("%s sys.config.engcplog.enable= %s\n", __FUNCTION__,
              get_propvalue);
      if (flag == 0) {
        ret = create_log_dir();
        if (!ret) {
          flag = 1;
          test_fd = open_log_path();
          if (test_fd < 0) {
            ENG_LOG("eng_vdiag_r cannot open %s.\n", external_path);
          }
        }
      }

      if (test_fd >= 0) {
        ret = eng_write_data_to_file(diag_data, r_cnt, test_fd);
        if (ret < 0) {
          ENG_LOG("eng_vdiag_r write to logfile failed\n");
        }
      }

      ENG_LOG("%s: r_cnt=%d\n", __FUNCTION__, r_cnt);
      eng_filter_calibration_log_diag(diag_data, r_cnt, &ser_fd);
    } else {
      // printf dump memory len
      dump_mem_len_print(r_cnt, &dumpmemlen, diag_data);
      if (0 == eng_diag_write2pc_ptr(diag_data, r_cnt, &ser_fd)) {
        close(modem_fd);
        return 0;
      }
    }
  }

out:
  ENG_LOG("eng_vdiag_r thread end\n");
  if (modem_fd >= 0) close(modem_fd);
  if (test_fd >= 0) close(test_fd);
  close(ser_fd);

  return 0;
}
static eng_dev_info_t* vdev_info;
void report_nmea_log(const char* nmea, int length) {
  int r_cnt = 0, w_cnt = 0, offset = 0;
  int retry_num = 0;
  char tmpbuf[GPS_DATA_BUF_SIZE / 2] = {0};
  int ser_fd;
  MSG_HEAD_T* msg_head = (MSG_HEAD_T*)tmpbuf;
  msg_head->type = 0x3A;
  msg_head->subtype = 0x5;
  ENG_LOG("%s: gps log thread start \n", __FUNCTION__);

  r_cnt = length;
  ser_fd = get_ser_diag_fd();

  if (r_cnt > 0) {
    memcpy(tmpbuf + sizeof(MSG_HEAD_T), nmea, r_cnt);
    msg_head->len = r_cnt + sizeof(MSG_HEAD_T);
    r_cnt = translate_packet(gps_log_data, tmpbuf, msg_head->len);

    offset = 0;
    w_cnt = 0;
    do {
      w_cnt = write(ser_fd, gps_log_data + offset, r_cnt);
      if (w_cnt < 0) {
        if (errno == EBUSY) {
          usleep(59000);
        } else {
          retry_num = 0;
          while (-1 == restart_gser(&ser_fd, vdev_info->host_int.dev_log)) {
            ENG_LOG("eng_gps_log open ser port failed\n");
            sleep(1);
            retry_num++;
            if (retry_num > MAX_OPEN_TIMES) {
              ENG_LOG("eng_gps_log: thread stop for open ser error!\n");
              return;
            }
          }
        }
      } else {
        r_cnt -= w_cnt;
        offset += w_cnt;
      }
    } while (r_cnt > 0);
  }
}
#ifndef CONFIG_MINIENGPC
void* eng_gps_log_thread(void* x) {
  vdev_info = (eng_dev_info_t*)x;
  set_report_ptr(report_nmea_log);

  return 0;
}
#endif
int create_log_dir() {
  time_t when;
  struct tm start_tm;
  char path[MAX_NAME_LEN];
  char* p;
  int type;
  int ret = 0;
  char value[PROPERTY_VALUE_MAX];

  when = time(NULL);
  localtime_r(&when, &start_tm);

  memset(external_path, 0, MAX_NAME_LEN);
  p = getenv("SECONDARY_STORAGE");

  if (p) {
    type = atoi(p);
    p = NULL;
    if (type == 0 || type == 1) {
      p = getenv("EXTERNAL_STORAGE");
    } else if (type == 2) {
      p = getenv("SECONDARY_STORAGE");
    }
    if (p) {
      sprintf(top_logdir, "%s/armlog", p);
      ENG_LOG("%s:the top_logdir : %s", __FUNCTION__, top_logdir);
    } else {
      ENG_LOG(
          "%s:SECOND_STORAGE_TYPE is %d, but can't find the external storage "
          "environment",
          __FUNCTION__, type);
    }
  }

  property_get("persist.storage.type", value, "3");
  type = atoi(value);
  if (type == 0 || type == 1 || type == 2) {
    p = NULL;
    if (type == 0 || type == 1) {
      p = getenv("EXTERNAL_STORAGE");
    } else if (type == 2) {
      p = getenv("SECONDARY_STORAGE");
    }
    if (p) {
      sprintf(top_logdir, "%s/armlog", p);
      ENG_LOG("%s:the top_logdir : %s", top_logdir);
    } else {
      ENG_LOG(
          "%s:SECOND_STORAGE_TYPE is %d, but can't find the external storage "
          "environment",
          __FUNCTION__, type);
    }
  }

  p = getenv("SECONDARY_STORAGE");
  if (p == NULL) p = getenv("EXTERNAL_STORAGE");
  if (p == NULL) {
    ENG_LOG("%s:Can't find the external storage environment", __FUNCTION__);
  }
  sprintf(top_logdir, "%s/armlog", p);
  ret = mkdir(top_logdir, S_IRWXU | S_IRWXG | S_IRWXO);
  if (-1 == ret && (errno != EEXIST)) {
    ENG_LOG("%s:mkdir %s failed.error: %s\n", __FUNCTION__, top_logdir,
            strerror(errno));
    return -1;
  }
  sprintf(external_path, "%s/cp0_20%02d-%02d-%02d-%02d-%02d-%02d.log",
          top_logdir, start_tm.tm_year % 100, start_tm.tm_mon + 1,
          start_tm.tm_mday, start_tm.tm_hour, start_tm.tm_min, start_tm.tm_sec);
  ENG_LOG("%s:the external_path : %s", __FUNCTION__, external_path);

  return 0;
}

int open_log_path() {
  int test_fd = -1;
  int retry_num = 0;

  test_fd = open(external_path, O_CREAT | O_RDWR | O_TRUNC, 0666);
  if (test_fd < 0) {
    ENG_LOG("eng_write_data_to_file() cannot open %s, error: %s\n",
            external_path, strerror(errno));
  }

  return test_fd;
}

int eng_write_data_to_file(char* diag_data, int r_cnt, int test_fd) {
  int offset = 0;
  int w_cnt = 0;
  int retry_num = 0;

  do {
    w_cnt = write(test_fd, diag_data + offset, r_cnt);
    if (w_cnt <= 0) {
      ENG_LOG(
          "eng_write_data_to_file() write to test file no diag data : "
          "w_test_cnt=%d, %s\n",
          w_cnt, strerror(errno));
      return -1;
    } else {
      r_cnt -= w_cnt;
      offset += w_cnt;
      ENG_LOG("%s w_cnt = %d,r_cnt = %d.\n", __FUNCTION__, w_cnt, r_cnt);
    }
  } while (r_cnt > 0);

  fsync(test_fd);
  return 0;
}

int eng_is_send_to_usb(char* buf, int len) {
  int i = 0;
  int ret = CMD_COMMON;
  MSG_HEAD_T* head_ptr = NULL;
  char diag_header[20];
  char* tmp = buf;

  if (*tmp == 0x7e) {
    while (*(++tmp) == 0x7e) i++;
    if (i) {
      ENG_LOG("%s: before diag decode7d7e *tmp=0x%x\n", __FUNCTION__, *tmp);
    }
  }

  memcpy(diag_header, tmp, 20);
  eng_diag_decode7d7e(diag_header, 20);
  head_ptr = (MSG_HEAD_T*)(diag_header);

  ENG_LOG("%s: cmd=0x%x; subcmd=0x%x\n", __FUNCTION__, head_ptr->type,
          head_ptr->subtype);

  // decide the diag is armlog or dsplog
  if ((0x98 == head_ptr->type) || (0x9D == head_ptr->type) ||
      (0xF8 == head_ptr->type))
    return 0;

  return 1;
}

int eng_diag_write2pc_ptr(char* diag_data, int r_cnt, int *fd_ptr) {
  int offset = 0;  // reset the offset
  int w_cnt = 0;
  int split_flag = 0;
  int retry_num = 0;
  int i = 0;

  if (((r_cnt % 64) == 0) &&
      (s_dev_info->host_int.cali_flag || g_autotest_flag) &&
      (s_dev_info->host_int.dev_type == CONNECT_USB))
    split_flag = 1;
  do {
    if (split_flag)
      w_cnt = write(*fd_ptr, diag_data + offset, r_cnt - 32);
    else
      w_cnt = write(*fd_ptr, diag_data + offset, r_cnt);
    if (w_cnt < 0) {
      if (errno == EBUSY) {
        usleep(59000);
      } else {
        ENG_LOG("eng_vdiag_r no log data write:%d ,%s\n", w_cnt,
                strerror(errno));

        // FIX ME: retry to open
        retry_num = 0;  // reset the try number.
        while (-1 == restart_gser(fd_ptr, s_dev_info->host_int.dev_diag)) {
          ENG_LOG("eng_vdiag_r open gser port failed\n");
          sleep(1);
          retry_num++;
          if (retry_num > MAX_OPEN_TIMES) {
            ENG_LOG("eng_vdiag_r: vlog thread stop for gser error !\n");
            return 0;
          }
        }
        s_ser_diag_fd = *fd_ptr;
      }
    } else {
      r_cnt -= w_cnt;
      offset += w_cnt;
      split_flag = 0;
      // ENG_LOG("eng_vdiag: r_cnt: %d, w_cnt: %d, offset: %d\n", r_cnt, w_cnt,
      // offset);
    }
  } while (r_cnt > 0);

  return offset;
}

int eng_diag_write2pc(char* diag_data, int r_cnt, int fd) {
  int offset = 0;  // reset the offset
  int w_cnt = 0;
  int split_flag = 0;
  int retry_num = 0;
  int i = 0;

  if (((r_cnt % 64) == 0) &&
      (s_dev_info->host_int.cali_flag || g_autotest_flag) &&
      (s_dev_info->host_int.dev_type == CONNECT_USB))
    split_flag = 1;
  do {
    if (split_flag)
      w_cnt = write(fd, diag_data + offset, r_cnt - 32);
    else
      w_cnt = write(fd, diag_data + offset, r_cnt);
    if (w_cnt < 0) {
      if (errno == EBUSY) {
        usleep(59000);
      } else {
        ENG_LOG("eng_vdiag_r no log data write:%d ,%s\n", w_cnt,
                strerror(errno));

        // FIX ME: retry to open
        retry_num = 0;  // reset the try number.
        while (-1 == restart_gser(&fd, s_dev_info->host_int.dev_diag)) {
          ENG_LOG("eng_vdiag_r open gser port failed\n");
          sleep(1);
          retry_num++;
          if (retry_num > MAX_OPEN_TIMES) {
            ENG_LOG("eng_vdiag_r: vlog thread stop for gser error !\n");
            return 0;
          }
        }
        s_ser_diag_fd = fd;
      }
    } else {
      r_cnt -= w_cnt;
      offset += w_cnt;
      split_flag = 0;
      // ENG_LOG("eng_vdiag: r_cnt: %d, w_cnt: %d, offset: %d\n", r_cnt, w_cnt,
      // offset);
    }
  } while (r_cnt > 0);

  return offset;
}

void eng_filter_calibration_log_diag(char* diag_data, int r_cnt, int *fd_ptr) {
  int processlen = 0;
  int remainlen = 0;
  int log_state = ENG_LOG_NO_WAIT;
  int ret = -1;
  char* tmp = diag_data;

  do {
    if (log_state == ENG_LOG_WAIT_END &&
        *tmp == 0x7e) {  // the start is 0x7e and the last data is not a
                         // complete diag
      goto FRAME;
    }

    while (*tmp == 0x7e) {
      ext_log_data_buf[ext_log_buf_len++] = *tmp;
      tmp++;  // skip header 0x7e
    }

    // All the left bytes are 0x7E,so don't wait end byte.
    if ((int)(tmp - diag_data) == r_cnt) break;

    while (*tmp != 0x7e) {
      remainlen = (int)(tmp - diag_data);
      if (r_cnt == remainlen) {
        break;  // the last is not the 0x7e
      }
      ext_log_data_buf[ext_log_buf_len++] = *tmp;
      tmp++;  // find the end 0x7e
    }

    if (*tmp != 0x7e) {
      log_state = ENG_LOG_WAIT_END;  // continue to receive,wait for the end
                                     // 0x7e,herit on the last break
      break;
    } else {
    FRAME:
      ext_log_data_buf[ext_log_buf_len++] = *tmp;
      ENG_LOG("%s: ext_buf_len=%d\n", __FUNCTION__, ext_log_buf_len);
      if (eng_is_send_to_usb(ext_log_data_buf, ext_log_buf_len)) {
        eng_diag_write2pc_ptr(ext_log_data_buf, ext_log_buf_len,
                          fd_ptr);  // the start series 0x7e will send to the
                                    // usb/uart,tools can parse it
      }
      if (log_state == ENG_LOG_WAIT_END) {
        processlen = (int)(tmp - diag_data) + 1;
        log_state = ENG_LOG_NO_WAIT;
      } else {
        processlen += ext_log_buf_len;
      }
      memset(ext_log_data_buf, 0, ext_log_buf_len);
      ext_log_buf_len = 0;
      if (processlen != r_cnt) {
        tmp++;
      }
    }
  } while (processlen != r_cnt);

  return;
}
