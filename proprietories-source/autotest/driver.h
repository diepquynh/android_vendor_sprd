// 
// Spreadtrum Auto Tester
//
// anli   2012-11-24
// add key info 2013-01-22 anli.wei
//
#ifndef _DRIVER_20121124_H__
#define _DRIVER_20121124_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_drv {
//-----------------------------------------------------------------------------
    
#define GPIO_DIR_IN  0
#define GPIO_DIR_OUT 1

int drvOpen( void );

int drvI2CRead( uchar bus, uchar addr, uchar reg, uchar *value );

int drvGIODir( ushort gpio, ushort pull, uchar dir );
int drvGIOGet( ushort gpio, uchar *val );
int drvGIOSet( ushort gpio, uchar val );
int drvGIOClose( ushort gpio );

int drvLcdSendData( uint data );
int drvLcd_mipi_on( void );
int drvLcd_mipi_off( void );

int drvGetKpdInfo( ushort val, ushort * row, ushort * col, ushort * gio );

int drvClose( void );

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _DRIVER_20121124_H__
