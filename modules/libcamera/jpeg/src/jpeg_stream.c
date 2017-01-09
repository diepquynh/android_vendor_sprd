/******************************************************************************
 ** File Name:       jpeg_stream.c                                            *
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

/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/
#include "jpeg_stream.h"
//#include "os_api.h"
//#include "jpeg.h"

#include "jpegenc_bitstream.h"

/**---------------------------------------------------------------------------*
 **                         MICROS                                             *
 **---------------------------------------------------------------------------*/
#undef JPEG_PRINT_LOW

#define JPEG_PRINT_LOW(_x_)               	ALOGE _x_

/**---------------------------------------------------------------------------*
 **                         local functions                                   *
 **---------------------------------------------------------------------------*/

#if 0
/*****************************************************************************
**	Name : 			Jpeg_ReadStream
**	Description:	read jpeg strean, be called by Jpeg_GetC when stream left in buffer
**					is 0
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:        context pointer
**	Note:
*****************************************************************************/
LOCAL void Jpeg_ReadStream(JPEG_READ_STREAM_CONTEXT_T *context_ptr)
{
	//IMG_ASSERT_CHECK
	//SCI_PASSERT(context_ptr->stream_left_size < 1, ("[Jpeg_ReadStream] stream_left_size = %d !",
	//												context_ptr->stream_left_size));
	if (context_ptr->stream_left_size >= 1)
	{
		JPEG_PRINT(("[Jpeg_ReadStream] warning ! stream_left_size = %d !", context_ptr->stream_left_size));
	}

	if (NULL != context_ptr->read_file_func && NULL != context_ptr->stream_buf) //jpeg stream in file, read them into the temp buffer
	{
		uint32 actual_read_size 	= 0;

		(context_ptr->read_file_func)(context_ptr->stream_buf, context_ptr->read_file_size,
									context_ptr->stream_buf_size, &actual_read_size);
        context_ptr->read_file_size += actual_read_size;

		if (actual_read_size > 0)
		{
			context_ptr->stream_ptr = context_ptr->stream_buf;
			context_ptr->stream_left_size = actual_read_size;
		}
		else
		{
			context_ptr->stream_ptr = NULL;
			context_ptr->stream_left_size = 0;
			JPEG_PRINT(("[Jpeg_ReadStream] stream in file, no more bytes"));
		}

		JPEG_PRINT_LOW(("[Jpeg_ReadStream] stream in file, actual read size = %d, stream_ptr = 0x%x, stream_left_size = %d, read_file_size = %d",
						actual_read_size, context_ptr->stream_ptr, context_ptr->stream_left_size, context_ptr->read_file_size));
	}
	else if (NULL != context_ptr->stream_buf)//jpeg stream in buffer
	{
		if (NULL == context_ptr->stream_ptr)
		{
			context_ptr->stream_ptr = context_ptr->stream_buf;
			context_ptr->stream_left_size = context_ptr->stream_buf_size;
		}
		else
		{
			context_ptr->stream_ptr = NULL;
			context_ptr->stream_left_size = 0;
			JPEG_PRINT(("[Jpeg_ReadStream] stream in buffer, no more bytes"));
		}

		JPEG_PRINT(("[Jpeg_ReadStream] stream in buffer, stream_ptr = 0x%x, stream_left_size = %d",
					context_ptr->stream_ptr, context_ptr->stream_left_size));
	}
	else
	{
		context_ptr->stream_ptr = NULL;
		context_ptr->stream_left_size = 0;
		JPEG_PRINT(("[Jpeg_ReadStream] no source found !"));
	}
}
#endif

