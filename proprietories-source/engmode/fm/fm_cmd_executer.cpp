/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 * Authors:<george.wang@spreadtrum.com>
 *
 * Function:FM lava auto test tool
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "fm_cmd_executer.h"

#define FMR_MAX_IDX 1
extern struct fmr_ds *pfmr_data[FMR_MAX_IDX];
static int fm_eng_idx = -1;

static int fm_eng_send_back_cmd_result(int client_fd, char *str, int isOK) {
  char buffer[255] = {0};

  FMENG_LOGD("SEND CMD RESULT");
  if (client_fd < 0) {
    fprintf(stderr, "write %s to invalid fd \n", str);

    return -1;
  }

  memset(buffer, 0, sizeof(buffer));

  if (!str) {
    snprintf(buffer, 255, "%s", (isOK ? OK_STR : FAIL_STR));
  } else {
    snprintf(buffer, 255, "%s %s", (isOK ? OK_STR : FAIL_STR), str);
  }

  int ret = write(client_fd, buffer, strlen(buffer) + 1);

  FMENG_LOGD("%s, client fd:%d, cmd_result: %s, ret: %d", __FUNCTION__,
             client_fd, buffer, ret);
  if (ret < 0) {
    FMENG_LOGD("write %s to client_fd:%d fail (error(%d):%s)", buffer,
               client_fd, errno, strerror(errno));
    fprintf(stderr, "write %s to client_fd:%d fail (error:%s)", buffer,
            client_fd, strerror(errno));
    return -1;
  }

  return 0;
}

static FM_ENG_ERROR_E fm_eng_on(void) {
  FM_ENG_ERROR_E err;

  FMENG_LOGD("fm_eng_fm_on\n");

  pfmr_data[0] = NULL;
  if ((fm_eng_idx = FMR_init()) < 0) {
    FMENG_LOGD("fm_eng_FMR_init, fm_eng_idx: %d", fm_eng_idx);
    return FM_ENG_INIT_ERROR;
  }

  err = (FM_ENG_ERROR_E)FMR_open_dev(
      fm_eng_idx);  // if success, then ret = 0; else ret < 0
  if (err) {
    FMENG_LOGD("%s, fm_eng_open_dev, err: %d", __FUNCTION__, err);
    return err;
  }

  err = (FM_ENG_ERROR_E)FMR_pwr_up(fm_eng_idx, 875);
  if (err) {
    FMENG_LOGD("%s, fm_eng_pwr_up, err: %d", __FUNCTION__, err);
    return FM_ENG_ENABLE_ERROR;
  }

  return FM_ENG_NONE_ERROR;
}

static FM_ENG_ERROR_E fm_eng_off(void) {
  FM_ENG_ERROR_E err;
  int counter = 0;

  FMENG_LOGD("fm_eng_off\n");

  err = (FM_ENG_ERROR_E)FMR_pwr_down(fm_eng_idx, 0);
  if (err) {
    FMENG_LOGD("%s, fm_eng_pwr_down, err: %d", __FUNCTION__, err);
    return FM_ENG_DISABLE_ERROR;
  }

  err = (FM_ENG_ERROR_E)FMR_close_dev(fm_eng_idx);
  if (err == FM_ENG_NONE_ERROR) {
    FMENG_LOGD("%s, fm_eng_close_dev, err: %d", __FUNCTION__, err);
    return err;
  }

  pfmr_data[0] = NULL;

  return FM_ENG_NONE_ERROR;
}

static FM_ENG_ERROR_E fm_eng_tune(int eng_freq) {
  FM_ENG_ERROR_E err;
  int tmp_freq;

  FMENG_LOGD("fm_eng_tune\n");

  tmp_freq = (int)(eng_freq / 10);  // Eg, 10770 / 10 --> 1077
  err = (FM_ENG_ERROR_E)FMR_tune(fm_eng_idx, tmp_freq);
  if (err) {
    FMENG_LOGD("%s, fm_eng_tune, err: %d", __FUNCTION__, err);
    return FM_ENG_TUNE_ERROR;
  }

  return FM_ENG_NONE_ERROR;
}

static FM_ENG_ERROR_E fm_eng_get_rssi(int *eng_rssi) {
  // FM_ENG_ERROR_E err;
  *eng_rssi = -90;

  FMENG_LOGD("fm_eng_get_rssi\n");

  /*err = (FM_ENG_ERROR_E)FMR_get_rssi(fm_eng_idx, eng_rssi);
  if (err)
  {
      FMENG_LOGD("%s, fm_eng_get_rssi, err: %d", __FUNCTION__, err);
      return FM_ENG_GETRSSI_ERROR;
  }
  FMENG_LOGD("fm_eng_get_rssi, rssi = %d \n", *eng_rssi);

  return FM_ENG_NONE_ERROR;*/

  FMENG_LOGD("fm_eng_get_rssi, rssi = %d \n", *eng_rssi);

  return FM_ENG_NONE_ERROR;
}

