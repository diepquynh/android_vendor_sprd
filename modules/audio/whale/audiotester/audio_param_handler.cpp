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

#define LOG_TAG "audio_hw_param_handler"
#define LOG_NDEBUG 0

#include "tinyxml.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "string.h"
#include "cutils/log.h"
#include "audio_param/audio_param.h"
#include "audio_xml_utils.h"
#include "audio_debug.h"
#include "audio_param/audio_param.h"
#include "audiotester/audiotester_server.h"
#include "audio_control.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LINE_LEN 512
#define SPLIT "\n"
#define BACKSLASH "\\"
#define MAX_SOCKT_LEN 65535
#define MAX_AUDIO_PARAM_TIME_STR_LEN 128
static int set_audio_param(AUDIO_PARAM_T *audio_param, int cmd, char *buf, int len);
extern int get_register_value(const char *infor,int size, uint8_t * data, int max_data_size);
extern int set_register_value(const char *infor,int size);
extern int get_audio_param_mode_name(char *str);
int send_cmd_to_dsp_thread(struct dsp_control_t *agdsp_ctl,int cmd,void * parameter);
extern int set_dsp_volume(struct audio_control *ctl,int volume);
int ext_contrtol_process(struct tiny_audio_device *adev,const char *cmd_string);

void clear_g_data_buf(struct socket_handle *tunning);

#define GET_AUDIO_PARAM_CHANGE_FLAGE(param,param_string,ret)            \
    do{         \
        int profile=-1;         \
        profile=select_the_param_profile(param,param_string);         \
        switch(profile) {         \
        case SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP :         \
            ret |= ENG_DSP_VBC_OPS;         \
            break;         \
        case SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE :         \
            ret |= ENG_AUDIO_STRECTURE_OPS;         \
            break;         \
        case SND_AUDIO_PARAM_NXP_PROFILE :         \
            ret |= ENG_NXP_OPS;         \
            break;         \
        case SND_AUDIO_PARAM_CODEC_PROFILE :         \
            ret |= ENG_CODEC_OPS;         \
            break;         \
        case SND_AUDIO_PARAM_PGA_PROFILE :         \
            ret |= ENG_PGA_OPS;         \
            break;         \
        case SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE :         \
            ret |= ENG_AUDIO_PROCESS_OPS;         \
            break;         \
        default:         \
            LOG_D("%s %d err profile:%d",__func__,__LINE__,profile);         \
            ret = -1;         \
            break;         \
        }         \
    }while(0)

void add_path(char *dst, const char *src)
{
    strcat(dst, src);
    strcat(dst, BACKSLASH);
}

int connect_audiotester_process(struct socket_handle *tunning_handle,int sockfd,int max_buffer_size,void *fun){
    tunning_handle->seq=0;
    tunning_handle->rx_packet_total_len=0;
    tunning_handle->rx_packet_len=0;
    tunning_handle->max_len=max_buffer_size;
    tunning_handle->data_state=0;

    for(int i=0;i<SND_AUDIO_PARAM_PROFILE_MAX;i++){
        tunning_handle->param_sync[i]=false;
    }

    if(NULL == tunning_handle->audio_received_buf){
        tunning_handle->audio_received_buf=(uint8_t *)malloc(tunning_handle->max_len);
        if(tunning_handle->audio_received_buf==NULL){
            LOG_E("connect_audiotester_process malloc audio_received_buf failed");
            goto Err;
        }
    }

    if(NULL == tunning_handle->audio_cmd_buf){
        tunning_handle->audio_cmd_buf=(uint8_t *)malloc(tunning_handle->max_len);
        if(tunning_handle->audio_cmd_buf==NULL){
            LOG_E("connect_audiotester_process malloc audio_cmd_buf failed");
            goto Err;
        }
    }

    if(NULL == tunning_handle->send_buf){
        tunning_handle->send_buf=(uint8_t *)malloc(tunning_handle->max_len);
        if(tunning_handle->send_buf==NULL){
            LOG_E("connect_audiotester_process malloc send_buf failed");
            goto Err;
        }
    }

    if(NULL == tunning_handle->time_buf){
        tunning_handle->time_buf=(uint8_t *)malloc(MAX_AUDIO_PARAM_TIME_STR_LEN);
        if(tunning_handle->time_buf==NULL){
            LOG_E("connect_audiotester_process malloc time_buf failed");
            goto Err;
        }
        memset(tunning_handle->time_buf, 0, MAX_AUDIO_PARAM_TIME_STR_LEN);
        memcpy(tunning_handle->time_buf, "unknow time", strlen("unknow time"));
    }

#if 0
    if(NULL == tunning_handle->data_buf){
        tunning_handle->data_buf=(uint8_t *)malloc(tunning_handle->max_len);
        if(tunning_handle->data_buf==NULL){
            LOG_E("connect_audiotester_process malloc data_buf failed");
            goto Err;
        }
    }
#else
    tunning_handle->data_buf=tunning_handle->send_buf+1+sizeof(MSG_HEAD_T) + sizeof(DATA_HEAD_T);
#endif
    LOG_D("connect_audiotester_process 0x%p 0x%p 0x%p 0x%p",
        tunning_handle->audio_received_buf,
        tunning_handle->audio_cmd_buf,
        tunning_handle->send_buf,
        tunning_handle->data_buf);
    tunning_handle->sockfd=sockfd;
    tunning_handle->wire_connected=true;
    tunning_handle->process=(AUDIO_SOCKET_PROCESS_FUN)fun;
    tunning_handle->update_flag=0;

    clear_g_data_buf(tunning_handle);

    return 0;

Err:

    if(tunning_handle->audio_received_buf!=NULL){
        free(tunning_handle->audio_received_buf);
        tunning_handle->audio_received_buf=NULL;
    }

    if(tunning_handle->audio_cmd_buf!=NULL){
        free(tunning_handle->audio_cmd_buf);
        tunning_handle->audio_cmd_buf=NULL;
    }

    return -1;
}

int disconnect_audiotester_process(struct socket_handle *tunning_handle){

    if(false==tunning_handle->wire_connected){
        LOG_D("disconnect_audiotester_process:audiotester is not connect");
        return 0;
    }

    tunning_handle->seq=0;
    tunning_handle->rx_packet_total_len=0;
    tunning_handle->rx_packet_len=0;
    tunning_handle->wire_connected=false;

    if(tunning_handle->audio_received_buf!=NULL){
        free(tunning_handle->audio_received_buf);
        tunning_handle->audio_received_buf=NULL;
    }

    if(tunning_handle->audio_cmd_buf!=NULL){
        free(tunning_handle->audio_cmd_buf);
        tunning_handle->audio_cmd_buf=NULL;
    }

    if(tunning_handle->send_buf!=NULL){
        free(tunning_handle->send_buf);
        tunning_handle->send_buf=NULL;
    }

    if(tunning_handle->time_buf!=NULL){
        free(tunning_handle->time_buf);
        tunning_handle->time_buf=NULL;
    }
#if 0
    if(tunning_handle->data_buf!=NULL){
        free(tunning_handle->data_buf);
        tunning_handle->data_buf=NULL;
    }
#else
    tunning_handle->data_buf=NULL;
#endif
    tunning_handle->sockfd=-1;
    tunning_handle->process=NULL;
    return 0;
}

