#ifndef __SPRD_DM_TASK_H__
#define  __SPRD_DM_TASK_H__

#include "sprd_dm.h"

#ifdef __cplusplus
extern "C" {
#endif  


typedef enum{
	SPDM_TASK_START = 0,
  	SPDM_TASK_EXIT,
  	SPDM_TASK_ATTENTION,
  	SPDM_TASK_NOTIFY,
  	SPDM_TASK_NOTIFYRST,
  	SPDM_TASK_TRANSPORT,
  	SPDM_TASK_CLOSE
}SPDM_TASK_TYPE;


typedef enum
{
	SPRD_DM_STATE_IDLE,
	SPRD_DM_STATE_CONTINUE,
	SPRD_DM_STATE_WAITUI,
	SPRD_DM_STATE_CANCEL,
	SPRD_DM_STATE_EXIT
}SPRD_DM_TASK_STATE;

typedef struct SPRD_TASK_REQ_TAG
{
	SPDM_TASK_TYPE type;
	int				  result_reason;
	char 			*buf;
	int 				buflen;
	int 				timeout;
	struct SPRD_TASK_REQ_TAG *next;
}SPRD_TASK_REQ_T;

bool spdm_task_init();
bool spdm_task_check_running(void);
bool spdm_task_sendsignal(SPRD_TASK_REQ_T *req_ptr);
void spdm_task_need_confirm_msg(char *msg, int msglen, int timeout);


#ifdef __cplusplus
}
#endif  

#endif