/*****************************************************************************
**	Name : 			Jpeg_GetStreamPos
**	Description:	get the current position of the jpeg stream
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:        context pointer
**	Note:
*****************************************************************************/
PUBLIC uint32 Jpeg_GetStreamPos(JPEG_READ_STREAM_CONTEXT_T *context_ptr)
{
   uint32 offset = 0;

   if (NULL != context_ptr->read_file_func && NULL != context_ptr->stream_buf)
   {
        //stream in file
       uint32 offset_in_buf         = 0;
       uint32 bytes_in_buf          = 0;
       uint32 consumed_file_size    = 0;

       if ((unsigned long)context_ptr->stream_ptr >= (unsigned long)context_ptr->stream_buf)
       {
           offset_in_buf = (unsigned long)context_ptr->stream_ptr - (unsigned long)context_ptr->stream_buf;
           bytes_in_buf = offset_in_buf + context_ptr->stream_left_size;
       }

       if (context_ptr->read_file_size >= bytes_in_buf)
       {
           //consumed data which is not in buffer
           consumed_file_size = context_ptr->read_file_size - bytes_in_buf;
       }

       offset = consumed_file_size + offset_in_buf;
   }
   else
   {
       if ((unsigned long)context_ptr->stream_ptr >= (unsigned long)context_ptr->stream_buf
          && (unsigned long)context_ptr->stream_ptr <= (unsigned long)context_ptr->stream_buf + context_ptr->stream_buf_size)
       {
            offset = (unsigned long)context_ptr->stream_ptr - (unsigned long)context_ptr->stream_buf;
       }
       else
       {
       		JPEG_PRINT(("[Jpeg_GetStreamPos] error of stream ptr"));
       		context_ptr->stream_ptr = context_ptr->stream_buf + context_ptr->stream_buf_size;
			context_ptr->stream_left_size = 0;
			offset = context_ptr->stream_buf_size;
           //SCI_PASSERT(0, ("[Jpeg_GetStreamPos] error of stream ptr"));
       }
   }

   return offset;
}

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
PUBLIC void Jpeg_SetStreamPos(JPEG_READ_STREAM_CONTEXT_T *context_ptr, uint32 offset)
{
   if (NULL != context_ptr->read_file_func && NULL != context_ptr->stream_buf)
   {
        //stream in file
       uint32 offset_in_buf         = 0;
       uint32 bytes_in_buf          = 0;
       uint32 consumed_file_size    = 0;

	   if ((unsigned long)context_ptr->stream_ptr > (unsigned long)context_ptr->stream_buf + context_ptr->stream_buf_size)
	   {
		    JPEG_PRINT(("[Jpeg_SetStreamPos] error of stream ptr"));
			context_ptr->stream_ptr = context_ptr->stream_buf + context_ptr->stream_buf_size;
	   }

       if ((unsigned long)context_ptr->stream_ptr >= (unsigned long)context_ptr->stream_buf)
       {
           offset_in_buf = (unsigned long)context_ptr->stream_ptr - (unsigned long)context_ptr->stream_buf;
           bytes_in_buf = offset_in_buf + context_ptr->stream_left_size;
       }

       if (context_ptr->read_file_size >= bytes_in_buf)
       {
           //consumed data which is not in buffer
           consumed_file_size = context_ptr->read_file_size - bytes_in_buf;
       }

       if (offset < consumed_file_size
           || offset >= (consumed_file_size + bytes_in_buf))
       {
            //the data of specified position is not in the stream buffer, reload the data in buffer
            context_ptr->read_file_size = offset;
			context_ptr->stream_ptr = NULL;
			context_ptr->stream_left_size = 0;
       }
       else
       {
           //the data of specified position is in the stream buffer, only move the pointer
           context_ptr->stream_ptr = (uint8 *)context_ptr->stream_buf + (offset - consumed_file_size);
           offset_in_buf = (unsigned long)context_ptr->stream_ptr - (unsigned long)context_ptr->stream_buf;
           context_ptr->stream_left_size = bytes_in_buf - offset_in_buf;
       }
   }
   else
   {
        //stream in buffer
        if (offset > context_ptr->stream_buf_size)
        {
			offset = context_ptr->stream_buf_size;
		}

        context_ptr->stream_ptr = (uint8 *)context_ptr->stream_buf + offset;
        context_ptr->stream_left_size = context_ptr->stream_buf_size - offset;
   }
}

