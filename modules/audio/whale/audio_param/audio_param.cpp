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
#include "aud_proc.h"
#include "audio_xml_utils.h"
#include "audio_param.h"
#include "tinyalsa_util.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "tinyxml.h"
#include <tinyalsa/asoundlib.h>
#include "stdint.h"
#include "audio_debug.h"
#define LOG_TAG "audio_hw_param"
#ifdef __cplusplus
extern "C" {
#endif

extern void add_path(char *dst, const char *src);
typedef int (*load_sprd_audio_param) (void *,param_group_t ele, struct param_infor_t  * param_infor,char * path);
extern int parse_device_gain(struct device_usecase_gain *use_gain,
                             struct mixer *mixer, TiXmlElement *device);
static int load_sprd_audio_pga_param(void *use_gain,param_group_t ele, param_infor_t  * param_infor,char *path);

static int _load_sprd_ap_audio_param(TiXmlElement *param_group, char *data,bool root,int *offset,int max_size);
static int load_sprd_ap_audio_param(void *param,param_group_t ele,param_infor_t  * param_infor, char *path);
static void parse_realtek_extend_param(void);
static int upload_realtek_extend_param(struct mixer *mixer);
static int copy_audio_param(const char *src, const char *dst);

static int init_sprd_xml(void * load_param_func_res,
                   struct xml_handle *xml,
                   const char *tunning_param_path,
                   const char *param_path,
                   param_infor_t  * param_infor,
                   load_sprd_audio_param load_param_func,bool update_param);

const struct audio_param_file_t  audio_param_file_table[SND_AUDIO_PARAM_PROFILE_MAX] = {
    {SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP,        DSP_VBC_XML_TUNNING_PATH  , DSP_VBC_BIN_PATH  , DSP_VBC_XML_PATH},
    {SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE,    AUDIO_STRUCTURE_XML_TUNNING_PATH  , AUDIO_STRUCTURE_BIN_PATH  , AUDIO_STRUCTURE_XML_PATH },
    {SND_AUDIO_PARAM_NXP_PROFILE,                NXP_XML_TUNNING_PATH  , NXP_BIN_PATH  , NXP_XML_PATH },
    {SND_AUDIO_PARAM_PGA_PROFILE,                PGA_GAIN_XML_TUNNING_PATH  , NULL  , PGA_GAIN_XML_PATH },
    {SND_AUDIO_PARAM_CODEC_PROFILE,              CODEC_XML_TUNNING_PATH, CODEC_BIN_PATH  , CODEC_XML_PATH},
    {SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE, AUDIO_PROCESS_XML_TUNNING_PATH, NULL,AUDIO_PROCESS_PATH}
};

bool is_file_exist(const char *path)
{
    int fd;
    if (path == NULL) {
        return false;
    }
    fd  = open(path, O_RDONLY);
    if (fd > 0) {
        close(fd);
        return true;
    }
    return false;
}


int load_xml_handle(struct xml_handle *xmlhandle, const char *xmlpath)
{
    LOG_D("load_xml_handle:%s",xmlpath);
    TiXmlElement *root=NULL;
    xmlhandle->param_doc = XML_open_param(xmlpath);
    if (xmlhandle->param_doc == NULL) {
        LOG_E("load xml handle failed (%s)", xmlpath);
        return -1;
    }

    xmlhandle->param_root =  XML_get_root_param(xmlhandle->param_doc);
    root = (TiXmlElement *)xmlhandle->param_root;
    if (root != NULL) {
        root = (TiXmlElement *)xmlhandle->param_root;
        xmlhandle->first_name = strdup(root->Value());
    }
    return 0;
}

void release_xml_handle(struct xml_handle *xmlhandle)
{
    if (xmlhandle->param_doc) {
        XML_release_param(xmlhandle->param_doc);
        xmlhandle->param_doc = NULL;
        xmlhandle->param_root = NULL;
    }
    if (xmlhandle->first_name) {
        free(xmlhandle->first_name);
        xmlhandle->first_name = NULL;
    }
}

const char * tinymix_get_enum(struct mixer_ctl *ctl)
{
    unsigned int num_enums;
    unsigned int i;
    const char *string;

    num_enums = mixer_ctl_get_num_enums(ctl);

    for (i = 0; i < num_enums; i++) {
        string = mixer_ctl_get_enum_string(ctl, i);
        if (mixer_ctl_get_value(ctl, 0) == (int)i){
            return string;
        }
    }

    return NULL;
}

static const struct audio_param_mode_t  audio_param_mode_table[] = {
    { PROFILE_MODE_AUDIO_Handset_GSM,                "Audio\\Handset\\GSM"          },
    { PROFILE_MODE_AUDIO_Handset_TDMA,               "Audio\\Handset\\TDMA"         },
    { PROFILE_MODE_AUDIO_Handset_WCDMA_NB,           "Audio\\Handset\\WCDMA_NB"     },
    { PROFILE_MODE_AUDIO_Handset_WCDMA_WB,           "Audio\\Handset\\WCDMA_WB"     },
    { PROFILE_MODE_AUDIO_Handset_VOLTE_NB,           "Audio\\Handset\\VOLTE_NB"     },
    { PROFILE_MODE_AUDIO_Handset_VOLTE_WB,           "Audio\\Handset\\VOLTE_WB"     },
    { PROFILE_MODE_AUDIO_Handset_VOIP,               "Audio\\Handset\\VOIP"         },

    { PROFILE_MODE_AUDIO_Handsfree_GSM,              "Audio\\Handsfree\\GSM"        },
    { PROFILE_MODE_AUDIO_Handsfree_TDMA,             "Audio\\Handsfree\\TDMA"       },
    { PROFILE_MODE_AUDIO_Handsfree_WCDMA_NB,         "Audio\\Handsfree\\WCDMA_NB"   },
    { PROFILE_MODE_AUDIO_Handsfree_WCDMA_WB,         "Audio\\Handsfree\\WCDMA_WB"   },
    { PROFILE_MODE_AUDIO_Handsfree_VOLTE_NB,         "Audio\\Handsfree\\VOLTE_NB"   },
    { PROFILE_MODE_AUDIO_Handsfree_VOLTE_WB,         "Audio\\Handsfree\\VOLTE_WB"   },
    { PROFILE_MODE_AUDIO_Handsfree_VOIP,             "Audio\\Handsfree\\VOIP"       },

    { PROFILE_MODE_AUDIO_Headset4P_GSM,              "Audio\\Headset4P\\GSM"        },
    { PROFILE_MODE_AUDIO_Headset4P_TDMA,             "Audio\\Headset4P\\TDMA"       },
    { PROFILE_MODE_AUDIO_Headset4P_WCDMA_NB,         "Audio\\Headset4P\\WCDMA_NB"   },
    { PROFILE_MODE_AUDIO_Headset4P_WCDMA_WB,         "Audio\\Headset4P\\WCDMA_WB"   },
    { PROFILE_MODE_AUDIO_Headset4P_VOLTE_NB,         "Audio\\Headset4P\\VOLTE_NB"   },
    { PROFILE_MODE_AUDIO_Headset4P_VOLTE_WB,         "Audio\\Headset4P\\VOLTE_WB"   },
    { PROFILE_MODE_AUDIO_Headset4P_VOIP,             "Audio\\Headset4P\\VOIP"       },

    { PROFILE_MODE_AUDIO_Headset3P_GSM,              "Audio\\Headset3P\\GSM"        },
    { PROFILE_MODE_AUDIO_Headset3P_TDMA,             "Audio\\Headset3P\\TDMA"       },
    { PROFILE_MODE_AUDIO_Headset3P_WCDMA_NB,         "Audio\\Headset3P\\WCDMA_NB"   },
    { PROFILE_MODE_AUDIO_Headset3P_WCDMA_WB,         "Audio\\Headset3P\\WCDMA_WB"   },
    { PROFILE_MODE_AUDIO_Headset3P_VOLTE_NB,         "Audio\\Headset3P\\VOLTE_NB"   },
    { PROFILE_MODE_AUDIO_Headset3P_VOLTE_WB,         "Audio\\Headset3P\\VOLTE_WB"   },
    { PROFILE_MODE_AUDIO_Headset3P_VOIP,             "Audio\\Headset3P\\VOIP"       },

    { PROFILE_MODE_AUDIO_BTHS_GSM,                   "Audio\\BTHS\\GSM"             },
    { PROFILE_MODE_AUDIO_BTHS_TDMA,                  "Audio\\BTHS\\TDMA"            },
    { PROFILE_MODE_AUDIO_BTHS_WCDMA_NB,              "Audio\\BTHS\\WCDMA_NB"        },
    { PROFILE_MODE_AUDIO_BTHS_WCDMA_WB,              "Audio\\BTHS\\WCDMA_WB"        },
    { PROFILE_MODE_AUDIO_BTHS_VOLTE_NB,              "Audio\\BTHS\\VOLTE_NB"        },
    { PROFILE_MODE_AUDIO_BTHS_VOLTE_WB,              "Audio\\BTHS\\VOLTE_WB"        },
    { PROFILE_MODE_AUDIO_BTHS_VOIP,                  "Audio\\BTHS\\VOIP"            },

    { PROFILE_MODE_AUDIO_BTHSNREC_GSM,               "Audio\\BTHSNREC\\GSM"         },
    { PROFILE_MODE_AUDIO_BTHSNREC_TDMA,              "Audio\\BTHSNREC\\TDMA"        },
    { PROFILE_MODE_AUDIO_BTHSNREC_WCDMA_NB,          "Audio\\BTHSNREC\\WCDMA_NB"    },
    { PROFILE_MODE_AUDIO_BTHSNREC_WCDMA_WB,          "Audio\\BTHSNREC\\WCDMA_WB"    },
    { PROFILE_MODE_AUDIO_BTHSNREC_VOLTE_NB,          "Audio\\BTHSNREC\\VOLTE_NB"    },
    { PROFILE_MODE_AUDIO_BTHSNREC_VOLTE_WB,          "Audio\\BTHSNREC\\VOLTE_WB"    },
    { PROFILE_MODE_AUDIO_BTHSNREC_VOIP,              "Audio\\BTHSNREC\\VOIP"        },

    { PROFILE_MODE_MUSIC_Headset_Playback,           "Music\\Headset\\Playback"     },
    { PROFILE_MODE_MUSIC_Headset_Record,             "Music\\Headset\\Record"       },
    { PROFILE_MODE_MUSIC_Headset_FM,                 "Music\\Headset\\FM"           },

    { PROFILE_MODE_MUSIC_Handsfree_Playback,         "Music\\Handsfree\\Playback"   },
    { PROFILE_MODE_MUSIC_Handsfree_Record,           "Music\\Handsfree\\Record"     },
    { PROFILE_MODE_MUSIC_Handsfree_FM,               "Music\\Handsfree\\FM"         },

    { PROFILE_MODE_MUSIC_Headfree_Playback,          "Music\\Headfree\\Playback"    },

    { PROFILE_MODE_MUSIC_Handset_Playback,           "Music\\Handset\\Playback"     },

    { PROFILE_MODE_MUSIC_Bluetooth_Record,           "Music\\Bluetooth\\Record"     },

    { PROFILE_MODE_LOOP_Handset_MainMic,             "Loopback\\Handset\\MainMic"       },
    { PROFILE_MODE_LOOP_Handsfree_MainMic,           "Loopback\\Handsfree\\MainMic"     },
    { PROFILE_MODE_LOOP_Handsfree_AuxMic,            "Loopback\\Handsfree\\AuxMic"      },
    { PROFILE_MODE_LOOP_Headset4P_HeadMic,           "Loopback\\Headset4P\\HeadMic"     },
    { PROFILE_MODE_LOOP_Headset3P_MainMic,           "Loopback\\Headset3P\\MainMic"     },
};

uint8_t get_audio_param_id(const char *name){
    for(uint8_t i=0;i<PROFILE_MODE_MAX;i++){
        if(strncmp(audio_param_mode_table[i].name,name,strlen(audio_param_mode_table[i].name)-1)==0){
            LOG_D("get_audio_param_id :%s len:%d return:%d",name,strlen(name),i);
            return (uint8_t)audio_param_mode_table[i].mode;
        }
    }
    LOG_E("get_audio_param_id err:%s len:%d",name,strlen(name));
    return 0xff;
}

const char * get_audio_param_name(uint8_t param_id){
    for(uint8_t i=0;i<PROFILE_MODE_MAX;i++){
        if(param_id==audio_param_mode_table[i].mode){
            return audio_param_mode_table[i].name;
        }
    }
    return NULL;
}

void dump_data(char *buf, int len)
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

        sprintf(dump_buf + dump_buf_len, "%04x: ", i*16);
        dump_buf_len = 5;

        for(j = 0; j < size; j++) {
            sprintf(dump_buf + dump_buf_len, " %02x", tmp[j]);
            dump_buf_len += 3;
        }

        LOG_I("%s\n", dump_buf);

    }
}

