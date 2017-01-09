#include <stdio.h>
#include <stdlib.h>
#include "eng_busmonitor.h"
#include "eng_diag.h"
#include "engopt.h"
#include "calibration.h"
#include "sprd_bm.h"

int eng_diag_busmonitor_enable(char *buf, int len, char *rsp, int rsplen)
{
    char *rsp_ptr;
    unsigned short ret = 0;
    MSG_HEAD_T* msg_head_ptr = (MSG_HEAD_T*)(buf + 1);

    if(NULL == buf){
        ENG_LOG("%s,null pointer",__FUNCTION__);
        return 0;
    }

    rsplen = sizeof(char)  + sizeof(MSG_HEAD_T);
    rsp_ptr = (char*)malloc(rsplen);
    if(NULL == rsp_ptr){
        ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
        return 0;
    }

    memset(rsp_ptr, 0, rsplen);
    memcpy(rsp_ptr,msg_head_ptr,sizeof(MSG_HEAD_T));
    ((MSG_HEAD_T*)rsp_ptr)->len = rsplen;

    ret = sprd_bm_prof_enable();

    if(ret)
        *((unsigned short*)(rsp_ptr + sizeof(MSG_HEAD_T))) = 0x00;
    else
        *((unsigned short*)(rsp_ptr + sizeof(MSG_HEAD_T))) = 0x01;

    rsplen = translate_packet(rsp,(unsigned char*)rsp_ptr,((MSG_HEAD_T*)rsp_ptr)->len);
    free(rsp_ptr);
    return rsplen;
}

int eng_diag_busmonitor_disable(char *buf, int len, char *rsp, int rsplen)
{
    char *rsp_ptr;
    unsigned short ret = 0;
    MSG_HEAD_T* msg_head_ptr = (MSG_HEAD_T*)(buf + 1);

    if(NULL == buf){
        ENG_LOG("%s,null pointer",__FUNCTION__);
        return 0;
    }

    rsplen = sizeof(char)  + sizeof(MSG_HEAD_T);
    rsp_ptr = (char*)malloc(rsplen);
    if(NULL == rsp_ptr){
        ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
        return 0;
    }

    memset(rsp_ptr, 0, rsplen);
    memcpy(rsp_ptr,msg_head_ptr,sizeof(MSG_HEAD_T));
    ((MSG_HEAD_T*)rsp_ptr)->len = rsplen;

    ret = sprd_bm_prof_disable();

    if(ret)
        *((unsigned short*)(rsp_ptr + sizeof(MSG_HEAD_T))) = 0x00;
    else
        *((unsigned short*)(rsp_ptr + sizeof(MSG_HEAD_T))) = 0x01;

    rsplen = translate_packet(rsp,(unsigned char*)rsp_ptr,((MSG_HEAD_T*)rsp_ptr)->len);
    free(rsp_ptr);
    return rsplen;
}

int eng_diag_busmonitor_get_chaninfo(char *buf, int len, char *rsp, int rsplen)
{
    char *rsp_ptr;
    int i;
    unsigned short cnt,ret = 0;
    MSG_HEAD_T* msg_head_ptr = (MSG_HEAD_T*)(buf + 1);
    char name[16] = {0};

    if(NULL == buf){
        ENG_LOG("%s,null pointer",__FUNCTION__);
       return 0;
    }

    cnt = sprd_bm_get_chn_cnt();

    rsplen = 2*sizeof(unsigned short)  + cnt*sizeof(BUSMONITOR_CHAN_NAME) + sizeof(MSG_HEAD_T);
    rsp_ptr = (char*)malloc(rsplen);
    if(NULL == rsp_ptr){
        ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
        return 0;
    }

    memset(rsp_ptr, 0, rsplen);
    memcpy(rsp_ptr,msg_head_ptr,sizeof(MSG_HEAD_T));
    ((MSG_HEAD_T*)rsp_ptr)->len = rsplen;
    *((unsigned short*)(rsp_ptr + sizeof(MSG_HEAD_T))) = cnt;

    for(i = 0; i < cnt; i++){
        memcpy((rsp_ptr + sizeof(MSG_HEAD_T)) + 2*sizeof(unsigned short) + i*sizeof(BUSMONITOR_CHAN_NAME),(char*)sprd_bm_get_chn_name(i),sizeof(BUSMONITOR_CHAN_NAME));
    }

    rsplen = translate_packet(rsp,(unsigned char*)rsp_ptr,((MSG_HEAD_T*)rsp_ptr)->len);
    free(rsp_ptr);
    return rsplen;
}

