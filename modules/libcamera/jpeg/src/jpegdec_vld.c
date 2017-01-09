/******************************************************************************
 ** File Name:    JpegDec_vld.c												  *
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

const uint8 g_DC_offset [17] =
{
	0,
		DC_LEN1_OFFSET,
		DC_LEN2_OFFSET,
		DC_LEN3_OFFSET,
		DC_LEN4_OFFSET,
		DC_LEN5_OFFSET,
		DC_LEN6_OFFSET,
		DC_LEN7_OFFSET,
		DC_LEN8_OFFSET,
		DC_LEN9_OFFSET,
		DC_LEN10_OFFSET,
		DC_LEN11_OFFSET,
		DC_LEN12_OFFSET,
		DC_LEN13_OFFSET,
		DC_LEN14_OFFSET,
		DC_LEN15_OFFSET,
		DC_LEN16_OFFSET,
};

uint32 s_active_base_max[4][17];    //0: luma, dc, 1: luma ac, 2: chroma dc, 3:chroma ac
								//31: active or not, 23~16: base address, 15~0 max value of the length

LOCAL void configure_maxReg_DC (uint32 * pMaxcode, uint32 beChroma)
{
	uint32 i = 0;
	uint32 maxCode = 0;
	uint32 offset = 0;

	offset = beChroma ? Chroma_DC_LUT_OFFSET : Luma_DC_LUT_OFFSET;

	for (i = 0; i < 16; i++)
	{
		maxCode  = pMaxcode [i+1] & 0xffff;
#if defined(TEST_VECTOR)
	// 	fprintf(g_pfhuffVldTab, "0x%x  ,", maxCode);
#endif
		if (beChroma)
		{
			JPG_WRITE_REG(JPG_VLD_REG_BASE+offset+i*(sizeof(uint32)), maxCode, "VSP_VLD_REG_BASE: configure Chroma DC max register");
		}else
		{
			JPG_WRITE_REG(JPG_VLD_REG_BASE+offset+i*(sizeof(uint32)), maxCode, "VSP_VLD_REG_BASE: configure Luma DC max register");
		}
	}
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
LOCAL void configure_maxReg_AC (uint32 * pMaxcode, uint32 beChroma)
{
	uint32 i = 0;
	uint32 baseAddr =0 ;
	uint32 maxCode = 0;
	uint32 regValue = 0;
	uint32 offset = 0;

	offset = beChroma ? Chroma_AC_LUT_OFFSET : Luma_AC_LUT_OFFSET;

	for(i = 0; i < 16; i++)
	{
		baseAddr = (pMaxcode [i+1] >> 16) & 0xff;
		maxCode  = pMaxcode [i+1] & 0xffff;
		regValue = (maxCode << 8) | baseAddr;
#if defined(TEST_VECTOR)
// 		fprintf(g_pfhuffVldTab, "0x%x  ,", regValue);
#endif
		if(beChroma)
		{
			JPG_WRITE_REG(JPG_VLD_REG_BASE+offset+i*(sizeof(uint32)), regValue, "VSP_VLD_REG_BASE: configure Chroma AC max register");
		}else
		{
			JPG_WRITE_REG(JPG_VLD_REG_BASE+offset+i*(sizeof(uint32)), regValue, "VSP_VLD_REG_BASE: configure Luma AC max register");
		}
	}
}

LOCAL uint32 collect_valid_bits (uint32 * pMax_code_base_act)
{
	uint32 i = 0;
	uint32 valid = 0;

	for (i = 0; i < 16; i++)
	{
		valid |= (pMax_code_base_act[i+1] >> 31) << (15-i);
	}

	return valid;
}

void JPEGFW_configure_validReg(void)
{
	HUFF_TBL_T *htbl = NULL;
	uint32 DCValid = 0, ACValid = 0;
	uint32 valid0 = 0, valid1 = 0, valid2 = 0, valid3 = 0;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	SCI_ASSERT(jpeg_fw_codec != PNULL);

	htbl = &jpeg_fw_codec->dc_huff_tbl[JPEG_FW_LUM_ID];
	valid0 = collect_valid_bits (s_active_base_max[0]);

	htbl = &jpeg_fw_codec->dc_huff_tbl[JPEG_FW_CHR_ID];
	valid1 = collect_valid_bits (s_active_base_max[2]);
	DCValid = (valid0 << 16) | (valid1 << 0);
#if defined(TEST_VECTOR)
// 	fprintf(g_pfhuffVldTab, "0x%x  ,", DCValid);
#endif
	JPG_WRITE_REG(JPG_VLD_REG_BASE+DC_VALID_OFFSET, DCValid, "VLD_DC_VALID: DC valid");

	htbl = &jpeg_fw_codec->ac_huff_tbl[JPEG_FW_LUM_ID];
	valid2 = collect_valid_bits (s_active_base_max[1]);

	htbl = &jpeg_fw_codec->ac_huff_tbl[JPEG_FW_CHR_ID];
	valid3 = collect_valid_bits (s_active_base_max[3]);
	ACValid = (valid2 << 16) | (valid3 << 0);
	JPG_WRITE_REG(JPG_VLD_REG_BASE+AC_VALID_OFFSET, ACValid, "VLD_AC_VALID: ACValid");
#if defined(TEST_VECTOR)
// 	fprintf(g_pfhuffVldTab, "0x%x  ,", ACValid);
#endif
}

/*
1. configure max register for Y and C
2. configure the valid register for Y anc C
*/
PUBLIC void JPEGFW_configure_vld_reg_jpegDec(void)
{
	HUFF_TBL_T *htbl = NULL;

	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	SCI_ASSERT(jpeg_fw_codec != PNULL);

	if(!jpeg_fw_codec->using_default_huff_tab)
	{
		/*configure valid register*/
		JPEGFW_configure_validReg ();

		/*DC max register*/
		htbl = &jpeg_fw_codec->dc_huff_tbl[JPEG_FW_LUM_ID];
		configure_maxReg_DC (s_active_base_max[0], FALSE);

		htbl = &jpeg_fw_codec->dc_huff_tbl[JPEG_FW_CHR_ID];
		configure_maxReg_DC (s_active_base_max[2], TRUE);

		/*AC max register*/
		htbl = &jpeg_fw_codec->ac_huff_tbl[JPEG_FW_LUM_ID];
		configure_maxReg_AC (s_active_base_max[1], FALSE);

		htbl = &jpeg_fw_codec->ac_huff_tbl[JPEG_FW_CHR_ID];
		configure_maxReg_AC (s_active_base_max[3], TRUE);

	#if defined(TEST_VECTOR)
// 		fprintf(g_pfhuffVldTab, "\n\n");
	#endif

		//	printf_max_reg ();
		//	printf_valide_reg ();
	}else//default value;
	{
		uint32 i;
		const uint32 *pMaxCode = jpeg_fw_vld_default_max_code;

		//config DC and AC valid;
		JPG_WRITE_REG(JPG_VLD_REG_BASE+DC_VALID_OFFSET, pMaxCode[0], "VLD_DC_VALID: DC valid");
		JPG_WRITE_REG(JPG_VLD_REG_BASE+AC_VALID_OFFSET, pMaxCode[1], "VLD_AC_VALID: AC valid");

		//config AC and DC max register
		for(i = 2; i < 66; i++)
		{
			JPG_WRITE_REG(JPG_VLD_REG_BASE+DC_VALID_OFFSET+i*4, pMaxCode[i], "configure AC and DC max register");
		}

		SCI_MEMCPY(g_huffTab, jpeg_fw_vld_default_huffTab, 162*4);
	}
}

