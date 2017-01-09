///============================================================================
/// Copyright 2012-2014  spreadtrum  --
/// This program be used, duplicated, modified or distributed
/// pursuant to the terms and conditions of the Apache 2 License.
/// ---------------------------------------------------------------------------
/// file gps_lib.c
/// for converting NMEA to Android like APIs
/// ---------------------------------------------------------------------------
/// zhouxw mod 20130920,version 1.00,include test,need add supl so on...
///============================================================================
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>
#include <gps.h>
#include <pthread.h>
#include <stdio.h>
#include <cutils/log.h>
#include <stdlib.h>
#include <unistd.h>    
#include <sys/stat.h>   
#include <string.h>  
#include <semaphore.h>
#include <dlfcn.h>

#include "gps_pc_mode.h"

// GPS PC MODE
#define INIT_MODE  0x07
#define STOP_MODE  0x03

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define  LOG_TAG  "LIBGPS_ENGPC"

#define  GPS_DEBUG
#ifdef GPS_DEBUG
#define  E(...)   ALOGE(__VA_ARGS__)
#define  D(...)   ALOGD(__VA_ARGS__)
#else
#define  E(...)   ((void)0)
#define  D(...)   ((void)0)
#endif

#define ENG_GPS_NUM     13

#define  GNSS_WR_REG
#ifdef GNSS_WR_REG
int (*hal_write_register)(unsigned int addr,unsigned int value);
unsigned int (*hal_read_register)(unsigned int addr);
#endif
double (*hal_read_tsxtemp)();

static int eutmode = 0;
static int first_open = 0;
static int eut_gps_state;
static int gps_search_state;
static char pc_mode = 0;
static int set_mode = 0;
static char buf[512];
static sem_t sem_a;
static pthread_mutex_t mutex_a = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_b = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_tsx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_cwcn = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_tcxo = PTHREAD_MUTEX_INITIALIZER;
static GpsSvStatus SvState;
static GpsLocation GPSloc;
static int fix_status;
unsigned int gps_mode = 0;   // cold start : 125     cw cn0: 50002
#define  GREENEYE_I    1
#define  GREENEYE_II   2
static int chip_id = 0;
static char *version = "VERSION: 2016-04-05\r\n";

#define TOKEN_LEN(tok)  (tok.end>tok.p?tok.end-tok.p:0)
#define  MAX_NMEA_TOKENS  32
int cwcn_value = 0;
double tsx_value = 0;
double tcxo_value = 0;
extern int CN0; // CW_CN0

typedef struct {
    const char*  p;
    const char*  end;
} Token;

typedef struct {
    int     count;
    Token   tokens[ MAX_NMEA_TOKENS ];
} NmeaTokenizer;

static int str2int( const char*  p, const char*  end )
{
    int   result = 0;
    int   len    = end - p;

    for ( ; len > 0; len--, p++ )
    {
        int  c;

        if (p >= end)
            goto Fail;

        c = *p - '0';
        if ((unsigned)c >= 10)
            goto Fail;

        result = result*10 + c;
    }
    return  result;

Fail:
    return -1;
}

static double str2float( const char*  p, const char*  end )
{
    int   result = 0;
    int   len    = end - p;
    char  temp[16];

    if (len >= (int)sizeof(temp))
        return 0.;
    if(len == 0)
        return 0.;
    memcpy( temp, p, len );
    temp[len] = 0;
    return strtod( temp, NULL );
}

static int strhex2int( const char*  p, int len )
{
	int   result = 0;
	D("strhex2int: (%s) len=%d \n", p, len);	
	//len = len -1;

	for ( ; len > 0; len--, p++ )
	{
		int  c;

		if(*p == '\0')
		break;

		if (len <= 0)
		goto Fail;
		
		if ((*p >= 'a') && (*p <= 'f'))
			c = *p - 'a' + 10;
		else if ((*p >= 'A') && (*p <= 'F'))
			c = *p - 'A' + 10;
		else if ((*p >= '0') && (*p <= '9'))
			c = *p - '0';
		else
		{       
			D("strhex2int: fail \n");
			//goto Fail;
			break;
		}
		result = result*0x10 + c;
	}

	D("strhex2int: (%d) \n", result);	
	
	return  result;

Fail:
	return -1;
}

