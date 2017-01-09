// 
// Spreadtrum Auto Tester
//
// anli   2012-11-26
//
#ifndef _LCD_20121126_H__
#define _LCD_20121126_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_lcd {
//-----------------------------------------------------------------------------
    
int lcdOpen( void );

int lcdDrawBackground( uint rgbValue );

int lcdClose( void );

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _LCD_20121126_H__
