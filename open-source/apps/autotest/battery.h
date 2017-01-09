// 
// Spreadtrum Auto Tester
//
// anli   2012-11-23
//
#ifndef _BATTERY_20121123_H__
#define _BATTERY_20121123_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_bat {
//-----------------------------------------------------------------------------

int batOpen( void );

int batEnableCharger( int enb );
    
int batIsCharging( void );

int batClose( void );

int batStatus( void );
//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _BATTERY_20121123_H__
