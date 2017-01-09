#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/param.h>

#include "bt_cfg.h"
#include "bt_rda.h"


__u32 RDA5876_PSKEY_MISC[][2] =
{

    {0x800004ec,0xf88dffff}, ///disable edr   
    {0x800004f0,0x83793b98}, ///disable 3m esco ev4 ev5
    
    //{0x80000060,0x000e1000},//baud 921600
    
    {0x80000070,0x00002000},
#ifdef EXTERNAL_32K
    {0x80000074,0xa5025010},
#else
    {0x80000074,0x05025010}, //0x05025010 for no sleep ; 0xa5025010 for sleep
#endif
    {0x80000078,0x0f054001},
    {0x8000007c,0xb530b530},

    {0x800000a4,0x08a19024}, //hostwake pull up for 10ms 
    {0x800000a8,0x0Bbaba30},

    {0x4024002c,0x00b81000},
    {0x80002be4,0x00010a02},
    {0x80000040,0x0600f000}, //PSKEY: modify flag
};




__u32 RDA5876_ENABLE_SPI[][2] =
{
    {0x40240000,0x2004f39c},                               
};


__u32 rdabt_rf_init_12[][2] = 
{   
    {0x0000003f,0x00000000},//
    {0x00000001,0x00001FFF},//                                                               
    {0x00000006,0x000007F7},//padrv_set,increase the power.                                  
    {0x00000008,0x000001E7},//                                                               
    {0x00000009,0x00000520},//                                                               
    {0x0000000B,0x000003DF},//filter_cap_tuning<3:0>1101                                     
    {0x0000000C,0x000085E8},//                                                               
    {0x0000000F,0x00000DBC},// 0FH,16'h1D8C; 0FH,16'h1DBC;adc_clk_sel=1 20110314 ;adc_digi_pw
    {0x00000012,0x000007F7},//padrv_set,increase the power.                                  
    {0x00000013,0x00000327},//agpio down pullen .                                            
    {0x00000014,0x00000CCC},//h0CFE; bbdac_cm 00=vdd/2.                                      
    {0x00000015,0x00000526},//Pll_bypass_ontch:1,improve ACPR.                               
    {0x00000016,0x00008918},//add div24 20101126                                             
    {0x00000018,0x00008800},//add div24 20101231                                             
    {0x00000019,0x000010C8},//pll_adcclk_en=1 20101126                                       
    {0x0000001A,0x00009128},//Mdll_adcclk_out_en=0                                           
    {0x0000001B,0x000080C0},//1BH,16'h80C2                                                   
    {0x0000001C,0x00003613},//                                                               
    {0x0000001D,0x000013E3},//Pll_cp_bit_tx<3:0>1110;13D3                                    
    {0x0000001E,0x0000300C},//Pll_lpf_gain_tx<1:0> 00;304C                                   
    {0x00000023,0x00002222},//                                                               
    {0x00000024,0x0000359F},//                                                               
    {0x00000027,0x00000011},//                                                               
    {0x00000028,0x0000124F},//                                                               
    {0x00000039,0x0000A5FC},//      
    {0x0000003f,0x00000001},//                                                        
    {0x00000000,0x0000043F},//agc                                                            
    {0x00000001,0x0000467F},//agc                                                            
    {0x00000002,0x000028FF},//agc//2011032382H,16'h68FF;agc                                  
    {0x00000003,0x000067FF},//agc                                                            
    {0x00000004,0x000057FF},//agc                                                            
    {0x00000005,0x00007BFF},//agc                                                            
    {0x00000006,0x00003FFF},//agc                                                            
    {0x00000007,0x00007FFF},//agc                                                            
    {0x00000018,0x0000F3F5},//                                                               
    {0x00000019,0x0000F3F5},//                                                               
    {0x0000001A,0x0000E7F3},//                                                               
    {0x0000001B,0x0000F1FF},//                                                               
    {0x0000001C,0x0000FFFF},//                                                               
    {0x0000001D,0x0000FFFF},//                                                               
    {0x0000001E,0x0000FFFF},//                                                               
    {0x0000001F,0x0000FFFF},//padrv_gain;9FH,16'hFFEC;padrv_gain20101103;improve ACPR;       
#ifdef EXTERNAL_32K
    {0x00000023,0x00004224},//;ext32k
#endif
    {0x00000024,0x00000110},
    {0x00000025,0x000043E1},//ldo_vbit:110,1.96v                                             
    {0x00000026,0x00004BB5},//reg_ibit:101,reg_vbit:110,1.12v,reg_vbit_deepsleep:110,750mV   
    {0x00000032,0x00000079},//TM mod                                                         
    {0x0000003f,0x00000000},//
};

__u32 RDA5876_PSKEY_RF[][2] =
{
    {0x40240000,0x2004f39c}, //houzhen 2010.02.07 for rda5990 bt
    {0x800000C0,0x00000021},
    {0x800000C4,0x003F0000},
    {0x800000C8,0x00414003},
    {0x800000CC,0x004225BD},
    {0x800000D0,0x004908E4},
    {0x800000D4,0x0043B074},
    {0x800000D8,0x0044D01A},
    {0x800000DC,0x004A0800},
    {0x800000E0,0x0054A020},
    {0x800000E4,0x0055A020},
    {0x800000E8,0x0056A542},
    {0x800000EC,0x00574C18},
    {0x800000F0,0x003F0001},
    {0x800000F4,0x00410900},
    {0x800000F8,0x0046033F},
    {0x800000FC,0x004C0000},
    {0x80000100,0x004D0015},
    {0x80000104,0x004E002B},
    {0x80000108,0x004F0042},
    {0x8000010C,0x0050005A},
    {0x80000110,0x00510073},
    {0x80000114,0x0052008D},
    {0x80000118,0x005300A7},
    {0x8000011C,0x005400C4},
    {0x80000120,0x005500E3},
    {0x80000124,0x00560103},
    {0x80000128,0x00570127},
    {0x8000012C,0x0058014E},
    {0x80000130,0x00590178},
    {0x80000134,0x005A01A1},
    {0x80000138,0x005B01CE},
    {0x8000013C,0x005C01FF},
    {0x80000140,0x003F0000},
    {0x80000144,0x00000000}, //;         PSKEY: Page 0
    {0x80000040,0x10000000},
};