void append_g_data_buf(struct socket_handle *tunning,const char *value)
{
    if (value == NULL) {
        return;
    }

    if(tunning==NULL){
        return;
    }
    strcat((char *)tunning->data_buf, value);
    tunning->cur_len += strlen(value);
}

void clear_g_data_buf(struct socket_handle *tunning)
{
    memset(tunning->send_buf, 0, tunning->max_len);
    memset(tunning->send_buf, '0', sizeof(MSG_HEAD_T) + sizeof(DATA_HEAD_T)+1);
    tunning->cur_len = 0;
}

static void dump_line(int line_number,char *buf, int size){
    int i=0;
    char *dump_buf=NULL;
    char *start=NULL;
    int line_size=0;

    for(;i<size;i++){
        if(((buf[i]>='a') && (buf[i]<='z')) || ((buf[i]>='A') && (buf[i]<='Z'))){
            start=&buf[i];
            break;
        }
    }

    for(;i<size;i++){
        if(buf[i]==0x0a){
            if(line_number--){
                break;
            }
            line_size++;
        }
        line_size++;
    }

    if(NULL==dump_buf){
        dump_buf=(char *)malloc(line_size+1);
        if(NULL==dump_buf){
            LOG_E("%s malloc %d bytes failed",i+1);
            return;
        }
        memset(dump_buf,0,line_size+1);
        memcpy(dump_buf,start,line_size);
        LOG_I("start:%s",dump_buf);
        free(dump_buf);
    }

    return;
}


static  void _dump_line(int line_number,char *buf, int size){

    int i=size;
    char *dump_buf=NULL;
    char *start=NULL;
    int line_size=0;
    while(i--){
        if(((buf[i]>='a') && (buf[i]<='z')) || ((buf[i]>='A') && (buf[i]<='Z'))){
            start=&buf[i];
            break;
        }
    }

    while(i--){
        if(buf[i]==0x0a){
            if(line_number--){
                break;
            }
            line_size++;
        }
        line_size++;
    }
   LOG_I("end  :%s",buf+i+1);
}

void send_buffer(struct socket_handle *tunning,int sub_type, int data_state)
{
    int index = 0;
    int ret;
    MSG_HEAD_T m_head;
    DATA_HEAD_T data_head;
    uint8_t * send_buf;

    LOG_I("send_buffer:%p %p :%d",tunning,tunning->send_buf,index);

    send_buf=tunning->send_buf;

    *(send_buf + index) = 0x7e;
    index += 1;

    m_head.seq_num = tunning->diag_seq;
    if(0x7e==(m_head.seq_num& 0xff)){
        LOG_E("seq_num is :0x%x",m_head.seq_num);
        tunning->diag_seq++;
        m_head.seq_num = tunning->diag_seq;
    }

    tunning->diag_seq++;
    m_head.type = 0x99;//spec
    m_head.subtype = sub_type;
    m_head.len = tunning->cur_len + sizeof(MSG_HEAD_T) + sizeof(DATA_HEAD_T);
    if(0x7e==(m_head.len& 0xff)){
        LOG_E("m_head len is :0x%x",m_head.len);
    }
    memcpy(send_buf + index, &m_head, sizeof(MSG_HEAD_T));
    index += sizeof(MSG_HEAD_T);

    data_head.data_state = data_state;
    memcpy(send_buf + index, &data_head, sizeof(DATA_HEAD_T));
    index += sizeof(DATA_HEAD_T);
    if (tunning->cur_len != 0) {
        index += tunning->cur_len;
    }
    *(send_buf + index) = 0x7e;
    index += 1;
#if 0
    if(AUDIO_EXT_TEST_CMD!=sub_type){
        dump_line(2,(char *)send_buf+sizeof(MSG_HEAD_T) + sizeof(DATA_HEAD_T)+1 ,index-sizeof(MSG_HEAD_T) - sizeof(DATA_HEAD_T)-1);
        _dump_line(2,(char *)send_buf+sizeof(MSG_HEAD_T) + sizeof(DATA_HEAD_T)+1 ,index-sizeof(MSG_HEAD_T) - sizeof(DATA_HEAD_T)-1);
    }
#endif
    ret = write(tunning->sockfd, send_buf, index);

/*
    if(NULL!=tunning->debug){
        if(audio_param->config->dump.tunning.tx_debug.txt_fd>0){
            LOG_D("send_buffer txt_fd:0x%x 0x%x %p %p",
                audio_param->config->dump.tunning.tx_debug.txt_fd,audio_param->config->dump.tunning.rx_debug.txt_fd,
                &tunning->debug->tx_debug,
                &tunning->debug->rx_debug);
            write(audio_param->config->dump.tunning.tx_debug.txt_fd, tunning->data_buf, tunning->cur_len);
        }
        if(audio_param->config->dump.tunning.tx_debug.hex_fd>0){
            write(audio_param->config->dump.tunning.tx_debug.hex_fd, send_buf, index);
        }
    }
*/
    LOG_I("send the param data to AudioTester  send=%d:%d state:%d\n",  index,
    ret,data_state);

    clear_g_data_buf(tunning);
}

void send_error(struct socket_handle *tunning,const char *error)
{
    LOG_E("send audiotester err:  %s", error);
    int index = 0;
    MSG_HEAD_T m_head;
    DATA_HEAD_T data_head;
    char error_buf[1000];
    *(error_buf + index) = 0x7e;
    index += 1;
    m_head.seq_num = tunning->diag_seq++;
    m_head.type = 0x99;//spec
    //m_head.subtype = sub_type;
    m_head.len = sizeof(MSG_HEAD_T) + sizeof(DATA_HEAD_T) + strlen(error);
    memcpy(error_buf + index, &m_head, sizeof(MSG_HEAD_T));
    index += sizeof(MSG_HEAD_T);

    data_head.data_state = DATA_STATUS_ERROR;
    memcpy(error_buf + index, &data_head, sizeof(DATA_HEAD_T));
    index += sizeof(DATA_HEAD_T);
    strcpy(error_buf + index, error);
    index += strlen(error);
    *(error_buf + index) = 0x7e;
    index += 1;

    write(tunning->sockfd, error_buf, index);
}


