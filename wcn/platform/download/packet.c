#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <pthread.h>
#include <utils/Log.h>
#include "cmd_def.h"
#include "packet.h"
#include "fdl_crc.h"
#include <unistd.h>

#define BSL_UART_PACKET 0
#define BSL_SPI_PACKET	1

#define COUNTDOWN_START(name, begin, end, timeout) \
    do{\
	if(-1 == clock_gettime(CLOCK_MONOTONIC, &(begin))){\
            DOWNLOAD_LOGE("get "name" begin time error!\n");\
            return -1;\
	}\
    }while(0)

#define COUNTDOWN_END(name, begin, end, timeout) \
    ({\
        int ret=0;\
        if(-1 == clock_gettime(CLOCK_MONOTONIC, &(end))){\
            DOWNLOAD_LOGE("get "name" end time error!\n");\
            return -1;\
        }\
        if (delta_miliseconds(&(begin), &(end)) > (timeout)){\
            DOWNLOAD_LOGE("wait "name" timeout!!!\n");\
	    ret = -1;\
        }\
        ret = ret;\
    })

#define WAIT_ACK_START(begin, end) COUNTDOWN_START("ack", begin, end, 500)
#define WAIT_ACK_END(begin, end) if (COUNTDOWN_END("ack", begin, end, 500) == -1)break
#define WRITE_START(begin, end) COUNTDOWN_START("write", begin, end, 3)
#define WRITE_END(begin, end) if (COUNTDOWN_END("write", begin, end, 3) == -1)break

extern unsigned int delta_miliseconds(struct timespec *begin, struct timespec *end);

static unsigned long send_buffer[1000]={0};
static unsigned max_transfer_size = 0;

/******************************************************************************
**  Description:    This function scan the data in src buffer ,add 0x7e as begin
**                  and end of src,at the same time replace 0x7E,0x7D with rules
**                  below:
**                        0x7e is replaced by 0x7d and 0x5e;
**                        0x7D is replace 0x7d and 0x5d
**                  the translated data will be save in dest buffer
**  Author:         jiayong.yang
**  parameter:      dest is buffer result of process
**                  src is buffer where data is saved
**                  size is size of src data.
******************************************************************************/
static int translate_packet(char *dest,char *src,int size)
{
        int i;
        int translated_size = 0;

        dest[translated_size++] = 0x7E;

        for(i=0;i<size;i++){
                if(src[i] == 0x7E){
                        dest[translated_size++] = 0x7D;
                        dest[translated_size++] = 0x5E;
                } else if(src[i] == 0x7D) {
                        dest[translated_size++] = 0x7D;
                        dest[translated_size++] = 0x5D;
                } else
                        dest[translated_size++] = src[i];
        }
        dest[translated_size++] = 0x7E;
        return translated_size;
}
/******************************************************************************
**  Description:    This function scan the data in src buffer ,add 0x7e as begin
**                  and end of src,at the same time replace 0x7E,0x7D with rules
**                  below:
**                        0x7e is replaced by 0x7d and 0x5e;
**                        0x7D is replace 0x7d and 0x5d
**                  the translated data will be save in dest buffer
**  Author:         jiayong.yang
**  parameter:      dest is buffer result of process
**                  src is buffer where data is saved
**                  size is size of src data.
******************************************************************************/
static int untranslate_packet(char *dest,char *src,int size)
{
        int i;
        int translated_size = 0;
        int status = 0;
        for(i=0;i<size;i++){
                switch(status){
                        case 0:
                                if(src[i] == 0x7e)
                                        status = 1;
                        break;
                        case 1:
                                if(src[i] != 0x7e)
                                {
                                        status = 2;
                                        dest[translated_size++] = src[i];
                                }
                        break;
                        case 2:
                                if(src[i] == 0x7E){
                                        unsigned short crc;
                                        crc = crc_16_l_calc((char *)dest,translated_size-2);
                                        return translated_size;
                                }else if(dest[translated_size-1] == 0x7D){
                                        if(src[i] == 0x5E){
                                                dest[translated_size-1] = 0x7E;
                                        } else if(src[i] == 0x5D) {
                                                dest[translated_size-1] = 0x7D;
                                        }
                                }else {
                                        dest[translated_size++] = src[i];
                                }
                        break;
                }
        }
        return translated_size;
}