#if 0
//////////////////////////////////////////////////////////////////////////
//used in jpeg decode
/*****************************************************************************
**	Name : 			Jpeg_GetC
**	Description:	Get a byte of jpeg stream and move the stream current
**                  position one byte forward
**	Author:			Shan,he
**  Parameters:
**                  context_ptr:        context pointer
**                  output_ptr:         pointer of output data, ouput parameter
**	Note:
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_GetC(JPEG_READ_STREAM_CONTEXT_T *context_ptr, uint8 *output_ptr)
{
	uint8 c = 0;

	if (context_ptr->stream_left_size < 1)
	{
		Jpeg_ReadStream(context_ptr);
	}

	if (PNULL != context_ptr->stream_ptr && context_ptr->stream_left_size > 0)
	{
		c = *context_ptr->stream_ptr++;
		context_ptr->stream_left_size--;
	}
	else
	{
		JPEG_PRINT(("[Jpeg_GetC] no more bytes !"));
		return FALSE;
	}

	*output_ptr = c;

	return TRUE;
}


/*****************************************************************************
**	Name : 			Jpeg_GetWBigEndian
**	Description:	get an uint16 type data in big-endian mode and move the
**                  current position of jpeg stream 2 bytes forward
**	Author:			Shan.He
**  Parameters:
**                  context_ptr:        context pointer
**                  output_ptr:         pointer of output data, ouput parameter
**	Note:
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_GetWBigEndian(JPEG_READ_STREAM_CONTEXT_T *context_ptr, uint16 *output_ptr)
{
	uint16	w = 0;
	uint8	c0 = 0;
	uint8	c1 = 0;

	if (!Jpeg_GetC(context_ptr, &c0) || !Jpeg_GetC(context_ptr, &c1))
	{
		JPEG_PRINT(("[Jpegd_GetW] no more bytes !"));
		return FALSE;
	}

	w = ((c0 << 8) & 0xFF00) | (c1);
	*output_ptr = w;

	return TRUE;
}

/*****************************************************************************
**	Name : 			Jpeg_GetWBigEndian
**	Description:	get an uint16 type data in little-endian mode and move the
**                  current position of jpeg stream 2 bytes forward
**	Author:			Shan.He
**  Parameters:
**                  context_ptr:        context pointer
**                  output_ptr:         pointer of output data, ouput parameter
**	Note:
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_GetWLittleEndian(JPEG_READ_STREAM_CONTEXT_T *context_ptr,
                                     uint16 *output_ptr)
{
	uint16	w = 0;
	uint8	c0 = 0;
	uint8	c1 = 0;

	if (!Jpeg_GetC(context_ptr, &c0) || !Jpeg_GetC(context_ptr, &c1))
	{
		JPEG_PRINT(("[Jpegd_GetW] no more bytes !"));
		return FALSE;
	}

	w = ((c1 << 8) & 0xFF00) | (c0);
	*output_ptr = w;

	return TRUE;
}

/*****************************************************************************
**	Name : 			Jpeg_GetWBigEndian
**	Description:	get an uint32 type data in big-endian mode and move the
**                  current position of jpeg stream 4 bytes forward
**	Author:			Shan.He
**  Parameters:
**                  context_ptr:        context pointer
**                  output_ptr:         pointer of output data, ouput parameter
**	Note:
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_GetLBigEndian(JPEG_READ_STREAM_CONTEXT_T *context_ptr,
                                       uint32 *output_ptr)
{
	uint32	l = 0;
	uint8	c0 = 0;
	uint8	c1 = 0;
    uint8   c2 = 0;
    uint8   c3 = 0;

	if (!Jpeg_GetC(context_ptr, &c0) || !Jpeg_GetC(context_ptr, &c1)
        || !Jpeg_GetC(context_ptr, &c2) || !Jpeg_GetC(context_ptr, &c3))
	{
		JPEG_PRINT(("[Jpegd_GetUINT32] no more bytes !"));
		return FALSE;
	}

	l = ((c0 << 24) & 0xFF000000) | ((c1 << 16) & 0xFF0000)
        | ((c2 << 8) & 0xFF00) | c3;
	*output_ptr = l;

	return TRUE;
}


/*****************************************************************************
**	Name : 			Jpeg_GetWBigEndian
**	Description:	get an uint32 type data in little-endian mode and move the
**                  current position of jpeg stream 4 bytes forward
**	Author:			Shan.He
**  Parameters:
**                  context_ptr:        context pointer
**                  output_ptr:         pointer of output data, ouput parameter
**	Note:
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_GetLLittleEndian(JPEG_READ_STREAM_CONTEXT_T *context_ptr,
                                          uint32 *output_ptr)
{
	uint32	l = 0;
	uint8	c0 = 0;
	uint8	c1 = 0;
    uint8   c2 = 0;
    uint8   c3 = 0;

	if (!Jpeg_GetC(context_ptr, &c0) || !Jpeg_GetC(context_ptr, &c1)
        || !Jpeg_GetC(context_ptr, &c2) || !Jpeg_GetC(context_ptr, &c3))
	{
		JPEG_PRINT(("[Jpegd_GetUINT32] no more bytes !"));
		return FALSE;
	}

	l = ((c3 << 24) & 0xFF000000) | ((c2 << 16) & 0xFF0000)
        | ((c1 << 8) & 0xFF00) | c0;
	*output_ptr = l;

	return TRUE;
}

/*****************************************************************************
**	Name : 			Jpeg_Skip
**	Description:	Skip some bytes and reset the current position of stream
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:        context pointer
**                  len:                skip length
**	Note:
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_Skip(JPEG_READ_STREAM_CONTEXT_T *context_ptr, uint32 len)
{
	BOOLEAN ret = FALSE;

	JPEG_PRINT_LOW(("[Jpeg_Skip], skip len = %d, stream_ptr = 0x%x, left_size = %d",
						len, context_ptr->stream_ptr, context_ptr->stream_left_size));

	if (context_ptr->stream_left_size < 1)
	{
		Jpeg_ReadStream(context_ptr);
	}

	while (NULL != context_ptr->stream_ptr)
	{
		if (context_ptr->stream_left_size >= len)
		{
			context_ptr->stream_left_size -= len;
			context_ptr->stream_ptr += len;
			ret = TRUE;

			JPEG_PRINT_LOW(("[Jpeg_Skip], skip success, len = %d, stream_ptr = 0x%x, left_size = %d",
						len, context_ptr->stream_ptr, context_ptr->stream_left_size));
			break;
		}
		else
		{
			len -= context_ptr->stream_left_size;
			context_ptr->stream_left_size = 0;
			context_ptr->stream_ptr += context_ptr->stream_left_size;
			Jpeg_ReadStream(context_ptr);
		}
	}

	return ret;
}
#endif

/**---------------------------------------------------------------------------*
 **                         Write stream functions                            *
 **---------------------------------------------------------------------------*/
