/******************************************************************************
 ** File Name:      vsp_global_define.h                                       *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/11/2009                                                *
 ** Copyright:      2009 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP MEA Module Driver									  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 06/11/2009    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _JPG_GLOBAL_DEFINE_H_
#define _JPG_GLOBAL_DEFINE_H_
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

/*JPEG MCU FORMAT*/
typedef enum {
		JPEG_FW_YUV420 = 0, 
		JPEG_FW_YUV411, 
		JPEG_FW_YUV444, 
		JPEG_FW_YUV422, 
		JPEG_FW_YUV400, 
		JPEG_FW_YUV422_R, 
		JPEG_FW_YUV411_R
		}JPEG_MCU_FORMAT_E;

/*down sample*/
#define DOWN_SAMPLE_DIS     0
#define DOWN_SAMPLE_EN     1

/*vsp work mode*/
#define MANUAL_MODE     0x00
#define AUTO_MODE           0x01
#define FREE_MODE           0x02

#define JPG_RESET_REG       0x60d00004      //bit[6]
#define JPG_CLK_EN_REG  0x60d00000  //bit[5]
#define AXI_CLK_EN_REG  0x60d00008      //bit[6]

#define JPG_CTRL_BASE       0x60b00000

#define	JPG_MEMO0_ADDR				(JPG_CTRL_BASE + 0x0300)
#define	JPG_MEMO1_ADDR				(JPG_CTRL_BASE + 0x2000)
#define	JPG_MEMO2_ADDR				(JPG_CTRL_BASE + 0x3000)
#define	JPG_MEMO3_ADDR				(JPG_CTRL_BASE + 0x3400)
#define	JPG_MEMO4_ADDR				(JPG_CTRL_BASE + 0x3800)
#define	JPG_MEMO5_ADDR				(JPG_CTRL_BASE + 0x3c00)

#define MEMO0_ADDR_SIZE				(64*4)
#define MEMO1_ADDR_SIZE				(162*4)
#define MEMO2_ADDR_SIZE				(216*3)
#define MEMO3_ADDR_SIZE				(216*3)
#define MEMO4_ADDR_SIZE				(128*4)
#define MEMO5_ADDR_SIZE				(128*4)

#define QUANT_TBL_ADDR			        JPG_MEMO0_ADDR
#define INV_QUANT_TBL_ADDR		        JPG_MEMO0_ADDR

#define HUFFMAN_TBL_ADDR			        JPG_MEMO1_ADDR
#define DCT_IO_BUF_ADDR                             JPG_MEMO2_ADDR
#define IDCT_IO_BUF_ADDR                           JPG_MEMO3_ADDR
#define SRC_MCU_BUF_ADDR		                JPG_MEMO4_ADDR
#define MCU_OUT_BUF_ADDR		                JPG_MEMO4_ADDR

/*define frame address and bitstream address*/
/*for JPEG encoding*/
#define SRC_FRAME0_Y                  0x00100000
#define SRC_FRAME0_UV               0x00300000

#define SRC_FRAME1_Y                  0x00400000
#define SRC_FRAME1_UV               0x00600000

#define BIT_STREAM_ENC_0        0x00000000
#define BIT_STREAM_ENC_1        0x00080000

/*for JPEG encoding*/
#define DEC_FRAME0_Y                  0x00100000
#define DEC_FRAME0_UV               0x00300000

#define DEC_FRAME1_Y                  0x00400000
#define DEC_FRAME1_UV               0x00600000

#define BIT_STREAM_DEC_0        0x00000000
#define BIT_STREAM_DEC_1        0x00080000

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif //_JPG_GLOBAL_DEFINE_H_