LOCAL void build_hufftab_DC(uint32 *phufftab, const uint8 *pHuffVal, const uint8 *bits, uint32 beChroma, const uint8 *pDCoffset)
{
	uint32 i = 0, j = 0;
	uint32 number = 0;
	uint32 startAddr = 0;
	uint32 *pTab_nbits = PNULL;
	uint32 shift_bit = 0;

	shift_bit = beChroma ? 24 : 16;

	for(i = 1; i <= 16; i++)
	{
		number = bits[i];
		startAddr = pDCoffset[i];
		pTab_nbits = phufftab + startAddr;

		for(j = 0; j < number; j++)
		{
			pTab_nbits[j] = ((*pHuffVal++) << shift_bit) | pTab_nbits[j];
		}
	}
}

LOCAL void build_hufftab_AC (uint32 * phufftab, const uint8 * pHuffVal, uint32 beChroma)
{
	uint32 i = 0;
	uint32 shift_bits = 0;

	shift_bits = beChroma ? 8 : 0;

	for (i = 0; i < 162; i++)
	{
		phufftab[i] = phufftab[i] | ((*pHuffVal++) << shift_bits);
	}
}

PUBLIC void JPEGFW_build_hufftab_jpegDec(void)
{
	HUFF_TBL_T *htbl = NULL;

	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	SCI_ASSERT(jpeg_fw_codec != PNULL);
	SCI_MEMSET(g_huffTab, 0, 162*(sizeof(uint32)));

	/*build DC huffman table*/
	//luma
	htbl = &jpeg_fw_codec->dc_huff_tbl[JPEG_FW_LUM_ID];
	build_hufftab_DC(g_huffTab, htbl->huffval, htbl->bits, JPEG_FW_LUM_ID, g_DC_offset);

	/*build AC huffman table*/
	//luma
	htbl = &jpeg_fw_codec->ac_huff_tbl[JPEG_FW_LUM_ID];
	build_hufftab_AC(g_huffTab, htbl->huffval, JPEG_FW_LUM_ID);
	if(jpeg_fw_codec->num_components != 1)
	{
		//chroma
		htbl = &jpeg_fw_codec->dc_huff_tbl[JPEG_FW_CHR_ID];
		build_hufftab_DC(g_huffTab, htbl->huffval, htbl->bits, JPEG_FW_CHR_ID, g_DC_offset);

		//chroma
		htbl = &jpeg_fw_codec->ac_huff_tbl[JPEG_FW_CHR_ID];
		build_hufftab_AC(g_huffTab, htbl->huffval, JPEG_FW_CHR_ID);
	}
#if defined(TEST_VECTOR)
// 	printf_huffValTab (g_pfhuffVldTab, g_huffTab, 162);
#endif
//	printf_configured_reg (g_pfConfigure_reg);
}

