/******************************************************************************
 ** File Name:    JpegDec_pvld.c											  *
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

#if PROGRESSIVE_SUPPORT
//#if defined(JPEG_DEC)
//////////////////////////////////////////////////////////////////////////

#define GET_CHAR(reslt, pBitstrm) { reslt = *pBitstrm++;}

#define GET_WORD(reslt, pBitstrm)	\
{ register	uint16 ret1 = *pBitstrm++;\
  register  uint16 ret2 = *pBitstrm++;\
  reslt = ((ret1<<8)|ret2);\
}

JPEG_RET_E get_huff_table(uint8 *pBitstrm)
{
	uint16 length = 0;
	uint16 i = 0, index = 0, count = 0;
	HUFF_TBL_T *htblptr;
	uint8 *bits, *huffval;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

	GET_WORD(length,pBitstrm);

	length -= 2;

	while(length >0)
	{
		GET_CHAR(index, pBitstrm);

		if(index & 0x10)
		{
			index -= 0x10;
			htblptr = &(jpeg_fw_codec->ac_huff_tbl[index]);

		}else
		{
			htblptr = &(jpeg_fw_codec->dc_huff_tbl[index]);
		}

		bits = htblptr->bits;
		huffval = htblptr->huffval;

		if(index >= NUM_HUFF_TBLS)
		{
			JPEG_TRACE("error huffman table id = %d\n", index);
			return JPEG_FAILED;
		}

		/*read bits*/
		bits[0] = 0;
		count = 0;

		for(i = 1; i <= 16; i++)
		{
			GET_CHAR(bits[i], pBitstrm);
			count += bits[i];
		}

		if(count > 256)
		{
			JPEG_TRACE("huff value table len = %d is larger than 256\n", count);
			return JPEG_FAILED;
		}

		for(i = 0; i < count; i++)
		{
			GET_CHAR(huffval[i], pBitstrm);
		}

		length -= (1+16+count);
	}

	return JPEG_SUCCESS;
}

