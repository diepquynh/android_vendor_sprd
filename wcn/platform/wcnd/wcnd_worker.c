#define LOG_TAG 	"WCND"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <ctype.h>
#include <pthread.h>
#include <errno.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <signal.h>

#include "wcnd.h"
#include "wcnd_sm.h"


/**
* Note: if need, the worker threads can be extended to a thread pool.
*/



/*
* The thread to handle the engineer command
*/
static void* eng_worker_thread(void *arg)
{
	WcndManager *pWcndManger = (WcndManager *)arg;
	int retval = 0;
	WcndWorker *pWorker = NULL;

	/* loop looking for work */
	for (;;)
	{
		if (pthread_mutex_lock(&(pWcndManger->worker_lock)) != 0)
		{
			/* Fatal error */
			WCND_LOGE("!Fatal: mutex lock failed\n");
		}

		while (pWcndManger->eng_cmd_queue == NULL)
		{
			/* Sleep until be wakeup */
			retval = pthread_cond_wait(&(pWcndManger->worker_cond),
				&(pWcndManger->worker_lock)); //need timeout??

		}

		pWorker = pWcndManger->eng_cmd_queue;
		if(pWorker)
			pWcndManger->eng_cmd_queue = pWorker->next;
		else
			pWcndManger->eng_cmd_queue = NULL;

		if (pthread_mutex_unlock(&(pWcndManger->worker_lock)) != 0)
		{
			/* Fatal error */
			WCND_LOGE("Fatal: mutex unlock failed!");
		}

		if (pWorker)
		{
			if(pWorker->handler)
				pWorker->handler(pWorker);

			if(pWorker->data) free(pWorker->data);

			free(pWorker);
			pWorker = NULL;
		}
	}

	WCND_LOGE("eng_worker_thread exit unexceptly!!");
	return NULL;	
}

/*
* To init a worker threads.
* Now noly engineer mode work thread is needed.
*/
int wcnd_worker_init(WcndManager *pWcndManger)
{
	if(!pWcndManger) return -1;

	pthread_mutex_init(&pWcndManger->worker_lock, NULL);
	if (pthread_cond_init(&(pWcndManger->worker_cond), NULL) != 0)
	{
		WCND_LOGE("wcnd_worker_init: pthread_cond_init (%s)", strerror(errno));
		return -1;
	}


	pthread_t thread_id;

	if (pthread_create(&thread_id, NULL, eng_worker_thread, pWcndManger))
	{
		WCND_LOGE("wcnd_worker_init: pthread_create (%s)", strerror(errno));
		return -1;
	}

	return 0;
}

/*
* To put a worker to the correct queue specified by 'type' (Now 'type' is not used, because
* only engineer mode is needed currently).
* Then wake up the thread to do it.
*/
int wcnd_worker_dispatch(WcndManager *pWcndManger, int (*handler)(void *), char *data, int fd, int type)
{
	WcndWorker *pNewWorker = NULL;

	if(!pWcndManger || !data || !handler) return -1;

	if (pthread_mutex_lock(&(pWcndManger->worker_lock)) != 0)
	{
		WCND_LOGE("Mutex lock failed!");
		return -1;
	}

	pNewWorker = (WcndWorker *)malloc(sizeof(WcndWorker));
	if(!pNewWorker)
	{
		WCND_LOGE("wcnd_worker_dispatch Malloc failed!");

		if (pthread_mutex_unlock(&(pWcndManger->worker_lock)) != 0) {
			WCND_LOGE("Mutex unlock failed!");
		}

		return -1;
	}

	pNewWorker->handler = handler;
	pNewWorker->ctx = pWcndManger;
	pNewWorker->data = strdup(data);
	pNewWorker->replyto_fd = fd;
	pNewWorker->next = NULL;

	WcndWorker *item = pWcndManger->eng_cmd_queue;

	if (item == NULL)
	{
		pWcndManger->eng_cmd_queue = pNewWorker;
	}
	else
	{
		while(item->next)
			item = item->next;
		item->next = pNewWorker;
	}

	pthread_cond_signal(&(pWcndManger->worker_cond));

	if (pthread_mutex_unlock(&(pWcndManger->worker_lock)) != 0) {
		WCND_LOGE("Mutex unlock failed!");
	}

	return 0;

}