static int nmea_tokenizer_init( NmeaTokenizer*  t, const char*  p, const char*  end )
{
    int    count = 0;
    char*  q;

    // the initial '$' is optional
    if (p < end && p[0] == '$')
        p += 1;

    // remove trailing newline
    if (end > p && end[-1] == '\n') {
        end -= 1;
        if (end > p && end[-1] == '\r')
            end -= 1;
    }

    // get rid of checksum at the end of the sentecne
    if (end >= p+3 && end[-3] == '*') {
        end -= 3;
    }

    while (p < end) {
        const char*  q = p;

        q = memchr(p, ',', end-p);
        if (q == NULL)
            q = end;

        if (q >= p) {
            if (count < MAX_NMEA_TOKENS) {
                t->tokens[count].p   = p;
                t->tokens[count].end = q;
                count += 1;
            }
        }
        if (q < end)
            q += 1;

        p = q;
    }

    t->count = count;
    return count;
}

static Token  nmea_tokenizer_get( NmeaTokenizer*  t, int  index )
{
	Token  tok;
	static const char*  dummy = "";

	if (index < 0 || index >= t->count) 
	{
		tok.p = tok.end = dummy;
	} 
	else
	{
		tok = t->tokens[index];
	}
	
	return tok;
}

static int nmea_update_cwcn(Token cwcn)
{
	Token   tok = cwcn;

	if (tok.p >= tok.end)
		return -1;
	pthread_mutex_lock(&mutex_cwcn);
	cwcn_value = str2int(tok.p, tok.end);
	D("nmea_update_cwcn: value=%d \n", cwcn_value);	
	pthread_mutex_unlock(&mutex_cwcn);
	return 0;
}

static int nmea_update_tsx(Token tsx)
{
	Token   tok = tsx;

	if (tok.p >= tok.end)
		return -1;
	pthread_mutex_lock(&mutex_tsx);
	tsx_value = str2float(tok.p, tok.end);
	D("nmea_update_tsx: value=%f \n", tsx_value);
	pthread_mutex_unlock(&mutex_tsx);
	return 0;
}

static int nmea_update_tcxo(Token tcxo)
{
	Token   tok = tcxo;

	if (tok.p >= tok.end)
		return -1;
	pthread_mutex_lock(&mutex_tcxo);
	tcxo_value = str2float(tok.p, tok.end);
	D("nmea_update_tcxo: value=%f \n", tcxo_value);
	pthread_mutex_unlock(&mutex_tcxo);
	return 0;
}

static void  nmea_parse(const char* nmea, int length)
{
	NmeaTokenizer  tzer[1];
	Token          tok;

	if((chip_id == GREENEYE_II) && (eut_gps_state == 2))
	{
		cw_data_capture(nmea, length); // CW_CN0
	}
	else
	{
		if((length < 9) && (eut_gps_state != 5))
		{
			//E("nmea_parse: sentence too short.");
			return;
		}
	}

	nmea_tokenizer_init(tzer, nmea, nmea+length);

	tok = nmea_tokenizer_get(tzer, 0);
	if (tok.p + 5 > tok.end) 
	{
		//E("sentence id '%.*s' too short, ignored.", tok.end-tok.p, tok.p);
		return;
	}
	
	//if ( !memcmp(tok.p,"PCGDS",5))
	if ( !memcmp(tok.p,"PCGDS",5))
	{
		D("PCGDS enter now\n");
		Token  name = nmea_tokenizer_get(tzer,1);
		if( !memcmp(name.p,"CWCN0",5))
		{
			D("PCGDS ==>> CWCN0\n");
			Token  value = nmea_tokenizer_get(tzer,2);
			nmea_update_cwcn(value);
		}
		else if( !memcmp(name.p,"TSXTEMP",7))
		{
			D("PCGDS ==>> TSXTEMP\n");
			Token  value = nmea_tokenizer_get(tzer,2);
			nmea_update_tsx(value);
		}
		else if( !memcmp(name.p,"TCXO",4))
		{
			D("PCGDS ==>> TCXO\n");
			Token  value = nmea_tokenizer_get(tzer,2);
			nmea_update_tcxo(value);
		}
	}
}

static void location_callback(GpsLocation* location)
{
	fix_status = 1;
	memcpy(&GPSloc,location,sizeof(GpsLocation));
	D("%s called\n",__FUNCTION__);
}

static void status_callback(GpsStatus* status)
{
	D("%s called\n",__FUNCTION__);
}