void build_vld_table(d_derived_tbl *tbl, int32 is_dc, int32 tbl_no)
{
	uint16 p = 0, i = 0, l = 0, lastp = 0, si = 0;
	uint16 code = 0;
	int16 symbol_num = 0;
	uint16 look_bits;
	int16 ctr;
	uint8 *huffsize = (uint8 *)JpegDec_ExtraMemAlloc(sizeof(uint8) * (AC_SYMBOL_NUM+1));
	uint16 *huffcode = (uint16 *)JpegDec_ExtraMemAlloc(sizeof(uint16) * (AC_SYMBOL_NUM+1));
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	HUFF_TBL_T *pub = tbl->pub;

	/* Figure 7.3.5.4.2.1: make table of Huffman code length for each symbol */
	/* Note that this is in code-length order. */
	SCI_MEMSET(huffsize, 0, sizeof(uint8) * (AC_SYMBOL_NUM+1));
	SCI_MEMSET(huffcode, 0xFF, sizeof(uint16) * (AC_SYMBOL_NUM+1));

	if(is_dc)
	{
		SCI_MEMCPY(pub->bits, jpeg_fw_codec->dc_huff_tbl[tbl_no].bits, 17);
		SCI_MEMCPY(pub->huffval, jpeg_fw_codec->dc_huff_tbl[tbl_no].huffval, 257);
	}else
	{
		SCI_MEMCPY(pub->bits, jpeg_fw_codec->ac_huff_tbl[tbl_no].bits, 17);
		SCI_MEMCPY(pub->huffval, jpeg_fw_codec->ac_huff_tbl[tbl_no].huffval, 257);
	}

	p = 0;
	for(l = 0; l <= 16; l++)
	{
		for(i = 1; i <= pub->bits[l]; i++)
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
	while(huffsize[p])
	{
		while(huffsize[p] == si)
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
	for(l = 1; l <= 16; l++)
	{
		if(pub->bits[l])
		{
			tbl->valoffset[l] = (int32)p - (int32)huffcode[p];/* offset */
			p += pub->bits[l];
			tbl->maxcode[l] = huffcode[p-1];
		}else
		{
			tbl->maxcode[l] = -1;
		}
	}


	p = 0;
	for(l = 1; l <= HUFF_FIRST_READ; l++)
	{
		for(i = 1; i <= (int32)pub->bits[l]; i++, p++)
		{
			/* l = current code's length, p = its index in huffcode[] & huffval[]. */
			/* Generate left-justified code followed by all possible bit sequences */
			look_bits = (huffcode[p] <<(HUFF_FIRST_READ-l));
			for(ctr = 1<<(HUFF_FIRST_READ-l); ctr > 0; ctr--)
			{
				tbl->look_nbits[look_bits] = l;
				tbl->look_sym[look_bits] = pub->huffval[p];
				look_bits++;
			}
		}
	}

	JpegDec_FreeNBytes(sizeof(uint8) * (AC_SYMBOL_NUM+1));
	JpegDec_FreeNBytes(sizeof(uint16) * (AC_SYMBOL_NUM+1));

	return;
}

#define HUFF_EXTEND(x, s)	((x) < (1 << ((s)-1)) ? \
	(x) + (-1 << (s)) + 1 : \
(x))

/*lint --e{737}*/
BOOLEAN decode_mcu_DC_first(int16 **MCU_data)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();
	int32 curr_scan = progressive_info->cur_scan;
	int32 Al = progressive_info->Al;
	register int32 s, r;
	int32 blkn, ci;
	int16 *block;
	phuff_entropy_info *entropy = &(progressive_info->buf_storage[curr_scan].entropy);
	jpeg_component_info *compptr;
	d_derived_tbl *tbl;

	/* Process restart marker if needed; may have to suspend */
	if((jpeg_fw_codec->restart_interval) && (jpeg_fw_codec->restart_interval != 0x3FFFF))
	{
		if(entropy->restarts_to_go == 0)
		{
			if(!check_RstMarker())
			{
				return JPEG_FAILED;
			}

			entropy->next_restart_num += 1;
			entropy->next_restart_num &= 0x07;
			entropy->restarts_to_go = (uint8)jpeg_fw_codec->restart_interval;
			entropy->last_dc_value[0] = 0;
			entropy->last_dc_value[1] = 0;
			entropy->last_dc_value[2] = 0;
		}
	}

	/* If we've run out of data, just leave the MCU set to zeroes.
     * This way, we return uniform gray for the remainder of the segment.
     */
    /* Outer loop handles each block in the MCU */
	for(blkn = 0; blkn < progressive_info->block_num; blkn++)
	{
		block = MCU_data[blkn];
		ci = progressive_info->blocks_membership[blkn];
		compptr = progressive_info->cur_comp_info[ci];
		tbl = entropy->vld_table[compptr->dc_tbl_no];

		/* Decode a single block's worth of coefficients */
		/* Section F.2.2.1: decode the DC coefficient difference */
		HUFF_DECODE(s, tbl, label1);

		if(s)
		{
			CHECK_BIT_BUFFER((uint32)s);
			r = JPEG_GETBITS(s);
			s = HUFF_EXTEND(r, s);
		}

		/* Convert DC difference to actual value, update last_dc_val */
		s += entropy->last_dc_value[ci];
		entropy->last_dc_value[ci] = s;

		/* Scale and output the coefficient (assumes jpeg_natural_order[0]=0) */
		block[0] = (int16)(s<<Al);
	}

	/* Account for restart interval (no-op if not using restarts) */
	entropy->restarts_to_go--;

	return TRUE;
}

/*
 * MCU decoding for AC initial scan (either spectral selection,
 * or first pass of successive approximation).
 */
BOOLEAN decode_mcu_AC_first (int16 **MCU_data)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();
	int32 curr_scan = progressive_info->cur_scan;
	int32 Se = progressive_info->Se;
	int32 Al = progressive_info->Al;
	register int32 s, k, r;
	uint32 EOBRUN;
