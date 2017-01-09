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
  \brief  time utility library

  Module for time functions
*/
/*******************************************************************************
 * $Id: ubx_log.cpp 62581 2012-10-17 16:46:03Z jon.bowern $
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <grp.h>
#include <sys/time.h>
#include "private/android_filesystem_config.h"
#include <cutils/open_memstream.h>
#include <fcntl.h>

#include "std_types.h"
#include "ubx_log.h"
#include "ubxgpsstate.h"

///////////////////////////////////////////////////////////////////////////////
// Static data
static CLog s_logMessages("SUPL-MESSAGE.LOG", 128*1024, false);

///////////////////////////////////////////////////////////////////////////////
//! Lookup a string from a string table
/*! 
	\param v : Index into string table 
	\param l : Pointer to table of strings
	\param n : Max number of strings in table
	\return  : Pointer to string
*/
const char* _strLookup(unsigned int v, const char** l, unsigned int n)
{
	if ((v >= n) || (l[v] == NULL)) 
		return "UNKNOWN";
	else 
		return l[v];
}

///////////////////////////////////////////////////////////////////////////////
//! Lookup a set of strings
/*! Lookup a set of strings from a string table according to flags,
    and assemble into a buffer, a pointer to which is returned to the caller.
	\param v : Flags  into string table 
	\param l : Pointer to table of strings
	\param n : Max number of strings in table
	\return  : pointer to buffer of assembled strings
*/
const char* _strLookupX(unsigned int v, const char** l, unsigned int n)
{
	static char s[LOG_SPECIAL_BUFFER_COUNT][1024];
	static int  si = -1;
	
	si++;
	if (si == LOG_SPECIAL_BUFFER_COUNT)
	{
		si = 0;
	}

	char* p = s[si];
	p[0] = '\0';
	unsigned int i;
	for (i = 0; i < n; i ++)
	{
		if (v & (1<<i))
		{
			const char* f = (l[i]!=NULL) ? l[i] : "UNKNOWN%02d";
			p += sprintf(p, f, i);
			*p ++ = '+';
		}
	}
	if (p > s[si])
	{
		p[-1] = '\0';
	}
	return s[si];
}

#ifdef SUPL_ENABLED
///////////////////////////////////////////////////////////////////////////////
//! Transfer a large memory buffer to  the log
/*! Message buffer may be too big to log in one hit, so parse the message
    using 0x0A characters to split into multiple lines. Then log each line.
	\param pMsg : Pointer to message buffer to log
	\param size : Size of buffer
*/
static void renderMemStreamToLog(char* pBuf, size_t size)
{
	unsigned int i = 0;
	unsigned int start = i;
	while(i < size)
	{
		if (pBuf[i] == 0x0A)
		{
			pBuf[i] = 0;
			if (CUbxGpsState::getInstance()->getSuplMsgToFile())
			{
				s_logMessages.writeFile(&pBuf[start], i - start);
				s_logMessages.writeFile("\r\n", 2);
			}

			if(CUbxGpsState::getInstance()->getLogSuplMessages())
			{
				LOGV("%s", &pBuf[start]);
			}

			start = i + 1;
		}
		i++;
	}
}

///////////////////////////////////////////////////////////////////////////////
//! Log a separator
/*! 
*/
static void logSep(void)
{
	char buffer[200];
	int len = sprintf(buffer, "================================================================================\n");
	renderMemStreamToLog(buffer, len);
}

///////////////////////////////////////////////////////////////////////////////
//! Decode a Supl message to text and send to log
/*! 
	\param pMsg     : Pointer to Supl message to decode to log
	\param incoming : Direction of message
*/
void logSupl(struct ULP_PDU * pMsg, bool incoming)
{
	if (CUbxGpsState::getInstance()->getLogSuplMessages() ||
	    CUbxGpsState::getInstance()->getSuplMsgToFile())
	{
		logSep();
		
		char buffer[200];
		int len = sprintf(buffer, "%s\n", incoming ? "<------- From Server" : "-------> To Server");
		renderMemStreamToLog(buffer, len);
		
		char* pBuf = NULL;
		size_t size = 0;
		FILE* pFile = open_memstream(&pBuf, &size);	// Supl print message needs a file stream
		asn_fprint(pFile, &asn_DEF_ULP_PDU, pMsg);	
		fclose(pFile);
		renderMemStreamToLog(pBuf, size);			// Now transfer from memory to Android log
		free(pBuf);
		logSep();
	}
}

