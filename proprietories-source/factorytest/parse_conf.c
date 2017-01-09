#include "testitem.h"

#define MAX_LINE_LEN			256
#define PCBA_SUPPORT_CONFIG   "/system/etc/factorytest/PCBA.conf"
extern int key_cnt;
extern struct test_key key_info[8];

static int parse_string(char * buf, char gap, int* value)
{
    int len = 0;
    char *ch = NULL;
    char str[10] = {0};

    if(buf != NULL && value  != NULL){
        ch = strchr(buf, gap);
        if(ch != NULL){
            len = ch - buf ;
            strncpy(str, buf, len);
            *value = atoi(str);
        }
    }

    return len;
}

char *parse_string_ptr(char *src, char c)
{
	char *results;
	results = strchr(src, c);
	if(results == NULL) {
		LOGD("%s is null!", results);
		return NULL;
	}
	results++ ;
	while(results[0]== c)
		results++;
	return results;
}

char * parse_string_get(char *buf, char c, char *str)
{
    int len = 0;
    char *ch = NULL;

    if(buf != NULL){
        ch = strchr(buf, c);
        if(ch != NULL){
            len = ch - buf ;
            strncpy(str, buf, len);
        }
    }
    return str;
}

int parse_case_entries(char *type, int* arg1, int* arg2)
{
	int len;
	char *str = type;

	/* sanity check */
	if(str == NULL) {
		LOGD("type is null!");
		return -1;
	}
	if((len = parse_string(str, '\t', arg1)) <= 0)	return -1;

	str += len + 1;
	if(str == NULL) {
		LOGD("mmitest type is null!");
		return -1;
	}
	if((len = parse_string(str, '\t', arg2)) <= 0)	return -1;

	return 0;
}

int get_sensor_Num(char* s)
{
   if(!strcmp(s,"Ts")) return SENSOR_TS;
   if(!strcmp(s,"Acc")) return SENSOR_ACC;
   if(!strcmp(s,"Pxy")) return SENSOR_PXY;
   if(!strcmp(s,"Lux")) return SENSOR_LUX;
   if(!strcmp(s,"Hde")) return SENSOR_HDE;
   if(!strcmp(s,"Hdk"))	return SENSOR_HDK;

   return -1;
}

int parse_sensor_entries(char *buf)
{
	char *pos1, *pos2;
	char type[BUF_LEN] = {0};
	char name[BUF_LEN] = {0};
	int id;

	/* fetch each field */
	if((pos1 = parse_string_ptr(buf, '\t')) == NULL)
		return -1;
	else parse_string_get(pos1, '\t',type);

	if((pos2 = parse_string_ptr(pos1, '\t')) == NULL)
		return -1;
	else  parse_string_get(pos2, '\n',name);

	/* init data structure according to type */
	LOGD("sensor type = %s; sensor name = %s", type, name);
	//if(NULL != type && NULL != name) {
		id = get_sensor_Num(strdup(type));
		switch (id)
		{
			case SENSOR_TS:
				strncpy(TS_DEV_NAME,strdup(name),strlen(strdup(name)));
				LOGD("TS_DEV_NAME:%s",TS_DEV_NAME);
				break;
			case SENSOR_ACC:
				strncpy(ACC_DEV_NAME,strdup(name),strlen(strdup(name)));
				LOGD("ACC_DEV_NAME:%s",ACC_DEV_NAME);
				break;
			case SENSOR_PXY:
				strncpy(PXY_DEV_NAME,strdup(name),strlen(strdup(name)));
				LOGD("PXY_DEV_NAME:%s",PXY_DEV_NAME);
				break;
			case SENSOR_LUX:
				strncpy(LUX_DEV_NAME,strdup(name),strlen(strdup(name)));
				LOGD("LUX_DEV_NAME:%s",LUX_DEV_NAME);
				break;
			case SENSOR_HDE:
				strncpy(HED_DEV_NAME,strdup(name),strlen(strdup(name)));
				LOGD("HED_DEV_NAME:%s",HED_DEV_NAME);
				break;
			case SENSOR_HDK:
				strncpy(HEK_DEV_NAME,strdup(name),strlen(strdup(name)));
				LOGD("HEK_DEV_NAME:%s",HEK_DEV_NAME);
				break;
		}
	//}

	return 0;
}

int get_key_Num(char* s)
{
   if(!strcmp(s,"Power")) return KEY_TYPE_POWER;
   if(!strcmp(s,"VolumeDown")) return KEY_TYPE_VOLUMEDOWN;
   if(!strcmp(s,"VolumeUp")) return KEY_TYPE_VOLUMEUP;
   if(!strcmp(s,"Camera")) return KEY_TYPE_CAMERA;
   if(!strcmp(s,"Menu")) return KEY_TYPE_MENU;
   if(!strcmp(s,"Home")) return KEY_TYPE_HOME;
   if(!strcmp(s,"Back")) return KEY_TYPE_BACK;

   return -1;
}