//	int32 blkn, ci;
	int16 *block;
	phuff_entropy_info *entropy = &(progressive_info->buf_storage[curr_scan].entropy);
	d_derived_tbl *tbl;

	/* Process restart marker if needed; may have to suspend */
	/* Process restart marker if needed; may have to suspend */
	if((jpeg_fw_codec->restart_interval) && (jpeg_fw_codec->restart_interval != 0x3FFFF))
	{
		if(entropy->restarts_to_go == 0)
		{
			if(!check_RstMarker())
			{
				return JPEG_FAILED;
			}
			entropy->next_restart_num += 1;
			entropy->next_restart_num &= 0x07;
			entropy->restarts_to_go = (uint8)jpeg_fw_codec->restart_interval;
			entropy->last_dc_value[0] = 0;
			entropy->last_dc_value[1] = 0;
			entropy->last_dc_value[2] = 0;
		}
	}

	/* If we've run out of data, just leave the MCU set to zeroes.
	 * This way, we return uniform gray for the remainder of the segment.
	 */

	/* Load up working state.
     * We can avoid loading/saving bitread state if in an EOB run.
     */
	EOBRUN = entropy->EOBRUN;	/* only part of saved state we need */

	/* There is always only one block per MCU */
	if(EOBRUN > 0)/* if it's a band of zeroes... */
	{
		EOBRUN--;/* ...process it now (we do nothing) */
	}else
	{
		block = MCU_data[0];
		tbl = entropy->ac_derived_tbl;

		for(k = progressive_info->Ss; k <= Se; k++)
		{
			HUFF_DECODE(s, tbl, label1);
			r = s >> 4;
			s &= 15;
			if(s)
			{
				k += r;
				CHECK_BIT_BUFFER((uint32)s);
				r = JPEG_GETBITS(s);
				s = HUFF_EXTEND(r, s);
				/* Scale and output coefficient in natural (dezigzagged) order */
				block[jpeg_fw_zigzag_order[k]] = (int16)(s<<Al);
			}else
			{
				if(r == 15)
				{
					/* ZRL */
					k += 15; /* skip 15 zeroes in band */
				}else
				{
					/* EOBr, run length is 2^r + appended bits */
					EOBRUN = 1 << r;
					if(r)
					{
						/* EOBr, r > 0 */
						CHECK_BIT_BUFFER((uint32)r);
						r = JPEG_GETBITS(r);
						EOBRUN += r;
					}
					EOBRUN--; /* this band is processed at this moment */
					break;
				}
			}
		}
	}

	/* Completed MCU, so update state */
	entropy->EOBRUN = EOBRUN; /* only part of saved state we need */

	/* Account for restart interval (no-op if not using restarts) */
	entropy->restarts_to_go--;

	return TRUE;
}

/*
 * MCU decoding for DC successive approximation refinement scan.
 * Note: we assume such scans can be multi-component, although the spec
 * is not very clear on the point.
 */
BOOLEAN decode_mcu_DC_refine (int16 **MCU_data)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();
	int32 curr_scan = progressive_info->cur_scan;
	int32 blkn;
	int32 p1 = 1<< (progressive_info->Al);/* 1 in the bit position being coded */
	int16 *block;
	phuff_entropy_info *entropy = &(progressive_info->buf_storage[curr_scan].entropy);

	/* Process restart marker if needed; may have to suspend */
	if((jpeg_fw_codec->restart_interval) && (jpeg_fw_codec->restart_interval != 0x3FFFF))
	{
		if(entropy->restarts_to_go == 0)
		{
			if(!check_RstMarker())
			{
				return JPEG_FAILED;
			}

			entropy->next_restart_num += 1;
			entropy->next_restart_num &= 0x07;
			entropy->restarts_to_go = (uint8)jpeg_fw_codec->restart_interval;
			entropy->last_dc_value[0] = 0;
			entropy->last_dc_value[1] = 0;
			entropy->last_dc_value[2] = 0;
		}
	}

	/* Not worth the cycles to check insufficient_data here,
	* since we will not change the data anyway if we read zeroes.
	*/

	/* Outer loop handles each block in the MCU */
	for(blkn = 0; blkn < progressive_info->block_num; blkn++)
	{
		block = MCU_data[blkn];

		/* Encoded data is simply the next bit of the two's-complement DC value */
		CHECK_BIT_BUFFER(1);
		if(JPEG_GETBITS(1))
		{
			block[0] |= p1;/* Note: since we use |=, repeating the assignment later is safe */
		}
	}

	/* Note: since we use |=, repeating the assignment later is safe */
	entropy->restarts_to_go--;

	return TRUE;
}

/*
 * MCU decoding for AC successive approximation refinement scan.
 */
