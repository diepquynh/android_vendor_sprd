/******************************************************************************
 ** File Name:      vsp_vld.h	                                              *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/11/2009                                                *
 ** Copyright:      2009 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP VLD Module Driver									  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 06/11/2009    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _VSP_VLD_H_
#define _VSP_VLD_H_
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
#define JPG_VLD_REG_BASE	(JPG_CTRL_BASE + 0x4800)
#define JPG_VLD_REG_SIZE	0x18C

#define VLD_CTRL_OFFSET                                                0x00
#define JPEG_RESTART_MCU_CNT_OFFSET        0x70
#define JPEG_DC_Y_OFFSET                                    0x74
#define JPEG_DC_UV_OFFSET                                 0x78
#define JPEG_TOTAL_MCU_OFFSET                       0x7C
#define JPEG_RESTART_MCU_INTV_OFFSET	0x80
#define DC_VALID_OFFSET					0x84
#define AC_VALID_OFFSET					0x88
#define Luma_DC_LUT_OFFSET				0x8C
#define Chroma_DC_LUT_OFFSET			        0xCC
#define Luma_AC_LUT_OFFSET				0x10C
#define Chroma_AC_LUT_OFFSET			        0x14C

typedef struct jpg_vld_reg_tag
{
	volatile uint32 CTRL;				//[31]:		VLD_STATUS 0: VLD is idle	1: VLD is busy
											//[30]:		VLD_ERROR, VLD has error, active high
											//[0]:		VLD_START, Start VLD, active high											
	

	/*jpeg configure register*/

	volatile uint32 rsv0 [27];
                                            
	volatile uint32 RESTART_MCU_CNT;	//[19:0]: 	RESTART_MCU_CNT: The number of VLD restart MCU
	
	volatile uint32 DC_Y;				//[10:0]: 	JPEG_DC_Y,

	volatile uint32 DC_UV;				//[26:16]: 	JPEG_DC_V,
											//[10:0]: 	JPEG_DC_U,

	volatile uint32 TOTAL_MCU;				//[19:0]: 	Total number of VLD restart MCU 

	volatile uint32 RESTART_MCU_INTV;	//[19:0] 	The interval of VLD restart MCU		
	
	volatile uint32 DC_VALID;				//[31:0]: 	[31]: Luma 1-bit DC valid	[30]: Luma 2-bit DC valid
											//[15]:		Chroma 1-bit DC valid		[0]: Chroma 16-bit DC valid
		

	volatile uint32 AC_VALID;				//[31:0]: 	[31]: Luma 1-bit AC valid	[30]: Luma 2-bit AC valid	
											//[15]: 	Chroma 1-bit AC valid		[0]: Chroma 16-bit AC valid
		
	volatile uint32 Luma_DC_LUT[16];		//[15:0]: 	luma DC max code
	
	volatile uint32 Chrom_DC_LUT[16];		//[15:0]: 	chroma DC max code

	volatile uint32 Luma_AC_LUT[16];		//[23:8]:	Luma AC max code
											//[7:0]: 	base address

	volatile uint32 Chroma_AC_LUT[16];		//[23:8]: 	AC max code
											//[7:0]: 	base address

}JPG_VLD_REG_T;

#define JPG_VLD_IsError	(((*(volatile uint32*)(JPG_VLD_REG_BASE + VLD_CTRL_OFFSET) & 0x40000000) == 0x40000000) ? 1 : 0)

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif  //_VSP_VLD_H_