static int select_the_param_profile(AUDIO_PARAM_T *audio_param,const char *kvpair){
    for(int i=0;i<SND_AUDIO_PARAM_PROFILE_MAX;i++){
        if(strncmp(audio_param->param[i].xml.first_name, kvpair,strlen(audio_param->param[i].xml.first_name) )== 0){
            return i;
        }
    }
    return -1;
}

static int do_send_all_group_param(struct socket_handle *tunning,TiXmlElement *param_group, char *prev_path,
                            bool root, int sub_type)
{
    char cur_path[MAX_LINE_LEN] = {0};
    const char *val;
    const char *name;
    const char *visible;
    int int_val = 0;
    int true_string_size=strlen(TRUE_STRING);
    char string_val[16] = {0};
    int ret = 0;
    bool send_folder = false;
    if (prev_path != NULL) {
        strcat(cur_path, prev_path);
    }

    TiXmlElement *group = param_group;
    while (group != NULL) {
        visible = group->Attribute(VISIBLE);
        if((NULL==visible) || ((NULL != visible) && (0 ==
        strncmp(visible,TRUE_STRING,true_string_size)))){
        val = group->Attribute(VALUE);
        name = group->Attribute(NAME);
        if (val == NULL) {
            char next_path[MAX_LINE_LEN] = {0};
            strcat(next_path, cur_path);
            if (name == NULL) {
                add_path(next_path, group->Value());
            } else {
                add_path(next_path, name);
            }
            do_send_all_group_param(tunning,group->FirstChildElement(), next_path, false, sub_type);
        } else {
                append_g_data_buf(tunning,cur_path);
                if (name == NULL) {
                    append_g_data_buf(tunning,group->Value());
                } else {
                    append_g_data_buf(tunning,name);
                }
                LOG_D("%s%s=%s",cur_path,group->Value(),group->Attribute(VALUE));
                append_g_data_buf(tunning,"=");
                append_g_data_buf(tunning,group->Attribute(VALUE));
                append_g_data_buf(tunning,SPLIT);
                if (tunning->cur_len > tunning->max_len-1024) {
                    int data_state;
                    if (tunning->data_state == DATA_START) {
                        data_state = DATA_START;
                        tunning->data_state = DATA_MIDDLE;
                    } else {
                        data_state = DATA_MIDDLE;
                    }
                    send_buffer(tunning,sub_type, data_state);
                    send_folder = false;
                }
            }
        }
        if (root) {
            break; //didnt loop the root's Sibling
        }

        group = group->NextSiblingElement();
    }
    return ret;
}

static int init_audio_param_xml(AUDIO_PARAM_T *audio_param,int profile)
{
    int ret=0;
    TiXmlElement *root=(TiXmlElement *)audio_param->param[profile].xml.param_root;

    if(root != NULL) {
        XML_save_param(audio_param->param[profile].xml.param_doc,
                        audio_param_file_table[profile].tunning_file);
        release_xml_handle(&(audio_param->param[profile].xml));
        save_audio_param_to_bin(audio_param,profile);
    }

    if(access(audio_param_file_table[profile].tunning_file, R_OK) == 0) {
        ret=load_xml_handle(&(audio_param->param[profile].xml),
                        audio_param_file_table[profile].tunning_file);
    } else {
        ret=load_xml_handle(&(audio_param->param[profile].xml),
                        audio_param_file_table[profile].src_file);
    }
    return ret;
}

static int init_from_flash(AUDIO_PARAM_T *audio_param)
{
    TiXmlElement *root;
    int ret = 0;
    int profile = 0;

    for(profile = SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP;
        profile <SND_AUDIO_PARAM_PROFILE_MAX; profile++) {
        root = (TiXmlElement *)audio_param->param[profile].xml.param_root;
        ret=init_audio_param_xml(audio_param,profile);
    }

    return ret;
}

int save_audio_param(AUDIO_PARAM_T *audio_param,int profile, bool isram)
{
    TiXmlElement *root;
    int ret = 0;

    if(profile <SND_AUDIO_PARAM_PROFILE_MAX){
        LOG_D("save_audio_param profile:%d isram:%d",profile,isram);
        save_audio_param_to_bin(audio_param,profile);//save audio param bin file

        //save audio param to xml file
        if(false==isram){
            root = (TiXmlElement *)audio_param->param[profile].xml.param_root;
            if(root != NULL) {
                LOG_I("save_audio_param profile:%d %s param_doc:%p", profile,
                    audio_param_file_table[profile].tunning_file,audio_param->param[profile].xml.param_doc);
                XML_save_param(audio_param->param[profile].xml.param_doc,
                               audio_param_file_table[profile].tunning_file);
            }
        }
    }else{
        ret=-1;
    }
    return ret;
}

int get_audio_param_infor(AUDIO_PARAM_T *audio_param,
                         int sub_type){
    struct socket_handle * tunning;
    char infor[20480]={0};
    char tmp[128]={0};

    sprintf(tmp,"// ChipName=Whale");
    strcat(infor,tmp);
    strcat(infor, SPLIT);

    sprintf(tmp,"// AudioParamVersion=1.0");
    strcat(infor,tmp);
    strcat(infor, SPLIT);
    sprintf(tmp,"// SocketBufferSize=0x%x",MAX_SOCKT_LEN);
    strcat(infor,tmp);
    strcat(infor, SPLIT);
    sprintf(tmp,"// Config\\Record\\Nr=Yes");
    strcat(infor,tmp);
    strcat(infor, SPLIT);
    if((audio_param->dev_ctl->codec_type>=0) && (audio_param->dev_ctl->codec_type<AUD_CODEC_TYPE_MAX)){
        sprintf(tmp,"// Config\\Codec\\Type=%s",audio_codec_chip_name[audio_param->dev_ctl->codec_type]);
    }else{
        sprintf(tmp,"// Config\\Codec\\Type=unknow");
    }

    strcat(infor,tmp);
    strcat(infor, SPLIT);

    if((audio_param->config->fm_type>=AUD_FM_DIGITAL_TYPE) && (audio_param->config->fm_type<AUD_TYPE_MAX)){
        sprintf(tmp,"// Config\\FM\\Type=%s",fm_type_name[audio_param->config->fm_type]);
    }else{
        sprintf(tmp,"// Config\\FM\\Type=unknow");
    }

    strcat(infor,tmp);
    strcat(infor, SPLIT);

    get_audio_param_mode_name(infor);

    LOG_I("get_audio_param_infor:%s",infor);
    tunning=&audio_param->tunning;

    append_g_data_buf(tunning,infor);
    send_buffer(tunning,sub_type, DATA_SINGLE);
    return 0;
}