static void sv_status_callback(GpsSvStatus* sv_status)
{
	D("%s called\n",__FUNCTION__);
	pthread_mutex_lock(&mutex_b);
	memcpy(&SvState,sv_status,sizeof(GpsSvStatus));
	pthread_mutex_unlock(&mutex_b);
}
static report_ptr g_func;

void set_report_ptr(report_ptr func)
{
	D("set callback by engpc");
	g_func = func;
}

static void nmea_callback(GpsUtcTime timestamp, const char* nmea, int length)
{
	int ret = 0;
	if(first_open == 0)
	{
		E("not init,callback fail");
		return;
	}
	if(g_func != NULL)
	{
		g_func(nmea,length);
	}
	nmea_parse(nmea,length);
}

static void set_capabilities_callback(uint32_t capabilities)
{
	D("%s called, capabilities: %d\n",__FUNCTION__,capabilities);
}

static void acquire_wakelock_callback()
{
	D("%s called\n",__FUNCTION__);
}

static void release_wakelock_callback()
{
	D("%s called\n",__FUNCTION__);
}

static void request_utc_time_callback()
{
	D("%s called\n",__FUNCTION__);
}

#ifdef GNSS_ANDROIDN
static void gnss_set_system_info_test(const GnssSystemInfo* info)
{
    D("%s called\n",__FUNCTION__);
}

static void gnss_sv_status_callback_test(GnssSvStatus* sv_info)
{
    D("%s called\n",__FUNCTION__);
}
#endif


static pthread_t create_thread_callback(const char* name, void (*start)(void *), void* arg)
{
	pthread_t pid;
	D("%s called\n",__FUNCTION__); 
	pthread_create(&pid, NULL,(void *)start, arg);
	return pid;
}

GpsCallbacks sGpsCallbacks = {
	sizeof(GpsCallbacks),
	location_callback,
	status_callback,
	sv_status_callback,
	nmea_callback,
	set_capabilities_callback,
	acquire_wakelock_callback,
	release_wakelock_callback,
	create_thread_callback,
	request_utc_time_callback,
#ifdef GNSS_ANDROIDN
    gnss_set_system_info_test,
    gnss_sv_status_callback_test
#endif
};

GpsInterface *pGpsface;
AGpsInterface *pAGpsface;

#ifdef GNSS_WR_REG
int write_register(unsigned int addr,unsigned int value)
{
	D("write_register =>> addr=0x%x, value=0x%x\n", addr, value);
	hal_write_register(addr,value);
	return 0;
}

unsigned int read_register(unsigned int addr)
{
	unsigned int value = 0;
	
	D("read_register =>> addr=0x%x\n",addr);
	value = hal_read_register(addr);
	D("read_register =>> addr=0x%x, value=0x%x\n",addr, value);
	return value;
}

int config_register(int data,char *rsp)
{
	unsigned int value = 0;
	unsigned int addr = 0x00F4;

	if((gps_search_state == 0) && (eut_gps_state == 0))
	{
		sprintf(rsp,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSEARCH);
		return 0;
	}

	D("config_register\n");
	value = read_register(addr);
	D("config_register: addr=0x%x, value=0x%x \n",addr,value);
	sprintf(rsp,"addr=0x%x, value=0x%x",addr,value);

	return 0;
}

#endif

void set_pc_mode(char input_pc_mode)
{
	pc_mode = input_pc_mode;
	D("set_pc_mode is enter\n");
}

int start_engine(void)
{
	pid_t pid; 
	pid=fork();
	if(pid==0)
	{
		D("start engine");
		system("GPSenseEngine");
	}
	return 0;
}

int search_for_engine(void)
{
	char rsp[256];
	FILE *stream;
	int ret = 0;

	memset(rsp,0,sizeof(rsp));

	stream = popen("ps | grep GPSenseEngine","r");

	fgets(rsp,sizeof(rsp),stream);
	if(strstr(rsp,"GPSenseEngine") != NULL)
	{
		D("GPSenseEngine is start success\n");
		ret = 1;
	}
	else
	{
		E("GPSenseEngine is start fail\n");
		ret = 0;
	}
	pclose(stream);
	return ret;
}

