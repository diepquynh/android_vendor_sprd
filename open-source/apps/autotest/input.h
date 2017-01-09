// 
// Spreadtrum Auto Tester
//
// anli   2012-11-09
//
#ifndef _INPUT_20121109_H__
#define _INPUT_20121109_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_input {
//-----------------------------------------------------------------------------
    
struct kpd_info_t {
	ushort key;
	ushort row;
	ushort col;
	ushort gio;
};

int inputOpen( void );
int inputOpen2( void );

int inputKPDGetKeyInfo( struct kpd_info_t * info );

int inputKPDWaitKeyPress( struct kpd_info_t * info, int timeout );
int inputKPDWaitKeyPress2( struct kpd_info_t * info, int timeout );

int inputTPGetPoint( int *x, int *y, int timeout );

int inputClose( void );
int inputClose2( void );

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _INPUT_20121109_H__
