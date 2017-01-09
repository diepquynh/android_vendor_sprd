/*
 *  wiq_drv_api.h - WCDMA I/Q driver API.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *  2015-7-24 Zhang Ziyi
 *  Initial version.
 */

#ifndef WIQ_DRV_API_H_
#define WIQ_DRV_API_H_

#define WCDMA_IQ_BUF_SIZE (128 * 1024 * 1024)

#define IQ_BUF_INIT 0x5A5A5A5A
#define IQ_BUF_WRITE_FINISHED 0x5A5A8181
#define IQ_BUF_READ_FINISHED 0x81815A5A
#define IQ_BUF_READING 0x81005a00

// ioctl command type
typedef enum {
  CMD_GET_IQ_BUF_INFO = 0x0,  // get buffer info
  CMD_SET_IQ_CH_TYPE = 0x80,  // set usb or slog
  CMD_SET_IQ_WR_FINISHED,     // write buffer finished
  CMD_SET_IQ_RD_FINISHED      // read buffer finished
} ioctl_cmd_t;

typedef enum {
  IQ_USB_MODE = 0,
  IQ_SLOG_MODE,
  PLAY_BACK_MODE,
} iq_mode_t;

struct iq_buf_info {
  unsigned base_offs;
  unsigned data_len;
};

// static char *path = "/dev/iq_mem";

#endif  //! WIQ_DRV_API_H_
