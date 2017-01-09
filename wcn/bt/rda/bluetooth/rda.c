#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>

#include "bt_cfg.h"
#include "bt_rda.h"



extern int rda_init(int fd, int iBaudrate);
extern void rda_setup_flow_ctl(int fd);
extern int RDABT_core_Intialization(int fd);
static int setup_uart_param(int fd,int iBaudrate,int iFlowControl);
/**************************************************************************
 *                 G L O B A L   V A R I A B L E S                        *
***************************************************************************/

bt_vendor_callbacks_t *bt_vnd_cbacks = NULL;
static int     bt_fd = -1;
static uint8_t chipstate = 1;

/**************************************************************************
  *                       F U N C T I O N S                               *
***************************************************************************/


/* Register callback functions to libbt-hci.so */
void set_callbacks(const bt_vendor_callbacks_t* p_cb)
{
     bt_vnd_cbacks = p_cb;
}

/* Cleanup callback functions previously registered */
void clean_callbacks()
{
     bt_vnd_cbacks = NULL;
}

extern unsigned int rdabt_chip_version;
/* Set Bluetooth chip power */
int set_bluetooth_power(int on)
{
    int fd = -1;
    int ret = 0;

    fd = open(RDA_BT_DEVICE_PATH, O_RDWR);
    
    if (fd < 0) 
    {
        ERR_PRINT("[###### RDA BT #######]Open %s to set BT power fails: %s(%d)", RDA_BT_DEVICE_PATH, strerror(errno), errno);
        
        goto out;
    }
    
    if(on){
    		ret=RDA_pin_to_high(fd);
		DBG_PRINT("set_bluetooth_power:  on, ret = %d", ret);
    }else{
   		ret=RDA_pin_to_low(fd);
    	DBG_PRINT("set_bluetooth_power: off, ret = %d", ret);
    }

    if(ret < 0) 
    {
        ERR_PRINT("[###### RDA BT #######]Set BT power %d fails: %s(%d)\n", on, strerror(errno), errno);
    }

out:
    if (fd >= 0)
        close(fd);
        
    return ret;
}

/* Initialize UART port */
int init_uart(void)
{
    int fd = -1;
    
    DBG_PRINT("init_uart");

    fd = open(CUST_BT_SERIAL_PORT, O_RDWR | O_NOCTTY);
    
    if (fd < 0) 
    {
        ERR_PRINT("Can't open serial port\n");
        
        return -1;
    }

#if (CUST_BT_SERIAL_FLOW_CTRL == FLOW_CTL_HW)
	if(setup_uart_param(fd, 115200, FLOW_CTL_HW) <0)
#elif(CUST_BT_SERIAL_FLOW_CTRL == FLOW_CTL_NONE)
    if(setup_uart_param(fd, 115200, FLOW_CTL_NONE) <0)
#endif
    {
        ERR_PRINT("Can't set serial port\n");
        close(fd);
        return -1;
    }

    if (rda_init(fd, CUST_BT_SERIAL_BOURATE) < 0)
    {
    	close(fd);
    	return -1;	
    }
#if (CUST_BT_SERIAL_FLOW_CTRL == FLOW_CTL_HW)
    if(setup_uart_param(fd, CUST_BT_SERIAL_BOURATE, FLOW_CTL_HW) <0)
#elif(CUST_BT_SERIAL_FLOW_CTRL == FLOW_CTL_NONE)
    if(setup_uart_param(fd, CUST_BT_SERIAL_BOURATE, FLOW_CTL_NONE) <0)
#endif
    {
        ERR_PRINT("Can't set serial port\n");
        close(fd);
        return -1;
    }

    bt_fd = fd;
   
    return bt_fd;
}

/* Close UART port previously opened */
void close_uart()
{
    if (bt_fd >= 0){
        close(bt_fd);
    }
    bt_fd = -1;
}


static int uart_speed(int s)
{
    switch (s) 
    {
    case 9600:
         return B9600;
    case 19200:
         return B19200;
    case 38400:
         return B38400;
    case 57600:
         return B57600;
    case 115200:
         return B115200;
    case 230400:
         return B230400;
    case 460800:
         return B460800;
    case 500000:
         return B500000;
    case 576000:
         return B576000;
    case 921600:
         return B921600;
    case 1000000:
         return B1000000;
    case 1152000:
         return B1152000;
    case 1500000:
         return B1500000;
    case 2000000:
         return B2000000;
    case 3000000:
         return B3000000;
    case 4000000:
         return B4000000;
    default:
         return B57600;
    }
}


