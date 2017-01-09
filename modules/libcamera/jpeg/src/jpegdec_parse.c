/******************************************************************************
 ** File Name:    JpegDec_parse.c											  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
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

/* Find the next JPEG marker */
/* Note that the output might not be a valid marker code, */
/* but it will never be 0 or FF */
LOCAL BOOLEAN GetNextMarker(uint32 *ret)
{
	uint8 c = 0;
	int32 nbytes = 0;

	do
	{
		/* skip any non-FF bytes */
		do
		{
			nbytes++;

			if (!get_char(&c))
			{
				JPEG_TRACE("[GetNextMarker] get next marker failed !");
				return FALSE;
			}

		} while (c != 0xFF);

		do
		{
			/* skip any duplicate FFs */
			nbytes++;

			if (!get_char(&c))
			{
				JPEG_TRACE("[GetNextMarker] get next marker failed !");
				return FALSE;
			}
		} while(c == 0xFF);
	}while (c == 0); /* repeat if it was a stuffed FF/00 */

	if(nbytes != 2)
	{
		JPEG_TRACE("Warning!There are some bytes is stuffed in the head!\n");
	}

	*ret = c;

	return TRUE;
}
/************************************************************************/
/*do jpeg baseline and progressive, huffman                                             */
/************************************************************************/
LOCAL JPEG_RET_E GetSOF(BOOLEAN isProgressive, JPEG_DEC_INPUT_PARA_T  *jpeg_dec_input)
{
	uint16	length = 0;
	uint16	width = 0;
	uint16 	height = 0;
	uint8	ci = 0, c = 0;
	uint8 	component_num = 0;
	int8	yuv_id = 0;
	jpeg_component_info *compptr;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	jpeg_dec_input->progressive_mode = isProgressive;

	if(isProgressive)
	{
		JPEG_TRACE("\nJPEG Mode is: Progressive\n");
	}
	else
	{
		JPEG_TRACE("JPEG Mode is: Baseline\n");
	}

	if (!get_short_word(&length) || length < 8)	/*get length*/
	{
		JPEG_TRACE("GetSOF get length error %d", length);
		return JPEG_FAILED;
	}


	if (!get_char(&c))		/*get sample precision*/
	{
		JPEG_TRACE("GetSOF pricision error, %d", c);
		return JPEG_FAILED;
	}

	if (c !=8 )
	{
 		JPEG_TRACE("ERROR!!! pricision = %d\n", c);
		return JPEG_FAILED;
	}

	if (!get_short_word(&height) || !get_short_word(&width))
	{
 		JPEG_TRACE("GetSOF get size error ");
		return JPEG_FAILED;
	}

	jpeg_dec_input->input_height = height;
	jpeg_dec_input->input_width = width;

	if ((jpeg_dec_input->input_height < 8)||(jpeg_dec_input->input_width < 8))
	{
		JPEG_TRACE("WARNING!!! width =%d, height = %d\n", jpeg_dec_input->input_height, jpeg_dec_input->input_width);
	}

	if (!get_char(&component_num)) /*get the component number*/
	{
 		JPEG_TRACE("GetSOF get component num error");
		return JPEG_FAILED;
	}

	if(component_num != 3)
	{
 		JPEG_TRACE("WARNING!!! component num = %d\n", component_num);
	}

	/*check length*/
	if (length != (component_num * 3 + 8))
	{
 		JPEG_TRACE("Error!!! length = %d\n", length);
		return JPEG_FAILED;
	}

	jpeg_fw_codec->num_components = component_num;

	JPEG_TRACE("JPEG: GetSOF, width=%d, height=%d\n",jpeg_dec_input->input_width, jpeg_dec_input->input_height);

	/*Caution: current we only support the YUV format, sequence is Y..U..V*/
// 	if (jpeg_dec_input->comp_info == NULL)	/* do only once, even if suspend */
	{
		jpeg_fw_codec->comp_info = (jpeg_component_info *)JpegDec_ExtraMemAlloc(component_num * sizeof(jpeg_component_info));
	}

	for(ci = 0, compptr = jpeg_fw_codec->comp_info; ci < component_num; ci++, compptr++)
	{
		if (!get_char(&c))
		{
 			JPEG_TRACE("[GetSOF] get yuvid error = %d", c);
			return JPEG_FAILED;
		}

		yuv_id = c;

		compptr->component_index = ci;
		compptr->component_id = yuv_id;

		//index starts from 1, but in our program, ratio and tbl_map starts from 0,
		//so we need minus yuv_id to 1 before use it. Noted by xiaowei.luo@20090107
		yuv_id--;

		if((yuv_id<0)||(yuv_id>2))
		{
			JPEG_TRACE("Error!!! component id = %d\n", yuv_id);
			return JPEG_FAILED;
		}

		if (!get_char(&c))	/*get sample ratio*/
		{
			JPEG_TRACE("Get sample ratio error");
			return JPEG_FAILED;
		}

		compptr->h_samp_factor = (c >> 4) & 0x0F;
		compptr->v_samp_factor = (c     ) & 0x0F;
		if(ci == 0)
		{
			compptr->MCU_width = compptr->h_samp_factor;
			compptr->MCU_height = compptr->v_samp_factor;
		}else
		{
			compptr->MCU_width = 1;
			compptr->MCU_height = 1;
		}

		if (!get_char(&c))	/*get quant table*/
		{
			JPEG_TRACE("Get quant table error");
			return JPEG_FAILED;
		}

		if(c > 2/*JPEG_FW_CHR_ID*/)  //be compliant with two chroma quant table.
		{
			//JPEG_ERROR(JPEG_EID_MISSQUANT, "quant id = %d\n", c);
			return JPEG_FAILED;
		}
		jpeg_fw_codec->tbl_map[yuv_id].quant_tbl_id = c;
	}

	c = (jpeg_fw_codec->comp_info[0].h_samp_factor << 4) | (jpeg_fw_codec->comp_info[0].v_samp_factor);

	if (1 == component_num)
	{
		jpeg_dec_input->input_mcu_info = JPEG_FW_YUV400;
		JPEG_TRACE("YUV Mode is: 4:0:0\n");
	}
	else // componet num is 2 or 3
	{
		switch(c)
		{
		case 0x11:
			jpeg_dec_input->input_mcu_info = JPEG_FW_YUV444;
			JPEG_TRACE("YUV Mode is: 4:4:4\n");
			break;
		case 0x21:
			jpeg_dec_input->input_mcu_info = JPEG_FW_YUV422;
			JPEG_TRACE("YUV Mode is: 4:2:2, V1,H2\n");
			break;
		case 0x41:
			jpeg_dec_input->input_mcu_info = JPEG_FW_YUV411;
			JPEG_TRACE("YUV Mode is: 4:1:1\n");
			break;
		case 0x14:
			jpeg_dec_input->input_mcu_info = JPEG_FW_YUV411_R;
			JPEG_TRACE("YUV Mode is: 4:1:1, V4, H1\n");
			break;
		case 0x22:
			jpeg_dec_input->input_mcu_info = JPEG_FW_YUV420;
			JPEG_TRACE("YUV Mode is: 4:2:0\n");
			break;
		case 0x12:
			jpeg_dec_input->input_mcu_info = JPEG_FW_YUV422_R;
			JPEG_TRACE("YUV Mode is: 4:2:2, V2,H1\n");
			break;
		default:
			//			JPEG_ERROR(JPEG_EID_SAMPLEFORMAT, "format = %d\n", sample_format);
			JPEG_TRACE("unsupport format = %d", c);
			return JPEG_FAILED;
		}

		//check u sample ratio
		c = (jpeg_fw_codec->comp_info[1].h_samp_factor << 4) | (jpeg_fw_codec->comp_info[1].v_samp_factor);
		if (0x11 != c)
		{
			JPEG_TRACE("unsupport u sample ratio = %d", c);
			return JPEG_FAILED;
		}

		//check v sample ratio
		if (component_num > 2)
		{
			//check u sample ratio
			c = (jpeg_fw_codec->comp_info[2].h_samp_factor << 4) | (jpeg_fw_codec->comp_info[2].v_samp_factor);
			if (0x11 != c)
			{
				JPEG_TRACE("unsupport v sample ratio = %d", c);
				return JPEG_FAILED;
			}
		}
	}

	return JPEG_SUCCESS;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
LOCAL JPEG_RET_E GetSOS()
{
	uint16	length = 0;
	uint8	i = 0, c = 0, cc = 0, n = 0, ci = 0;
	uint8   	yuv_id = 0;
	jpeg_component_info *compptr;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	if (!get_short_word(&length) || length < 3)
	{
		JPEG_TRACE("[GetSOS] get length error");
		return JPEG_FAILED;
	}

	if (!get_char(&n))/* Number of components */
	{
		JPEG_TRACE("[GetSOS] get number of component error");
		return JPEG_FAILED;
	}

	JPEG_PRINTF("component = %d\n", n);

	length -= 3;

	if (length != ( n* 2 + 3))
	{
		JPEG_PRINTF("error length = %d\n", length);
		return JPEG_FAILED;
	}

	jpeg_fw_codec->comps_in_scan = n;
	/*CAUTION: current we only support YUV format, and the scan sequence is Y..U..V*/

	for (i = 0, yuv_id = 1; i < n; i++, yuv_id++)
	{
		if (!get_char(&cc))	/*get component id*/
		{
			JPEG_TRACE("[GetSOS] get component id error");
			return JPEG_FAILED;
		}

		if (!get_char(&c))	/*get dc/ac table*/
		{
			JPEG_TRACE("[GetSOS] get dc/ac table error");
			return JPEG_FAILED;
		}

		length -= 2;
		if(cc != yuv_id)
		{
			JPEG_TRACE("sorry, this scan sequence not support\n");
			return JPEG_FAILED;
		}

		/*set huffman table*/
		jpeg_fw_codec->tbl_map[i].dc_huff_tbl_id = (c>>4)&0x0F;
		jpeg_fw_codec->tbl_map[i].ac_huff_tbl_id = c&0x0F;

		for (ci = 0, compptr = jpeg_fw_codec->comp_info; ci < jpeg_fw_codec->num_components; ci++, compptr++)
		{
			if (cc == compptr->component_id)
			{
				jpeg_fw_codec->comp_id_map[i] = cc;
				goto id_found;
			}
		}
id_found:
		JPEG_TRACE("JPEG Component Info: component id = %d, dc_tbl_no = %d, ac_tbl_no = %d\n", jpeg_fw_codec->comp_id_map[i],jpeg_fw_codec->tbl_map[i].dc_huff_tbl_id,
			        jpeg_fw_codec->tbl_map[i].ac_huff_tbl_id);
	}

	/* Collect the additional scan parameters Ss, Se, Ah/Al. */
	if (!get_char(&c) || ! get_char(&cc))
	{
		JPEG_TRACE("[GetSOS] get ss/se error");
		return JPEG_FAILED;
	}

	jpeg_fw_codec->Ss = c;
	jpeg_fw_codec->Se = cc;

	if (!get_char(&c))
	{
		JPEG_TRACE("[GetSOS] get ah/al error");
		return JPEG_FAILED;
	}

	jpeg_fw_codec->Ah = (c >> 4) & 15;
	jpeg_fw_codec->Al = (c     ) & 15;

	JPEG_TRACE("JTRC_SOS_PARAMS: Ss = %d, Se = %d, Ah = %d, Al = %d\n", jpeg_fw_codec->Ss, jpeg_fw_codec->Se,
		jpeg_fw_codec->Ah, jpeg_fw_codec->Al);

	/* Prepare to scan data & restart markers */
	jpeg_fw_codec->next_restart_num = 0;

	/* Count another SOS marker */
	jpeg_fw_codec->input_scan_number++;

	return JPEG_SUCCESS;
}

/* Skip over an unknown or uninteresting variable-length marker */
LOCAL JPEG_RET_E SkipVariable(JPEG_DEC_INPUT_PARA_T  *jpeg_dec_input)
{
	uint16 length = 0;
	//int32 read_bytes = 0;

	if (!get_short_word(&length) || length < 2)
	{
		JPEG_TRACE("[SkipVariable] read length failed, length = %d", length);
		return JPEG_FAILED;
	}

	JPEG_TRACE("JPEG: Skipping length %d\n", length);
	for(length-=2; length > 0; length--)
	{
		uint8 c;

		if (!get_char(&c))
		{
			JPEG_TRACE("[SkipVariable] skip failed");
			return JPEG_FAILED;
		}
	}

	return JPEG_SUCCESS;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
LOCAL JPEG_RET_E GetDRI()
{
	uint16	length = 0;
	uint16 	restart_interval = 0;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	if (!get_short_word(&length) || 4 != length)
	{
		JPEG_TRACE("[GetDRI] read length failed, length = %d", length);
		return JPEG_FAILED;
	}

	if (!get_short_word(&restart_interval))
	{
		JPEG_TRACE("[GetDRI] read restart interval failed");
		return JPEG_FAILED;
	}

	jpeg_fw_codec->restart_interval = restart_interval;

	jpeg_fw_codec->restart_to_go	= jpeg_fw_codec->restart_interval;
	jpeg_fw_codec->next_restart_num = 0;

	return JPEG_SUCCESS;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
LOCAL JPEG_RET_E GetAPP0()
{
#define JFIF_LEN 14

	uint16	length = 0;
	uint8	b[JFIF_LEN];
	uint16	buffp = 0;
	uint8	c = 0;

	if( !get_short_word(&length) || length < 2)
	{
		JPEG_TRACE("[GetAPP0] get length error, length = %d", length);
		return JPEG_FAILED;
	}

	length -= 2;

	/* See if a JFIF APP0 marker is present */
	if (length >= JFIF_LEN)
	{
		for (buffp = 0; buffp < JFIF_LEN; buffp++)
		{
			if (!get_char(&c))
			{
				JPEG_TRACE("[GetAPP0] get app0 error " );
				return JPEG_FAILED;
			}

			b[buffp] = c;
		}
		length -= JFIF_LEN;

		while (length-- > 0)/* skip any remaining data */
		{
			if (!get_char(&c))
			{
				JPEG_TRACE("[GetAPP0] skip error " );
				return JPEG_FAILED;
			}
		}
	}

	return JPEG_SUCCESS;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
LOCAL JPEG_RET_E GetHuffTbl()
{
	uint16 		length = 0;
	uint16		i = 0, index = 0, count = 0;
	HUFF_TBL_T	*htblptr = NULL;
	uint8		*bits;
	uint8		*huffval;
	uint8		*default_bits;
	uint8		*default_huffval;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	uint8		c = 0;

	if(!g_huff_tbl_malloced)
	{
		for(i = 0; i < NUM_HUFF_TBLS; i++)
		{
			jpeg_fw_codec->dc_huff_tbl[i].bits = (uint8*)JpegDec_ExtraMemAlloc((sizeof(uint8))*(MAX_BITS_SIZE+1));
			jpeg_fw_codec->dc_huff_tbl[i].huffval = (uint8*)JpegDec_ExtraMemAlloc((sizeof(uint8))*(AC_SYMBOL_NUM+1));
			jpeg_fw_codec->ac_huff_tbl[i].bits = (uint8*)JpegDec_ExtraMemAlloc((sizeof(uint8))*(MAX_BITS_SIZE+1));
			jpeg_fw_codec->ac_huff_tbl[i].huffval = (uint8*)JpegDec_ExtraMemAlloc((sizeof(uint8))*(AC_SYMBOL_NUM+1));
		}
		g_huff_tbl_malloced = TRUE;
	}

	if (!get_short_word(&length) || length < 2)
	{
		JPEG_TRACE("[GetHuffTbl] get length error, length = %d", length);
		return JPEG_FAILED;
	}

	length -= 2;

	while (length > 0)
	{
		if (!get_char(&c))
		{
			JPEG_TRACE("[GetHuffTbl] get table error");
			return JPEG_FAILED;
		}

		//clear the invalid bits
		index = c & 0x13;

		if (index & 0x10)
		{
			index -= 0x10;	/* AC table definition */
			htblptr = &(jpeg_fw_codec->ac_huff_tbl[index]);
			bits = htblptr->bits;
			huffval = htblptr->huffval;
			if(index == 00) //luma
			{
				default_bits = jpeg_fw_lum_ac_bits_default;
				default_huffval = jpeg_fw_lum_ac_huffvalue_default;
			}
			else //chroma
			{
				default_bits = jpeg_fw_chr_ac_bits_default;
				default_huffval = jpeg_fw_chr_ac_huffvalue_default;
			}
		}
		else
		{				/* DC table definition */
			htblptr = &(jpeg_fw_codec->dc_huff_tbl[index]);
			bits = htblptr->bits;
			huffval = htblptr->huffval;
			if(index == 00) //luma
			{
				default_bits = jpeg_fw_lum_dc_bits_default;
				default_huffval = jpeg_fw_lum_dc_huffvalue_default;
			}
			else //chroma
			{
				default_bits = jpeg_fw_chr_dc_bits_default;
				default_huffval = jpeg_fw_chr_dc_huffvalue_default;
			}
		}

		/*read bits*/
		bits[0] = 0;
		count = 0;
		for (i = 1; i <= 16; i++)
		{
			if (!get_char(&c))
			{
				JPEG_TRACE("[GetHuffTbl] get table error");
				return JPEG_FAILED;
			}

			bits[i] = c;
			if(bits[i] != default_bits[i])
			{
				jpeg_fw_codec->using_default_huff_tab = FALSE;
			}
			count += bits[i];
		}
		if (count > 256)
		{
 			JPEG_TRACE("huff value table len = %d is larger than 256\n", count);
			return JPEG_FAILED;
		}

		for (i = 0; i < count; i++)
		{
			if (!get_char(&c))
			{
				JPEG_TRACE("[GetHuffTbl] get table error");
				return JPEG_FAILED;
			}

			huffval[i] = c;
			if(huffval[i] != default_huffval[i])
			{
				jpeg_fw_codec->using_default_huff_tab = FALSE;
			}
		}

		if (length < (1 + 16 + count))
		{
			JPEG_TRACE("[GetHuffTbl] huffman table error!");
			return JPEG_FAILED;
		}
		else
		{
			length -= (1 + 16 + count);
		}

//		htblptr->bits = bits;
//		htblptr->huffval = huffval;
	}
	return JPEG_SUCCESS;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
LOCAL JPEG_RET_E GetQuantTbl()
{
	uint16	length = 0;
	uint8	n = 0, j = 0, prec = 0;
	uint8	*quant_ptr = NULL;
	int32	has_two_chroma_quant_tbl = FALSE;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	jpeg_fw_codec->using_default_quant_tab = FALSE;

	if (!get_short_word(&length) || length < 2)
	{
		JPEG_TRACE("[GetHuffTbl] get length error, length = %d", length);
		return JPEG_FAILED;
	}

	length -= 2;

	while (length > 0)
	{
		if (!get_char(&n))
		{
			JPEG_TRACE("[GetQuantTbl] get table error");
			return JPEG_FAILED;
		}

		prec = n>>4;
		n &= 0x0F;
		if (prec)
		{
 			JPEG_TRACE("error quant table precision = %d\n", prec);
	// 		return JPEG_FAILED;
		}

		if (n > 2)
		{
 			JPEG_TRACE("warning: error quant table id = %d\n", n);
			return JPEG_FAILED;
		}

		if (n > 1)
		{
 			JPEG_TRACE("warning: error quant table id = %d\n", n);
			//return JPEG_SUCCESS; //removed by xwluo, @20090330
			has_two_chroma_quant_tbl = TRUE;
		}

		jpeg_fw_codec->quant_tbl[n] = (uint8*)JpegDec_ExtraMemAlloc((sizeof(uint8))*64);
		quant_ptr = &jpeg_fw_codec->quant_tbl[n][0];

		if (prec == 0)
		{
			for(j = 0; j < 64; j+=4)
			{
				uint8 c = 0;

				if (!get_char(&c))
				{
					JPEG_TRACE("[GetQuantTbl] get table error");
					return JPEG_FAILED;
				}

				quant_ptr[jpeg_fw_zigzag_order[j]] = c;
				if (!get_char(&c))
				{
					JPEG_TRACE("[GetQuantTbl] get table error");
					return JPEG_FAILED;
				}


				quant_ptr[jpeg_fw_zigzag_order[j+1]] = c;

				if (!get_char(&c))
				{
					JPEG_TRACE("[GetQuantTbl] get table error");
					return JPEG_FAILED;
				}

				quant_ptr[jpeg_fw_zigzag_order[j+2]] = c;
				if (!get_char(&c))
				{
					JPEG_TRACE("[GetQuantTbl] get table error");
					return JPEG_FAILED;
				}

				quant_ptr[jpeg_fw_zigzag_order[j+3]] = c;
			}
		}
		else
		{
			for(j = 0; j < 64; j+=4)
			{
				uint16 c = 0;

				if (!get_short_word(&c))
				{
					JPEG_TRACE("[GetQuantTbl] get table error");
					return JPEG_FAILED;
				}

				quant_ptr[jpeg_fw_zigzag_order[j]] = (uint8)c;

				if (!get_short_word(&c))
				{
					JPEG_TRACE("[GetQuantTbl] get table error");
					return JPEG_FAILED;
				}

				quant_ptr[jpeg_fw_zigzag_order[j+1]] = (uint8)c;

				if (!get_short_word(&c))
				{
					JPEG_TRACE("[GetQuantTbl] get table error");
					return JPEG_FAILED;
				}

				quant_ptr[jpeg_fw_zigzag_order[j+2]] = (uint8)c;

				if (!get_short_word(&c))
				{
					JPEG_TRACE("[GetQuantTbl] get table error");
					return JPEG_FAILED;
				}

				quant_ptr[jpeg_fw_zigzag_order[j+3]] = (uint8)c;
			}
		}

		if (length < (64 + 1 + 64 * prec))
		{
			JPEG_TRACE("[GetQuantTbl] huffman table error!");
			return JPEG_FAILED;
		}
		else
		{
			length -= (64 + 1 + 64 * prec);
		}
	}

	if(has_two_chroma_quant_tbl) //check if these two chroma quant tables are same or not.
	{
		uint8	*u_tbl_ptr = &jpeg_fw_codec->quant_tbl[1][0];
		uint8	*v_tbl_ptr = &jpeg_fw_codec->quant_tbl[2][0];

		for(j = 0; j < 64; j++)
		{
			if(u_tbl_ptr[j] != v_tbl_ptr[j])
			{
				return JPEG_FAILED;
			}
		}

		JPEG_TRACE("U and V quant table is same\n");
	}

	return JPEG_SUCCESS;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
/* Scan and process JPEG markers that can appear in any order */
/* Return when an SOI, EOI, SOFn, or SOS is found */
PUBLIC JPEG_MARKER_E ProcessTables(JPEG_DEC_INPUT_PARA_T  *jpeg_dec_input)
{
	uint32 c = 0;

	while (TRUE)/*lint !e716*/
	{
		if(!GetNextMarker(&c))
		{
			return M_ERROR;
		}

		switch (c)
		{
			case M_SOF0:
			case M_SOF1:
				if(GetSOF(FALSE, jpeg_dec_input) != JPEG_SUCCESS)
				{
					return M_ERROR;
				}
				break;
			case M_SOF2:
				if(GetSOF(TRUE, jpeg_dec_input) != JPEG_SUCCESS)
				{
					return M_ERROR;
				}
				JPEG_TRACE("Sorry, we can not support the Progressive image!\n");
				break;
				//return M_ERROR;
			case M_SOI:
			case M_EOI:
			case M_SOS:
				return c;

			case M_DHT:
				if(GetHuffTbl() != JPEG_SUCCESS)
				{
					return M_ERROR;
				}
				break;

			case M_DQT:
				if(GetQuantTbl() != JPEG_SUCCESS)
				{
					return M_ERROR;
				}
				break;

			case M_APP0:
				if(GetAPP0() != JPEG_SUCCESS)
				{
					return M_ERROR;
				}
				break;

			case M_DRI:
				if(GetDRI() != JPEG_SUCCESS)
				{
					return M_ERROR;
				}
				break;

		//	case M_SOF1:
			case M_SOF3:
			case M_SOF5:
			case M_SOF6:
			case M_SOF7:
			case M_JPG:
			case M_SOF9:
			case M_SOF10:
			case M_SOF11:
			case M_SOF13:
			case M_SOF14:
			case M_SOF15:
			case M_DAC:
			case M_RST0:
			case M_RST1:
			case M_RST2:
			case M_RST3:
			case M_RST4:
			case M_RST5:
			case M_RST6:
			case M_RST7:
			case M_TEM:
	 			JPEG_TRACE("Unexpected marker 0x%02x\n", c);
				return M_ERROR;

			default:	/* must be DNL, DHP, EXP, APPn, JPGn, COM, or RESn */
				if(SkipVariable(jpeg_dec_input) != JPEG_SUCCESS)
				{
					return M_ERROR;
				}
				break;
		}
	}
}

PUBLIC JPEG_RET_E ParseHead(JPEG_DEC_INPUT_PARA_T  *jpeg_dec_input)
{
	uint32 c = 0;

	/* Expect an SOI marker first */
	while(1)/*lint !e716*/
	{
		if ((!GetNextMarker(&c)) || (c == M_EOI) )
		{
			JPEG_TRACE("[JPEG_HWParseHead] find the SOI marker error: %x", c);
			return JPEG_FAILED;
		}

		if(c == M_SOI)
		{
			break;
		}
	}

	/* Process markers until SOF */
	c = ProcessTables(jpeg_dec_input);

	switch (c)
	{
		case M_ERROR:
			return JPEG_FAILED;

		case M_SOS:
			if(GetSOS() != JPEG_SUCCESS)
			{
				return JPEG_FAILED;
			}
			break;

		case M_EOI:
			return JPEG_FAILED;

		default:
			return JPEG_FAILED;
	}

	return JPEG_SUCCESS;
}

/******************************************************************************
// Purpose:	get information of res jpeg
// Author:	shan.he
 // Input:    None
// Output:  None
// Return:
// Note:
******************************************************************************/
PUBLIC JPEG_RET_E ParseHead_Res(JPEG_DEC_INPUT_PARA_T  *jpeg_dec_input)
{
	JPEG_RET_E eRet  = JPEG_SUCCESS;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	uint16 w = 0;
	uint16 h = 0;
	uint16 format = 0;
	uint16 quant_level = 0;

	/*skip the res ID*/
	if (!skip_n_byte(4))
	{
		JPEG_TRACE("[ParseHead_Res] skip ID error");
		return JPEG_FAILED;
	}

	if (!get_short_word(&h) || !get_short_word(&w) )
	{
		JPEG_TRACE("[ParseHead_Res] get size error");
		return JPEG_FAILED;
	}

	jpeg_dec_input->input_height = h;
	jpeg_dec_input->input_width = w;

	jpeg_fw_codec->num_components = 3;

	if (!get_short_word(&format))
	{
		JPEG_TRACE("[ParseHead_Res] get format error");
		return JPEG_FAILED;
	}

	switch (format)
	{
	case 0:
		jpeg_dec_input->input_mcu_info = JPEG_FW_YUV420;
		break;
	case 1:
		jpeg_dec_input->input_mcu_info = JPEG_FW_YUV411;
		break;
	case 2:
		jpeg_dec_input->input_mcu_info = JPEG_FW_YUV444;
		break;
	case 3:
		jpeg_dec_input->input_mcu_info = JPEG_FW_YUV422;
		break;
	case 4:
		jpeg_dec_input->input_mcu_info = JPEG_FW_YUV400;
		jpeg_fw_codec->num_components = 1;
		break;
	default:
		//SCI_PASSERT(0, ("unsupported sample"));
		return JPEG_FAILED;
	}

	if (!get_short_word(&quant_level))
	{
		JPEG_TRACE("[ParseHead_Res] get format error");
		return JPEG_FAILED;
	}

	switch (quant_level)
	{
	case 0:
		jpeg_dec_input->quant_level = JPEG_QUALITY_LOW;
		break;

	case 1:
		jpeg_dec_input->quant_level = JPEG_QUALITY_MIDDLE_LOW;
		break;

	case 2:
		jpeg_dec_input->quant_level = JPEG_QUALITY_MIDDLE;
		break;

	case 3:
		jpeg_dec_input->quant_level = JPEG_QUALITY_MIDDLE_HIGH;
		break;

	case 4:
		jpeg_dec_input->quant_level = JPEG_QUALITY_HIGH;
		break;

	default:
		//SCI_PASSERT(0, ("unsupported quality"));
		return JPEG_FAILED;
	}

	return eRet;
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
