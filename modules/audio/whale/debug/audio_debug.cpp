#define LOG_NDEBUG 0
#include "audio_hw.h"
#include "fcntl.h"
#include "audio_control.h"
#include "audio_xml_utils.h"
#include "audio_param/audio_param.h"
#include "tinyalsa_util.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "stdint.h"
#include "tinyxml.h"
#include "audio_debug.h"
#include <stdio.h>
#include <sys/stat.h>
#include "stdint.h"
#include "fcntl.h"
#include <stdlib.h>
#define LOG_TAG "audio_hw_debug"

int log_level=3;

static int _parse_audio_tunning_debug(param_group_t parm,struct 
    param_tunning_debug_handle * debug){
    const char *tmp=NULL;
    debug->hex_enable=0;
    debug->txt_enable=0;
    TiXmlElement *group = (TiXmlElement *)parm;

    ALOGD("_parse_audio_tunning_debug:%p",debug);
    tmp = group->Attribute("txt_enable");
    if(NULL!=tmp){
        debug->txt_enable=strtoul(tmp,NULL,0);
        ALOGD("txt_enable :%s val:%d",tmp,debug->txt_enable); 
    }

    tmp = group->Attribute("hex_enable");
    if(NULL!=tmp){
        debug->hex_enable=strtoul(tmp,NULL,0);
        ALOGD("hex_enable :%s val:%d",tmp,debug->hex_enable); 
    }

    tmp = group->Attribute("err_enable");
    if(NULL!=tmp){
        debug->err_enable=strtoul(tmp,NULL,0);
        ALOGD("err_enable :%s val:%d",tmp,debug->err_enable);
    }

    debug->hex_fd=-1;
    debug->txt_fd=-1;
    debug->err_fd=-1;

    if(debug->txt_enable){
        tmp = group->Attribute("txt_file");
        if(tmp!=NULL){
            debug->txt_fd=open(tmp, O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if(debug->txt_fd < 0) {
                ALOGE("open %s failed\n",tmp);
                goto Err;
            }
        }
        ALOGD("txt_file :%s txt_fd:0x%x %p",tmp,debug->txt_fd,debug);
    }

    if(debug->hex_enable){
        tmp = group->Attribute("hex_file");

        if(tmp!=NULL){
            debug->hex_fd=open(tmp, O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if(debug->hex_fd < 0) {
                ALOGE("open %s failed\n",tmp);
                goto Err;
            }
        }
        ALOGD("hex_file :%s hex_fd:0x%x %p",tmp,debug->hex_fd,debug);
    }

    if(debug->err_enable){
        tmp = group->Attribute("err_file");

        if(tmp!=NULL){
            debug->err_fd=open(tmp, O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if(debug->err_fd < 0) {
                ALOGE("open %s failed\n",tmp);
                goto Err;
            }
        }
        ALOGD("err_file :%s hex_fd:0x%x %p",tmp,debug->err_fd,debug);
    }

    return 0;
Err:
    if(debug->txt_fd > 0) {
        close(debug->txt_fd);
        debug->txt_fd=-1;
    }

    if(debug->hex_fd > 0) {
        close(debug->hex_fd);
        debug->hex_fd=-1;
    }
    return -1;
}

static int parse_audio_tunning_debug(param_group_t parm,struct tunning_debug *tunning){
    TiXmlElement * ele_tmp;
    const char *tmp;
    ALOGD("parse_audio_tunning_debug tx:%p rx:%p tunning:%p\n",
        &(tunning->tx_debug),
        &(tunning->rx_debug),
        tunning);
    ele_tmp=(TiXmlElement *)XML_get_first_sub_group(parm);
    while(ele_tmp!=NULL){
        tmp = ele_tmp->Value();
        if(0 == strncmp(tmp,"socket_rx",strlen("socket_rx"))){
        ALOGD("parse_audio_tunning_debug :%s %p",ele_tmp->Value(),
            &(tunning->rx_debug));
            _parse_audio_tunning_debug(ele_tmp,&(tunning->rx_debug));
        }else if(0 == strncmp(tmp,"socket_tx",strlen("socket_tx"))){
        ALOGD("parse_audio_tunning_debug :%s %p",ele_tmp->Value(),
            &(tunning->tx_debug)); 
            _parse_audio_tunning_debug(ele_tmp,&(tunning->tx_debug));
        }
        ele_tmp = (TiXmlElement *)XML_get_next_sibling_group(ele_tmp);
    }
    return 0;
}

static int _parse_audio_dump(param_group_t parm,struct param_pcm_dump_handle *dump){
    const char *tmp=NULL;
    dump->hex_enable=0;
    TiXmlElement *group = (TiXmlElement *)parm;

    ALOGD("_parse_audio_dump:%p",dump);

    tmp = group->Attribute("hex_enable");
    if(NULL!=tmp){
        dump->hex_enable=strtoul(tmp,NULL,0);
        ALOGD("hex_enable :%s val:%d",tmp,dump->hex_enable); 
    }

    dump->hex_fd=-1;

    if(dump->hex_enable){
        tmp = group->Attribute("hex_file");

        if(tmp!=NULL){
            dump->hex_fd=open(tmp, O_WRONLY | O_CREAT |O_TRUNC ,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if(dump->hex_fd < 0) {
                ALOGE("open %s failed\n",tmp);
                goto Err;
            }
            dump->hex_file_name = strdup(tmp);
        }
        ALOGD("hex_file :%s hex_fd:0x%x %p",dump->hex_file_name,dump->hex_fd,dump);
    }

    return 0;
Err:

    if(dump->hex_fd > 0) {
        close(dump->hex_fd);
        dump->hex_fd=-1;
    }
    return -1;
}

static int parse_audio_playback_dump(param_group_t parm,struct playback_dump *playback){
    TiXmlElement * ele_tmp;
    const char *tmp;
    ele_tmp=(TiXmlElement *)XML_get_first_sub_group(parm);
    while(ele_tmp!=NULL){
        tmp = ele_tmp->Value();
        if(0 == strncmp(tmp,"normal",strlen("normal"))){
            _parse_audio_dump(ele_tmp,&(playback->normal));
        }else if(0 == strncmp(tmp,"offload",strlen("offload"))){
            _parse_audio_dump(ele_tmp,&(playback->offload));
        }else if(0 == strncmp(tmp,"dsploop",strlen("dsploop"))){
            _parse_audio_dump(ele_tmp,&(playback->dsploop));
        }
        ele_tmp = (TiXmlElement *)XML_get_next_sibling_group(ele_tmp);
    }
    return 0;
}

static int parse_audio_record_dump(param_group_t parm,struct record_dump *record){
    TiXmlElement * ele_tmp;
    const char *tmp;
    ele_tmp=(TiXmlElement *)XML_get_first_sub_group(parm);
    while(ele_tmp!=NULL){
        tmp = ele_tmp->Value();
        if(0 == strncmp(tmp,"normal",strlen("normal"))){
            _parse_audio_dump(ele_tmp,&(record->normal));
        }else if(0 == strncmp(tmp,"dsploop",strlen("dsploop"))){
            _parse_audio_dump(ele_tmp,&(record->dsploop));
        }else if(0 == strncmp(tmp,"vbc",strlen("vbc"))){
            _parse_audio_dump(ele_tmp,&(record->vbc));
        }else if(0 == strncmp(tmp,"process",strlen("process"))){
            _parse_audio_dump(ele_tmp,&(record->process));
        }else if(0 == strncmp(tmp,"nr",strlen("nr"))){
            _parse_audio_dump(ele_tmp,&(record->nr));
        }
        ele_tmp = (TiXmlElement *)XML_get_next_sibling_group(ele_tmp);
    }
    return 0;
}

static int parse_audio_log_level(param_group_t parm){
    TiXmlElement * ele_tmp;
    const char *tmp;
    int ret=4;
    ele_tmp=(TiXmlElement *)parm;
    if(ele_tmp!=NULL){
        tmp=ele_tmp->Attribute("level");
        ret=strtoul(tmp,NULL,10);
        ALOGD("parse_audio_log_level level:%d",ret);
    }
    return ret;
}

#ifdef __cplusplus
extern "C" {
#endif
int parse_audio_debug(struct tiny_audio_device *adev){
    param_group_t root_param;
    TiXmlElement * ele_tmp;
    TiXmlDocument *doc;
    struct xml_handle xmlhandle;
    const char *tmp=NULL;

    ALOGD("parse_audio_debug");
    if(access(AUDIO_DEBUG_CONFIG_TUNNING_PATH, R_OK) == 0){
        load_xml_handle(&xmlhandle, AUDIO_DEBUG_CONFIG_TUNNING_PATH,AUDIO_DEBUG_CONFIG_FIRSTNAME);
        root_param = xmlhandle.param_root;
        if(NULL==root_param){
            load_xml_handle(&xmlhandle, AUDIO_DEBUG_CONFIG_PATH,AUDIO_DEBUG_CONFIG_FIRSTNAME);
        }
    }else{
        load_xml_handle(&xmlhandle, AUDIO_DEBUG_CONFIG_PATH,AUDIO_DEBUG_CONFIG_FIRSTNAME);
    }
    root_param = xmlhandle.param_root;
    memset(&adev->debug,0,sizeof(struct audio_debug_handle));

    if(NULL==root_param){
        return -1;
    }

    if(NULL!=root_param){
        ele_tmp=(TiXmlElement *)XML_get_first_sub_group(root_param);
        while(ele_tmp!=NULL){
            tmp = ele_tmp->Value();
            ALOGD("parse_audio_debug :%s %p",ele_tmp->Value(),&(adev->debug.tunning));
            if(strncmp(tmp,"tunning",strlen("tunning"))==0){
                parse_audio_tunning_debug(ele_tmp,&(adev->debug.tunning));
            }else if(strncmp(tmp,"log",strlen("log"))==0){
                adev->debug.log_level=parse_audio_log_level(ele_tmp);
                log_level=adev->debug.log_level;
            }else if(strncmp(tmp,"playback",strlen("playback"))==0){
                parse_audio_playback_dump(ele_tmp,&(adev->debug.playback));
            }else if(strncmp(tmp,"record",strlen("record"))==0){
                parse_audio_record_dump(ele_tmp,&(adev->debug.record));
            }else if(strncmp(tmp,"card_name",strlen("card_name"))==0){
                adev->debug.card_name=strdup(ele_tmp->Attribute("val"));
            }
            ele_tmp = (TiXmlElement *)XML_get_next_sibling_group(ele_tmp);
        }
        release_xml_handle(&xmlhandle);
    }
    return 0;
}

long getCurrentTimeUs(void)
{
   struct timeval tv;
   gettimeofday(&tv,NULL);
   return tv.tv_sec* 1000000 + tv.tv_usec;
}

static unsigned char* hex_to_string(unsigned char *data, int size){
    unsigned char* str=NULL;
    int str_size=0;
    int i=0;
    if(size<=0){
        return NULL;
    }
    str_size=size*2+1;
    str=(unsigned char*)malloc(str_size);
    if(NULL==str){
        return NULL;
    }

    for(i=0;i<size;i++){
        sprintf((char *)(str + i*2), "%02x", data[i]);
    }
    str[str_size]='\0';
    return str;
}

int string_to_hex(unsigned char * dst,const  char *str, int max_size){
    int size=0;
    const  char *tmp=str;
    unsigned char data=0;
    unsigned char char_tmp=0;
    while(1){
        char_tmp=(unsigned char)*tmp;
        if((char_tmp==NULL) || (char_tmp=='\0')){
            break;
        }
        if((char_tmp>='0') && (char_tmp<='9')){
            data= (char_tmp-'0')<<4;
        }else if((char_tmp>='a') && (char_tmp<='f')){
            data= ((char_tmp-'a')+10)<<4;
        }else if((char_tmp>='A') && (char_tmp<='F')){
            data= ((char_tmp-'A')+10)<<4;
        }else{
            break;
        }

        char_tmp=(unsigned char)*(tmp+1);
        if((char_tmp==NULL) || (char_tmp=='\0')){
            break;
        }

        if((char_tmp>='0') && (char_tmp<='9')){
            data |= (char_tmp-'0');
        }else if((char_tmp>='a') && (char_tmp<='f')){
            data |= ((char_tmp-'a')+10);
        }else if((char_tmp>='A') && (char_tmp<='F')){
            data |= (char_tmp-'A')+10;
        }else{
            break;
        }

        tmp+=2;
        dst[size++]=data;

        if(size>=max_size){
            break;
        }

        data=0;
    }
    return size;
}

#ifdef __cplusplus
}
#endif
