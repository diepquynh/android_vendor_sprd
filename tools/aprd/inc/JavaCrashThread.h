
#ifndef LOGGERTHREAD_H
#define LOGGERTHREAD_H
#include "Thread.h"
#include "AprData.h"

struct log_device_t;

class JavaCrashThread : public Thread
{
public:
	JavaCrashThread(AprData* aprData);
	~JavaCrashThread();

protected:
	log_device_t* m_devices;
	int m_devCount;
	struct logger_list* m_logger_list;
	void processBuffer(log_device_t* dev, struct log_msg *buf);

	virtual void Setup();
	virtual void Execute(void* arg);
	virtual void Dispose();

private:
	AprData* m_aprData;
	struct e_info m_ei;
};

#endif

