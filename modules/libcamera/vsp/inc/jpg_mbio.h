/******************************************************************************
 ** File Name:      vsp_mbc.h	                                              *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/11/2009                                                *
 ** Copyright:      2009 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP MBC Module Driver									  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 06/11/2009    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _VSP_MBIO_H_
#define _VSP_MBIO_H_
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
#define JPG_MBIO_REG_BASE			(JPG_CTRL_BASE + 0x4000)
#define JPG_MBIO_REG_SIZE			0x20

#define CFG_OFFSET					0x00

#define MB_START_OFFSET			0x08
#define MCU_NUM_OFFSET			0x0C
#define BUF_STS_OFFSET				0x10

#define CTRL_OFFSET				0x18
#define STS_OFFSET					0x1C

typedef struct  
{
	volatile uint32 CFG;			//[31:2]: Reserved.
										//[1:0]:		MBIO_RUN_MODE, MBIO start mode, 2'b10: Free run mode; Others: reserved

	volatile uint32 RSV0;			

	volatile uint32 MB_START;			//[25:16]:	MBIO_START_Y_ID
										//[9:0]:	MBIO_START_X_ID
										
	volatile uint32 MCU_NUM;			//[31:20]:	Reserved
										//[19:0]:	MCU_NUM, MBIO total MCU number

	volatile uint32 BUF_STS;			//[2]:		MBIO_JPEG_END, JPEG picture decoding is completed.
	                                                                        //Note: SW can only write "1'b1" to clear this bit.
										//[1]:	MBIO_VDB_BUF1_RDY, VDB buffer1 is ready to be accessed by HW, active high
										//Note: SW can only write "1'b1" to set this bit.HW will clear this bit when the task is completed.
										//[0]:	MBIO_VDB_BUF0_RDY, VDB buffer1 is ready to be accessed by HW, active high
										//Note: SW can only write "1'b1" to set this bit.HW will clear this bit when the task is completed.

	volatile uint32 RSV1;		
                                                                            
	volatile uint32 CTRL;		//[0]: MBIO_CFG_FLAG: MBIO SW configuration is completed
										//Note: only used for free run mode to indicates HW to work. Once one picture codec is completed, this signal will be cleared by HW.

	volatile uint32 STS;		//[10:8]: VDBM_FSM_ST, MBIO_VDBM FSM state
	                                        //[5:4]: CTRL_FSM_ST, MBIO_CTRL FSM state
	                                        //[2]: MBIO_VDB_REQ, Data transfer to AXI
	                                        //[1]: WFIFO_MBIO_FULL, Master WFIFO full flag
	                                        //[0]: RFIFO_MBIO_EMP, master RFIFO empty flag

}JPG_MBIO_REG_T;

#define JPG_MBIO_IsJpegDecEnd (((*(volatile uint32 *)(JPG_MBIO_REG_BASE+BUF_STS_OFFSET) & 0x4) == 0x4) ? 1 : 0)
#define JPG_MBIO_MCU_X  ((*(volatile uint32 *)(JPG_MBIO_REG_BASE+MB_START_OFFSET)) & 0x000003ff)
#define JPG_MBIO_MCU_Y  (((*(volatile uint32 *)(JPG_MBIO_REG_BASE+MB_START_OFFSET))>>16) & 0x000003ff)

/*mbio run mode*/
typedef enum {
		MBIO_RUN_MANUAL_MODE = 0, 
		MBIO_RUN_FREE_MODE = 2  
		}MBIO_RUN_MODE_E;	

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif //_VSP_MBIO_H_