int get_current_audioparam(AUDIO_PARAM_T *audio_param,
                         int sub_type){
    struct audio_control *dev_ctl=audio_param->dev_ctl;
    struct audio_param_res  *param_res = &(dev_ctl->param_res);
    struct socket_handle * tunning;
    const char *param_name=NULL;
    bool flag=false;

    tunning=&audio_param->tunning;

    if((param_res->cur_vbc_playback_id >=PROFILE_MODE_MAX) &&((param_res->cur_vbc_id >=PROFILE_MODE_MAX))){
        append_g_data_buf(tunning,"NULL");
        send_buffer(tunning,sub_type, DATA_SINGLE);
        return 0;
    }

    if(param_res->cur_vbc_id <PROFILE_MODE_MAX){
        param_name=get_audio_param_name(param_res->cur_vbc_id);
        if(NULL!=param_name){
            append_g_data_buf(tunning,get_audio_param_name(param_res->cur_vbc_id));
            flag=true;
        }else{
            LOG_I("%s %d %d",__func__,__LINE__,param_res->cur_vbc_id);
        }
        param_name=NULL;
    }

    if((param_res->cur_vbc_playback_id <PROFILE_MODE_MAX)
        &&(param_res->cur_vbc_playback_id !=param_res->cur_vbc_id)){
        if(true==flag){
            append_g_data_buf(tunning,",");
        }
        param_name=get_audio_param_name(param_res->cur_vbc_playback_id);
        if(NULL!=param_name){
            append_g_data_buf(tunning,get_audio_param_name(param_res->cur_vbc_playback_id));
            flag=true;
        }else{
            LOG_I("%s %d %d",__func__,__LINE__,param_res->cur_vbc_playback_id);
        }
        param_name=NULL;
    }
    send_buffer(tunning,sub_type, DATA_SINGLE);
    return 0;
}

static int send_all_audio_param(AUDIO_PARAM_T *audio_param,
                         int sub_type)
{
    TiXmlElement *root;
    TiXmlElement *param_group;
    struct xml_handle *xml;
    char  root_path[32] = {0};
    int ret = 0;
    int profile=-1;
    audio_param->tunning.data_state = DATA_START;

    clear_g_data_buf(&audio_param->tunning);
    for(profile=0;profile<SND_AUDIO_PARAM_PROFILE_MAX;profile++){
        root = (TiXmlElement *)audio_param->param[profile].xml.param_root;
        if((GET_PARAM_FROM_FLASH==sub_type)&&(audio_param->tunning.param_sync[profile]==true)){
            if(profile==SND_AUDIO_PARAM_PGA_PROFILE){
                reload_sprd_audio_pga_param_withflash(audio_param);
            }else if(profile==SND_AUDIO_PARAM_PGA_PROFILE){
                reload_sprd_audio_process_param_withflash(audio_param);
            }else{
                LOG_I("send_all_audio_param profile:%d sync start",profile);
                release_xml_handle(&(audio_param->param[profile].xml));
                init_sprd_audio_param_from_xml(audio_param,profile);
                LOG_I("send_all_audio_param sync end");
            }
            audio_param->tunning.param_sync[profile]==false;
        }

        LOG_I("send_all_audio_param:%d",profile);
        if (audio_param->tunning.cur_len > 0) {
            int data_state;
            if (audio_param->tunning.data_state == DATA_START) {
                data_state = DATA_START;
                audio_param->tunning.data_state = DATA_MIDDLE;
            } else {
                data_state = DATA_MIDDLE;
            }
            send_buffer(&audio_param->tunning,sub_type, data_state);
        }
        if (root != NULL) {
            memset(root_path,0,sizeof(root_path));
            add_path(root_path, root->Value());
            param_group = root->FirstChildElement();
            while(param_group != NULL) {
                LOG_I("send_all_audio_param %s->:%s",root_path,param_group->Value());
                ret = do_send_all_group_param(&audio_param->tunning,param_group, root_path, true, sub_type);
                param_group = param_group->NextSiblingElement();
            }
        } else {
            LOG_E("profile:%d param_root is null",profile);
        }
    }
    if(ret >= 0) {
        send_buffer(&audio_param->tunning,sub_type, DATA_END);
    } else {
        send_buffer(&audio_param->tunning,sub_type, DATA_STATUS_ERROR);
    }
    return ret;
}

int read_and_send_param(struct socket_handle *tunning,param_group_t group,const char * path, const char *kvpair)
{
    char *eq = strchr(kvpair, '=');
    const char *svalue;
    char *key = NULL;
    if (eq) {
        key = strndup(kvpair, eq - kvpair);
    } else {
        LOG_E("read_and_send_param find = err");
        key = strdup(kvpair);

    }
    svalue = XML_get_string_in_group(group, key);
    if (svalue == NULL) {
        LOG_E("read_and_send_param can not find the value %s", kvpair);
        return -1;
    }

    append_g_data_buf(tunning,path);
    append_g_data_buf(tunning,key);
    append_g_data_buf(tunning,"=");
    append_g_data_buf(tunning,svalue);
    append_g_data_buf(tunning,SPLIT);
    LOG_D("read_and_send_param end:%s", svalue);
    if (key) { free(key); }

    return 0;
}

