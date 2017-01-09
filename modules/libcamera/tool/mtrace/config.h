#include <stdlib.h>
#include <pthread.h>

#ifndef __ROM_
   #define  TRACE_MEM_PRINT_ENABLE   1
   #define  TRACE_MEM_MODE   0                   //0,1,2,3 ¿ÉÒÔÐÞ¸Ä
#else   //dont modify below
   #define  TRACE_MEM_PRINT_ENABLE   0
   #define  TRACE_MEM_MODE   0
#endif

#define MAX_ALLOC_ENTRYS	256

#define raw_calloc(n, size)      calloc(n, size)
#define raw_malloc(size)         malloc(size)
#define raw_realloc(p, size)     realloc(p, size)
#define raw_free(p)              free(p)

#define DECLEAR_KEY(x)           pthread_mutex_t *x = NULL
#define INIT_THREAD_MUTEX(x)     {x=(pthread_mutex_t *)raw_calloc(sizeof(pthread_mutex_t),1); \
									if(x) pthread_mutex_init(x, NULL);\
								 }
#define THREAD_LOCK(x)   pthread_mutex_lock(x)
#define THREAD_UNLOCK(x) pthread_mutex_unlock(x)


#if TRACE_MEM_PRINT_ENABLE

#include <sys/types.h>
#include <utils/Log.h>

//#define mtrace_log_error  ALOGE
//#define mtrace_log_info   ALOGI
#define mtrace_log_error(format,...)	ALOGE("mtrace:error " format,##__VA_ARGS__)
#define mtrace_log_warn(format,...)		ALOGW("mtrace:warn " format,##__VA_ARGS__)
#define mtrace_log_info(format,...)		ALOGV("mtrace:info " format,##__VA_ARGS__)

#else

#define mtrace_log_error(format,...)	//
#define mtrace_log_warn(format,...)		//
#define mtrace_log_info(format,...)		//

#endif