BOOLEAN decode_mcu_AC_refine (int16 **MCU_data)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();
	int32 curr_scan = progressive_info->cur_scan;
	int32 Se = progressive_info->Se;
	register int32 s = 0, k = 0, r = 0;
	uint32 EOBRUN = 0;
	int32 p1 = 1<< (progressive_info->Al);/* 1 in the bit position being coded */
	int32 m1 = (-1)<<(progressive_info->Al);/* -1 in the bit position being coded */
	int16 *block;
	phuff_entropy_info *entropy = &(progressive_info->buf_storage[curr_scan].entropy);
	d_derived_tbl *tbl;
	int16 *thiscoef;
	int32 num_newnz = 0;
	int32 newnz_pos[JPEG_FW_DCTSIZE2] = {0};

	/* Process restart marker if needed; may have to suspend */
	if((jpeg_fw_codec->restart_interval) && (jpeg_fw_codec->restart_interval != 0x3FFFF))
	{
		if(entropy->restarts_to_go == 0)
		{
			if(!check_RstMarker())
			{
				return JPEG_FAILED;
			}

			entropy->next_restart_num += 1;
			entropy->next_restart_num &= 0x07;
			entropy->restarts_to_go = (uint8)jpeg_fw_codec->restart_interval;
			entropy->last_dc_value[0] = 0;
			entropy->last_dc_value[1] = 0;
			entropy->last_dc_value[2] = 0;
		}
	}

	/* If we've run out of data, don't modify the MCU.
	*/
	/* Load up working state */
	EOBRUN = entropy->EOBRUN; /* only part of saved state we need */

	/* There is always only one block per MCU */
	block = MCU_data[0];
	tbl = entropy->ac_derived_tbl;

	/* If we are forced to suspend, we must undo the assignments to any newly
     * nonzero coefficients in the block, because otherwise we'd get confused
     * next time about which coefficients were already nonzero.
     * But we need not undo addition of bits to already-nonzero coefficients;
     * instead, we can test the current bit to see if we already did it.
     */
	num_newnz = 0;

	/* initialize coefficient loop counter to start of band */
	k = progressive_info->Ss;

	if(EOBRUN == 0)
	{
		for(; k <= Se; k++)
		{
			HUFF_DECODE(s, tbl, label1);
			r = s>>4;
			s &= 15;
			if(s)
			{
				if(s != 1)/* size of new coef should always be 1 */
				{
					JPEG_TRACE("JWRN_HUFF_BAD_CODE!\n");
				}
				CHECK_BIT_BUFFER(1);
				if(JPEG_GETBITS(1))
				{
					s = p1;/* newly nonzero coef is positive */
				}else
				{
					s = m1;/* newly nonzero coef is negative */
				}
			}else
			{
				if(r != 15)
				{
					EOBRUN = 1 <<r;/* EOBr, run length is 2^r + appended bits */
					if(r)
					{
						CHECK_BIT_BUFFER((uint32)r);
						r = JPEG_GETBITS(r);
						EOBRUN += r;
					}

					break;/* rest of block is handled by EOB logic */

				}
				/* note s = 0 for processing ZRL */
			}

			/* Advance over already-nonzero coefs and r still-zero coefs,
			* appending correction bits to the nonzeroes.  A correction bit is 1
			* if the absolute value of the coefficient must be increased.
			*/
			do {
				thiscoef = block + jpeg_fw_zigzag_order[k];

				if(*thiscoef != 0)
				{
					CHECK_BIT_BUFFER(1);
					if(JPEG_GETBITS(1))
					{
						if((*thiscoef & p1) == 0)
						{
							 /* do nothing if already set it */
							if(*thiscoef >= 0)
							{
								*thiscoef += p1;
							}else
							{
								*thiscoef += m1;
							}
						}
					}
				}else
				{
					if(--r < 0)
					{
						break; /* reached target zero coefficient */
					}
				}

				k++;
			} while(k <= Se);

			if(s)
			{
				int32 pos = jpeg_fw_zigzag_order[k];

				/* Output newly nonzero coefficient */
				block[pos] = (int16)s;

				/* Remember its position in case we have to suspend */
				newnz_pos[num_newnz++] = pos;
			}
		}
	}

	if(EOBRUN > 0)
	{
		/* Scan any remaining coefficient positions after the end-of-band
		* (the last newly nonzero coefficient, if any).  Append a correction
		* bit to each already-nonzero coefficient.  A correction bit is 1
		* if the absolute value of the coefficient must be increased.
		*/
		for(; k <= Se; k++)
		{
			thiscoef = block + jpeg_fw_zigzag_order[k];

			if(*thiscoef != 0)
			{
				CHECK_BIT_BUFFER(1);
				if(JPEG_GETBITS(1))
				{
					if((*thiscoef & p1) == 0)
					{
						/* do nothing if already changed it */
						if(*thiscoef >= 0)
						{
							*thiscoef += p1;
						}else
						{
							*thiscoef += m1;
						}
					}
				}
			}
		}

		/* Count one block completed in EOB run */
		EOBRUN--;
	}

	/* Completed MCU, so update state */
	entropy->EOBRUN = EOBRUN; /* only part of saved state we need */

	/* Account for restart interval (no-op if not using restarts) */
	entropy->restarts_to_go--;

	return TRUE;
}

