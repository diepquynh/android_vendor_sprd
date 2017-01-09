



#ifndef APRDATA_H
#define APRDATA_H

#include "Observable.h"
#include "Thread.h"
#include <string>

enum EExceptionType {
	E_ANR,
	E_NATIVE_CRASH,
	E_JAVA_CRASH,
	E_MODEM_BLOCKED,
	E_MODEM_ASSERT,
	E_WCN_ASSERT,
	E_BOOTMODE,
	E_MAXMUM,
};

struct e_info {
	EExceptionType et;
	void *private_data;
};

char* get_et_name(EExceptionType et);

class AprData:public Observable
{
public:
	AprData();
	~AprData();
	virtual void getSubjectInfo();

protected:
	void init();

public:
	// <hardwareVersion> </hardwareVersion>
	string getHardwareVersion();
	// <SN> </SN>
	string getSN();
	// <buildNumber> </buildNumber>
	string getBuildNumber();
	// <CPVersion> </CPVersion>
	string getCPVersion();
	// <extraInfo> </extraInfo>
	string getExtraInfo();
	// <xxxTime> </xxxTime>
	int getStartTime(char *strbuf);
	int getUpTime(char* strbuf);
	// <timestamp></timestamp>
	int getWallClockTime(char* strbuf, size_t max, const char* format="%Y-%m-%d %H:%M:%S");
	// get ro.boot.mode
	string getBootMode();
	// set BootMode switch
	void setBootModeSW(bool sw);

private:
	string m_hardwareVersion;
	string m_SN;
	string m_buildNumber;
	string m_CPVersion;
	string m_extraInfo;
	string m_bootMode;
	bool m_swBM;
	int64_t m_sTime;
	// <exceptions>
	// <entry>
//	string m_timestamp;
//	string m_type;
	// </entry>
	// </exceptions>
};

#endif
