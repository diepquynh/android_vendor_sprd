/******************************************************************************
 ** File Name:      vsp_vlc.h	                                              *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/11/2009                                                *
 ** Copyright:      2009 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP VLC Module Driver									  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 06/11/2009    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _VSP_VLC_H_
#define _VSP_VLC_H_
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
#define JPG_VLC_REG_BASE	(JPG_CTRL_BASE + 0x4c00)
#define JPG_VLC_REG_SIZE	0xC

#define VLC_TOTAL_MCU_OFFSET			0x00
#define VLC_CTRL_OFFSET		0x04

typedef struct  jpg_vlc_reg_tag
{
	
	volatile uint32 TOTAL_MCU;		//[19:0]: TOTAL_MCU_CNT, Total MCU number of VLC MCU.
		
	volatile uint32 CTRL;		//[31]: VLC_STS: VLC status, 0: idle; 1: busy
									//[0]: VLC_CLR, Write 1 to clear VLC module
}JPG_VLC_REG_T;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif //_VSP_VLC_H_