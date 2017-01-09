/******************************************************************************
 ** File Name:    JpegDec_malloc.c                                             *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         01/23/2007                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
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

//#if defined(JPEG_DEC)
//////////////////////////////////////////////////////////////////////////

//for jpeg decoder memory. 4Mbyte,extra
LOCAL uint32 s_used_extra_mem = 0x0;
LOCAL uint32 s_extra_mem_size = 0x1000000;  //16M

LOCAL uint8 *s_extra_mem_bfr_ptr = NULL;

/*****************************************************************************
 **	Name : 			JpegDec_ExtraMemAlloc
 ** Description:	Alloc the common memory for mp4 decoder.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *JpegDec_ExtraMemAlloc(uint32 mem_size)
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
 **	Name : 			JpegDec_FreeNBytes
 ** Description:	free n bytes memory which has malloced.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void JpegDec_FreeNBytes(uint32 mem_size)
{
	s_used_extra_mem -= mem_size;

	if(mem_size > s_used_extra_mem)
	{
		SCI_ASSERT(0);
	}
}

/*****************************************************************************
 **	Name : 			JpegDec_MemFree
 ** Description:	Free the common memory for mp4 decoder.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void JpegDec_FreeMem(void)
{
	s_used_extra_mem = 0;
}

PUBLIC void JpegDec_InitMem(JPEG_MEMORY_T *dec_buffer_ptr)
{
	unsigned long addr = 0;
	uint32 diff = 0;
	uint32 actual_size = 0;

	SCI_ASSERT(dec_buffer_ptr != PNULL);

	if(PNULL == dec_buffer_ptr->buf_ptr || dec_buffer_ptr->buf_size < 4)
	{
		return;
	}

	addr = (unsigned long)dec_buffer_ptr->buf_ptr;
	addr = ((addr + 3) >> 2) << 2;			//make the address 4-byte aligned
	diff = addr - (unsigned long)dec_buffer_ptr->buf_ptr;
	actual_size = dec_buffer_ptr->buf_size - diff;

	s_extra_mem_bfr_ptr = (uint8 *)addr;
	s_extra_mem_size = actual_size;
	SCI_MEMSET(s_extra_mem_bfr_ptr, 0, s_extra_mem_size);

	//reset memory used count
	s_used_extra_mem = 0;
}

//////////////////////////////////////////////////////////////////////////
//#endif //JPEG_DEC
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
