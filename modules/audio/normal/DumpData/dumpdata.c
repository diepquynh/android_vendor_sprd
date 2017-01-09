
/*
 * Copyright (C) 2008 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "DUMP.DATA"

#include <cutils/log.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <pthread.h>

#include <cutils/properties.h>

#include <expat.h>
#include "dumpdata.h"

//#define XDUMP_DEBUG

#ifdef XDUMP_DEBUG
#define XDUMP_TRACE  ALOGW
#else
#define XDUMP_TRACE
#endif


#define DUMP_FILE_MAX_SIZE 	500*1024*1024

static aud_dump_item_t aud_dump_file_info[DUMP_FILE_MAX_NUM];

static int32_t dump_switch = 0;

static int32_t total_switch = 0;

static pthread_mutex_t  Dumplock = PTHREAD_MUTEX_INITIALIZER;

static int dump_open_file(int );


//char AUDIO_OUT_FILE_PATH[DUMP_FILE_PATH_LEN_MAX]; //"data/audio_out.pcm"
FILE *AUDIO_OUT_fd[DUMP_FILE_MAX_NUM];


#define AUDIO_XML_PATH "/system/etc/audio_hw.xml"
#define BUF_SIZE 1024

static  aud_dump_item_t *dump_create_mem(aud_dump_t  *aud_dump_info, const char *num)
{	
    aud_dump_item_t *a;
	     XDUMP_TRACE("enter dump_create_mem---");
    if (!atoi((char *)num)) {
        ALOGE("Unnormal dump item num!");
        return NULL;
    }


    aud_dump_info->num = atoi((char *)num);

    if(!aud_dump_info->aud_dump_item_info)
    {
    		aud_dump_info->aud_dump_item_info = malloc(aud_dump_info->num *
               sizeof(aud_dump_item_t));
		XDUMP_TRACE("enter dump_create_mem---1 =%d" ,aud_dump_info->num);
        	if (aud_dump_info->aud_dump_item_info  == NULL) {
            		ALOGE("Unable to allocate dump item profiles");
            		return NULL;
        	} 
        	else
        	{

            		memset((void*)aud_dump_info->aud_dump_item_info,0x00,aud_dump_info->num *
                    	sizeof(aud_dump_item_t));
			XDUMP_TRACE("enter dump_create_mem---2");
       	 	}
    }


    return aud_dump_info->aud_dump_item_info ;
}

static void dump_start_tag(void *data, const XML_Char *tag_name,
        const XML_Char **attr)
{
    struct dump_parse_state *state = data;
    aud_dump_t  *aud_dump_info = NULL;

    unsigned int i;
    int value ,ret = 0;
    struct mixer_ctl *ctl;
   int s_len = 0;


     XDUMP_TRACE("enter dump_start_tag---%s ",tag_name);


    if (strcmp(tag_name, "audio") == 0) {
        if (strcmp(attr[0], "device") == 0) {
            XDUMP_TRACE("The device name is %s", attr[1]);
        } else {
            ALOGE("Unnamed audio!");
        }
    }
    else if (strcmp(tag_name, "dumpmusic") == 0) {
		dump_create_mem(state->aud_dmp_info, attr[1]);	
		if( strcmp(attr[2], "dump_switch") == 0 )  {
			XDUMP_TRACE("The DUMP SWITCH is '%s'", attr[3]);
			ret = sscanf(attr[3] , "%x",&( state->aud_dmp_info->dump_switch));
			if (ret <0 )
				ALOGE(" dump switch is error!");
			property_set(DUMP_SWITCH_PROPERTY , attr[3]);
			dump_switch = state->aud_dmp_info->dump_switch;
		}
		else {
            		ALOGE("NO dump switch!");
        	}

		if( strcmp(attr[4], "dump_path") == 0 )  {
			ALOGD("The DUMP PATH is '%s'", attr[5]);
            		if((strlen(attr[5]) +1) <= DUMP_FILE_PATH_LEN_MAX){
				s_len = strlen(attr[5]);
				memcpy(state->aud_dmp_info->path_name, attr[5], strlen(attr[5])+1);
				XDUMP_TRACE("The dump path name 2 is %d last n = %'",strlen(attr[5]) ,state->aud_dmp_info->path_name[s_len-1] );
				
	                		
				if ( state->aud_dmp_info->path_name[s_len-1] != '/')
					strcat(state->aud_dmp_info->path_name , "/");
				
		 		property_set(DUMP_PATH_PROPERTY , attr[5]);
				XDUMP_TRACE("The dump path name 2 is '%s'", state->aud_dmp_info->path_name);			  	
            			} 
			}
		else{
			ALOGE("NO dump path!");
		}

		if( strcmp(attr[6], "total_switch") == 0 )  {
			ALOGD("total switch  '%s'", attr[7]);
            		if(strcmp(attr[7],"ON") == 0){
                		total_switch = 1;	
		        }
		        else
			{
				total_switch = 0;
		        	XDUMP_TRACE("OFF for this item");
		        }
		}
		else{
			total_switch = 0;
			ALOGE("NO total_switch!");
		}
    }
 

    else if ((strcmp(tag_name, "dump_play") == 0) ||(strcmp(tag_name, "dump_record") == 0))
    {

    		aud_dump_info = state->aud_dmp_info;

		
    		 
        if (strcmp(attr[0], "index") == 0) {
            XDUMP_TRACE("The dump index is '%s'", attr[1]);
            aud_dump_info->aud_dump_item_info->index= atoi((char *)attr[1]);
        } else {
            ALOGE("no INDEX for DUMP!");
        }

	if (strcmp(attr[2], "item_name") == 0) {
            XDUMP_TRACE("The dump item_name is '%s'", attr[3]);
           memcpy(aud_dump_info->aud_dump_item_info->item_name,(void*)attr[3],strlen((char *)attr[3])+1);
		   XDUMP_TRACE("The dump item_name 2 is '%s'", aud_dump_info->aud_dump_item_info->item_name);
        } else {
            ALOGE("no iis_ctl index for bt call!");
        }
		
	 if (strcmp(attr[4], "dump_file_name") == 0) {
            XDUMP_TRACE("The DUMP File Name is '%s'", attr[5]);
            if((strlen(attr[5]) +1) <= DUMP_NAME_LEN_MAX){
                memcpy(aud_dump_info->aud_dump_item_info->file_name, attr[5], strlen(attr[5])+1);	
		 XDUMP_TRACE("The dump FILE name 2 is '%s'", aud_dump_info->aud_dump_item_info->file_name);		  
            }        
	     } else {
            ALOGE("no dump_file_name for DUMP!");
        }
		
        if (strcmp(attr[6], "switch") == 0) {
            XDUMP_TRACE("The DUMP switch is '%s'", attr[7]);
            if(strcmp(attr[7],"1") == 0){
                	aud_dump_info->aud_dump_item_info->switch_t= 1;
            	}
        	else if(strcmp(attr[7],"0") == 0){
			aud_dump_info->aud_dump_item_info->switch_t= 0;
        	}else
		{
            		ALOGE("no swithc for this item");
        	}
        }
	aud_dump_info->aud_dump_item_info->fp = NULL;
	memcpy( (void*)(&(aud_dump_file_info[aud_dump_info->aud_dump_item_info->index])),(void*)(aud_dump_info->aud_dump_item_info) , sizeof(aud_dump_item_t) );
	
	dump_open_file(aud_dump_info->aud_dump_item_info->index);
	ALOGE("dump_open_file ------index =  %d   filenanme =  %s ",aud_dump_info->aud_dump_item_info->index , aud_dump_file_info[aud_dump_info->aud_dump_item_info->index].file_name);
	aud_dump_info->aud_dump_item_info ++;
    }
	


attr_err:
    return;
}
static void dump_end_tag(void *data, const XML_Char *tag_name)
{
    struct modem_config_parse_state *state = data;
	XDUMP_TRACE("dump_end_tag----!");
}


int dump_parse_xml()
{
    struct dump_parse_state state;
    XML_Parser parser;
    FILE *file;
    int bytes_read;
    void *buf;
    int i;
    int ret = 0;

	XDUMP_TRACE("enter dump_parse_xml----");


	memset(&aud_dump_file_info ,0 , DUMP_FILE_MAX_NUM * sizeof(aud_dump_item_t ));
	
	//property_set(DUMP_PATH_PROPERTY,"/data/");
	//property_set(DUMP_SWITCH_PROPERTY,"0771");
	
		state.aud_dmp_info = calloc(1, sizeof(aud_dump_t));
		if (!state.aud_dmp_info)
		{
			ret = -ENOMEM;
			goto err_calloc;
		}
		state.aud_dmp_info->dump_type= 0;
		state.aud_dmp_info->num = 0;
		state.aud_dmp_info->dump_switch=0;
		state.aud_dmp_info->path_name[0]  = '\0';

	
	
    file = fopen(AUDIO_XML_PATH, "r");
    if (!file) {
        ALOGE("Failed to open %s", AUDIO_XML_PATH);
        ret = -ENODEV;
        goto err_fopen;
    }

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        ALOGE("Failed to create XML parser");
        ret = -ENOMEM;
        goto err_parser_create;
    }



    XML_SetUserData(parser, &state);
    XML_SetElementHandler(parser, dump_start_tag, dump_end_tag);

    for (;;) {
        buf = XML_GetBuffer(parser, BUF_SIZE);
        if (buf == NULL)
        {
            ret = -EIO;
            goto err_parse;
        }
        bytes_read = fread(buf, 1, BUF_SIZE, file);
        if (bytes_read < 0)
        {
            ret = -EIO;
            goto err_parse;
        }
        if (XML_ParseBuffer(parser, bytes_read,
                    bytes_read == 0) == XML_STATUS_ERROR) {
            ALOGE("Error in codec PGA xml (%s)", AUDIO_XML_PATH);
            ret = -EINVAL;
            goto err_parse;
        }

        if (bytes_read == 0)
            break;
    }
	XDUMP_TRACE("enter dump_parse_xml----end");
    if(NULL != parser)
	    XML_ParserFree(parser);
    fclose(file);
    return ret;

err_parse:
    XML_ParserFree(parser);
err_parser_create:
    fclose(file);
err_fopen:
err_calloc:
		if(state.aud_dmp_info)
		{
			free(state.aud_dmp_info);
			state.aud_dmp_info = NULL;
		}

    return ret;
}





static int dump_open_file(int index )
{
	char AUDIO_OUT_FILE_PATH[PROPERTY_VALUE_MAX]; //"data/audio_out.pcm"
	char   attr[DUMP_FILE_MAX_NUM*2];
	char  * w_file_path = NULL;
	int 	ret= 0 ;
	//aud_dump_file_info[index].fp =  NULL;
	AUDIO_OUT_FILE_PATH[0] = '\0';
	
	property_get(DUMP_PATH_PROPERTY	 , AUDIO_OUT_FILE_PATH, NULL);

	//XDUMP_TRACE(" dump_switch  = %x %s" , dump_switch,AUDIO_OUT_FILE_PATH);
	
	if( '\0' ==  AUDIO_OUT_FILE_PATH[0] )
		return -1;
	
	//XDUMP_TRACE(" dump_path =  %s %x" , aud_dump_file_info[index].file_name , aud_dump_file_info[index].fp);
	//if ( NULL == aud_dump_file_info[index].file_name)
	//	return -2;

		if( dump_switch &  (1<<index) )
		{
			if( NULL != aud_dump_file_info[index].fp )
				return -3;
			w_file_path = strcat(AUDIO_OUT_FILE_PATH, aud_dump_file_info[index].file_name);
			//XDUMP_TRACE(" w_file_path =  %s " , w_file_path);
			if( NULL != w_file_path )
			{
				aud_dump_file_info[index].fp =  fopen(w_file_path, "wb+");
				aud_dump_file_info[index].wr_file_size = 0;
				XDUMP_TRACE(" DUMP FILE PATH =%s file name + %s ,AUDIO_OUT_fd[i] = %x ,index =%d",w_file_path ,aud_dump_file_info[index].file_name, aud_dump_file_info[index].fp , index);
			}
		}




	return 1;
}

int  dump_data(dump_data_info_t dump_info)
{
	
	FILE  *fd ;
	char  attr[PROPERTY_VALUE_MAX];
	int ret = 0;
	property_get(DUMP_SWITCH_PROPERTY , attr, NULL);

	ret = sscanf(attr , "%x",&(dump_switch));
	if (ret <0 )
		ALOGE(" dump switch is error!");

	XDUMP_TRACE(" dump_switch == %x " , dump_switch);
	if( ( total_switch != 1) || (dump_switch == 0))
		return 0;
	
	XDUMP_TRACE("dump_data   -----fp = %x , file name =%s, index = %d" ,aud_dump_file_info[dump_info.dump_switch_info].fp  ,aud_dump_file_info[dump_info.dump_switch_info].file_name,dump_info.dump_switch_info);

	pthread_mutex_lock(&Dumplock);
	dump_open_file(dump_info.dump_switch_info);
	

	if( DUMP_FILE_MAX_SIZE <= aud_dump_file_info[dump_info.dump_switch_info].wr_file_size ){
		if( NULL != aud_dump_file_info[dump_info.dump_switch_info].fp ){
			fclose(aud_dump_file_info[dump_info.dump_switch_info].fp);
			aud_dump_file_info[dump_info.dump_switch_info].fp = NULL;
		}
		dump_open_file(dump_info.dump_switch_info);
		//pthread_mutex_unlock(&Dumplock);	
		//return 0;
		}
	
	fd = aud_dump_file_info[dump_info.dump_switch_info].fp;

	XDUMP_TRACE("dump_data buf_len=%d, fd=0x%x, index=%d",
	    dump_info.buf_len,
	    fd,
	    dump_info.dump_switch_info);

	if( NULL == fd ){
		pthread_mutex_unlock(&Dumplock);	
		return 0;
		}


	fwrite(dump_info.buf,dump_info.buf_len,1,fd);

	aud_dump_file_info[dump_info.dump_switch_info].wr_file_size += dump_info.buf_len;
	
	pthread_mutex_unlock(&Dumplock);	
	return 1;
}


int dump_close_file()
{
	int	i;
	char * file_path;
	for( i = 0 ; i < DUMP_FILE_MAX_NUM ; i ++)
	{
		if( NULL !=  aud_dump_file_info[i].fp)
		{
			 
			fclose(aud_dump_file_info[i].fp);
			aud_dump_file_info[i].fp = NULL;
		}

	}
	return 1;

}




