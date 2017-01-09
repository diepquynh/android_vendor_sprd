#ifndef AWB_H_
#define AWB_H_

#ifdef WIN32
#include "sci_types.h"
#else 
#include <linux/types.h>
#include <sys/types.h>
#include <utils/Log.h>
#endif

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif



struct awb_stat_img 
{
    uint32_t *r;
    uint32_t *g;
    uint32_t *b;	
};

struct awb_rgb_gain
{
    uint32_t r_gain;
    uint32_t g_gain;
    uint32_t b_gain;
    
    uint32_t ct;
    int32_t pg; //0 neutral, -: green, +: purple
    int32_t green100;
};

enum
{
	WB_AUTO,
	WB_CLOUDY_DAYLIGHT,
	WB_DAYLIGHT,
	WB_FLUORESCENT,
	WB_A,
	WB_SHADE,
	WB_TWILIGHT,
	WB_WARM_FLUORESCENT,
	WB_CANDLE,
};

struct awbParaGenIn
{
    int version;
    
    
    // AWB OTP
    unsigned int otp_golden_r;
    unsigned int otp_golden_g;
    unsigned int otp_golden_b;
    unsigned int otp_random_r;
    unsigned int otp_random_g;
    unsigned int otp_random_b;
    
    
    // graychart under neutral light
    int grayNum;     /* [3, 10] */
    int grayCt[10];
    double grayRgb[10][3];
    
    // graychart under CWF
    double cwfRgb[3];
    
    // colorchecker under D65
    double colorChecker24[24][3];
    
    
    
    // weight table
    int wTabSz;
    double wTabEv[4];
    double wTabData[4][6]; // start, mid, mid2, end, peak, dc
    
    
    // BalanceRange
    double ctShiftBv[4];
    
    double null_data1[4][2];
    
    double null_data2[4];
    double null_data3[4];
    
    int ctShiftTickNum[4];
    float ctShiftTick[4][64];
    float ctShiftValTick[4][64];
    float pgrShiftTick[4][64];
    
    //purple
    short purpleBvNum;
    float purpleBv[5];
    short purpleNodeNum[5];
    float purpleUpPgratioOri[5];
    float purpleDnPgratioOri[5];
    short purpleCt[5][10];
    short purpleUpCt[5][10];
    float purpleUpPgRatio[5][10];
    short purpleDnCt[5][10];
    float purpleDnPgRatio[5][10];
    
    
    // Green
    double grassYellowLevel;
    double grassGreenLevel;
    
    short pgBalanceBvNum;
    float pgBalanceBv[5];
    short pgBalanceNodeNum[5];
    short pgBalanceCt[5][10];
    float artificialGreenRatio[5][10];
    float purpleBalanceRatio[5][10];
    float purpleBalanceTransRatio[5][10];
    
    //boundary
    short limNum;
    short limMode;
    float limBv[4];
    float limCtRt[4];
    float limCtLt[4];
    float limPgRatioUp[4];
    float limPgRatioDn[4];
    
    float limCtRtTrans[4];
    float limCtLtTrans[4];
    float limPgUpTrans[4];
    float limPgDnTrans[4];
    
    float limPgRatioUp2[4];
    float limPgRatioUp3[4];
    float limPgRatioDn2[4];
    float limPgRatioDn3[4];
    float limCtMid[4];
    float limDefaultCt[4];
    float limDefaultPgRatio[4];
    
    //awbMode
    int modeNum;
    int modeId[10];
    double modeCt[10];
    double modePgr[10];

	
    int ctShiftNum;
	short ctShiftCt[10];
	float ctShiftPgRatio[10];
	short ctShiftNeutral[10];
	short ctShiftDown[10];
};

struct awb_tuning_param
{
    int magic;
    int version;
    int date;
    int time;
    
    /* MWB table */
    unsigned char wbModeNum;
    unsigned char wbModeId[10];
    struct awb_rgb_gain wbMode_gain[10];
    struct awb_rgb_gain mwb_gain[101];   // mwb_gain[0] is init_gain  
    
    /* AWB parameter */
    int rgb_12bit_sat_value;
    