int gps_export_start(void)
{
	int ret = 0;
	D("gps_export_start is enter\n");
	if(pGpsface == NULL)
	{
		E("start fail,for not init");
		return ret;
	}
   	//pGpsface->start(); 
	//in future,should add ge2 option,such as if((eut_gps_state == 1) && (hardware = CELLGUIDE))
	//if((eut_gps_state == 1)&&(chip_id == GREENEYE_I))
	if(chip_id == GREENEYE_I)
	{
		//ret = search_for_engine();
		//if(ret == 0)
		//if(eutmode == 1)
		//{
		//	D("begin start engine");
		//	ret = start_engine();
		//}
		sleep(1);
		pGpsface->start(); 
		sleep(1);
	}
	else
	{
		usleep(500000);    // sleep(1)
		pGpsface->start();
		usleep(200000);    // sleep(1)
	}

	return ret;
}

int gps_export_stop(void)
{
	int ret =0;
	D("gps_export_stop ret is %d\n",ret);

	if(pGpsface == NULL)
	{
		E("stop fail,for not init");
		return ret;
	}
   	pGpsface->stop(); 
	sleep(4);
	return ret;
}

int set_gps_mode(unsigned int mode)
{
	int ret = 0;

	if(pGpsface == NULL)
	{
		E("set mode fail,for not init");
		return 0;
	}
	switch(mode)
	{
		case 0: // Hot start
			set_mode = HOT_START;
			pGpsface->delete_aiding_data(set_mode);   //trigger cs/hs/ws
			ret = 1;
			break;
		case 1: // Warm start
			set_mode = WARM_START;
			pGpsface->delete_aiding_data(set_mode);   //trigger cs/hs/ws
			ret = 1;
			break;
		case 2: // Cold start
			set_mode = COLD_START;
			pGpsface->delete_aiding_data(set_mode);   //trigger cs/hs/ws
			ret = 1;
			break;
		case 8: // ps start
			E("post debug is open");
			pGpsface->delete_aiding_data(100);   //trigger cs/hs/ws
			ret = 0;
			break;
		case 10: // TCXO mode
			D("set TCXO mode");
			pGpsface->delete_aiding_data(50001);	// TCXO stability test
			ret = 1;
			break;
		case 11: // GPS
		case 12: // GLONASS
		case 13: // Beidou
			D("set rf data tool mode");
			eut_gps_state = 5;
			pGpsface->delete_aiding_data(50003);	 // RF data tool
			ret = 1;
			break;
		case 20: // Fac start
			set_mode = FAC_START;
			pGpsface->delete_aiding_data(set_mode);   //trigger cs/hs/ws
			ret = 1;
			break;
       case 21:
           D("set TSX-TCXO  new mode");
           pGpsface->delete_aiding_data(50005);    // TCXO  stability test
           ret = 1;
           break;
		default:
			break;
	}

	return ret;
 }

int get_nmea_data(char *nbuff)
{
	int len = 0;
	D("get nmea data enter");
#if 0
	sem_wait(&sem_a);
	pthread_mutex_lock(&mutex_a);
	len = strlen(buf);  //don't add 1 for '\0'
	if(len > 9)
	{
		memcpy(nbuff,buf,len);
	}
	pthread_mutex_unlock(&mutex_a);
#endif
	return len;
}


void astatus_cb(AGpsStatus* status)
{
     D("astatus  enter");
    return;
}

pthread_t acreate_thread_cb(const char* name, void (*start)(void *), void* arg)
{
    pthread_t apid = 0;
    return apid;
}

AGpsCallbacks acallbacks = {
    astatus_cb,
    acreate_thread_cb,
};

