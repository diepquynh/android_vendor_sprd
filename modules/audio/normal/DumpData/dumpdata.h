#ifndef _DUMPDATA_H
#define _DUMPDATA_H


#ifdef __cplusplus
extern   "C"
{
#endif



#define	DUMP_FILE_MAX_NUM 16


#define DUMP_NAME_LEN_MAX 30
#define DUMP_FILE_PATH_LEN_MAX 100

#define DUMP_PATH_PROPERTY	"media.dump.path"

#define DUMP_SWITCH_PROPERTY	"media.dump.switch"

typedef enum {
    DUMP_MUSIC_HWL_BEFORE_EXPRESS,
    DUMP_MUSIC_HWL_BEFOORE_VBC,
    DUMP_MUSIC_HWL_MIX_VAUDIO,
    DUMP_MUSIC_HWL_VOIP_WRITE,
    DUMP_MUSIC_HWL_BT_SCO_WRITE,
    DUMP_MUSIC_POINTER5,
    DUMP_MUSIC_POINTER6,
    DUMP_MUSIC_POINTER7,
    DUMP_RECORD_HWL_AFTER_VBC,
    DUMP_RECORD_HWL_AFTER_EXPRESS,
}dump_switch_e;


typedef enum {
    DUMP_PLAY,
    DUMP_RECORD,
    DUMP_MAX
}dump_type_t;


typedef struct 
{
	dump_switch_e dump_switch_info;

	void * buf;

	int32_t buf_len;

}dump_data_info_t;


typedef struct{
	int  	index;
	int   switch_t;
	char item_name[DUMP_NAME_LEN_MAX];
	char 	file_name[DUMP_NAME_LEN_MAX];	
	FILE	*fp;
	long int wr_file_size;
}aud_dump_item_t;
typedef struct{
	dump_type_t dump_type;
	int	num;
	int32_t	dump_switch;
	char 	path_name[DUMP_FILE_PATH_LEN_MAX];	
	aud_dump_item_t * aud_dump_item_info;
}aud_dump_t;
struct dump_parse_state{
	int		num;
	aud_dump_t  *aud_dmp_info;	
};



extern int dump_parse_xml();
//extern int dump_open_file();
extern int  dump_data(dump_data_info_t dump_info);
extern int dump_close_file();




#ifdef __cplusplus
}
#endif
#endif
