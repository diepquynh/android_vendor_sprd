#include "CMutex.h"
#include <sys/time.h>
#include <unistd.h>

CMutex::CMutex()
{
    __init();
}

CMutex::~CMutex()
{
    __uninit();
}


void CMutex::__init()
{
    pthread_mutex_init(&mMutex, NULL);
    pthread_cond_init(&mCond, NULL);
}

void CMutex::__uninit()
{
    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCond);
}

int CMutex::wait()
{
    return pthread_cond_wait(&mCond, &mMutex);
}

int CMutex::wait(unsigned int msec, unsigned int sec)
{
    struct timespec  t;
    struct timeval now;

    gettimeofday(&now, 0);
    t.tv_sec = now.tv_sec + sec;
    t.tv_nsec = now.tv_usec + sec * 1000;

    return pthread_cond_timedwait(&mCond, &mMutex, &t);
}

int CMutex::lock()
{
    return pthread_mutex_lock(&mMutex);
}

int CMutex::signal()
{
    return pthread_cond_signal(&mCond);
}

int CMutex::unlock()
{
    return pthread_mutex_unlock(&mMutex);
}



