#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>

#include "audio_xml_utils.h"
#include "audio_param.h"
#include <sys/stat.h>


#include "fcntl.h"
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

#define SHELL_DBG(...)   fprintf(stdout, ##__VA_ARGS__)

#define LOG_D  SHELL_DBG
#define LOG_E  SHELL_DBG
int log_level = 3;

int load_xml_handle(struct xml_handle *xmlhandle, const char *xmlpath,
                    const char *first_name)
{
    xmlhandle->param_doc = XML_open_param(xmlpath);
    if (xmlhandle->param_doc == NULL) {
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

#if 0
int _load_audio_param_from_bin(AUDIOVBCEQ_PARAM_T *param,
                               TiXmlElement *param_group, bool root)
{
    const char *tmp = NULL;
    int id = -1, offset = -1, bits = -1, val = 0, values = 0;
    const char *visible;
    int pre_val = 0;

    TiXmlElement *group = (TiXmlElement *)param_group;
    TiXmlElement *parent_group = NULL;
    int size = 0;
    int mask = 0;
    int i = 0;
    char val_str[32] = {0};

    while(NULL != group) {
        // SHELL_DBG("%s\n",group->Value());
        tmp = group->Attribute(VALUE);
        if (tmp == NULL) {
            // SHELL_DBG("\n");
            _load_audio_param_from_bin(param, group->FirstChildElement(), false);
        } else {
            //        SHELL_DBG("%s ",group->Values());
            //SHELL_DBG("%s %d\n",__func__,__LINE__);

            tmp = group->Attribute(OFFSETS);
            if(NULL == tmp) {
                SHELL_DBG("%s %d\n", __func__, __LINE__);
                return -2;
            }
            offset = strtoul(tmp, NULL, 16);

            tmp = group->Attribute(BITS);
            if(NULL == tmp) {
                return -3;
            }
            bits = strtoul(tmp, NULL, 16);

            tmp = group->Attribute(ID);

            if(NULL == tmp) {
                parent_group = (TiXmlElement *)group->Parent();
                if(NULL == parent_group) {
                    SHELL_DBG("set_ele_value find parent_group failed\n");
                    return -2;
                }

                tmp = parent_group->Attribute(TYPE);
                if(NULL == tmp) {
                    SHELL_DBG("%s %d\n", __func__, __LINE__);
                    return -4;
                }

                tmp = parent_group->Attribute(ID);
                if(NULL == tmp) {
                    SHELL_DBG("%s %d\n", __func__, __LINE__);
                    return -1;
                }
                id = strtoul(tmp, NULL, 16);
            } else {
                tmp = group->Attribute(TYPE);
                if(NULL == tmp) {
                    SHELL_DBG("%s %d\n", __func__, __LINE__);
                    return -4;
                }

                tmp = group->Attribute(ID);
                if(NULL == tmp) {
                    SHELL_DBG("%s %d\n", __func__, __LINE__);
                    return -1;
                }
                id = strtoul(tmp, NULL, 16);
            }

            if(strcmp(tmp, U32) == 0) {
                size = sizeof(int32_t);
            } else if(strcmp(tmp, U16) == 0) {
                size = sizeof(int16_t);
            } else if(strcmp(tmp, U8) == 0) {
                size = sizeof(uint8_t);
            }

            values = 0;

            //SHELL_DBG("%s bits:%d offset:%d\n",group->Value(),bits,offset);
            memcpy(&values, param->data + offset, size);



            for(i = 0; i < bits; i++) {
                mask |= 1 << i;
            }

            val = (values >> offset) & (mask);


            tmp = group->Attribute(VALUE);
            if(NULL == tmp) {
                return -3;
            }
            pre_val = strtoul(tmp, NULL, 16);

            if(pre_val != val) {
                SHELL_DBG("%s SetAttribute :0x%x to 0x%x\n", group->Value(), pre_val, val);
            }

            sprintf(val_str, "0x%x", val);

            group->SetAttribute(VALUE, val_str);

        }

        if (root) {
            SHELL_DBG("%s %d\n", __func__, __LINE__);
            break;
        }

        group = group->NextSiblingElement();
    }
    return 0;
}
#else
int _load_audio_param_from_bin(AUDIOVBCEQ_PARAM_T *param,
                               TiXmlElement *param_group, bool root)
{
    const char *tmp = NULL;
    int id = -1, offset = -1, bits = -1, val = 0, values = 0;
    const char *visible;
    int pre_val = 0;

    TiXmlElement *group = (TiXmlElement *)param_group;
    TiXmlElement *parent_group = NULL;
    TiXmlElement *type_ele = NULL;
    int size = 0;
    int mask = 0;
    int i = 0;
    char val_str[32] = {0};

    while(NULL != group) {
        tmp = group->Attribute(VALUE);
        if (tmp == NULL) {
            _load_audio_param_from_bin(param, group->FirstChildElement(), false);
        } else {

            tmp = group->Attribute(TYPE);
            if(NULL == tmp) {

                parent_group = (TiXmlElement *)group->Parent();
                if(NULL == parent_group) {
                    SHELL_DBG("set_ele_value find parent_group failed\n");
                    return -2;
                }

                type_ele = parent_group;
            } else {
                type_ele = group;
            }

            tmp = type_ele->Attribute(TYPE);
            if(strcmp(tmp, U32) == 0) {
                size = sizeof(int32_t);
            } else if(strcmp(tmp, U16) == 0) {
                size = sizeof(int16_t);
            } else if(strcmp(tmp, U8) == 0) {
                size = sizeof(uint8_t);
            }

            tmp = type_ele->Attribute(ID);
            if(NULL == tmp) {
                SHELL_DBG("%s %d\n", __func__, __LINE__);
                return -1;
            }
            id = string_to_value(tmp);


            tmp = group->Attribute(OFFSETS);
            if(NULL == tmp) {
                SHELL_DBG("%s %d\n", __func__, __LINE__);
                return -2;
            }
            offset = string_to_value(tmp);

            tmp = group->Attribute(BITS);
            if(NULL == tmp) {
                SHELL_DBG("%s %d\n", __func__, __LINE__);
                return -3;
            }
            bits = string_to_value(tmp);

            values = 0;

            memcpy(&values, param->data + offset, size);

            for(i = 0; i < bits; i++) {
                mask |= 1 << i;
            }

            val = (values >> offset) & (mask);


            tmp = group->Attribute(VALUE);
            if(NULL == tmp) {
                SHELL_DBG("%s %d\n", __func__, __LINE__);
                return -3;
            }
            pre_val = string_to_value(tmp);

            if(pre_val != val) {
                SHELL_DBG("%s SetAttribute :0x%x to 0x%x values:0x%x bits:%x offset:%x\n",
                          group->Value(), pre_val, val,
                          values, bits, offset);
            }

            sprintf(val_str, "0x%x", val);

            group->SetAttribute(VALUE, val_str);

        }

        if (root) {
            SHELL_DBG("%s %d\n", __func__, __LINE__);
            break;
        }

        group = group->NextSiblingElement();
    }
    return 0;
}

#endif

TiXmlElement *private_get_param(param_group_t group, const char *param_name);


static int _clone_audio_param(TiXmlElement *group, bool root,
                              TiXmlElement *src_group)
{
    const char *tmp = NULL;
    int path_len = 0;
    TiXmlElement *Element = (TiXmlElement *)group;
    TiXmlElement *parent_group = NULL;

    while(NULL != Element) {
        tmp = Element->Attribute("mode");
        if(tmp != NULL) {
            _clone_audio_param(Element->FirstChildElement(), false, src_group);
        } else {

            if(src_group != Element) {
                parent_group = (TiXmlElement *)Element->Parent();
                if(NULL == parent_group) {
                    SHELL_DBG("set_ele_value find parent_group failed\n");
                    return -4;
                }

                parent_group->ReplaceChild(Element, *src_group);
            }
        }

        if(root) {
            break;
        }
        Element = Element->NextSiblingElement();
    }

    return 0;
}

int clone_audio_param(const char *src_xml, const char *dst_xml,
                      const char *src_name , const char *dst_name)
{

    AUDIOVBCEQ_PARAM_T param;
    TiXmlElement *group = NULL;

    TiXmlElement *src_group = NULL;
    TiXmlElement *dst_group = NULL;
    TiXmlElement *parent_group = NULL;

    TiXmlElement *root = NULL;

    const char *tmp = NULL;

    int ret = 0;

    memset(&param, 0, sizeof(AUDIOVBCEQ_PARAM_T));

    ret = load_xml_handle(&param.xml, src_xml, NULL);
    if (ret != 0) {
        SHELL_DBG("load xml handle failed (%s)", src_xml);
        return ret;
    }

    group = (TiXmlElement *)param.xml.param_root;
    if(NULL == group) {
        SHELL_DBG("%s %d\n", __func__, __LINE__);
        return -1;
    }

    src_group = (TiXmlElement *)private_get_param(group, src_name);
    if(NULL == src_group) {
        SHELL_DBG("%s %d\n", __func__, __LINE__);
        return -2;
    }

    if(0 == strncmp(dst_name, "all", strlen("all"))) {
        _clone_audio_param((TiXmlElement *)param.xml.param_root, true, src_group);
    } else {
        dst_group = (TiXmlElement *)private_get_param(group, dst_name);
        if(NULL == dst_group) {
            SHELL_DBG("%s %d\n", __func__, __LINE__);
            return -3;
        }

        parent_group = (TiXmlElement *)dst_group->Parent();
        if(NULL == parent_group) {
            SHELL_DBG("set_ele_value find parent_group failed\n");
            return -4;
        }

        parent_group->ReplaceChild(dst_group, *src_group);
    }

    SHELL_DBG("%s %d\n", __func__, __LINE__);
    if(NULL != param.xml.param_doc) {

        root = (TiXmlElement *)param.xml.param_root;
        tmp = root->Attribute(STRUCT_SIZE);
        if(tmp != NULL) {
            root->RemoveAttribute(STRUCT_SIZE);
        }

        tmp = root->Attribute(NUM_MODE);
        if(tmp != NULL) {
            root->RemoveAttribute(NUM_MODE);
        }

        XML_save_param(param.xml.param_doc,
                       dst_xml);
    }
    return 0;
}


int Insert_audio_param(const char *src_xml, const char *dst_xml,
                       const char *src_name , const char *dst_name)
{

    AUDIOVBCEQ_PARAM_T param;
    TiXmlElement *group = NULL;

    TiXmlElement *src_group = NULL;
    TiXmlElement *dst_group = NULL;
    TiXmlElement *parent_group = NULL;

    TiXmlElement *root = NULL;

    const char *tmp = NULL;

    int ret = 0;

    memset(&param, 0, sizeof(AUDIOVBCEQ_PARAM_T));

    ret = load_xml_handle(&param.xml, src_xml, NULL);
    if (ret != 0) {
        SHELL_DBG("load xml handle failed (%s)", src_xml);
        return ret;
    }

    group = (TiXmlElement *)param.xml.param_root;
    if(NULL == group) {
        SHELL_DBG("%s %d\n", __func__, __LINE__);
        return -1;
    }

    src_group = (TiXmlElement *)private_get_param(group, src_name);
    if(NULL == src_group) {
        SHELL_DBG("%s %d\n", __func__, __LINE__);
        return -2;
    }

    dst_group = (TiXmlElement *)private_get_param(group, dst_name);
    if(NULL == dst_group) {
        SHELL_DBG("%s %d\n", __func__, __LINE__);
        return -3;
    }

    parent_group = (TiXmlElement *)dst_group->Parent();
    if(NULL == parent_group) {
        SHELL_DBG("set_ele_value find parent_group failed\n");
        return -4;
    }

    //SprdReplaceChild(dst_group,src_group);
    parent_group->InsertAfterChild(dst_group, *src_group);

    SHELL_DBG("%s %d\n", __func__, __LINE__);
    if(NULL != param.xml.param_doc) {

        root = (TiXmlElement *)param.xml.param_root;
        tmp = root->Attribute(STRUCT_SIZE);
        if(tmp != NULL) {
            root->RemoveAttribute(STRUCT_SIZE);
        }

        tmp = root->Attribute(NUM_MODE);
        if(tmp != NULL) {
            root->RemoveAttribute(NUM_MODE);
        }

        XML_save_param(param.xml.param_doc,
                       dst_xml);
    }
    return 0;
}

static int _Division_audio_param(TiXmlElement *group, bool root, char *path,
                                 const char *dst_dir)
{
    const char *tmp = NULL;
    int path_len = 0;
    TiXmlElement *Element = (TiXmlElement *)group;

    while(NULL != Element) {
        tmp = Element->Attribute(ID);
        if(tmp != NULL) {
            strcat(path, "_");
            strcat(path, Element->Value());
            _Division_audio_param(Element->FirstChildElement(), false, path, dst_dir);
        } else {
            TiXmlDocument doc;
            TiXmlDeclaration *dec = new TiXmlDeclaration("1.0", "gb2312", "");
            doc.LinkEndChild(dec);
            doc.LinkEndChild(Element);
            strcat(path, ".xml");
            doc.SaveFile(path);
            path_len = strlen(path) + 1;
            memset(path, 0, path_len);
            strcat(path, dst_dir);
        }

        if(root) {
            break;
        }
        Element = Element->NextSiblingElement();
    }

    return 0;
}

int Division_audio_param(const char *src_xml, const char *dst_dir)
{

    AUDIOVBCEQ_PARAM_T param;
    TiXmlElement *group = NULL;

    TiXmlElement *src_group = NULL;
    TiXmlElement *dst_group = NULL;
    TiXmlElement *parent_group = NULL;

    TiXmlElement *root = NULL;

    const char *tmp = NULL;
    char path[1024] = {0};

    int ret = 0;

    memset(&param, 0, sizeof(AUDIOVBCEQ_PARAM_T));

    ret = load_xml_handle(&param.xml, src_xml, NULL);
    if (ret != 0) {
        SHELL_DBG("load xml handle failed (%s)", src_xml);
        return ret;
    }

    group = (TiXmlElement *)param.xml.param_root;
    if(NULL == group) {
        SHELL_DBG("%s %d\n", __func__, __LINE__);
        return -1;
    }

    strcat(path, dst_dir);

    _Division_audio_param(group, true, path, dst_dir);
    return 0;
}

static int _read_param_form_bin(TiXmlElement *group, char *data, bool root)
{
    TiXmlElement *Element = (TiXmlElement *)group;
    int ret = 0;
    TiXmlElement *child = NULL;
    const char *tmp = NULL;
    char val_string[16] = {0};

    while(NULL != Element) {
        tmp = Element->Attribute(ID);
        if(tmp == NULL) {
            _read_param_form_bin(Element->FirstChildElement(), data, false);
        } else {
            int size = 0;
            int addr_offset = 0;
            int values = 0;

            if(NULL == tmp) {
                SHELL_DBG("%s %d\n", __func__, __LINE__);
                return -1;
            }
            addr_offset = string_to_value(tmp);

            tmp = Element->Attribute(TYPE);
            if(NULL == tmp) {
                SHELL_DBG("%s %d\n", __func__, __LINE__);
                return -1;
            }

            if(strcmp(tmp, U32) == 0) {
                size = sizeof(int32_t);
            } else if(strcmp(tmp, U16) == 0) {
                size = sizeof(int16_t);
            } else if(strcmp(tmp, U8) == 0) {
                size = sizeof(uint8_t);
            }

            values = 0;

            memcpy(&values, data + addr_offset, size);

            child = Element->FirstChildElement();

            if(NULL != child) {
                while(NULL != child) {
                    int bits = 0;
                    int offset = 0;
                    int val = 0;
                    int i = 0;
                    int mask = 0;

                    tmp = child->Attribute(BITS);
                    if(NULL == tmp) {
                        SHELL_DBG("%s %d\n", __func__, __LINE__);
                        return -1;
                    }
                    bits = string_to_value(tmp);

                    tmp = child->Attribute(OFFSETS);
                    if(NULL == tmp) {
                        SHELL_DBG("%s %d\n", __func__, __LINE__);
                        return -1;
                    }
                    offset = string_to_value(tmp);

                    mask=0;
                    for(i = 0; i < bits; i++) {
                        mask |= 1 << i;
                    }
                    val = (values >> offset)&mask;

                    memset(val_string, 0, sizeof(val_string));
                    sprintf(val_string, "0x%x", val);

                    child->SetAttribute(VALUE, val_string);

                    //SHELL_DBG("ele:%s values:0x%x attr_val:%s mask:%x bits:%d\n",child->Value(),values,val_string,mask,bits);

                    child = child->NextSiblingElement();

                }
            } else {
                memset(val_string, 0, sizeof(val_string));
                sprintf(val_string, "0x%x", values);
                Element->SetAttribute(VALUE, val_string);
                //SHELL_DBG("%s values:0x%x :%s\n",Element->Value(),values,val_string);
            }
        }

        if(root) {
            break;
        }

        Element = Element->NextSiblingElement();
    }

    return 0;
}

int read_param_from_bin(const char *src_xml, const char *dst_xml,const char *src_bin)
{

    AUDIOVBCEQ_PARAM_T param;
    TiXmlElement *group = NULL;
    TiXmlElement *root = NULL;

    const char *tmp=NULL;
    int fd = -1;

    int ret = 0;

    memset(&param, 0, sizeof(AUDIOVBCEQ_PARAM_T));

    ret = load_xml_handle(&param.xml, src_xml, NULL);
    if (ret != 0) {
        SHELL_DBG("load xml handle failed (%s)", src_xml);
        return ret;
    }

    group = (TiXmlElement *)param.xml.param_root;
    if(NULL == group) {
        SHELL_DBG("%s %d\n", __func__, __LINE__);
        return -1;
    }

    fd = open(src_bin, O_RDONLY);
    if(fd < 0) {
        return fd;
    } else {
        struct vbc_fw_header fw_header;
        int buffer_size = 0;
        memset(&fw_header, 0, sizeof(struct vbc_fw_header));
        ret = read(fd, &fw_header, sizeof(struct vbc_fw_header));

        if(ret != sizeof(struct vbc_fw_header)) {
            SHELL_DBG("%s %d\n", __func__, __LINE__);
            return ret;
        }

        buffer_size = fw_header.num_mode * fw_header.len;

        param.data = (char *)malloc(buffer_size);
        if(NULL == param.data) {
            SHELL_DBG("%s %d\n", __func__, __LINE__);
            return -1;
        }

        memset(param.data,0,buffer_size);

        ret = read(fd, param.data, buffer_size);
        if(ret != buffer_size) {
            SHELL_DBG("%s %d\n", __func__, __LINE__);
            return ret;
        }

        close(fd);
        _read_param_form_bin((TiXmlElement *)param.xml.param_root, param.data, true);
    }

    if(NULL != param.xml.param_doc) {

        root = (TiXmlElement *)param.xml.param_root;
        tmp = root->Attribute(STRUCT_SIZE);
        if(tmp != NULL) {
            root->RemoveAttribute(STRUCT_SIZE);
        }

        tmp = root->Attribute(NUM_MODE);
        if(tmp != NULL) {
            root->RemoveAttribute(NUM_MODE);
        }

        XML_save_param(param.xml.param_doc,
                       dst_xml);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if((argc != 6) && (argc != 4) &&(argc != 5)) {
        SHELL_DBG("argc err :%d\n",__LINE__);
        return -1;

    }

    if(0 == strncmp(argv[1], "clone", strlen("clone"))) {
        if(argc != 6) {
            SHELL_DBG("argc err :%d\n",__LINE__);
            return -1;
        }
        return clone_audio_param(argv[2], argv[3], argv[4], argv[5]);
    } else if(0 == strncmp(argv[1], "Insert", strlen("Insert"))) {
        if(argc != 6) {
            SHELL_DBG("argc err :%d\n",__LINE__);
            return -1;
        }
        return Insert_audio_param(argv[2], argv[3], argv[4], argv[5]);
    } else if(0 == strncmp(argv[1], "Division", strlen("Division"))) {
        if(argc != 4) {
            SHELL_DBG("argc err :%d\n",__LINE__);
            return -1;
        }
        return Division_audio_param(argv[2], argv[3]);
    }else if(0 == strncmp(argv[1], "Load", strlen("Load"))) {
        if(argc != 5) {
            SHELL_DBG("argc err :%d\n",__LINE__);
            return -1;
        }
        return read_param_from_bin(argv[2], argv[3],argv[4]);
    }
    return 0;
}

