// 
// Spreadtrum Auto Tester
//
// anli   2012-12-03
//
#ifndef _FM_20121203_H__
#define _FM_20121203_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_fm {
//-----------------------------------------------------------------------------
    
int fmOpen( void );

int fmPlay( uint freq );

int fmStop( void );

int fmClose( void );

//int fmCheckStatus(uchar *fm_status );

int Radio_Open( uint freq );

int Radio_Close( void );

int Radio_Play( uint freq );

int Radio_Seek( int *freq );

int Radio_CheckStatus(uchar *fm_status, uint *fm_freq, int *fm_rssi);

int Radio_GetRssi(int *fm_rssi);

int Radio_SKD_Test(uchar *fm_status, uint *fm_freq, int *fm_rssi);

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _FM_20121203_H__
