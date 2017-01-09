/*
* Copyright (C) 2016 Spreadtrum Communications Inc.
*  sprd_bm.h
*
*  Created on: 2016-01-07
*  Author: Aiden.Chen
*/

#ifndef SPRD_BM_H_
#define SPRD_BM_H_

#define BM_CHN_NAME_CMD_OFFSET 0x7
enum sci_bm_cmd_index {
  BM_STATE = 0x0,
  BM_CHANNELS,
  BM_AXI_DEBUG_SET,
  BM_AHB_DEBUG_SET,
  BM_PERFORM_SET,
  BM_PERFORM_UNSET,
  BM_OCCUR,
  BM_CONTINUE_SET,
  BM_CONTINUE_UNSET,
  BM_DFS_SET,
  BM_DFS_UNSET,  // 10
  BM_PANIC_SET,
  BM_PANIC_UNSET,
  BM_BW_CNT_START,
  BM_BW_CNT_STOP,
  BM_BW_CNT_RESUME,
  BM_BW_CNT,  // 0x10
  BM_BW_CNT_CLR,
  BM_DBG_INT_CLR,
  BM_DBG_INT_SET,
  BM_DISABLE,
  BM_ENABLE,
  BM_PROF_SET,
  BM_PROF_CLR,
  BM_CHN_CNT,
  BM_RD_CNT,
  BM_WR_CNT,
  BM_RD_BW,
  BM_WR_BW,
  BM_RD_LATENCY,
  BM_WR_LATENCY,
  BM_KERNEL_TIME,  // 0x1f
  BM_TMR_CLR,
  BM_VER_GET,
  BM_CMD_MAX,
};

struct bm_id_name {
  unsigned char chn_name[18];
};
static struct bm_id_name bm_name[24] = {};

int sprd_bm_prof_enable(void);
int sprd_bm_prof_disable(void);
unsigned short sprd_bm_get_chn_cnt(void);
unsigned long sprd_bm_get_chn_name(int bm_index);
unsigned long sprd_bm_get_rdcnt(int bm_index);
unsigned long sprd_bm_get_wrcnt(int bm_index);
unsigned long sprd_bm_get_rdbw(int bm_index);
unsigned long sprd_bm_get_wrbw(int bm_index);
unsigned long sprd_bm_get_rdlatency(int bm_index);
unsigned long sprd_bm_get_rdlatency(int bm_index);
uint64_t sprd_bm_get_time(void);

#endif  // SPRD_BM_H_
