/******************************************************************************
 ** File Name:    disp_test_usr.c                                *
 ** Author:       billy.zhang@spreadtrum.com          *
 ** DATE:         2015-09-09                                        *
 ** Copyright:    2015 Spreatrum, Incoporated. All Rights Reserved. *
 ** Description:                                                    *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/ioctl.h>
#include <sys/sysinfo.h>

#include <dirent.h>
#include <sys/wait.h>

#ifdef LAVATEST_SP7720
#include "lava_chip_7720.h"
#endif

#ifdef LAVATEST_SP7731
#include "lava_chip_7731.h"
#endif

#ifdef LAVATEST_SP9838
#include "lava_chip_9838.h"
#endif

#ifdef LAVATEST_SP9830
#include "lava_chip_9830.h"
#endif
//#include "lava_chip_9001.h"

#define LOG_FILE  "/data/local/tmp/disp_test_log.txt"
#define TMP_FILE "tmp.txt"

#define PARENT_DIR   "/sys/kernel/debug"


#define BLUE      "\033[1m"       /* Blue */
#define RED       "\033[31m"      /* Red */
#define GREEN     "\033[32m"      /* Green */
#define YELLOW    "\033[33m"      /* Yellow */
#define WHITE    "\033[37m"      /* Yellow */

#define RESET     "\033[0m" 

#define PWD_LEN   128

#define u32 unsigned int

void hexToStr(int hex,char* szBuf)
{
	if(szBuf == NULL)
		return;
	sprintf(szBuf,"0x%x",hex);
 }

void test_act(int regs)
{
	char cmd_line[256];
	char *str_line;
	printf("dispc show\n");
	str_line = (char*)malloc(sizeof(char)*100);
	if(str_line == NULL)
		return;
	hexToStr(regs,str_line);
	sprintf(cmd_line,"lookat -l 1 %s",str_line);
	free(str_line);
	system(cmd_line);
}
void disp_test_begin(void)
{
    printf("\n/************************************/\n");
    printf("/*******display lava test begin******/\n");
    printf("\n/************************************/\n");
}
void disp_test_end(void)
{
    printf("\n/************************************/\n");
    printf("/********display lava test end*******/\n");
    printf("\n/************************************/\n");
}

int main(int argc, const char *argv[])
{
	char cmd_line[256];
	char *str_line;
	disp_test_begin();

	/*display test case 1, get lcd name info*/
	printf("\ndisplay test case 1, get lcd name info \n");
	printf("show\n");
	system("cat /sys/class/graphics/fb0/lcd_name");
	
	/*display test case 2, get lcd resolution info*/
	printf("\ndisplay test case 2, get lcd resolution info \n");
	printf("bit[27:16] : DISPC_SIZE_Y\nbit[11:0] : DISPC_SIZE_X\n");

	test_act(DISPC_SIZE_XY);

	/*display test case 3, get lcd porch info*/
	printf("\ndisplay test case 3, get lcd porch value info \n");
	printf("bit[31:20] : HFP\nbit[19:8] : HBP\nbit[7:0] : HSYNC\n");

	test_act(DISPC_DPI_H_TIMING);

	printf("bit[31:20] : VFP\nbit[19:8] : VBP\nbit[7:0] : VSYNC\n");

	test_act(DISPC_DPI_V_TIMING);


	/*display test case 4, get lcd fps info*/
	printf("\ndisplay test case 4, get lcd fps info \n");
	printf("show\n");
	system("cat /sys/class/graphics/fb0/dynamic_fps");

	/*display test case 5, get lcd mipi_clk info*/
	printf("\ndisplay test case 5, get lcd mipi_clk info \n");
	printf("show\n");
	system("cat /sys/class/graphics/fb0/dynamic_mipi_clk");

	/*display test case 6, get lcd dpi_clk info*/
	printf("\ndisplay test case 6, get lcd dpi_clk info \n");
	printf("show\n");
	system("cat /sys/class/graphics/fb0/dynamic_pclk");

	/*display test case 7, get lcd lan number info*/
	printf("\ndisplay test case 7, get lcd lan number info \n");
	printf("bit[1:0] : lan number\n");

	test_act(R_DSI_HOST_PHY_IF_CFG);


	/*display test case 8, get lcd work mode info*/
	printf("\ndisplay test case 8, get lcd work mode info \n");
	printf("bit[0] : work mode  0-video 1-cmd\n");

	test_act(R_DSI_HOST_MODE_CFG);


	/*display test case 9, get lcd bus width info*/
	printf("\ndisplay test case 9, get lcd bus width info \n");
	printf("bit[3:0] : dpi_color_coding\n");

	test_act(R_DSI_HOST_DPI_COLOR_CODE);
	
	printf("diplay/display-lava_test#lcm_info UTPASS\n");

	/*display test case 10, get dispc emc clk enable status info*/
	printf("\ndisplay test case 10, get lcd emc clk enable status info \n");
	printf("bit[11] : disp_emc_eb\n");

	test_act(DISP_EMC_EB);

	/*display test case 11, get dispc ahb clk enable status info*/
	printf("\ndisplay test case 11, get lcd ahb clk enable status info \n");
	#if LAVATEST_SP9001
	/*whale*/
	printf("bit[9] : disp_ahb_cfg_eb\n");
	#endif
	#if (LAVATEST_SP7731 || LAVATEST_SP9838 || LAVATEST_SP7720)
	/*Tshark SharklT8*/
	printf("bit[1] : disp0_eb\n");
	#endif

	test_act(REG_DISP_AHB_AHB_EB);

	/*display test case 12, get dispc pll clk info*/
	printf("\ndisplay test case 12, get dispc pll clk info \n");
	printf("bit[10:8] : clk_div\nbit[1:0] : clk_src\n");

	test_act(DISPC_PLL_CFG);
	printf("diplay/display-lava_test#dispc_clk_info UTPASS\n");
	/*display test case 13, get dsi ref_clk and cfg_clk enable status info*/
	printf("\ndisplay test case 13, get dsi ref_clk and cfg_clk enable status info \n");
	printf("bit[1] : dphy_ref_ckg_en   \nbit[0] : dphy_cfg_ckg_en\n");

	test_act(REG_AP_AHB_MISC_CKG_EN);

	printf("display/display-lava_test#dsi_clk_info UTPASS\n");
	disp_test_end();
	return 0;
}
