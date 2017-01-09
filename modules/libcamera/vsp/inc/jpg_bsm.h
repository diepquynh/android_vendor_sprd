/******************************************************************************
 ** File Name:      vsp_bsmr.h	                                              *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           11/20/2007                                                *
 ** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP bsm Driver for video codec.	  						  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 11/20/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _JPG_BSM_H_
#define _JPG_BSM_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define JPG_BSM_REG_BASE	(JPG_CTRL_BASE + 0x4400)
#define JPG_BSM_REG_SIZE	0x24

#define BSM_CFG0_OFFSET		0x00
#define BSM_CFG1_OFFSET		0x04
#define BSM_CFG2_OFFSET		0x08
#define BSM_WDATA_OFFSET		0x0c
#define BSM_RDATA_OFFSET		0x10
#define BSM_TOTAL_BITS_OFFSET	0x14
#define BSM_STS0_OFFSET		0x18
#define BSM_STS1_OFFSET		0x1c
#define BSM_RDY_OFFSET		0x20

typedef struct  bsmr_control_register_tag
{
	volatile uint32 CFG0;		//[31]: BUF0_STS, 0: Buffer is ready for write 	1: Buffer is ready for read
									//[30]: BUF1_STS, 0: Buffer is ready for write 	1: Buffer is ready for read
									//[20:0]: BS_BUF_SIZE, Buffer size for bit stream, the unit is 1 words		

	volatile uint32 CFG1;		//[31]: DESTUFFING_EN, Destuffing function enable, active high
									//[30:21]: Reserved
									//[20:0]: BS_OFFSET, Start offset address in bit stream buffer , unit is word

	volatile uint32 CFG2;		//[31:30]: Reserved
                                                                        //[29:24]: OPT_BITS, Number of bits to be flushed, only valid in decoding. The supported range is from 1 to 32 bits
									//[2]: CNT_CLR, Clear statistical counter
									//[1]: BSM_CLR, Move data remained in FIFO to external memory or discard data in FIFO
									//[0]: BSM_FLUSH, Remove n bit from bit stream buffer, only valid in decoding		

	volatile uint32 WDATA;		//[31:0]: BSM_WDATA, The data to be added to the bit stream


	volatile uint32 RDATA;		//[31:0]: BSM_RDATA, Current 32-bit bit stream in the capture window

	volatile uint32 TOTAL_BITS;		//[31:0]: BSM_TOTAL_BITS, The number of bits added to or remove from the bit stream

	volatile uint32 STS0;		//[31]: BSM_STATUS, BSM is active/inactive, When encoding: 0 - busy, 1 -idle; When decoding 0 - idle, 1 - busy
									//[30:28]: BSM_STATE, BSM control status
									//[27]: DATA_TRAN, 0: BSM is idle, BSM clear action is enable,1, BSM is transferring data, BSM clear action is disable
									//[16:12]: BSM_SHIFT_REG, The bit amount has been shifted in BS shifter
									//[9:8]: DESTUFFING_LEFT_DCNT, The remained data amount in the de-stuffing module, uinit is word
									//[4]: PING-PONG_BUF_SEL, Current ping-pong buffer ID, buffer0 or buffer1
									//[3:0]: FIFO_DEPTH, BSM FIFO depth
	
	volatile uint32 STS1;		//[1]: BSM_MST_REQ, The data transfer request to AHBM
									//[0]: RFIFO_EMPTY, AXI RFIFO empty flag

	volatile uint32 RDY;		//[0]: BSM_RDY
									// 0: SW can't access BSM internal FIFO
									//	1: SW is allowed to access BSM internal FIFO
									//Note: when SW will read/write the 
									//	  BSM FIFO (access the 0x08 register) this bit should be checked.

}JPG_BSM_REG_T;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_JPG_BSM_H_
