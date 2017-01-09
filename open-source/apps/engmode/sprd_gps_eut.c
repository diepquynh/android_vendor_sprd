#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cutils/properties.h>

#include "eut_opt.h"
#include "engopt.h"

static int eut_gps_state;
static int gps_search_state;
static void *start_gps_thread();

static int gps_eut(int command_code,char *rsp)
{
	if(command_code == 1)
		start_gpseut(rsp);
	else if(command_code == 0)
		end_gpseut(rsp);
	return 0;

}

static int start_gpseut(char *result)
{
	eng_thread_t t1;
	
	ALOGI("start_gpseut");
	if (0 != eng_thread_create(&t1,start_gps_thread,NULL)){
         ALOGI("start_gps_thread start error");
       }
	eut_gps_state=1;
	strcpy(result,EUT_GPS_OK);
	return 0;
}

static int end_gpseut(char *result)
{
	ALOGI("end_gpseut");
	strcpy(result,EUT_GPS_OK);
	eut_gps_state=0;
	return 0;
}

static int gpseut_req(char *result)
{
	ALOGI("gpseut_req");
	sprintf(result,"%s%d",EUT_GPS_REQ,eut_gps_state);
	return 0;
}
static int gps_search(int command_code,char *result)
{
	if(command_code == 1)
		start_gpssearch(result);
	else if(command_code == 0)
		end_gpssearch(result);
	return 0;
}


static int start_gpssearch(char * result)
{
	int error;
	int pid = 0;
	eng_thread_t t1;
	ALOGI("start_gpssearch");
	if (0 != eng_thread_create(&t1,start_gps_thread,NULL)){
         ALOGI("start_gps_thread start error");
       }
	gps_search_state=1;
	strcpy(result,EUT_GPS_OK);
	return 0;
}

static int end_gpssearch(char * result)
{
	ALOGI("end_gpssearch");
	if(property_set("ctl.stop", "GPSenseEngine")<0) 
		ALOGI("Failed to stop GPSenseEngine"); 
	strcpy(result,EUT_GPS_OK);
	gps_search_state=0;
	return 0;
}

static int gps_search_req(char *result)
{
	ALOGI("gps_search_req");
	sprintf(result,"%s%d",EUT_GPS_SEARCH_REQ,gps_search_state);
	return 0;
}

static int gpssearch_result(char *result)
{
	FILE* file;
	int i,j,k;
	char *line;
	char *key;
	char *cur;
	char *prv;
	char res[50]={0};
	char prn[20][5];
	char buf[100]={0};
	int sv_num;
	
	ALOGI("gpssearch_result");
	memset(prn,0,sizeof(prn));
	file = fopen("/dev/ttyV1","r");
	if(file == NULL){
		sprintf(result,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSTATE);
		ALOGI("does not find /dev/ttyV1");
		return 0;
	}
	sleep(3);
	i=0;
	sv_num=0;
	while((line=fgets(buf,100,file)) != NULL){
		j=0;
		ALOGI("read buf is %s",line);
		key=strstr(buf,"GPGSV");
		if(key == NULL){
			continue;
		}

		cur = strchr(buf,',');
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

			if((i==0)&&(j==2)){
				sv_num = atoi(prv);
				if(sv_num == 0)
					break;
			}

			if(j==3||j==7||j==11||j==15){
				if((len >= 0) &&(i<=20)){
					memcpy(prn[i],prv,len);
				}
				i++;
			}
			j++;	
		}

		if(i == sv_num)
			break;
		if(i>20)
			break;	
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
	fclose(file);
	return i;
}

static int gps_prnstate_req(char * result)
{
	ALOGI("gps_prnstate_req");
	if((gps_search_state == 0) && (eut_gps_state == 0)){
		ALOGI("gps has not search");
		sprintf(result,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSEARCH);
		return 0;
	}

	if(gpssearch_result(result)==0){
		ALOGI("gps_prnstate_req==0");
		sprintf(result,"%s%s",EUT_GPS_PRN_REQ,EUT_GPS_NO_FOUND_STAELITE);
	}
	return 0;

}

