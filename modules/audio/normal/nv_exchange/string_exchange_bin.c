/**********************code following is for temp,need to modify,by wz*********************/

#define LOG_TAG "string_exchange_bin"
/*#define LOG_NDEBUG 0*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "string_exchange_bin.h"
#include <cutils/log.h>
#include <expat.h>
#include <sys/types.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
aud_mode_t *s_audiomode =NULL;

//#define PARSE_DEBUG
#ifdef PARSE_DEBUG
#define PARSE_LOG ALOGW
#else
#define PARSE_LOG ALOGV
#endif
static aud_mode_t  * parse(void);

int adev_get_audiomodenum4eng(void)
{
    if(!s_audiomode)
    {
        s_audiomode = parse();
    }
    ALOGV("adev_get_audiomodenum4eng s_audionum:0x%0x\n",s_audiomode->num);
    return s_audiomode->num;
}

static  audio_mode_item_t *audmode_create(aud_mode_t  *aud_mode, const char *num)
{
    if (!atoi((char *)num)) {
        ALOGE("Unnormal mode num!");
        return NULL;
    }
    aud_mode->num = atoi((char *)num);
    /* check if we need to allocate  space for modem profile */
    if(!aud_mode->audio_mode_item_info)
    {
        aud_mode->audio_mode_item_info = malloc(aud_mode->num *
                sizeof(audio_mode_item_t));
        if (aud_mode->audio_mode_item_info == NULL)
        {
            ALOGE("Unable to allocate modem profiles");
            return NULL;
        }
        else
        {
            /* initialise the new profile */
            memset((void*)aud_mode->audio_mode_item_info,0x00,aud_mode->num *
                    sizeof(audio_mode_item_t));
        }
    }
    /* return the profile just added */
    return aud_mode->audio_mode_item_info;
}

void adev_free_audmode(void)
{
    if(s_audiomode)
    {
        if(s_audiomode->audio_mode_item_info)
        {
            free(s_audiomode->audio_mode_item_info);
            s_audiomode->audio_mode_item_info = NULL;
        }
        free(s_audiomode);
        s_audiomode = NULL;
    }
}
static void start_tag(void *data, const XML_Char *tag_name,
        const XML_Char **attr)
{
    struct modem_config_parse_state *state = data;
    aud_mode_t *aud_mode= state->audio_mode_info;
    unsigned int i;
    int value;

    /* Look at tags */
    if(strcmp(tag_name, "audiomode") == 0)
    {
        /* Obtain the modem num */
        if (strcmp(attr[0], "num") == 0) {
            ALOGV("Thse num of audio mode  is '%s'", attr[1]);
            state->audio_mode_item_info = audmode_create(aud_mode, attr[1]);
        } else {
            ALOGE("no audio mode num!");
        }
    }
    else if(strcmp(tag_name, "item") == 0)
    {
        if (state->audio_mode_item_info) {
            /* Obtain the modem name  \pipe\vbc   filed */
            if (strcmp(attr[0], "name") != 0) {
                ALOGE("Unnamed modem!");
                goto attr_err;
            }
            if (strcmp(attr[2], "index") != 0) {
                ALOGE("'%s' No index filed!", attr[1]);
                goto attr_err;
            }
            ALOGV("the mode name is '%s',index is '%s'", attr[1],attr[3]);
            memcpy((void*)state->audio_mode_item_info->mode_name,(void*)attr[1],(strlen((char *)attr[1]) < AUDIO_MODE_NAME_MAX_LEN ? strlen((char *)attr[1]): AUDIO_MODE_NAME_MAX_LEN));
            state->audio_mode_item_info->index = atoi((char *)attr[3]);
            state->audio_mode_item_info++;

        } else {
            ALOGE("error profile!");
        }
    }
attr_err:
    return;
}
static void end_tag(void *data, const XML_Char *tag_name)
{
    struct modem_config_parse_state *state = data;
}

static aud_mode_t  *parse(void)
{
    struct modem_config_parse_state state;
    XML_Parser parser;
    FILE *file;
    int bytes_read;
    void *buf;
    int i;
    int ret = 0;
    if(!s_audiomode)
    {
        s_audiomode = calloc(1, sizeof(aud_mode_t));
        if (!s_audiomode)
        {
            ret = -ENOMEM;
            goto err_calloc;
        }
        s_audiomode->num = 0;
        s_audiomode->audio_mode_item_info = NULL;
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
        memset(&state, 0, sizeof(state));
        state.audio_mode_info = s_audiomode;
        XML_SetUserData(parser, &state);
        XML_SetElementHandler(parser, start_tag, end_tag);
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
        XML_ParserFree(parser);
        fclose(file);
    }
    return s_audiomode;
err_parse:
    XML_ParserFree(parser);
err_parser_create:
    fclose(file);
err_fopen:
err_calloc:
    adev_free_audmode();
    return s_audiomode;
}
int my_strtol(char *src)
{
    int len = 0;
    int value = 0;
    int dest_data[8] = {0};
    int i=0;
    src+=2;
    int flag = 0;

    for(i=0; i<8; i++)
    {
        int src_data = 0;
        flag = 0;
        switch(src[i])
        {
            case '0':
                src_data = 0;
                break;
            case '1':
                src_data = 1;
                break;
            case '2':
                src_data = 2;
                break;
            case '3':
                src_data = 3;
                break;
            case '4':
                src_data = 4;
                break;
            case '5':
                src_data = 5;
                break;
            case '6':
                src_data = 6;
                break;
            case '7':
                src_data = 7;
                break;
            case '8':
                src_data = 8;
                break;
            case '9':
                src_data = 9;
                break;
            case 'a':
            case 'A':
                src_data = 10;
                break;
            case 'b':
            case 'B':
                src_data = 11;
                break;
            case 'c':
            case 'C':
                src_data = 12;
                break;
            case 'd':
            case 'D':
                src_data = 13;
                break;
            case 'e':
            case 'E':
                src_data = 14;
                break;
            case 'f':
            case 'F':
                src_data = 15;
                break;
            default:
                flag = 1;
                break;
        }
        //value |= (src_data<<(28-i*4));
        dest_data[i] = src_data;
        if(flag==1)
        {
            break;
        }
        else
        {
            len++;
        }
    }

    for(i=0; i<len; i++)
    {
        value |= (dest_data[i]<<((len-i-1)*4));
    }

    return value;
}