int send_part_audio_param(AUDIO_PARAM_T *audio_param,
                          char *buf, int len, int sub_type)
{
    int ret = 0;
    char *tmpstr;
    int profile=-1;
    param_group_t sub_group = NULL;
    bool flash_sync[SND_AUDIO_PARAM_PROFILE_MAX]={false};
    char data_buf[MAX_SOCKT_LEN];
    memcpy(data_buf, buf, len);
    *(data_buf + len) = '\0';

    audio_param->tunning.data_state = DATA_START;
    char *line = strtok_r(data_buf, SPLIT, &tmpstr);
    while (line != NULL) {
        param_group_t root;
        //LOG_I("send_part_audio_param:%s", line);
        profile = select_the_param_profile(audio_param,line);
        if(profile >= 0) {
            char path[512]={0};

            if((GET_PARAM_FROM_FLASH==sub_type)&&(audio_param->tunning.param_sync[profile]==true)){
                if(profile==SND_AUDIO_PARAM_PGA_PROFILE){
                    reload_sprd_audio_pga_param_withflash(audio_param);
                }else if(profile==SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE){
                    reload_sprd_audio_process_param_withflash(audio_param);
                }else{
                    LOG_I("send_part_audio_param profile:%d sync start",profile);
                    release_xml_handle(&(audio_param->param[profile].xml));
                    init_sprd_audio_param_from_xml(audio_param,profile);
                    LOG_I("send_part_audio_param sync end");
                }
                audio_param->tunning.param_sync[profile]==false;
            }
            root = audio_param->param[profile].xml.param_root;
            add_path(path,audio_param->param[profile].xml.first_name);
            ret=read_and_send_param(&audio_param->tunning,root, path,
                line+strlen(audio_param->param[profile].xml.first_name)+1);
            if(ret<0){
                TiXmlElement *ele_tmp=NULL;
                memset(path,0,sizeof(path));
                ele_tmp =(TiXmlElement *)XML_open_param_group(audio_param->param[profile].xml.param_root,
                    line+strlen(audio_param->param[profile].xml.first_name)+1);
                if(NULL!=ele_tmp){
                    char * str1 = strstr(line, ele_tmp->Value());
                    if(str1==NULL){
                        LOG_E("send_part_audio_param str1:%s",ele_tmp->Value())
                            return -1;
                    }
                    LOG_I("send_part_audio_param befor copy:%p :%p  %d",line,str1,str1-line-1);
                    memcpy(path,line,str1-line-1);
                    strcat(path, BACKSLASH);
                    LOG_E("send_part_audio_param next path:%s err",path);
                    ret = do_send_all_group_param(&audio_param->tunning,ele_tmp, path, true, sub_type);
                }
                break;
            }
            if (audio_param->tunning.cur_len > audio_param->tunning.max_len-1024) {
                int data_state;
                if (audio_param->tunning.data_state == DATA_START){
                    data_state = DATA_START;
                    audio_param->tunning.data_state = DATA_MIDDLE;
                } else {
                    data_state = DATA_MIDDLE;
                }

                send_buffer(&audio_param->tunning,sub_type, data_state);
            }
        } else {
            ret= -1;
        }
        line = strtok_r(NULL, SPLIT, &tmpstr);
    }

    if((profile >= 0) && (ret >= 0)) {
        if(DATA_START == audio_param->tunning.data_state){
            audio_param->tunning.data_state=DATA_SINGLE;
            send_buffer(&audio_param->tunning,sub_type, DATA_SINGLE);
        }else{
            audio_param->tunning.data_state=DATA_END;
            send_buffer(&audio_param->tunning,sub_type, DATA_END);
        }
    } else {
        send_buffer(&audio_param->tunning,sub_type, DATA_STATUS_ERROR);
    }
    return ret;
}

static int do_save_param(AUDIOVBCEQ_PARAM_T *param, param_group_t group,
                  char *kvpair)
{
    int ret = 0;
    char *eq = strchr(kvpair, '=');
    char *svalue = NULL;
    char *key = NULL;
    TiXmlElement *tmpgroup = NULL;
    if (eq) {
        key = strndup(kvpair, eq - kvpair);
        if (*(++eq)) {
            svalue = strdup(eq);
        }
    }
    tmpgroup = (TiXmlElement *)XML_set_string_in_group(group, key, svalue);
    if(NULL == tmpgroup){
        LOG_E("do_save_param ERR key:%s values;%s",key,svalue);
        ret=-1;
    }else{
        if((NULL != param) && (NULL !=param->data)){
            ret =read_audio_param_for_element(param->data,tmpgroup);
        }else{
            ret=0;
        }
    }

Err:
    if (key) {
        free(key);
        key = NULL;
    }
    if (svalue) {
        free(svalue);
        svalue = NULL;
    }
    return ret;
}

static int get_new_line(char *buf, int len){
    int i=0;

    for(;i<len;i++){
        if(buf[i] == 0x5c){
            buf[i] = 0x2f;
        }else if(0x0a==buf[i]){
            return i+1;
        }
    }
    return len;
}

static int audioteser_pcm_dump(AUDIO_PARAM_T *audio_param, int cmd, char *buf, int len)
{
    int ret = 0;
    AUDIOVBCEQ_PARAM_T *param_data_buffer=NULL;
    char *tmpstr;
    char data_buf[MAX_SOCKT_LEN];
    int count=0;
    memcpy(data_buf, buf, len);
    *(data_buf + len) = '\0';
    char *line = strtok_r(data_buf, SPLIT, &tmpstr);
    while (line != NULL) {
        ext_contrtol_process(audio_param->dev_ctl->adev,line);
        line = strtok_r(NULL, SPLIT, &tmpstr);
    }
    return ret;
}
static int set_audio_param(AUDIO_PARAM_T *audio_param, int cmd, char *buf, int len)
{
    int ret = 0;
    int profile = -1;
    AUDIOVBCEQ_PARAM_T *param_data_buffer=NULL;
    param_group_t sub_group = NULL;
    char *tmpstr;
    char *key=NULL;
    char data_buf[MAX_SOCKT_LEN];
    int count=0;
    memcpy(data_buf, buf, len);
    *(data_buf + len) = '\0';

    char *line = strtok_r(data_buf, SPLIT, &tmpstr);
    while (line != NULL) {
        param_group_t root;
        profile = select_the_param_profile(audio_param,line);
        LOG_I("set_audio_param count:%d :%s :%d",count, line,profile);

        if(profile >= 0){
            root = audio_param->param[profile].xml.param_root;
            param_data_buffer=&(audio_param->param[profile]);
            key=line+strlen(audio_param->param[profile].xml.first_name)+1;

            if(NULL==root){
                LOG_E("set_audio_param root is null:%s",line);
            }
            if(do_save_param(param_data_buffer, root, key)<0){
                LOG_E("set_audio_param:do_save_param failed");
                ret=-1;
                break;
            }

            if(cmd==SET_PARAM_FROM_RAM){
                audio_param->tunning.param_sync[profile]=true;
            }else{
                audio_param->tunning.param_sync[profile]=false;
            }

            switch(profile) {
            case SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP :
                ret |= ENG_DSP_VBC_OPS;
                break;

            case SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE :
                ret |= ENG_AUDIO_STRECTURE_OPS;
                break;

            case SND_AUDIO_PARAM_NXP_PROFILE :
                ret |= ENG_NXP_OPS;
                break;
            case SND_AUDIO_PARAM_CODEC_PROFILE :
                ret |= ENG_CODEC_OPS;
                break;
            case SND_AUDIO_PARAM_PGA_PROFILE :
                ret |= ENG_PGA_OPS;
                break;
            case SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE :
                ret |= ENG_AUDIO_PROCESS_OPS;
                break;
            default:
                LOG_D("set_audio_param err:%d",__LINE__);
                ret = -1;
                break;
            }
        } else {
            LOG_E("set_audio_param:profile err");
            ret= -1;
            break;
        }
        count++;
        line = strtok_r(NULL, SPLIT, &tmpstr);
    }
    if((profile >= 0) && (ret >= 0)) {
        send_buffer(&(audio_param->tunning),cmd, DATA_STATUS_OK);
        LOG_I("set_audio_param DATA_STATUS_OK ret:%x count:%d",ret,count);
    } else {
        send_buffer(&(audio_param->tunning),cmd, DATA_STATUS_ERROR);
        LOG_E("set_audio_param DATA_STATUS_ERROR ret:%x",ret);
        return ret;
    }
    return ret;
}

