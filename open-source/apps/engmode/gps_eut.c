#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "eut_opt.h"
#include "engopt.h"

static int eut_gps_state;
static int gps_search_state;
static float snr;

static void *gpssearch_thread();
static void *prnstate_thread();
static void *gpsprn_thread();

struct eng_gps_eutops gps_eutops ={
    gps_eut,
    gpseut_req,
    gps_search,
    gps_search_req,
    set_gps_prn,
    gps_prnstate_req,
    gps_snr_req

};

static void parseNumbers(char* line, const char* fmt, void* data, int size, int n)
{
    int i;
    for ( i =0 ; i < n; ++i)
    {
        line = strchr(line, ',');
        if (NULL == line)
        {
            break;
        }
        line++;
        sscanf(line, fmt, (char*)data + i * size);
    }
}

static int parseFts(char* nmea)
{
    enum {
        SENTENCE_VERSION        = 0,
        MESSAGE_NAME            = 1, // Text
        PRN                     = 2,
        LAST_SIGNAL_STRENGTH    = 3,
        AVERAGED_SIGNAL_STRENGTH = 4,
        UNITS_DBM               = 5,
        LAST_CN_ANTENNA         = 6,
        AVERAGED_CN_ANTENNA     = 7,
        LAST_CN_BASEBAND        = 8,
        AVERAGED_CN_BASEBAND    = 9,
        UNITS_DBHZ              = 10,
        REF_CLOCK_OFFSET_PPB    = 11,
        UNCERTAINTY_PPB         = 12,
        RTC_OFFSET_PPM          = 13,
        CNTIN                   = 14, // Text
        TCXO_PPB                = 15,
        DECODED_WORD_COUNT      = 16,
        DECODING_ATTEMPT_COUNT  = 17,
        WORD_ERROR_RATE         = 18,
        RF_AGC_VALUE            = 19,
        ENERGY_DROP_OUT_COUNTER = 20,

        // Not a field name, for accounting only
        CHECKSUM		= 21,

        FIELD_COUNT
    };

    float value[FIELD_COUNT] = {0};

    parseNumbers(nmea, "%f", value, sizeof(value[0]), FIELD_COUNT);

    int svid	= value[PRN];
    snr 	= value[LAST_SIGNAL_STRENGTH];
    float cn	= value[LAST_CN_BASEBAND];

    ALOGI("SVID = %d , SignalStrengh = %f , C/No = %f\n", svid , snr, cn);
    return 0;
}



int gps_eut(int command_code,char *rsp)
{
    if(command_code == 1)
        start_gpseut(rsp);
    else if(command_code == 0)
        end_gpseut(rsp);
    return 0;

}

int gpseut_req(char *result)
{
    sprintf(result,"%s%d",EUT_GPS_REQ,eut_gps_state);
    return 0;
}
int gps_search(int command_code,char *result)
{
    if(command_code == 1)
        start_gpssearch(result);
    else if(command_code == 0)
        end_gpssearch(result);
    return 0;
}
int start_gpseut(char *result)
{
    ALOGI("start_gpseut");
    eng_thread_t t1;
    if (0 != eng_thread_create(&t1,gpsprn_thread,NULL)){
        ALOGI("gpsprn_thread start error");
    }
    strcpy(result,EUT_GPS_OK);
    return 0;
}

int end_gpseut(char *result)
{
    strcpy(result,EUT_GPS_OK);
    eut_gps_state=0;
    return 0;
}

int start_gpssearch(char * result)
{
    int error;
    int pid = 0;
    eng_thread_t t1;
    ALOGI("start_gpssearch");
    if (0 != eng_thread_create(&t1,gpssearch_thread,NULL)){
        ALOGI("gpssearch_thread start error");
    }
    gps_search_state=1;
    strcpy(result,EUT_GPS_OK);
    return 0;
}

static void *gpssearch_thread(){
    ALOGI("gpssearch_thread");
    system("glgps -c /system/etc/gpsconfig.xml Periodic");
    return 0;
}
int end_gpssearch(char * result)
{
    system("killall glgps");
    strcpy(result,EUT_GPS_OK);
    gps_search_state=0;
    return 0;
}

int gps_search_req(char *result)
{
    sprintf(result,"%s%d",EUT_GPS_SEARCH_REQ,gps_search_state);
    return 0;
}

