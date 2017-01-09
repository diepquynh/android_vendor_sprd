#define LOG_TAG "audio_hw_xml_utils"

#include "audio_xml_utils.h"
#include "tinyxml.h"
#include "cutils/log.h"
#include "audio_debug.h"
int private_set_sub_struct(TiXmlElement *struct_group, void *struct_val,
                           int *len, bool root);

int string_to_value(const char *svalue)
{
    int value;
    if (strchr(svalue, 'x')) {
        //hex format
        sscanf(svalue, "%x", &value);
    } else {
        value = atoi(svalue);
    }
    return value;
}

void value_to_string(int value, char *svalue)
{
    sprintf(svalue, "0x%x", value);
}

param_doc_t XML_open_param(const char *file_name)
{
    TiXmlDocument *doc = new TiXmlDocument();
    if (doc == NULL || !doc->LoadFile(file_name)) {
        LOG_E("open the xml failed, file_name=%s\n", file_name);
        return NULL;
    }
    return (param_doc_t)doc;
}

param_group_t XML_get_root_param(param_doc_t pdoc)
{
    TiXmlDocument *doc = (TiXmlDocument *)pdoc;
    TiXmlElement *root = doc->RootElement();
    return (param_group_t)root;
}

void XML_save_param(param_doc_t param_doc, const char *savename)
{
    TiXmlDocument *doc = (TiXmlDocument *) param_doc;
    if (doc != NULL) {
        doc->SaveFile(savename);
    }
}

void XML_release_param(param_doc_t param_doc)
{
    TiXmlDocument *doc = (TiXmlDocument *) param_doc;
    delete doc;
}

param_group_t open_xml(const char *file_name)
{
    TiXmlDocument *doc = new TiXmlDocument();
    if (doc == NULL || !doc->LoadFile(file_name)) {
        LOG_E("open the xml failed, file_name=%s\n", file_name);
        return NULL;
    }
    TiXmlElement *root = doc->RootElement();
    return (param_group_t) root;
}

const char *XML_get_group_attr(param_group_t group, const char *attr)
{
    if (group == NULL) {
        LOG_E("the group NULL");
        return NULL;
    }
    return ((TiXmlElement *)group)->Attribute(attr);
}

TiXmlElement *private_get_param(param_group_t group, const char *param_name)
{
    char *sub_name;
    TiXmlElement *param_group;
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

    sub_name = strtok(name, "\\");

    LOG_D("%s %d values:%s %s",__func__,__LINE__,((TiXmlElement *)group)->Value(),sub_name);
    TiXmlElement *param = ((TiXmlElement *)group)->FirstChildElement(sub_name);
    if (param == NULL)
    {
        LOG_E("can not find the param group %s %s, %s",((TiXmlElement *)group)->Value(),sub_name, param_name);
        return NULL;
    }

    TiXmlElement *pre_param = NULL;
    do
    {
        sub_name = strtok(NULL, "\\");
        if (sub_name == NULL)
        {
            break;
        }

        pre_param = param;
        LOG_D("private_get_param %s find %s",param->Value(),sub_name);
        param = param->FirstChildElement(sub_name);

        if (param == NULL)
        {
            LOG_D("private_get_param %s find end ",pre_param->Value());
            param = pre_param->FirstChildElement();
            for (; param != NULL; param = param->NextSiblingElement())
            {
                LOG_D("private_get_param Attribute:%s sub_name:%s",param->Attribute(NAME),sub_name);
                if (param->Attribute(NAME) != NULL &&
                        strcmp(param->Attribute(NAME), sub_name) == 0)
                {
                    break;
                }
            }
        }else{
           LOG_D("private_get_param %s find %s success",param->Value(),sub_name);
        }
    }while (param != NULL);
    free(name);

    return param;

}

const char *XML_get_group_name(param_group_t group)
{
    if (group == NULL) {
        return NULL;
    }
    TiXmlElement *group_t = (TiXmlElement *)group;
    return group_t->Value();
}

