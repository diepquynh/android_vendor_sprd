/******************************************************************************
 ** File Name:      vpp_drv_interface.h                                         *
 ** Author:           Duan Xue                                               *
 ** DATE:             11/24/2014                                                *
 ** Copyright:       2014 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    														  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE              NAME            DESCRIPTION                                 *
 ** 11/24/2014    Duan Xue       Create.                                     *
 *****************************************************************************/
#ifndef _VPP_DRV_INTERFACE_H_
#define _VPP_DRV_INTERFACE_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vpp_deint.h"

#include<stdlib.h>
#include<stdio.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <sys/mman.h>

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/*----------------------------------------------------------------------------*
**                            Macro Definitions                               *
**---------------------------------------------------------------------------*/
/*
	Bit define, for video
*/
#define V_BIT_0               0x00000001
#define V_BIT_1               0x00000002
#define V_BIT_2               0x00000004
#define V_BIT_3               0x00000008
#define V_BIT_4               0x00000010
#define V_BIT_5               0x00000020
#define V_BIT_6               0x00000040
#define V_BIT_7               0x00000080
#define V_BIT_8               0x00000100
#define V_BIT_9               0x00000200
#define V_BIT_10              0x00000400
#define V_BIT_11              0x00000800
#define V_BIT_12              0x00001000
#define V_BIT_13              0x00002000
#define V_BIT_14              0x00004000
#define V_BIT_15              0x00008000
#define V_BIT_16              0x00010000
#define V_BIT_17              0x00020000
#define V_BIT_18              0x00040000
#define V_BIT_19              0x00080000
#define V_BIT_20              0x00100000
#define V_BIT_21              0x00200000
#define V_BIT_22              0x00400000
#define V_BIT_23              0x00800000
#define V_BIT_24              0x01000000
#define V_BIT_25              0x02000000
#define V_BIT_26              0x04000000
#define V_BIT_27              0x08000000
#define V_BIT_28              0x10000000
#define V_BIT_29              0x20000000
#define V_BIT_30              0x40000000
#define V_BIT_31              0x80000000

/* ------------------------------------------------------------------------
** Constants
** ------------------------------------------------------------------------ */
#define SPRD_VPP_DRIVER "/dev/sprd_vpp"
#define SPRD_VPP_MAP_SIZE 0x4000            // 4kbyte

#define SPRD_VPP_IOCTL_MAGIC 'd'
#define SPRD_VPP_ALLOCATE_PHYSICAL_MEMORY _IO(SPRD_VPP_IOCTL_MAGIC, 1)
#define SPRD_VPP_FREE_PHYSICAL_MEMORY         _IO(SPRD_VPP_IOCTL_MAGIC, 2)
#define SPRD_VPP_DEINT_COMPLETE                     _IO(SPRD_VPP_IOCTL_MAGIC, 3)
#define SPRD_VPP_DEINT_ACQUIRE                     _IO(SPRD_VPP_IOCTL_MAGIC, 4)
#define SPRD_VPP_DEINT_RELEASE                     _IO(SPRD_VPP_IOCTL_MAGIC, 5)
#define VPP_RESET                     _IO(SPRD_VPP_IOCTL_MAGIC, 6)


#define TIME_OUT_CLK			0xffff
#define TIME_OUT_CLK_FRAME	0x7fffff


/* -----------------------------------------------------------------------
** Control Register Address on ARM
** ----------------------------------------------------------------------- */




/* -----------------------------------------------------------------------
** Structs
** ----------------------------------------------------------------------- */

typedef struct tagVPPObject
{
    unsigned long s_vpp_Vaddr_base;
    int32 s_vpp_fd;
    uint32 vpp_freq_div;
    int32	error_flag;
} VPPObject;

typedef struct vpp_drv_buffer_t {
	unsigned int size;
	unsigned long phys_addr;
	unsigned long base;	     /*kernel logical address in use kernel*/
	unsigned long virt_addr; /* virtual user space address */
} VPP_DRV_BUFFER_T;

//error id, added by xiaowei, 20130910
#define ER_SREAM_ID   		(1<<0)
#define ER_MEMORY_ID  		(1<<1)
#define ER_REF_FRM_ID     	(1<<2)
#define ER_HW_ID                   (1<<3)
#define ER_FORMAT_ID           (1<<4)

/* -----------------------------------------------------------------------
** Global
** ----------------------------------------------------------------------- */

int32 vpp_init (VPPObject *);
int32 vpp_open_dev(VPPObject *);
int32 vpp_close_dev(VPPObject *);
int32 vpp_read_reg_poll(VPPObject *, uint32 , uint32 ,uint32 , uint32 , char *);
void vpp_free_dma_memory(VPPObject *vo, VPP_DRV_BUFFER_T*vb);
int32 vpp_allocate_dma_memory(VPPObject *vo, VPP_DRV_BUFFER_T *vb);
int32 vpp_poll_complete(VPPObject *vo);

#define VPP_WRITE_REG(reg_addr, value, pstring) (*(volatile uint32 *)(reg_addr + vo->s_vpp_Vaddr_base)  = (value))
#define VPP_READ_REG(reg_addr, pstring)	(*(volatile uint32 *)(reg_addr + vo->s_vpp_Vaddr_base))
#define VPP_READ_REG_POLL(reg_addr, msk_data, exp_value, time, pstring) \
    vpp_read_reg_poll((VPPObject *)vo, reg_addr, msk_data, exp_value, time, pstring)
    

/**---------------------------------------------------------------------------
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_VSP_DRV_SC8830_H_
