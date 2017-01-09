#ifndef _AUDIO_XML1_UTILS_H_
#define _AUDIO_XML1_UTILS_H_

#include "stdint.h"
#ifdef __cplusplus
extern "C" {

#define VALUE "val"

#define TYPE "t"
#define NAME "name"
#define STR_LEN "l"

#define U16 "u16"
#define U32 "u32"
#define U8  "u8"
#define STR "str"


/** the element in XML, it can contains sub element, and also can be contains in it's father group*/
typedef void *param_group_t;

typedef void *param_doc_t;


/** open the xml file with tinyxml, and return the root element*/
param_group_t open_xml(const char *file_name);

param_doc_t XML_open_param(const char *file_name);
param_group_t XML_get_root_param(param_doc_t pdoc);
void XML_save_param(param_doc_t param_doc, const char *savename);
void XML_release_param(param_doc_t param_doc);


param_group_t XML_get_first_sub_group(param_group_t group);
param_group_t XML_get_next_sibling_group(param_group_t group);


/** open the sub group in group, sub group name is group_name
    param: group, the group that contains the sub group
    param: param_name, the sub group's name
    return value: the sub group
 * */
param_group_t XML_open_param_group(param_group_t group, const char *group_name);

const char *XML_get_group_attr(param_group_t group, const char *attr);
const char *XML_get_group_name(param_group_t group);


/** get the int value in group
 * */
int XML_get_int_in_group(param_group_t group, const char *param_name);
int XML_set_int_int_group(param_group_t group, const char *param_name,
                          int value);

/** get the string value in group
 * */
const char *XML_get_string_in_group(param_group_t group,
                                    const char *param_name);
param_group_t XML_set_string_in_group(param_group_t group,
        const char *param_name,
        const char *svalue);
int XML_get_struct_value(param_group_t struct_group, void *struct_val,
                         int struct_size);
int XML_set_struct_value(param_group_t struct_group, void *struct_val,
                         int struct_size);
}

int string_to_value(const char *svalue);
void value_to_string(int value, char *svalue);
#else

#define VALUE "val"

#define TYPE "t"
#define NAME "name"
#define STR_LEN "l"


/** the element in XML, it can contains sub element, and also can be contains in it's father group*/
typedef void *param_group_t;

typedef void *param_doc_t;


/** open the xml file with tinyxml, and return the root element*/
param_group_t open_xml(const char *file_name);

param_doc_t XML_open_param(const char *file_name);
param_group_t XML_get_root_param(param_doc_t pdoc);
void XML_save_param(param_doc_t param_doc, const char *savename);
void XML_release_param(param_doc_t param_doc);


param_group_t XML_get_first_sub_group(param_group_t group);
param_group_t XML_get_next_sibling_group(param_group_t group);


/** open the sub group in group, sub group name is group_name
    param: group, the group that contains the sub group
    param: param_name, the sub group's name
    return value: the sub group
 * */
param_group_t XML_open_param_group(param_group_t group, const char *group_name);

const char *XML_get_group_attr(param_group_t group, const char *attr);
const char *XML_get_group_name(param_group_t group);


/** get the int value in group
 * */
int XML_get_int_in_group(param_group_t group, const char *param_name);
int XML_set_int_int_group(param_group_t group, const char *param_name,
                          int value);

/** get the string value in group
 * */
const char *XML_get_string_in_group(param_group_t group,
                                    const char *param_name);
param_group_t XML_set_string_in_group(param_group_t group, const char *param_name,
                            const char *svalue);

int XML_get_struct_value(param_group_t struct_group, void *struct_val,
                         int struct_size);
int XML_set_struct_value(param_group_t struct_group, void *struct_val,
                         int struct_size);

#endif
#endif//_AUDIO_UTILS_H_
