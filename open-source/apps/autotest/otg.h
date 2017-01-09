#ifndef __OTG_20150306_H__
#define __OTG_20150306_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_otg {
//-----------------------------------------------------------------------------

int otgEnable(void);

int otgDisable(void);

int otgIdStatus(void);

int otgVbusOpen(void);

int otgVbusClose(void);

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif //__OTG_20150306_H__