__u32 RDA5876_DCCAL[][2]=
{
    {0x0000003f,0x00000000},
    {0x00000030,0x00000129},
    {0x00000030,0x0000012B},
};

__u32 RDA5876_DISABLE_SPI[][2] = 
{
	{0x40240000,0x2000f29c},
};

__u32 RDA5876_TRAP[][2] = 
{
    {0x40180100,0x000068b8},//inc power
    {0x40180120,0x000069f4},

    {0x40180104,0x000066b8},//dec power
    {0x40180124,0x000069f4},

    {0x40180108,0x0001544c},//esco w
    {0x40180128,0x0001568c},

    {0x80000100,0xe3a0700f}, ///2ev3 ev3 hv3
    {0x4018010c,0x0000bae8},//esco packet
    {0x4018012c,0x80000100},


    //{0x40180110,0x000247a0},//sleep patch
    //{0x40180130,0x0002475c},
    {0x40180114,0x0000f8c4},///all rxon
    {0x40180134,0x00026948},

    {0x40180118,0x000130b8},///qos PRH_CHN_QUALITY_MIN_NUM_PACKETS
    {0x40180138,0x0001cbb4},

    {0x4018011c,0x0000bac8},
    {0x4018013c,0x0000bae4},

    {0x40180000,0x0000ff00},  
};


int rda5876_setup_flow_ctl(int fd)
{
	unsigned int i, num_send;

	unsigned char rda_flow_ctl[][14] =
	{
		{0x01,0x02,0xfd,0x0a,0x00,0x01,0x44,0x00,0x20,0x40,0x3c,0x00,0x00,0x00},                                                                  
		{0x01,0x02,0xfd,0x0a,0x00,0x01,0x10,0x00,0x00,0x50,0x22,0x01,0x00,0x00},// flow control
	};

  ERR_PRINT("[######FLOW-CTRL#######] rda_setup_flow_ctl.\n");
  
  /*Setup flow control */
	for (i = 0; i < sizeof(rda_flow_ctl)/sizeof(rda_flow_ctl[0]); i++)
	{
		num_send = write(fd, rda_flow_ctl[i], sizeof(rda_flow_ctl[i]));
		
		if (num_send != sizeof(rda_flow_ctl[i]))
		{
			perror("");
			printf("num_send = %d (%d)\n", num_send, sizeof(rda_flow_ctl[i]));
			return -1;
		}
		
		usleep(5000);
	}

	usleep(50000);
        
	return 0;
}

void rda5876_change_baudrate(int fd)
{

__u32 RDA5876_SWITCH_BAUDRATE[][2] =
{
//default baud rate 115200
    {0x80000060,0x0001c200},
    {0x80000040,0x00000100}   
};
    RDA5876_SWITCH_BAUDRATE[0][1] = rda_bt_baudrate_map(CUST_BT_SERIAL_BOURATE);
    RDA_uart_write_array(fd, RDA5876_SWITCH_BAUDRATE ,sizeof(RDA5876_SWITCH_BAUDRATE)/sizeof(RDA5876_SWITCH_BAUDRATE[0]),0);	
}



void RDA5876_RfInit(int fd)
{
    RDA_uart_write_array(fd,RDA5876_ENABLE_SPI,sizeof(RDA5876_ENABLE_SPI)/sizeof(RDA5876_ENABLE_SPI[0]),0);
    RDA_uart_write_array(fd,rdabt_rf_init_12,sizeof(rdabt_rf_init_12)/sizeof(rdabt_rf_init_12[0]),1);
    usleep(50000);//50ms?
}


void RDA5876_Pskey_RfInit(int fd)
{
    RDA_uart_write_array(fd,RDA5876_PSKEY_RF,sizeof(RDA5876_PSKEY_RF)/sizeof(RDA5876_PSKEY_RF[0]),0);
}

void RDA5876_Dccal(int fd)
{
    RDA_uart_write_array(fd,RDA5876_DCCAL,sizeof(RDA5876_DCCAL)/sizeof(RDA5876_DCCAL[0]),1);
    RDA_uart_write_array(fd,RDA5876_DISABLE_SPI,sizeof(RDA5876_DISABLE_SPI)/sizeof(RDA5876_DISABLE_SPI[0]),0);
}


void RDA5876_Trap(int fd)
{
    RDA_uart_write_array(fd,RDA5876_TRAP,sizeof(RDA5876_TRAP)/sizeof(RDA5876_TRAP[0]),0);
}


void RDA5876_Pskey_Misc(int fd)
{
    RDA_uart_write_array(fd,RDA5876_PSKEY_MISC,sizeof(RDA5876_PSKEY_MISC)/sizeof(RDA5876_PSKEY_MISC[0]),0);

}

