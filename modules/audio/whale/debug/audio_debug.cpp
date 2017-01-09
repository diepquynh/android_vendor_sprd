/*
* Copyright (C) 2010 The Android Open Source Project
* Copyright (C) 2012-2015, The Linux Foundation. All rights reserved.
*
* Not a Contribution, Apache license notifications and license are retained
* for attribution purposes only.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

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

    if((debug->hex_fd) && (debug->hex_enable==false)){
        close(debug->hex_fd);
        debug->hex_fd=-1;
    }

    if((debug->txt_fd) && (debug->txt_enable==false)){
        close(debug->txt_fd);
        debug->txt_fd=-1;
    }

    if((debug->err_fd) && (debug->err_enable==false)){
        close(debug->err_fd);
        debug->err_fd=-1;
    }

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
        debug->txt_fd=false;
    }

    if(debug->hex_fd > 0) {
        close(debug->hex_fd);
        debug->hex_fd=-1;
        debug->hex_enable=false;
    }

    if(debug->err_fd > 0) {
        close(debug->err_fd);
        debug->err_fd=-1;
        debug->err_enable=false;
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

    if((dump->hex_fd) && (dump->hex_enable==false)){
        close(dump->hex_fd);
        dump->hex_fd = -1;
    }

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
        }else if(0 == strncmp(tmp,"deepbuffer",strlen("deepbuffer"))){
            _parse_audio_dump(ele_tmp,&(playback->deep_buffer));
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
        }else if(0 == strncmp(tmp,"mixer_vbc",strlen("mixer_vbc"))){
            _parse_audio_dump(ele_tmp,&(record->mixer_vbc));
        }
        ele_tmp = (TiXmlElement *)XML_get_next_sibling_group(ele_tmp);
    }
    return 0;
}

static int parse_audio_dump(param_group_t parm,struct hex_dump_handle *dump){
    TiXmlElement * ele_tmp;
    const char *tmp;
    ele_tmp=(TiXmlElement *)XML_get_first_sub_group(parm);
    while(ele_tmp!=NULL){
        tmp = ele_tmp->Value();
        if(0 == strncmp(tmp,"playback",strlen("playback"))){
            parse_audio_playback_dump(ele_tmp,&dump->playback);
        }else if(0 == strncmp(tmp,"record",strlen("record"))){
            parse_audio_record_dump(ele_tmp,&dump->record);
        }else if(0 == strncmp(tmp,"tunning",strlen("tunning"))){
            parse_audio_tunning_debug(ele_tmp,&dump->tunning);
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
        ret=string_to_value(tmp);
        ALOGD("parse_audio_log_level level:%d",ret);
    }
    return ret;
}

static int parse_audio_mute_ctl(param_group_t parm,struct mute_control_name *mute){
    TiXmlElement * ele_tmp;
    const char *tmp;
    ele_tmp=(TiXmlElement *)XML_get_first_sub_group(parm);

    while(ele_tmp!=NULL){
        tmp = ele_tmp->Value();
        if(0 == strncmp(tmp,"spk_mute",strlen("spk_mute"))){
            tmp=ele_tmp->Attribute("ctl");
            if(NULL!=mute->spk_mute){
                free(mute->spk_mute);
                mute->spk_mute =NULL;
            }

            if(NULL!=tmp){
                mute->spk_mute=strdup(tmp);
            }
        }else if(0 == strncmp(tmp,"spk2_mute",strlen("spk2_mute"))){
            tmp=ele_tmp->Attribute("ctl");
            if(NULL!=mute->spk2_mute){
                free(mute->spk2_mute);
                mute->spk2_mute =NULL;
            }

            if(NULL!=tmp){
                mute->spk2_mute=strdup(tmp);
            }
        }else if(0 == strncmp(tmp,"handset_mute",strlen("handset_mute"))){
            tmp=ele_tmp->Attribute("ctl");
            if(NULL!=mute->handset_mute){
                free(mute->handset_mute);
                mute->handset_mute =NULL;
            }

            if(NULL!=tmp){
                mute->handset_mute=strdup(tmp);
            }
        }else if(0 == strncmp(tmp,"headset_mute",strlen("headset_mute"))){
            tmp=ele_tmp->Attribute("ctl");
            if(NULL!=mute->headset_mute){
                free(mute->headset_mute);
                mute->headset_mute =NULL;
            }

            if(NULL!=tmp){
                mute->headset_mute=strdup(tmp);
            }
        }else if(0 == strncmp(tmp,"linein_mute",strlen("linein_mute"))){
            tmp=ele_tmp->Attribute("ctl");
            if(NULL!=mute->linein_mute){
                free(mute->linein_mute);
                mute->linein_mute=NULL;
            }

            if(NULL!=tmp){
                mute->linein_mute=strdup(tmp);
            }
        }else if(0 == strncmp(tmp,"dsp_da0_mdg_mute",strlen("dsp_da0_mdg_mute"))){
            tmp=ele_tmp->Attribute("ctl");
            if(NULL!=mute->dsp_da0_mdg_mute){
                free(mute->dsp_da0_mdg_mute);
                mute->dsp_da0_mdg_mute=NULL;
            }

            if(NULL!=tmp){
                mute->dsp_da0_mdg_mute=strdup(tmp);
            }
        }else if(0 == strncmp(tmp,"dsp_da1_mdg_mute",strlen("dsp_da1_mdg_mute"))){
            tmp=ele_tmp->Attribute("ctl");
            if(NULL!=mute->dsp_da1_mdg_mute){
                free(mute->dsp_da1_mdg_mute);
                mute->dsp_da1_mdg_mute=NULL;
            }

            if(NULL!=tmp){
                mute->dsp_da1_mdg_mute=strdup(tmp);
            }
        }else if(0 == strncmp(tmp,"audio_mdg_mute",strlen("audio_mdg_mute"))){
            tmp=ele_tmp->Attribute("ctl");
            if(NULL!=mute->audio_mdg_mute){
                free(mute->audio_mdg_mute);
                mute->audio_mdg_mute=NULL;
            }

            if(NULL!=tmp){
                mute->audio_mdg_mute=strdup(tmp);
            }
        }
        ele_tmp = (TiXmlElement *)XML_get_next_sibling_group(ele_tmp);
    }

    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

int _parse_audio_config(struct audio_control *dev_ctl,struct xml_handle * xmlhandle){
    param_group_t root_param;
    TiXmlElement * ele_tmp;
    const char *tmp=NULL;
    root_param = xmlhandle->param_root;

    if(NULL==root_param){
        return -1;
    }

    if(NULL!=root_param){
        ele_tmp=(TiXmlElement *)XML_get_first_sub_group(root_param);
        while(ele_tmp!=NULL){
            tmp = ele_tmp->Value();
            if(strncmp(tmp,"log",strlen("log"))==0){
                dev_ctl->config.log_level=parse_audio_log_level(ele_tmp);
                log_level=dev_ctl->config.log_level;
            }else if(strncmp(tmp,"pcm_dump",strlen("pcm_dump"))==0){
                parse_audio_dump(ele_tmp,&(dev_ctl->config.dump));
            }else if(strncmp(tmp,"card_name",strlen("card_name"))==0){
                dev_ctl->config.card_name=strdup(ele_tmp->Attribute("val"));
            }else if(strncmp(tmp,"record_process",strlen("record_process"))==0){
                tmp=ele_tmp->Attribute("nr");
                if(NULL!=tmp){
                    dev_ctl->config.record.record_nr_enable=string_to_value(tmp);
                }else{
                    dev_ctl->config.record.record_nr_enable=false;
                }

                tmp=ele_tmp->Attribute("process");
                if(NULL!=tmp){
                    dev_ctl->config.record.record_process_enable=string_to_value(tmp);
                }else{
                    dev_ctl->config.record.record_process_enable=false;
                }
            }else if(strncmp(tmp,"fm",strlen("fm"))==0){
                tmp=ele_tmp->Attribute("fm_type");
                if(NULL!=tmp){
                    dev_ctl->config.fm_type=string_to_value(tmp);
                }else{
                    dev_ctl->config.fm_type=0;
                }
            }else if(strncmp(tmp,"support_24bits_output",strlen("support_24bits_output"))==0){
                tmp=ele_tmp->Attribute("val");
                if(NULL!=tmp){
                    dev_ctl->config.support_24bits=string_to_value(tmp);
                }else{
                    dev_ctl->config.support_24bits=false;
                }

            }else if(strncmp(tmp,"mic_config",strlen("mic_config"))==0){
                bool enable=false;
                tmp=ele_tmp->Attribute("main_mic");
                if(NULL!=tmp){
                    enable=string_to_value(tmp);
                    if(enable){
                        dev_ctl->config.mic_switch |= 1<<0;
                    }else{
                        dev_ctl->config.mic_switch &= ~(1<<0);
                    }
                }else{
                        dev_ctl->config.mic_switch &= ~(1<<0);
                }

                tmp=ele_tmp->Attribute("aux_mic");
                if(NULL!=tmp){
                    enable=string_to_value(tmp);
                    if(enable){
                        dev_ctl->config.mic_switch |= 1<<1;
                    }else{
                        dev_ctl->config.mic_switch &= ~(1<<1);
                    }
                }else{
                        dev_ctl->config.mic_switch &= ~(1<<1);
                }
            }else if(strncmp(tmp,"mute",strlen("mute"))==0){
                parse_audio_mute_ctl(ele_tmp,&(dev_ctl->config.mute));
            }
            ele_tmp = (TiXmlElement *)XML_get_next_sibling_group(ele_tmp);
        }
    }
    return 0;
}

int parse_audio_config(struct audio_control *dev_ctl){
    param_group_t root_param;
    TiXmlElement * ele_tmp;
    TiXmlDocument *doc;
    struct xml_handle xmlhandle;
    const char *tmp=NULL;
    memset(&dev_ctl->config,0,sizeof(struct audio_config_handle));

    ALOGD("parse_audio_config");
    if(access(AUDIO_DEBUG_CONFIG_TUNNING_PATH, R_OK) == 0){
        load_xml_handle(&xmlhandle, AUDIO_DEBUG_CONFIG_TUNNING_PATH);
        root_param = xmlhandle.param_root;
        if(NULL==root_param){
            load_xml_handle(&xmlhandle, AUDIO_DEBUG_CONFIG_PATH);
        }
    }else{
        load_xml_handle(&xmlhandle, AUDIO_DEBUG_CONFIG_PATH);
    }

    _parse_audio_config(dev_ctl,&xmlhandle);
    release_xml_handle(&xmlhandle);

    LOG_I("parse_audio_config :%x %x %x",
        dev_ctl->config.log_level,dev_ctl->config.fm_type,dev_ctl->config.mic_switch);
    return 0;
}

TiXmlElement *_audio_config_ctrl(param_group_t group, const char *param_name,char *val_str)
{
    char *sub_name;
    TiXmlElement *param_group;
    const char *tmp=NULL;
    TiXmlElement *pre_param = NULL;

    if (group == NULL)
    {
        LOG_E("the root is %p, goup_name is %s ", group, param_name);
        return NULL;
    }

    if (param_name == NULL)
    {
        return (TiXmlElement *)group;
    }

    char *name = strdup(param_name);

    sub_name = strtok(name, "/");

    LOG_I("%s %d values:%s %s",__func__,__LINE__,((TiXmlElement *)group)->Value(),sub_name);
    TiXmlElement *param = ((TiXmlElement *)group)->FirstChildElement(sub_name);
    if (param == NULL)
    {
        LOG_E("can not find the param group %s %s, %s",((TiXmlElement *)group)->Value(),sub_name, param_name);
        return NULL;
    }

    do
    {
        sub_name = strtok(NULL, "/");
        LOG_I("sub_name:%s",sub_name);
        if (sub_name == NULL) {
            LOG_E("_audio_config_ctrl %s %d",__func__,__LINE__);
            break;
        }

        pre_param = param;
        LOG_I("private_get_param %s find %s",param->Value(),sub_name);
        param = param->FirstChildElement(sub_name);

        if (param == NULL){
            tmp = pre_param->Attribute(sub_name);
            if(NULL!=tmp){
                pre_param->SetAttribute(sub_name,val_str);
                param = pre_param;
                break;
            }else{
                param=NULL;
                LOG_E("_audio_config_ctrl failed");
            }
        }
    }while (param != NULL);

    free(name);

    return param;
}

int audio_config_ctrl(void *dev,struct str_parms *parms,int opt, char * kvpair){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    struct xml_handle xmlhandle;
    struct xml_handle *xml=NULL;
    struct socket_handle *tunning=NULL;
    int ret=0;

    char *eq = strchr(kvpair, ',');
    char *svalue = NULL;
    char *key = NULL;
    TiXmlElement *tmpgroup = NULL;
    if (eq) {
        key = strndup(kvpair, eq - kvpair);
        if (*(++eq)) {
            svalue = strdup(eq);
        }
    }

    ALOGI("audio_config_ctrl xml:%p kvpair:%s key:%s valuse:%s \n\n",adev->dev_ctl->audio_param->tunning.audio_config_xml
        ,kvpair,key,svalue);

    if((adev->dev_ctl==NULL)||(adev->dev_ctl->audio_param==NULL)){
        LOG_E("audio_config_ctrl failed");
        ret=-1;
        return ret;
    }

    tunning=&(adev->dev_ctl->audio_param->tunning);

    if(tunning->wire_connected==true){
        if((adev->dev_ctl!=NULL)
            &&(adev->dev_ctl->audio_param!=NULL)){
            xml=(struct xml_handle *)tunning->audio_config_xml;
            if(xml==NULL){
                xml=(struct xml_handle *)malloc(sizeof(struct xml_handle));
                xml->first_name=NULL;
                xml->param_root=NULL;
                xml->param_doc=NULL;
            }
        }
    }else{
        xml=&xmlhandle;
        xml->first_name=NULL;
        xml->param_root=NULL;
        xml->param_doc=NULL;
    }

    if(NULL==xml->param_root){
        LOG_I("audio_config_ctrl load_xml_handle");
        if(access(AUDIO_DEBUG_CONFIG_TUNNING_PATH, R_OK) == 0){
            load_xml_handle(xml, AUDIO_DEBUG_CONFIG_TUNNING_PATH);
            if(NULL==xml->param_root){
                load_xml_handle(xml, AUDIO_DEBUG_CONFIG_PATH);
            }
        }else{
            load_xml_handle(xml, AUDIO_DEBUG_CONFIG_PATH);
        }
    }

    if(NULL!=xml->param_root){
        tmpgroup = (TiXmlElement *)_audio_config_ctrl(xml->param_root, key, svalue);
    }else{
        LOG_E("audio_config_ctrl param_root is null");\
        ret=-1;
        goto exit;
    }


    if(NULL == tmpgroup){
        LOG_E("audio_config_ctrl ERR key:%s values;%s",key,svalue);
        ret=-1;
    }else{
        _parse_audio_config(adev->dev_ctl,xml);
    }

exit:
    if(tunning->wire_connected==false){
        release_xml_handle(xml);
        tunning->audio_config_xml=NULL;
    }else{
        tunning->audio_config_xml=(void *)xml;
    }
    return ret;
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