static int gpssnr_result(int prn_num,char *result,int count)
{
	FILE* file;
	int i,j,k;
	char *line;
	char *key;
	char *cur;
	char *prv;
	char snr[20][5];
	char max_snr[5]={0};
	char max_snr_id[5]={0};
	char prn[20][5];
	char buf[100]={0};
	int flag_snr = 0;
	int sv_num;
	
	memset(snr,0,sizeof(snr));
	memset(prn,0,sizeof(prn));
	
	file = fopen("/dev/ttyV1","r");
	if(file == NULL){
		sprintf(result,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSTATE);
		ALOGI("can't open /dev/ttyV1");
		return 0;
	}
	sleep(3);
	i=0;
	sv_num=0;
	while((line=fgets(buf,100,file)) != NULL){
		j=0;
		ALOGI("read buf is %s",line);
		key=strstr(buf,"GPGSV");
		if(key == NULL){
		continue;
		}

		cur = strchr(buf,',');
		while(cur != NULL){
			int len =0;
			
			cur++;
			prv = cur;
			cur=strchr(cur,',');
			if(cur == NULL){
				cur=strchr(prv,'*');
				if(cur!=NULL){
					len = cur-prv;
					cur = NULL;
				}
				else
					len = 0;
			}else{
				len = cur-prv;
			}

			if((i==0)&&(j==2))
			{
				sv_num = atoi(prv);
				if(sv_num == 0)
					break;
			}

			if(j==3||j==7||j==11||j==15||j==6||j==10||j==14||j==18){			
				if((len >= 0) &&(i<=20)){
					if(j%2){
						memcpy(prn[i],prv,len);
					}

					if(j%2 == 0){
						memcpy(snr[i-1],prv,len);
					}
				}
				if(j==3||j==7||j==11||j==15)
					i++;
			}
			j++;
		}
		
		if(i == sv_num)
			break;

		if(i>20)
			break;
	}

	if(i>0){
		if(count == 0){
			
			flag_snr = 1;
			memcpy(max_snr,snr[0],strlen(snr[0]));
			memcpy(max_snr_id,prn[0],strlen(prn[0]));
			for(k=0;k<i;k++){
				if(atoi(max_snr) < atoi(snr[k]))
				{
					memcpy(max_snr,snr[k],strlen(snr[k]));
					memcpy(max_snr_id,prn[k],strlen(prn[k]));
				}
			}
			sprintf(result,"%s%s %s%s %s%d",EUT_GPS_SNR_REQ,max_snr,\
				EUT_GPS_SV_ID,max_snr_id,EUT_GPS_SV_NUMS,sv_num);
		}
		else{
			for(k=0;k<i;k++){
				if(prn_num == atoi(prn[k])){
					sprintf(result,"%s%s,%s",EUT_GPS_SNR_REQ,prn[k],snr[k]);
					flag_snr = 1;
					break;
				}
			}
		}
		
		if(flag_snr == 0)
			sprintf(result,"%s%s",EUT_GPS_SNR_REQ,EUT_GPS_SNR_NO_EXIST);
	}

	fclose(file);
	return i;
}


static int gps_snr_req(char * result)
{
	int i,retry;

	ALOGI("gps_snr_req");
	if((gps_search_state == 0) && (eut_gps_state == 0)){
		ALOGI("gps has not search");
		sprintf(result,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSEARCH);
		return 0;
	}
	
	if(gpssnr_result(0,result,0)==0){
		ALOGI("gps_snr_req ==0");
		sprintf(result,"%s%s",EUT_GPS_SNR_REQ,EUT_GPS_NO_FOUND_STAELITE);
	};
	
	return 0;
}

static int set_gps_prn(int prn, char * result)
{
	ALOGI("set_gps_prn");
	if((gps_search_state == 0) && (eut_gps_state == 0)){
		ALOGI("gps has not search");
		sprintf(result,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSEARCH);
		return 0;
	}

	if(gpssnr_result(prn,result,1)==0){
		ALOGI("set_gps_prn == 0");
		sprintf(result,"%s%s",EUT_GPS_SNR_REQ,EUT_GPS_NO_FOUND_STAELITE);
	};
	return 0;
}

static void *start_gps_thread(){
	ALOGI("start_gps_thread");
	if (property_set("ctl.start", "GPSenseEngine") < 0) 
		ALOGI("Failed to start GPSenseEngine"); 
	return 0;
}

struct eng_gps_eutops gps_eutops ={
	gps_eut,
	gpseut_req,
	gps_search,
	gps_search_req,
	set_gps_prn,
	gps_prnstate_req,
	gps_snr_req
};