int get_init_mode(void)
{
	if(first_open == 0)
	{
		void *handle;
		GpsInterface* (*get_interface)(struct gps_device_t* dev);
		void* (*get_extension)(const char* name);
		char *error;
		int i = 0;

		D("begin gps init\n");
		if(access("/system/etc/GPSenseEngine.xml",0) == -1)
		{
			//GreenEye2
			chip_id = GREENEYE_II;
			if(access("/data/gnss",0) == -1)
			{
				D("===========>>>>>>>>>>gnss file is not exit");
				system("mkdir /data/gnss");
				system("mkdir /data/gnss/supl");
				system("mkdir /data/gnss/config");
				system("mkdir /data/gnss/lte");
				chmod("/data/gnss",0777);
				chmod("/data/gnss/supl",0777);
				chmod("/data/gnss/config",0777);
				chmod("/data/gnss/lte",0777);
			}
			sleep(4);    // need 4s to wait for GE2 download complete
		}
		else
		{
			//GreenEye			
			chip_id = GREENEYE_I;
			if(access("/data/cg",0) == -1)
			{
				D("===========>>>>>>>>>>cg file is not exit");
				system("mkdir /data/cg");
				system("mkdir /data/cg/supl");
				system("mkdir /data/cg/online");
				chmod("/data/cg",0777);
				chmod("/data/cg/supl",0777);
				chmod("/data/cg/online",0777);
			}
		}
		
		D("before dlopen");
		handle = dlopen("/system/lib/hw/gps.default.so", RTLD_LAZY);
		if (!handle) {
		   E("%s\n", dlerror());
		   return INIT_MODE;
		}
		D("after dlopen\n");
		dlerror();    /* Clear any existing error */

		#ifdef GNSS_WR_REG
		hal_read_register= dlsym(handle, "app_read_register");
		hal_write_register= dlsym(handle, "app_write_register");
		#endif
		hal_read_tsxtemp = dlsym(handle, "engpc_read_tsxtemp");
		get_interface = dlsym(handle, "gps_get_hardware_interface");
		D("obtain GPS HAL interface \n");
		pGpsface = get_interface(NULL);
		pGpsface->init(&sGpsCallbacks);
		
		get_extension= dlsym(handle, "gps_get_extension"); 
		pAGpsface = get_extension("agps");
		pAGpsface->init(&acallbacks);
		first_open = 1;
		sem_init(&sem_a,0,0);
	}
	return INIT_MODE;
}

int get_stop_mode(void)
{
	return STOP_MODE;
}

int eut_parse(int data,int sub_data, char *rsp)
{
	eutmode = 1;

	if((data == 1)||(data == 2)||(data == 3)||(data == 4))
	{	
		D("%s\n", version);
		eut_gps_state = 1;
		if(first_open == 0)
		{
			get_init_mode();
			gps_mode = 0;
		}
		
		if(data == 2)
		{
			eut_gps_state = 2;
			pGpsface->delete_aiding_data(50002);    // CW CN0 test
			gps_mode = 50002;
		}
		else if(data == 3)
		{
			eut_gps_state = 3;
			pGpsface->delete_aiding_data(50004);    // TSX TEMP test
			gps_mode = 50004;
		}
		else if(data == 4)
		{
			eut_gps_state = 4;
			pGpsface->delete_aiding_data(50001);    // TCXO stability  test
			gps_mode = 50001;
		}
		else if(data == 1)
		{
			pGpsface->delete_aiding_data(65535);	// factory start
			gps_mode = 0;
		}
		
		gps_export_start();
		strcpy(rsp,EUT_GPS_OK);
	}
	else if(data == 0)
	{
		eut_gps_state = 0;
		gps_mode = 0;
		gps_export_stop();   //block,for test
		strcpy(rsp,EUT_GPS_OK);
	}

	return 0;
}

int eut_eq_parse(int data,int sub_data,char *rsp)
{
	sprintf(rsp,"%s%d",EUT_GPS_REQ,eut_gps_state);
	return 0;
}

int search_eq_parse(int data,int sub_data,char *rsp)
{
	sprintf(rsp,"%s%d",EUT_GPS_SEARCH_REQ,gps_search_state);
	return 0;
}

int search_parse(int data,int sub_data,char *rsp)
{	
	if(data != eut_gps_state)
	{
		eut_parse(data,sub_data,rsp);
	}
	gps_search_state = eut_gps_state;
	strcpy(rsp,EUT_GPS_OK);
	return 0;
}

int get_prn_list(char *rsp)
{
	int i = 0,lenth = 0;
	memcpy(rsp,EUT_GPS_PRN_REQ,strlen(EUT_GPS_PRN_REQ));
	lenth = strlen(EUT_GPS_PRN_REQ);
	pthread_mutex_lock(&mutex_b);
	for(i = 0; i < SvState.num_svs; i++)
	{
		lenth = lenth + sprintf(rsp + lenth,"%d,",SvState.sv_list[i].prn);
	}
	pthread_mutex_unlock(&mutex_b);
	return SvState.num_svs;
}

int prnstate_parse(int data,int sub_data,char *rsp)
{
	if((gps_search_state == 0) && (eut_gps_state == 0))
	{
		sprintf(rsp,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSEARCH);
		return 0;
	}

	if(get_prn_list(rsp) == 0)
	{
		sprintf(rsp,"%s%s",EUT_GPS_PRN_REQ,EUT_GPS_NO_FOUND_STAELITE);
	}
	return 0;
}