int eng_diag_busmonitor_get_rtctime(char *buf, int len, char *rsp, int rsplen)
{
    char *rsp_ptr;
    unsigned long rtc_time ;
    MSG_HEAD_T* msg_head_ptr = (MSG_HEAD_T*)(buf + 1);

    if(NULL == buf){
        ENG_LOG("%s,null pointer",__FUNCTION__);
       return 0;
    }

    rsplen = sizeof(unsigned long)  + sizeof(MSG_HEAD_T);
    rsp_ptr = (char*)malloc(rsplen);
    if(NULL == rsp_ptr){
        ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
        return 0;
    }

    rtc_time = sprd_bm_get_time();

    memset(rsp_ptr, 0, rsplen);
    memcpy(rsp_ptr,msg_head_ptr,sizeof(MSG_HEAD_T));
    ((MSG_HEAD_T*)rsp_ptr)->len = rsplen;
    *((unsigned long*)(rsp_ptr + sizeof(MSG_HEAD_T))) = rtc_time;

    rsplen = translate_packet(rsp,(unsigned char*)rsp_ptr,((MSG_HEAD_T*)rsp_ptr)->len);
    free(rsp_ptr);
    return rsplen;
}

int eng_diag_busmonitor_get_monitordata(char *buf, int len, char *rsp, int rsplen)
{
    char *rsp_ptr;
    int i;
    unsigned short cnt,ret = 0;
    BUSMONITOR_CHANNEL_DATA*data = NULL;
    MSG_HEAD_T* msg_head_ptr = (MSG_HEAD_T*)(buf + 1);

    if(NULL == buf){
        ENG_LOG("%s,null pointer",__FUNCTION__);
       return 0;
    }

    cnt = sprd_bm_get_chn_cnt();

    rsplen = 2*sizeof(unsigned short)  + cnt*sizeof(BUSMONITOR_CHANNEL_DATA) + sizeof(MSG_HEAD_T);
    rsp_ptr = (char*)malloc(rsplen);
    if(NULL == rsp_ptr){
        ENG_LOG("%s: Buffer malloc failed\n", __FUNCTION__);
        return 0;
    }

    memset(rsp_ptr, 0, rsplen);
    memcpy(rsp_ptr,msg_head_ptr,sizeof(MSG_HEAD_T));
    ((MSG_HEAD_T*)rsp_ptr)->len = rsplen;
    *((unsigned short*)(rsp_ptr + sizeof(MSG_HEAD_T))) = cnt;

    for(i = 0; i < cnt; i++){
        data = (BUSMONITOR_CHANNEL_DATA*)(rsp_ptr + 2*sizeof(unsigned short) + sizeof(MSG_HEAD_T) +  i*sizeof(BUSMONITOR_CHANNEL_DATA));
        data->ReadCount = sprd_bm_get_rdcnt(i);
        data->WriteCount = sprd_bm_get_wrcnt(i);
        data->ReadBandWidth = sprd_bm_get_rdbw(i);
        data->WriteBandWidth = sprd_bm_get_wrbw(i);
        data->ReadLatency = sprd_bm_get_rdlatency(i);
        data->WriteLatency = sprd_bm_get_wrlatency(i);
    }

    rsplen = translate_packet(rsp,(unsigned char*)rsp_ptr,((MSG_HEAD_T*)rsp_ptr)->len);
    free(rsp_ptr);
    return rsplen;
}

