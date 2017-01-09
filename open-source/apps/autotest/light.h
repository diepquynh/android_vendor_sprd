// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#ifndef _LIGHT_20121110_H__
#define _LIGHT_20121110_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_light {
//-----------------------------------------------------------------------------
    
int lightOpen( void );

int lightSetLCD( int brightness );
//int lightSetKeyBoard( int brightness );
//int lightSetButtons( int brightness );

int lightSetKeypad( int brightness );

int lightSetRgb(int index , int brightness);

int lightClose( void );

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _LIGHT_20121110_H__
