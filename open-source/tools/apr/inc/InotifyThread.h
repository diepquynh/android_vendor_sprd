


#ifndef INOTIFYTHREAD_H
#define INOTIFYTHREAD_H
#include <string>
#include <vector>
#include <sys/inotify.h>
#include <errno.h>

#include "Thread.h"
#include "CrashBehavior.h"
using namespace std;

class InotifyThread : public Thread
{
public:
	InotifyThread(CrashBehavior* cb);
	~InotifyThread();

protected:
	virtual void Setup();
	virtual void Execute(void* arg);
private:
	int _inotify_init();
	int _inotify_dele();
private:
	/* inotify */
	int m_ifd;
	vector<wd_context> m_wds;
	CrashBehavior *m_cb;
};

#endif

