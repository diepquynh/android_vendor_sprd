/******************************************************************************
 ** File Name:    JpegEnc_malloc.c                                             *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         03/13/2009                                                  *
 ** Copyright:    2009 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8830_video_header.h"

#if !defined(_SIMULATION_)
//#include "os_api.h"
#endif
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C"
    {
#endif

#if defined(JPEG_ENC)
//////////////////////////////////////////////////////////////////////////

//for jpeg decoder memory. 4Mbyte,extra
LOCAL uint32 s_used_extra_mem = 0x0;
LOCAL uint32 s_extra_mem_size = 0x1000000;  //16M

LOCAL uint8 *s_extra_mem_bfr_ptr = NULL;

/*****************************************************************************
 **	Name : 			JpegEnc_ExtraMemAlloc
 ** Description:	Alloc the common memory for jpeg encoder.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *JpegEnc_ExtraMemAlloc(uint32 mem_size)
{
	uint8 *pMem;
	mem_size = ((mem_size + 3) &(~3));

	if((0 == mem_size)||(mem_size> (s_extra_mem_size-s_used_extra_mem)))
	{
		SCI_ASSERT(0);
		//return 0;
	}

	pMem = s_extra_mem_bfr_ptr + s_used_extra_mem;
	s_used_extra_mem += mem_size;

	return pMem;
}

/*****************************************************************************
 **	Name : 			JpegEnc_MemFree
 ** Description:	Free the common memory for jpeg encoder.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void JpegEnc_FreeMem(void)
{
	s_used_extra_mem = 0;
}

PUBLIC void JpegEnc_InitMem(JPEG_MEMORY_T *enc_buffer_ptr)
{
	s_extra_mem_bfr_ptr = enc_buffer_ptr->buf_ptr;
	s_extra_mem_size = enc_buffer_ptr->buf_size;

	//reset memory used count
	s_used_extra_mem = 0;
}

//////////////////////////////////////////////////////////////////////////
#endif //JPEG_ENC
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
