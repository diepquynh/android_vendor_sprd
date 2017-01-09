#include "audio_debug.h"
#include "audio_param/audio_param.h"
int _set_register_value(int paddr, int value)
{
    char cmd[128]={0};
    FILE *pipe_file=NULL;
    char result[128]={0};

    if (paddr & 0x3) {
        LOG_E("address should be 4-byte aligned\n");
        return -1;
    }

    snprintf(cmd,sizeof(cmd),"lookat -s 0x%x 0x%x",value,paddr);
    if((pipe_file=popen(cmd, "r"))!=NULL){
        fread(result, 1, sizeof(result), pipe_file); 
        pclose(pipe_file);
        pipe_file = NULL;
    }
    LOG_I("_set_register_value cmd:%s result:%s",cmd,result);
    return 0;
}


static int _get_register_value(int paddr, int nword, int *reg)
{
    char *result=NULL;
    char cmd[128]={0};
    FILE *pipe_file;

    int result_size=nword*32+96;
    result=(char *)malloc(result_size);
    if(NULL==result){
        printf("%s malloc :%d size failed\n",nword);
        return -1;
    }

    snprintf(cmd,sizeof(cmd),"lookat -l %d 0x%x",nword,paddr);
    if((pipe_file=popen(cmd, "r"))!=NULL){
        fread(result, 1, result_size, pipe_file);
        pclose(pipe_file);
        pipe_file = NULL;
    }

    {
        char *tmpstr=NULL;
        char data_buf[64]={0};
        char *line=NULL;
        char *reg_addr=NULL;
        char *reg_values=NULL;
        char *eq =NULL;
        int addr=0;
        int offset=0;
        line = strtok_r(result, SPLIT, &tmpstr);
        while (line != NULL) {
            memcpy(data_buf,line,strlen(line));
            char *eq = strchr(data_buf, '|');
            if (eq) {
                reg_addr = strndup(data_buf, eq - data_buf);
                if (*(++eq)) {
                    reg_values = strdup(eq);
                    addr=strtoul(reg_addr,NULL,0);
                    if(addr>paddr){
                        offset=(addr-paddr)/4;
                        if(offset>=nword){
                            LOG_E("addr err:%x :%s *----\n",addr,line);
                        }else{
                            reg[offset]=strtoul(reg_values,NULL,0);
                        }
                    }
                }
            }

            if(reg_addr!=NULL){
                free(reg_addr);
                reg_addr=NULL;
            }

            if(reg_values!=NULL){
                free(reg_values);
                reg_values=NULL;
            }
            line = strtok_r(NULL, SPLIT, &tmpstr);
        }

    }
    return 0;
}

int get_register_value(const char *infor, int size, uint8_t *data,
                       int max_data_size)
{
    /* addr=xx_addr,length=xx */
    char *tmp = NULL;
    char str_tmp[64];
    int str_size = 0;

    int addr = -1;
    int len = -1;
    char *reg = NULL;

    int i = 0;
    int string_size = 0;

    tmp = strchr(infor, ',');
    str_size = infor - tmp;
    if(str_size >= sizeof(str_tmp)) {
        str_size = sizeof(str_tmp) - 1;
    }
    strncpy(str_tmp, infor, str_size);
    if(strncmp(str_tmp, "addr", strlen("addr")) == 0) {
        tmp = strchr(str_tmp, '=');
        if(NULL == tmp) {
            return -1;
        }
        addr = strtoul(tmp, NULL, 16);
    } else if(strncmp(str_tmp, "length", strlen("length")) == 0) {
        tmp = strchr(str_tmp, '=');
        if(NULL == tmp) {
            return -2;
        }
        len = strtoul(tmp, NULL, 16);
    }

    tmp = strchr(infor, ',');
    strncpy(str_tmp, tmp + 1, sizeof(str_tmp) - 1);
    if(strncmp(str_tmp, "addr", strlen("addr")) == 0) {
        tmp = strchr(str_tmp, '=');
        if(NULL == tmp) {
            return -3;
        }
        addr = strtoul(tmp, NULL, 16);
    } else if(strncmp(str_tmp, "length", strlen("length")) == 0) {
        tmp = strchr(str_tmp, '=');
        if(NULL == tmp) {
            return -4;
        }
        len = strtoul(tmp, NULL, 16);
    }

    if((addr <= 0) || (len <= 0)) {
        return -5;
    }

    reg = malloc(len * sizeof(int32_t));
    if(NULL == reg) {
        return -6;
    }

    return _get_register_value(addr, size, data);
}

int set_register_value(const char *infor, int size)
{
    /* addr=0x400386c0,value=0x04 */
    char *tmp = NULL;
    char str_tmp[64];
    int str_size = 0;

    int addr = -1;
    int val = -1;

    tmp = strchr(infor, ',');
    str_size = infor - tmp;
    if(str_size >= sizeof(str_tmp)) {
        str_size = sizeof(str_tmp) - 1;
    }
    strncpy(str_tmp, infor, str_size);
    if(strncmp(str_tmp, "addr", strlen("addr")) == 0) {
        tmp = strchr(str_tmp, '=');
        if(NULL == tmp) {
            return -1;
        }
        addr = strtoul(tmp, NULL, 16);
    } else if(strncmp(str_tmp, "value", strlen("value")) == 0) {
        tmp = strchr(str_tmp, '=');
        if(NULL == tmp) {
            return -2;
        }
        val = strtoul(tmp, NULL, 16);
    }

    tmp = strchr(infor, ',');
    strncpy(str_tmp, tmp + 1, sizeof(str_tmp) - 1);
    if(strncmp(str_tmp, "addr", strlen("addr")) == 0) {
        tmp = strchr(str_tmp, '=');
        if(NULL == tmp) {
            return -3;
        }
        addr = strtoul(tmp, NULL, 16);
    } else if(strncmp(str_tmp, "value", strlen("value")) == 0) {
        tmp = strchr(str_tmp, '=');
        if(NULL == tmp) {
            return -4;
        }
        val = strtoul(tmp, NULL, 16);
    }

    if((addr <= 0) || (val < 0)) {
        return -5;
    }
    return _set_register_value(addr, val);
}
