// 
// Spreadtrum Auto Tester
//
// anli   2013-03-01
//
#ifndef _GPS_20130301_H__
#define _GPS_20130301_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_gps {
//-----------------------------------------------------------------------------

int gpsOpen( void );

int gpsStart( void );

int gpsGetSVNum( void );

int gpsGet_PRN_SN_Num( int *p, int a[] );

int gpsStop( void );

int gpsClose( void );

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _GPS_20130301_H__
