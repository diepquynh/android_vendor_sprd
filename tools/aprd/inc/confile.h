/**
 * INI配置文件管理函数库
 * Ini file parse functions.
 * By Hoverlees http://www.hoverlees.com me[at]hoverlees.com
 */

#ifndef _HOVERLEES_INI_CONFIG_H
#define _HOVERLEES_INI_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CONFIG_BTREE_NODE{
	char*	key;
	void*	data;
	struct _CONFIG_BTREE_NODE* left;
	struct _CONFIG_BTREE_NODE* right;
	char mem[2];
}CONFIG_BTREE_NODE;

typedef struct _CONFIG_BTREE{
	int numNodes;
	CONFIG_BTREE_NODE* root;
}CONFIG_BTREE;

typedef CONFIG_BTREE INI_CONFIG;

typedef int (*CONFIG_BTREE_TRAVERSE_CB)(CONFIG_BTREE_NODE* node);
typedef int (*CONFIG_BTREE_SAVE_TRAVERSE_CB)(FILE* fp,CONFIG_BTREE_NODE* node);

/**
 * ini内容解析函数,从字符串解析配置
 * @param str 字符串
 * @param slen 字符串长度,可以为0,如果为０,函数自动计算字符串长度
 * @param isGBK 如果ini文件使用GBK字符集,设置成1,否则设置成0
 * @return 成功返回INI_CONFIG指针,失败返回null
 */
INI_CONFIG* ini_config_create_from_string(unsigned char* str,int slen,int isGBK);

/**
 * ini内容解析函数,从文件解析配置
 * @param filename 配置文件名
 * @param isGBK 如果ini文件使用GBK字符集,设置成1,否则设置成0
 * @return 成功返回INI_CONFIG指针,失败返回null
 */
INI_CONFIG* ini_config_create_from_file(const char* filename,int isGBK);

/**
 * 配置释放函数,释放所占用的内存及数据结构
 * @param config 配置对象指针
 * @return 成功返回１,失败返回０
 */
void ini_config_destroy(INI_CONFIG* config);
/**
 * 获取配置整数值
 * @param config 配置对象指针
 * @param section 段名,没有段名时可以为NULL
 * @param key 键名
 * @param default_int　默认值
 * @return 如果配置中有此键对应的值,返回该值,否则返回参数指定的默认值　
 */
int ini_config_get_int(INI_CONFIG* config,const char* section,const char* key,int default_int);
/**
 * 获取配置字符串值
 * @param config 配置对象指针
 * @param section 段名,没有段名时可以为NULL
 * @param key 键名
 * @param default_string　默认值
 * @return 如果配置中有此键对应的值,返回该值,否则返回参数指定的默认值　
 */
char* ini_config_get_string(INI_CONFIG* config,const char* section,const char* key,char* default_string);

int ini_config_get_bool(INI_CONFIG* config,const char* section,const char* key, int default_bool);
/**
 * 设置变量
 * @param config 配置对象指针
 * @param section　段名,没有段名时可以为NULL
 * @param key　键名
 * @param key_len　键长
 * @param value　值
 * @param value_len　值长度
 * @return 成功为１,失败为０
 */
int ini_config_set_string(INI_CONFIG* config,const char* section,const char* key,int key_len,const char* value,int value_len);
/**
 * 设置变量
 * @param config 配置对象指针
 * @param section　段名,没有段名时可以为NULL
 * @param key　键名
 * @param key_len　键长
 * @param value　整数值
 * @return 成功为１,失败为０
 */
int ini_config_set_int(INI_CONFIG* config,const char* section,const char* key,int key_len,int value);
/**
 * 保存配置到文件中　*提示,原先配置文件中的注释信息将不会保存.
 * @param config 配置对象指针
 * @param filename 保存到的文件
 * @return 成功为１,失败为0
 */
int ini_config_save(INI_CONFIG* config,const char* filename);
/**
 * 类似于ini_config_save,只是参数是文件指针,此函数可以直接使用stdin,stdout,stderr.　*提示:本函数不负责关闭fp.
 * @param config 配置对象指针
 * @param fp 文件指针
 * @return 成功为１,失败为0
 */
int ini_config_print(INI_CONFIG* config,FILE* fp);

#ifdef __cplusplus
}
#endif

#endif
