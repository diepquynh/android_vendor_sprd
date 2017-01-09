/******************************************************************************
** File Name:      JpegCodec_bufmgr.h                                             *
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

#ifndef _JPEGCODEC_BUFMGR_H_
#define _JPEGCODEC_BUFMGR_H_

#include "jpegcodec_def.h"

PUBLIC void		  JPEG_HWSet_BSM_Buf_ReadOnly(uint8 buf_id);
PUBLIC void		  JPEG_HWSet_BSM_Buf_WriteOnly(uint8 buf_id);
PUBLIC void		  JPEG_HWSet_MBIO_Buf_WriteOnly(uint8 buf_id);
PUBLIC void		  JPEG_HWSet_MBIO_Buf_ReadOnly(uint8 buf_id);
PUBLIC void		  JPEG_HWResetVSP(void);
PUBLIC BOOLEAN	  JPEG_HWWaitingEnd(void);


#endif//_JPEGCODEC_BUFMGR_H_