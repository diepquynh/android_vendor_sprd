



#include "common.h"
#include "AprData.h"

char* get_et_name(EExceptionType et)
{
	switch (et) {
	case E_ANR:
		return "anr";
	case E_NATIVE_CRASH:
		return "native crash";
	case E_JAVA_CRASH:
		return "java crash";
	case E_MODEM_BLOCKED:
		return "modem blocked";
	case E_MODEM_ASSERT:
		return "modem assert";
	case E_WCN_ASSERT:
		return "wcn assert";
	}

	return "unknown";
}

AprData::AprData()
{
	init();
}

AprData::~AprData()
{

}

void AprData::getSubjectInfo()
{
	APR_LOGD("AprData::getSubjectInfo()\n");
	return;
}

void AprData::init()
{
	APR_LOGD("AprData::init()\n");
	char value[PROPERTY_VALUE_MAX];
	char *default_value = (char*)"unknown";
	int iter;

	m_sTime = uptime(NULL);
	//     <hardwareVersion> </hardwareVersion>
	property_get("ro.product.hardware", value, default_value);
	m_hardwareVersion.assign(value);
	//     <SN> </SN>
	property_get("ro.boot.serialno", value, default_value);
	m_SN.assign(value);
	//     <buildNumber> </buildNumber>
	property_get("ro.build.description", value, default_value);
	m_buildNumber.assign(value);
	//     <CPVersion> </CPVersion>
	for(iter=1; iter <= 60; iter++)
	{
		property_get("gsm.version.baseband", value, default_value);
		if (strcmp(value, default_value))
			break;
		sleep(5);
	}
	m_CPVersion.assign(value);
	//     <extraInfo> </extraInfo>
	property_get("ro.sprd.extrainfo", value, default_value);
	m_extraInfo.assign(value);
	// get ro.boot.mode
	property_get("flag.apr.bootmode", value, default_value);
	if (strcmp(value, "1")) {
		property_get("ro.boot.mode", value, default_value);
		m_bootMode.assign(value);
		property_set("flag.apr.bootmode", "1");
	} else {
		m_bootMode.assign(default_value);
	}
	m_swBM = false;
}

string AprData::getHardwareVersion()
{
	return m_hardwareVersion;
}

string AprData::getSN()
{
	return m_SN;
}

string AprData::getBuildNumber()
{
	return m_buildNumber;
}

string AprData::getCPVersion()
{
	return m_CPVersion;
}

string AprData::getExtraInfo()
{
	return m_extraInfo;
}

int AprData::getStartTime(char* strbuf)
{
	int ret = -1;
	if(strbuf) {
		sprintf(strbuf, "%lld", m_sTime);
		ret = 0;
	}

	return ret;
}

int AprData::getUpTime(char* strbuf)
{
	int64_t ut;
	ut = uptime(strbuf);
	m_sTime = ut;
	return 0;
}

int AprData::getWallClockTime(char* strbuf, size_t max, const char* format)
{
	getdate(strbuf, max, format);
	return 0;
}

string AprData::getBootMode()
{
	string tmp;
	if (m_swBM) {
		return m_bootMode;
	} else {
		tmp.assign("unknown");
		return tmp;
	}
}

void AprData::setBootModeSW(bool sw)
{
	m_swBM = sw;
}
