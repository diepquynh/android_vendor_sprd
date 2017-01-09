


#ifndef THREAD_H
#define	THREAD_H

#include <pthread.h>

#define CtrlPipe_Shutdown 0

class Thread
{
public:
	Thread();
	virtual ~Thread();
	int Start(void* arg);
	int Stop();
	int StartWithPipe(void* arg);
	int StopWithPipe();

protected:
	int Run(void* arg);
	static void * EntryPoint(void*);
	virtual void Setup();
	virtual void Execute(void*);
	virtual void Dispose();
	void * Arg() const { return Arg_; }
	void Arg(void* a) { Arg_ = a; }

private:
	pthread_t ThreadId_;
	void* Arg_;

protected:
	int CtrlPipe[2];
};

#endif /* THREAD_H */
