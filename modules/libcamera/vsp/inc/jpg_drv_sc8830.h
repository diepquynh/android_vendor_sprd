/******************************************************************************
 ** File Name:      vsp_drv_sc8825.h                                         *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           01/07/2010                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    														  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 06/11/2009    Xiaowei Luo     Create.                                     *
 ** 27/09/2012    Leon Li      Modified.                                     *
 *****************************************************************************/
#ifndef _VSP_DRV_SC8830_H_
#define _VSP_DRV_SC8830_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif 
#include "sci_types.h"
#include "jpg_vld.h"
#include "jpg_global.h"
#include "jpg_bsm.h"
#include "jpg_vlc.h"
#include "jpg_mbio.h"
#include "jpg_dct.h"
#include "jpg_global_define.h"

#ifdef _VSP_LINUX_
#define LOG_TAG "JPG"
#include <utils/Log.h>
#define  SCI_TRACE_LOW   ALOGE
#define SCI_MEMSET  memset
#define SCI_MEMCPY	memcpy
#define SCI_ASSERT(...) 
#define SCI_PASSERT(condition, format...)
#endif 
#include <linux/ioctl.h>
/*----------------------------------------------------------------------------*
**                            Macro Definitions                               *
**---------------------------------------------------------------------------*/
#define SPRD_JPG_MAP_SIZE 0x6000

#define SPRD_JPG_IOCTL_MAGIC 	'm'
#define JPG_CONFIG_FREQ 		_IOW(SPRD_JPG_IOCTL_MAGIC, 1, unsigned int)
#define JPG_GET_FREQ    			_IOR(SPRD_JPG_IOCTL_MAGIC, 2, unsigned int)
#define JPG_ENABLE      			_IO(SPRD_JPG_IOCTL_MAGIC, 3)
#define JPG_DISABLE     			_IO(SPRD_JPG_IOCTL_MAGIC, 4)
#define JPG_ACQUAIRE    			_IO(SPRD_JPG_IOCTL_MAGIC, 5)
#define JPG_RELEASE     			_IO(SPRD_JPG_IOCTL_MAGIC, 6)
#define JPG_START       			_IO(SPRD_JPG_IOCTL_MAGIC, 7)
#define JPG_RESET       			_IO(SPRD_JPG_IOCTL_MAGIC, 8)
#define JPG_ACQUAIRE_MBIO_DONE _IOR(SPRD_JPG_IOCTL_MAGIC, 9,unsigned int)

#define INTS_MBIO 3
#define INTS_VLD 2
#define INTS_VLC 1
#define INTS_BSM 0

#define TIME_OUT_CLK			0xfffffff

#define IS_TIME_OUT				1
#define NOT_TIME_OUT			0

#ifdef _VSP_LINUX_
extern unsigned long g_jpg_Vaddr_base;
typedef int (*FunctionType_ResetJPG)(int fd);
PUBLIC void  JPG_reg_reset_callback(FunctionType_ResetJPG p_cb,int fd);
void  JPG_SetVirtualBaseAddr(unsigned long jpg_Vaddr_base);
#endif


PUBLIC void JPG_Reset (void);
PUBLIC void configure_huff_tab (uint32 *pHuff_tab, int32 n);
PUBLIC void flush_unalign_bytes (int32 nbytes);
PUBLIC void open_jpg_iram (void);
PUBLIC void close_jpg_iram (void);


#if !defined(_VSP_)
PUBLIC void jpg_write_register(uint32 reg_addr, int32 value/*, char *pstring*/);
PUBLIC uint32 jpg_read_register (uint32 reg_addr/*, int8 *pString*/);
PUBLIC int32 jpg_read_reg_poll(uint32 addr, uint32 msk,uint32 exp_value, uint32 time/*, char *pstring*/);
#else
#ifdef _VSP_LINUX_
static inline void jpg_write_register(uint32 reg_addr, int32 value/*, char *pstring*/)
#else
PUBLIC __inline void jpg_write_register(uint32 reg_addr, int32 value/*, char *pstring*/)	
#endif
{
#ifdef _VSP_LINUX_
       //extern uint32 g_jpg_Vaddr_base;
	*(volatile uint32 *)(reg_addr-JPG_CTRL_BASE+g_jpg_Vaddr_base)  = (value);
#else
	*(volatile uint32 *)(reg_addr)  = (value);
#endif
}

#ifdef _VSP_LINUX_
static inline uint32 jpg_read_register(uint32 reg_addr/*, char *pstring*/)	
#else
PUBLIC __inline uint32 jpg_read_register(uint32 reg_addr/*, char *pstring*/)	
#endif
{
#ifdef _VSP_LINUX_
       //extern uint32 g_vsp_Vaddr_base;
	return (*(volatile uint32 *)(reg_addr-JPG_CTRL_BASE+g_jpg_Vaddr_base));
#else
	return (*(volatile uint32 *)(reg_addr));
#endif
}

#ifdef _VSP_LINUX_
static inline int32 jpg_read_reg_poll(uint32 reg_addr, uint32 msk,uint32 exp_value, uint32 time/*, char *pstring*/) 
#else
PUBLIC __inline int32 jpg_read_reg_poll(uint32 reg_addr, uint32 msk,uint32 exp_value, uint32 time/*, char *pstring*/)
#endif
{
	uint32 vsp_time_out_cnt = 0;
#ifdef _VSP_LINUX_
       //extern uint32 g_jpg_Vaddr_base;
	while ((  (*(volatile uint32*)(reg_addr-JPG_CTRL_BASE+g_jpg_Vaddr_base)) & msk) != exp_value)
	{
		if (vsp_time_out_cnt > time)
		{
			SCI_TRACE_LOW("reg_addr 0x%x poll timeout",reg_addr);
			return 1;
		}
		vsp_time_out_cnt++;
	}
#else	
	while ((*(volatile uint32*)reg_addr & msk) != exp_value)
	{
		if (vsp_time_out_cnt > time)
		{
			SCI_TRACE_LOW("reg_addr 0x%x poll timeout",reg_addr);
			return 1;
		}
		vsp_time_out_cnt++;
	}
#endif
	return 0;
}

#define JPG_WRITE_REG(reg_addr, value, pstring) do{jpg_write_register(reg_addr, (value));\
	if((uint32)(value) !=jpg_read_register(reg_addr))SCI_TRACE_LOW("WRITE_REG %s,%d 0x%x : 0x%x",__FUNCTION__,__LINE__,(value),jpg_read_register(reg_addr));\
	else {\
		}\
		}while(0)
#define JPG_READ_REG(reg_addr, pstring)		jpg_read_register(reg_addr)
#define JPG_READ_REG_POLL(reg_addr, msk, exp_value, time, pstring) jpg_read_reg_poll(reg_addr, (msk), (exp_value), time)


/**
stop vsp
**/
PUBLIC void JPG_Stop();
#endif //!defined(_VSP_)


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif //_VSP_DRV_SC8830_H_
