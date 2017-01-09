/*
 *  wiq_drv_api.h - WCDMA I/Q driver API.
 *
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *  History:
 *
 *  2015-7-24 Zhang Ziyi
 *  Initial version.
 *
 *  2015-9-24 Zheng Wanhu
 *  WCDMA I/Q playback API added.
 */

#ifndef WIQ_DRV_API_H_
#define WIQ_DRV_API_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// WCDMA I/Q capture definitions

#define WCDMA_IQ_BUF_SIZE (128 * 1024 * 1024)
#define WCDMA_IQ_BUF_HEADER_LEN 256
#define WCDMA_IQ_BUF_DATA_LEN (WCDMA_IQ_BUF_SIZE - WCDMA_IQ_HEADER_LEN)

#define IQ_BUF_INIT                0x5A5A5A5A
#define IQ_BUF_WRITE_FINISHED      0x5A5A8181
#define IQ_BUF_READ_FINISHED       0x81815A5A
#define IQ_BUF_READING             0x81005a00

// WCDMA I/Q playback definitions

#define MAX_CHAR_NUM               128

#define IQ_BUF_OPEN                0x424F504E
#define IQ_BUF_LOCK                0x424C434B
#define DATA_AP_MOVE               0x4441504D
#define DATA_AP_MOVING             0x504D4441
#define DATA_CP_INJECT             0x44435049
#define DATA_AP_MOVE_FINISH        DATA_CP_INJECT
#define DATA_RESET                 0x44525354
#define MAX_PB_HEADER_SIZE		   0x100

// ioctl types
typedef enum
{
	CMD_GET_IQ_BUF_INFO = 0x0,	//获取buffer的信息
	CMD_GET_IQ_PB_INFO,		//get iq_pb_data_header
	CMD_SET_IQ_CH_TYPE = 0x80,	//设置是走usb还是slog
	CMD_SET_IQ_WR_FINISHED,		//buffer写完成标志
	CMD_SET_IQ_RD_FINISHED,		//buffer读完成标志
	CMD_SET_IQ_MOVE_FINISHED	//set iq_pb_data_header
} ioctl_cmd_t;

typedef enum
{
	IQ_USB_MODE = 0,
	IQ_SLOG_MODE,
	PLAY_BACK_MODE,
} iq_mode_t;

// WCDMA I/Q capture header
struct iq_buf_info
{
	unsigned base_offs;
	unsigned data_len;
};

// WCDMA I/Q playback header
struct iq_pb_data_header
{
	uint32_t data_status;
	uint32_t iqdata_offset;
	uint32_t iqdata_length;
	char iqdata_filename[MAX_CHAR_NUM];
};

#ifdef __cplusplus
}
#endif

#endif  //!WIQ_DRV_API_H_
