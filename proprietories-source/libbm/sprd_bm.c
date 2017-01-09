/*
* Copyright (C) 2016 Spreadtrum Communications Inc.
*  sprd_bm.c
*
*  Created on: 2016-01-07
*  Author: Aiden.Chen
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sprd_bm.h>

#if defined(ANDROID)
#include <android/log.h>
#include <utils/Log.h>
#undef BM_LOG_TAG
#define BM_LOG_TAG "sprd_bm"
#define BM_LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  BM_LOG_TAG, fmt, ##args)
#define BM_LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, BM_LOG_TAG, fmt, ##args)
#define BM_LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, BM_LOG_TAG, fmt, ##args)
#endif

#define BM_DEV_NODE "/dev/sprd_axi_bm"
int bm_fd = -1;

int sprd_bm_prof_enable(void) {
  int ret;
  bm_fd = open(BM_DEV_NODE, O_WRONLY);
  if (bm_fd < 0) {
    BM_LOGE("bm device open fail");
  return -1;
  }
  ret = ioctl(bm_fd, BM_ENABLE, 0);
  if (ret)
    BM_LOGE("bm enable fail");
  ret = ioctl(bm_fd, BM_PROF_SET, 0);
  if (ret)
    BM_LOGE("bm enable prof mode fail");
  return ret;
}

int sprd_bm_prof_disable(void) {
  int ret;
  ret = ioctl(bm_fd, BM_PROF_CLR, 0);
  if (ret)
    BM_LOGE("bm disable prof mode fail");
  ret = ioctl(bm_fd, BM_DISABLE, 0);
  if (ret)
    BM_LOGE("bm disable fail");
  close(bm_fd);
  return ret;
}

unsigned short sprd_bm_get_chn_cnt(void) {
  unsigned short bm_cnt = 0;
  if (ioctl(bm_fd, BM_CHN_CNT, &bm_cnt))
    BM_LOGE("bm get bm cnt fail");
  return bm_cnt;
}

unsigned long sprd_bm_get_chn_name(int bm_index) {
  unsigned long i, cmd;
  if (bm_name[0].chn_name[0] == 0) {
    for (i = 0; i < 10; i++) {
      cmd = (BM_CHANNELS << BM_CHN_NAME_CMD_OFFSET) + i;
      if (ioctl(bm_fd, cmd, &bm_name[i].chn_name[0]))
        BM_LOGE("bm get bm name fail");
    }
  }
  return (unsigned long)&bm_name[bm_index].chn_name[0];
}

unsigned long sprd_bm_get_rdcnt(int bm_index) {
  if (ioctl(bm_fd, BM_RD_CNT, &bm_index))
    BM_LOGE("bm get bm rd cnt fail");
  return bm_index;
}

unsigned long sprd_bm_get_wrcnt(int bm_index) {
  if (ioctl(bm_fd, BM_WR_CNT, &bm_index))
    BM_LOGE("bm get bm wr cnt fail");
  return bm_index;
}

unsigned long sprd_bm_get_rdbw(int bm_index) {
  if (ioctl(bm_fd, BM_RD_BW, &bm_index))
    BM_LOGE("bm get bm rd bw fail");
  return bm_index;
}

unsigned long sprd_bm_get_wrbw(int bm_index) {
  if (ioctl(bm_fd, BM_WR_BW, &bm_index))
    BM_LOGE("bm get bm wr bw fail");
  return bm_index;
}

unsigned long sprd_bm_get_rdlatency(int bm_index) {
  if (ioctl(bm_fd, BM_RD_LATENCY, &bm_index))
    BM_LOGE("bm get bm rd latency fail");
  return bm_index;
}

unsigned long sprd_bm_get_wrlatency(int bm_index) {
  if (ioctl(bm_fd, BM_WR_LATENCY, &bm_index))
    BM_LOGE("bm get bm wr latency fail");
  return bm_index;
}

uint64_t sprd_bm_get_time(void) {
  uint64_t kernel_time = 0;
  if (ioctl(bm_fd, BM_KERNEL_TIME, &kernel_time))
    BM_LOGE("bm get bm kernel time fail");
  return kernel_time;
}

unsigned long sprd_bm_tmr_clr(void) {
  unsigned long tmr_gap;
  if (ioctl(bm_fd, BM_TMR_CLR, &tmr_gap))
    BM_LOGE("bm tmr cnt clr fail");
  return tmr_gap;
}

int sprd_bm_ver_get(void) {
  int bm_ver = 0;
  if (ioctl(bm_fd, BM_VER_GET, &bm_ver))
  BM_LOGE("bm tmr cnt clr fail");
  return bm_ver;
}

