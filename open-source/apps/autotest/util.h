// 
// Spreadtrum Auto Tester
//
// anli   2013-02-28
//
#ifndef _UTIL_20130228_H__
#define _UTIL_20130228_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_util {
//-----------------------------------------------------------------------------
    
int utilEnableService( const char * svcName );
int utilDisableService( const char * svcName );
int utilGetPidByName( const char * exeName );

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _UTIL_20130228_H__
