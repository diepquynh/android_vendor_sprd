/******************************************************************************
 ** File Name:      JpegDec_bitstream.c                                       *
 ** Author:         yi.wang													  *
 ** DATE:           07/12/2007                                                *
 ** Copyright:      2007 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    Initialize the encoder									  *
 ** Note:           None                                                      *
******************************************************************************/
/******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 07/12/2007     yi.wang	         Create.                                  *
******************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8830_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C"
    {
#endif

//#if defined(JPEG_DEC)
//////////////////////////////////////////////////////////////////////////

//for bitstream operation
uint32 s_read_bytes;
uint32 s_jstream_words;
uint32 s_jremain_bit_num;
uint32 s_jremain_byte_num;
uint32 s_jremain_bit_num_back;

uint8 *s_inter_buf_bitstream;
uint8 *s_inter_buf_ptr;
uint32 s_inter_buf_bitstream_len;
uint32 s_header_len;

uint32 s_file_offset;

static uint32 ESC_MODE = 1;

#if PROGRESSIVE_SUPPORT
void Update_Global_Bitstrm_Info(bitstream_info *pBitstrmInfo)
{
	s_inter_buf_bitstream = pBitstrmInfo->src_buf;
	s_jremain_bit_num = pBitstrmInfo->jremain_bit_num;
	s_jstream_words = pBitstrmInfo->jstream_words;
	s_jremain_byte_num = pBitstrmInfo->bytes_in_buf;
}

void Update_Local_Bitstrm_Info(bitstream_info *pBitstrmInfo)
{
	pBitstrmInfo->src_buf = s_inter_buf_bitstream;
	pBitstrmInfo->jremain_bit_num = s_jremain_bit_num;
	pBitstrmInfo->jstream_words = s_jstream_words;
	pBitstrmInfo->bytes_in_buf = s_jremain_byte_num;
}
#endif

int32 check_RstMarker(void)
{
	uint32 i = s_jremain_bit_num>>3;
//	uint32 flush_bits = s_jremain_bit_num - i*8;
	uint8 ret = 0;

//	flush_read(0xFFFFFFFF);
	s_jremain_bit_num = i*8;

	//ret = (uint8)JPEG_GETBITS(8);
	ret = (uint8)JPEG_GETBITS(8);

	if((ret < M_RST0) || (ret > M_RST7))
	{
		return JPEG_FAILED;
	}

	return JPEG_SUCCESS;
}

void JPEG_Fill_Bit_Buffer(void)
{
	register uint8 tmp = 0;

	while((s_jremain_bit_num < 15) && s_jremain_byte_num)
	{
		tmp = *s_inter_buf_bitstream++;
		s_jremain_byte_num--;

		if(ESC_MODE && (tmp == 0xFF))
		{
			do {
				tmp = *s_inter_buf_bitstream++;
				s_jremain_byte_num--;
			} while(tmp == 0xFF);

			if(tmp == 0x00)
			{
				s_jstream_words = (s_jstream_words<<8) | 0xFF;
				s_jremain_bit_num += 8;
			}else if((tmp == M_SOS) || (tmp == M_DHT) || (tmp == M_EOI))
			{
				return;
			}else
			{
				s_inter_buf_bitstream -= 2;
				s_jremain_byte_num+=2;
			}
		}else
		{
			s_jstream_words = (s_jstream_words<<8) | tmp;
			s_jremain_bit_num += 8;
		}
	}
}
#if PROGRESSIVE_SUPPORT
uint8 huff_DECODE_Progressive(d_derived_tbl *tbl, int32 min_bits)
{
	register uint16 l = min_bits;
	register int32 code;

	CHECK_BIT_BUFFER(l);
	code = JPEG_GETBITS(l);
	while(code > tbl->maxcode[l])
	{
		CHECK_BIT_BUFFER(1);
		code = ((code << 1) | JPEG_GETBITS(1));
		l++;
	}

	if(l > 16)
	{
		return 0;
	}

	return tbl->pub->huffval[(int32)(code + tbl->valoffset[l])];
}
#endif
PUBLIC JPEG_RET_E  JpegDec_InitBitream(JPEG_DEC_INPUT_PARA_T  *jpeg_dec_input)
{
	if(PNULL != jpeg_dec_input->read_bitstream)
	{
		jpeg_dec_input->bitstream_len = 3*1024;
		jpeg_dec_input->bitstream_ptr = (uint8 *)JpegDec_ExtraMemAlloc(jpeg_dec_input->bitstream_len);
	}

	s_inter_buf_bitstream = jpeg_dec_input->bitstream_ptr;
	s_jremain_byte_num = jpeg_dec_input->bitstream_len;
	s_inter_buf_bitstream_len = s_jremain_byte_num;

	s_header_len = 0;

	s_file_offset = 0;

	if(PNULL != jpeg_dec_input->read_bitstream)
	{
		JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
		uint32 return_size = 0;

		jpeg_fw_codec->read = jpeg_dec_input->read_bitstream;

		jpeg_fw_codec->read(s_inter_buf_bitstream, s_file_offset, s_inter_buf_bitstream_len, &return_size);
		s_file_offset += return_size;

		s_jremain_byte_num = return_size;
	}

	s_inter_buf_ptr = s_inter_buf_bitstream;

	return JPEG_SUCCESS;
}

PUBLIC BOOLEAN get_char(uint8 *value_ptr)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	BOOLEAN ret = FALSE;

	if(0 == s_jremain_byte_num && PNULL != jpeg_fw_codec->read)
	{
		uint32 return_size = 0;

		jpeg_fw_codec->read(s_inter_buf_bitstream, s_file_offset, s_inter_buf_bitstream_len, &return_size);

		//JPEG_TRACE("[get_char] read stream from file, s_file_offset = %d, read length= %d, return lenght = %d",
		//				s_file_offset, s_inter_buf_bitstream_len, return_size);

		s_file_offset += return_size;
		s_jremain_byte_num = return_size;
		s_inter_buf_ptr = s_inter_buf_bitstream;
	}

	if (s_jremain_byte_num > 0 && PNULL != value_ptr)
	{
		*value_ptr = *s_inter_buf_ptr++;
		s_jremain_byte_num--;
		s_header_len++;

		ret = TRUE;
	}

	return ret;
}

PUBLIC BOOLEAN get_short_word(uint16 *value_ptr)
{
	BOOLEAN ret = FALSE;
	uint8 value0 = 0;
	uint8 value1= 0;

	if (get_char(&value0) && get_char(&value1) && PNULL != value_ptr)
	{
		*value_ptr = (value0 << 8) | value1;
		ret = TRUE;
	}

	return ret;
}

PUBLIC BOOLEAN get_word(uint32 *value_ptr)
{
	BOOLEAN ret = FALSE;
	uint16 value0 = 0;
	uint16 value1= 0;

	if (get_short_word(&value0) && get_short_word(&value1) && PNULL != value_ptr)
	{
		*value_ptr = (value0 << 16) | value1;
		ret = TRUE;
	}

	return ret;
}

PUBLIC BOOLEAN skip_n_byte(uint32 n)
{
	if (s_jremain_byte_num >= n)
	{
		s_inter_buf_ptr += n;
		s_jremain_byte_num -= n;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

PUBLIC uint32 get_header_len(JPEG_DEC_INPUT_PARA_T  *jpeg_dec_input)
{
	//check 0xff00
	{
		uint32 wordAlign_len = ((s_header_len>>2)<<2);
		uint32 unligned_byte = s_header_len - wordAlign_len;
		uint8 *ptr = s_inter_buf_ptr - unligned_byte;
		uint32 i;


		if(unligned_byte>1)
		{
			for(i = 0; i < unligned_byte; i++)
			{
				if(*ptr == 0xff)
				{
					if(*(ptr+1) == 0x00 )
					{
						s_header_len--;
					}

				}

				ptr++;
			}
		}
	}

	return s_header_len;
}

///////////////////////////////////////////////////////////////////////////////////////////
#define SJPG_HEADER_SIZE		  12
#define JPEG_RES_ID_0 			  'S'
#define JPEG_RES_ID_1 			  'J'
#define JPEG_RES_ID_2 			  'P'
#define JPEG_RES_ID_3 			  'G'
///////////////////////////////////////////////////////////////////////////////////////////
PUBLIC uint32 is_res_jpg_file(void)
{
	if (JPEG_RES_ID_0 == s_inter_buf_ptr[0] &&
	    JPEG_RES_ID_1 == s_inter_buf_ptr[1] &&
        JPEG_RES_ID_2 == s_inter_buf_ptr[2] &&
        JPEG_RES_ID_3 == s_inter_buf_ptr[3])
	{
		return TRUE;
	}else
	{
		return FALSE;
	}
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
