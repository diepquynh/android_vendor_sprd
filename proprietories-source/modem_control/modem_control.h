/**
 * modem_control.h ---
 *
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 */

#ifndef MODEM_CONTROL_H_
#define MODEM_CONTROL_H_

#include <utils/Log.h>

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "MODEM_CTRL"
#endif

#ifdef MODEM_DEBUG
#define MODEM_LOGIF ALOGD
#else
#define MODEM_LOGIF(x...)
#endif

#define MODEM_LOGE ALOGE
#define MODEM_LOGD ALOGD

#include <utils/Log.h>

#ifndef BOARD_EXTERNEL_MODEM
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "MODEM_CTRL"
#endif

#define MODEM_DEBUG
#ifdef MODEM_DEBUG
#define MODEM_LOGD ALOGD
#define MODEM_LOGE ALOGE
#else
#define MODEM_LOGD(x...)  // printf
#define MODEM_LOGE(x...)  // printf
#endif
#endif

#ifndef bool
#define bool int
#endif

#define MODEM_RADIO_TYPE "ro.radio.modemtype"


#define ASSERT_DEV_PROP "ro.modem.%s.assert"
#define DIAG_DEV_PROP "ro.modem.%s.diag"
#define LOG_DEV_PROP "ro.modem.%s.log"
#define TTY_DEV_PROP "ro.modem.%s.tty"
#define PROC_DEV_PROP "ro.modem.%s.dev"

#define PERSIST_MODEM_PROP "persist.modem.%s.nvp"
#define PERSIST_MODEM_PATH "ro.product.partitionpath"

#define MODEM_RESET_PROP "persist.sys.sprd.modemreset"

#define MODEM_BANK "modem"
#define DELTANV_BANK "deltanv"
#define DSP_BANK "dsp"
#define TGDSP_BANK "tgdsp"
#define GDSP_BANK  "gdsp"
#define LDSP_BANK "ldsp"
#define WARM_BANK "warm"
#define CMDLINE_BANK "cpcmdline"
#define WDOG_BANK "wdtirq"

#define FIXNV_BANK  "fixnv"
#define FIXNV1_BANK "fixnv1"
#define FIXNV2_BANK "fixnv2"

#define RUNNV_BANK "runnv"
#define RUNNV1_BANK "runtimenv1"
#define RUNNV2_BANK "runtimenv2"

#define MODEM_START "start"
#define MODEM_STOP "stop"

/* modem/dsp partition, in here, will config them enough big */
#define MODEM_SIZE (20 * 1024 * 1024)
#define DELTANV_SIZE (1 * 1024 * 1024)
#define TGDSP_SIZE (5 * 1024 * 1024)
#define LDSP_SIZE  (12 * 1024 * 1024)
#define WARM_SIZE  (5 * 1024 * 1024)
#define PMCP_SIZE    (1 * 1024 * 1024)

#define CPCMDLINE_SIZE (0x1000)


#define min(A, B) (((A) < (B)) ? (A) : (B))

#define TD_MODEM 0x3434
#define W_MODEM 0x5656
#define LTE_MODEM 0x7878

#define SOCKET_NAME_MODEM_CTL "control_modem"

#define MODEM_ALIVE "Modem Alive"
#define PREPARE_RESET "Prepare Reset"
#define MODEM_RESET "Modem Reset"

#define BOOT_LOAD 0x1
#define REBOOT_LOAD 0x2
#define ALWAYS_LOAD 0x3

#define MAX_PATH_LEN 128
#define MAX_ASSERT_INFO_LEN 256


#define mstrncat2(dst, str1, str2)\
do {\
  snprintf(dst, sizeof(dst), "%s%s", str1, str2);\
} while (0)
#define mstrncat3(dst, str1, str2, str3)\
do {\
  snprintf(dst, sizeof(dst), "%s%s%s", str1, str2, str3);\
} while (0)

typedef struct image_load_stru {
  char path_w[MAX_PATH_LEN + 1];
  char path_r[MAX_PATH_LEN + 1];
  int offset_in;
  int offset_out;
  uint size;
  uint is_cmdline;
#ifdef SECURE_BOOT_ENABLE
  uint is_secured; /* 0: normal image; 1: secure image; */
#endif
} IMAGE_LOAD_S;

typedef struct load_table_stru {
  IMAGE_LOAD_S modem;
  IMAGE_LOAD_S deltanv;
  IMAGE_LOAD_S dsp;
  IMAGE_LOAD_S gdsp;
  IMAGE_LOAD_S ldsp;
  IMAGE_LOAD_S warm;
  IMAGE_LOAD_S cmdline;
  IMAGE_LOAD_S nvitem;
  IMAGE_LOAD_S pmcp;
} LOAD_TABLE_S;

typedef struct load_value_stru {
  LOAD_TABLE_S load_table;
  char cp_start[128];
  char cp_stop[128];
  char pmcp_start[128];
  char pmcp_stop[128];
  char nv1_read[128];
  char nv2_read[128];
  char nv_write[128];
} LOAD_VALUE_S;

void* detect_sipc_modem(void *param);

void modemd_enable_busmonitor(bool bEnable);
void modemd_enable_dmc_mpu(bool bEnable);

#endif  // MODEM_CONTROL_H_

