// 
// Spreadtrum Auto Tester
//
// anli   2012-11-09
//
#ifndef _TESTER_20121109_H__
#define _TESTER_20121109_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_tester {
//-----------------------------------------------------------------------------

int test_Init( void );

int test_DoTest( const uchar * req, int req_len, uchar * rsp, int rsp_size );

int test_Deinit( void );

int test_GetMonoPcm( const uchar ** pcm_data, int * pcm_bytes );
int test_GetLeftPcm( const uchar ** pcm_data, int * pcm_bytes );
int test_GetRightPcm( const uchar ** pcm_data, int * pcm_bytes );

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _TESTER_20121109_H__