void stringfile2nvstruct(char *filename, void *para_ptr, int lenbytes)
{
    char nvLine[1024] = {0};
    char outputstyle[NAME_LEN_MAX] = {0};
    int i=0;
    FILE *fpRead = NULL;/////////////////////////todo
    //short iValue = 0;
    int iValue = 0;
    int output_arm = -1;
    int output_eq = -1;
    //int output_arm_before = -1;
    //int output_eq_before = -1;
    int index_dev_set = 0;
    int index_aud_proc_exp_control = 0;
    int index_agc_input_gain = 0;
    int index_arm_volume = 0;

    int index_reserve_1 = 0;
    int index_reserve_3 = 0;

    int index_extendArray = 0;
    int index_nrArray = 0;
    int idx = 0;

    char* lpFind_arm_arm =	NULL;
    char* lpFind_eq_eq =  NULL;
    char* lpFind_eq_eq_eq = NULL;
    char* lp_Find  = NULL;

    char nvtemp[128] = {0};
    char* lpFindtemp = NULL;
    char data[128] = {0};

    AUDIO_TOTAL_T * aud_params_ptr = (AUDIO_TOTAL_T *)para_ptr;
    aud_mode_t *aud_mode = NULL;
    int curLine=0, len = 0;
    aud_mode = parse();
    len = sizeof(AUDIO_TOTAL_T)*adev_get_audiomodenum4eng();
    fpRead = fopen(filename, "r");
    if (NULL == fpRead)
    {
        ALOGW("file %s open failed\n",filename);
        return;
    }
    ALOGI("stringfile2nvstruct :%d ----> %s\n",aud_mode->num,(aud_mode->audio_mode_item_info+aud_mode->num-1)->mode_name);
    while(!feof(fpRead))////////////////////todo
    {
        curLine++;
        PARSE_LOG("stringfile2nvstruct pre item iValue:%d\n", iValue);
        if(fgets(nvLine,1024,fpRead) != NULL)///////////////////////todo
        {
            PARSE_LOG("stringfile2nvstruct :nvLine ----> %s\n",nvLine);

            if(strstr(nvLine,"audio_arm\\audio_arm\\") != NULL)
            {
                if(sscanf(nvLine,"audio_arm\\audio_arm\\%[^\\]",outputstyle) != 0)
                {
                    for(idx=0;idx < aud_mode->num;idx ++)
                    {
                        char* modename = (aud_mode->audio_mode_item_info +idx)->mode_name;
                        if(strcmp(outputstyle,modename) == 0)
                        {
                            output_arm = (aud_mode->audio_mode_item_info +idx)->index;
                            PARSE_LOG("stringfile2nvstruct modename:%s ----> %d, line:%d\n",modename,output_arm, curLine);
                            break;
                        }
                    }
                    //aud_params_ptr[output_arm].audio_nv_arm_mode_info
                    len = strlen(outputstyle);
                    if (len > NAME_LEN_MAX - 1){
                        len = NAME_LEN_MAX - 1;
                        ALOGE("parse wrong arm,outputstyle too long");
                    }

                    memcpy(aud_params_ptr[output_arm].audio_nv_arm_mode_info.ucModeName,outputstyle,len);
                    aud_params_ptr[output_arm].audio_nv_arm_mode_info.ucModeName[len] = '\0';

                    /*********************do the arm parsing following*******************/
                    if(strstr(nvLine,"AudioStructure\\midi_opt") != NULL)
                    {
                        //get the true value of each item
                        memset(nvtemp,0,128);
                        lp_Find = strchr(nvLine,'=');
                        if(lp_Find != NULL)
                        {
                            strcpy(nvtemp,lp_Find+1);
                        }
                        if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                        {
                            char *pEnd = NULL;
                            iValue = strtol(nvtemp,&pEnd,16);
                        }
                        else
                        {
                            iValue = atoi(nvtemp);
                        }
                        aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.midi_opt = iValue;
                        continue;
                    }

                    if(strstr(nvLine,"AudioStructure\\aud_dev") != NULL)
                    {
                        //get the true value of each item
                        memset(nvtemp,0,128);
                        lp_Find = strchr(nvLine,'=');
                        if(lp_Find != NULL)
                        {
                            strcpy(nvtemp,lp_Find+1);
                        }
                        if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                        {
                            char *pEnd = NULL;
                            iValue = (short)strtol(nvtemp,&pEnd,16);
                        }
                        else
                        {
                            iValue = (short)atoi(nvtemp);
                        }
                        aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.aud_dev = iValue;
                        PARSE_LOG("aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.aud_dev: 0x%x \n",
                                aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.aud_dev);
                        continue;
                    }

                    if(strstr(nvLine,"AudioStructure\\reserve") != NULL)
                    {
                        //get the true value of each item
                        memset(nvtemp,0,128);
                        lp_Find = strchr(nvLine,'=');
                        if(lp_Find != NULL)
                        {
                            strcpy(nvtemp,lp_Find+1);
                        }
                        if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                        {
                            char *pEnd = NULL;
                            iValue = (short)strtol(nvtemp,&pEnd,16);
                        }
                        else
                        {
                            iValue = (short)atoi(nvtemp);
                        }
                        aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.reserve[index_reserve_1] = iValue;
                        index_reserve_1++;
                        if(index_reserve_1 == AUDIO_NV_ARM_PARA_RESERVE)
                            index_reserve_1 = 0;
                        continue;
                    }
                    if(strstr(nvLine,"dev_path_set\\valid_dev_set_count") != NULL)
                    {
                        //get the true value of each item
                        memset(nvtemp,0,128);
                        lp_Find = strchr(nvLine,'=');
                        if(lp_Find != NULL)
                        {
                            strcpy(nvtemp,lp_Find+1);
                        }
                        if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                        {
                            char *pEnd = NULL;
                            iValue = (short)strtol(nvtemp,&pEnd,16);
                        }
                        else
                        {
                            iValue = (short)atoi(nvtemp);
                        }
                        aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.dev_path_set.valid_dev_set_count = iValue;
                        continue;
                    }
                    //lpFind_arm_arm = strstr(nvLine,"dev_path_set\reserve");
                    if(strstr(nvLine,"dev_path_set\\reserve") != NULL)
                    {
                        //get the true value of each item
                        memset(nvtemp,0,128);
                        lp_Find = strchr(nvLine,'=');
                        if(lp_Find != NULL)
                        {
                            strcpy(nvtemp,lp_Find+1);
                        }
                        if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                        {
                            char *pEnd = NULL;
                            iValue = (short)strtol(nvtemp,&pEnd,16);
                        }
                        else
                        {
                            iValue = (short)atoi(nvtemp);
                        }
                        aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.dev_path_set.reserve = iValue;
                        continue;
                    }
                    //lpFind_arm_arm = strstr(nvLine,"dev_path_set\dev_set");
                    if(strstr(nvLine,"dev_path_set\\dev_set") != NULL)
                    {
                        //get the true value of each item
                        memset(nvtemp,0,128);
                        lp_Find = strchr(nvLine,'=');
                        if(lp_Find != NULL)
                        {
                            strcpy(nvtemp,lp_Find+1);
                        }
                        if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                        {
                            char *pEnd = NULL;
                            iValue = (short)strtol(nvtemp,&pEnd,16);
                        }
                        else
                        {
                            iValue = (short)atoi(nvtemp);
                        }
                        aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.dev_path_set.dev_set[index_dev_set] = iValue;
                        index_dev_set++;
                        if(index_dev_set == AUDIO_DEVICE_MODE_PARAM_MAX)
                            index_dev_set = 0;
                        continue;
                    }
                    //lpFind_arm_arm = strstr(nvLine,"app_config_info_set\valid_app_set_count");
                    if(strstr(nvLine,"app_config_info_set\\valid_app_set_count") != NULL)
                    {
                        //get the true value of each item
                        memset(nvtemp,0,128);
                        lp_Find = strchr(nvLine,'=');
                        if(lp_Find != NULL)
                        {
                            strcpy(nvtemp,lp_Find+1);
                        }
                        if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                        {
                            char *pEnd = NULL;
                            iValue = (short)strtol(nvtemp,&pEnd,16);
                        }
                        else
                        {
                            iValue = (short)atoi(nvtemp);
                        }
                        aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.valid_app_set_count = iValue;
                        continue;
                    }
                    //lpFind_arm_arm = strstr(nvLine,"app_config_info_set\valid_agc_input_gain_count");
                    if(strstr(nvLine,"app_config_info_set\\valid_agc_input_gain_count") != NULL)
                    {
                        //get the true value of each item
                        memset(nvtemp,0,128);
                        lp_Find = strchr(nvLine,'=');
                        if(lp_Find != NULL)
                        {
                            strcpy(nvtemp,lp_Find+1);
                        }
                        if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                        {
                            char *pEnd = NULL;
                            iValue = (short)strtol(nvtemp,&pEnd,16);
                        }
                        else
                        {
                            iValue = (short)atoi(nvtemp);
                        }
                        aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.valid_agc_input_gain_count = iValue;
                        continue;
                    }
                    //lpFind_arm_arm = strstr(nvLine,"app_config_info_set\aud_proc_exp_control");
                    if(strstr(nvLine,"app_config_info_set\\aud_proc_exp_control") != NULL)
                    {
                        //get the true value of each item
                        memset(nvtemp,0,128);
                        lp_Find = strchr(nvLine,'=');
                        if(lp_Find != NULL)
                        {
                            strcpy(nvtemp,lp_Find+1);
                        }
                        if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                        {
                            char *pEnd = NULL;
                            iValue = (short)strtol(nvtemp,&pEnd,16);
                        }
                        else
                        {
                            iValue = (short)atoi(nvtemp);
                        }
                        aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.aud_proc_exp_control[index_aud_proc_exp_control] = iValue;
                        index_aud_proc_exp_control++;
                        if(index_aud_proc_exp_control == 2)
                            index_aud_proc_exp_control=0;
                        continue;
                    }
                    lpFindtemp = strstr(nvLine,"app_config_info_set\\app_config_info\\app_config_info[");
                    int j = strlen("app_config_info_set\\app_config_info\\app_config_info[");
                    if(lpFindtemp != NULL)
                    {
                        sscanf(lpFindtemp+j,"%[0-9]%[^]]",data);// tested.
                        int tmpdata = (short)atoi(data);
                        if(strstr(lpFindtemp,"eq_switch") != NULL)//2_lpFind = strstr(lpFindtemp,"eq_switch");
                        {
                            //get the true value of each item
                            memset(nvtemp,0,128);
                            lp_Find = strchr(lpFindtemp,'=');
                            if(lp_Find != NULL)
                            {
                                strcpy(nvtemp,lp_Find+1);
                            }
                            if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                            {
                                char *pEnd = NULL;
                                iValue = (short)strtol(nvtemp,&pEnd,16);
                            }
                            else
                            {
                                iValue = (short)atoi(nvtemp);
                            }
                            aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[tmpdata].eq_switch = iValue;
                            continue;
                        }
                        if(strstr(lpFindtemp,"agc_input_gain") != NULL)
                        {
                            //get the true value of each item
                            memset(nvtemp,0,128);
                            lp_Find = strchr(lpFindtemp,'=');
                            if(lp_Find != NULL)
                            {
                                strcpy(nvtemp,lp_Find+1);
                            }
                            if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                            {
                                char *pEnd = NULL;
                                iValue = (short)strtol(nvtemp,&pEnd,16);
                            }
                            else
                            {
                                iValue = (short)atoi(nvtemp);
                            }
                            aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[tmpdata].agc_input_gain[index_agc_input_gain] = iValue;
                            index_agc_input_gain++;
                            if(index_agc_input_gain == AUDIO_AGC_INPUG_GAIN_MAX)
                                index_agc_input_gain = 0;
                            continue;
                        }
                        if(strstr(lpFindtemp,"valid_volume_level_count") != NULL)
                        {
                            //get the true value of each item
                            memset(nvtemp,0,128);
                            lp_Find = strchr(lpFindtemp,'=');
                            if(lp_Find != NULL)
                            {
                                strcpy(nvtemp,lp_Find+1);
                            }
                            if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                            {
                                char *pEnd = NULL;
                                iValue = strtol(nvtemp,&pEnd,16);
                            }
                            else
                            {
                                iValue = atoi(nvtemp);
                            }
                            aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[tmpdata].valid_volume_level_count = iValue;
                            continue;
                        }
                        if(strstr(lpFindtemp,"arm_volume") != NULL)
                        {
                            //get the true value of each item
                            char *pdestvol = NULL;
                            memset(nvtemp,0,128);
                            lp_Find = strchr(lpFindtemp,'=');
                            if(lp_Find != NULL)
                            {
                                strcpy(nvtemp,lp_Find+1);
                            }
                            if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                            {
                                if(strstr(nvtemp,"0x") != NULL)
                                {
                                    pdestvol = strstr(nvtemp,"0x");
                                }
                                else
                                {
                                    pdestvol = strstr(nvtemp,"0X");
                                }
                                iValue = my_strtol(pdestvol);
                            }
                            else
                            {
                                iValue = atoi(nvtemp);
                            }
                            aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[tmpdata].arm_volume[index_arm_volume] = iValue;
                            index_arm_volume++;
                            if(index_arm_volume == AUDIO_ARM_VOLUME_LEVEL)
                                index_arm_volume  = 0;
                            continue;
                        }
                        if(strstr(lpFindtemp,"reserve") != NULL)
                        {
                            //get the true value of each item
                            memset(nvtemp,0,128);
                            lp_Find = strchr(lpFindtemp,'=');
                            if(lp_Find != NULL)
                            {
                                strcpy(nvtemp,lp_Find+1);
                            }
                            if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                            {
                                char *pEnd = NULL;
                                iValue = (short)strtol(nvtemp,&pEnd,16);
                            }
                            else
                            {
                                iValue = (short)atoi(nvtemp);
                            }
                            aud_params_ptr[output_arm].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[tmpdata].reserve[index_reserve_3] = iValue;
                            index_reserve_3++;
                            if(index_reserve_3 == AUDIO_NV_ARM_APP_PARA_RESERVE)
                                index_reserve_3 = 0;
                            continue;
                        }
                    }
                }
            }
            else if(strstr(nvLine,"%audio\\")!= NULL)
            {
                if(sscanf(nvLine,"%%audio\\%[^\\]",outputstyle) != 0)//tested,it's ok!
                {
                    for(idx=0;idx < aud_mode->num;idx ++)
                    {
                        char* modename = (aud_mode->audio_mode_item_info +idx)->mode_name;
                        if(strcmp(outputstyle+strlen("EQ_"),modename) == 0)
                        {
                            output_eq = (aud_mode->audio_mode_item_info +idx)->index;
                            PARSE_LOG("stringfile2nvstruct eq modename:%s ----> %d, curLine:%d\n",modename,output_eq, curLine);
                            break;
                        }
                    }

                    //if(output_eq != output_eq_before)
                    //{//aud_params_ptr[output_eq].audio_enha_eq
                    len = strlen(outputstyle);
                    if (len > NAME_LEN_MAX - 1){
                        len = NAME_LEN_MAX - 1;
                        ALOGE("parse wrong eq,outputstyle too long");
                    }

                    memcpy(aud_params_ptr[output_eq].audio_enha_eq.para_name,outputstyle,len);//todo: do the confirm to shujing
                    aud_params_ptr[output_eq].audio_enha_eq.para_name[len] = '\0';
                    //}
                    //output_eq_before = output_eq;
                    /**********************do the eq parsing following***********************/
                    if(strstr(nvLine,"eq_control") != NULL)
                    {
                        memset(nvtemp,0,128);
                        lp_Find = strchr(nvLine,'=');
                        if(lp_Find != NULL)
                        {
                            strcpy(nvtemp,lp_Find+1);
                        }
                        if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                        {
                            char *pEnd = NULL;
                            iValue = (short)strtol(nvtemp,&pEnd,16);
                        }
                        else
                        {
                            iValue = (short)atoi(nvtemp);
                        }
                        aud_params_ptr[output_eq].audio_enha_eq.eq_control = iValue;
                        continue;
                    }

                    lpFind_eq_eq = strstr(nvLine,"eq_mode_");
                    if(lpFind_eq_eq != NULL)
                    {
                        sscanf(lpFind_eq_eq,"eq_mode_%[1-9]%[^\\]",data);//tested
                        int tmpdata = atoi(data);
                        if(strstr(lpFind_eq_eq,"agc_in_gain") != NULL)
                        {
                            memset(nvtemp,0,128);
                            lp_Find = strchr(nvLine,'=');
                            if(lp_Find != NULL)
                            {
                                strcpy(nvtemp,lp_Find+1);
                            }
                            if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                            {
                                char *pEnd = NULL;
                                iValue = strtol(nvtemp,&pEnd,16);
                            }
                            else
                            {
                                iValue = atoi(nvtemp);
                            }
                            aud_params_ptr[output_eq].audio_enha_eq.eq_modes[tmpdata-1].agc_in_gain = iValue;
                            continue;
                        }
                        if(strstr(lpFind_eq_eq,"band_control") != NULL)
                        {
                            memset(nvtemp,0,128);
                            lp_Find = strchr(nvLine,'=');
                            if(lp_Find != NULL)
                            {
                                strcpy(nvtemp,lp_Find+1);
                            }
                            if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                            {
                                char *pEnd = NULL;
                                iValue = strtol(nvtemp,&pEnd,16);
                            }
                            else
                            {
                                iValue = atoi(nvtemp);
                            }
                            aud_params_ptr[output_eq].audio_enha_eq.eq_modes[tmpdata-1].band_control = iValue;
                            continue;
                        }
                        lpFind_eq_eq_eq = strstr(lpFind_eq_eq,"eq_band_");
                        if(lpFind_eq_eq_eq != NULL)
                        {
                            sscanf(lpFind_eq_eq_eq,"eq_band_%[0-9]%[^\\]",data);//tested
                            int tmpdata_data = (short)atoi(data);
                            if(strstr(lpFind_eq_eq_eq,"fo") != NULL)
                            {
                                memset(nvtemp,0,128);
                                lp_Find = strchr(nvLine,'=');
                                if(lp_Find != NULL)
                                {
                                    strcpy(nvtemp,lp_Find+1);
                                }
                                if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                                {
                                    char *pEnd = NULL;
                                    iValue = (short)strtol(nvtemp,&pEnd,16);
                                }
                                else
                                {
                                    iValue = (short)atoi(nvtemp);
                                }
                                aud_params_ptr[output_eq].audio_enha_eq.eq_modes[tmpdata-1].eq_band[tmpdata_data-1].fo = iValue;
                                continue;
                            }
                            if(strstr(lpFind_eq_eq_eq+9,"q") != NULL)//puls 9,because "q" is not unique,from eq_band_x.
                            {
                                memset(nvtemp,0,128);
                                lp_Find = strchr(nvLine,'=');
                                if(lp_Find != NULL)
                                {
                                    strcpy(nvtemp,lp_Find+1);
                                }
                                if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                                {
                                    char *pEnd = NULL;
                                    iValue = (short)strtol(nvtemp,&pEnd,16);
                                }
                                else
                                {
                                    iValue = (short)atoi(nvtemp);
                                }
                                aud_params_ptr[output_eq].audio_enha_eq.eq_modes[tmpdata-1].eq_band[tmpdata_data-1].q = iValue;
                                continue;
                            }
                            if(strstr(lpFind_eq_eq_eq,"boostdB") != NULL)
                            {
                                memset(nvtemp,0,128);
                                lp_Find = strchr(nvLine,'=');
                                if(lp_Find != NULL)
                                {
                                    strcpy(nvtemp,lp_Find+1);
                                }
                                if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                                {
                                    char *pEnd = NULL;
                                    iValue = (short)strtol(nvtemp,&pEnd,16);
                                }
                                else
                                {
                                    iValue = (short)atoi(nvtemp);
                                }
                                aud_params_ptr[output_eq].audio_enha_eq.eq_modes[tmpdata-1].eq_band[tmpdata_data-1].boostdB = iValue;
                                continue;
                            }
                            if(strstr(lpFind_eq_eq_eq,"gaindB") != NULL)
                            {
                                memset(nvtemp,0,128);
                                lp_Find = strchr(nvLine,'=');
                                if(lp_Find != NULL)
                                {
                                    strcpy(nvtemp,lp_Find+1);
                                }
                                if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                                {
                                    char *pEnd = NULL;
                                    iValue = (short)strtol(nvtemp,&pEnd,16);
                                }
                                else
                                {
                                    iValue = (short)atoi(nvtemp);
                                }
                                aud_params_ptr[output_eq].audio_enha_eq.eq_modes[tmpdata-1].eq_band[tmpdata_data-1].gaindB = iValue;
                                continue;
                            }

                        }
                    }
                    if(strstr(nvLine,"extendArray") != NULL)
                    {
                        memset(nvtemp,0,128);
                        lp_Find = strchr(nvLine,'=');
                        if(lp_Find != NULL)
                        {
                            strcpy(nvtemp,lp_Find+1);
                        }
                        if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                        {
                            char *pEnd = NULL;
                            iValue = (short)strtol(nvtemp,&pEnd,16);
                        }
                        else
                        {
                            iValue = (short)atoi(nvtemp);
                        }
                        aud_params_ptr[output_eq].audio_enha_eq.externdArray[index_extendArray] = iValue;
                        index_extendArray++;
                        if(index_extendArray == 59)
                            index_extendArray = 0;
                        continue;
                    }
#ifdef NRARRAY_ANALYSIS
                    if(strstr(nvLine,"nrArray") != NULL)
                    {
                        memset(nvtemp,0,128);
                        lp_Find = strchr(nvLine,'=');
                        if(lp_Find != NULL)
                        {
                            strcpy(nvtemp,lp_Find+1);
                        }
                        if(strstr(nvtemp,"0x") != NULL||strstr(nvtemp,"0X") != NULL)
                        {
                            char *pEnd = NULL;
                            iValue = (short)strtol(nvtemp,&pEnd,16);
                        }
                        else
                        {
                            iValue = (short)atoi(nvtemp);
                        }
                        aud_params_ptr[output_eq].audio_enha_eq.nrArray[index_nrArray] = iValue;
                        index_nrArray++;
                        if(index_nrArray == 8)
                            index_nrArray = 0;
                        continue;
                    }
#endif
                }
            }
            else
            {
                continue;
            }
        }
    }
    fclose(fpRead);
    {
        int i=0;
        for(i=0; i<aud_mode->num;i++)
        {
            ALOGV("%d ------> %s",i,aud_params_ptr[i].audio_nv_arm_mode_info.ucModeName);
        }
    }
    ALOGV("stringfile2nvstruct end\n");
    return;
}