/*****************************************************************************
**	Name :
**	Description:	write the stream in the temp buffer to file
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:     pointer of context
**	Note:           return TRUE if successful else return FALSE
*****************************************************************************/
LOCAL BOOLEAN Jpeg_WriteStreamToFile(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr)
{
	JPEG_PRINT_LOW(("JPEG Stream: Jpeg_WriteStreamToFile should not called \n"));

    if (PNULL != context_ptr->write_file_func && PNULL != context_ptr->write_buf)
    {
        uint32 write_size = 0;
        uint32 actual_write_size = 0;
	uint8   *write_buf      = context_ptr->write_buf;
	uint32 i = 0;

        if ((unsigned long)context_ptr->write_ptr > (unsigned long)context_ptr->write_buf)
        {
            write_size = (unsigned long)context_ptr->write_ptr - (unsigned long)context_ptr->write_buf;
        }

        context_ptr->write_file_func(context_ptr->write_buf, context_ptr->write_file_size,
                                        write_size, &actual_write_size);

        context_ptr->write_ptr = context_ptr->write_buf;
        context_ptr->write_file_size += actual_write_size;

        return (actual_write_size == write_size) ? TRUE : FALSE;
    }
    else
    {
        return FALSE;
    }
}

/*****************************************************************************
**	Name :
**	Description:	set the writing position
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:     pointer of context
**                  offset:          writing position
**	Note:           return TRUE if successful else return FALSE
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_SetWritePos(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr, uint32 offset)
{
    uint8   *write_buf      = context_ptr->write_buf;
    uint32  write_buf_size  = context_ptr->write_buf_size;
    uint32  write_file_size = context_ptr->write_file_size;

    if (PNULL != context_ptr->write_file_func)
    {
        if (offset < write_file_size || offset >= (write_file_size + write_buf_size))
        {
            if (!Jpeg_WriteStreamToFile(context_ptr))
            {
                return FALSE;
            }

            context_ptr->write_ptr = context_ptr->write_buf;
            context_ptr->write_file_size = offset;
        }
        else
        {
            context_ptr->write_ptr = context_ptr->write_buf + (offset - write_file_size);
        }
    }
    else
    {
        if (offset > write_buf_size)
        {
            return FALSE;
        }

        context_ptr->write_ptr = write_buf + offset;
    }

    return TRUE;
}

/*****************************************************************************
**	Name :
**	Description:	get the writing position
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:     pointer of context
**                  offset:          writing position
**	Note:           return the writing position
*****************************************************************************/
PUBLIC uint32 Jpeg_GetWritePos(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr)
{
    uint8   *write_ptr      = context_ptr->write_ptr;
    uint8   *write_buf      = context_ptr->write_buf;
    uint32  write_buf_size  = context_ptr->write_buf_size;
    uint32  write_pos       = 0;

    if (((unsigned long)write_ptr >= (unsigned long)write_buf) && ((unsigned long)write_ptr <= (unsigned long)write_buf + write_buf_size))
    {
        write_pos = ((unsigned long)write_ptr - (unsigned long)write_buf);
    }

    write_pos = (PNULL != context_ptr->write_file_func)
                        ? (write_pos + context_ptr->write_file_size)
                        : write_pos;

    return write_pos;
}