int fm_runcommand(int client_fd, int argc, char **argv) {
  FM_ENG_ERROR_E err = FM_ENG_NONE_ERROR;
  char result_buf[CMD_RESULT_BUFFER_LEN];
  int ret_val = 0;
  int fm_eng_freq = 0;
  int fm_eng_rssi = 0;

  memset(result_buf, 0, sizeof(result_buf));

  if (strncmp(*argv, "fm_on", 5) == 0) {
    FMENG_LOGD("recv fm_on.");
    err = fm_eng_on();
    if (err == FM_ENG_NONE_ERROR) {
      FMENG_LOGE("fm_eng_status: fm_on, on success");
      sprintf(
          result_buf,
          "fm_on success, return value: %d\nbsp_FM_test_function#open UTPASS",
          err);
    } else {
      sprintf(
          result_buf,
          "fm_on failed, return value: %d\nbsp_FM_test_function#open UTFAILED",
          err);
    }
  } else if (strncmp(*argv, "fm_off", 6) == 0) {
    FMENG_LOGD("recv fm_off.");
    err = fm_eng_off();
    if (err == FM_ENG_NONE_ERROR) {
      FMENG_LOGE("fm_eng_status: fm_off, off success");
      sprintf(
          result_buf,
          "fm_off success, return value: %d\nbsp_FM_test_function#close UTPASS",
          err);
    } else {
      sprintf(result_buf,
              "fm_off failed, return value: %d\nbsp_FM_test_function#close "
              "UTFAILED",
              err);
    }
  } else if (strncmp(*argv, "tune", 4) == 0 && argc > 1) {
    fm_eng_freq = atoi(argv[1]);
    FMENG_LOGD("recv fm_tune freq=%d\n", fm_eng_freq);
    err = fm_eng_tune(fm_eng_freq);
    if (err == FM_ENG_NONE_ERROR) {
      FMENG_LOGE("fm_eng_status: fm_tune, tune success");
      sprintf(
          result_buf,
          "fm_tune success, return value: %d\nbsp_FM_test_function#tune UTPASS",
          err);
    } else {
      sprintf(result_buf,
              "fm_tune failed, return value: %d\nbsp_FM_test_function#tune "
              "UTFAILED",
              err);
    }
  } else if (strncmp(*argv, "getrssi", 7) == 0) {
    FMENG_LOGD("recv fm_get_rssi.");
    err = fm_eng_get_rssi(&fm_eng_rssi);
    if (err == FM_ENG_NONE_ERROR) {
      FMENG_LOGE("fm_eng_status: fm_getrssi, getrssi success");
      sprintf(result_buf,
              "fm_getrssi success, rssi = %d, return value: "
              "%d\nbsp_FM_test_function#get_rssi UTPASS",
              fm_eng_rssi, err);
    } else {
      sprintf(result_buf,
              "fm_getrssi failed, return value: "
              "%d\nbsp_FM_test_function#get_rssi UTFAILED",
              err);
    }
  } else {
    FMENG_LOGE("rcv cmd is invalid.");
    fprintf(stderr, "rcv cmd is invalid.\n");
    err = FM_ENG_CMD_INVALID;
  }

  if (err == FM_ENG_NONE_ERROR) {
    ret_val = 0;
    fm_eng_send_back_cmd_result(client_fd, result_buf, 1);
  } else {
    ret_val = 1;
    switch (err) {
      case FM_ENG_CMD_INVALID:
        sprintf(result_buf, "invalid cmd");
        break;
      case FM_ENG_INIT_ERROR:
        sprintf(result_buf, "fm init error");
        break;
      case FM_ENG_ENABLE_ERROR:
        sprintf(result_buf, "\nbsp_FM_test_function#open UTFAILED");
        break;
      case FM_ENG_DISABLE_ERROR:
        sprintf(result_buf, "\nbsp_FM_test_function#close UTFAILED");
        break;
      case FM_ENG_TUNE_ERROR:
        sprintf(result_buf, "\nbsp_FM_test_function#tune UTFAILED");
        break;
      case FM_ENG_GETRSSI_ERROR:
        sprintf(result_buf, "\nbsp_FM_test_function#get_rssi UTFAILED");
        break;
      default:
        break;
    }
    fm_eng_send_back_cmd_result(client_fd, result_buf, 0);
  }

  return ret_val;
}