/******************************************************************************
**  Description:    This function setup the download protocol packet
**  Author:         jiayong.yang
**  parameter:      msg : msg type
**                  buffer: where packet is saved
**                  data_size: length of message body
**                  packet_type: UART message or SPI message.
******************************************************************************/
int setup_packet(CMD_TYPE msg,char *buffer,int offset,int data_size,int flag,int image_size)
{
	struct pkt_header_tag *head;
	int length = sizeof(struct pkt_header_tag)+data_size;
	int total_size = 0;
	unsigned short crc16;

	head = (struct pkt_header_tag *)&buffer[4];
	switch(msg)
	{
		case BSL_CMD_CONNECT:
                        if(flag==BSL_UART_PACKET){
                                head->type = cpu2be16((unsigned short)msg);
                                head->length = cpu2be16((unsigned short)data_size);
				*((unsigned short *)&buffer[length+4]) = 0;
				total_size = length + 2;
			}else{

				*((unsigned short *)&buffer[0]) =  data_size;
				*((unsigned short *)&buffer[2]) = (unsigned short)msg;
				*((unsigned short *)&buffer[8]) = 0;
                *((unsigned short *)&buffer[4]) = boot_checksum((const unsigned char *)buffer+offset,data_size);
                *((unsigned short *)&buffer[6]) = boot_checksum((const unsigned char *)buffer,6);
				total_size = 12;
			}
		break;
		case BSL_CMD_START_DATA:
                        if(flag==BSL_UART_PACKET){
                                head->type = cpu2be16((unsigned short)msg);
                                head->length = cpu2be16((unsigned short)data_size);
                                crc16 = crc_16_l_calc((char *)head,length);
                                *((unsigned short *)&buffer[length+4]) = cpu2be16(crc16);
				total_size = length + 2;
			}else{

				*((unsigned short *)&buffer[0]) =  data_size;
				*((unsigned short *)&buffer[2]) = (unsigned short)msg;
				*((unsigned short *)&buffer[4]) = boot_checksum((const unsigned char *)buffer+offset,data_size);
				*((unsigned short *)&buffer[6]) = boot_checksum((const unsigned char *)buffer,6);
				total_size = data_size + 8;
			}
		break;
		case BSL_CMD_DUMP_MEM:
                        if(flag==BSL_UART_PACKET){
                                head->type = cpu2be16((unsigned short)msg);
                                head->length = cpu2be16((unsigned short)data_size);
                                crc16 = crc_16_l_calc((char *)head,length);
                                *((unsigned short *)&buffer[length+4]) = cpu2be16(crc16);
				total_size = length + 2;
			}else{

				*((unsigned short *)&buffer[0]) =  data_size;
				*((unsigned short *)&buffer[2]) = (unsigned short)msg;
				*((unsigned short *)&buffer[4]) = boot_checksum((const unsigned char *)buffer+offset,data_size);
				*((unsigned short *)&buffer[6]) = boot_checksum((const unsigned char *)buffer,6);
				total_size = data_size + 8;
			}
		break;
		case BSL_CMD_MIDST_DATA:
                        if(flag==BSL_UART_PACKET){
                                head->type = cpu2be16((unsigned short)msg);
                                head->length = cpu2be16((unsigned short)data_size);
                                crc16 = crc_16_l_calc((char *)head,length);
                                *((unsigned short *)&buffer[length+4]) = cpu2be16(crc16);
				total_size = length + 2;
			}else{
				if(data_size == 0)
					*((unsigned short *)&buffer[0]) =  4;
				else
					*((unsigned short *)&buffer[0]) =image_size&0xffff;
				*((unsigned short *)&buffer[2]) = (unsigned short)msg|((image_size&0xff0000)>>8);
				*((unsigned short *)&buffer[4]) = 0;//boot_checksum((const unsigned char *)(buffer+offset),data_size);
				*((unsigned short *)&buffer[6]) = boot_checksum((const unsigned char *)buffer,6);
				//printf("data_size = %x check_sum(0x%02x) : 0x%04x \n",data_size,buffer[offset],*((unsigned short *)&buffer[4]));
				total_size = data_size + 8;
			}
		break;
		case BSL_CMD_END_DATA:
                        if(flag==BSL_UART_PACKET){
                                head->type = cpu2be16((unsigned short)msg);
                                head->length = cpu2be16((unsigned short)data_size);
                                crc16 = crc_16_l_calc((char *)head,length);
                                *((unsigned short *)&buffer[length+4]) = cpu2be16(crc16);
				total_size = length + 2;
			}else{

				*((unsigned short *)&buffer[0]) =  4;
				*((unsigned short *)&buffer[2]) = (unsigned short)msg;
				*((unsigned short *)&buffer[4]) = boot_checksum((const unsigned char *)(buffer+offset),data_size);
				*((unsigned short *)&buffer[6]) = boot_checksum((const unsigned char *)buffer,6);
				*((unsigned int *)&buffer[8]) = 0;
				total_size = 12;
			}
		break;
		case BSL_CMD_EXEC_DATA:
                        if(flag==BSL_UART_PACKET){
                                head->type = cpu2be16((unsigned short)msg);
                                head->length = cpu2be16((unsigned short)data_size);
                                crc16 = crc_16_l_calc((char *)head,length);
                                *((unsigned short *)&buffer[length+4]) = cpu2be16(crc16);
				      total_size = length + 2;
			}else{

				*((unsigned short *)&buffer[0]) =  data_size;
				*((unsigned short *)&buffer[2]) = (unsigned short)msg;
				*((unsigned short *)&buffer[4]) = boot_checksum((const unsigned char *)(buffer+offset),data_size);
				*((unsigned short *)&buffer[6]) = boot_checksum((const unsigned char *)buffer,6);
				total_size = data_size + 8;
			}
		break;
		//==================
		case BSL_CMD_NORMAL_RESET:
                        if(flag==BSL_UART_PACKET){
                                head->type = cpu2be16((unsigned short)msg); // 2 bytes of CMD
                                head->length = cpu2be16((unsigned short)data_size); // 2 bytes of message length(the length of message body!!!)
                                crc16 = crc_16_l_calc((char *)head,length); //see above: length = sizeof(struct pkt_header_tag)+data_size;
                                *((unsigned short *)&buffer[length+4]) = cpu2be16(crc16);
				      total_size = length + 2; // add 2 bytes of CRC to total_size, the total_size is 10
			}else{

				*((unsigned short *)&buffer[0]) =  data_size;
				*((unsigned short *)&buffer[2]) = (unsigned short)msg;
				*((unsigned short *)&buffer[4]) = boot_checksum((const unsigned char *)(buffer+offset),data_size);
				*((unsigned short *)&buffer[6]) = boot_checksum((const unsigned char *)buffer,6);
				total_size = data_size + 8;
			}
		break;
		//==================
                case BSL_CMD_SWITCH_MODE:
                        if(flag==BSL_UART_PACKET){
                                head->type =  cpu2be16((unsigned short)msg);
                                head->length = cpu2be16((unsigned short)data_size);
                                crc16 = frm_chk((const unsigned short *)head,length);
                                *((unsigned short *)&buffer[length+4]) = (crc16);
                                total_size = length + 2;
                        }else{

                                *((unsigned short *)&buffer[0]) =  4;
                                *((unsigned short *)&buffer[2]) = (unsigned short)msg;
                                *((unsigned short *)&buffer[4]) = boot_checksum((const unsigned char *)(buffer+offset),data_size);
                                *((unsigned short *)&buffer[6]) = boot_checksum((const unsigned char *)buffer,6);
                                *((unsigned int *)&buffer[8]) = 0;
                                total_size = 12;
                        }
                break;
#ifdef GET_MARLIN_CHIPID
                 case BSL_CMD_READ_CHIPID:
                        if(flag==BSL_UART_PACKET){
                                head->type = cpu2be16((unsigned short)msg);
                                head->length = cpu2be16((unsigned short)data_size);
                                crc16 = crc_16_l_calc((char *)head,length);
                                *((unsigned short *)&buffer[length+4]) = cpu2be16(crc16);
                                total_size = length + 2;
                        }else{

                                *((unsigned short *)&buffer[0]) =  4;
                                *((unsigned short *)&buffer[2]) = (unsigned short)msg;
                                *((unsigned short *)&buffer[4]) = boot_checksum((const unsigned char *)(buffer+offset),data_size);
                                *((unsigned short *)&buffer[6]) = boot_checksum((const unsigned char *)buffer,6);
                                *((unsigned int *)&buffer[8]) = 0;
                                total_size = 12;
                        }
                break;
#endif
		default:
		break;
	}
	return total_size;
}
/******************************************************************************
**  Description:    This function setup connect message by uart
**  Author:         jiayong.yang
**  parameter:      none
******************************************************************************/
int  send_connect_message(int fd,int flag)
{
	char raw_buffer[32] = {0};
	int size;
	int translated_size;
	int offset;
	int retval,i;
	char *data;
	struct pkt_header_tag head;
	struct timespec begin, end;

	size = setup_packet(BSL_CMD_CONNECT,raw_buffer,8,0,flag,0);
	if(flag ==0)//uart
	{
		translated_size = translate_packet((char *)send_buffer,(char *)&raw_buffer[4],size);
		WRITE_START(begin, end);
		do{
			retval = write(fd,send_buffer,translated_size);
			if(retval > 0)
				translated_size -= retval;
			else
			{
				DOWNLOAD_LOGD("warning: write errno=%d,try again\n", errno);
				usleep(300);
			}
			WRITE_END(begin, end);
		}while(translated_size> 0);
		if (translated_size <= 0)
			DOWNLOAD_LOGD("send connect message ok\n");
	}
	else//sdio
	{
		retval = write(fd,raw_buffer,size);
		DOWNLOAD_LOGD("send_connect_message write retval = %d\n", retval);
	}
	if(retval > 0)
	{
		WAIT_ACK_START(begin,end);
		if(flag == 0)
			size = 8;
		else
			size = 6;
		offset = 0;
		do{
			retval = read(fd,&raw_buffer[offset],size);
			if(retval > 0)
			{
				offset += retval;
				size -= retval;
			}
			WAIT_ACK_END(begin,end);
		}while(size!=0);
	}
	if(retval > 0){
		if(flag == 0) {
			untranslate_packet((char *)send_buffer,(char *)raw_buffer,offset);
			data = (char *)send_buffer;
		} else {
			data = (char *)raw_buffer;
		}
		head.type = (data[0]<<8)|data[1];
		if(head.type == BSL_REP_ACK)
		{
			DOWNLOAD_LOGD(">>>>>>>ACK CMD_CONNECT\n");
			return 0;
		}
	}
	DOWNLOAD_LOGD("CONNECT_NACK:%x %x %x %x %x %x %x %x\n",raw_buffer[0],raw_buffer[1],raw_buffer[2],raw_buffer[3],raw_buffer[4],raw_buffer[5],raw_buffer[6],raw_buffer[7]);
	return -1;
}
/******************************************************************************
**  Description:    This function setup data start message by uart
**  Author:         jiayong.yang
**  parameter:      size : size of image to be sent
**                  addr : address where image to be saved in MODEM
******************************************************************************/
int  send_start_message(int fd,int size,unsigned long addr,int flag)
{
	char raw_buffer[32] = {0};
	char *data = raw_buffer;
	int translated_size;
	int offset;
	int retval;
	struct pkt_header_tag head;
	struct timespec begin, end;

        *(unsigned long *)&raw_buffer[8] = cpu2be32(addr);
        *(unsigned long *)&raw_buffer[12] = cpu2be32(size);
	size = setup_packet(BSL_CMD_START_DATA,raw_buffer,8,8,flag,0);
	if(flag ==0)
	{
		translated_size = translate_packet((char *)send_buffer,(char *)&raw_buffer[4],size);
		WRITE_START(begin, end);
		do{
			retval = write(fd,send_buffer,translated_size);
			if(retval > 0)
				translated_size -= retval;
			else
			{
				DOWNLOAD_LOGD("warning: write errno=%d,try again\n", errno);
				usleep(300);
			}
			WRITE_END(begin,end);
		}while(translated_size> 0);
		if (translated_size <= 0)
			DOWNLOAD_LOGD("send start message ok\n");
	}
	else
	{
		retval = write(fd,raw_buffer,size);
		DOWNLOAD_LOGD("send_start_message write retval = %d\n", retval);
	}
	if(retval > 0)
	{
		WAIT_ACK_START(begin,end);
		if(flag == 0)
			size = 8;
		else
			size = 6;
		offset = 0;
		do{
			retval = read(fd,&raw_buffer[offset],size);
			if(retval > 0)
			{
				offset += retval;
				size -= retval;
			}
			WAIT_ACK_END(begin,end);
		}while(size!=0);
	}
	if(retval > 0){
		if(flag == 0) {
			untranslate_packet((char *)send_buffer,(char *)raw_buffer,offset);
			data = (char *)send_buffer;
		} else {
			data = (char *)raw_buffer;
		}
		head.type = (data[0]<<8)|data[1];
		if((head.type == BSL_REP_ACK)||(head.type == 0))
		{
			DOWNLOAD_LOGD(">>>>>>>ACK DATA_START\n");
			return 0;
		}
	}
	DOWNLOAD_LOGD("START_NACK:%x %x %x %x %x %x %x %x\n",raw_buffer[0],raw_buffer[1],raw_buffer[2],raw_buffer[3],raw_buffer[4],raw_buffer[5],raw_buffer[6],raw_buffer[7]);
	return -1;
}
/******************************************************************************
**  Description:    This function setup data end message by uart
**  Author:         jiayong.yang
**  parameter:      none
******************************************************************************/
int  send_end_message(int fd,int flag)
{
	char raw_buffer[32] = {0};
	char *data = raw_buffer;
	int size;
	int translated_size;
	int offset;
	int retval;
	struct pkt_header_tag head;
	struct timespec begin, end;

	size = setup_packet(BSL_CMD_END_DATA,raw_buffer,8,0,flag,0);
	if(flag ==0)
	{	translated_size = translate_packet((char *)send_buffer,(char *)&raw_buffer[4],size);
		WRITE_START(begin, end);
		do{
			retval = write(fd,send_buffer,translated_size);
			if(retval > 0)
				translated_size -= retval;
			else
			{
				DOWNLOAD_LOGD("warning: write errno=%d,try again\n", errno);
				usleep(300);
			}
			WRITE_END(begin,end);
		}while(translated_size> 0);
		if (translated_size <= 0)
			DOWNLOAD_LOGD("send end message ok\n");
	}
	else
	{
		retval = write(fd,raw_buffer,size);
		DOWNLOAD_LOGD("send_end_message write retval = %d\n", retval);
	}
	if(retval >0)
	{
		WAIT_ACK_START(begin,end);
		if(flag == 0)
			size = 8;
		else
			size = 6;
		offset = 0;
		do{
			retval = read(fd,&raw_buffer[offset],size);
			if(retval > 0)
			{
				offset += retval;
				size -= retval;
			}
			WAIT_ACK_END(begin,end);
		}while(size!=0);
	}
	if(retval > 0){
		if(flag == 0) {
			untranslate_packet((char *)send_buffer,(char *)raw_buffer,offset);
			data = (char *)send_buffer;
		} else {
			data = (char *)raw_buffer;
		}
		head.type = (data[0]<<8)|data[1];
		if((head.type == BSL_REP_ACK)||(head.type == 0))
		{
			DOWNLOAD_LOGD(">>>>>>>ACK DATA_END\n");
			return 0;
		}
	}
	DOWNLOAD_LOGD("END_NACK:%x %x %x %x %x %x %x %x\n",raw_buffer[0],raw_buffer[1],raw_buffer[2],raw_buffer[3],raw_buffer[4],raw_buffer[5],raw_buffer[6],raw_buffer[7]);
	return -1;
}

