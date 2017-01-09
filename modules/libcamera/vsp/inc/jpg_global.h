/******************************************************************************
 ** File Name:      vsp_global.h	                                          *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/11/2009                                                *
 ** Copyright:      2009 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    														  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 06/11/2009    Xiaowei Luo     Create.                                     *
 ** 27/09/2012    Leon Li      Modified.                                     *
 *****************************************************************************/
#ifndef _VSP_GLOBAL_H_
#define _VSP_GLOBAL_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif 

#define JPG_GLB_REG_BASE	(JPG_CTRL_BASE + 0x0000)
#define JPG_GLB_REG_SIZE	0x6C

/*register offset*/
#define GLB_CTRL_OFFSET		0x00
#define MB_CFG_OFFSET		0x04

#define GLB_PITCH_OFFSET		0x0C
#define GLB_STS_OFFSET			0x10

#define GLB_INT_STS_OFFSET	0x20
#define GLB_INT_EN_OFFSET	        0x24
#define GLB_INT_CLR_OFFSET	0x28
#define GLB_INT_RAW_OFFSET	0x2C

#define GLB_FRM_ADDR0_OFFSET		0x40
#define GLB_FRM_ADDR1_OFFSET		0x44
#define GLB_FRM_ADDR2_OFFSET		0x48
#define GLB_FRM_ADDR3_OFFSET		0x4C
#define GLB_FRM_ADDR4_OFFSET		0x50
#define GLB_FRM_ADDR5_OFFSET		0x54
#define GLB_FRM_ADDR6_OFFSET		0x58
#define GLB_FRM_ADDR7_OFFSET		0x5C

#define BUS_GAP_OFFSET		0x60
#define MST_STS_OFFSET		        0x68

#if 0
typedef struct ahb_soft_rest_tag
{
	volatile uint32 ahb_soft_reset;		//[31:12]: reserved
										//[11]: emc reset
										//[10]: usbd software reset
										//[9:5]: reserved
										//[4]: vsp software reset
										//[3]: ccir software reset
										//[2]: dcam software reset
										//[1]: dma software reset
										//[0]: reserved

}AHB_SOFT_RST_T;
#endif

typedef struct jpg_global_reg_tag
{
	volatile uint32 CTRL;		//[31:9]: reserved
									//[8]: MEM_CTRL_STS, Internal memory control status, 0: controlled by JPG module; 1: controlled by AHB bus
									//[7:6]: WR_SWT, 32bits WORD order mode when write, assume the write 32bits data is {B0, B1, B2, B3}
									//[5:4]: RD_SWT, 32bits WORD order mode when read, assume the write 32bits data is {B0, B1, B2, B3}
									//[3]: AXI_SWT, 64bits AXI bus order mode, assume the accessed 64bits data is {W1, W0}, 0: {W1, W0}; 1:{W1, W0} Wx is 32bits WORD
									//[2]: AHB_CTRL_MEM AHB control internal memory, 0: JPG module control memory; 1: AHB bus control memory.
									//[1]: ENC_EN, Encoding enable, 0: decoding; 1: encoding
									//[0]: JPG_EN, JPG module enable, active high
	
	volatile uint32 MB_CFG;		//[31:28] reserved
									//[27]: UV_MODE, UV mode0: UV interlaced;1: UV separated.
									//[26:24]: MB_FORMAT, MCU format info
									//      3'b000 - 4:2:0, 3'b001 - 4:1:1, 3'b010 - 4:4:4, 3'b011 - 4:2:2, 
									//      3'b100 - 4:0:0, 3'b101 - 4:1:1 rotation, 3'b110 - 4:2:2 rotation
									//[23:22]: Reserved
									//[21:12]: MB_Y_MAX, Max MB ID in Y direction, Note: maximum is 256 MCU  
									//[11:10]: Reserved
									//[9:0]: MB_X_MAX, Max MB ID in X direction, Note: maximum is 256 MCU  

	volatile uint32 RSV0;		
	
	volatile uint32 PITCH;		//[31:13]: reserved
									//[12:0]: JPG_PITCH, YUV data storage pitch in memory, it is equal to image with normally.

	volatile uint32 STS;		//[31:4]: reserved
									//[3:2]: WFIFO_STS, Write FIFO status, Bit[3]: full flag, active high; Bit[2]: empty flag, active high.
									//[1:0]: RFIFO_STS, Read FIFO status, Bit[1]: full flag, active high; Bit[0]: empty flag, active high.

	volatile uint32 RSV1[3];		                                    

	volatile uint32 INT_STS;		//[31:4]: reserved
									//[3]: MBIO_INT_STS, MBIO masked interrupt status, active high
									//[2]: VLD_INT_STS, VLD masked interrupt status, active high
									//[1]: VLC_INT_STS, VLC masked interrupt status, active high
									//[0]: BSM_INT_STS, BSM masked interrupt status, active high
	
	volatile uint32 INT_EN;		//[31:4]: reserved
									//[3]: MBIO_INT_EN, MBIO  interrupt enable, active high
									//[2]: VLD_INT_EN, VLD  interrupt enable, active high
									//[1]: VLC_INT_EN, VLC  interrupt enable, active high
									//[0]: BSM_INT_EN, BSM  interrupt enable, active high

        volatile uint32 INT_CLR;		//[31:4]: reserved
									//[3]: MBIO_INT_CLR, MBIO interrupt clear, write '1' to do it
									//[2]: VLD_INT_CLR, VLD interrupt clear, write '1' to do it
									//[1]: VLC_INT_CLR, VLC interrupt clear, write '1' to do it
									//[0]: BSM_INT_CLR, BSM interrupt clear, write '1' to do it
									
	volatile uint32 INT_RAW;		//[31:4]: reserved
									//[3]: MBIO_INT_RAW, MBIO raw interrupt status, active high
									//[2]: VLD_INT_RAW, VLD raw interrupt status, active high
									//[1]: VLC_INT_RAW, VLC raw interrupt status, active high
									//[0]: BSM_INT_RAW, BSM raw interrupt status, active high

	volatile uint32 RSV2[4];	

        volatile uint32 FRM_ADDR[8];		//[31:2]: frame address (word unit)
                                                                        //[1:0]: reserved

        volatile uint32 BUS_GAP;		//[31:8]: reserved                                                                        
                                                                        //[7:0]: JPG module AXI bus accessing gap.

     	volatile uint32 RSV3;	
                                                                   
        volatile uint32 MST_STS;		//[31:1]: reserved                                                                        
                                                                        //[0]: JPG master status, 0: Idle; 1: busy.
                                
}JPG_GLB_REG_T;


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 

#endif //_VSP_GLOBAL_H_

