#ifndef CRASHBEHAVIOR_H
#define CRASHBEHAVIOR_H

#include <string>
#include <vector>

using namespace std;

struct wd_context {
	int wd;
	uint32_t mask;
	string pathname;

	wd_context(string _pn, uint32_t _mask)
	{
		wd = -1;
		pathname = _pn;
		mask = _mask;
	}
};

class CrashBehavior
{
public:
	CrashBehavior() {};
	virtual ~CrashBehavior() {} ;

	virtual void init_wd_context(vector<wd_context>* pvec)=0;
	virtual void handle_event(int ifd, vector<wd_context>* pvec,
					struct inotify_event* event)=0;
};

#endif
