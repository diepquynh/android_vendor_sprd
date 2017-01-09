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

#define BSL_UART_PACKET 0
#define BSL_SPI_PACKET	1
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
	size = setup_packet(BSL_CMD_CONNECT,raw_buffer,8,0,flag,0);
	if(flag ==0)
	{
		translated_size = translate_packet((char *)send_buffer,(char *)&raw_buffer[4],size);
		do{
			retval = write(fd,send_buffer,translated_size);
			if(retval > 0)
				translated_size -= retval;
		}while(translated_size> 0);
	}
	else
	{
		retval = write(fd,raw_buffer,size);
	}
	if(retval > 0)
	{
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
		}while(size!=0);
	}
	else
	{
		MODEM_LOGD("write retval = %d:\n",retval);
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
			MODEM_LOGD(">>>>>>>ACK CMD_CONNECT\n");
			return 0;
		}
	}
MODEM_LOGD("CONNECT_NACK:%x %x %x %x %x %x %x %x\n",raw_buffer[0],raw_buffer[1],raw_buffer[2],raw_buffer[3],raw_buffer[4],raw_buffer[5],raw_buffer[6],raw_buffer[7]);
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

        *(unsigned long *)&raw_buffer[8] = cpu2be32(addr);
        *(unsigned long *)&raw_buffer[12] = cpu2be32(size);
	size = setup_packet(BSL_CMD_START_DATA,raw_buffer,8,8,flag,0);
	if(flag ==0)
	{
		translated_size = translate_packet((char *)send_buffer,(char *)&raw_buffer[4],size);
		do{
			retval = write(fd,send_buffer,translated_size);
			if(retval > 0)
				translated_size -= retval;
		}while(translated_size> 0);
	}
	else
	{
		retval = write(fd,raw_buffer,size);
		MODEM_LOGD("send_start_message write retval = %d\n", retval);
	}
	if(retval > 0)
	{
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
			MODEM_LOGD(">>>>>>>ACK DATA_START\n");
                        return 0;
		}
	}
MODEM_LOGD("START_NACK:%x %x %x %x %x %x %x %x\n",raw_buffer[0],raw_buffer[1],raw_buffer[2],raw_buffer[3],raw_buffer[4],raw_buffer[5],raw_buffer[6],raw_buffer[7]);
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

	size = setup_packet(BSL_CMD_END_DATA,raw_buffer,8,0,flag,0);
	if(flag ==0)
	{	translated_size = translate_packet((char *)send_buffer,(char *)&raw_buffer[4],size);
		do{
			retval = write(fd,send_buffer,translated_size);
			if(retval > 0)
				translated_size -= retval;
			else if(retval < 0){
				MODEM_LOGE("write error : error = %d \n",errno);
			}
		}while(translated_size> 0);
	}
	else
	{
		retval = write(fd,raw_buffer,size);
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
			if(retval > 0)
			{
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
			//printf(">>>>>>>ACK DATA_END\n");
                        return 0;
		}
	}
