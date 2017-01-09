#ifndef CMUTEX_H
#define CMUTEX_H

extern "C"{
#include <pthread.h>
}

class CMutex
{
    public:
        CMutex();
        virtual ~CMutex();

    public:
        int trylock();
        int wait(unsigned int msec/*0-999*/, unsigned int sec = 0);
        int wait();
        int lock();
        int unlock();
        int signal();

    protected:
        void __init();
        void __uninit();

    private:
        pthread_cond_t mCond;
        pthread_mutex_t mMutex;
};

#endif // CMUTEX_H
