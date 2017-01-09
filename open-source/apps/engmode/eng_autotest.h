//
// for Spreadtrum Auto Test
//
// anli   2013-02-20
//
#ifndef _ENGAUTOTEST_20130220_H__
#define _ENGAUTOTEST_20130220_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef uchar
typedef unsigned char uchar;
#endif // uchar

int eng_autotestStart( void );

int eng_autotestIsConnect( void );

int eng_autotestDoTest(const uchar * req, int req_len, uchar * rsp, int rsp_size);

//int eng_autotestStop( void );

//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _ENGAUTOTEST_20130220_H__