param_group_t XML_open_param_group(param_group_t group, const char *group_name)
{
    return (param_group_t)private_get_param(group, group_name);
}

param_group_t XML_get_first_sub_group(param_group_t group)
{
    if (group == NULL) {
        return NULL;
    }
    TiXmlElement *group_t = (TiXmlElement *)group;
    return (param_group_t) group_t->FirstChildElement();
}

param_group_t XML_get_next_sibling_group(param_group_t group)
{
    if (group == NULL) {
        return NULL;
    }
    TiXmlElement *group_t = (TiXmlElement *)group;
    return (param_group_t) group_t->NextSiblingElement();
}

int XML_get_int_in_group(param_group_t group, const char *param_name)
{
    if (group == NULL || param_name == NULL) {
        LOG_E("get value failed %s, group = %p", param_name, group);
        return 0;
    }
    const char *svalue;
    param_group_t param_t = private_get_param(group, param_name);
    TiXmlElement *param = (TiXmlElement *)param_t;
    if (param == NULL) {
        LOG_E("can not find param %s", param_name);
        //TODO, if use the wrong param name, return zero is not good design
        return 0;
    }
    svalue = param->Attribute(VALUE);

    if (svalue == NULL) {
        LOG_E("can not get the value for %s", param_name);
    }

    return string_to_value(svalue);
}

int XML_set_int_int_group(param_group_t group, const char *param_name,
                          int value)
{
    char val_str[32]={0};
    if (group == NULL || param_name == NULL) {
        LOG_E("set value failed %s, group = %p", param_name, group);
        return -1;
    }
    const char *svalue;
    param_group_t param_t = private_get_param(group, param_name);
    TiXmlElement *param = (TiXmlElement *)param_t;
    if (param == NULL) {
        LOG_E("can not find param %s", param_name);
        //TODO, if use the wrong param name, return zero is not good design
        return -1;
    }

    sprintf(val_str,"0x%x",value);
    param->SetAttribute(VALUE, val_str);
    return 0;
}


const char *XML_get_string_in_group(param_group_t group, const char *param_name)
{
    if (group == NULL || param_name == NULL) {
        LOG_E("XML_get_string_in_group get value failed %s, group = %p", param_name,
              group);
        return NULL;
    }
    param_group_t param_t = private_get_param(group, param_name);
    if (param_t == NULL) {
        LOG_E("XML_get_string_in_group 2 get value failed %s, group = %p", param_name,
              group);
        return NULL;
    }
    TiXmlElement *param = (TiXmlElement *)param_t;

    if (param == NULL) {
        LOG_E("XML_get_string_in_group can not find param %s", param_name);
        //TODO, if use the wrong param name, return zero is not good design
        return 0;
    }
    return param->Attribute(VALUE);
}

param_group_t XML_set_string_in_group(param_group_t group,
        const char *param_name,
        const char *svalue)
{
    if (group == NULL || param_name == NULL) {
        LOG_E("get value failed %s, group = %p", param_name, group);
        return NULL;
    }
    param_group_t param_t = private_get_param(group, param_name);
    if (param_t == NULL) {
        LOG_E("can not get the param:%s",param_name);
        return NULL;
    }
    TiXmlElement *param = (TiXmlElement *)param_t;

    if (param == NULL) {
        LOG_E("can not find param %s", param_name);
        //TODO, if use the wrong param name, return zero is not good design
        return NULL;
    }
    param->SetAttribute(VALUE, svalue);
    LOG_D("XML_set_string_in_group:%s:%s:(%s)",param_name,param->Attribute(VALUE),svalue);
    return param;
}

