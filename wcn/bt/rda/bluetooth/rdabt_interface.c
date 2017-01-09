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


void rdabt_write_memory(int fd,__u32 addr,__u32 *data,__u8 len,__u8 memory_type)
{
   __u16 num_to_send;
   __u16 i,j;
   __u8 data_to_send[256]={0};
   __u32 address_convert;

   data_to_send[0] = 0x01;
   data_to_send[1] = 0x02;
   data_to_send[2] = 0xfd;
   data_to_send[3] = (__u8)(len*4+6);
   //data_to_send[4] = (memory_type+0x80);  // add the event display(0x80); no event(0x00)
   data_to_send[4] = (memory_type+0x00);  // add the event display(0x80); no event(0x00)
   data_to_send[5] = len;

   if(memory_type == 0x01)
   {
      address_convert = addr*4+0x200;

      data_to_send[6] = (__u8)address_convert;
      data_to_send[7] = (__u8)(address_convert>>8);
      data_to_send[8] = (__u8)(address_convert>>16);
      data_to_send[9] = (__u8)(address_convert>>24);
   }
   else
   {
      data_to_send[6] = (__u8)addr;
      data_to_send[7] = (__u8)(addr>>8);
      data_to_send[8] = (__u8)(addr>>16);
      data_to_send[9] = (__u8)(addr>>24);
   }

   for(i=0;i<len;i++,data++)
   {
       j=10+i*4;
       data_to_send[j]   =  (__u8)(*data);
       data_to_send[j+1] = (__u8)((*data)>>8);
       data_to_send[j+2] = (__u8)((*data)>>16);
       data_to_send[j+3] = (__u8)((*data)>>24);
   }

   num_to_send = 4+data_to_send[3];

   write(fd,&(data_to_send[0]),num_to_send);
}



void RDA_uart_write_array(int fd, const __u32 buf[][2],__u16 len,__u8 type)
{
   __u32 i;

   for(i=0;i<len;i++)
   {
      rdabt_write_memory(fd,buf[i][0],&buf[i][1],1,type);

      usleep(12000);//12ms?
   }
}


/*此函数实现拉高LDO_DON,并延时50ms*/
int  RDA_pin_to_high(int fd)
{
    return ioctl(fd, RDA_BT_LDO_ON_HIGH_IOCTL);
}

/*此函数实现拉低LDO_DON,并延时50ms*/
int RDA_pin_to_low(int fd)
{
    return ioctl(fd, RDA_BT_LDO_ON_LOW_IOCTL);
}


void rdabt_write_pskey(int fd,unsigned char id, const unsigned int *data,unsigned char len)
{

   unsigned short num_to_send;
   unsigned char data_to_send[20];
   data_to_send[0] = 0x01;
   data_to_send[1] = 0x05;
   data_to_send[2] = 0xfd;
   data_to_send[3] = len+1;
   data_to_send[4] = id;

   num_to_send = 5;
   write(fd,data_to_send,num_to_send);
   usleep(1000);
   num_to_send = len;
   write(fd,data,num_to_send);
   usleep(3000);
}



