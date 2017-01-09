// 
// Spreadtrum Auto Tester
//
// anli   2012-11-09
//
#ifndef _DEBUG_20121109_H__
#define _DEBUG_20121109_H__

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define  LOG_TAG "AutoTst"
#include <cutils/log.h>

#include <assert.h>
#include <errno.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef DBG_FILE_NAME
#define DBG_FILE_NAME "autotst.log"
#endif // DBG_FILE_NAME

//-----------------------------------------------------------------------------
void dbgMsg2FileEnable( int enb );
int  dbgMsg2FileOpen( const char * filename );
int  dbgMsg2File( const char * fmt, ... );
void dbgMsg2FileClose( void );

void dbgLogcat2File( const char * filename );
//-----------------------------------------------------------------------------

//#define DBGMSG_2_CONSOLE
#define DBGMSG_2_FILE

//-----------------------------------------------------------------------------
// debug message to console
//-----------------------------------------------------------------------------
#ifdef DBGMSG_2_CONSOLE
#define PRTF  printf
#else
#define PRTF(fmt, arg...) do{}while(0)
#endif // DBGMSG_2_CONSOLE

#define LOGI ALOGI
#define LOGD ALOGD
#define LOGW ALOGW
#define LOGE ALOGE

//-----------------------------------------------------------------------------
// debug message to file
//-----------------------------------------------------------------------------
#ifdef DBGMSG_2_FILE
#define DM2F dbgMsg2File
#else
#define DM2F(fmt, arg...) do{}while(0)
#endif // DBGMSG_2_FILE

//-----------------------------------------------------------------------------

#define AT_DBG(fmt, arg...)  { \
    LOGD(  "%s(): " fmt, __FUNCTION__, ##arg); \
    PRTF(  "%s(): " fmt, __FUNCTION__, ##arg); \
    DM2F("D %s(): " fmt, __FUNCTION__, ##arg); }

#define AT_WRN(fmt, arg...)  { \
    LOGE(  "%s(): " fmt, __FUNCTION__, ##arg); \
    PRTF(  "%s(): " fmt, __FUNCTION__, ##arg); \
    DM2F("W %s(): " fmt, __FUNCTION__, ##arg); }

#define AT_ERR(fmt, arg...)  { \
    LOGE(  "%s(): " fmt, __FUNCTION__, ##arg); \
    PRTF(  "%s(): " fmt, __FUNCTION__, ##arg); \
    DM2F("E %s(): " fmt, __FUNCTION__, ##arg); }

#define AT_INF(fmt, arg...)  { \
    LOGI(  fmt, ##arg);  \
    PRTF(  fmt, ##arg);  \
    DM2F(  fmt, ##arg); }
    
#define AT_STEP  \
	LOGD("%s() %d\n", __FUNCTION__, __LINE__);

//-----------------------------------------------------------------------------
#define AT_ASSERT(x)         \
    { int cnd = (x); if(!cnd) {LOGE("!!!!!!!! %s(), line: %d Asserted !!!!!!!!\n", __FUNCTION__, __LINE__); assert(cnd);} }

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#ifdef  DBG_ENABLE_DBGMSG
#define DBGMSG(fmt, arg...)	  AT_DBG(fmt, ##arg)
#else
#define DBGMSG(fmt, arg...)   do{}while(0)
#endif // DBG_ENABLE_DBGMSG
//------------------------------------------------------------------------------
#ifdef  DBG_ENABLE_WRNMSG
#define WRNMSG(fmt, arg...)	  AT_WRN(fmt, ##arg)
#else
#define WRNMSG(fmt, arg...)   do{}while(0)
#endif // DBG_ENABLE_WRNMSG
//------------------------------------------------------------------------------
#ifdef  DBG_ENABLE_ERRMSG
#define ERRMSG(fmt, arg...)	  AT_ERR(fmt, ##arg)
#else
#define ERRMSG(fmt, arg...)   do{}while(0)
#endif // DBG_ENABLE_ERRMSG
//------------------------------------------------------------------------------
#ifdef  DBG_ENABLE_INFMSG
#define INFMSG(fmt, arg...)	  AT_INF(fmt, ##arg)
#else
#define INFMSG(fmt, arg...)   do{}while(0)
#endif // DBG_ENABLE_INFMSG
//------------------------------------------------------------------------------
#ifdef  DBG_ENABLE_FUNINF
#define FUN_ENTER             AT_INF("[ %s ++ ]\n", __FUNCTION__)
#define FUN_EXIT              AT_INF("[ %s -- ]\n", __FUNCTION__)
#else
#define FUN_ENTER             do{}while(0)
#define FUN_EXIT              do{}while(0)
#endif // DBG_ENABLE_FUNINF
//------------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _DEBUG_20121109_H__
