// 
// Spreadtrum Auto Tester
//
// anli   2012-11-13
//
#ifndef _VIBRATOR_20121113_H__
#define _VIBRATOR_20121113_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_vib {
//-----------------------------------------------------------------------------

int vibOpen( void );

int vibTurnOn( int timeout_s  ); // second
int vibTurnOff( void );

int vibClose();

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _VIBRATOR_20121113_H__
