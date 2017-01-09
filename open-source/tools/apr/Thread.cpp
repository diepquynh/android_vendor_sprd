
#include "common.h"
#include "Thread.h"

using namespace std;

Thread::Thread()
{
	ThreadId_ = -1;
	Arg_ = NULL;
}

Thread::~Thread() {}

int Thread::StartWithPipe(void* arg)
{
	int ret = 0;
	if (pipe(CtrlPipe)) {
		APR_LOGE("pipe failed (%s)\n", strerror(errno));
		return -1;
	}

	ret = this->Start(arg);
	return  ret;
}

int Thread::Start(void* arg)
{
	Arg(arg); // store user data
	if (pthread_create(&ThreadId_, NULL, Thread::EntryPoint, this)) {
		APR_LOGE("pthrad_create (%s)\n", strerror(errno));
		return -1;
	}

	return 0;
}

int Thread::StopWithPipe()
{
	char c = CtrlPipe_Shutdown;
	int rc;

	rc = TEMP_FAILURE_RETRY(write(CtrlPipe[1], &c, 1));
	if (rc != 1) {
		APR_LOGE("Error writing to control pipe (%s)\n", strerror(errno));
		return -1;
	}

	void *ret;
	if (pthread_join(ThreadId_, &ret)) {
		APR_LOGE("Error joining to thread (%s)\n", strerror(errno));
		return -1;
	}
	ThreadId_ = -1;
	close(CtrlPipe[0]);
	close(CtrlPipe[1]);
	CtrlPipe[0] = -1;
	CtrlPipe[1] = -1;

	return 0;
}

int Thread::Stop()
{
	void *ret;
	if (pthread_kill(ThreadId_, 0) != ESRCH) {
		APR_LOGD("thread %d exists!\n", ThreadId_);
		pthread_kill(ThreadId_, SIGQUIT);

		if (pthread_join(ThreadId_, &ret)) {
			APR_LOGE("Error joining to thread (%s)\n", strerror(errno));
			return -1;
		}
		ThreadId_ = -1;

		Dispose();
	}

	return 0;
}

int Thread::Run(void* arg)
{
	Setup();
	Execute(arg);

	return 0;
}

/* static */
void* Thread::EntryPoint(void* pthis)
{
	Thread* pt = static_cast<Thread*>(pthis);
	pt->Run(pt->Arg());
	pthread_exit(NULL);
	return NULL;
}

void Thread::Setup()
{
	APR_LOGD("Thread::Setup()\n");
	// Do any setup here
}

void Thread::Execute(void* arg)
{
	APR_LOGD("Thread::Execute()\n");
	// Your code goes here
}

void Thread::Dispose()
{
	APR_LOGD("Thread::Dispose()\n");
	// Release resource
}