#ifdef GET_MARLIN_CHIPID
#define BSL_REP_IS_2331_AA 0xAA		    /* 0xAA */
#define BSL_REP_IS_2331_BA	0xBA		    /* 0xBA */
/******************************************************************************
**  Description:    This function send cmd to cp2 and get chipid
**  Author:         tingle.xu
**  parameter:
**	return value:
**	chipid
******************************************************************************/
unsigned int send_getchipid_message(int fd,int flag)
{
	char raw_buffer[32] = {0};
	char *data = raw_buffer;
	int size;
	int translated_size;
	int offset;
	int retval;
	struct pkt_header_tag head;
	struct timespec begin, end;
	unsigned int chip_id=0;

	DOWNLOAD_LOGD("send_getchipid_message enter,flag=%d \n",flag);

	size = setup_packet(BSL_CMD_READ_CHIPID,raw_buffer,8,0,flag,0);
	if(flag ==0)
	{	translated_size = translate_packet((char *)send_buffer,(char *)&raw_buffer[4],size);
		WRITE_START(begin, end);
		do{
			retval = write(fd,send_buffer,translated_size);
			if(retval > 0)
				translated_size -= retval;
			else
			{
				DOWNLOAD_LOGD("[tingle]: warning: write errno=%d,try again\n", errno);
				usleep(300);
			}
			WRITE_END(begin,end);
		}while(translated_size> 0);
		if (translated_size <= 0)
			DOWNLOAD_LOGD("send message ok\n");
	}
	else
	{
		retval = write(fd,raw_buffer,size);
		DOWNLOAD_LOGD("[Tingle]: send_getchipid_message write retval = %d\n", retval);
	}
	if(retval >0)
	{
		WAIT_ACK_START(begin,end);
		if(flag == 0)
			size = 8;
		else
			size = 6;
		offset = 0;
		do{
			retval = read(fd,&raw_buffer[offset],size);
			if(retval > 0)
			{
				DOWNLOAD_LOGE("[Tingle]:  read retval=%d \n",retval);
				offset += retval;
				size -= retval;
			}
			WAIT_ACK_END(begin,end);
		}while(size!=0);
	}
	if(retval > 0){
		if(flag == 0) {
			untranslate_packet((char *)send_buffer,(char *)raw_buffer,offset);
			data = (char *)send_buffer;
		} else {
			data = (char *)raw_buffer;
		}
		DOWNLOAD_LOGE("[Tingle]: %x %x %x %x %x %x %x %x\n",raw_buffer[0],raw_buffer[1],raw_buffer[2],raw_buffer[3],raw_buffer[4],raw_buffer[5],raw_buffer[6],raw_buffer[7]);

		chip_id= ((data[0]<<24)|(data[1]<<16)|(data[2]<<8)|(data[3]));
		DOWNLOAD_LOGE("[Tingle]: chip_id=0x%04x \n",chip_id);
		return chip_id;
	}
	DOWNLOAD_LOGD("[Tingle]: END_NACK:%x %x %x %x %x %x %x %x\n",raw_buffer[0],raw_buffer[1],raw_buffer[2],raw_buffer[3],raw_buffer[4],raw_buffer[5],raw_buffer[6],raw_buffer[7]);
	return 0;
}
#endif