uint32 init_scan_entropy_info(phuff_entropy_info *p_entropy_info)
{
	JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();
	phuff_entropy_info *entropy = p_entropy_info;
	int32 curr_scan_num = progressive_info->cur_scan;
	int32 is_DC_band, bad;
	int32 ci, tbl_no;
	jpeg_component_info *compptr;
	int32 Ss, Se, Al, Ah, comps_in_scan;
	int i;

	/* Mark derived tables unallocated */
	for (i = 0; i < NUM_HUFF_TBLS; i++)
	{
		entropy->vld_table[i] = (d_derived_tbl*)JpegDec_ExtraMemAlloc(sizeof(d_derived_tbl));
		if(entropy->vld_table[i] == NULL)
		{
			return JPEG_FAILED;
		}

		entropy->vld_table[i]->pub = (HUFF_TBL_T *)JpegDec_ExtraMemAlloc(sizeof(HUFF_TBL_T));

		entropy->vld_table[i]->pub->bits = (uint8*)JpegDec_ExtraMemAlloc(17);
		entropy->vld_table[i]->pub->huffval = (uint8*)JpegDec_ExtraMemAlloc(257);
	}

	//
	Ss = progressive_info->buf_storage[curr_scan_num].Ss;
	Se = progressive_info->buf_storage[curr_scan_num].Se;
	Al = progressive_info->buf_storage[curr_scan_num].Al;
	Ah = progressive_info->buf_storage[curr_scan_num].Ah;
	comps_in_scan = progressive_info->buf_storage[curr_scan_num].comps_in_scan;

	is_DC_band = (Ss == 0);

	/* Validate scan parameters */
	bad = FALSE;
	if(is_DC_band)
	{
		if(Se != 0)
		{
			bad = TRUE;
		}
	}else
	{
		/* need not check Ss/Se < 0 since they came from unsigned bytes */
		if((Ss > Se) || (Se >= JPEG_FW_DCTSIZE2))
		{
			bad = TRUE;
		}

		/* AC scans may have only one component */
		if(comps_in_scan != 1)
		{
			bad = TRUE;
		}
	}

	if(Ah != 0)
	{
		/* Successive approximation refinement scan: must have Al = Ah-1. */
		if(Al != Ah -1)
		{
			bad = TRUE;
		}
	}

	if(Al > 13) /* need not check for < 0 */
	{
		bad = TRUE;
	}
/* Arguably the maximum Al value should be less than 13 for 8-bit precision,
   * but the spec doesn't say so, and we try to be liberal about what we
   * accept.  Note: large Al values could result in out-of-range DC
   * coefficients during early scans, leading to bizarre displays due to
   * overflows in the IDCT math.  But we won't crash.
   */

	if (bad)
	{
		JPEG_TRACE("%d,%d,%d,%d", Ss, Se, Ah, Al);
		return JPEG_FAILED;
	}

	/* Update progression status, and verify that scan order is legal.
   * Note that inter-scan inconsistencies are treated as warnings
   * not fatal errors ... not clear if this is right way to behave.
   */
  /* Select MCU decoding routine */
	if(Ah == 0)
	{
		if(is_DC_band)
		{
			entropy->decode_mcu = decode_mcu_DC_first;
		}else
		{
			entropy->decode_mcu = decode_mcu_AC_first;
		}
	}else
	{
		if(is_DC_band)
		{
			entropy->decode_mcu = decode_mcu_DC_refine;
		}else
		{
			entropy->decode_mcu = decode_mcu_AC_refine;
		}
	}

	for(ci = 0; ci < comps_in_scan; ci++)
	{
		compptr = &(progressive_info->buf_storage[curr_scan_num].cur_comp_info[ci]);

		/* Make sure requested tables are present, and compute derived tables.
		* We may build same derived table more than once, but it's not expensive.
		*/
		if(is_DC_band)
		{
			if(Ah == 0)
			{
				tbl_no = compptr->dc_tbl_no;
				build_vld_table(entropy->vld_table[tbl_no], TRUE, tbl_no);
			}
		}else
		{
			tbl_no = compptr->ac_tbl_no;
			build_vld_table(entropy->vld_table[tbl_no], FALSE, tbl_no);
			/* remember the single active table */
			entropy->ac_derived_tbl = entropy->vld_table[tbl_no];
		}

		/* Initialize DC predictions to 0 */
		entropy->last_dc_value[ci] = 0;
	}

	return JPEG_SUCCESS;
}