/* host uart param configuration callback */
int setup_uart_param(int fd,int iBaudrate,int iFlowControl)
{
    struct termios ti;
    
#if 1 
    tcflush(fd, TCIOFLUSH);
    
    if (tcgetattr(fd, &ti) < 0)
    {
        ERR_PRINT("Can't get UART port settings\n");
        return -1;
    }
    
    cfmakeraw(&ti);
    
    ti.c_cflag |= CLOCAL;
    ti.c_cflag &= ~CRTSCTS;
    ti.c_iflag &= ~(IXON | IXOFF | IXANY | 0x80000000);
    
    if (iFlowControl == 1)
    {
        /* HW flow control */
        ti.c_cflag |= CRTSCTS;
    }
    else if (iFlowControl == 2)
    {
        /* MTK SW flow control */
        //ti.c_iflag |= (IXON | IXOFF | IXANY);
        ti.c_iflag |= 0x80000000;
    }
    
    if (tcsetattr(fd, TCSANOW, &ti) < 0)
    {
        ERR_PRINT("Can't set UART port settings\n");
        return -1;
    }

    ti.c_cc[VTIME] = 30;
    ti.c_cc[VMIN] = 0;

    /* Set initial baudrate */
    cfsetospeed(&ti, uart_speed(iBaudrate));
    cfsetispeed(&ti, uart_speed(iBaudrate));
    
    if (tcsetattr(fd, TCSANOW, &ti) < 0) 
    {
        ERR_PRINT("Can't set UART baud rate\n");
        return -1;
    }
     
    tcflush(fd, TCIOFLUSH);
#else    
	tcflush(fd, TCIOFLUSH);

	if (tcgetattr(fd, &ti) < 0)
	{
		perror("Can't get port settings");
		return -1;
	}

	cfmakeraw(&ti);

	ti.c_cflag |= CLOCAL;
	ti.c_cflag |= CRTSCTS;

	if (tcsetattr(fd, TCSANOW, &ti) < 0)
	{
		perror("Can't set port settings");
		return -1;
	}

    /* Set initial baudrate */
    cfsetospeed(&ti, uart_speed(iBaudrate));
    cfsetispeed(&ti, uart_speed(iBaudrate));
    
    if (tcsetattr(fd, TCSANOW, &ti) < 0) 
    {
        ERR_PRINT("Can't set UART baud rate\n");
        return -1;
    }

	tcflush(fd, TCIOFLUSH);
#endif

    if (iFlowControl == FLOW_CTL_HW)
        rda_setup_flow_ctl(fd);
    
    return 0;
}

/* RDA specific chip initialization */
int rda_fw_cfg(void)
{
    int ret = BT_VND_OP_RESULT_SUCCESS;
    
    if (bt_vnd_cbacks)
    {
        bt_vnd_cbacks->fwcfg_cb(ret);
    }
    
    return 0;
}

int rda_sco_cfg(void)
{
    int ret = BT_VND_OP_RESULT_SUCCESS;
    
    if (bt_vnd_cbacks)
    {
        bt_vnd_cbacks->scocfg_cb(ret);
    }   

    return 0;
}

int rda_sleep_cfg(void)
{
    int ret = BT_VND_OP_RESULT_SUCCESS;
    
    if (bt_vnd_cbacks)
    {
        bt_vnd_cbacks->lpm_cb(ret);
    } 

    return 0;
}

#define HCI_VSC_WRITE_WAKEUP   0xC0FC

static void wakeup_complete(void *p_evt)
{
    HC_BT_HDR *p_buf = (HC_BT_HDR *)p_evt;
    
    if (bt_vnd_cbacks)
    {
        bt_vnd_cbacks->dealloc(p_buf);
    }
}

int rda_wake_chip(uint8_t wake_assert)
{	   
   if((!chipstate)&&wake_assert)
   {   
        if (bt_vnd_cbacks)
        {
            ERR_PRINT("[###### TCC BT #######]rda_wake_chip:old(%d)new(%d).\n", chipstate, wake_assert);
            /* to wakeup chip,send special cmd to chip  */
            uint8_t    *p     = NULL;
            HC_BT_HDR  *p_buf = (HC_BT_HDR *) bt_vnd_cbacks->alloc(BT_HC_HDR_SIZE + 3);

            if (p_buf != NULL)
            {
                p_buf->event          = MSG_STACK_TO_HC_HCI_CMD;
                p_buf->offset         = 0;
                p_buf->len            = 1; //p_buf->len            = 3;
                p_buf->layer_specific = 0;

                p = (uint8_t *) (p_buf + 1);
                
                /*
                *p++ = 0xC0;
                *p++ = 0xFC;
                *p   = 0;
                */
                *p   = 0xFF;
                bt_vnd_cbacks->xmit_cb(HCI_VSC_WRITE_WAKEUP, p_buf, wakeup_complete);

                bt_vnd_cbacks->dealloc(p_buf);                  
            }
        }       
    }	   

    chipstate = wake_assert;

    return 0;
}