int snr_parse(int data,int sub_data,char *rsp)
{
	int max_id = 0,i = 0;
	D("snr parse enter");
	if((gps_search_state == 0) && (eut_gps_state == 0))
	{
		E("snr_parse: gps has not search");
		sprintf(rsp,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSEARCH);
		return 0;
	}
	if(SvState.num_svs == 0)
	{
		E("snr_parse: num_svs is 0, return \n");
		sprintf(rsp,"%s no sv_num is found",EUT_GPS_SNR_REQ);
		return 0;
	}
	pthread_mutex_lock(&mutex_b);
	for(i = 0; i < SvState.num_svs; i++)
	{
		if(SvState.sv_list[i].snr > SvState.sv_list[max_id].snr)
		{
			max_id = i;
		}
	}
	sprintf(rsp,"%s%f %s%d %s%d",EUT_GPS_SNR_REQ,SvState.sv_list[max_id].snr,
								EUT_GPS_SV_ID,SvState.sv_list[max_id].prn,
								EUT_GPS_SV_NUMS,SvState.num_svs);
	pthread_mutex_unlock(&mutex_b);
	D("snr_parse: %s",rsp);
	return 0;
}

int prn_parse(int data,int sub_data,char *rsp)
{
	int i = 0,found = 0;
	if((gps_search_state == 0) && (eut_gps_state == 0))
	{
		sprintf(rsp,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSEARCH);
		return 0;
	}

	pthread_mutex_lock(&mutex_b);
	for(i = 0; i < SvState.num_svs; i++)
	{
		if(SvState.sv_list[i].prn == data)
		{
			sprintf(rsp,"%s%d,CN0=%f",EUT_GPS_PRN_REQ,data,SvState.sv_list[i].snr);
			found = 1;
			break;
		}
	}
	pthread_mutex_unlock(&mutex_b);

	if(found == 0)
	{
		E("prn_parse: can not find prn and it's snr \n");
		sprintf(rsp,"%s%s",EUT_GPS_PRN_REQ,EUT_GPS_NO_FOUND_STAELITE);
	}
	return 0;
}

int fix_parse(int data,int sub_data,char *rsp)
{
	int i = 0,found = 0;
	if((gps_search_state == 0) && (eut_gps_state == 0))
	{
		sprintf(rsp,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSEARCH);
		return 0;
	}

 	if(fix_status == 0)
	{
		E("fix_parse: cannot fix location \n");
		sprintf(rsp,"+SPGPSTEST:LOCATION=FAIL");
	}	
	else
	{
		E("fix_parse: fixed \n");
		sprintf(rsp,"+SPGPSTEST:LOCATION=SUCC,%f,%f",GPSloc.latitude,GPSloc.longitude);
	}
	return 0;
}

int cwcn_parse(int data,int sub_data,char *rsp)
{	
	D("cwcn_parse  enter");
	if((gps_search_state == 0) && (eut_gps_state == 0))
	{
		E("cwcn_parse: gps has not search");
		sprintf(rsp,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSEARCH);
		return 0;
	}

	if(chip_id == GREENEYE_I)
	{
		pthread_mutex_lock(&mutex_cwcn);
		D("cwcn_parse: cwcn_value=%d \n", cwcn_value);
		sprintf(rsp,"%s%d",EUT_GPS_RSSI_REQ, cwcn_value);
		pthread_mutex_unlock(&mutex_cwcn);
		D("cwcn_parse: %s",rsp);
	}
	else
	{
		D("cwcn_parse: CN0=%d \n", CN0);
		sprintf(rsp,"%s%d",EUT_GPS_RSSI_REQ, CN0);
		D("cwcn_parse: %s",rsp);
	}

	return 0;
}

