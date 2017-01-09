/*******************************************************************************
 *
 * Copyright (C) u-blox ag
 * u-blox ag, Thalwil, Switzerland
 *
 * All rights reserved.
 *
 * This  source  file  is  the  sole  property  of  u-blox  AG. Reproduction or
 * utilization of this source in whole or part is forbidden without the written 
 * consent of u-blox AG.
 *
 *******************************************************************************
 *
 * Project: PE_AND
 *
 ******************************************************************************/
/*!
  \file
  \brief  Interface for the logger

  provide logging capabilities
*/
/*******************************************************************************
 * $Id: ubx_log.h 62581 2012-10-17 16:46:03Z jon.bowern $
 ******************************************************************************/

#ifndef __UBX_LOG_H__
#define __UBX_LOG_H__


#define LOG_TAG "u-blox"        //!< Tag for logging
#define LOG_NDEBUG 0
#include <cutils/log.h>

#ifdef SUPL_ENABLED 
#include "ULP-PDU.h"
#include "PDU.h"
#endif

// macro names have changed since Android 4.0.3 (ALOGV() instead of LOGV() etc. in 4.0.9)
// see system/core/include/cutils/log.h
#ifndef LOGV
#  define LOGV ALOGV
#endif
#ifndef LOGD
#  define LOGD ALOGD
#endif
#ifndef LOGI
#  define LOGI ALOGI
#endif
#ifndef LOGW
#  define LOGW ALOGW
#endif
#ifndef LOGE
#  define LOGE ALOGE
#endif




#define LOG_SPECIAL_BUFFER_COUNT 4
#if defined MALLOC_DEBUG

#define MC_MALLOC(size) debug_malloc(size)
#define MC_CALLOC(size,num) debug_calloc(size,num)
#define MC_FREE(data) debug_free(data)

inline void *debug_malloc(size_t size);
inline void *debug_calloc(size_t size, size_t num);
inline void  debug_free(void *data);

#else

#define MC_MALLOC(size) malloc(size)            //!< macro for debug malloc
#define MC_CALLOC(size,num) calloc(size,num)    //!< macro for debug calloc
#define MC_FREE(data) free(data)                //!< macro for debugging free

#endif

///////////////////////////////////////////////////////////////////////////////
// value to string conversion helpers 

static const char* _strGpsPositionMode[] = {
	"GPS_POSITION_MODE_STANDALONE",
	"GPS_POSITION_MODE_MS_BASED",
	"GPS_POSITION_MODE_MS_ASSISTED"
};

static const char*_strGpsPositionRecurrence[] = {
	"GPS_POSITION_RECURRENCE_PERIODIC",
	"GPS_POSITION_RECURRENCE_SINGLE"
};

static const char* _strGpsStatusValue[] = {
	"GPS_STATUS_NONE",
	"GPS_STATUS_SESSION_BEGIN",
	"GPS_STATUS_SESSION_END",
	"GPS_STATUS_ENGINE_ON",
	"GPS_STATUS_ENGINE_OFF",
};

static const char* _xstrGpsLocationFlags[] = {
    "GPS_LOCATION_HAS_LAT_LONG",
    "GPS_LOCATION_HAS_ALTITUDE",
    "GPS_LOCATION_HAS_SPEED",
    "GPS_LOCATION_HAS_BEARING",
    "GPS_LOCATION_HAS_ACCURACY",
};

static const char* _xstrGpsCapabilityFlags[] = {
    "GPS_CAPABILITY_SCHEDULING",
    "GPS_CAPABILITY_MSB",
    "GPS_CAPABILITY_MSA",
    "GPS_CAPABILITY_SINGLE_SHOT",
    "GPS_CAPABILITY_ON_DEMAND_TIME",
};

static const char* _xstrGpsAidingData[] = {
    "GPS_DELETE_EPHEMERIS",
    "GPS_DELETE_ALMANAC",
    "GPS_DELETE_POSITION",
    "GPS_DELETE_TIME",
    "GPS_DELETE_IONO",
    "GPS_DELETE_UTC",
    "GPS_DELETE_HEALTH",
    "GPS_DELETE_SVDIR",
    "GPS_DELETE_SVSTEER",
    "GPS_DELETE_SADATA",
    "GPS_DELETE_RTI",
	NULL,
	NULL,
	NULL,
	NULL,
    "GPS_DELETE_CELLDB_INFO",
    // "GPS_DELETE_ALL", 0xFFFF
};

