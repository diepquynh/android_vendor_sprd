

#ifndef _BT_RDA_H_
#define _BT_RDA_H_

#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include "bt_hci_bdroid.h"
#include "bt_vendor_lib.h"

#define  LOG_TAG  "BT_rda"
#include <cutils/log.h>


#define RDA_BT_DEVICE_PATH              "/dev/rda_bt_dev"  


#define BT_DEV_MAJOR_NUM                'u'  	    
#define RDA_BT_LDO_ON_HIGH_IOCTL    _IO(BT_DEV_MAJOR_NUM, 100)
#define RDA_BT_LDO_ON_LOW_IOCTL  _IO(BT_DEV_MAJOR_NUM, 101)

#define IOCTL_BT_SET_EINT       	    _IO(BT_DEV_MAJOR_NUM, 102)


#define RDABT_5876             (0)
#define WLAN_VERSION_90_D (1)
#define WLAN_VERSION_90_E (2)
#define WLAN_VERSION_91   (3)
#define WLAN_VERSION_91_E (4)
#define WLAN_VERSION_91_F (5)
#define WLAN_VERSION_91_G (6)

#define DBG_PRINT(fmt, arg...)      { SLOGD("%s: "  fmt, __FUNCTION__ ,##arg);}
#define INFO_PRINT(fmt, arg...)     { SLOGI("%s: "  fmt, __FUNCTION__ ,##arg);}
#define WARN_PRINT(fmt, arg...)     { SLOGW("%s: "  fmt, __FUNCTION__ ,##arg);}
#define ERR_PRINT(fmt, arg...)      { SLOGE("%s: "  fmt, __FUNCTION__ ,##arg);}
#define TRC_PRINT(f)                { SLOGD("<%s> <%d>\n", __FUNCTION__, __LINE__);}

/*****************    APIs    ******************/
void set_callbacks(const bt_vendor_callbacks_t* p_cb);
void clean_callbacks(void);
int set_bluetooth_power(int on);
int init_uart(void);
void close_uart(void);
int rda_fw_cfg(void);
int rda_sco_cfg(void);
int rda_sleep_cfg(void);
int rda_wake_chip(uint8_t wake_assert);

void rdabt_write_memory(int fd,__u32 addr,__u32 *data,__u8 len,__u8 memory_type);
void RDA_uart_write_array(int fd,const __u32 buf[][2],__u16 len,__u8 type);
int  RDA_pin_to_high(int fd);
int  RDA_pin_to_low(int fd);
void rdabt_write_pskey(int fd,unsigned char id, const unsigned int *data,unsigned char len);

//int rda_setup_flow_ctl(int fd);
int rda_change_baudrate(int fd);
void RDABT_Write_Bt_Address(int s_fd,int d_fd);
int RDABT_core_Intialization(int fd);
int rda_init(int fd, int iBaudrate);
int rda_bt_baudrate_map(int iBaudrate);

// RDA5876
int rda5876_setup_flow_ctl(int fd);
void rda5876_change_baudrate(int fd);
void RDA5876_RfInit(int fd);
void RDA5876_Pskey_RfInit(int fd);
void RDA5876_Dccal(int fd);
void RDA5876_Trap(int fd);
void RDA5876_Pskey_Misc(int fd);


extern unsigned int rdabt_chip_version;

#define EM_NOT_RXTX_TEST	0
#define EM_RX_TEST		1
#define EM_TX_TEST		2
extern unsigned char EM_type;
#endif

