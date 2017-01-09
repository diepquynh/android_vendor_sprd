
#ifndef NATIVECRASH_H
#define NATIVECRASH_H

#include "CrashBehavior.h"
#include "AprData.h"

class NativeCrash : public CrashBehavior
{
public:
	NativeCrash(AprData* aprData);
	virtual ~NativeCrash() {};

	void init_wd_context(vector<wd_context>* pvec);
	void handle_event(int ifd, vector<wd_context>* pvec,
				struct inotify_event* event);

private:
	int event_wd0(int ifd, vector<wd_context>* pvec,
				struct inotify_event* event);
	int event_wd1(struct inotify_event* event);

private:
	AprData* m_aprData;
	struct e_info m_ei;
};

#endif