static int audiotester_updata_audioparam_time(AUDIO_PARAM_T *audio_param,int opt, char * time_str){
    int ret=0;
    LOG_D("audiotester_updata_audioparam_time opt:%x",opt);
    TiXmlElement *tmpgroup = NULL;

    if(opt&ENG_DSP_VBC_OPS){
        tmpgroup=(TiXmlElement *)audio_param->param[SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP].xml.param_root;
        if(NULL!=tmpgroup){
            tmpgroup->SetAttribute(TIME_ATTR, time_str);
        }else{
            ret=-1;
        }
    }

    if(opt&ENG_AUDIO_STRECTURE_OPS){
        tmpgroup=(TiXmlElement *)audio_param->param[SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE].xml.param_root;
        if(NULL!=tmpgroup){
            tmpgroup->SetAttribute(TIME_ATTR, time_str);
        }else{
            ret=-2;
        }
    }

    if(opt&ENG_NXP_OPS){
        tmpgroup=(TiXmlElement *)audio_param->param[SND_AUDIO_PARAM_NXP_PROFILE].xml.param_root;
        if(NULL!=tmpgroup){
            tmpgroup->SetAttribute(TIME_ATTR, time_str);
        }else{
            ret=-3;
        }
    }

    if(opt&ENG_PGA_OPS){
        tmpgroup=(TiXmlElement *)audio_param->param[SND_AUDIO_PARAM_PGA_PROFILE].xml.param_root;
        if(NULL!=tmpgroup){
            tmpgroup->SetAttribute(TIME_ATTR, time_str);
        }else{
            ret=-4;
        }
    }

    if(opt&ENG_CODEC_OPS){
        tmpgroup=(TiXmlElement *)audio_param->param[SND_AUDIO_PARAM_CODEC_PROFILE].xml.param_root;
        if(NULL!=tmpgroup){
            tmpgroup->SetAttribute(TIME_ATTR, time_str);
        }else{
            ret=-5;
        }
    }

    if(opt&ENG_AUDIO_PROCESS_OPS){
        tmpgroup=(TiXmlElement *)audio_param->param[SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE].xml.param_root;
        if(NULL!=tmpgroup){
            tmpgroup->SetAttribute(TIME_ATTR, time_str);
        }else{
            ret=-6;
        }
    }
    return ret;
}

int audiotester_updata_audioparam(AUDIO_PARAM_T *audio_param,int opt, bool is_ram){
    int ret=0;
    LOG_D("audiotester_updata_audioparam opt:%x",opt);
    struct audio_control *actl=audio_param->dev_ctl;

    if(is_ram==false){
        ret=audiotester_updata_audioparam_time(audio_param,opt,(char *)audio_param->tunning.time_buf);
    }

    if(opt&ENG_DSP_VBC_OPS){
        ret=save_audio_param(audio_param,SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP,is_ram);
        upload_audio_profile_param_firmware(audio_param,SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP);//upload firmware to kernel
    }

    if(opt&ENG_AUDIO_STRECTURE_OPS){
        ret=save_audio_param(audio_param,SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE,is_ram);
        upload_audio_profile_param_firmware(audio_param,SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE);//upload firmware to kernel
    }

    if(opt&ENG_NXP_OPS){
        ret=save_audio_param(audio_param,SND_AUDIO_PARAM_NXP_PROFILE,is_ram);
        upload_audio_profile_param_firmware(audio_param,SND_AUDIO_PARAM_NXP_PROFILE);//upload firmware to kernel
    }

    if(opt&ENG_PGA_OPS){
        ret=save_audio_param(audio_param,SND_AUDIO_PARAM_PGA_PROFILE,is_ram);
        if(is_ram==true){
            ret=reload_sprd_audio_pga_param_withram(audio_param);
        }else{
            ret=reload_sprd_audio_pga_param_withflash(audio_param);
        }
    }

    if(opt&ENG_CODEC_OPS){
        ret=save_audio_param(audio_param,SND_AUDIO_PARAM_CODEC_PROFILE,is_ram);
        if(AUD_REALTEK_CODEC_TYPE == audio_param->dev_ctl->codec_type){
            ret |=upload_audio_profile_param_firmware(audio_param,SND_AUDIO_PARAM_CODEC_PROFILE);
        }
    }

    if(opt&ENG_AUDIO_PROCESS_OPS){
        ret=save_audio_param(audio_param,SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE,is_ram);
    }

    LOG_I("audiotester_updata_audioparam tunning_param_sn:%d backup_param_sn:%d",
        audio_param->tunning_param_sn,audio_param->backup_param_sn)
    if(audio_param->tunning_param_sn<=audio_param->backup_param_sn){
        audio_param->tunning_param_sn=audio_param->backup_param_sn+1;
    }
    audio_param->infor->param_sn=audio_param->tunning_param_sn;
    save_audio_param_infor(audio_param->infor);

    set_audioparam(actl,PARAM_AUDIOTESTER_CHANGE,NULL,false);

    return ret;
}

static int _import_xml_audioparam(AUDIO_PARAM_T *audio_param,char *file_path,char *param_name){
    int ret=0;
    struct xml_handle xml;
    AUDIOVBCEQ_PARAM_T *param=NULL;
    TiXmlElement *src = NULL;
    TiXmlElement *dst = NULL;
    TiXmlElement *clone_src=NULL;
    TiXmlElement *parent_group = NULL;
    int profile=0;

    LOG_I("_import_xml_audioparam:%s %s",file_path,param_name);

    ret = load_xml_handle(&xml, file_path);
    if (ret != 0) {
        LOG_E("_import_xml_audioparam failed (%s)", file_path);
        return ret;
    }

    profile = select_the_param_profile(audio_param,xml.first_name);
    if(profile<0){
        ret=-1;
        LOG_E("_import_xml_audioparam not fined the profile:%s\n",xml.first_name);
        goto out;
    }

    param=&(audio_param->param[profile]);

    src =(TiXmlElement *)XML_open_param_group(xml.param_root,param_name+strlen(xml.first_name)+1);
    dst =(TiXmlElement *)XML_open_param_group(param->xml.param_root,param_name+strlen(xml.first_name)+1);

    if((NULL==src)||(NULL==dst)){
        LOG_E("import_xml_audioparam not fined the param:%s\n",param_name);
        ret=-2;
        goto out;
    }

    clone_src=new TiXmlElement(*src);
    if(NULL==clone_src){
        LOG_E("_import_xml_audioparam clone failed\n");
        ret=-3;
    }

    parent_group = (TiXmlElement *)dst->Parent();
    if(NULL == parent_group) {
        LOG_E("_import_xml_audioparam find parent_group failed\n");
        ret=-4;
        goto out;
    }

    parent_group->ReplaceChild(dst, *clone_src);

out:

    if(ret<0){
        LOG_E("_import_xml_audioparam failed:%d",ret);
        ret=0;
    }else{
        GET_AUDIO_PARAM_CHANGE_FLAGE(audio_param,param_name,ret);
    }

    release_xml_handle(&xml);
    return ret;
}