static const char* _strAGpsType[] = {
	NULL,
	"AGPS_TYPE_SUPL",
	"AGPS_TYPE_C2K",
};

static const char* _strAGpsSetIDType[] = {
	"AGPS_SETID_TYPE_NONE",
	"AGPS_SETID_TYPE_IMSI",
    "AGPS_SETID_TYPE_MSISDN",
};

static const char* _strGpsNiType[] = {
	NULL,
    "GPS_NI_TYPE_VOICE",
    "GPS_NI_TYPE_UMTS_SUPL",
    "GPS_NI_TYPE_UMTS_CTRL_PLANE",
};

static const char* _xstrGpsNiNotifyFlags[] = {
    "GPS_NI_NEED_NOTIFY",
    "GPS_NI_NEED_VERIFY",
    "GPS_NI_PRIVACY_OVERRIDE",
};

static const char* _strGpsUserResponseType[] = {
	NULL,
    "GPS_NI_RESPONSE_ACCEPT",
    "GPS_NI_RESPONSE_DENY",
    "GPS_NI_RESPONSE_NORESP",
};

static const char* _strGpsNiEncodingType[] = {
    // "GPS_ENC_UNKNOWN", = -1
    "GPS_ENC_NONE", 
    "GPS_ENC_SUPL_GSM_DEFAULT",
    "GPS_ENC_SUPL_UTF8",
    "GPS_ENC_SUPL_UCS2",
};

static const char* _strAGpsStatusValue[] = {
	NULL,
	"GPS_REQUEST_AGPS_DATA_CONN",
	"GPS_RELEASE_AGPS_DATA_CONN",
	"GPS_AGPS_DATA_CONNECTED",
	"GPS_AGPS_DATA_CONN_DONE",
	"GPS_AGPS_DATA_CONN_FAILED",
};

static const char* _strAGpsRefLocation[] = {
	NULL,
	"AGPS_REF_LOCATION_TYPE_GSM_CELLID",
	"AGPS_REF_LOCATION_TYPE_UMTS_CELLID",
	"AGPS_REF_LOCATION_TYPE_TYPE_MAC",
};

static const char* _strAgpsRilNetworkType[] = { // Network types for update_network_state "type" parameter
    "AGPS_RIL_NETWORK_TYPE_MOBILE",
    "AGPS_RIL_NETWORK_TYPE_WIFI",
    "AGPS_RIL_NETWORK_TYPE_MOBILE_MMS",
    "AGPS_RIL_NETWORK_TYPE_MOBILE_SUPL",
    "AGPS_RIL_NETWORK_TTYPE_MOBILE_DUN",
    "AGPS_RIL_NETWORK_TTYPE_MOBILE_HIPRI",
    "AGPS_RIL_NETWORK_TTYPE_WIMAX",
};

static const char* _xstrAgpsRilRequestSetId[] = {
	"AGPS_RIL_REQUEST_SETID_IMSI",
	"AGPS_RIL_REQUEST_SETID_MSISDN",
};

static const char* _xstrAgpsRilRequestRefLoc[] = {
	"AGPS_RIL_REQUEST_REFLOC_CELLID",
	"AGPS_RIL_REQUEST_REFLOC_MAC",
};

#define _LOOKUPSTR(v, t) _strLookup(v, _str##t, sizeof(_str##t)/sizeof(_str##t[0]) )
#define _LOOKUPSTRX(v, t) _strLookupX(v, _xstr##t, sizeof(_xstr##t)/sizeof(_xstr##t[0]) )

const char* _strLookup(unsigned int v, const char** l, unsigned int n);
const char* _strLookupX(unsigned int v, const char** l, unsigned int n);

#ifdef SUPL_ENABLED 
void logSupl(struct ULP_PDU * pMsg, bool incoming);
void logRRLP(PDU_t *pRrlpMsg, bool incoming);
#endif

///////////////////////////////////////////////////////////////////////////////
#ifdef SUPL_ENABLED
class CLog
{
public:
	CLog(const char* name, int max, bool verbose = false);
	~CLog();
	
	void write(unsigned int code, const char* fmt, ...);
	void txt(int code, const char* txt);
	void writeFile(const char* pBuf, int len);

protected:
	void open(const char* name, int max);
	static const char* timestamp(char* buf = NULL);
	char m_name[256];
	int m_max;
	bool m_verbose;
};

extern CLog	logGps;
extern CLog logAgps;
#endif

#endif /* __UBX_LOG_H__ */