MODEM_LOGD("END_NACK:%x %x %x %x %x %x %x %x\n",raw_buffer[0],raw_buffer[1],raw_buffer[2],raw_buffer[3],raw_buffer[4],raw_buffer[5],raw_buffer[6],raw_buffer[7]);
	return -1;
}
/******************************************************************************
**  Description:    This function setup data message by uart
**  Author:         jiayong.yang
**  parameter:      none
******************************************************************************/
int  send_data_message(int fd,char *buffer,int data_size,int flag,int image_size,int image_fd)
{
	char raw_buffer[32] = {0};
        char    uart_buffer[300]={0};
	char *data = raw_buffer;
	int size;
	int translated_size;
	int offset;
	int retval;
	struct pkt_header_tag head;

	if(flag ==0)
	{
		memcpy(&uart_buffer[8],buffer,data_size);
		size = setup_packet(BSL_CMD_MIDST_DATA,uart_buffer,8,data_size,flag,0);
		translated_size = translate_packet((char *)send_buffer,(char *)&uart_buffer[4],size);
		do{
			retval = write(fd,send_buffer,translated_size);
			if(retval > 0)
				translated_size -= retval;
		}while(translated_size> 0);
	}
	else
	{
		int count,i;
		size = setup_packet(BSL_CMD_MIDST_DATA,(char *)buffer,8,data_size,flag,image_size);
		retval = write(fd,buffer,size);
		count = image_size/data_size;
		for(i=1;i<count;i++){
			retval = read(image_fd,buffer,data_size);
			retval = write(fd,buffer,data_size);
		}
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
			if(retval > 0)
			{
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
		//	MODEM_LOGD(">>>>>>>ACK DATA %d\n", head.type);
                        return 0;
		}
	}
MODEM_LOGD("DATA_NACK:%x %x %x %x %x %x %x %x\n",raw_buffer[0],raw_buffer[1],raw_buffer[2],raw_buffer[3],raw_buffer[4],raw_buffer[5],raw_buffer[6],raw_buffer[7]);
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

	*(unsigned long *)&raw_buffer[8] = cpu2be32(addr);
	size = setup_packet(BSL_CMD_EXEC_DATA,raw_buffer,8,4,flag,0);
	if(flag == 0)
	{
		translated_size = translate_packet((char *)send_buffer,(char *)&raw_buffer[4],size);
		do{
			retval = write(fd,send_buffer,translated_size);
			if(retval > 0)
				translated_size -= retval;
		}while(translated_size> 0);
	}
	else
	{
		retval = write(fd,raw_buffer,size);
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
			MODEM_LOGD(">>>>>>>ACK EXEC\n");
                        return 0;
		}
	}
	MODEM_LOGD("EXEC_NACK:%x %x %x %x %x %x %x %x\n",raw_buffer[0],raw_buffer[1],raw_buffer[2],raw_buffer[3],raw_buffer[4],raw_buffer[5],raw_buffer[6],raw_buffer[7]);
	return -1;
}

/******************************************************************************
**  Description:    This function change the transfer  to SPI bus
**  Author:         jiayong.yang
**  parameter:      none
******************************************************************************/
int  uart_send_change_spi_mode_message(int fd,int max_tran_sz)
{
        char raw_buffer[32] = {0};
        char *data = raw_buffer;
        int size;
        int translated_size;
        int offset;
        int retval;
        struct pkt_header_tag head;

	max_transfer_size = max_tran_sz;
	*(unsigned long *)&raw_buffer[8] = cpu2be32(max_tran_sz);
        size = setup_packet(BSL_CMD_SWITCH_MODE,raw_buffer,8,0,BSL_UART_PACKET,0);
        translated_size = translate_packet((char *)send_buffer,(char *)&raw_buffer[4],size);
	do{
		retval = write(fd,(char *)send_buffer,translated_size);
		if(retval > 0)
			translated_size -= retval;
		else if(retval < 0){
			MODEM_LOGE("write error : error = %d \n",errno);
		}
	}while(translated_size> 0);

        if(retval >0)
        {
                size = 8;
                offset = 0;
                do{
                        retval = read(fd,(char *)&raw_buffer[offset],size);
			if(retval >= 0)	{
				offset += retval;
				size -= retval;
			} else if(retval < 0){
				MODEM_LOGE("read error : error = %d \n",errno);
				sleep(1);
			}
                }while(size!=0);
        }
        if(retval > 0){
                data = (char *)send_buffer;
                untranslate_packet(data,(char *)raw_buffer,offset);
                head.type = (data[0]<<8)|data[1];

                if(head.type == BSL_REP_ACK)
		{
			MODEM_LOGD(">>>>>>>ACK CMD_CHANGE_MODE\n");
			return 0;
		}
        }
MODEM_LOGD("EXEC_NACK:%x %x %x %x %x %x %x %x\n",raw_buffer[0],raw_buffer[1],raw_buffer[2],raw_buffer[3],raw_buffer[4],raw_buffer[5],raw_buffer[6],raw_buffer[7]);
        return -1;
}

