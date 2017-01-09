/******************************************************************************
 ** File Name:      vsp_dct.h	                                              *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/11/2009                                                *
 ** Copyright:      2009 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP DCT Module Driver									  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 06/11/2009    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _VSP_DCT_H_
#define _VSP_DCT_H_
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
#define JPG_DCT_REG_BASE	(JPG_CTRL_BASE + 0x5000)
#define JPG_DCT_REG_SIZE	0x44

#define DCT_CFG_OFFSET						0x00
#define DCT_IN_CBP_OFFSET						0x04
#define DCT_Y_QUAN_OFFSET					0x08
#define DCT_UV_QUAN_OFFSET				0x0C
#define DCT_STS_OFFSET						0x10
#define DCT_OUT_CBP_OFFSET						0x14
#define DCT_QUAN_OFFSET					0x18

#define DCT_START_OFFSET						0x2C

#define DCT_CFG_DONE_OFFSET						0x40

typedef struct  
{
	volatile uint32 DCT_CFG;					//[8]: DCT_QUANT_EN, 1'b1: quant enable	1'b0: quant disable		
												//[7]: ASP_MODE, Asp mode enable
												//[6]: ASP_QUANT_TYPE, 0: first type. With quan table	1: second type. Without quan table		
												//[1]: DCT_RUN_MODE, DCT/IDCT run mode.	1: Auto mode	0: Manual mode		
												//[0]: F_TRANS_EN, DCT mode. 1: DCT mode		0: IDCT mode		
	
	volatile uint32 DCT_IN_CBP;				//[5:0]:  DCT_IN_CBP, For 6 8x8 block. When doing DCT/IDCT, This register will be the flag that which block need to be processed

	volatile uint32 DCT_Y_QUAN;				//[27:16]: Y_QUAN_INV_SHIFT, In doing DCT the 1/(2XQP) and shift the 1 to the highest bit.
												//[3:0]: Y_QUAN_SHIFT, The shift value of quan_inv_shift

	volatile uint32 DCT_UV_QUAN;				//[27:16]: UV_Quan_inv_shift, In doing DCT the 1/(2XQP) and shift the 1 to the highest bit.
												//[3:0]: The shift value of quan_inv_shift

	volatile uint32 DCT_STS;					//[4]: DCT_DONE, In manual mode indicate the DCT finished,wirte '1' to clear it.
												//[0]: DCT_IDLE, DCT in IDLE mode

	volatile uint32 DCT_OUT_CBP;				//[5:0]: DCT_OUT_CBP, For 6 8x8 block. When doing DCT or IDCT, This register will be the flag that which block need to be process.

	volatile uint32 DCT_QUAN;				//[20:9], Dct_quan_inv_shift, In doing DCT the 1/(2XQP) and shift the 1 to the highest bit.
												//[8:4], Dct_quan_value, QP value
												//[3:0], Dct_quan_shift, The shift value of quan_inv_shift
	volatile uint32 rsv0[4];

	volatile uint32 DCT_START;					//[0]: Write  1 to start the dct begin to work in manal mode. Read this register always get a 0.
		

	volatile uint32 rsv1[4];			

	volatile uint32 DCT_CFG_DONE;			//[0]: DCT register configuration done

}VSP_DCT_REG_T;

/*DCT OR IDCT */
typedef enum {
		IDCT_MODE = 0, 
		DCT_MODE  
		}DCT_MODE_E;	

/*QUANT MODE*/
typedef enum {
		DCT_QUANT_DISABLE = 0, 
		DCT_QUANT_EN  
		}DCT_QUANT_ENABLE_E;

/*QUANT MODE*/
typedef enum {
		DCT_QUANT_WITH_TBL = 0, 
		DCT_QUANT_WIDHOUT_TBL  
		}DCT_QUANT_TYPE_E;

/*dct work mode*/	
typedef enum {
		DCT_MANUAL_MODE = 0, 
		DCT_AUTO_MODE,  
		DCT_FREE_MODE
		}DCT_WORK_MODE_E;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif //_VSP_DCT_H_