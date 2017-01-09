#ifndef __BT_CFG_H__
#define __BT_CFG_H__

//#include "tgt_ap_gpio_setting.h"



#define SERIAL_BOURATE_115200           (115200)
#define SERIAL_BOURATE_921600           (921600)
#define SERIAL_BOURATE_1500000          (1500000)
#define SERIAL_BOURATE_3000000          (3000000)

/* SERIAL BOURATE */
#define CUST_BT_SERIAL_BOURATE          SERIAL_BOURATE_115200


/* SERIAL FLOW CTRL */

#define FLOW_CTL_HW	    	0x0001
#define FLOW_CTL_SW	    	0x0002
#define FLOW_CTL_NONE   	0x0000
#define FLOW_CTL_MASK   	0x0003
#define CUST_BT_SERIAL_FLOW_CTRL        FLOW_CTL_NONE // 0: NOT FLOW CTRL, USE DMA; 1: HW FLOW CTRL;



/* use UART_RX_BREAK if HOST_WAKE gpio is not present */
#ifndef _TGT_AP_GPIO_BT_HOST_WAKE
#define RDA_UART_RX_BREAK   //cancel this option if don't use uart break
#endif /*_TGT_AP_GPIO_BT_HOST_WAKE*/



#define CUST_BT_SERIAL_PORT             "/dev/ttyS0"   //"/dev/sttybt0" //

/* Only Support By 5876 */
//#define EXTERNAL_32K                    //Open sleep mode 


#endif /* __BT_CFG_H__ */