/*****************************************************************************
**	Name :
**	Description:	write an uint8 type value
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:    pointer of context
**                  c:              value to write
**	Note:           return TRUE if successful else return FALSE
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_WriteC(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr, uint8 c)
{
    uint8   *write_ptr = NULL;
    uint8   *write_buf = NULL;
    uint32  write_buf_size = 0;

    if (PNULL == context_ptr->write_ptr)
    {
        context_ptr->write_ptr = context_ptr->write_buf;
    }

    write_ptr = context_ptr->write_ptr;
    write_buf = context_ptr->write_buf;
    write_buf_size = context_ptr->write_buf_size;

    if ((unsigned long)write_ptr >= ((unsigned long)write_buf + write_buf_size))
    {
        if (!Jpeg_WriteStreamToFile(context_ptr))
        {
            return FALSE;
        }
    }

    if (NULL == write_ptr || (((unsigned long)write_ptr + 1) > ((unsigned long)write_buf + write_buf_size)))
    {
        return FALSE;
    }
    else
    {
        *context_ptr->write_ptr++ = c;
        return TRUE;
    }
}

/*****************************************************************************
**	Name :
**	Description:	write an uint16 type value
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:    pointer of context
**                  w:              value to write
**	Note:           return TRUE if successful else return FALSE
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_WriteW(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr, uint16 w)
{
    if(Jpeg_WriteC(context_ptr, (uint8)(w >> 8))
        && Jpeg_WriteC(context_ptr, (uint8)(w)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*****************************************************************************
**	Name :
**	Description:	write an uint32 type value
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:    pointer of context
**                  l:              value to write
**	Note:           return TRUE if successful else return FALSE
*****************************************************************************/
PUBLIC BOOLEAN Jpeg_WriteL(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr, uint32 l)
{
    uint8   *write_ptr = context_ptr->write_ptr;
    uint8   *write_buf = context_ptr->write_buf;
    uint32  write_size = context_ptr->write_buf_size;

    if (NULL == write_ptr || (((unsigned long)write_ptr + 4) > ((unsigned long)write_buf + write_size)))
    {
        return FALSE;
    }
    else
    {
        *context_ptr->write_ptr++ = (uint8)(l >> 24);
        *context_ptr->write_ptr++ = (uint8)(l >> 16);
        *context_ptr->write_ptr++ = (uint8)(l >> 8);
        *context_ptr->write_ptr++ = (uint8)(l);

        return TRUE;
    }
}

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
                              uint8 *block_buf_ptr, uint32 block_size)
{
    uint8   *write_ptr = context_ptr->write_ptr;
    uint8   *write_buf = context_ptr->write_buf;
    uint32  write_size = context_ptr->write_buf_size;
    uint32 i;
    BOOLEAN ret = TRUE;
#if 1
	if (NULL == write_ptr || NULL == block_buf_ptr )
	{
		return FALSE;
	}
	else
	{
		for(i=0; i<block_size; i++)
		{
			if(TRUE != Jpeg_WriteC(context_ptr, *block_buf_ptr++)) {
				ret = FALSE;
				break;
			}
		}
	}
#else
    if (NULL == write_ptr || NULL == block_buf_ptr
        || (((uint32)write_ptr + block_size) > ((uint32)write_buf + write_size)))
    {
        return FALSE;
    }
    else
    {
        SCI_MEMCPY(write_ptr, block_buf_ptr, block_size);
        context_ptr->write_ptr += block_size;
    }
#endif

    return ret;
}