/*****************for test,by wz,end******************/

void  nvstruct2stringfile(char* filename,void *para_ptr, int lenbytes)
{
    int i = 0;
    int k = 0;
    int j0 = 0;
    int j1 = 0;
    int j2 = 0;
    int j3 = 0;
    int j4 = 0;
    int j5 = 0;
    int j6 = 0;

    char *arm_name = NULL;//={{"Headset"},{"Headfree"},{"Handset"},{"Handsfree"}};
    char *eq_name = NULL; //={{"EQ_Headset"},{"EQ_Headfree"},{"EQ_Handset"},{"EQ_Handsfree"}};
    unsigned int ivalue = 0;
    char valuebuf[128] = {0};
    char nvline[1024] = {0};
    FILE* file= NULL;
    char numbuf[10] = {0};
    char numbuf1[10] = {0};
    AUDIO_TOTAL_T * audio_total = NULL;
    aud_mode_t *aud_mode = NULL;
    char* modename = NULL;
    int idx = 0;
    char *arm_name_pc = NULL;
    char *eq_name_pc = NULL;
    int need_cpy_len = 0;
    int f=0;
    //char numbuf2[10] = {0};
    int aud_modenum = adev_get_audiomodenum4eng();
    int destfd = 0;

    arm_name = calloc(aud_modenum,NAME_LEN_MAX);
    if(!arm_name)
    {
        ALOGW("nvstruct2stringfile modename malloc armname  memory error\n");
        return;
    }

    eq_name = calloc(aud_modenum,NAME_LEN_MAX);
    if(!eq_name)
    {
        free(arm_name);
        ALOGW("nvstruct2stringfile modename malloc  eqname memory error\n");
        return;
    }
    memset(arm_name,0x00,NAME_LEN_MAX*aud_modenum);
    memset(eq_name,0x00,NAME_LEN_MAX*aud_modenum);

    aud_mode = parse();
    //inital audioname and eq name
    arm_name_pc = arm_name;
    eq_name_pc = eq_name;
    for(idx=0;idx<aud_modenum;idx++)
    {

        modename = (aud_mode->audio_mode_item_info +idx)->mode_name;
        memcpy(arm_name_pc,modename,NAME_LEN_MAX);
        ALOGW("nvstruct2stringfile arm_name_pc:%s\n",arm_name_pc);
        arm_name_pc+= NAME_LEN_MAX;

        memcpy(eq_name_pc,"EQ_",strlen("EQ_"));
        memcpy(eq_name_pc+strlen("EQ_"),modename,NAME_LEN_MAX - strlen("EQ_"));
        ALOGW("nvstruct2stringfile eq_name_pc:%s\n",eq_name_pc);
        eq_name_pc+= NAME_LEN_MAX;
    }

    for(f=0;f<aud_mode->num;f++)
    {
        ALOGW("nvstruct2stringfile :%d ----> %s,%s,%s\n",aud_mode->num,(aud_mode->audio_mode_item_info+f)->mode_name,&arm_name[i*NAME_LEN_MAX],&eq_name[i*NAME_LEN_MAX]);
    }
    if((unsigned int)lenbytes!=(sizeof(AUDIO_TOTAL_T)*aud_modenum))
    {
        ALOGE("nvstruct2stringfile len is not ok:%d, %d.\n",
                lenbytes, (sizeof(AUDIO_TOTAL_T)*aud_modenum));
        free(arm_name);
        free(eq_name);
        return;
    }
    audio_total = (AUDIO_TOTAL_T *)para_ptr;
    file = fopen(filename,"a+");//
    if(file)
    {
        fclose(file);
        remove(filename);
        ALOGE("nvstruct2stringfile remove outdata file.\n");
    }

    destfd = open(filename,O_CREAT|O_RDWR, 0660);
    if (destfd < 0) {
        ALOGE("file Cannot create \"%s\": %s", filename, strerror(errno));
        free(arm_name);
        free(eq_name);
        return;
    }
    close(destfd);
    if(chmod(filename, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH) != 0) {
        ALOGE("nvstruct2stringfile Cannot set RW to \"%s\": %s", filename, strerror(errno));
    }
    file = fopen(filename,"w");
    if(file == NULL)
    {
        //todo
        ALOGE("open file wrong during writing ");
        free(arm_name);
        free(eq_name);
        return;
    }

    for( i = 0;i<aud_modenum;i++)//
    {

        memset(nvline,0,1024);
        ivalue = audio_total[i].audio_nv_arm_mode_info.tAudioNvArmModeStruct.dev_path_set.valid_dev_set_count;
        sprintf(valuebuf,"%0x",ivalue);
        //"audio_arm\audio_arm\Headset\AudioStructure\dev_path_set\valid_dev_set_count=0x3"
        strcat(nvline,"audio_arm\\audio_arm\\");
        strcat(nvline,&arm_name[i*NAME_LEN_MAX]);
        strcat(nvline,"\\AudioStructure\\dev_path_set\\valid_dev_set_count=0x");
        strcat(nvline,valuebuf);
        fputs(nvline,file);
        fputc('\n',file);

        /********************do things about arm**********************/

        memset(nvline,0,1024);
        memset(valuebuf,0,128);
        ivalue = audio_total[i].audio_nv_arm_mode_info.tAudioNvArmModeStruct.dev_path_set.reserve;
        sprintf(valuebuf,"%0x",ivalue);
        //audio_arm\audio_arm\Headset\AudioStructure\dev_path_set\reserve=0x0
        strcat(nvline,"audio_arm\\audio_arm\\");
        strcat(nvline,&arm_name[i*NAME_LEN_MAX]);
        strcat(nvline,"\\AudioStructure\\dev_path_set\\reserve=0x");
        strcat(nvline,valuebuf);
        fputs(nvline,file);
        fputc('\n',file);

        for( j0 = 0;j0<AUDIO_DEVICE_MODE_PARAM_MAX;j0++)
        {
            memset(numbuf,0,10);
            sprintf(numbuf,"%d",j0);
            memset(nvline,0,1024);
            memset(valuebuf,0,128);
            ivalue = audio_total[i].audio_nv_arm_mode_info.tAudioNvArmModeStruct.dev_path_set.dev_set[j0];
            sprintf(valuebuf,"%0x",ivalue);
            //audio_arm\audio_arm\Headset\AudioStructure\dev_path_set\dev_set\dev_set[0]=0x12
            strcat(nvline,"audio_arm\\audio_arm\\");
            strcat(nvline,&arm_name[i*NAME_LEN_MAX]);
            strcat(nvline,"\\AudioStructure\\dev_path_set\\dev_set\\dev_set[");
            strcat(nvline,numbuf);
            strcat(nvline,"]=0x");
            strcat(nvline,valuebuf);
            fputs(nvline,file);
            fputc('\n',file);
        }

        //audio_arm\audio_arm\Headset\AudioStructure\app_config_info_set\valid_app_set_count=0x1
        memset(nvline,0,1024);
        memset(valuebuf,0,128);
        ivalue = audio_total[i].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.valid_app_set_count;
        sprintf(valuebuf,"%0x",ivalue);
        strcat(nvline,"audio_arm\\audio_arm\\");
        strcat(nvline,&arm_name[i*NAME_LEN_MAX]);
        strcat(nvline,"\\AudioStructure\\app_config_info_set\\valid_app_set_count=0x");
        strcat(nvline,valuebuf);
        fputs(nvline,file);
        fputc('\n',file);

        //audio_arm\audio_arm\Headset\AudioStructure\app_config_info_set\valid_agc_input_gain_count=0x3
        memset(nvline,0,1024);
        memset(valuebuf,0,128);
        ivalue = audio_total[i].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.valid_agc_input_gain_count;
        sprintf(valuebuf,"%0x",ivalue);
        strcat(nvline,"audio_arm\\audio_arm\\");
        strcat(nvline,&arm_name[i*NAME_LEN_MAX]);
        strcat(nvline,"\\AudioStructure\\app_config_info_set\\valid_agc_input_gain_count=0x");
        strcat(nvline,valuebuf);
        fputs(nvline,file);
        fputc('\n',file);

        //audio_arm\audio_arm\Headset\AudioStructure\app_config_info_set\aud_proc_exp_control\aud_proc_exp_control[0]=0x0
        for( j1 = 0;j1<2;j1++)//be careful
        {
            memset(numbuf,0,10);
            sprintf(numbuf,"%d",j1);
            memset(nvline,0,1024);
            memset(valuebuf,0,128);
            ivalue = audio_total[i].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.aud_proc_exp_control[j1];
            sprintf(valuebuf,"%0x",ivalue);
            strcat(nvline,"audio_arm\\audio_arm\\");
            strcat(nvline,&arm_name[i*NAME_LEN_MAX]);
            strcat(nvline,"\\AudioStructure\\app_config_info_set\\aud_proc_exp_control\\aud_proc_exp_control[");
            strcat(nvline,numbuf);
            strcat(nvline,"]=0x");
            strcat(nvline,valuebuf);
            fputs(nvline,file);
            fputc('\n',file);
        }

        //audio_arm\audio_arm\Headset\AudioStructure\app_config_info_set\app_config_info\app_config_info[0]\eq_switch=0xf
        for( j2 = 0;j2<AUDIO_ARM_APP_TYPE_MAX;j2++)
        {
            memset(numbuf,0,10);
            sprintf(numbuf,"%d",j2);
            memset(nvline,0,1024);
            memset(valuebuf,0,128);
            ivalue = audio_total[i].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[j2].eq_switch;
            sprintf(valuebuf,"%0x",ivalue);
            strcat(nvline,"audio_arm\\audio_arm\\");
            strcat(nvline,&arm_name[i*NAME_LEN_MAX]);
            strcat(nvline,"\\AudioStructure\\app_config_info_set\\app_config_info\\app_config_info[");
            strcat(nvline,numbuf);
            strcat(nvline,"]\\eq_switch=0x");
            strcat(nvline,valuebuf);
            fputs(nvline,file);
            fputc('\n',file);

            //audio_arm\audio_arm\Headset\AudioStructure\app_config_info_set\app_config_info\app_config_info[0]\agc_input_gain\agc_input_gain[0]=0x400
            for( k = 0;k<AUDIO_AGC_INPUG_GAIN_MAX;k++)
            {
                memset(numbuf1,0,10);
                sprintf(numbuf1,"%d",k);
                memset(nvline,0,1024);
                memset(valuebuf,0,128);
                ivalue = audio_total[i].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[j2].agc_input_gain[k];
                sprintf(valuebuf,"%0x",ivalue);
                strcat(nvline,"audio_arm\\audio_arm\\");
                strcat(nvline,&arm_name[i*NAME_LEN_MAX]);
                strcat(nvline,"\\AudioStructure\\app_config_info_set\\app_config_info\\app_config_info[");
                strcat(nvline,numbuf);
                strcat(nvline,"]\\agc_input_gain\\agc_input_gain[");
                strcat(nvline,numbuf1);
                strcat(nvline,"]=0x");
                strcat(nvline,valuebuf);
                fputs(nvline,file);
                fputc('\n',file);
            }

            //audio_arm\audio_arm\Headset\AudioStructure\app_config_info_set\app_config_info\app_config_info[1]\valid_volume_level_count=0x0
            memset(nvline,0,1024);
            memset(valuebuf,0,128);
            ivalue = audio_total[i].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[j2].valid_volume_level_count;
            sprintf(valuebuf,"%0x",ivalue);
            strcat(nvline,"audio_arm\\audio_arm\\");
            strcat(nvline,&arm_name[i*NAME_LEN_MAX]);
            strcat(nvline,"\\AudioStructure\\app_config_info_set\\app_config_info\\app_config_info[");
            strcat(nvline,numbuf);
            strcat(nvline,"]\\valid_volume_level_count=0x");
            strcat(nvline,valuebuf);
            fputs(nvline,file);
            fputc('\n',file);

            //audio_arm\audio_arm\Headset\AudioStructure\app_config_info_set\app_config_info\app_config_info[1]\arm_volume\arm_volume[0]=0x0
            for( k = 0;k<AUDIO_ARM_VOLUME_LEVEL;k++)
            {
                memset(numbuf1,0,10);
                sprintf(numbuf1,"%d",k);
                memset(nvline,0,1024);
                memset(valuebuf,0,128);
                ivalue = audio_total[i].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[j2].arm_volume[k];
                sprintf(valuebuf,"%0x",ivalue);//////
                strcat(nvline,"audio_arm\\audio_arm\\");
                strcat(nvline,&arm_name[i*NAME_LEN_MAX]);
                strcat(nvline,"\\AudioStructure\\app_config_info_set\\app_config_info\\app_config_info[");
                strcat(nvline,numbuf);
                strcat(nvline,"]\\arm_volume\\arm_volume[");
                strcat(nvline,numbuf1);
                strcat(nvline,"]=0x");
                strcat(nvline,valuebuf);
                fputs(nvline,file);
                fputc('\n',file);
            }
            //audio_arm\audio_arm\Headset\AudioStructure\app_config_info_set\app_config_info\app_config_info[1]\reserve\reserve[0]=0x0
            for( k = 0;k<AUDIO_NV_ARM_APP_PARA_RESERVE;k++)
            {
                memset(numbuf1,0,10);
                sprintf(numbuf1,"%d",k);
                memset(nvline,0,1024);
                memset(valuebuf,0,128);
                ivalue = audio_total[i].audio_nv_arm_mode_info.tAudioNvArmModeStruct.app_config_info_set.app_config_info[j2].reserve[k];
                sprintf(valuebuf,"%0x",ivalue);
                strcat(nvline,"audio_arm\\audio_arm\\");
                strcat(nvline,&arm_name[i*NAME_LEN_MAX]);
                strcat(nvline,"\\AudioStructure\\app_config_info_set\\app_config_info\\app_config_info[");
                strcat(nvline,numbuf);
                strcat(nvline,"]\\reserve\\reserve[");
                strcat(nvline,numbuf1);
                strcat(nvline,"]=0x");
                strcat(nvline,valuebuf);
                fputs(nvline,file);
                fputc('\n',file);
            }
        }
        //audio_arm\audio_arm\Headfree\AudioStructure\midi_opt=0x618
        memset(nvline,0,1024);
        memset(valuebuf,0,128);
        ivalue = audio_total[i].audio_nv_arm_mode_info.tAudioNvArmModeStruct.midi_opt;
        sprintf(valuebuf,"%0x",ivalue);
        strcat(nvline,"audio_arm\\audio_arm\\");
        strcat(nvline,&arm_name[i*NAME_LEN_MAX]);
        strcat(nvline,"\\AudioStructure\\midi_opt=0x");
        strcat(nvline,valuebuf);
        fputs(nvline,file);
        fputc('\n',file);

        //audio_arm\audio_arm\Headfree\AudioStructure\aud_dev=0x22
        memset(nvline,0,1024);
        memset(valuebuf,0,128);
        ivalue = audio_total[i].audio_nv_arm_mode_info.tAudioNvArmModeStruct.aud_dev;
        sprintf(valuebuf,"%0x",ivalue);
        strcat(nvline,"audio_arm\\audio_arm\\");
        strcat(nvline,&arm_name[i*NAME_LEN_MAX]);
        strcat(nvline,"\\AudioStructure\\aud_dev=0x");
        strcat(nvline,valuebuf);
        fputs(nvline,file);
        fputc('\n',file);

        //audio_arm\audio_arm\Headfree\AudioStructure\reserve\reserve[0]=0x50
        for( j3 = 0;j3<AUDIO_NV_ARM_PARA_RESERVE;j3++)
        {
            memset(numbuf,0,10);
            sprintf(numbuf,"%d",j3);
            memset(nvline,0,1024);
            memset(valuebuf,0,128);
            ivalue = audio_total[i].audio_nv_arm_mode_info.tAudioNvArmModeStruct.reserve[j3];
            sprintf(valuebuf,"%0x",ivalue);
            strcat(nvline,"audio_arm\\audio_arm\\");
            strcat(nvline,&arm_name[i*NAME_LEN_MAX]);
            strcat(nvline,"\\AudioStructure\\reserve\\reserve[");
            strcat(nvline,numbuf);
            strcat(nvline,"]=0x");
            strcat(nvline,valuebuf);
            fputs(nvline,file);
            fputc('\n',file);
        }

        /********************do things about eq**********************/
        //%audio\EQ_Headset\eq_control=0x0za
        memset(nvline,0,1024);
        memset(valuebuf,0,128);
        ivalue = audio_total[i].audio_enha_eq.eq_control;
        sprintf(valuebuf,"%0x",ivalue&0xffff);
        strcat(nvline,"%audio\\");
        strcat(nvline,&eq_name[i*NAME_LEN_MAX]);
        strcat(nvline,"\\eq_control=0x");
        strcat(nvline,valuebuf);
        fputs(nvline,file);
        fputc('\n',file);

        for( j4 = 1;j4<=EQ_MODE_MAX;j4++)
        {
            //%audio\EQ_Headset\eq_mode_1\agc_in_gain=0x1000
            memset(numbuf,0,10);
            sprintf(numbuf,"%d",j4);
            memset(nvline,0,1024);
            memset(valuebuf,0,128);
            ivalue = audio_total[i].audio_enha_eq.eq_modes[j4-1].agc_in_gain;
            sprintf(valuebuf,"%0x",ivalue&0xffff);
            strcat(nvline,"%audio\\");
            strcat(nvline,&eq_name[i*NAME_LEN_MAX]);
            strcat(nvline,"\\eq_mode_");
            strcat(nvline,numbuf);
            strcat(nvline,"\\agc_in_gain=0x");
            strcat(nvline,valuebuf);
            fputs(nvline,file);
            fputc('\n',file);
            //%audio\EQ_Headset\eq_mode_1\band_control=0xe000
            memset(nvline,0,1024);
            memset(valuebuf,0,128);
            ivalue = audio_total[i].audio_enha_eq.eq_modes[j4-1].band_control;
            sprintf(valuebuf,"%0x",ivalue&0xffff);
            strcat(nvline,"%audio\\");
            strcat(nvline,&eq_name[i*NAME_LEN_MAX]);
            strcat(nvline,"\\eq_mode_");
            strcat(nvline,numbuf);
            strcat(nvline,"\\band_control=0x");
            strcat(nvline,valuebuf);
            fputs(nvline,file);
            fputc('\n',file);
            //%audio\EQ_Headset\eq_mode_1\eq_band_1\fo=0x2bc
            for( k = 1;k<=EQ_BAND_MAX;k++)
            {
                memset(numbuf1,0,10);
                sprintf(numbuf1,"%d",k);

                memset(nvline,0,1024);
                memset(valuebuf,0,128);
                ivalue = audio_total[i].audio_enha_eq.eq_modes[j4-1].eq_band[k-1].fo;
                sprintf(valuebuf,"%0x",ivalue&0xffff);
                strcat(nvline,"%audio\\");
                strcat(nvline,&eq_name[i*NAME_LEN_MAX]);
                strcat(nvline,"\\eq_mode_");
                strcat(nvline,numbuf);
                strcat(nvline,"\\eq_band_");
                strcat(nvline,numbuf1);
                strcat(nvline,"\\fo=0x");
                strcat(nvline,valuebuf);
                fputs(nvline,file);
                fputc('\n',file);
                //%audio\EQ_Headset\eq_mode_1\eq_band_1\q=0x200
                memset(nvline,0,1024);
                memset(valuebuf,0,128);
                ivalue = audio_total[i].audio_enha_eq.eq_modes[j4-1].eq_band[k-1].q;
                sprintf(valuebuf,"%0x",ivalue);
                strcat(nvline,"%audio\\");
                strcat(nvline,&eq_name[i*NAME_LEN_MAX]);
                strcat(nvline,"\\eq_mode_");
                strcat(nvline,numbuf);
                strcat(nvline,"\\eq_band_");
                strcat(nvline,numbuf1);
                strcat(nvline,"\\q=0x");
                strcat(nvline,valuebuf);
                fputs(nvline,file);
                fputc('\n',file);
                //%audio\EQ_Headset\eq_mode_1\eq_band_1\boostdB=0x5a
                memset(nvline,0,1024);
                memset(valuebuf,0,128);
                ivalue = audio_total[i].audio_enha_eq.eq_modes[j4-1].eq_band[k-1].boostdB;
                sprintf(valuebuf,"%0x",ivalue&0xffff);
                strcat(nvline,"%audio\\");
                strcat(nvline,&eq_name[i*NAME_LEN_MAX]);
                strcat(nvline,"\\eq_mode_");
                strcat(nvline,numbuf);
                strcat(nvline,"\\eq_band_");
                strcat(nvline,numbuf1);
                strcat(nvline,"\\boostdB=0x");
                strcat(nvline,valuebuf);
                fputs(nvline,file);
                fputc('\n',file);
                //%audio\EQ_Headset\eq_mode_1\eq_band_1\gaindB=0xffa6
                memset(nvline,0,1024);
                memset(valuebuf,0,128);
                ivalue = audio_total[i].audio_enha_eq.eq_modes[j4-1].eq_band[k-1].gaindB;
                sprintf(valuebuf,"%0x",ivalue&0xffff);
                strcat(nvline,"%audio\\");
                strcat(nvline,&eq_name[i*NAME_LEN_MAX]);
                strcat(nvline,"\\eq_mode_");
                strcat(nvline,numbuf);
                strcat(nvline,"\\eq_band_");
                strcat(nvline,numbuf1);
                strcat(nvline,"\\gaindB=0x");
                strcat(nvline,valuebuf);
                fputs(nvline,file);
                fputc('\n',file);
            }
        }
        //%audio\EQ_Headset\extendArray\extendArray[0]=0x7
        for( j5 = 0;j5<59;j5++)
        {
            memset(numbuf,0,10);
            sprintf(numbuf,"%d",j5);
            memset(nvline,0,1024);
            memset(valuebuf,0,128);
            ivalue = audio_total[i].audio_enha_eq.externdArray[j5];
            sprintf(valuebuf,"%0x",ivalue&0xffff);
            strcat(nvline,"%audio\\");
            strcat(nvline,&eq_name[i*NAME_LEN_MAX]);
            strcat(nvline,"\\extendArray\\extendArray[");
            strcat(nvline,numbuf);
            strcat(nvline,"]=0x");
            strcat(nvline,valuebuf);
            fputs(nvline,file);
            fputc('\n',file);
        }
#ifdef NRARRAY_ANALYSIS
        for( j6 = 0;j6<8;j6++)
        {
            memset(numbuf,0,10);
            sprintf(numbuf,"%d",j6); /*NR */
            memset(nvline,0,1024);
            memset(valuebuf,0,128);
            ivalue = audio_total[i].audio_enha_eq.nrArray[j6];
            sprintf(valuebuf,"%0x",ivalue&0xffff);
            strcat(nvline,"%audio\\");
            strcat(nvline,&eq_name[i*NAME_LEN_MAX]);
            strcat(nvline,"\\nrArray\\nrArray[");
            strcat(nvline,numbuf);
            strcat(nvline,"]=0x");
            strcat(nvline,valuebuf);
            fputs(nvline,file);
            fputc('\n',file);
        }
#endif
    }
    fclose(file);
    free(arm_name);
    free(eq_name);
}