/************************************************************************/
/*init the bitstream information of the scan                            */
/************************************************************************/
void init_scan_bitstrm_info(bitstream_info *address, uint8 *buf_addr, uint32 buf_len)
{
	address->src_buf = buf_addr;
	address->src_buf_len = buf_len;
	address->bytes_in_buf = buf_len;
	address->jstream_words = 0;
	address->jremain_bit_num = s_jremain_bit_num;
	address->read_write_bytes = buf_len;
}

/************************************************************************/
/*get one sos segment                                                   */
/************************************************************************/
LOCAL JPEG_RET_E get_sos(uint8 *pBitstrm, int32 *scan_len)
{
	uint16	length = 0;
	uint8	i = 0, c = 0, cc = 0, n = 0, ci = 0;
	uint8   yuv_id = 0;
	jpeg_component_info *compptr;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();

	GET_WORD(*scan_len, pBitstrm);
	GET_CHAR(n, pBitstrm);  /* Number of components */
	JPEG_PRINTF("component = %d\n", n);

	length = *scan_len - 3;

	if (length != ( n* 2 + 3))
	{
		JPEG_PRINTF("error length = %d\n", length);
		return JPEG_FAILED;
	}

	progressive_info->comps_in_scan = n;
	/*CAUTION: current we only support YUV format, and the scan sequence is Y..U..V*/

	for (i = 0, yuv_id = 1; i < n; i++, yuv_id++)
	{
		GET_CHAR(cc, pBitstrm);	/*get component id*/
		GET_CHAR(c,pBitstrm);	/*get dc/ac table*/

		length -= 2;
		if(cc != yuv_id)
		{
		//	JPEG_TRACE("sorry, this scan sequence not support\n");
		// 	return JPEG_FAILED;
		}

		/*set huffman table*/
		jpeg_fw_codec->tbl_map[i].dc_huff_tbl_id = (c>>4)&0x0F;
		jpeg_fw_codec->tbl_map[i].ac_huff_tbl_id = c&0x0F;

		for (ci = 0, compptr = progressive_info->comp_info; ci < jpeg_fw_codec->num_components; ci++, compptr++)
		{
			if (cc == compptr->component_id)
			{
				progressive_info->comp_id_map[i] = cc;
				goto id_found;
			}
		}
id_found:
		JPEG_TRACE("JPEG Component Info: component id = %d, dc_tbl_no = %d, ac_tbl_no = %d\n", cc,jpeg_fw_codec->tbl_map[i].dc_huff_tbl_id,
			        jpeg_fw_codec->tbl_map[i].ac_huff_tbl_id);
	}

	/* Collect the additional scan parameters Ss, Se, Ah/Al. */
	GET_CHAR(progressive_info->Ss,pBitstrm);
	GET_CHAR(progressive_info->Se,pBitstrm);

	GET_CHAR(c,pBitstrm);
	progressive_info->Ah = (c >> 4) & 15;
	progressive_info->Al = (c     ) & 15;

	JPEG_TRACE("JTRC_SOS_PARAMS: Ss = %d, Se = %d, Ah = %d, Al = %d\n", progressive_info->Ss, progressive_info->Se,
		progressive_info->Ah, progressive_info->Al);

	/* Prepare to scan data & restart markers */
	jpeg_fw_codec->next_restart_num = 0;

	/* Count another SOS marker */
	progressive_info->input_scan_number++;

	return JPEG_SUCCESS;
}

