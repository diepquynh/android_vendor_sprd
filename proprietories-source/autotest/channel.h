// 
// Spreadtrum Auto Tester
//
// anli   2012-11-21
//
#ifndef _CHANNEL_20121121_H__
#define _CHANNEL_20121121_H__

//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" { 
#endif // __cplusplus
//--namespace sci_chl {
//------------------------------------------------------------------------------

int  chlOpen( const char * name );

int  chlRead( int fd, unsigned char * buf, int size, int timeout );

int  chlWrite( int fd, unsigned char * buf, int len );

void chlClose( int fd );    

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _CHANNEL_20121121_H__