int gpssearch_result(char *result)
{
    FILE* file;
    int i,j,k;
    char *line;
    char *key;
    char *cur;
    char *prv;
    char res[50];
    char prn[20][5];
    char buf[100];
    int pid =0;
    eng_thread_t t1;

    if (0 != eng_thread_create(&t1,prnstate_thread,NULL)){
        ALOGI("prnstate_thread start error");
    }

    sleep(1);
    file = fopen("/data/gpsresult.txt","r");
    if(file == NULL){
        sprintf(result,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSTATE);
        ALOGI("does not find /data/gpsresult.txt");
        return 0;
    }
    i=0;
    while((line=fgets(buf,100,file)) != NULL){
        //char* buf_s = buf;
        j=0;
        ALOGI("read buf is %s",line);
        key=strstr(buf,"GPGSV");
        if(key == NULL){
            continue;
        }
        cur = strchr(buf,',');
        j = 0;
        while(cur != NULL){
            int len =0;
            cur++;
            prv = cur;

            cur=strchr(cur,',');
            if(cur == NULL){
                len = 0;
            }else{
                len = cur-prv;
            }
            if(j==3||j==7||j==11||j==15){
                if((len >= 0) &&(i<=20)){
                    memcpy(prn[i],prv,len);
                }
                i++;
            }
            j++;
        }
    }
    if(i>0){
        for(k=0;k<i;k++){
            if(k == 0){
                strcat(res,prn[k]);
            }
            if(strstr(res,prn[k+1]))
                break;
            strcat(res,",");
            strcat(res,prn[k+1]);
        }
        sprintf(result,"%s%s",EUT_GPS_PRN_REQ,res);
    }
    system("rm /data/gpsresult.txt");
    fclose(file);
    return i;
}

int gps_prnstate_req(char * result)
{
    if(!gps_search_state){
        ALOGI("gps has not search");
        sprintf(result,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSTATE);
        return 0;
    }
    int i,retry;
    i=gpssearch_result(result);
    if(i==0){
        //sleep(15);
        retry=0;
        for(retry;retry<4;retry++){
            i=gpssearch_result(result);
            if(i>0)
                break;
        }
        if(i==0){
            ALOGI("gps_prnstate_req  i==0");
            sprintf(result,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSTATE);
        }
    }
    return 0;

}

static void *prnstate_thread(){
    ALOGI("cat /data/gpspipe > /data/gpsresult.txt");
    system("cat /data/gpspipe > /data/gpsresult.txt");
    return 0;
}
int gps_snr_req(char * result)
{
    ALOGI("gps_snr_req");
    int pipefd = open("/data/gpspipe",O_RDONLY);
    if(pipefd < 0)
    {
        ALOGI("open pipe error");
        sprintf(result,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSEARCH);
        return 0;
    }
    int retry = 5;
    do{
        ALOGI("get the snr");
        char nmea[1024]={0};
        int byteCount =0;
        if((byteCount = read(pipefd, (unsigned char*)nmea, sizeof(nmea)) ) < 0)
        {
            perror("read pipe error\n");
            ALOGI("cmd_set_ratio");
        }
        if (byteCount > 0)
        {
            ALOGI("byteCount > 0 %s",nmea);

            // process only factory NMEA sentence,
            // here may need to note the version, to use "$PGLOR,0,FTS" or "$PGLOR,1,FTS"
            if (NULL != strstr(nmea, "FTS"))
            {
                //if each nmea need to be showed please open here
                parseFts(nmea) ;
                //break;
            }
        }
        else if (retry == 0)
        {
            break;
        }
        else
        {
            //printf("No data...\n");
            continue;
        }
        retry--;
    }while (retry);
    close(pipefd);
    sprintf(result,"%s%f",EUT_GPS_SNR_REQ,snr);
    return 0;
}


int set_gps_prn(int prn, char * result)
{

    int pid = 0;
    eng_thread_t t1;
    if (0 != eng_thread_create(&t1,gpsprn_thread,NULL)){
        ALOGI("gpsprn_thread start error");
    }
    sprintf(result,"%s%d",EUT_GPS_PRN_REQ,prn);
    return 0;
}

static void *gpsprn_thread(){
    ALOGI("glgps -c /system/etc/gpsconfig.xml  Factory_High_SNR");
    eut_gps_state=1;
    system("glgps -c /system/etc/gpsconfig.xml  Factory_High_SNR");
    return 0;
}
