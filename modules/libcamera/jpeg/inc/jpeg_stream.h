/******************************************************************************
 ** File Name:       jpeg_stream.h                                            *
 ** Author:          Shan.He                                       			  *
 ** DATE:            11/04/2009                                               *
 ** Copyright:       2009 Spreatrum, Incoporated. All Rights Reserved.        *
 ** Description:     Stream operations                                        *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ---------------------------------------------------------------------------*
 ** DATE              NAME             DESCRIPTION                            *
 ******************************************************************************/
 #ifndef _JPEG_STREAM_H_
#define _JPEG_STREAM_H_

#include "sci_types.h"
//#include "jpeg_interface.h"
#include "jpeg_exif_header.h"
//#include "jpeg.h"
//#include "jpeg_head_info.h"

#include "jpeg_fw_def.h"

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
    extern   "C"
    {
#endif

/**---------------------------------------------------------------------------*
 **                         MICROS                                             *
 **---------------------------------------------------------------------------*/\
#if 0
#define JPEG_GET_C(_context_ptr, _output_ptr, _error_action)	\
	do	\
	{	\
		if (!Jpeg_GetC(_context_ptr, _output_ptr))	\
		{	\
			_error_action;	\
		}	\
	}while(0)

#define JPEG_GET_W(_context_ptr, _output_ptr, _error_action)	\
	do	\
	{	\
		if (!Jpeg_GetWBigEndian(_context_ptr, _output_ptr))	\
		{	\
			_error_action;	\
		}	\
	}while(0)

#define JPEG_GET_DATA(_get_func_ptr, _context_ptr, _output_ptr, _error_action)   \
    do  \
    {   \
		if (!_get_func_ptr(_context_ptr, _output_ptr))	\
		{	\
			_error_action;	\
		}	\
    } while(0)
#endif

#define JPEG_WRITE_DATA(_write_func_ptr, _context_ptr, write_data, _error_action)   \
    do  \
    {   \
		if (!_write_func_ptr(_context_ptr, write_data))	\
		{	\
			_error_action;	\
		}	\
    } while(0)


/**---------------------------------------------------------------------------*
 **                         structures                                        *
 **---------------------------------------------------------------------------*/
typedef struct
{
	JINF_READ_FILE_FUNC	read_file_func;
	uint32				read_file_size;
	uint8 				*stream_buf;
	uint32				stream_buf_size;
	uint8				*stream_ptr;
	uint32				stream_left_size;
	uint32				stream_cur_pos;
}JPEG_READ_STREAM_CONTEXT_T;

typedef struct
{
    uint8                   *write_buf;
    uint32                  write_buf_size;
    uint8                   *write_ptr;
	JINF_WRITE_FILE_FUNC	write_file_func;
	uint32				    write_file_size;
}JPEG_WRITE_STREAM_CONTEXT_T;

//get data function tables
typedef struct get_data_func_tab_tag
{
    BOOLEAN (*get_c_func_ptr)(JPEG_READ_STREAM_CONTEXT_T *, uint8 *);
    BOOLEAN (*get_w_func_ptr)(JPEG_READ_STREAM_CONTEXT_T *, uint16 *);
    BOOLEAN (*get_l_func_ptr)(JPEG_READ_STREAM_CONTEXT_T *, uint32 *);
}GET_DATA_FUNC_TAB_T;

typedef BOOLEAN (*FUNC_GET_W)(JPEG_READ_STREAM_CONTEXT_T *, uint16 *);

/*****************************************************************************
**	Name : 			Jpeg_GetStreamPos
**	Description:	get the current position of the jpeg stream
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:        context pointer
**	Note:
*****************************************************************************/
PUBLIC uint32 Jpeg_GetStreamPos(JPEG_READ_STREAM_CONTEXT_T *context_ptr);

/*****************************************************************************
**	Name : 			Jpeg_SetStreamPos
**	Description:	set the current the position of the jpeg stream
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:        context pointer
**                  offset:             position to set, relative to the beginning
**                                      of jpeg stream
**	Note:
*****************************************************************************/
PUBLIC void Jpeg_SetStreamPos(JPEG_READ_STREAM_CONTEXT_T *context_ptr, uint32 offset);

/*****************************************************************************
**	Name : 			Jpegd_GetC
**	Description:	Get a byte of jpeg stream and move the stream current
**                  position one byte forward
**	Author:			Shan,he
**  Parameters:
**                  context_ptr:        context pointer
**                  output_ptr:         pointer of output data, ouput parameter
**	Note:
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_GetC(JPEG_READ_STREAM_CONTEXT_T *context_ptr, uint8 *output_ptr);

/*****************************************************************************
**	Name : 			Jpegd_GetWBigEndian
**	Description:	get an uint16 type data in big-endian mode and move the
**                  current position of jpeg stream 2 bytes forward
**	Author:			Shan.He
**  Parameters:
**                  context_ptr:        context pointer
**                  output_ptr:         pointer of output data, ouput parameter
**	Note:
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_GetWBigEndian(JPEG_READ_STREAM_CONTEXT_T *context_ptr, uint16 *output_ptr);

/*****************************************************************************
**	Name : 			Jpegd_GetWBigEndian
**	Description:	get an uint16 type data in little-endian mode and move the
**                  current position of jpeg stream 2 bytes forward
**	Author:			Shan.He
**  Parameters:
**                  context_ptr:        context pointer
**                  output_ptr:         pointer of output data, ouput parameter
**	Note:
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_GetWLittleEndian(JPEG_READ_STREAM_CONTEXT_T *context_ptr,
                                     uint16 *output_ptr);

/*****************************************************************************
**	Name : 			Jpegd_GetWBigEndian
**	Description:	get an uint32 type data in big-endian mode and move the
**                  current position of jpeg stream 4 bytes forward
**	Author:			Shan.He
**  Parameters:
**                  context_ptr:        context pointer
**                  output_ptr:         pointer of output data, ouput parameter
**	Note:
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_GetLBigEndian(JPEG_READ_STREAM_CONTEXT_T *context_ptr,
                                       uint32 *output_ptr);

/*****************************************************************************
**	Name : 			Jpegd_GetWBigEndian
**	Description:	get an uint32 type data in little-endian mode and move the
**                  current position of jpeg stream 4 bytes forward
**	Author:			Shan.He
**  Parameters:
**                  context_ptr:        context pointer
**                  output_ptr:         pointer of output data, ouput parameter
**	Note:
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_GetLLittleEndian(JPEG_READ_STREAM_CONTEXT_T *context_ptr,
                                          uint32 *output_ptr);

/*****************************************************************************
**	Name : 			Jpeg_Skip
**	Description:	Skip some bytes and reset the current position of stream
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:        context pointer
**                  len:                skip length
**	Note:
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_Skip(JPEG_READ_STREAM_CONTEXT_T *context_ptr, uint32 len);

/*****************************************************************************
**	Name :
**	Description:	set the writing position
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:     pointer of context
**                  offset:          writing position
**	Note:           return TRUE if successful else return FALSE
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_SetWritePos(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr, uint32 offset);

/*****************************************************************************
**	Name :
**	Description:	get the writing position
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:     pointer of context
**                  offset:          writing position
**	Note:           return the writing position
*****************************************************************************/
PUBLIC uint32 Jpeg_GetWritePos(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr);

/*****************************************************************************
**	Name :
**	Description:	write an uint8 type value
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:    pointer of context
**                  c:              value to write
**	Note:           return TRUE if successful else return FALSE
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_WriteC(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr, uint8 c);

/*****************************************************************************
**	Name :
**	Description:	write an uint16 type value
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:    pointer of context
**                  w:              value to write
**	Note:           return TRUE if successful else return FALSE
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_WriteW(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr, uint16 w);

/*****************************************************************************
**	Name :
**	Description:	write an uint32 type value
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:    pointer of context
**                  l:              value to write
**	Note:           return TRUE if successful else return FALSE
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_WriteL(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr, uint32 l);

/*****************************************************************************
**	Name :
**	Description:	write a block of data
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:    pointer of context
**                  block_buf_ptr:  buffer pointer in which data will be written
**                  block_size      block size in bytes
**	Note:           return TRUE if successful else return FALSE
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_WriteBlock(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr,
                              uint8 *block_buf_ptr, uint32 block_size);

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
    }
#endif

#endif