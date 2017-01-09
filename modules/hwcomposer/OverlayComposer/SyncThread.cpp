#include <pthread.h>
#include <semaphore.h>
#include <utils/threads.h>
#include <utils/Log.h>

using namespace android;

sem_t         sync_sem;
bool fb_postdone = false;
static int sem_inited = 0;
static Mutex semLock;
#define TIME_SPEC_NSEC_MAX_VALUE  1000000000UL

int InitSem(void)
{
    Mutex::Autolock lock(semLock);
    if (sem_inited == 0)
    {
        sem_init(&sync_sem, 0, 0);
        sem_inited = 1;
     }
     return 0;
}
void exhaustAllSem()
{
    while(sem_trywait(&sync_sem) == 0)
    {
        ;
    }
}
int semWaitTimedOut(unsigned int milliSeconds)
{
    struct timespec ts;
    struct timespec tsoriginal;
    int ret;
    unsigned int seconds = (milliSeconds/1000);
    unsigned int millDelta = (milliSeconds%1000);
    clock_gettime(CLOCK_REALTIME, &ts);
    tsoriginal.tv_sec = ts.tv_sec;
    tsoriginal.tv_nsec = ts.tv_nsec;
    ts.tv_sec += seconds;
    ts.tv_nsec += (millDelta * 1000000);
    if((unsigned int)(ts.tv_nsec) >= TIME_SPEC_NSEC_MAX_VALUE)
    {
     ts.tv_sec += 1;
     ts.tv_nsec -= TIME_SPEC_NSEC_MAX_VALUE;
    }
    ret = sem_timedwait(&sync_sem , &ts);
    if(-1 == ret)
    {
     ALOGE("sem timedwait error:%d" , errno);
     return ret;
    }
    return ret;
}

int postSem()
{
    return sem_post(&sync_sem);
}

int desroySem()
{
    Mutex::Autolock lock(semLock);
    if(sem_inited)
    {
        sem_inited = 0;
        return sem_destroy(&sync_sem);
    }
    return 0;
}