LOCAL void JPEGFW_FixHuffTbl(HUFF_TBL_T *htbl, int32 is_dc, int32 is_luma)
{
	uint16 p = 0, i = 0, l = 0, lastp = 0, si = 0;
	uint16 code = 0;
	int16  symbol_num = 0;
	uint8 *huffsize  = (uint8*)JpegDec_ExtraMemAlloc((sizeof(uint8)) *(AC_SYMBOL_NUM+1));
	uint16 *huffcode = (uint16*)JpegDec_ExtraMemAlloc((sizeof(uint16)) *(AC_SYMBOL_NUM+1));
	int32  *maxcode = (int32*)JpegDec_ExtraMemAlloc((sizeof(int32)) * (MAX_BITS_SIZE+1));	/* largest code of length k (-1 if none) */
	uint8  *valptr  = (uint8*)JpegDec_ExtraMemAlloc((sizeof(uint8)) * (MAX_BITS_SIZE+1));	/* huffval[] index of 1st symbol of length k */
	uint32 *active_base_max = s_active_base_max[((is_luma?0:1)<<1)|(is_dc?0:1)];

	/* Figure 7.3.5.4.2.1: make table of Huffman code length for each symbol */
	/* Note that this is in code-length order. */
	SCI_MEMSET(huffsize, 0, 257);
	SCI_MEMSET(huffcode, 0xFF, 257*2);

	SCI_ASSERT(htbl->bits != PNULL);
	SCI_ASSERT(htbl->huffval != PNULL);

	p = 0;
	for (l = 1; l <= 16; l++)
	{
		for (i = 1; i <= htbl->bits[l]; i++)
		{
			huffsize[p] = (uint8)l;
			p++;
		}
	}
	huffsize[p] = 0;
	lastp = p;

	symbol_num = p;

	/* Figure 7.3.5.4.2.2: generate the codes themselves */
	/* Note that this is in code-length order. */
	code = 0;
	si = huffsize[0];
	p = 0;
	while (huffsize[p])
	{
		while (huffsize[p] == si)
		{
			huffcode[p] = code;
			p++;
			code++;
		}
		code <<= 1;
		si++;
	}

	/* Figure 13.4.2.3.1: generate decoding tables */
	p = 0;
	for (l = 1; l <= 16; l++)
	{
		if (htbl->bits[l])
		{
			valptr[l] = (uint8)p;	/* huffval[] index of 1st sym of code len l */
			p += htbl->bits[l];
			maxcode [l] = huffcode[p-1];
		}
		else
		{
			maxcode [l] = -1;
		}
	}

	for (l = 1; l <= 16; l++)
	{
		uint32 active;
		uint32 base_addr;
		uint32 max_code;
		if (maxcode [l] != -1)
		{
			active = 1;
			base_addr = valptr [l];
			max_code = maxcode [l];

			active_base_max [l] = (active << 31) | (base_addr << 16) | max_code;
		}
		else
		{
			active = 0;
			base_addr = 0;
			max_code = 0;
			if (l > 1)
			{
				if (maxcode[l-1] != -1)
				{
					max_code = (maxcode[l-1] + 1) * 2;
				}
				else
				{
					max_code = (active_base_max[l-1] & 0xffff) * 2;
				}
			}

			active_base_max[l] = (active << 31) | (base_addr << 16) | max_code;
		}
	}
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
PUBLIC void JPEGFW_InitHuffTbl(void)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	SCI_ASSERT(jpeg_fw_codec != PNULL);

	//luma dc and ac
	JPEGFW_FixHuffTbl(&jpeg_fw_codec->dc_huff_tbl[JPEG_FW_LUM_ID], TRUE, TRUE);
	JPEGFW_FixHuffTbl(&jpeg_fw_codec->ac_huff_tbl[JPEG_FW_LUM_ID], FALSE, TRUE);

	//chroma dc and ac
	if(jpeg_fw_codec->num_components != 1)
	{
		JPEGFW_FixHuffTbl(&jpeg_fw_codec->dc_huff_tbl[JPEG_FW_CHR_ID], TRUE, FALSE);
		JPEGFW_FixHuffTbl(&jpeg_fw_codec->ac_huff_tbl[JPEG_FW_CHR_ID], FALSE, FALSE);
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