int get_audio_param_mode_name(char *str){
    int i=0;
    char mode_name[10240]={0};
    for(i=PROFILE_MODE_AUDIO_Handset_GSM;i<PROFILE_MODE_MAX;i++){
        sprintf(mode_name,"// ModeName:%04d=%s",i,audio_param_mode_table[i].name);
        strcat(str, mode_name);
        strcat(str, SPLIT);
    }
    return 0;
}

int get_audio_param_id_frome_name(const char *name)
{
    int i = 0;
    LOG_D("get_audio_param_id_frome_name:%s\n", name);
    int max_mode = sizeof(audio_param_mode_table) / sizeof(struct
                   audio_param_mode_t);
    for(i = 0; i < max_mode; i++) {
        if(strcmp(name, audio_param_mode_table[i].name) == 0) {
            return audio_param_mode_table[i].mode;
        }
    }
    return -1;
}

int save_audio_param_infor(struct param_infor *infor){
    int fd=-1;

    fd= open(AUDIO_PARAM_INFOR_TUNNING_PATH, O_RDWR | O_CREAT,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if(fd<0){
        LOG_E("save_audio_param_infor open failed");
        return -1;
    }

    write(fd, infor, sizeof(struct param_infor));

    close(fd);

    return 0;
}

static int load_audio_param_infor(struct param_infor *infor){
    int fd=-1;
    int size=0;

    if(access(AUDIO_PARAM_INFOR_PATH, R_OK) == 0){
        fd= open(AUDIO_PARAM_INFOR_PATH, O_RDONLY);

        if(fd<0){
            LOG_E("load_audio_param_infor open failed:%s",AUDIO_PARAM_INFOR_PATH);
            goto read_tunning;
        }

        size=read(fd, infor, sizeof(struct param_infor));
        LOG_I("load_audio_param_infor etc size:0x%x 0x%x",size,sizeof(struct param_infor));
        close(fd);

        if(size!=sizeof(struct param_infor)){
            fd=-1;
            size=0;
            goto read_tunning;
        }
        return 0;
    }

read_tunning:

    if(access(AUDIO_PARAM_INFOR_TUNNING_PATH, R_OK) == 0){
        fd= open(AUDIO_PARAM_INFOR_TUNNING_PATH, O_RDONLY);

        if(fd<0){
            LOG_E("load_audio_param_infor open failed:%s",AUDIO_PARAM_INFOR_TUNNING_PATH);
            return -1;
        }

        size=read(fd, infor, sizeof(struct param_infor));
        LOG_I("load_audio_param_infor tunning size:0x%x 0x%x",size,sizeof(struct param_infor));
        close(fd);

        if(size!=sizeof(struct param_infor)){
            LOG_E("load_audio_param_infor open failed:%s size:%d",AUDIO_PARAM_INFOR_TUNNING_PATH,size);
            return -1;
        }
    }
    return 0;
}

int save_audio_param_to_bin(AUDIO_PARAM_T *param, int profile)
{
    int ret = 0;
    int i = 0;
    struct vbc_fw_header *fw_header;
    AUDIOVBCEQ_PARAM_T *audio_param;
    int size = 0;
    audio_param = &(param->param[profile]);
    fw_header = &(param->header[profile]);

    if((NULL ==audio_param_file_table[profile].bin_file)
       ||(NULL==param->select_mixer[profile])
       ||(NULL==param->update_mixer[profile])){
        LOG_D("save_audio_param_to_bin:No need to save");
        return 0;
    }

    if(param->fd_bin[profile]<=0){
        LOG_D("save_audio_param_to_bin open:%s",audio_param_file_table[profile].bin_file)
        param->fd_bin[profile] = open(audio_param_file_table[profile].bin_file, O_RDWR | O_CREAT,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        strncpy(fw_header->magic, audio_param->xml.first_name, FIRMWARE_MAGIC_MAX_LEN);
    }

    if(param->fd_bin[profile] < 0) {
        LOG_E("open:%s failed\n",audio_param_file_table[profile].bin_file);
        return -1;
    }

    strncpy(fw_header->magic, AUDIO_PARAM_FIRMWARE_NAME, FIRMWARE_MAGIC_MAX_LEN);
    fw_header->num_mode = audio_param->num_mode;
    fw_header->len = audio_param->param_struct_size;

    ret += write(param->fd_bin[profile], fw_header, sizeof(struct vbc_fw_header));

    size = audio_param->param_struct_size * audio_param->num_mode;
    ret += write(param->fd_bin[profile], audio_param->data, size);
    LOG_I("save_audio_param_to_bin[%d] point:%p size:0x%x ret:0x%x :0x%x 0x%x\n",profile,audio_param->data,
          size, ret, audio_param->param_struct_size, audio_param->num_mode);

    close(param->fd_bin[profile]);
    param->fd_bin[profile]=-1;
    return 0;
}

int _get_param_size_from_xml(TiXmlElement *param_group)
{
    const char *type;
    int size = 0;

    TiXmlElement *group = param_group;
    while (group != NULL) {
        //     LOG_D("_get_param_size_from_xml %s \n",group->Value());
        type = group->Attribute(TYPE);
        if (type == NULL) {
            size += _get_param_size_from_xml(group->FirstChildElement());
            LOG_D("_get_param_size_from_xml %s size:0x%x\n", group->Value(), size);
        } else {
            if(strcmp(type, U32) == 0) {
                size += sizeof(int32_t);
            } else if(strcmp(type, U16) == 0) {
                size += sizeof(int16_t);
            } else if(strcmp(type, U8) == 0) {
                size += sizeof(uint8_t);
            }
        }
        group = group->NextSiblingElement();
    }

    //  LOG_D("_get_param_size_from_xml return:%d\n",size);
    return size;
}

int get_param_size_from_xml(TiXmlElement *Element)
{
    TiXmlElement *tmpElement = NULL;
    TiXmlElement *tmpElement01;
    TiXmlElement *tmpElement02;
    TiXmlElement *tmpElement03;
    const char *type = NULL;
    int mode_find = 0;
    int size = 0;


    tmpElement = (TiXmlElement *)Element;
    tmpElement01 = (TiXmlElement *)XML_get_first_sub_group(Element);
    if(tmpElement01 != NULL) {
        tmpElement02 = (TiXmlElement *)XML_get_first_sub_group(tmpElement01);
        if(tmpElement02 != NULL) {
            tmpElement03 = (TiXmlElement *)XML_get_first_sub_group(tmpElement02);
            if(NULL!=tmpElement03){
                LOG_D("get_param_size_from_xml:%s\n", tmpElement03->Value());
                size = _get_param_size_from_xml((TiXmlElement *)XML_get_first_sub_group(
                                                    tmpElement03));
              }
        }
    }
    LOG_D("get_param_size_from_xml size:0x%x\n", size);
    return size;
}

int get_param_count_from_xml(TiXmlElement *Element)
{
    TiXmlElement *tmpElement01;
    TiXmlElement *tmpElement02;
    TiXmlElement *tmpElement03;
    int size = 0;

    tmpElement01 = (TiXmlElement *)XML_get_first_sub_group(Element);
    while(tmpElement01 != NULL) {
        tmpElement02 = (TiXmlElement *)XML_get_first_sub_group(tmpElement01);
        while(tmpElement02 != NULL) {
            tmpElement03 = (TiXmlElement *)XML_get_first_sub_group(tmpElement02);
            while(tmpElement03 != NULL){
                tmpElement03->SetAttribute("mode", size);
                size++;
                tmpElement03 = (TiXmlElement *)XML_get_next_sibling_group(tmpElement03);
            }
            tmpElement02 = (TiXmlElement *)XML_get_next_sibling_group(tmpElement02);
        }
        tmpElement01 = (TiXmlElement *)XML_get_next_sibling_group(tmpElement01);
    }
    LOG_D("get_param_count_from_xml size:0x%x\n", size);
    return size;
}

int read_audio_param_for_element(char *data,param_group_t Element){
    TiXmlElement *group = (TiXmlElement *)Element;
    TiXmlElement *child=NULL;
    const char *tmp=NULL;
    const char *type=NULL;
    bool has_child=false;
    int addr_offset=0;

    int size=0;
    int values=0;

    int child_val=0;
    int bits=0;
    int offset=0;

    int i=0;
    int mask=0;

    if((data==NULL) || (NULL==group)){
        LOG_E("%s %d",__func__,__LINE__);
        return -1;
    }

    type =group->Attribute(TYPE);
    if(NULL==type){
        group = (TiXmlElement *)group->Parent();
        if(NULL == group){
            LOG_E("%s %d",__func__,__LINE__);
            return -2;
        }

        type =group->Attribute(TYPE);

        has_child=true;
    }

    if(NULL==type){
            LOG_E("%s %d",__func__,__LINE__);
            return -3;
    }

    if(strcmp(type, U32) == 0) {
        size = sizeof(int32_t);
    } else if(strcmp(type, U16) == 0) {
        size = sizeof(int16_t);
    } else if(strcmp(type, U8) == 0) {
        size = sizeof(uint8_t);
    }

    tmp=group->Attribute(ID);
    if(NULL==tmp){
            LOG_E("%s %d",__func__,__LINE__);
            return -4;
    }
    addr_offset=string_to_value(tmp);

    if(false==has_child){
        tmp=group->Attribute(BITS);
        if(NULL==tmp){
                LOG_E("%s %d %s",__func__,__LINE__,group->Value());
                return -6;
        }

        bits=string_to_value(tmp);

        tmp=group->Attribute(OFFSETS);
        if(NULL==tmp){
                LOG_E("%s %d %s",__func__,__LINE__,group->Value());
                return -7;
        }

        offset=string_to_value(tmp);

        tmp=group->Attribute(VALUE);
        if(NULL==tmp){
                LOG_E("%s %d %s",__func__,__LINE__,group->Value());
                return -7;
        }

        values=string_to_value(tmp);

        if(bits<32) {
            mask=0;
            for(i=0;i<bits;i++) {
                mask |=1<<i;
            }
            values &=mask;
        }

        LOG_I("read_audio_param_for_xml %s offset:0x%x mask:%x values:0x%x",
            group->Value(),offset,mask,values);

        values=strtoul(tmp,NULL,16);
    }else{
        child=group->FirstChildElement();
        while(NULL!=child){
            tmp=child->Attribute(BITS);
            if(NULL==tmp){
                    LOG_E("%s %d %s",__func__,__LINE__,child->Value());
                    return -6;
            }

            bits=string_to_value(tmp);

            tmp=child->Attribute(OFFSETS);
            if(NULL==tmp){
                    LOG_E("%s %d %s",__func__,__LINE__,child->Value());
                    return -7;
            }

            offset=string_to_value(tmp);

            tmp=child->Attribute(VALUE);
            if(NULL==tmp){
                    LOG_E("%s %d %s",__func__,__LINE__,child->Value());
                    return -7;
            }

            child_val=string_to_value(tmp);

            mask=0;
            for(i=0;i<bits;i++){
                mask |=1<<i;
            }

            child_val &=mask;

            values |= (child_val<<offset);

            LOG_I("read_audio_param_for_xml child:%s child_val:0x%x offset:0x%x mask:%x values:0x%x",
                child->Value(),child_val,offset,mask,values);

            child=child->NextSiblingElement();
        }
    }

    if(sizeof(int16_t) == size){
        values &=0xffff;
    }else if(sizeof(uint8_t) == size){
        values &=0xff;
    }

    memcpy(data+addr_offset,&values,size);

    return 0;

}

int get_ele_value(param_group_t Element){
    TiXmlElement *ele;
    TiXmlElement *tmpele = (TiXmlElement *)Element;
    const char *tmp = NULL;
    int values = 0;
    int bits = 0;
    int offset = 0;
    int val = 0;

    ele = (TiXmlElement *)XML_get_first_sub_group(tmpele);
    if(NULL == ele) {
        tmp = tmpele->Attribute(VALUE);
        if(NULL == tmp) {
            LOG_E("%s Not find :%s", tmpele->Value(), VALUE);
            return -1;
        }

        values = string_to_value(tmp);
        LOG_D("name:%s values:0x%x\n",
              tmpele->Value(), values);
    } else {
        LOG_D("get_ele_value 1 %s\n", tmpele->Value());
        while(ele != NULL) {

            tmp = ele->Attribute(BITS);
            if(tmp == NULL) {
                LOG_D("%s name:%s not find:%s\n", __func__, ele->Value(), BITS);
                return -2;
            }
            bits = string_to_value(tmp);

            if((bits < 0) || (bits > 32)) {
                LOG_D("%s name:%s :%s err:%d\n", __func__, ele->Value(), BITS, bits);
                return -3;
            }

            tmp = ele->Attribute(OFFSETS);
            if(tmp == NULL) {
                LOG_D("%s name:%s not find:%s\n", __func__, ele->Value(), BITS);
                return -2;
            }
            offset = string_to_value(tmp);
            if((offset < 0) || (offset > 32)) {
                LOG_D("%s name:%s :%s err:%d\n", __func__, ele->Value(), OFFSETS, bits);
                return -3;
            }

            tmp = ele->Attribute(VALUE);
            if(tmp == NULL) {
                LOG_D("%s name:%s not find:%s\n", __func__, ele->Value(), BITS);
                return -2;
            }
            val = string_to_value(tmp);

            if(bits<32) {
                val = val & ((1 << bits) - 1);
            }else {
                if(offset!=0) {
                    LOG_E("offset error:%d bits:%d ele:%d",offset,bits,ele->Value());
                    offset=0;
                }
            }
            values |= (val << offset);
            LOG_D("name:%s bits:%d offset:%d val:0x%x values:0x%x\n",
                  ele->Value(), bits, offset, val, values);
            ele = (TiXmlElement *) XML_get_next_sibling_group(ele);
        }
    }
    return values;
}


int _init_sprd_audio_param_from_xml(param_group_t param,bool root,
                                  unsigned char *data,int *offset,int depth,
                                  struct param_infor_t  * param_infor,
                                  char *prev_path)
{
    int ret=0;
    int size=0;
    char val_str[32]={0};

    char cur_path[MAX_LINE_LEN] = {0};
    if (prev_path != NULL) {
        strcat(cur_path, prev_path);
    }

    const char * tmp=NULL;
    
    TiXmlElement *group = (TiXmlElement *)param;
    while(group!=NULL){
        LOG_D("ele:%d: %s total_size:0x%x",depth,group->Value(),*offset);
        tmp = group->Attribute(TYPE);
        if(tmp==NULL){
            char next_path[MAX_LINE_LEN] = {0};
            char *path=NULL;

            if(param_infor!=NULL){
                if((depth>=1)&&(depth<3)){
                    strcat(next_path, cur_path);
                    add_path(next_path, group->Value());
                    path=next_path;
                }else if (depth==3){
                    int id=0xff;
                    strcat(next_path, cur_path);
                    strcat(next_path, group->Value());
                    id=get_audio_param_id_frome_name(next_path);
                    param_infor->offset[id]=*offset;
                    LOG_D("next_path:%s name:%s depth:%d id:%d offset:0x%08x",next_path,group->Value(),depth,id,param_infor->offset[id]);
                    path=NULL;
                }else{
                    path=NULL;
                }
            }else{
                path=NULL;
            }

            if((data!=NULL) && (offset!=NULL)){
                 _init_sprd_audio_param_from_xml(group->FirstChildElement(),false,data,offset,depth+1,param_infor,path);
            }else{
                 _init_sprd_audio_param_from_xml(group->FirstChildElement(),false,NULL,NULL,depth+1,param_infor,path);
            }
        }else{
            if(strcmp(tmp, U32) == 0){
                size = sizeof(int32_t);
            } else if(strcmp(tmp, U16) == 0){
                size = sizeof(int16_t);
            } else if(strcmp(tmp, U8) == 0){
                size = sizeof(uint8_t);
            };

            sprintf(val_str,"0x%x",*offset);
            group->SetAttribute(ID, val_str);
            ret=get_ele_value(group);
            if(NULL!=data){
                memcpy(data+(*offset),&ret,size);
                *offset+=size;
            }
            LOG_D("%s id:0x%x val:0x%x",group->Value(),*offset,ret);
        }

        if(root){
            break;
        }

        if(depth==2){
           LOG_D("ele NextSiblingElement1:%d: %s total_size:0x%x",depth,group->Value(),*offset); 
        }
        group = group->NextSiblingElement();
        if((depth==2) &&(group!=NULL)) {
           LOG_D("ele NextSiblingElement2:%d: %s total_size:0x%x",depth,group->Value(),*offset); 
        }
    }
    return *offset;
}

static int open_audio_param_file(AUDIO_PARAM_T *param,int profile){
    int ret=0;
    bool tunning=false;
    AUDIOVBCEQ_PARAM_T *audio_param = &(param->param[profile]);

    if(NULL!=audio_param->xml.param_root){
        return ret;
    }

    if((false==param->audio_param_update)&&(access(audio_param_file_table[profile].tunning_file, R_OK) == 0) ){
        tunning=true;
    } else {
        if(access(audio_param_file_table[profile].src_file, R_OK)!= 0){
            LOG_E("open_audio_param_file %s not exit",audio_param_file_table[profile].src_file);
            return -1;
        }
        tunning=false;
    }

    if(true==tunning){
        LOG_I("open_audio_param_file:%s",audio_param_file_table[profile].tunning_file);
        load_xml_handle(&(audio_param->xml), audio_param_file_table[profile].tunning_file);
    }else{
        LOG_I("open_audio_param_file:%s",audio_param_file_table[profile].src_file);
        load_xml_handle(&(audio_param->xml), audio_param_file_table[profile].src_file);
    }

    if(NULL==audio_param->xml.param_root) {
        LOG_E("open_audio_param_file can not get root\n");
        if(true==tunning){
            char cmd[256]={0};
            snprintf(cmd,sizeof(cmd),"rm -rf %s",audio_param_file_table[profile].tunning_file);
            system(cmd);
            LOG_E("open_audio_param_file system:%s",cmd);
            snprintf(cmd,sizeof(cmd),"rm -rf %s",audio_param_file_table[profile].bin_file);
            system(cmd);
            LOG_E("open_audio_param_file system:%s",cmd);
            load_xml_handle(&(audio_param->xml), audio_param_file_table[profile].src_file);
            if(NULL==audio_param->xml.param_root) {
                LOG_E("open_audio_param_file src file can not get root:%s\n");
                ret=-1;
            }
        }else{
            ret=-1;
        }
    }
    return ret;
}

static int malloc_sprd_audio_param(AUDIO_PARAM_T *param,int profile){
    int ret=0;
    const char *parampath;
    AUDIOVBCEQ_PARAM_T *audio_param = &(param->param[profile]);
    param_group_t root_param;
    TiXmlElement *tmpElement;
    const char *tmp = NULL;
    char val_str[32]={0};


    ret=open_audio_param_file(param,profile);
    if(ret!=0){
        LOG_E("open_audio_param_file failed");
        return ret;
    }
    root_param=audio_param->xml.param_root;
    if(NULL==root_param){
        audio_param->data=NULL;
        LOG_E("malloc_sprd_audio_param param_root is null");
        return -1;
    }

    LOG_D("%s %d %p",__func__,__LINE__,root_param);
    tmpElement = (TiXmlElement *)root_param;
    tmp = tmpElement->Attribute(STRUCT_SIZE);
    if(tmp != NULL) {
        audio_param->param_struct_size = string_to_value(tmp);
    }

    tmp = tmpElement->Attribute(NUM_MODE);
    if(tmp != NULL) {
        audio_param->num_mode = string_to_value(tmp);
    }

    LOG_D("malloc_sprd_audio_param<%s> mode:%x size:0x%x\n",
          tmpElement->Value(), audio_param->num_mode, audio_param->param_struct_size);

    if((audio_param->num_mode <= 0) || (audio_param->param_struct_size <= 0)) {
        audio_param->num_mode = 0;
        audio_param->param_struct_size = 0;
        tmpElement = (TiXmlElement *)root_param;
        LOG_D("malloc_sprd_audio_param name:%s\n", tmpElement->Value());
        audio_param->param_struct_size = get_param_size_from_xml(tmpElement);
        audio_param->num_mode = get_param_count_from_xml(tmpElement);
    }

    ret=audio_param->num_mode *audio_param->param_struct_size;

    if(audio_param->data == NULL){
        audio_param->data = (char *)malloc(ret);
        LOG_D("malloc_sprd_audio_param profile %x malloc :%x %p", profile, audio_param->num_mode *
              audio_param->param_struct_size, audio_param->data);
        if(audio_param->data == NULL) {
            LOG_E("malloc_sprd_audio_param malloc audio_param err\n");
            ret = 0;
            goto err;
        }
    }

    tmpElement = (TiXmlElement *)root_param;
    sprintf(val_str,"0x%x",audio_param->num_mode);
    tmpElement->SetAttribute(NUM_MODE, val_str);
    memset(val_str,0,sizeof(val_str));
    sprintf(val_str,"0x%x",audio_param->param_struct_size);
    tmpElement->SetAttribute(STRUCT_SIZE, val_str);
    return ret;
err:
    LOG_E("malloc_sprd_audio_param err");
    if(NULL != audio_param->data){
        free(audio_param->data);
        audio_param->data=NULL;
    }
    return ret;
}

int init_sprd_audio_param_from_xml(AUDIO_PARAM_T *param,int profile)
{
    AUDIOVBCEQ_PARAM_T *audio_param = &(param->param[profile]);
    param_group_t root_param;
    TiXmlDocument *doc;
    const char *tmp = NULL;
    int ret = 0;
    const char *parampath;
    int offset=0;
    int mode=0;
    const char *str=NULL;
    struct param_infor_t *infor=NULL;
    if(param->infor==NULL){
        infor=NULL;
    }else{
        infor=&(param->infor->data[profile]);
    }

    LOG_D("%s %d",__func__,__LINE__);
    ret=malloc_sprd_audio_param(param,profile);
    if(ret>0){

        if(infor!=NULL){
            infor->param_struct_size=audio_param->param_struct_size;
        }

        LOG_I("init_sprd_audio_param_from_xml:%d",profile);

        ret=_init_sprd_audio_param_from_xml(audio_param->xml.param_root,true,(unsigned char*)
                audio_param->data,&offset,0,
                infor,
                NULL);
        doc = (TiXmlDocument *)audio_param->xml.param_doc;

        if((true==param->audio_param_update)||(access(audio_param_file_table[profile].tunning_file, R_OK) != 0)){
            LOG_I("init_sprd_audio_param_from_xml save:%s",audio_param_file_table[profile].tunning_file);
            doc->SaveFile(audio_param_file_table[profile].tunning_file);
        }

        save_audio_param_to_bin(param, profile);
        return ret;
    }

err:
    LOG_E("init_sprd_audio_param_from_xml err");
    if(param->fd_bin[profile]>0){
        close(param->fd_bin[profile]);
    }
    param->fd_bin[profile]=-1;

    if(NULL != audio_param->data){
        free(audio_param->data);
        audio_param->data=NULL;
    }
    return ret;
}

int reload_sprd_audio_process_param_withflash(AUDIO_PARAM_T *audio_param){
    LOG_I("reload_sprd_audio_process_param_withflash");
    init_sprd_xml(&audio_param->param[SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE],&(audio_param->param[SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE].xml),
        AUDIO_PROCESS_XML_TUNNING_PATH,AUDIO_PROCESS_PATH,
      &(audio_param->infor->data[SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE]),
        load_sprd_ap_audio_param,false);
    return 0;
}

int reload_sprd_audio_pga_param_withflash(AUDIO_PARAM_T *param){
    int profile=SND_AUDIO_PARAM_PGA_PROFILE;

    param->param[SND_AUDIO_PARAM_PGA_PROFILE].data=NULL;
    LOG_I("reload_sprd_audio_pga_param_withflash");
    free_sprd_audio_pga_param(&param->dev_ctl->dg_gain);
    init_sprd_xml(&param->dev_ctl->dg_gain,&(param->param[profile].xml),audio_param_file_table[profile].tunning_file,
        audio_param_file_table[profile].src_file,NULL,load_sprd_audio_pga_param,false);
    return 0;
}

int reload_sprd_audio_pga_param_withram(AUDIO_PARAM_T *param){
    struct xml_handle *xmlhandle=&(param->param[SND_AUDIO_PARAM_PGA_PROFILE].xml);
    TiXmlElement * ele_1=NULL;
    TiXmlElement * ele_2=NULL;
    TiXmlElement * ele_3=NULL;
    TiXmlElement * group=NULL;
    TiXmlElement * root=NULL;
    char *ele_1_name=NULL;
    char *ele_2_name=NULL;
    char *ele_3_name=NULL;
    char path[MAX_LINE_LEN] = {0};

    LOG_I("reload_sprd_audio_pga_param_withram");
    if(NULL==xmlhandle->param_root){
        LOG_E("reload_sprd_audio_pga_param_withram param_root is null");
        return -1;
    }
    free_sprd_audio_pga_param(&param->dev_ctl->dg_gain);

    root=(TiXmlElement * )xmlhandle->param_root;

    ele_1 = root->FirstChildElement();
    while(ele_1!=NULL){
        ele_1_name=strdup(ele_1->Value());
        ele_2 = ele_1->FirstChildElement();
        while(ele_2!=NULL){
            ele_2_name=strdup(ele_2->Value());
            ele_3 = ele_2->FirstChildElement();
            while(NULL!=ele_3){
                ele_3_name=strdup(ele_3->Value());

                strcat(path, ele_1_name);
                strcat(path, BACKSLASH);
                strcat(path, ele_2_name);
                strcat(path, BACKSLASH);
                strcat(path, ele_3_name);
                group=ele_3;
                load_sprd_audio_pga_param(&param->dev_ctl->dg_gain,group,NULL,path);
                memset(path,0,sizeof(path));

                free(ele_3_name);
                ele_3_name=NULL;
                ele_3 = ele_3->NextSiblingElement();
            }
            ele_2 = ele_2->NextSiblingElement();

            free(ele_2_name);
            ele_2_name=NULL;
        }
        ele_1 = ele_1->NextSiblingElement();

        free(ele_1_name);
        ele_1_name=NULL;
    }
    return 0;
}


bool check_updata_audioparam(AUDIO_PARAM_T  *audio_param){
    struct param_infor *base=NULL;
    struct param_infor *data_infor=NULL;
    int ret=0;
    int base_fd=-1;
    int data_infor_fd=-1;
    int read_size=0;

    bool base_exist=false;
    bool tunning_exist=false;

    bool return_value=false;

    audio_param->backup_param_sn=0;
    audio_param->tunning_param_sn=0;
    audio_param->audio_param_update=false;

    if(access(AUDIO_PARAM_INFOR_PATH, R_OK)== 0){
        base=(struct param_infor *)malloc(sizeof(struct param_infor));
        if(NULL==base){
            LOG_I("check_updata_audioparam malloc param_infor failed");
            ret=-1;
            goto read_tunning_infor;
        }
        memset(base,AUDIO_PARAM_INVALID_8BIT_OFFSET,(sizeof(struct param_infor)));

        base_fd= open(AUDIO_PARAM_INFOR_PATH, O_RDONLY);
        if(base_fd<0){
            LOG_E("open %s failed",AUDIO_PARAM_INFOR_PATH);
            ret=-2;
            goto read_tunning_infor;
        }

        read_size=read(base_fd, base, sizeof(struct param_infor));
        if(read_size!=sizeof(struct param_infor)){
            LOG_E("read bytes %d",read_size);
            ret= -3;
            goto read_tunning_infor;
        }
        audio_param->backup_param_sn=base->param_sn;
        base_exist=true;
    }

    if(ret<0){
        LOG_W("check_updata_audioparam Error:%d",ret);
    }

read_tunning_infor:

    if(access(AUDIO_PARAM_INFOR_TUNNING_PATH, R_OK)== 0){
        data_infor=(struct param_infor *)malloc(sizeof(struct param_infor));
        if(NULL==data_infor){
            LOG_I("check_updata_audioparam malloc param_infor failed");
            ret=-4;
            goto out;
        }
        memset(data_infor,AUDIO_PARAM_INVALID_8BIT_OFFSET,(sizeof(struct param_infor)));

        data_infor_fd= open(AUDIO_PARAM_INFOR_TUNNING_PATH, O_RDONLY);
        if(data_infor_fd<0){
            LOG_E("open %s failed",AUDIO_PARAM_INFOR_TUNNING_PATH);
            ret= -5;
            goto out;
        }

        read_size=read(data_infor_fd, data_infor, sizeof(struct param_infor));
        if(read_size!=sizeof(struct param_infor)){
            LOG_E("read bytes %d",read_size);
            audio_param->audio_param_update=true;
            audio_param->backup_param_sn=1;
            audio_param->tunning_param_sn=audio_param->backup_param_sn;
            return_value=true;
            ret= -6;
            goto out;
        }
        audio_param->tunning_param_sn=data_infor->param_sn;
        tunning_exist=true;
    }

    LOG_I("check_updata_audioparam backup_param_sn:%d tunning_param_sn:%d",
        audio_param->backup_param_sn,audio_param->tunning_param_sn);

    if(base_exist==true){
        if(tunning_exist==false){
            audio_param->tunning_param_sn=audio_param->backup_param_sn;
            return_value=true;
        }else{
            if(audio_param->backup_param_sn>audio_param->tunning_param_sn){
                LOG_I("check_updata_audioparam SN:%d !=:%d",audio_param->backup_param_sn,audio_param->tunning_param_sn);
                audio_param->tunning_param_sn=audio_param->backup_param_sn;
                audio_param->audio_param_update=true;
                return_value=true;
            }else if((audio_param->tunning_param_sn!=audio_param->backup_param_sn)&&
                (audio_param->tunning_param_sn!=(audio_param->backup_param_sn+1))){
                LOG_I("check_updata_audioparam SN:%d !=:%d",audio_param->backup_param_sn,audio_param->tunning_param_sn);
                audio_param->tunning_param_sn=audio_param->backup_param_sn;
                audio_param->audio_param_update=true;
                return_value=true;
            }
        }
    }else{
        if(tunning_exist==false){
            audio_param->backup_param_sn=1;
            audio_param->tunning_param_sn=audio_param->backup_param_sn;
            return_value=true;
        }else{
            audio_param->backup_param_sn=audio_param->tunning_param_sn;
            return_value=false;
        }
    }

    if(ret<0){
        LOG_W("check_updata_audioparam Error:%d",ret);
    }

out:

    LOG_I("check_updata_audioparam ret:%d  update:%d backup_param_sn:%d tunning_param_sn:%d",
        ret,audio_param->audio_param_update,audio_param->backup_param_sn,audio_param->tunning_param_sn);

    if(NULL!=base){
        free(base);
        base=NULL;
    }

    if(NULL!=data_infor){
        free(data_infor);
        data_infor=NULL;
    }

    if(base_fd>0){
        close(base_fd);
        base_fd=-1;
    }

    if(data_infor_fd>0){
        close(data_infor_fd);
        data_infor_fd=-1;
    }

    return return_value;
}

int init_sprd_audio_param(AUDIO_PARAM_T  *audio_param,bool force)
{
    int ret = 0;
    int profile=0;

    struct param_infor_t *infor=NULL;
    if(AUD_REALTEK_CODEC_TYPE == audio_param->dev_ctl->codec_type){
        parse_realtek_extend_param();
        copy_audio_param(AUDIO_PARAM_RELTEK_EXTEND_PARAM_PATH,AUDIO_PARAM_RELTEK_EXTEND_PARAM_TUNNING_PATH);
    }

    LOG_D("init_sprd_audio_param1 enter");
    if(audio_param== NULL) {
        LOG_D("init_sprd_audio_param error");
        return -1;
    }

    LOG_I("init_sprd_audio_param:%d audio_param_update:%d",force,audio_param->audio_param_update);

    if(true==force){
        if(NULL==audio_param->infor){
            audio_param->infor=(struct param_infor *)malloc(sizeof(struct param_infor));
            if(NULL==audio_param->infor){
                LOG_I("init_sprd_audio_param malloc param_infor failed");
                return -1;
            }
        }
        memset(audio_param->infor,AUDIO_PARAM_INVALID_8BIT_OFFSET,(sizeof(struct param_infor)));
        force=true;
    }

    for(profile=0;profile<SND_AUDIO_PARAM_PROFILE_MAX;profile++){
        if(SND_AUDIO_PARAM_PGA_PROFILE ==profile){
            infor=NULL;

            if(force){
                free_sprd_audio_pga_param(&audio_param->dev_ctl->dg_gain);
                audio_param->param[profile].data=NULL;
                init_sprd_xml(&audio_param->dev_ctl->dg_gain,&(audio_param->param[profile].xml),audio_param_file_table[profile].tunning_file,
                    audio_param_file_table[profile].src_file,infor,load_sprd_audio_pga_param,audio_param->audio_param_update);
            }else{
                init_sprd_xml(&audio_param->dev_ctl->dg_gain,NULL,audio_param_file_table[profile].tunning_file,
                    audio_param_file_table[profile].src_file,infor,load_sprd_audio_pga_param,audio_param->audio_param_update);
            }
        }else if(SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE ==profile){
            LOG_I("init_sprd_audio_param audio_process");
            if(audio_param->infor==NULL){
                infor=NULL;
            }else{
                infor=&(audio_param->infor->data[profile]);
            }

            if(force){
                init_sprd_xml(&audio_param->param[profile],&(audio_param->param[profile].xml),AUDIO_PROCESS_XML_TUNNING_PATH,AUDIO_PROCESS_PATH,
                    infor
                    ,load_sprd_ap_audio_param,audio_param->audio_param_update);
            }else{
                init_sprd_xml(&audio_param->param[profile],NULL,AUDIO_PROCESS_XML_TUNNING_PATH,AUDIO_PROCESS_PATH,infor,
                    load_sprd_ap_audio_param,audio_param->audio_param_update);
            }
        }else{
            if((false== force) && (false==audio_param->audio_param_update)&&(access(audio_param_file_table[profile].bin_file, R_OK) == 0)) {
                LOG_D("init_sprd_audio_param %s is exist\n", audio_param_file_table[profile].bin_file);
            } else {
                ret = init_sprd_audio_param_from_xml(audio_param,profile);
            }
        }
    }

    if(NULL!=audio_param->infor){
        audio_param->infor->param_sn=audio_param->tunning_param_sn;
        save_audio_param_infor(audio_param->infor);
    }else{
        audio_param->infor=(struct param_infor *)malloc(sizeof(struct param_infor));
        if(NULL==audio_param->infor){
            LOG_I("init_sprd_audio_param malloc param_infor failed");
            return -1;
        }
        memset(audio_param->infor,0xff,(sizeof(struct param_infor)));
        load_audio_param_infor(audio_param->infor);
    }


    if(access(AUDIO_PARAM_INFOR_PATH, R_OK)!= 0){
        audio_param->backup_param_sn=audio_param->tunning_param_sn;
    }

    audio_param->audio_param_update=false;

    LOG_I("init_sprd_audio_param exit param_sn:%d",audio_param->infor->param_sn);
    return 0;
}

int clear_audio_param(AUDIO_PARAM_T  *audio_param){
    LOG_D("clear_audio_param:%x",audio_param->current_param);
    pthread_mutex_lock(&audio_param->audio_param_lock);
    audio_param->current_param=0;
    pthread_mutex_unlock(&audio_param->audio_param_lock);
    return 0;
}

static int load_sprd_audio_pga_param(void *gain,param_group_t ele, param_infor_t  * param_infor,char *path){
    int id=0;
    struct device_usecase_gain *use_gain=(struct device_usecase_gain *)gain;
    id=get_audio_param_id(path);
    if(id<0){
        return id;
    }

    parse_device_gain(use_gain,use_gain->mixer,(TiXmlElement *)ele);
    (use_gain->dev_gain+use_gain->gain_size-1)->id=id;
    LOG_D("\nload_sprd_audio_pga_param ID:%d :%p",id,ele);

    for(int j = 0; j < use_gain->gain_size; j++) {
        struct device_gain *dump = NULL;
        dump = &(use_gain->dev_gain[j]);
        LOG_D("load_sprd_audio_pga_param dump audio gain[%d]:%s size:%d", j, dump->name,
              dump->ctl_size);
        for(int z = 0; z < dump->ctl_size; z++) {
            struct gain_mixer_control *dump_mixer = NULL;
            dump_mixer = &(dump->ctl[z]);
            LOG_D("[\t%s %d  %d]\n", dump_mixer->name, dump_mixer->volume_size,
                  dump_mixer->volume_value[0]);
        }
    }

    return 0;
}

int free_sprd_audio_pga_param(struct device_usecase_gain *use_gain){
    int i=0,j=0;
    struct device_gain *dev_gain;
    struct gain_mixer_control *ctl;

    if(NULL!=use_gain->dev_gain){
        for(i=0;i<use_gain->gain_size;i++){
            dev_gain=&use_gain->dev_gain[i];

            if(NULL!=dev_gain){
                LOG_D("1 dev_gain:%p",dev_gain);
                for(j=0;j<dev_gain->ctl_size;j++){
                    ctl=&dev_gain->ctl[j];

                    if(NULL!=ctl){
                        if(ctl->volume_value!=NULL){
                            free(ctl->volume_value);
                        }

                        if(NULL!=ctl->name){
                            free(ctl->name);
                        }

                    }
                }

                LOG_D("2 dev_gain:%p",dev_gain);
                if(NULL!=dev_gain->name){
                    free(dev_gain->name);
                }

                if(NULL!=dev_gain->ctl){
                    free(dev_gain->ctl);
                }


            }

        }

    }

    if(NULL!=use_gain->dev_gain){
        free(use_gain->dev_gain);
    }
    use_gain->dev_gain=NULL;
    use_gain->gain_size=0;

    return 0;
}

static int _load_sprd_ap_audio_param(TiXmlElement *param_group, char *data,bool root,int *offset,int max_size){
    const char *val=NULL;
    const char *visible=NULL;
    const char *type=NULL;
    int values=0;
    int size=0;
    int values_size=0;
    int true_string_size=strlen(TRUE_STRING);
    char val_str[16]={0};

    LOG_D("enter %s %p 0x%x",param_group->Value(),data,*offset);
    TiXmlElement *group = param_group;
    while(group != NULL){
        LOG_D("ELE:%s size:%d",group->Value(),size);
        val = group->Attribute(VALUE);
        if (val == NULL) {
            size+=_load_sprd_ap_audio_param(group->FirstChildElement(), data, false,offset,max_size);
        }else{
            type = group->Attribute(TYPE);
            if(type==NULL){
                LOG_E("_load_sprd_ap_audio_param %s type err",param_group->Value());
                size=0;
                goto exit;
            }

            if(strcmp(type, U32) == 0){
                values_size = sizeof(int32_t);
            } else if(strcmp(type, U16) == 0){
                values_size = sizeof(int16_t);
            } else if(strcmp(type, U8) == 0){
                values_size = sizeof(uint8_t);
            };

            if(*offset+values_size>max_size){
                LOG_E("_load_sprd_ap_audio_param error, offset:%x size:%d max:%d Element:%s",*offset,values_size,max_size,group->Value());
                return size;
            }
            values=strtoul(val,NULL,0);
            memcpy(data+*offset,&values,values_size);
            group->SetAttribute(ID,size);
            size+=values_size;

            sprintf(val_str,"0x%x",*offset);
            group->SetAttribute(ID, val_str);
            *offset+=values_size;
        }

        if (root) {
            break;
        }
        group = group->NextSiblingElement();
    }
exit:
    return size;
}


static int load_sprd_ap_audio_param(void *audio_param,param_group_t ele, param_infor_t  * param_infor,char *path){
    char * process_param=NULL;
    AUDIOVBCEQ_PARAM_T *param=(AUDIOVBCEQ_PARAM_T *)audio_param;
    TiXmlElement *param_group=(TiXmlElement *)ele;
    int id=0;
    const char  *tmp=NULL;
    const char *val;
    tmp=param_group->Value();
    int ret=-1;
    struct audio_ap_param *ap_param=NULL;
    int offset=0;

    if(NULL==tmp){
        LOG_E("load_sprd_ap_audio_param err");
        return -1;
    }

    id=get_audio_param_id(path);
    if(id<0){
        LOG_E("load_sprd_ap_audio_param:%s failed",path);
        return id;
    }

    if(NULL==param->data){
        ap_param=(struct audio_ap_param *)malloc(sizeof(struct audio_ap_param));
        if(NULL==ap_param){
               LOG_E("load_sprd_ap_audio_param malloc failed");
               param->data=NULL;
               return -1;
        }
        param->param_struct_size=sizeof(struct audio_ap_param);
        param->num_mode=1;
        param->data=(char *)ap_param;
    }else{
        ap_param=(struct audio_ap_param *)(param->data);
    }

    if((id>=PROFILE_MODE_AUDIO_Handset_GSM ) && (id<=PROFILE_MODE_AUDIO_BTHSNREC_VOIP)){
        process_param=(char *)&(ap_param->voice[id]);
   }else if(PROFILE_MODE_MUSIC_Handsfree_Record == id){
        process_param=(char *)&(ap_param->record[AUDIO_RECORD_MODE_HANDSFREE]);
   }else if(PROFILE_MODE_MUSIC_Headset_Record == id){
        process_param=(char *)&(ap_param->record[AUDIO_RECORD_MODE_HEADSET]);
   }else if(PROFILE_MODE_MUSIC_Bluetooth_Record == id){
        process_param=(char *)&(ap_param->record[AUDIO_RECORD_MODE_BLUETOOTH]);
   }else if((id >= PROFILE_MODE_LOOP_Handset_MainMic) && (id <= PROFILE_MODE_LOOP_Headset3P_MainMic)){
        process_param=(char *)&(ap_param->loop[id-PROFILE_MODE_LOOP_Handset_MainMic]);
   }else{
        LOG_E("load_sprd_ap_audio_param %s not support",tmp);
        return -1;
   }

    offset=(char *)process_param-(char *)ap_param;
    if(NULL!=param_infor){
        param_infor->offset[id]=offset;
        LOG_D("load_sprd_ap_audio_param return:%d process_param:%p  offset:%d id:%d %s",ret,process_param,param_infor->offset[id],id,path);
    }
    ret = _init_sprd_audio_param_from_xml(param_group,true,(unsigned char*)
    param->data,&offset,3,
    NULL,
    NULL);

    return 0;
}

static int init_sprd_xml(void * load_param_func_res,
                   struct xml_handle *xml,
                   const char *tunning_param_path,
                   const char *param_path,
                   struct param_infor_t  * param_infor,
                   load_sprd_audio_param load_param_func,bool update_param){
    int ret = 0;
    const char *parampath;
    bool istunning = false;
    TiXmlDocument *param_doc = NULL;

    TiXmlElement * ele_1=NULL;
    TiXmlElement * ele_2=NULL;
    TiXmlElement * ele_3=NULL;
    TiXmlElement * group=NULL;
    TiXmlElement * root=NULL;
    struct xml_handle *xmlhandle;
    struct xml_handle xml_tmp;
    char *ele_1_name=NULL;
    char *ele_2_name=NULL;
    char *ele_3_name=NULL;
    char path[MAX_LINE_LEN] = {0};

    LOG_I("init_sprd_xml enter");
    memset(&xml_tmp,0,sizeof(struct xml_handle));
    if(xml==NULL){
        xmlhandle=&xml_tmp;
    }else{
        xmlhandle=xml;
        if(NULL!=xmlhandle->param_root){
            release_xml_handle(xmlhandle);
        }
    }

    if(NULL==xmlhandle->param_root){
        xmlhandle->first_name=NULL;
        xmlhandle->param_root=NULL;
        xmlhandle->param_doc=NULL;

        if (tunning_param_path != NULL) {
            istunning = true;
        }
        if((false==update_param)&&(is_file_exist(tunning_param_path))){
            parampath = tunning_param_path;
        } else {
            parampath = param_path;
        }

        if(NULL!=parampath){
            LOG_I("init_sprd_xml:%s",parampath);
        }

        ret = load_xml_handle(xmlhandle, parampath);
        if (ret != 0) {
            LOG_E("load xml handle failed (%s)", parampath);
            if(tunning_param_path == parampath){
                parampath=param_path;
                ret = load_xml_handle(xmlhandle, parampath);
                if (ret != 0) {
                    LOG_E("load xml handle failed (%s)", param_path);
                    return ret;
                }
            }
        }
    }

    if(NULL==xmlhandle->param_root){
        LOG_E("init_sprd_xml param_root is null");
        return -1;
    }

    root=(TiXmlElement * )xmlhandle->param_root;

    ele_1 = root->FirstChildElement();
    while(ele_1!=NULL){
        ele_1_name=strdup(ele_1->Value());
        LOG_D("init_sprd_xml ele_1 start:%s",ele_1_name);
        ele_2 = ele_1->FirstChildElement();
        while(ele_2!=NULL){
            ele_2_name=strdup(ele_2->Value());
            LOG_D("init_sprd_xml ele_2 start:%s :%p",ele_2_name,ele_2);
            ele_3 = ele_2->FirstChildElement();
            while(NULL!=ele_3){
                ele_3_name=strdup(ele_3->Value());
                LOG_D("init_sprd_xml ele_3 start:%s :%p",ele_3_name,ele_3);

                if(NULL!=load_param_func){
                    strcat(path, ele_1_name);
                    strcat(path, BACKSLASH);
                    strcat(path, ele_2_name);
                    strcat(path, BACKSLASH);
                    strcat(path, ele_3_name);
                    group=ele_3;
                    load_param_func(load_param_func_res,group,param_infor,path);
                    memset(path,0,sizeof(path));
                }
                LOG_D("init_sprd_xml ele_3 end:%s :%p",ele_3_name,ele_3);
                ele_3 = ele_3->NextSiblingElement();

                free(ele_3_name);
                ele_3_name=NULL;
            }

            LOG_D("init_sprd_xml ele_2 end:%s :%p",ele_2_name,ele_2);
            ele_2 = ele_2->NextSiblingElement();

            free(ele_2_name);
            ele_2_name=NULL;
        }
        LOG_D("init_sprd_xml ele_1 end:%s :%p",ele_1_name);
        ele_1 = ele_1->NextSiblingElement();

        free(ele_1_name);
        ele_1_name=NULL;
    }

    if(xml==NULL){
        release_xml_handle(xmlhandle);
    }else{
        param_doc = (TiXmlDocument *)xmlhandle->param_doc;

        if((true==update_param)&&(tunning_param_path!=NULL)){
            LOG_I("init_sprd_xml update_param save:%s",tunning_param_path);
            param_doc->SaveFile(tunning_param_path);
        }else if((NULL != param_doc)&& (parampath == param_path)&& (tunning_param_path!=NULL)){
            LOG_I("init_sprd_xml save:%s",tunning_param_path);
            param_doc->SaveFile(tunning_param_path);
        }

        if (!istunning) {
            LOG_I("init_sprd_xml exit release_xml_handle");
            release_xml_handle(xmlhandle);
        }
    }
    LOG_I("init_sprd_xml exit");
    return ret;
}



int upload_audio_profile_param_firmware(AUDIO_PARAM_T * audio_param,int profile){
    struct mixer_ctl *mixer;
    struct mixer_ctl *eq_update=NULL;
    int ret=-1;

    mixer = audio_param->update_mixer[profile];
    if (!mixer) {
        LOG_E("upload_audio_profile_param_firmware Failed to open mixer, profile: %d", profile);
        return -1;
    }

    ret = mixer_ctl_set_value(mixer, 0, 1);
    if (ret != 0) {
        LOG_E("upload_audio_profile_param_firmware Failed profile:%d\n",profile);
    }
    LOG_I("upload_audio_profile_param_firmware:%d, ret(%d)\n", profile,ret);
    return 0;
}

int upload_audio_param_firmware(AUDIO_PARAM_T * audio_param){
    int ret=-1;
    LOG_D("upload_audio_param_firmware");
    ret |=upload_audio_profile_param_firmware(audio_param,SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP);
    ret |=upload_audio_profile_param_firmware(audio_param,SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE);
    ret |=upload_audio_profile_param_firmware(audio_param,SND_AUDIO_PARAM_NXP_PROFILE);
    if(AUD_REALTEK_CODEC_TYPE == audio_param->dev_ctl->codec_type){
        ret |=upload_audio_profile_param_firmware(audio_param,SND_AUDIO_PARAM_CODEC_PROFILE);
        ret |=upload_realtek_extend_param(audio_param->dev_ctl->mixer);
    }
    return ret;
}

static struct _realtek_extend *alloc_extend_param(struct realtek_extend *extern_data,int16_t size, uint8_t start_mode,uint8_t end_mode)
{
    struct _realtek_extend *new_extend=NULL;
    struct realtek_extend_reg *reg=NULL;

    new_extend = (struct _realtek_extend *)realloc(extern_data->data,(extern_data->count + 1) * sizeof(struct _realtek_extend));
    if (new_extend == NULL) {
        LOG_E("alloc alloc_extend_param failed");
        return NULL;
    } else {
        extern_data->data = new_extend;
    }

    reg=(struct realtek_extend_reg *)malloc(size*sizeof(struct realtek_extend_reg));
    if(reg==NULL){
        return NULL;
    }
    memset(reg,0,(size*sizeof(struct realtek_extend_reg)));

    extern_data->data[extern_data->count].header.end_mode=end_mode;
    extern_data->data[extern_data->count].header.start_mode=start_mode;
    extern_data->data[extern_data->count].header.size=size;
    extern_data->data[extern_data->count].reg=reg;

    extern_data->size +=sizeof(struct realtek_extend_header)+size*sizeof(struct  realtek_extend_reg);

    return &extern_data->data[extern_data->count++];
}

static void free_realtek_extend_param(struct realtek_extend *extern_data){
    int16_t count=0;
    for(count=0;count<extern_data->count;count++){
        free(extern_data->data[count].reg);
        extern_data->data[count].reg=NULL;
    }

    free(extern_data->data);
    extern_data->data=NULL;
    extern_data->size=0;
    extern_data->count=0;
    return ;
}


static int copy_audio_param(const char *src, const char *dst){
    int src_fd=-1;
    int dst_fd=-1;
    char buf[128]={0};
    int readbytes=0;
    int writebytes=0;
    char *write_ptr=NULL;
    int ret=0;
    if(access(src, R_OK) != 0){
        LOG_I("copy_audio_param:%s is not exist",src);
        return 0;
    }

    if(access(dst, R_OK) == 0){
        LOG_I("copy_audio_param:%s is exist",dst);
        return 0;
    }

    dst_fd= open(dst, O_RDWR | O_CREAT,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(dst_fd<0){
        LOG_E("copy_audio_param: create %s failed",dst);
        return -2;
    }

    src_fd= open(src, O_RDONLY);
    if(dst_fd<0){
        LOG_E("copy_audio_param: open %s failed",src);
        return -3;
    }

    readbytes=read(src_fd,buf,sizeof(buf));
    while(readbytes){
        write_ptr=buf;
        writebytes=0;
        do {
            writebytes = write(dst_fd, write_ptr, readbytes);
            if (writebytes > 0) {
                if (writebytes <= readbytes) {
                    readbytes -= writebytes;
                    write_ptr+=writebytes;
                }
            } else if ((!((errno == EAGAIN) || (errno == EINTR))) || (0 == writebytes)) {
                LOG_E("copy_audio_param:write error %d", errno);
                ret=errno;
                break;
            } else {
                LOG_E("copy_audio_param:write_warning: %d, writebytes is %d", errno, writebytes);
            }
        } while (readbytes);
        memset(buf,0,sizeof(buf));
        readbytes=read(src_fd,buf,sizeof(buf));
    }

    if(src_fd){
        close(src_fd);
    }

    if(dst_fd){
        close(dst_fd);
    }
    return ret;
}

static void parse_realtek_extend_param(void){
    struct xml_handle xmlhandle;
    struct realtek_extend extern_data;
    TiXmlElement * root=NULL;
    TiXmlElement * ele_1=NULL;
    TiXmlElement * ele_2=NULL;

    int ret=0;

    if((access(AUDIO_PARAM_RELTEK_EXTEND_PARAM_TUNNING_PATH, R_OK) == 0)
        ||(access(AUDIO_PARAM_RELTEK_EXTEND_PARAM_PATH, R_OK) == 0)){
        return;
    }

    if(access(AUDIO_PARAM_RELTEK_EXTEND_XML_PARAM, R_OK) != 0){
        LOG_I("parse_realtek_extend_param:%s not exist",AUDIO_PARAM_RELTEK_EXTEND_XML_PARAM);
        return;
    }

    extern_data.count=0;
    extern_data.size=0;
    extern_data.data=NULL;
    extern_data.size=4;

    ret = load_xml_handle(&xmlhandle,AUDIO_PARAM_RELTEK_EXTEND_XML_PARAM);
    root=(TiXmlElement *)xmlhandle.param_root;
    ele_1 = root->FirstChildElement();
    while(ele_1!=NULL){
        const char *param_name=ele_1->Attribute("param_name");
        const char *str=NULL;
        int16_t size=0;
        uint8_t start_mode=PROFILE_MODE_MAX;
        uint8_t end_mode=PROFILE_MODE_MAX;
        uint8_t reg_count=0;
        struct _realtek_extend *data=NULL;

        if(NULL==param_name){
            ret=-1;
            break;
        }

        str=ele_1->Attribute("size");
        size=strtoul(str,NULL,0);

        for(int i=0;i<PROFILE_MODE_MAX;i++){
            if(0 == strncmp(audio_param_mode_table[i].name,param_name,strlen(param_name))){
                if(PROFILE_MODE_MAX==start_mode){
                    start_mode=audio_param_mode_table[i].mode;
                }

                if(PROFILE_MODE_MAX==end_mode){
                    end_mode=audio_param_mode_table[i].mode;
                }

                if(audio_param_mode_table[i].mode<start_mode){
                    start_mode=audio_param_mode_table[i].mode;
                }

                if(audio_param_mode_table[i].mode>end_mode){
                    end_mode=audio_param_mode_table[i].mode;
                }
            }
        }

        data=alloc_extend_param(&extern_data,size,start_mode,end_mode);

        ele_2 = ele_1->FirstChildElement();
        reg_count=0;
        while((ele_2!=NULL)&&(reg_count<size)){
            str=ele_2->Attribute("addr");
            if(str!=NULL){
                data->reg[reg_count].addr=strtoul(str,NULL,16);
                str=NULL;
            }else{
                LOG_W("%s %s not find Attribute:addr",__func__,ele_2->Value());
                continue;
            }

            str=ele_2->Attribute("val");
            if(str!=NULL){
                data->reg[reg_count].val=strtoul(str,NULL,16);
            }else{
                LOG_W("%s %s not find Attribute:val",__func__,ele_2->Value());
                continue;
            }

            str=ele_2->Attribute("mask");
            if(str!=NULL){
                data->reg[reg_count].mask=strtoul(str,NULL,16);
            }else{
                LOG_W("%s %s not find Attribute:mask",__func__,ele_2->Value());
                data->reg[reg_count].mask=-1;
            }

            reg_count++;
            ele_2 =ele_2 ->NextSiblingElement();
        }

        ele_1 =ele_1 ->NextSiblingElement();
    }

    if(root!=NULL){
        release_xml_handle(&xmlhandle);
    }

    if(ret==0){
        int fd=-1;
        fd= open(AUDIO_PARAM_RELTEK_EXTEND_PARAM_TUNNING_PATH, O_RDWR | O_CREAT,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

        if(fd<0){
            LOG_E("parse_realtek_extend_param create %s failed",AUDIO_PARAM_RELTEK_EXTEND_PARAM_TUNNING_PATH);
        }else{
            char *buf=(char *)malloc(extern_data.size);
            int16_t buf_size=0;

            memcpy(buf,&extern_data.size,sizeof(int16_t));
            buf_size+=sizeof(int16_t);
            memcpy(buf+buf_size,&extern_data.count,sizeof(int16_t));
            buf_size+=sizeof(int16_t);

            for(int i=0;i<extern_data.count;i++){
                memcpy(buf+buf_size,&extern_data.data[i].header,sizeof(struct realtek_extend_header));
                buf_size+=sizeof(realtek_extend_header);

                memcpy(buf+buf_size,extern_data.data[i].reg,sizeof(struct realtek_extend_reg)*extern_data.data[i].header.size);
                buf_size+=sizeof(struct realtek_extend_reg)*extern_data.data[i].header.size;
            }

            write(fd, buf,buf_size);
            free(buf);
            LOG_I("parse_realtek_extend_param size:0x%x count:0x%x",extern_data.size,extern_data.count);
            close(fd);
        }
    }else{
        LOG_E("%s failed",__func__);
    }

    free_realtek_extend_param(&extern_data);
}

static int upload_realtek_extend_param(struct mixer *mixer){
    struct mixer_ctl * upload_ctl=NULL;
    int ret=-1;

    if(access(AUDIO_PARAM_RELTEK_EXTEND_PARAM_TUNNING_PATH, R_OK) != 0){
        LOG_I("upload_realtek_extend_param:access:%s failed",AUDIO_PARAM_RELTEK_EXTEND_PARAM_TUNNING_PATH);
        return 0;
    }

    upload_ctl=mixer_get_ctl_by_name(mixer, REALTEK_EXTEND_PARAM_UPDATE);
    if (!upload_ctl) {
        LOG_E("upload_realtek_extend_param Failed to open mixer:%s", REALTEK_EXTEND_PARAM_UPDATE);
        return -1;
    }

    ret = mixer_ctl_set_value(upload_ctl, 0, 1);
    if (ret != 0) {
        LOG_E("upload_realtek_extend_param Failed");
    }
    return ret;
}
#ifdef __cplusplus
}
#endif