void init_one_scan(int32 scan_index, uint8 *bs_ptr, int32 bs_len)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();
	JPEG_SOS_T *pScan = &(progressive_info->buf_storage[scan_index]);
	int32 comps_in_scan;
	int32 ci;

	//find one scan and print its previous scan information.
	JPEG_TRACE("Entry Point%d: Marker:%x%x, addr:0x%x, length:%d, Fisrt Byte:%x\n",
			scan_index, *(bs_ptr+bs_len), *(bs_ptr+bs_len+1), (uint32)(bs_ptr+1), bs_len-1, *(bs_ptr+1));

	progressive_info->cur_scan = scan_index;

	init_scan_bitstrm_info(&(progressive_info->buf_storage[scan_index].address), bs_ptr+1, bs_len-1);

	//the below information have been got when parsing sos header.
	pScan->Ss = progressive_info->Ss;
	pScan->Se = progressive_info->Se;
	pScan->Ah = progressive_info->Ah;
	pScan->Al = progressive_info->Al;
	pScan->comps_in_scan = comps_in_scan = progressive_info->comps_in_scan;
	//get component info of current scan
	for(ci = 0; ci < comps_in_scan; ci++)
	{
		jpeg_component_info *cur_comp_info = &(pScan->cur_comp_info[ci]);

		cur_comp_info->component_id = progressive_info->comp_id_map[ci];
		SCI_MEMCPY(cur_comp_info, &(progressive_info->comp_info[cur_comp_info->component_id -1]), sizeof(jpeg_component_info));
		cur_comp_info->dc_tbl_no = jpeg_fw_codec->tbl_map[ci].dc_huff_tbl_id;
		cur_comp_info->ac_tbl_no = jpeg_fw_codec->tbl_map[ci].ac_huff_tbl_id;

		if(comps_in_scan == 1)
		{
			if(jpeg_fw_codec->input_mcu_info == JPEG_FW_YUV420)
			{
				cur_comp_info->MCU_width = 1;
				cur_comp_info->MCU_height = 1;
			}else
			{
				cur_comp_info->MCU_width = cur_comp_info->h_samp_factor;
				cur_comp_info->MCU_height = cur_comp_info->v_samp_factor;
			}
		}
	}

	init_scan_entropy_info(&(pScan->entropy));

	return;
}

/************************************************************************/
/* Find the Entry point of SOS											*/
/************************************************************************/
uint32 JPEG_Generate_Entry_Point_Map_Progressive(void)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	JPEG_PROGRESSIVE_INFO_T *progressive_info = JPEGFW_GetProgInfo();
	uint8 *pBs = jpeg_fw_codec->stream_0;
	int32 m = 0, n = 0; //m: byte length in one scan. n: byte length of all scans.
	int32 scan_index = 0; //the first scan has been found when parsing jpeg header.
	int32 scan_len = 0;

	//Find the whole start points of the SOS
	for(;;)
	{
		m++; n++;

		while(((*pBs++) == 0xFF) && ((*pBs == M_DHT) ||(*pBs == M_SOS)))
		{
			//init previouse scan
			init_one_scan(scan_index++, pBs-m-1, m);

			//for get_dht
			if(*pBs == M_DHT)
			{
				int32 dht_len;
				get_huff_table(pBs+1);//Renew the huff table;
				dht_len = (*(pBs+1)<<8)|(*(pBs+2));
				n += (dht_len+2);
				pBs += (dht_len+2);
			}

			//following is the found scan
			//fill the content of the buf_storage;
			get_sos(pBs+1, &scan_len);

			pBs += (scan_len);

			m = 0;
		}

		if(((*(pBs-1) == 0xFF) && (*pBs == M_EOI)) || (n >= jpeg_fw_codec->decoded_stream_len))
		{
			//init previouse scan
			init_one_scan(scan_index, pBs-m-1, m);

			break;
		}
	}

	progressive_info->input_scan_number = scan_index+1;

	return JPEG_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//#endif //JPEG_DEC
#endif
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