int parse_key_entries(char *buf)
{
	char *pos1, *pos2;
	char type[BUF_LEN] = {0};
	int value;
	int id;

	/* fetch each field */
	if((pos1 = parse_string_ptr(buf, '\t')) == NULL)
		return -1;
	else parse_string_get(pos1, '\t',type);

	if((pos2 = parse_string_ptr(pos1, '\t')) == NULL)
		return -1;
	else  parse_string(pos2, '\n',&value);

	/* init data structure according to type */
	//if(NULL != type) {
		id = get_key_Num(strdup(type));
		switch (id)
		{
			case KEY_TYPE_POWER:
				strncpy(key_info[key_cnt].key_shown,TEXT_KEY_POWER,strlen(TEXT_KEY_POWER));
				break;
			case KEY_TYPE_VOLUMEDOWN:
				strncpy(key_info[key_cnt].key_shown,TEXT_KEY_VOLUMEDOWN,strlen(TEXT_KEY_VOLUMEDOWN));
				break;
			case KEY_TYPE_VOLUMEUP:
				strncpy(key_info[key_cnt].key_shown,TEXT_KEY_VOLUMEUP,strlen(TEXT_KEY_VOLUMEUP));
				break;
			case KEY_TYPE_CAMERA:
				strncpy(key_info[key_cnt].key_shown,TEXT_KEY_CAMERA,strlen(TEXT_KEY_CAMERA));
				break;
			case KEY_TYPE_MENU:
				strncpy(key_info[key_cnt].key_shown,TEXT_KEY_MENU,strlen(TEXT_KEY_MENU));
				break;
			case KEY_TYPE_HOME:
				strncpy(key_info[key_cnt].key_shown,TEXT_KEY_HOMEPAGE,strlen(TEXT_KEY_HOMEPAGE));
				break;
			case KEY_TYPE_BACK:
				strncpy(key_info[key_cnt].key_shown,TEXT_KEY_BACK,strlen(TEXT_KEY_BACK));
				break;
			default:
				LOGD("invalid key");
				break;
		}
		strncpy(key_info[key_cnt].key_name,strdup(type),strlen(strdup(type)));
		key_info[key_cnt].key = value;
		LOGD("key_info[%d].key:%s,key_info[%d].key:%d",key_cnt,key_info[key_cnt].key_name,key_cnt,key_info[key_cnt].key);
		key_cnt++;
	//}

	return 0;
}

int parse_config()
{
	FILE *fp;
	int ret = 0, count = 0, err = 0;
	int id,flag;
	char *type,*name;
	char buffer[MAX_LINE_LEN]={0};

	fp = fopen(PCBA_SUPPORT_CONFIG, "r");
	if(fp == NULL) {
		LOGE("mmitest open %s failed! %d IN", PCBA_SUPPORT_CONFIG, __LINE__);
		return -1;
	}

	/* parse line by line */
	ret = 0;
	while(fgets(buffer, MAX_LINE_LEN, fp) != NULL) {
		if('#'==buffer[0])
			continue;
		if((buffer[0]>='0') && (buffer[0]<='9')){
			ret = parse_case_entries(buffer,&id,&flag);
			if(ret != 0) {
				LOGD("mmitest parse %s,buffer=%s return %d.  reload",PCBA_SUPPORT_CONFIG, buffer,ret);
				fclose(fp);
				return -1;
			}
			support_result[count].id = id;
			support_result[count++].support= flag;
			err = 1;
		}else if(!strncmp("sensor", buffer, 6)){
			ret = parse_sensor_entries(buffer);
			if(ret != 0) {
				LOGD("mmitest parse %s,buffer=%s return %d.  reload",PCBA_SUPPORT_CONFIG, buffer,ret);
				fclose(fp);
				return -1;
			}
			err = 1;
		}else if(!strncmp("key", buffer, 3)){
			ret = parse_key_entries(buffer);
			if(ret != 0) {
				LOGD("mmitest parse %s,buffer=%s return %d.  reload",PCBA_SUPPORT_CONFIG, buffer,ret);
				fclose(fp);
				return -1;
			}
			err = 1;
		}
		if(0 == err)
			LOGD("can't check line = %s", buffer);
		err = 0;
	}

	fclose(fp);
	if(count < TOTAL_NUM) {
		LOGD("mmitest parse slog.conf failed");
	}

	return ret;
}
