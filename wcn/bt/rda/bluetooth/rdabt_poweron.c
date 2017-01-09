#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/param.h>

#include "bt_cfg.h"
#include "bt_rda.h"

unsigned int rdabt_chip_version = 0;

int rda_setup_flow_ctl(int fd)
{

	rda5876_setup_flow_ctl(fd);
	
   
    return 0;

}


int rda_change_baudrate(int fd)
{
	 rda5876_change_baudrate(fd);

    return 0;

}

void RDABT_Write_Bt_Address(int s_fd,int d_fd)
{
    __u8 bt_addr[10] = {0x01,0x1a,0xfc,0x06,0xae,0x2d,0x22,0x11,0x58,0x76};
    __u8 bt_addr_lt[6] = {0x00};
    int i;

    //revert bt address.
    for (i=0; i<6; i++) {
        bt_addr[4+i] = bt_addr_lt[5-i];
    }

    ERR_PRINT("[###### BT-ADDRESS #######][0x%x],[0x%x],[0x%x],[0x%x],[0x%x],[0x%x].\n",bt_addr[9],bt_addr[8],bt_addr[7],bt_addr[6],bt_addr[5],bt_addr[4]);
    write(s_fd,&(bt_addr[0]),sizeof(bt_addr));
    usleep(10000);
}


int RDABT_core_Intialization(int fd)
{
    int bt_fd = open(RDA_BT_DEVICE_PATH, O_RDWR);

    if( bt_fd < 0 )
    {
        ERR_PRINT("[###### TCC BT #######] open error.\n");

        return -1;
    }

    rdabt_chip_version = RDABT_5876;
    RDA_pin_to_high(bt_fd);

    RDA5876_RfInit(fd);

    RDA_pin_to_low(bt_fd);

    RDA_pin_to_high(bt_fd);

    usleep(50000);

    RDA5876_RfInit(fd);

    RDA5876_Pskey_RfInit(fd);

    RDA5876_Dccal(fd);

    RDA5876_Trap(fd);

    RDA5876_Pskey_Misc(fd);

    rda_change_baudrate(fd);
    if(bt_fd>0)
    {
        close(bt_fd);
    }

    return 0;
}



int rda_init(int fd, int iBaudrate)
{
    int ret = RDABT_core_Intialization(fd);

    if(ret < 0)
    {
        ERR_PRINT("[###### TCC BT #######]rda_init:fails: %s(%d)\n",strerror(errno), errno);
    }

    return ret;
}


int rda_bt_baudrate_map(int iBaudrate)
{
    switch(iBaudrate)
    {
        case SERIAL_BOURATE_1500000:
            return 1625000;
        case SERIAL_BOURATE_3000000:
           return 3250000;
    }

    return iBaudrate;
}

