#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include "audiotester_server.h"
#include <sys/stat.h>

#define SHELL_DBG(...)   fprintf(stdout, ##__VA_ARGS__)

void _dump_data(char *buf, int len)
{
    int i = len;
    int line = 0;
    int size = 0;
    int j = 0;
    char dump_buf[60] = {0};
    int dump_buf_len = 0;

    char *tmp = (char *) buf;
    line = i / 16 + 1;
    for(i = 0; i < line; i++) {
        dump_buf_len = 0;
        memset(dump_buf, 0, sizeof(dump_buf));

        if(i < line - 1) {
            size = 16;
        } else {
            size = len % 16;
        }
        tmp = (char *)buf + i * 16;

        for(j = 0; j < size; j++) {
            sprintf(dump_buf + dump_buf_len, "%02x", tmp[j]);
            dump_buf_len += 2;
        }

        SHELL_DBG("%s\n", dump_buf);

    }
}

unsigned long get_file_size(const char *path)  
{  
    unsigned long filesize = -1;      
    struct stat statbuff;  
    if(stat(path, &statbuff) < 0){  
        return filesize;  
    }else{  
        filesize = statbuff.st_size;  
    }  
    return filesize;  
}

int main(int argc, char *argv[])
{
    MSG_HEAD_T msg_head;
    DATA_HEAD_T data_command;
    uint8_t *audio_data_buf;
    int len=0;
    int sub_type=2;
    char * cmd_buf=NULL;
    int i=0;
    int num_read=0;
    int file_size=0;
    uint8_t * file_buf=NULL;
    FILE *file = NULL;

    if((argc != 3) && (argc != 2)){
        SHELL_DBG("argc err");
        return -1;

    }

    if(argc==2){
        audio_data_buf=NULL;
        len=sizeof(MSG_HEAD_T)+sizeof(DATA_HEAD_T)+2;
        cmd_buf=malloc(len);
    }else{
        audio_data_buf=argv[2];
        if(access(audio_data_buf, R_OK) == 0){
            file_size=get_file_size(audio_data_buf);
            len=file_size+sizeof(MSG_HEAD_T)+sizeof(DATA_HEAD_T)+2;
            file = fopen(audio_data_buf, "rb");
            if(file == NULL) {
                SHELL_DBG("open %s fail\n", audio_data_buf);
                return -1;
            }
            cmd_buf=malloc(len);
            file_buf=cmd_buf+1+sizeof(MSG_HEAD_T)+sizeof(DATA_HEAD_T);
            num_read=fread(cmd_buf+1+sizeof(MSG_HEAD_T)+sizeof(DATA_HEAD_T),1,file_size,file);
            fclose(file);
            file=NULL;
            SHELL_DBG("%s\n",file_buf);            
        }else{          
            len=strlen(audio_data_buf)+sizeof(MSG_HEAD_T)+sizeof(DATA_HEAD_T)+2;
            cmd_buf=malloc(len);
            memcpy(cmd_buf+1+sizeof(MSG_HEAD_T)+sizeof(DATA_HEAD_T),audio_data_buf,strlen(audio_data_buf));
        }
    }

    memset(&msg_head,0,sizeof(MSG_HEAD_T));
    memset(&data_command,0,sizeof(DATA_HEAD_T));

    sub_type=strtoul(argv[1],NULL,10);
    msg_head.subtype=strtoul(argv[1],NULL,10);
    msg_head.len=len-2;
    msg_head.type=0x99;

    data_command.data_state=1;

    cmd_buf[0]=0x7e;
    memcpy(cmd_buf+1,&msg_head,sizeof(MSG_HEAD_T));
    memcpy(cmd_buf+1+sizeof(MSG_HEAD_T),&data_command,sizeof(DATA_HEAD_T));

    cmd_buf[len-1]=0x7e;

    SHELL_DBG("SIZE:%d\n",len);
    _dump_data(cmd_buf,len);
    return 0;
}

