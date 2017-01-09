
#ifndef __SPRDDM_API_H__
#define __SPRDDM_API_H__


#define bool int
#define TRUE 	1
#define FALSE 	0


#ifdef __cplusplus
extern "C" {
#endif  

typedef  enum{
	SPRD_DM_NETWORK_BREAK = 0,
	SPRD_DM_USER_STOP,
	SPRD_DM_PARSE_ERROR,
	SPRD_DM_FINISH
}SPRD_DM_REASON;

#define  LOCAL_MEMPOOL   1  //we use global memory pool
#define MAX_LOCAL_POOL_LEN	512*1024

bool spdm_start(int type, char * data, int datalen); 

bool spdm_receiveData(char * data, int datalen);

void spdm_DialogconfirmCb(int retcode);

void spdm_stopDm(int reason); 

bool spdm_isDmRunning(); 

int   spdm_print(const char *fmt,...);

#ifdef __cplusplus
}
#endif 
#endif