///////////////////////////////////////////////////////////////////////////////
//! Decode a RRLP message to text and send to log
/*! 
	\param pMsg : Pointer to RRLP message to decode to log
*/
void logRRLP(PDU_t *pRrlpMsg, bool incoming)
{
	if (CUbxGpsState::getInstance()->getLogSuplMessages() ||
	    CUbxGpsState::getInstance()->getSuplMsgToFile())
	{
		logSep();
		
		char buffer[200];
		int len = sprintf(buffer, "RRLP Payload\n");
		renderMemStreamToLog(buffer, len);
		
		len = sprintf(buffer, "%s\n", incoming ? "<------- From Server" : "-------> To Server");
		renderMemStreamToLog(buffer, len);
		
		char* pBuf = NULL;
		size_t size = 0;
		FILE* pFile = open_memstream(&pBuf, &size);	// RRLP print message needs a file stream
		asn_fprint(pFile, &asn_DEF_PDU, pRrlpMsg);
		fclose(pFile);
		renderMemStreamToLog(pBuf, size);			// Now transfer from memory to Android log
		free(pBuf);
		logSep();
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
// CMCC compatible logging of files.

///////////////////////////////////////////////////////////////////////////////
//! Constructor
/*! 
	\param name : Pointer to log path and filename
	\param max  : Maximun size (in bytes) of log
*/
CLog::CLog(const char* name, int max, bool verbose)
{
	static const char* env = NULL;
	if (env == NULL) 
	{
		env = getenv("EXTERNAL_STORAGE");
		if (env == NULL) 
			env = "/mnt/sdcard";
		gid_t gids[] = { AID_SDCARD_RW };
		if (setgroups(sizeof(gids)/sizeof(gids[0]), gids) != 0)
		{
			LOGE("CLog::%s cannot get write accress to sdcard %d", __FUNCTION__, errno); 
			// gps log
			env = "/data"; // todo remove this line
		}
	}
	/*
    if (setgid(AID_SHELL) != 0) 
	{
        LOGE("CLog::%s cannot set gid %d %d %d", __FUNCTION__, getgid(), getegid(), errno); 
    }
	if (setuid(AID_SHELL) == -1)
	{
		LOGE("CLog::%s cannot set uid %d %d %d", __FUNCTION__, getuid(), geteuid(), errno); 
	}
  	int s = getgroups(0,NULL);
	gid_t* g = new gid_t[s];
	getgroups(s, g);
	LOGD("CLog::%s gids:", __FUNCTION__); 
	while (s --)
		LOGD("CLog::%s gid %d", __FUNCTION__, *g++); 
	*/	
	sprintf(m_name, "%s/%s", env, name); 
	m_max = max;
	m_verbose = verbose;
}

///////////////////////////////////////////////////////////////////////////////
//! Format and write to log file
/*! 
	\param code : Logging code
	\param fmt  : Pointrer to string containing formatting (as per a printf)
	\param ...  : Variable parameters to log (as per a printf)
*/
void CLog::write(unsigned int code, const char* fmt, ...)
{
	if (!CUbxGpsState::getInstance()->getCmccLogActive())
	{
		return;
	}

	char buf[256];
	char tmp[32];
	int i = sprintf(buf, "[%s] 0x%08X:", timestamp(tmp), code);
	va_list args;
	va_start(args, fmt);
	i += vsprintf(&buf[i], fmt, args);
	va_end (args);
	if (m_verbose)
		LOGD("CLog::%s file='%s' data='%s' size=%d", __FUNCTION__, m_name, buf, i);
	buf[i++] = '\r';
	buf[i++] = '\n';
	buf[i] = '\0';
	
	writeFile(buf, i);
}

///////////////////////////////////////////////////////////////////////////////
//! Write to log file
/*! 
	\param pBuf : Pointer to buffer to write
	\param len  : Number of bytes to write
*/
void CLog::writeFile(const char* pBuf, int len)
{
	FILE* pFile = fopen(m_name, "a");
	if (pFile == NULL)
	{
		// failed
		LOGE("CLog::%s : Can not open '%s' file : %i", __FUNCTION__, m_name, errno);
		return;
	}

	int size = ftell(pFile);
	if (size > m_max)
	{
		// Max file size reached, so recreate file with zero size
		pFile = freopen(m_name, "w", pFile);
		LOGW("CLog::%s : file='%s' resize", __FUNCTION__, m_name);
	}

	fwrite(pBuf, 1, len, pFile);
	fclose(pFile);
}

///////////////////////////////////////////////////////////////////////////////
//! Generate a time stamp for logging
/*! 
	\param tmp : Pointer to string place time stamp text into
	\return	   : Pointer to string containing time stamp
*/
const char* CLog::timestamp(char* tmp)
{
	static char buf[32];
	struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm st;
    gmtime_r(&tv.tv_sec, &st);
	sprintf(tmp?tmp:buf, "%04d%02d%02d%02d%02d%02d.%02d", 
				st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, 
				st.tm_hour, st.tm_min, st.tm_sec, (int)(tv.tv_usec/10000));
	return tmp?tmp:buf;
}

///////////////////////////////////////////////////////////////////////////////
//! Destructor
/*! 
*/
CLog::~CLog()
{

}

///////////////////////////////////////////////////////////////////////////////
//! Log text.
/*! Log text, but add a time stamp to the body of the text
	\param code : Logging code
	\param txt  : Pointer to text to log 
*/
void CLog::txt(int code, const char* txt)
{
	char tmp[32];
	write(code, "%s %s", timestamp(tmp), txt);
}

///////////////////////////////////////////////////////////////////////////////
// Instances for each log file
CLog logGps("GPS.LOG", 64*1024);
CLog logAgps("A-GPS.LOG", 32*1024, true);