/******************************************************************************
**  Description:    This function setup data message by uart
**  Author:         jiayong.yang
**  parameter:      none
******************************************************************************/
int  send_data_message(int fd,char *buffer,int data_size,int flag,int image_size,int image_fd)
{
	char raw_buffer[32] = {0};
        char    uart_buffer[1500]={0};
	char *data = raw_buffer;
	int size;
	int translated_size;
	int offset;
	int retval;
	struct pkt_header_tag head;
	struct timespec begin, end;
	//DOWNLOAD_LOGE("send_data_message\n");

	if(flag ==0)
	{
		memcpy(&uart_buffer[8],buffer,data_size);
		size = setup_packet(BSL_CMD_MIDST_DATA,uart_buffer,8,data_size,flag,0);
		translated_size = translate_packet((char *)send_buffer,(char *)&uart_buffer[4],size);
		WRITE_START(begin,end);
		do{
			DOWNLOAD_LOGE("send_data_message0 translated_size=%d\n",translated_size);
			retval = write(fd,send_buffer,translated_size);
			if(retval > 0)
				translated_size -= retval;
			else
			{
				DOWNLOAD_LOGD("warning: write errno=%d,try again\n", errno);
				usleep(300);
			}
			WRITE_END(begin,end);
		}while(translated_size> 0);
		if (translated_size <= 0)
			DOWNLOAD_LOGD("send data message ok\n");
	}
	else
	{
		int count,i;
		size = setup_packet(BSL_CMD_MIDST_DATA,(char *)buffer,8,data_size,flag,image_size);
		DOWNLOAD_LOGE("send_data_message size=%d\n",size);
		retval = write(fd,buffer,size);
		count = image_size/data_size;
		for(i=1;i<count;i++){
			retval = read(image_fd,buffer,data_size);
			retval = write(fd,buffer,data_size);
		}
	}
	if(retval >0)
	{
		WAIT_ACK_START(begin,end);
		if(flag == 0)
			size = 8;
		else
			size = 6;
		offset = 0;
		do{
			retval = read(fd,&raw_buffer[offset],size);
			if(retval > 0)
			{
				offset += retval;
				size -= retval;
			}
			WAIT_ACK_END(begin,end);
		}while(size!=0);
	}
	if(retval > 0){
		if(flag == 0) {
			untranslate_packet((char *)send_buffer,(char *)raw_buffer,offset);
			data = (char *)send_buffer;
		} else {
			data = (char *)raw_buffer;
		}
			head.type = (data[0]<<8)|data[1];
        if((head.type == BSL_REP_ACK)||(head.type == 0))
		{
			//DOWNLOAD_LOGD(">>>>>>>ACK DATA %d\n", head.type);
			return 0;
		}
	}
	DOWNLOAD_LOGD("DATA_NACK:%x %x %x %x %x %x %x %x\n",raw_buffer[0],raw_buffer[1],raw_buffer[2],raw_buffer[3],raw_buffer[4],raw_buffer[5],raw_buffer[6],raw_buffer[7]);
	return -1;
}
/******************************************************************************
**  Description:    This function setup execute message by uart
**  Author:         jiayong.yang
**  parameter:      addr: address MODEM start to run.
******************************************************************************/
int  send_exec_message(int fd,unsigned long addr,int flag)
{
	char raw_buffer[32] = {0};
	char *data = raw_buffer;
	int size;
	int translated_size;
	int offset;
	int retval;
	struct pkt_header_tag head;
	struct timespec begin, end;

	*(unsigned long *)&raw_buffer[8] = cpu2be32(addr);
	size = setup_packet(BSL_CMD_EXEC_DATA,raw_buffer,8,4,flag,0);
	if(flag == 0)
	{
		translated_size = translate_packet((char *)send_buffer,(char *)&raw_buffer[4],size);
		WRITE_START(begin, end);
		do{
			retval = write(fd,send_buffer,translated_size);
			if(retval > 0)
				translated_size -= retval;
			else
			{
				DOWNLOAD_LOGD("warning: write errno=%d,try again\n", errno);
				usleep(300);
			}
			WRITE_END(begin,end);
		}while(translated_size> 0);
		if (translated_size <= 0)
			DOWNLOAD_LOGD("send exec message ok\n");
	}
	else
	{
		retval = write(fd,raw_buffer,size);
		DOWNLOAD_LOGD("send_exec_message write retval = %d\n", retval);
	}
	if(retval >0)
	{
		WAIT_ACK_START(begin,end);
		if(flag == 0)
			size = 8;
		else
			size = 6;
		offset = 0;
		do{
			retval = read(fd,&raw_buffer[offset],size);
			if(retval >= 0)	{
				offset += retval;
				size -= retval;
			}
			WAIT_ACK_END(begin,end);
		}while(size!=0);
	}
	if(retval > 0){
		if(flag == 0) {
			untranslate_packet((char *)send_buffer,(char *)raw_buffer,offset);
			data = (char *)send_buffer;
		} else {
			data = (char *)raw_buffer;
		}
		head.type = (data[0]<<8)|data[1];
		if((head.type == BSL_REP_ACK)||(head.type == 0))
		{
			DOWNLOAD_LOGD(">>>>>>>ACK EXEC\n");
			return 0;
		}
	}
	DOWNLOAD_LOGD("EXEC_NACK:%x %x %x %x %x %x %x %x\n",raw_buffer[0],raw_buffer[1],raw_buffer[2],raw_buffer[3],raw_buffer[4],raw_buffer[5],raw_buffer[6],raw_buffer[7]);
	return -1;
}