int tsx_temp_parse(int data,int sub_data,char *rsp)
{	
	int i=0;
	double tsx_temp = 0.0;

	D("tsx_temp_parse  enter");
	if((gps_search_state == 0) && (eut_gps_state == 0))
	{
		E("tsx_temp_parse: gps has not search");
		sprintf(rsp,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSEARCH);
		return 0;
	}
	
	#if 0
	for(i=0;i<10;i++)
	{
		pthread_mutex_lock(&mutex_tsx);
		tsx_temp = tsx_value;
		pthread_mutex_unlock(&mutex_tsx);

		if(tsx_temp!=0)
		{
			D("tsx_temp_parse: tsx_temp=%f \n",tsx_temp);
			sprintf(rsp,"%s%f",EUT_GPS_TSXTEMP_REQ,tsx_temp);
			D("tsx_temp_parse: %s",rsp);
			return 0;
		}
		sleep(1);   // wait 1s
	}
	sprintf(rsp,"%s can not get tsx temperature",EUT_GPS_TSXTEMP_REQ);
	#endif

	tsx_temp = hal_read_tsxtemp();
	sprintf(rsp,"%s%f",EUT_GPS_TSXTEMP_REQ,tsx_temp);
	D("tsx_temp_parse: %s",rsp);
	return 0;
}

int tcxo_stability_parse(int data,int sub_data,char *rsp)
{	
	D("tcxo_stability_parse  enter");
	if((gps_search_state == 0) && (eut_gps_state == 0))
	{
		E("tcxo_stability_parse: gps has not search");
		sprintf(rsp,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSEARCH);
		return 0;
	}

	pthread_mutex_lock(&mutex_tcxo);
	D("tcxo_stability_parse: tcxo_value=%f \n",tcxo_value);
	sprintf(rsp,"%s%f",EUT_GPS_TCXO_REQ,tcxo_value);
	pthread_mutex_unlock(&mutex_tcxo);
	D("tcxo_stability_parse: %s",rsp);
	return 0;
}

int eut_read_register(int addr, int sub_data, char *rsp)
{	
	D("eut_read_register  enter");	
	unsigned int value = 0;
	
	if((gps_search_state == 0) && (eut_gps_state == 0))
	{
		E("eut_read_register: gps has not search");
		sprintf(rsp,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSEARCH);
		return 0;
	}
	D("eut_read_register: addr=0x%x \n",addr);
	value = read_register(addr);
	
	sprintf(rsp,"%s0x%x",EUT_GPS_READ_REQ,value);
	D("eut_read_register: %s",rsp);
	return 0;
}

int eut_write_register(int addr, int value, char *rsp)
{	
	D("eut_write_register  enter");
	int ret = 0;
	
	if((gps_search_state == 0) && (eut_gps_state == 0))
	{
		E("eut_write_register: gps has not search");
		sprintf(rsp,"%s%d",EUT_GPS_ERROR,EUT_GPSERR_PRNSEARCH);
		return 0;
	}

	D("eut_write_register: addr=0x%x, value=0x%x \n",addr, value);
	ret = write_register(addr,value);
	strcpy(rsp,EUT_GPS_OK);
	D("eut_write_register: %s",rsp);
	return 0;
}

//typedef int (*eut_function)(int data,char *rsp);
typedef int (*eut_function)(int data,int sub_data, char *rsp);
typedef struct
{
	char *name;
	eut_function func;
}eut_data; 

eut_data eut_gps_at_table[ENG_GPS_NUM] = {
	{"EUT?",eut_eq_parse},
	{"EUT",eut_parse},
	{"SEARCH?",search_eq_parse},
	{"SEARCH",search_parse},
	{"PRN?",prnstate_parse},
	{"SNR?",snr_parse},
	{"PRN",prn_parse},
	{"LOCATION?",fix_parse},
	{"RSSI?",cwcn_parse},            // CW CN0
	{"TSXTEMP?",tsx_temp_parse},     // TSX temp read
	{"TCXO?",tcxo_stability_parse},  // TCXO stability verify
	{"READ",eut_read_register},
	{"WRITE",eut_write_register},
};

#if 0
int (*state[ENG_GPS_NUM])(int data,char *rsp) = {
	eut_parse,
	eut_eq_parse,
	search_eq_parse,
	search_parse,
	prnstate_parse,
	snr_parse,
	prn_parse,
};

static char *eut_gps_name[ENG_GPS_NUM] = {
	"EUT",
	"EUT?",
	"SEARCH?",
	"SEARCH",
	"PRNSTATE?",
	"SNR?",
	"PRN",	
};

//EUT
int get_cmd_index(char *buf)
{
    int i;
    for(i = 0;i < ENG_GPS_NUM;i++)
	{
        if(strstr(buf,eut_gps_name[i]) != NULL)
        {
            break;
        }
    }
    return i;
}
#endif