int private_get_sub_struct(TiXmlElement *struct_group, void *struct_val,
                           int *len, bool root)
{
    const char *type;
    const char *value;
    const char *str_len;
    TiXmlElement *group = struct_group;
    while (group != NULL) {
        type = group->Attribute(TYPE);
        if (type == NULL) {
            private_get_sub_struct(group->FirstChildElement(), struct_val, len, false);
        } else {
            value = group->Attribute(VALUE);
            //String case
            if (strcmp(type, STR) == 0) {
                str_len = group->Attribute(STR_LEN);
                if (str_len == NULL) {
                    LOG_E("can not find the string lens, error\n");
                    return -1;
                }
                memcpy((uint8_t *)struct_val + *len, value, strlen(value));
                *((uint8_t *)((uint8_t *)struct_val + *len + strlen(value))) = '\0';
                *len += atoi(str_len);
            } else if (strcmp(type, U8) == 0) { //uint8_t
                int ival = string_to_value(value);
                *((uint8_t *)((uint8_t *)struct_val + *len)) = (uint8_t)ival;
                *len += 1;
            } else if (strcmp(type, U16) == 0) { //uint16_t
                int ival = string_to_value(value);
                *((uint16_t *)((uint8_t *)struct_val + *len)) = (uint16_t)ival;
                *len += 2;
            } else if (strcmp(type, U32) == 0) { //uint32_t
                int ival = string_to_value(value);
                *((uint32_t *)((uint8_t *)struct_val + *len)) = (uint32_t)ival;
                *len += 4;
            } else {
                LOG_E("the type is Unknown , error");
                return -1;
            }
        }
        if (root) {
            break;
        }
        group = group->NextSiblingElement();
    }
    return 0;
}

int XML_get_struct_value(param_group_t struct_group, void *struct_val,
                         int struct_size)
{
    int ret;
    if (struct_group == NULL) {
        LOG_E("struct group is NULL\n");
    }
    TiXmlElement *struct_ele = (TiXmlElement *)struct_group;
    int len = 0;
    ret = private_get_sub_struct(struct_ele, struct_val, &len, true);
    if (struct_size != len) {
        LOG_E("struct size = %d, parse from xml size is %d, not match ,error",
              struct_size, len);
        ret = -1;
    }
    return ret;
}

int private_set_sub_struct(TiXmlElement *struct_group, void *struct_val,
                           int *len, bool root)
{
    const char *type;
    char svalue[16];
    const char *str_len;
    TiXmlElement *group = struct_group;
    while (group != NULL) {
        type = group->Attribute(TYPE);
        if (type == NULL) {
            private_set_sub_struct(group->FirstChildElement(), struct_val, len, false);
        } else {
            if (strcmp(type, STR) == 0) {
                char str[100];
                str_len = group->Attribute(STR_LEN);
                if (str_len == NULL) {
                    LOG_E("can not find the string lens, error\n");
                    return -1;
                }
                strcpy(str, (char *)((char *)struct_val + *len));
                group->SetAttribute(VALUE, str);
                *len += atoi(str_len);
            } else if (strcmp(type, U8) == 0) {
                uint8_t value = *((uint8_t *)((uint8_t *)struct_val + *len));
                value_to_string(value, svalue);
                group->SetAttribute(VALUE, svalue);
                *len += 1;
            } else if (strcmp(type, U16) == 0) {
                uint16_t value = *((uint16_t *)((uint8_t *)struct_val + *len));
                value_to_string(value, svalue);
                group->SetAttribute(VALUE, svalue);
                *len += 2;
            } else if (strcmp(type, U32) == 0) {
                uint32_t value = *((uint32_t *)((uint8_t *)struct_val + *len));
                value_to_string(value, svalue);
                group->SetAttribute(VALUE, svalue);
                *len += 4;
            } else {
                LOG_E("the type is Unknown , error");
                return -1;
            }
        }
        group = group->NextSiblingElement();
    }
    return 0;

}


int XML_set_struct_value(param_group_t struct_group, void *struct_val,
                         int struct_size)
{
    int ret;
    if (struct_group == NULL) {
        LOG_E("struct group is NULL\n");
    }
    TiXmlElement *struct_ele = (TiXmlElement *)struct_group;
    int len = 0;
    ret = private_set_sub_struct(struct_ele, struct_val, &len, true);
    if (struct_size != len) {
        LOG_E("struct size = %d, parse from xml size is %d, not match ,error",
              struct_size, len);
        ret = -1;
    }
    return ret;
}