/******************************************************************************
**  Description:    This function setup dump start message
**  Author:         xiaodong.bi
**  parameter:      size : size of image to be sent
**                  addr : address where image to be saved in MODEM
******************************************************************************/
int  send_dump_mem_message(int fd,int size,unsigned long addr,int flag)
{
	char raw_buffer[32] = {0};
	int retval;

	*(unsigned long *)&raw_buffer[8] = cpu2be32(addr);
	*(unsigned long *)&raw_buffer[12] = cpu2be32(size);
	size = setup_packet(BSL_CMD_DUMP_MEM,raw_buffer,8,8,flag,0);

	retval = write(fd,raw_buffer,size);
	DOWNLOAD_LOGD("send_dump_mem_message write retval = %d\n", retval);
	return -1;
}

int  send_uart_speed_message(int fd,unsigned long speed,int flag)
{
	char raw_buffer[32] = {0};
	char *data = raw_buffer;
	int size;
	int translated_size;
	int offset;
	int retval;
	struct pkt_header_tag head;

	*(unsigned long *)&raw_buffer[8] = cpu2be32(speed);
	size = setup_packet(BSL_CMD_NORMAL_RESET,raw_buffer,8,4,flag,0);

	if(flag == 0)
	{
		translated_size = translate_packet((char *)send_buffer,(char *)&raw_buffer[4],size);
		do{
			retval = write(fd,send_buffer,translated_size);
			if(retval > 0)
				translated_size -= retval;
		}while(translated_size> 0);
		DOWNLOAD_LOGD("write  success retval=%d\n", retval);

	}
	else
	{
		retval = write(fd,raw_buffer,size);
	}

	if(retval >0)
	{
		//usleep(150*1000);//waiting for the uart bus transfer finished!!!!
		sleep(1);
		set_raw_data_speed(fd, speed);
		DOWNLOAD_LOGD("set AP uart speed = %d,Do Not check ACK\n", speed);
		return 0;
	}

	if(retval >0)
	{
		if(flag == 0)
			size = 8;
		else
			size = 6;
		offset = 0;
		do{
			retval = read(fd,&raw_buffer[offset],size);
			if(retval >= 0)	{
				offset += retval;
				size -= retval;
			}
		}while(size!=0);
	}

	if(retval > 0){
		if(flag == 0) {
			untranslate_packet((char *)send_buffer,(char *)raw_buffer,offset);
			data = (char *)send_buffer;
		} else {
			data = (char *)raw_buffer;
		}
		head.type = (data[0]<<8)|data[1];
		if((head.type == BSL_REP_ACK)||(head.type == 0))
		{
			DOWNLOAD_LOGD(">>>>>>>ACK SET SPEED\n");
			return 0;
		}
	}
	DOWNLOAD_LOGD("SET_SPEED_NACK:%x %x %x %x %x %x %x %x\n",raw_buffer[0],raw_buffer[1],raw_buffer[2],raw_buffer[3],raw_buffer[4],raw_buffer[5],raw_buffer[6],raw_buffer[7]);
	return -1;
}
