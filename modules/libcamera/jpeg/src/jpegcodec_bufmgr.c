/******************************************************************************
** File Name:      JpegCodec_bufmgr.c                                             *
** Author:         yi.wang													  *
** DATE:           07/12/2007                                                 *
** Copyright:      2007 Spreadtrum, Incoporated. All Rights Reserved.         *
** Description:    Buffer management									  *
** Note:           None                                                       *
*******************************************************************************

  *******************************************************************************
  **                        Edit History                                      *
  ** -------------------------------------------------------------------------*
  ** DATE           NAME             DESCRIPTION                              *
  ** 07/12/2007     yi.wang	         Create.                                  *
******************************************************************************/
#include "jpegcodec_def.h"
//#include "jpegcodec_global.h"
//#include "vsp_drv_sc8800s4.h"
//#include "video_common.h"		//remove by shan.he
#include "sc8830_video_header.h"

PUBLIC void JPEG_HWSet_BSM_Buf_ReadOnly(uint8 buf_id)
{
	(*(volatile uint32*)(0x60b04400)) &= 0x3fffffff;
	(*(volatile uint32*)(0x60b04400)) |= (1 << (31-buf_id));
}

PUBLIC void JPEG_HWSet_BSM_Buf_WriteOnly(uint8 buf_id)
{
	//(*(volatile uint32*)(0x20c10400)) &= (0xffffffff - (1 << (31 - buf_id)));
	uint32_t  value = JPG_READ_REG(JPG_BSM_REG_BASE+BSM_CFG0_OFFSET, "");
	value &= (0xffffffff - (1 << (31 - buf_id)));
	JPG_WRITE_REG(JPG_BSM_REG_BASE+BSM_CFG0_OFFSET, value, "BSM_CFG0_OFF: set which bsm buffer is valid now.");
}

//PUBLIC void JPEG_HWSet_MBIO_Buf_ReadOnly(uint8 buf_id)
//{
//	(*(volatile uint32*)(0x20c11c10)) &= (1 << buf_id);
//}

PUBLIC void JPEG_HWSet_MBIO_Buf_WriteOnly(uint8 buf_id)
{
#if _CMODEL_ //for RTL simulation
	JPG_WRITE_REG(JPG_MBIO_REG_BASE+BUF_STS_OFFSET, 1<<buf_id, "MBIO_BUF_ST: set which mbio buffer is valid now.");
#else
	(*(volatile uint32*)(0x60b04010)) |= (1 << buf_id);
#endif
}

PUBLIC void JPEG_HWSet_MBIO_Buf_ReadOnly(uint8 buf_id)
{
	JPG_WRITE_REG(JPG_MBIO_REG_BASE+BUF_STS_OFFSET, 1<<buf_id, "MEA_VDB_BUF_ST_OFF: set which mea buffer is valid now.");
}

PUBLIC void		  JPEG_HWResetVSP(void)
{
	JPG_Reset();
}

PUBLIC BOOLEAN JPEG_HWWaitingEnd(void)
{
	if(JPG_READ_REG_POLL(JPG_GLB_REG_BASE+MST_STS_OFFSET, V_BIT_0, 0, TIME_OUT_CLK, "polling AHB idle status"))
	{
		JPEG_TRACE("TIME OUT!\n");
		return FALSE;
	}

	return TRUE;
}