static int import_xml_audioparam(AUDIO_PARAM_T *audio_param,char *buf,int size){
    int ret=0;
    struct str_parms *parms;
    char file_path[128]={0};
    char param_name[128]={0};
    char *tmpstr;
    int param_flag=0;

    char *line = strtok_r(buf, SPLIT, &tmpstr);
    audio_param_dsp_cmd_t *res=NULL;

    while (line != NULL) {
        parms = str_parms_create_str(line);
        if(NULL==parms){
            LOG_E("import_xml_audioparam str_parms_create_str error");
            return -1;
        }

        memset(file_path,0,sizeof(file_path));
        memset(param_name,0,sizeof(param_name));

        ret = str_parms_get_str(parms, "file", file_path,
                              sizeof(file_path));
        if(ret<0){
            LOG_E("import_xml_audioparam file error");
            goto next;
        }

        ret = str_parms_get_str(parms, "param_name", param_name,
                              sizeof(param_name));

        if(ret<0){
            LOG_E("import_xml_audioparam param_name error");
            goto next;
        }

        param_flag |=_import_xml_audioparam(audio_param,file_path,param_name);

next:
        if(NULL!=parms){
            str_parms_destroy(parms);
        }

        if(ret<0){
            goto out;
        }
        line = strtok_r(NULL, SPLIT, &tmpstr);
    }

out:

    if(param_flag){
        res=(audio_param_dsp_cmd_t *)malloc(sizeof(audio_param_dsp_cmd_t));
        if(res!=NULL){
            res->opt=param_flag;
            res->res=audio_param;
            send_cmd_to_dsp_thread(audio_param->agdsp_ctl,AUDIO_TESTER_UPDATAE_AUDIO_PARAM_TO_FLASH,res);
        }
        ret=0;
    }else{
        ret=-1;
    }
    LOG_I("import_xml_audioparam param_flag:%d ret:%d",param_flag,ret);
    return ret;
}

static int _clone_xml_audioparam(AUDIO_PARAM_T *audio_param,char *src_name,char *dst_name){
    int ret=0;
    AUDIOVBCEQ_PARAM_T *param=NULL;
    TiXmlElement *src = NULL;
    TiXmlElement *dst = NULL;
    TiXmlElement *clone_src=NULL;
    TiXmlElement *parent_group = NULL;
    int profile=0;

    LOG_I("_clone_xml_audioparam src:%s dst:%s",src_name,dst_name);
    profile=select_the_param_profile(audio_param,src_name);
    if(profile<0){
        LOG_D("_clone_xml_audioparam not fined the profile src:%s dst:%s\n",src_name,dst_name);
        ret=-1;
        goto out;

    }

    param=&(audio_param->param[profile]);

    src =(TiXmlElement *)XML_open_param_group(param->xml.param_root,src_name);
    dst =(TiXmlElement *)XML_open_param_group(param->xml.param_root,dst_name);

    if((NULL==src)||(NULL==dst)){
        LOG_D("_clone_xml_audioparam not fined the param src:%s dst:%s\n",src_name,dst_name);
        ret=-2;
        goto out;
    }

    clone_src= new TiXmlElement(*src);
    if(NULL==clone_src){
        LOG_D("_clone_xml_audioparam clone failed\n");
        ret=-3;
    }

    parent_group = (TiXmlElement *)dst->Parent();
    if(NULL == parent_group) {
        LOG_D("_clone_xml_audioparam find parent_group failed\n");
        ret=-4;
        goto out;
    }

    parent_group->ReplaceChild(dst, *clone_src);

out:

    if(ret<0){
        LOG_E("_clone_xml_audioparam failed:%d",ret);
        ret=0;
    }else{
        GET_AUDIO_PARAM_CHANGE_FLAGE(audio_param,dst_name,ret);
    }

    return ret;
}

static int clone_xml_audioparam(AUDIO_PARAM_T *audio_param,char *buf,int size){
    int ret=0;
    struct str_parms *parms;
    char src_name[128]={0};
    char dst_name[128]={0};
    char *tmpstr;
    char *line = strtok_r(buf, SPLIT, &tmpstr);
    audio_param_dsp_cmd_t *res=NULL;
    int param_flag=0;

    while (line != NULL) {
        parms = str_parms_create_str(line);
        if(NULL==parms){
            LOG_E("import_xml_audioparam str_parms_create_str error");
            return -1;
        }

        memset(src_name,0,sizeof(src_name));
        memset(dst_name,0,sizeof(dst_name));

        ret = str_parms_get_str(parms, "src", src_name,
                              sizeof(src_name));
        if(ret<0){
            LOG_E("import_xml_audioparam file error");
            goto next;
        }

        ret = str_parms_get_str(parms, "dst", dst_name,
                              sizeof(dst_name));

        if(ret<0){
            LOG_E("import_xml_audioparam param_name error");
            goto next;
        }

        param_flag |=_clone_xml_audioparam(audio_param,src_name,dst_name);

next:
        if(NULL!=parms){
            str_parms_destroy(parms);
        }

        if(ret<0){
            goto out;
        }
        line = strtok_r(NULL, SPLIT, &tmpstr);
    }

out:

    if(0==ret){
        res=(audio_param_dsp_cmd_t *)malloc(sizeof(audio_param_dsp_cmd_t));
        if(res!=NULL){
            res->opt=param_flag;
            res->res=audio_param;
            send_cmd_to_dsp_thread(audio_param->agdsp_ctl,AUDIO_TESTER_UPDATAE_AUDIO_PARAM_TO_FLASH,res);
        }
    }
    return ret;
}