    int pg_x_ratio;
    int pg_y_ratio;
    int pg_shift;
    
    int isotherm_ratio;
    int isotherm_shift;
    
    int rmeanCali;
    int gmeanCali;
    int bmeanCali;
    
    int dct_scale_65536; //65536=1x 
    
    short pgBalanceBvNum;
    short pgBalanceBv[5];  //1024
    short pg_center[512];
    short pg_up[5][512];
    short pg_up_trans[5][512];
    short pg_dn[5][512];
    
    short cct_tab[512];
    short cct_tab_div;
    short cct_tab_sz;
    int cct_tab_a[40];
    int cct_tab_b[40];
    
    int dct_start;
    int pg_start;
    
    int dct_div1;
    int dct_div2;
    
    int w_lv_len;
    short w_lv[6];
    short w_lv_tab[6][512];
    
    
    int dct_sh_bv[4];
    short dct_sh[4][512];
    short dct_pg_sh100[4][512];
    short null_data1[4][2];
    short null_data2[4];
    short null_data3[4];
    short null_data4[4];
    short null_data5[4];
    
    int red_num;
    short red_x[10];
    short red_y[10];
    
    int blue_num;
    short blue_x[10];
    short blue_comp[10];
    short blue_sh[10];
    
    short ctCompX1;
    short ctCompX2;
    short ctCompY1;
    short ctCompY2;
    
    short defnum;
    short defev1024[4];
    short defx[9][4];
    short defxDev[4][4];
    short defrange[4][4];
    short defxy[2][4];
    
    
    
    
    short grassGreenLevel;//1024;
    
    //purple
    short purpleBvNum;
    short purpleBv[5];
    short purpleNodeNum[5];
    short purpleUpPgOri[5];
    short purpleDnPgOri[5];
    short purpleCt[5][10];
    short purpleUpCt[5][10];
    short purpleUpPg[5][10];
    short purpleDnCt[5][10];
    short purpleDnPg[5][10];
    
    int cwfPgAbs100;
    
    unsigned char ui_data[sizeof(struct awbParaGenIn)];
    

    int reserved[(8204-sizeof(struct awbParaGenIn))/4];


    // awb control param
    unsigned int skip_frame_num;
    unsigned int calc_interval_num;
    unsigned int smooth_buffer_num;
    

    int check;
};




struct awb_init_param 
{
    uint32_t stat_img_w; // 32
    uint32_t stat_img_h; // 32
    uint32_t r_pix_cnt;
    uint32_t g_pix_cnt;
    uint32_t b_pix_cnt;
    
    uint32_t otp_random_r;
    uint32_t otp_random_g;
    uint32_t otp_random_b;
    uint32_t otp_golden_r;
    uint32_t otp_golden_g;
    uint32_t otp_golden_b;
    
    struct awb_tuning_param tuning_param;
};


struct awb_calc_param
{
    struct awb_stat_img stat_img;
    int bv;
    int iso;
    int date; //ex: 20160331
    int time; //ex: 221059
    
    
    // just for simulation
    int matrix[9];
    unsigned char gamma[256];
};

struct awb_calc_result
{
    struct awb_rgb_gain awb_gain[10];
    
    uint8_t* log_buffer;
    uint32_t log_size;
};


enum 
{
    AWB_IOCTRL_SET_LSCINFO = 1,
    AWB_IOCTRL_CMD_MAX,
};

struct awb_lsc_info
{
    /*
    value: 
    index of light
    0 dnp
    1 a
    2 tl84
    3 d65 
    4 cwf
    */
    uint16_t value[2];
    uint16_t weight[2];
};

/*------------------------------------------------------------------------------*
*				Function Prototype					*
*-------------------------------------------------------------------------------*/
void* awb_init_v1(struct awb_init_param* init_param, struct awb_rgb_gain* gain);

int awb_calc_v1(void* awb_handle, struct awb_calc_param* calc_param, struct awb_calc_result* calc_result);

int awb_ioctrl_v1(void* awb_handle, int cmd, void* param);

int awb_deinit_v1(void* awb_handle);


/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End
