


#ifndef MODEMTHREAD_H
#define MODEMTHREAD_H
#include "Thread.h"
#include "AprData.h"

class ModemThread : public Thread
{
public:
	ModemThread(AprData* aprData);
	~ModemThread();

protected:
	virtual void Setup();
	virtual void Execute(void* arg);
	int ConnectService(const char *socket_name);

private:
	AprData* m_aprData;
	struct e_info m_ei;
};

#endif