int get_sub_str(char *buf, char **revdata, char a, char *delim, unsigned char count, unsigned char substr_max_len)
{
    int len, len1, len2;
    char *start = NULL;
    char *substr = NULL;
    char *end = buf;
    int str_len = strlen(buf);

    start = strchr(buf, a);
    substr = strstr(buf, delim);
    
    if(!substr)
    {
        /* if current1 not exist, return this function.*/
        return 0;
    }

    while (end && *end != '\0')
    {
        end++;
    }

    if((NULL != start) && (NULL != end))
    {
        char *tokenPtr = NULL;
        unsigned int index = 1; /*must be inited by 1, because data[0] is command name */

        start++;
        substr++;
        len = substr - start - 1;

        /* get cmd name */
        memcpy(revdata[0], start, len);

        /* get sub str by delimeter */
        tokenPtr = strtok(substr, delim);
        while(NULL != tokenPtr && index < count) 
        {
            strncpy(revdata[index++], tokenPtr, substr_max_len);

            /* next */
            tokenPtr = strtok(NULL, delim);
        }

    }

    return 0;
}

int get_sub_str_colon(char *buf, char **revdata, char a, char *delim, unsigned char count, unsigned char substr_max_len)
{
    int len, len1, len2;
    char *start = NULL;
    char *substr = NULL;
    char *end = buf;
    int str_len = strlen(buf);

    start = strchr(buf, a);
    substr = strstr(buf, delim);
    
    if(!substr)
    {
        /* if current1 not exist, return this function.*/
        return 0;
    }

    while (end && *end != '\0')
    {
        end++;
    }

    if((NULL != start) && (NULL != end))
    {
        char *tokenPtr = NULL;
        unsigned int index = 1; /*must be inited by 1, because data[0] is command name */

        start++;
        substr++;
        len = substr - start - 1;

        /* get cmd name */
        memcpy(revdata[0], start, len);

        /* get sub str by delimeter */
        tokenPtr = strtok(substr, ":");
        while(NULL != tokenPtr && index < count) 
        {
            strncpy(revdata[index++], tokenPtr, substr_max_len);

            /* next */
            tokenPtr = strtok(NULL, ":");
        }

    }

    return 0;
}

void gps_eut_parse(char *buf,char *rsp)
{
	int i = 0;   //data is get from buf,used arg1,arg2.
	int arg1_data,arg2_data;
	static char args0[32+1];
	static char args1[32+1];
	static char args2[32+1];
	static char args3[32+1];
	//should init to 0
	memset(args0,0,sizeof(args0));
	memset(args1,0,sizeof(args1));
	memset(args2,0,sizeof(args2));
	memset(args3,0,sizeof(args3));

	char *data[4] = {args0, args1, args2, args3};
	//get_sub_str(buf, data, '=', ",", 4, 32);
	get_sub_str_colon(buf, data, '=', ",", 4, 32);

	D("gps_eut_parse enter");
#if 0
	index = get_cmd_index(buf);
	if((index > ENG_GPS_NUM - 1) || (index < 0))
	{
		E("get index error!!\n");
		return;
	}
	state[index](data,rsp);
#endif
	for(i = 0;i < ENG_GPS_NUM;i++)
	{
		if(strstr(buf,eut_gps_at_table[i].name) != NULL)
		{			
			if(!memcmp(eut_gps_at_table[i].name,"READ",strlen("READ")))
			{
				arg1_data = strhex2int(data[1],strlen(data[1]));
				D("command arg1=%d, arg2=%d,i=%d \n",atoi(data[1]),atoi(data[2]),i);
				eut_gps_at_table[i].func(arg1_data,atoi(data[2]),rsp);
				break;
			}
			else if(!memcmp(eut_gps_at_table[i].name,"WRITE",strlen("WRITE")))
			{
				arg1_data = strhex2int(data[1],strlen(data[1]));
				arg2_data = strhex2int(data[2],strlen(data[2]));
				D("command arg1=%d, arg2=%d,i=%d \n",arg1_data,arg2_data,i);
				eut_gps_at_table[i].func(arg1_data,arg2_data,rsp);
				break;
			}
			else
			{
				D("command arg1=%d, arg2=%d,i=%d \n",atoi(data[1]),atoi(data[2]),i);
				eut_gps_at_table[i].func(atoi(data[1]),atoi(data[2]),rsp);
				break;
			}
		}
	}
	return;
}

