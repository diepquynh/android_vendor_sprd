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

static int apply_audio_profile_param_firmware(struct mixer_ctl *select,int id);
typedef int (*load_audio_param) (struct tiny_audio_device *adev);

typedef int (*load_sprd_audio_param) (void *,param_group_t ele, char * path);
extern int parse_device_gain(struct device_usecase_gain *use_gain,
                             struct mixer *mixer, TiXmlElement *device);
static int load_sprd_audio_pga_param(void *use_gain,param_group_t ele, char *path);

static int _load_sprd_audio_process_param(TiXmlElement *param_group, char *data,bool root,int *offset);
static int load_sprd_audio_process_param(void *param,param_group_t ele, char *path);
static int free_sprd_audio_pga_param(struct device_usecase_gain *use_gain);

static int init_sprd_xml(void * load_param_func_res,
                   struct xml_handle *xml,
                   const char *tunning_param_path,
                   const char *param_path,
                   const char *first_name,
                   load_sprd_audio_param load_param_func);

const struct audio_param_file_t  audio_param_file_table[SND_AUDIO_PARAM_PROFILE_MAX] = {
    {SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP,        DSP_VBC_XML_TUNNING_PATH  , DSP_VBC_BIN_PATH  , DSP_VBC_XML_PATH , DSP_VBC_FIRST_NAME },
    {SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE,    AUDIO_STRUCTURE_XML_TUNNING_PATH  , AUDIO_STRUCTURE_BIN_PATH  , AUDIO_STRUCTURE_XML_PATH , AUDIO_STRUCTURE_FIRST_NAME},
    {SND_AUDIO_PARAM_NXP_PROFILE,                NXP_XML_TUNNING_PATH  , NXP_BIN_PATH  , NXP_XML_PATH , NXP_FIRST_NAME},
    {SND_AUDIO_PARAM_PGA_PROFILE,                PGA_GAIN_XML_TUNNING_PATH  , NULL  , PGA_GAIN_XML_PATH , PGA_GAIN_FIRST_NAME},
    {SND_AUDIO_PARAM_CODEC_PROFILE,              CODEC_XML_TUNNING_PATH, CODEC_BIN_PATH  , CODEC_XML_PATH , CODEC_FIRST_NAME},
    {SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE, AUDIO_PROCESS_XML_TUNNING_PATH, NULL,AUDIO_PROCESS_PATH , AUDIO_PROCESS_FIRST_NAME}
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


int load_xml_handle(struct xml_handle *xmlhandle, const char *xmlpath,
                    const char *first_name)
{
    LOG_D("load_xml_handle:%s",xmlpath);
    xmlhandle->param_doc = XML_open_param(xmlpath);
    if (xmlhandle->param_doc == NULL) {
        LOG_E("load xml handle failed (%s)", xmlpath);
        return -1;
    }
    xmlhandle->param_root =  XML_get_root_param(xmlhandle->param_doc);
    if (first_name != NULL) {
        xmlhandle->first_name = strdup(first_name);
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
    { PROFILE_MODE_AUDIO_Handset_VOLTE_NB,           "Audio\\Handset\\VOLTE_WB"     },
    { PROFILE_MODE_AUDIO_Handset_VOLTE_WB,           "Audio\\Handset\\VOLTE_NB"     },
    { PROFILE_MODE_AUDIO_Handset_VOIP,               "Audio\\Handset\\VOIP"         },
    { PROFILE_MODE_AUDIO_Handset_Loopback,           "Audio\\Handset\\Loopback"     },

    { PROFILE_MODE_AUDIO_Handsfree_GSM,              "Audio\\Handsfree\\GSM"        },
    { PROFILE_MODE_AUDIO_Handsfree_TDMA,             "Audio\\Handsfree\\TDMA"       },
    { PROFILE_MODE_AUDIO_Handsfree_WCDMA_NB,         "Audio\\Handsfree\\WCDMA_NB"   },
    { PROFILE_MODE_AUDIO_Handsfree_WCDMA_WB,         "Audio\\Handsfree\\WCDMA_WB"   },
    { PROFILE_MODE_AUDIO_Handsfree_VOLTE_NB,         "Audio\\Handsfree\\VOLTE_WB"   },
    { PROFILE_MODE_AUDIO_Handsfree_VOLTE_WB,         "Audio\\Handsfree\\VOLTE_NB"   },
    { PROFILE_MODE_AUDIO_Handsfree_VOIP,             "Audio\\Handsfree\\VOIP"       },
    { PROFILE_MODE_AUDIO_Handsfree_Loopback,         "Audio\\Handsfree\\Loopback"   },

    { PROFILE_MODE_AUDIO_Headset4P_GSM,              "Audio\\Headset4P\\GSM"        },
    { PROFILE_MODE_AUDIO_Headset4P_TDMA,             "Audio\\Headset4P\\TDMA"       },
    { PROFILE_MODE_AUDIO_Headset4P_WCDMA_NB,         "Audio\\Headset4P\\WCDMA_NB"   },
    { PROFILE_MODE_AUDIO_Headset4P_WCDMA_WB,         "Audio\\Headset4P\\WCDMA_WB"   },
    { PROFILE_MODE_AUDIO_Headset4P_VOLTE_NB,         "Audio\\Headset4P\\VOLTE_WB"   },
    { PROFILE_MODE_AUDIO_Headset4P_VOLTE_WB,         "Audio\\Headset4P\\VOLTE_NB"   },
    { PROFILE_MODE_AUDIO_Headset4P_VOIP,             "Audio\\Headset4P\\VOIP"       },
    { PROFILE_MODE_AUDIO_Headset4P_Loopback,         "Audio\\Headset4P\\Loopback"   },

    { PROFILE_MODE_AUDIO_Headset3P_GSM,              "Audio\\Headset3P\\GSM"        },
    { PROFILE_MODE_AUDIO_Headset3P_TDMA,             "Audio\\Headset3P\\TDMA"       },
    { PROFILE_MODE_AUDIO_Headset3P_WCDMA_NB,         "Audio\\Headset3P\\WCDMA_NB"   },
    { PROFILE_MODE_AUDIO_Headset3P_WCDMA_WB,         "Audio\\Headset3P\\WCDMA_WB"   },
    { PROFILE_MODE_AUDIO_Headset3P_VOLTE_NB,         "Audio\\Headset3P\\VOLTE_WB"   },
    { PROFILE_MODE_AUDIO_Headset3P_VOLTE_WB,         "Audio\\Headset3P\\VOLTE_NB"   },
    { PROFILE_MODE_AUDIO_Headset3P_VOIP,             "Audio\\Headset3P\\VOIP"       },
    { PROFILE_MODE_AUDIO_Headset3P_Loopback,         "Audio\\Headset3P\\Loopback"   },

    { PROFILE_MODE_AUDIO_BTHSD8K_GSM,                "Audio\\BTHSD8K\\GSM"          },
    { PROFILE_MODE_AUDIO_BTHSD8K_TDMA,               "Audio\\BTHSD8K\\TDMA"         },
    { PROFILE_MODE_AUDIO_BTHSD8K_WCDMA,              "Audio\\BTHSD8K\\WCDMA"        },
    { PROFILE_MODE_AUDIO_BTHSD8K_VOLTE,              "Audio\\BTHSD8K\\VOLTE"        },
    { PROFILE_MODE_AUDIO_BTHSD8K_VOIP,               "Audio\\BTHSD8K\\VOIP"         },

    { PROFILE_MODE_AUDIO_BTHSD16K_GSM,               "Audio\\BTHSD16K\\GSM"         },
    { PROFILE_MODE_AUDIO_BTHSD16K_TDMA,              "Audio\\BTHSD16K\\TDMA"        },
    { PROFILE_MODE_AUDIO_BTHSD16K_WCDMA,             "Audio\\BTHSD16K\\WCDMA"       },
    { PROFILE_MODE_AUDIO_BTHSD16K_VOLTE,             "Audio\\BTHSD16K\\VOLTE"       },
    { PROFILE_MODE_AUDIO_BTHSD8K_VOIP,               "Audio\\BTHSD16K\\VOIP"        },

    { PROFILE_MODE_AUDIO_BTHSNRECD8K_GSM,            "Audio\\BTHSNRECD8K\\GSM"      },
    { PROFILE_MODE_AUDIO_BTHSNRECD8K_TDMA,           "Audio\\BTHSNRECD8K\\TDMA"     },
    { PROFILE_MODE_AUDIO_BTHSNRECD8K_WCDMA,          "Audio\\BTHSNRECD8K\\WCDMA"    },
    { PROFILE_MODE_AUDIO_BTHSNRECD8K_VOLTE,          "Audio\\BTHSNRECD8K\\VOLTE"    },
    { PROFILE_MODE_AUDIO_BTHSNRECD8K_VOIP,           "Audio\\BTHSNRECD8K\\VOIP"     },

    { PROFILE_MODE_AUDIO_BTHSNRECD16K_GSM,           "Audio\\BTHSNRECD16K\\GSM"     },
    { PROFILE_MODE_AUDIO_BTHSNRECD16K_TDMA,          "Audio\\BTHSNRECD16K\\TDMA"    },
    { PROFILE_MODE_AUDIO_BTHSNRECD16K_WCDMA,         "Audio\\BTHSNRECD16K\\WCDMA"   },
    { PROFILE_MODE_AUDIO_BTHSNRECD16K_VOLTE,         "Audio\\BTHSNRECD16K\\VOLTE"   },
    { PROFILE_MODE_AUDIO_BTHSNRECD16K_VOIP,          "Audio\\BTHSNRECD16K\\VOIP"    },

    { PROFILE_MODE_MUSIC_Headset_Playback,           "Music\\Headset\\Playback"     },
    { PROFILE_MODE_MUSIC_Headset_Record,             "Music\\Headset\\Record"       },
    { PROFILE_MODE_MUSIC_Headset_FM,                 "Music\\Headset\\FM"           },

    { PROFILE_MODE_MUSIC_Handsfree_Playback,         "Music\\Handsfree\\Playback"     },
    { PROFILE_MODE_MUSIC_Handsfree_Record,           "Music\\Handsfree\\Record"       },
    { PROFILE_MODE_MUSIC_Handsfree_FM,               "Music\\Handsfree\\FM"           },

    { PROFILE_MODE_MUSIC_Headfree_Playback,          "Music\\Headfree\\Playback"    },

    { PROFILE_MODE_MUSIC_Handset_Playback,           "Music\\Handset\\Playback"     },
};

int get_audio_param_id(const char *name){
    int profile=-1;
    for(int i=0;i<PROFILE_MODE_MAX;i++){
        if(strncmp(audio_param_mode_table[i].name,name,strlen(audio_param_mode_table[i].name)-1)==0){
            LOG_D("get_audio_param_id :%s len:%d return:%d",name,strlen(name),i);
            return i;
        }
    }
    LOG_E("get_audio_param_id err:%s len:%d",name,strlen(name));
    return -1;
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
            return i;
        }
    }
    return -1;
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
        strncpy(fw_header->magic, audio_param_file_table[profile].first_name, FIRMWARE_MAGIC_MAX_LEN);
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

        mask=0;
        for(i=0;i<bits;i++){
            mask |=1<<i;
        }

        values &=mask;

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

            val = val & ((1 << bits) - 1);

            values |= (val << offset);
            LOG_D("name:%s bits:%d offset:%d val:0x%x values:0x%x\n",
                  ele->Value(), bits, offset, val, values);
            ele = (TiXmlElement *) XML_get_next_sibling_group(ele);
        }
    }
    return values;
}


