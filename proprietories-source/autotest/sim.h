// 
// Spreadtrum Auto Tester
//
// anli   2012-11-27
//
#ifndef _SIM_20121127_H__
#define _SIM_20121127_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_sim {
//-----------------------------------------------------------------------------

int simOpen( void );

int simCheck( int index );

int simClose( void );
int sim_newCheck( int slot);
int check_sim_num(void);
//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _SIM_20121127_H__