int handle_audio_cmd_data(AUDIO_PARAM_T *audio_param,
                          uint8_t *buf, int len, int sub_type, int data_state)
{
    int ret = 0;
    struct socket_handle *tunning=(struct socket_handle*)(&audio_param->tunning);
    LOG_I("handle_audio_cmd_data len =%d  sub_type =%d data_state:%d",len,sub_type,data_state);
    audio_param_dsp_cmd_t *res=NULL;

    tunning=&(audio_param->tunning);


    if(NULL!=audio_param->config){
        if(audio_param->config->dump.tunning.rx_debug.txt_fd>0){
            write(audio_param->config->dump.tunning.rx_debug.txt_fd, buf, len);
        }
    }

    switch (sub_type) {
    case GET_INFO: {
        get_audio_param_infor(audio_param,sub_type);
        break;
    }
    case GET_PARAM_FROM_RAM: {
        if (len == 0) {
            ret = send_all_audio_param(audio_param,sub_type);
        } else {
            ret = send_part_audio_param(audio_param, (char *)buf, len,sub_type);
        }
        break;
    }
    case SET_PARAM_FROM_RAM: {
        ret = set_audio_param(audio_param, SET_PARAM_FROM_RAM, (char *)buf,len);

        if((DATA_END==data_state)||(DATA_SINGLE==data_state)){
            res=(audio_param_dsp_cmd_t *)malloc(sizeof(audio_param_dsp_cmd_t));
            if(res!=NULL){
                res->opt=ret;
                res->res=audio_param;
                audio_param->tunning.update_flag |=ret;
                send_cmd_to_dsp_thread(audio_param->agdsp_ctl,AUDIO_TESTER_UPDATAE_AUDIO_PARAM_TO_RAM,res);
                //select_audio_param(audio_param,&(audio_param->dev_ctl->param_res),audio_param->dev_ctl,true);
            }
        }
        break;
    }
    case GET_PARAM_FROM_FLASH: {
        if (len == 0){
            ret = send_all_audio_param(audio_param,sub_type);
        } else{
            ret =send_part_audio_param(audio_param, (char *)buf, len,sub_type);
        }
        break;
    }
    case SET_PARAM_FROM_FLASH:
        {
            ret = set_audio_param(audio_param, SET_PARAM_FROM_RAM, (char *)buf,len);
            if((DATA_END==data_state)||(DATA_SINGLE==data_state)){
            res=(audio_param_dsp_cmd_t *)malloc(sizeof(audio_param_dsp_cmd_t));
                if(res!=NULL){
                    res->opt=audio_param->tunning.update_flag|ret;
                    audio_param->tunning.update_flag=0;
                    res->res=audio_param;
                    send_cmd_to_dsp_thread(audio_param->agdsp_ctl,AUDIO_TESTER_UPDATAE_AUDIO_PARAM_TO_FLASH,res);
                    //select_audio_param(audio_param,&(audio_param->dev_ctl->param_res),audio_param->dev_ctl,true);
                }
            }
        }
        break;
    case GET_REGISTER: {
        LOG_I("GET_REGISTER:%s",buf);
        ret=get_register_value((const char *)buf,len,audio_param->tunning.send_buf,audio_param->tunning.max_len-1024);
        if(ret>0){
            send_buffer(&audio_param->tunning,sub_type, DATA_SINGLE);
        }else{
            send_buffer(&audio_param->tunning,sub_type, DATA_STATUS_ERROR);
        }
        break;
    }
    case SET_REGISTER: {
        LOG_I("SET_REGISTER:%s",buf);
        ret=set_register_value((const char *)buf,len);
        if(ret>0){
            send_buffer(&audio_param->tunning,sub_type, DATA_STATUS_OK);
        }else{
            send_buffer(&audio_param->tunning,sub_type, DATA_STATUS_ERROR);
        }
        break;
    }

    case SET_PARAM_VOLUME:
        set_dsp_volume(audio_param->dev_ctl,(int)strtoul((const char *)buf,NULL,16));
        set_fm_volume(audio_param->dev_ctl,(int)strtoul((const char *)buf,NULL,16));
        send_buffer(&audio_param->tunning,sub_type, DATA_STATUS_OK);
        break;
    case DIS_CONNECT_AUDIOTESTER: {
        ret=disconnect_audiotester_process(&(audio_param->tunning));
        break;
    }
    case IMPORT_XML_AUDIO_PARAM:
        if(0 == len){
            int profile=0;
            for(profile=0;profile<SND_AUDIO_PARAM_PROFILE_MAX;profile++){
                if(audio_param->param[profile].data!=NULL){
                    free(audio_param->param[profile].data);
                    audio_param->param[profile].data=NULL;
                }
                audio_param->param[profile].num_mode=0;
                audio_param->param[profile].param_struct_size=0;
                if(audio_param->param[profile].xml.param_root!=NULL){
                    release_xml_handle(&(audio_param->param[profile].xml));
                }
            }
            ret=init_sprd_audio_param(audio_param,true);
        }else{
            ret=import_xml_audioparam(audio_param,(char *)buf,len);
        }
        if(ret==0){
            send_buffer(&audio_param->tunning,sub_type, DATA_STATUS_OK);
        }else{
            send_buffer(&audio_param->tunning,sub_type, DATA_STATUS_ERROR);
        }
        break;
    case CLONE_XML_AUDIO_PARAM:
        ret=clone_xml_audioparam(audio_param,(char *)buf,len);
        if(ret==0){
            send_buffer(&audio_param->tunning,sub_type, DATA_STATUS_OK);
        }else{
            send_buffer(&audio_param->tunning,sub_type, DATA_STATUS_ERROR);
        }
        break;

    case GET_CURRENT_AUDIO_PARAM:
        ret=get_current_audioparam(audio_param,sub_type);
        break;
    case AUDIO_PCM_DUMP:
        ret=audioteser_pcm_dump(audio_param,sub_type,(char *)buf,len);
        if(ret==0){
            send_buffer(&audio_param->tunning,sub_type, DATA_STATUS_OK);
        }else{
            send_buffer(&audio_param->tunning,sub_type, DATA_STATUS_ERROR);
        }
        break;
    case UPATE_AUDIOPARAM_TIME:
        if(strncmp((const char *)buf,TIME_ATTR,strlen(TIME_ATTR))){
            LOG_W("%s,can not set:%s",__LINE__,buf);
            ret=-1;
        }else{
            if(audio_param->tunning.time_buf!=NULL){
                const char * src_str=(const char *)(buf+strlen(TIME_ATTR)+1);
                int src_str_len=strlen(src_str)>(len-strlen(TIME_ATTR)-1)?(len-strlen(TIME_ATTR)-1):strlen(src_str);

                if(src_str_len>=MAX_AUDIO_PARAM_TIME_STR_LEN-1){
                    src_str_len=MAX_AUDIO_PARAM_TIME_STR_LEN-1;
                }
                memset(audio_param->tunning.time_buf,0,MAX_AUDIO_PARAM_TIME_STR_LEN);
                memcpy((char *)audio_param->tunning.time_buf,src_str,src_str_len);

                ret=0;
            }else{
                ret=-2;
            }
        }

        if(ret==0){
            send_buffer(&audio_param->tunning,sub_type, DATA_STATUS_OK);
        }else{
            send_buffer(&audio_param->tunning,sub_type, DATA_STATUS_ERROR);
        }

        break;
    default:
        send_error(&audio_param->tunning,"the audio tunning command is unknown");
    }
    return ret;
}

#ifdef __cplusplus
}
#endif