int _init_sprd_audio_param_from_xml(param_group_t param,bool root,
                                  unsigned char *data,int *offset,int depth)
{
    int ret=0;
    int size=0;
    char val_str[32]={0};

    const char * tmp=NULL;
    
    TiXmlElement *group = (TiXmlElement *)param;
    while(group!=NULL){
        LOG_D("ele:%d: %s total_size:0x%x",depth,group->Value(),*offset);
        tmp = group->Attribute(TYPE);
        if(tmp==NULL){
            if((data!=NULL) && (offset!=NULL)){
                 _init_sprd_audio_param_from_xml(group->FirstChildElement(),false,data,offset,depth+1);
            }else{
                 _init_sprd_audio_param_from_xml(group->FirstChildElement(),false,NULL,NULL,depth+1);
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

    if(access(audio_param_file_table[profile].tunning_file, R_OK) == 0) {
        tunning=true;
    } else {
        if(access(audio_param_file_table[profile].src_file, R_OK)!= 0){
            LOG_E("open_audio_param_file %s not exit",audio_param_file_table[profile].src_file);
            return -1;
        }
        tunning=false;
    }

    if(true==tunning){
        load_xml_handle(&(audio_param->xml), audio_param_file_table[profile].tunning_file, audio_param_file_table[profile].first_name);
    }else{
        load_xml_handle(&(audio_param->xml), audio_param_file_table[profile].src_file, audio_param_file_table[profile].first_name);
    }

    if(NULL==audio_param->xml.param_root) {
        LOG_E("open_audio_param_file can not get root:%s\n", audio_param_file_table[profile].first_name);
        if(true==tunning){
            char cmd[256]={0};
            snprintf(cmd,sizeof(cmd),"rm -rf %s",audio_param_file_table[profile].tunning_file);
            system(cmd);
            LOG_E("open_audio_param_file system:%s",cmd);
            snprintf(cmd,sizeof(cmd),"rm -rf %s",audio_param_file_table[profile].bin_file);
            system(cmd);
            LOG_E("open_audio_param_file system:%s",cmd);
            load_xml_handle(&(audio_param->xml), audio_param_file_table[profile].src_file, audio_param_file_table[profile].first_name);
            if(NULL==audio_param->xml.param_root) {
                LOG_E("open_audio_param_file src file can not get root:%s\n", audio_param_file_table[profile].first_name);
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

static int init_sprd_audio_param_from_xml(AUDIO_PARAM_T *param,int profile)
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

    LOG_D("%s %d",__func__,__LINE__);
    ret=malloc_sprd_audio_param(param,profile);
    if(ret>0){
        ret=_init_sprd_audio_param_from_xml(audio_param->xml.param_root,true,(unsigned char*)
                    audio_param->data,&offset,0);
        doc = (TiXmlDocument *)audio_param->xml.param_doc;

        if(access(audio_param_file_table[profile].tunning_file, R_OK) != 0){
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

int reload_sprd_audio_pga_param(AUDIO_PARAM_T *param){
    int profile=SND_AUDIO_PARAM_PGA_PROFILE;

    param->param[SND_AUDIO_PARAM_PGA_PROFILE].data=NULL;
    LOG_I("reload_sprd_audio_pga_param dev_ctl:%p",param->dev_ctl);
    free_sprd_audio_pga_param(&param->dev_ctl->dg_gain);
    init_sprd_xml(&param->dev_ctl->dg_gain,&(param->param[profile].xml),audio_param_file_table[profile].tunning_file,
        audio_param_file_table[profile].src_file,audio_param_file_table[profile].first_name,load_sprd_audio_pga_param);
    return 0;
}

int init_sprd_audio_param(AUDIO_PARAM_T  *audio_param, int force)
{
    int ret = 0;
    int profile=0;
    LOG_D("init_sprd_audio_param1 enter");
    if(audio_param== NULL) {
        LOG_D("init_sprd_audio_param error");
        return -1;
    }

    for(profile=0;profile<SND_AUDIO_PARAM_PROFILE_MAX;profile++){
        if(SND_AUDIO_PARAM_PGA_PROFILE ==profile){
            if(force){
                free_sprd_audio_pga_param(&audio_param->dev_ctl->dg_gain);
                audio_param->param[profile].data=NULL;
                init_sprd_xml(&audio_param->dev_ctl->dg_gain,&(audio_param->param[profile].xml),audio_param_file_table[profile].tunning_file,
                    audio_param_file_table[profile].src_file,audio_param_file_table[profile].first_name,load_sprd_audio_pga_param);
            }else{
                init_sprd_xml(&audio_param->dev_ctl->dg_gain,NULL,audio_param_file_table[profile].tunning_file,
                    audio_param_file_table[profile].src_file,audio_param_file_table[profile].first_name,load_sprd_audio_pga_param);
            }
        }else if(SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE ==profile){
            LOG_I("init_sprd_audio_param audio_process");
            if(force){
                init_sprd_xml(&audio_param->param[profile],&(audio_param->param[profile].xml),AUDIO_PROCESS_XML_TUNNING_PATH,AUDIO_PROCESS_PATH,AUDIO_PROCESS_FIRST_NAME,load_sprd_audio_process_param);
            }else{
                init_sprd_xml(&audio_param->param[profile],NULL,AUDIO_PROCESS_XML_TUNNING_PATH,AUDIO_PROCESS_PATH,AUDIO_PROCESS_FIRST_NAME,load_sprd_audio_process_param);
            }
        }else{
            if((0 == force) && (access(audio_param_file_table[profile].bin_file, R_OK) == 0)) {
                LOG_D("init_sprd_audio_param %s is exist\n", audio_param_file_table[profile].bin_file);
            } else {
                ret = init_sprd_audio_param_from_xml(audio_param,profile);
            }
        }
    }
    LOG_D("init_sprd_audio_param1 exit");
    return 0;
}

int clear_audio_param(AUDIO_PARAM_T  *audio_param){
    LOG_I("clear_audio_param%x",audio_param->current_param);
    pthread_mutex_lock(&audio_param->audio_param_lock);
    audio_param->current_param=0;
    pthread_mutex_unlock(&audio_param->audio_param_lock);
    return 0;
}

int set_sprd_codec_param(struct tiny_audio_device *adev,
                         struct sprd_code_param_t *param){
    int ret = 0;

    int vol_index=0;
    struct sprd_codec_mixer_t *codec = &(adev->dev_ctl->codec);

    LOG_I("codec param:%x  pa:%x hp_pa:%x %x %x %p",
        param->mic_boost, param->inter_pa_config, param->inter_hp_pa_config,param->ear_playback_volume[vol_index],param->spkl_playback_volume[vol_index],param);

    if(adev->stream_status & ((1<<AUDIO_HW_APP_CALL) |(1<<AUDIO_HW_APP_VOIP)|(1<<AUDIO_HW_APP_VOIP))){
            vol_index=adev->dev_ctl->voice_volume;
    }

    switch(adev->out_devices) {
    case AUDIO_DEVICE_OUT_EARPIECE:
        if(NULL != codec->ear_playback_volume) {
            ret = mixer_ctl_set_value(codec->ear_playback_volume, 0,
                                      param->ear_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_codec_param set ear_playback_volume failed");
            }else{
                LOG_E("set_sprd_codec_param set ear_playback_volume :0x%x",param->ear_playback_volume[vol_index]);
            }
        }

        if(NULL != codec->dacl_playback_volume) {
            ret = mixer_ctl_set_value(codec->dacl_playback_volume, 0,
                                      param->dacl_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_codec_param set dacl_playback_volume failed");
            }else{
                LOG_E("set_sprd_codec_param set dacl_playback_volume :0x%x",param->dacl_playback_volume[vol_index]);
            }
        }

        break;
    case AUDIO_DEVICE_OUT_SPEAKER:
        if(NULL != codec->dacs_playback_volume) {
            ret = mixer_ctl_set_value(codec->dacs_playback_volume, 0,
                                      param->dacs_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_codec_param set dacs_playback_volume failed");
            }else{
                LOG_E("set_sprd_codec_param set dacs_playback_volume :0x%x",param->dacs_playback_volume[vol_index]);
            }
        }

        if(NULL != codec->spkl_playback_volume) {
            ret = mixer_ctl_set_value(codec->spkl_playback_volume, 0,
                                      param->spkl_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_codec_param set spkl_playback_volume failed");
            }else{
                LOG_E("set_sprd_codec_param set spkl_playback_volume :0x%x",param->spkl_playback_volume[vol_index]);
            }
        }

        if(NULL != codec->spkr_playback_volume) {
            ret = mixer_ctl_set_value(codec->spkr_playback_volume, 0,
                                      param->spkr_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_codec_param set spkr_playback_volume failed");
            }else{
                LOG_E("set_sprd_codec_param set spkr_playback_volume :0x%x",param->spkr_playback_volume[vol_index]);
            }
        }

        if(NULL != codec->inner_pa) {
            ret = mixer_ctl_set_value(codec->inner_pa, 0,
                                      param->inter_pa_config);
            if (ret != 0) {
                LOG_E("set_sprd_codec_param set inner_pa failed");
            }else{
                LOG_E("set_sprd_codec_param set inner_pa :0x%x",param->inter_pa_config);
            }
        }

        break;
    case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
    case AUDIO_DEVICE_OUT_WIRED_HEADSET:
        if(NULL != codec->dacs_playback_volume) {
            ret = mixer_ctl_set_value(codec->dacl_playback_volume, 0,
                                      param->dacl_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_codec_param set dacl_playback_volume failed");
            }else{
                LOG_E("set_sprd_codec_param set dacl_playback_volume :0x%x",param->dacl_playback_volume[vol_index]);
            }
        }

        if(NULL != codec->dacs_playback_volume) {
            ret = mixer_ctl_set_value(codec->dacr_playback_volume, 0,
                                      param->dacr_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_codec_param set dacr_playback_volume failed");
            }else{
                LOG_E("set_sprd_codec_param set dacr_playback_volume :0x%x",param->dacr_playback_volume[vol_index]);
            }
        }

        if(NULL != codec->adcl_capture_volume) {
            ret = mixer_ctl_set_value(codec->hpl_playback_volume, 0,
                                      param->hpl_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_codec_param set hpl_playback_volume failed");
            }else{
                LOG_E("set_sprd_codec_param set hpl_playback_volume :0x%x",param->hpl_playback_volume[vol_index]);
            }
        }
        if(NULL != codec->adcl_capture_volume) {
            ret = mixer_ctl_set_value(codec->hpr_playback_volume, 0,
                                      param->hpr_playback_volume[vol_index]);
            if (ret != 0) {
                LOG_E("set_sprd_codec_param set hpr_playback_volume failed");
            }else{
                LOG_E("set_sprd_codec_param set hpr_playback_volume :0x%x",param->hpr_playback_volume[vol_index]);
            }
        }

        if(NULL != codec->hp_inner_pa) {
            ret = mixer_ctl_set_value(codec->hp_inner_pa, 0,
                                      param->inter_hp_pa_config);
            if (ret != 0) {
                LOG_E("set_sprd_codec_param set hp_inner_pa failed");
            }else{
                LOG_E("set_sprd_codec_param set hp_inner_pa :0x%x",param->inter_hp_pa_config);
            }
        }
        break;
    default:
        break;
    }

    if(adev->stream_status & ((1<<AUDIO_HW_APP_CALL) |(1<<AUDIO_HW_APP_VOIP)|(1<<AUDIO_HW_APP_VOIP)|(1<<AUDIO_HW_APP_NORMAL_RECORD))) {
        if((adev->out_devices == AUDIO_DEVICE_OUT_EARPIECE)
           || (adev->out_devices == AUDIO_DEVICE_OUT_SPEAKER)
           || (adev->out_devices == AUDIO_DEVICE_OUT_WIRED_HEADPHONE)) {
            if(codec->switch_route & (0x01 << 0)) {
                if(NULL != codec->mic_boost) {
                    ret = mixer_ctl_set_value(codec->mic_boost, 0, param->mic_boost);
                    if (ret != 0) {
                        LOG_E("set_sprd_codec_param set mic_boost failed");
                    }else{
                        LOG_E("set_sprd_codec_param set mic_boost :0x%x",param->mic_boost);
                    }
                }
            }

            if(codec->switch_route & (0x01 << 1)) {
                if(NULL != codec->auxmic_boost) {
                    ret = mixer_ctl_set_value(codec->auxmic_boost, 0, param->mic_boost);
                    if (ret != 0) {
                        LOG_E("set_sprd_codec_param set auxmic_boost failed");
                    }else{
                        LOG_E("set_sprd_codec_param set auxmic_boost :0x%x",param->mic_boost);
                    }
                }
            }
        } else {
            if(NULL != codec->headmic_boost) {
                ret = mixer_ctl_set_value(codec->headmic_boost, 0, param->mic_boost);
                if (ret != 0) {
                    LOG_E("set_sprd_codec_param set headmic_boost failed");
                }else{
                    LOG_E("set_sprd_codec_param set headmic_boost :0x%x",param->mic_boost);
                }
            }
        }


        if(NULL != codec->adcl_capture_volume) {
            ret = mixer_ctl_set_value(codec->adcl_capture_volume, 0,
                                      param->adcl_capture_volume);
                if (ret != 0) {
                    LOG_E("set_sprd_codec_param set adcl_capture_volume failed");
                }else{
                    LOG_E("set_sprd_codec_param set adcl_capture_volume :0x%x",param->adcl_capture_volume);
                }
        }

        if(NULL != codec->adcr_capture_volume) {
            ret = mixer_ctl_set_value(codec->adcr_capture_volume, 0,
                                      param->adcr_capture_volume);
            if (ret != 0) {
                LOG_E("set_sprd_codec_param set adcr_capture_volume failed");
            }else{
                LOG_E("set_sprd_codec_param set adcr_capture_volume :0x%x",param->adcr_capture_volume);
            }
        }

    }

    return 0;
}

int select_audio_param(struct tiny_audio_device *adev,bool force){
    int param=0;
    int param_select=-1;
    int i=0;
    int dai_id=0;
    int ret=-1;
    int pre_param=0;
    char name[128]={0};
    bool bt_mode=false;
    AUDIOVBCEQ_PARAM_T *codec=NULL;

    if(adev->stream_status & ((1<<AUDIO_HW_APP_CALL) |(1<<AUDIO_HW_APP_VOIP)) ){
        strcat((char *)name ,"Audio");
        switch(adev->out_devices){
            case AUDIO_DEVICE_OUT_EARPIECE:
                strcat((char *)name ,"\\Handset");
                break;
            case AUDIO_DEVICE_OUT_SPEAKER:
                strcat((char *)name ,"\\Handsfree");
                break;
            case AUDIO_DEVICE_OUT_WIRED_HEADSET:
                strcat((char *)name ,"\\Headset4P");
                break;
            case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
                strcat((char *)name ,"\\Headset3P");
                break;
            case AUDIO_DEVICE_OUT_BLUETOOTH_SCO:
            case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
                bt_mode=true;
                if(adev->bt_infor.samplerate==16000){
                    if(adev->bt_infor.bluetooth_nrec==true)
                        strcat((char *)name ,"\\BTHSNRECD16K");
                    else
                        strcat((char *)name ,"\\BTHSD16K");
                }else{
                    if(adev->bt_infor.bluetooth_nrec==true)
                        strcat((char *)name ,"\\BTHSNRECD8K");
                    else
                        strcat((char *)name ,"\\BTHSD8K");
                }
                break;
            default:
                strcat((char *)name ,"\\None");
                break;
        }

        if(true==adev->voip_start){
            strcat((char *)name ,"\\VOIP");
            dai_id |=1<<DAI_ID_VOIP;
        }else{
            dai_id |=1<<DAI_ID_VOICE;
            switch(adev->net_mode){
                case AUDIO_NET_GSM:
                    strcat((char *)name ,"\\GSM");
                    break;
                case AUDIO_NET_TDMA:
                    strcat((char *)name ,"\\TDMA");
                    break;
                case AUDIO_NET_WCDMA_NB:
                    if(true!=bt_mode){
                        strcat((char *)name ,"\\WCDMA_NB");
                    }else{
                        strcat((char *)name ,"\\WCDMA");
                    }
                    break;
                case AUDIO_NET_WCDMA_WB:
                    if(true!=bt_mode){
                        strcat((char *)name ,"\\WCDMA_WB");
                    }else{
                        strcat((char *)name ,"\\WCDMA");
                    }
                    break;
                case AUDIO_NET_VOLTE_NB:
                    if(true!=bt_mode){
                        strcat((char *)name ,"\\VOLTE_NB");
                    }else{
                        strcat((char *)name ,"\\VOLTE");
                    }
                    break;
                case AUDIO_NET_VOLTE_WB:
                    if(true!=bt_mode){
                        strcat((char *)name ,"\\VOLTE_WB");
                    }else{
                        strcat((char *)name ,"\\VOLTE");
                    }
                    break;
                case AUDIO_NET_LOOP:
                    strcat((char *)name ,"\\Loopback");
                    break;
                default:
                    strcat((char *)name ,"\\None");
                    break;
            }
        }

    }else{
        strcat((char *)name ,"Music");
        switch(adev->out_devices){
            case AUDIO_DEVICE_OUT_EARPIECE:
                strcat((char *)name ,"\\Handset");
                break;
            case AUDIO_DEVICE_OUT_SPEAKER:
                strcat((char *)name ,"\\Handsfree");
                break;
            case AUDIO_DEVICE_OUT_WIRED_HEADSET:
                strcat((char *)name ,"\\Headfree");
                break;
            case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
                strcat((char *)name ,"\\Headset");
                break;
            default:
                strcat((char *)name ,"\\None");
                break;
        }

        if(adev->stream_status & (1<<AUDIO_HW_APP_FM) ){
                dai_id |=DAI_ID_FM;
                strcat((char *)name ,"\\FM");
        }else  if(adev->stream_status & (1<<AUDIO_HW_APP_NORMAL_RECORD)){
                dai_id |=DAI_ID_NORMAL_OUTDSP_CAPTURE;
                strcat((char *)name ,"\\Record");
        }else  if(adev->stream_status & (1<<AUDIO_HW_APP_OFFLOAD) ){
                dai_id |=DAI_ID_NORMAL_OUTDSP_PLAYBACK;
                strcat((char *)name ,"\\Playback");
        }else{
                strcat((char *)name ,"\\None");
        }
    }
    param_select=get_audio_param_id(name);
    if(param_select<0)
        goto exit;

    pthread_mutex_lock(&adev->audio_param.audio_param_lock);
    pre_param=adev->audio_param.current_param;
    pthread_mutex_unlock(&adev->audio_param.audio_param_lock);

    LOG_I("select_audio_param pre_param:0x%x select:0x%x name:%s stream_status:0x%x",pre_param,param_select,name,adev->stream_status);

    param |=(param_select << 24);// audio_param offset;
    param |=(param_select << 16);// mode

    param |=dai_id;
    if((adev->out_devices != AUDIO_DEVICE_OUT_BLUETOOTH_SCO) && (adev->out_devices != AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET)) {
        if(AUD_REALTEK_CODEC_TYPE == adev->dev_ctl->codec_type){
            ret |=apply_audio_profile_param_firmware(adev->audio_param.select_mixer[SND_AUDIO_PARAM_CODEC_PROFILE],param);
        }else{
            codec=&adev->audio_param.param[SND_AUDIO_PARAM_CODEC_PROFILE];
            LOG_I("select_audio_param codec:%p %p",codec,codec->data);
            if((codec->data!=NULL) && (codec->param_struct_size)){
                LOG_D("codec param size:%d num_mode:%d param_select:%d :codec->data:%p",codec->param_struct_size,codec->num_mode,param_select,codec->data);
                ret |=set_sprd_codec_param(adev,(struct sprd_code_param_t *)(codec->data+(codec->param_struct_size*param_select)));
            }
        }
    }

    if((pre_param==param) && (false==force)){
        LOG_I("select_audio_param the same audio param:0x%x",param);
        goto exit;
    }
    if(adev->stream_status & ((1<<AUDIO_HW_APP_CALL) |(1<<AUDIO_HW_APP_VOIP)|(1<<AUDIO_HW_APP_DSP_LOOP))) {
        ret |=apply_audio_profile_param_firmware(adev->audio_param.select_mixer[SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP],param);
        ret |=apply_audio_profile_param_firmware(adev->audio_param.select_mixer[SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE],param);
        ret |=apply_audio_profile_param_firmware(adev->audio_param.select_mixer[SND_AUDIO_PARAM_NXP_PROFILE],param);
        set_vdg_gain(&adev->dev_ctl->dg_gain,(param>>16)&0xff,adev->dev_ctl->voice_volume);
    }else if(adev->stream_status & (1<<AUDIO_HW_APP_OFFLOAD)){
        ret |=apply_audio_profile_param_firmware(adev->audio_param.select_mixer[SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP],param);
        set_vdg_gain(&adev->dev_ctl->dg_gain,(param>>16)&0xff,0);
    }else {
        goto exit;
    }

    pthread_mutex_lock(&adev->audio_param.audio_param_lock);
    adev->audio_param.current_param=param;
    pthread_mutex_unlock(&adev->audio_param.audio_param_lock);

exit:
    return param;
}

static int load_sprd_audio_pga_param(void *gain,param_group_t ele, char *path){
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

static int free_sprd_audio_pga_param(struct device_usecase_gain *use_gain){
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



static int _load_sprd_audio_process_param(TiXmlElement *param_group, char *data,bool root,int *offset){
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
            size+=_load_sprd_audio_process_param(group->FirstChildElement(), data, false,offset);
        }else{
            visible = group->Attribute(VISIBLE);

            if((NULL != visible) && (0 == strncmp(visible,TRUE_STRING,true_string_size))){
                type = group->Attribute(TYPE);
                if(type==NULL){
                    LOG_E("_load_sprd_audio_process_param %s type err",param_group->Value());
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

                values=strtoul(val,NULL,16);
                memcpy(data+*offset,&values,values_size);
                group->SetAttribute(ID,size);
                size+=values_size;

                sprintf(val_str,"0x%x",*offset);
                group->SetAttribute(ID, val_str);
                *offset+=values_size;
            }
        }

        if (root) {
            break;
        }
        group = group->NextSiblingElement();
    }
exit:
    return size;
}

static int load_sprd_audio_process_param(void *audio_param,param_group_t ele, char *path){
    char * process_param=NULL;
    AUDIOVBCEQ_PARAM_T *param=(AUDIOVBCEQ_PARAM_T *)audio_param;
    TiXmlElement *param_group=(TiXmlElement *)ele;
    int id=0;
    const char  *tmp=NULL;
    const char *val;
    tmp=param_group->Value();
    int ret=-1;
    int offset=0;

    if(NULL==tmp){
        LOG_E("load_sprd_audio_process_param err");
        return -1;
    }

    id=get_audio_param_id(path);
    if(id<0){
        LOG_E("load_sprd_audio_process_param:%s failed",path);
        return id;
    }

    process_param=(char * )(param->data);
    if(NULL==process_param){
        process_param=(char * )malloc(sizeof(struct audio_record_proc_param)*2);
        if(NULL==process_param){
               LOG_E("load_sprd_audio_process_param malloc failed");
               param->data=NULL;
               return -1;
        }
        param->param_struct_size=sizeof(struct audio_record_proc_param);
        param->num_mode=2;
        param->data=process_param;
        LOG_I("load_sprd_audio_process_param data:%p",process_param);
    }

   if(PROFILE_MODE_MUSIC_Handsfree_Record == id){
        offset=sizeof(struct audio_record_proc_param)*AUDIO_RECORD_MODE_HANDSFREE;
   }else if(PROFILE_MODE_MUSIC_Headset_Record == id){
        offset=sizeof(struct audio_record_proc_param)*AUDIO_RECORD_MODE_HEADSET;
   }else{
        LOG_E("load_sprd_audio_process_param %s not support",tmp);
        return -1;
   }

    process_param=(char * )(param->data+offset);
    ret=_load_sprd_audio_process_param(param_group,(char *)process_param,true,&offset);
    if(ret!=param->param_struct_size){
        LOG_E("load_sprd_audio_process_param param size %d  err,struct_size:%d mode:%d",ret,
            param->param_struct_size,
            param->num_mode);
    }

    LOG_I("load_sprd_audio_process_param return:%d",ret);
    return 0;
}

static int init_sprd_xml(void * load_param_func_res,
                   struct xml_handle *xml,
                   const char *tunning_param_path,
                   const char *param_path,
                   const char *first_name,
                   load_sprd_audio_param load_param_func){
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
        if (is_file_exist(tunning_param_path)) {
            parampath = tunning_param_path;
        } else {
            parampath = param_path;
        }
        ret = load_xml_handle(xmlhandle, parampath, first_name);
        if (ret != 0) {
            LOG_E("load xml handle failed (%s)", parampath);
            if(tunning_param_path == parampath){
                parampath=param_path;
                ret = load_xml_handle(xmlhandle, parampath, first_name);
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
                    load_param_func(load_param_func_res,group,path);
                    memset(path,0,sizeof(path));
                }
                LOG_D("init_sprd_xml ele_3 end:%s :%p",ele_3_name,ele_3);
                ele_3 = ele_3->NextSiblingElement();
            }

            LOG_D("init_sprd_xml ele_2 end:%s :%p",ele_2_name,ele_2);
            ele_2 = ele_2->NextSiblingElement();
        }
        LOG_D("init_sprd_xml ele_1 end:%s :%p",ele_1_name);
        ele_1 = ele_1->NextSiblingElement();
    }

    if(xml==NULL){
        release_xml_handle(xmlhandle);
    }else{
        param_doc = (TiXmlDocument *)xmlhandle->param_doc;
        if((NULL != param_doc)&& (parampath == param_path)&& (tunning_param_path!=NULL)){
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

int init_audio_param( struct tiny_audio_device *adev)
{
    int ret = 0;
    AUDIO_PARAM_T  *audio_param=&(adev->audio_param);
    memset(audio_param,0,sizeof(AUDIO_PARAM_T));
    audio_param->agdsp_ctl=&adev->agdsp_ctl;
    audio_param->dev_ctl=adev->dev_ctl;
    audio_param->debug=&adev->debug.tunning;

    /* dsp vbc */
    audio_param->update_mixer[SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP]=
        mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_DSP_PROFILE_UPDATE);

    audio_param->select_mixer[SND_AUDIO_PARAM_DSP_VBC_PROFILE_DSP]=
        mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_DSP_PROFILE_SELECT);

    /* audio structure  */
    audio_param->update_mixer[SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE]=
        mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_EQ_PROFILE_UPDATE);

    audio_param->select_mixer[SND_AUDIO_PARAM_AUDIO_STRUCTURE_PROFILE]=
        mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_EQ_PROFILE_SELECT);

    /* NXP  */
    audio_param->update_mixer[SND_AUDIO_PARAM_NXP_PROFILE]=
        mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_NXP_PROFILE_UPDATE);

    audio_param->select_mixer[SND_AUDIO_PARAM_NXP_PROFILE]=
        mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_NXP_PROFILE_SELECT);

    /* codec */
    if(adev->dev_ctl->codec_type==AUD_REALTEK_CODEC_TYPE){
        audio_param->update_mixer[AUD_REALTEK_CODEC_TYPE]=
            mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_CODEC_PROFILE_UPDATE);

        audio_param->select_mixer[AUD_REALTEK_CODEC_TYPE]=
            mixer_get_ctl_by_name(adev->dev_ctl->mixer, VBC_CODEC_PROFILE_SELECT);
    }

    return init_sprd_audio_param(audio_param,0);
}

int upload_audio_profile_param_firmware(AUDIO_PARAM_T * audio_param,int profile){
    struct mixer_ctl *mixer;
    struct mixer_ctl *eq_update=NULL;
    int ret=-1;

    mixer = audio_param->update_mixer[profile];
    if (!mixer) {
        LOG_E("upload_audio_profile_param_firmware Failed to open mixer");
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
    }
    return ret;
}

static int apply_audio_profile_param_firmware(struct mixer_ctl *eq_select,int id){
    struct mixer *mixer;
    int ret=-1;

    if(NULL !=eq_select){
        ret = mixer_ctl_set_value(eq_select, 0, id);
        if (ret != 0) {
            LOG_E("apply_audio_profile_param_firmware Failed \n");
        }else{
            LOG_I("apply_audio_profile_param_firmware ret:%d",ret);
        }
    }else{
        LOG_E("apply_audio_profile_param_firmware failed");
    }
    return 0;
}

#ifdef __cplusplus
}
#endif

