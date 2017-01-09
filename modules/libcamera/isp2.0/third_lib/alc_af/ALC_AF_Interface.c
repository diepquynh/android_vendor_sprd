/* --------------------------------------------------------------------------
 * Confidential
 *                    AcuteLogic Technologies Inc.
 *
 *                   Copyright (c) 2015 ALC Limited
 *                          All rights reserved
 *
 * Project  : SPRD_AF
 * Filename : ALC_AF_Interface.c
 *
 * $Revision: v1.00
 * $Author 	:
 * $Date 	: 2015.02.11
 *
 * --------------------------------------------------------------------------
 * Feature 	: AP TShark2
 * Note 	: vim(ts=4, sw=4)
 * ------------------------------------------------------------------------ */
#ifndef	__ALC_AF_INTERFACE_C__
#define	__ALC_AF_INTERFACE_C__
#define FILTER_SPRD //for test

 //for test
#define ALC_AF_DEBUG_STR     "ALC_AF: %d, %s: "
#define ALC_AF_DEBUG_ARGS    __LINE__,__FUNCTION__
#define ALC_AF_LOG(format,...) ALOGE(ALC_AF_DEBUG_STR format, ALC_AF_DEBUG_ARGS, ##__VA_ARGS__) 
#define ALC_AF_LOGI(format,...) ALOGI(ALC_AF_DEBUG_STR format, ALC_AF_DEBUG_ARGS, ##__VA_ARGS__)

#define ALC_AE_AWB_LOCK		0 //AE/AWB LOCK allways
#define ALC_AE_DIVISION		32 //AE Blk division number

/* ------------------------------------------------------------------------ */
#include "ALC_AF_Lib.h"
#include "ALC_AF_Ctrl.h"
#include "stdio.h"
#include "malloc.h"
#include "string.h"
#include <utils/Log.h> //for test
/* ------------------------------------------------------------------------ */

void Al_AF_GetFv(alc_af_handle_t handle,unsigned long dwValue[150])
{
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;

	struct alc_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;

	unsigned char i,j,k;

	af_ops->cb_get_afm_type1_statistic(handle, &dwValue[0]);
	af_ops->cb_get_afm_type2_statistic(handle, &dwValue[25]);

	af_ops->cb_get_sp_afm_statistic(handle, &dwValue[50]);
//ALC_AF_LOGI("ALC_posdw50 %d : %d %d %d %d %d %d %d",dwValue[50],dwValue[51],dwValue[52],dwValue[53],dwValue[54],dwValue[55],dwValue[56]);

	for(i=0;i<10;i++){
		dwValue[0+i] = dwValue[0+i];// RGB
		dwValue[10+i] = dwValue[25+i];// RGB
	}
	for(i=0;i<10;i++){
		dwValue[20+i] = dwValue[50+i];// YIQ LAPLACE
		dwValue[30+i] = dwValue[75+i];// YIQ SOBEL
	}
	for(i=0;i<10;i++){
		dwValue[40+i] = dwValue[100+i];// YIQ IIR1
		dwValue[50+i] = dwValue[125+i];// YIQ IIR2
	}
//ALC_AF_LOGI("ALC_posdw %d %d %d %d %d %d %d %d %d %d",dwValue[20],dwValue[21],dwValue[22],dwValue[23],dwValue[24],dwValue[25],dwValue[26],dwValue[27],dwValue[28],dwValue[29]);


	return 0;
}


void Al_FocusDataGet(TT_AfIfBuf *ppAfIfBuf , uint32_t *statis , uint32_t *ae_lumi)//tes_kano_0821
{
	if(ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode == 8){ //tes_kano_0824 for tap af
		//YIQ IIR2
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue0 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue1 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue2 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue3 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue4 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue5 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue6 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue7 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue8 = statis[59] ;
		
		//tes_kano_0904 data2:YIQ LAPLACE
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue0 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue1 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue2 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue3 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue4 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue5 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue6 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue7 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue8 = statis[29] ;
	}
	else if(ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode == 16){ //tes_kano_0825 for face af
		//YIQ IIR2
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue0 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue1 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue2 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue3 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue4 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue5 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue6 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue7 = statis[59] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue8 = statis[59] ;
		
		//tes_kano_0904 data2:YIQ LAPLACE
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue0 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue1 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue2 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue3 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue4 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue5 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue6 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue7 = statis[29] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue8 = statis[29] ;
	}
	else{
		//YIQ IIR2
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue0 = statis[50] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue1 = statis[51] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue2 = statis[52] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue3 = statis[53] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue4 = statis[54] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue5 = statis[55] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue6 = statis[56] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue7 = statis[57] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContL.muiAfEvaluationValue8 = statis[58] ;
		
		//tes_kano_0904 data2:YIQ LAPLACE
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue0 = statis[20] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue1 = statis[21] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue2 = statis[22] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue3 = statis[23] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue4 = statis[24] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue5 = statis[25] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue6 = statis[26] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue7 = statis[27] ;
		ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttContH.muiAfEvaluationValue8 = statis[28] ;
	}

	ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttLuminance.muiAfEvaluationValue0 = ae_lumi[0];
	ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttLuminance.muiAfEvaluationValue1 = ae_lumi[1];
	ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttLuminance.muiAfEvaluationValue2 = ae_lumi[2];
	ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttLuminance.muiAfEvaluationValue3 = ae_lumi[3];
	ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttLuminance.muiAfEvaluationValue4 = ae_lumi[4];
	ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttLuminance.muiAfEvaluationValue5 = ae_lumi[5];
	ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttLuminance.muiAfEvaluationValue6 = ae_lumi[6];
	ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttLuminance.muiAfEvaluationValue7 = ae_lumi[7];
	ppAfIfBuf->mttInfo.mttAfEvaluationValue.mttLuminance.muiAfEvaluationValue8 = ae_lumi[8];
}

UI_32 Al_GetAfY(UI_32 sensor_w , UI_32 sensor_h , UI_16 sx , UI_16 sy , UI_16 ex , UI_16 ey , UI_32 *r_dat , UI_32 *g_dat , UI_32 *b_dat)
{
	UI_16 a = 0 ;
	UI_16 b = 0 ;
	UI_16 c = 0 ;
	UI_16 d = 0 ;
	UI_16 e = 0 ;
	UI_16 f = 0 ;
	UI_16 g = 0 ;
	UI_32 h = 0 ;
	UI_32 i = 0 ;
	FT_32 j = 0 ;
	FT_32 k = 0.299 ;
	FT_32 l = 0.587 ;
	FT_32 m = 0.114 ;
	UI_32 n = 0 ;
	UI_32 r_sum = 0 ;
	UI_32 g_sum = 0 ;
	UI_32 b_sum = 0 ;
	UI_32 y_sum = 0 ;
	UI_32 bw = 0 ;
	UI_32 bh = 0 ;
	UI_32 w_oft = 0 ;
	UI_32 h_oft = 0 ;
	
	bw = sensor_w / ALC_AE_DIVISION ;
	bh = sensor_h / ALC_AE_DIVISION ;
	
	w_oft = (sensor_w - (bw * ALC_AE_DIVISION)) / 2 ;
	h_oft = (sensor_h - (bh * ALC_AE_DIVISION)) / 2 ;

//	UI_16 x = 0 ; //for debug
//	x = ppAfIfBuf->mttAcoAf.vd_cnt % 1200 ;
	
	a = (sy - h_oft) / bh ;
	b = (ey - h_oft + 1) / bh ;
	c = (sx - w_oft) / bw ;
	d = (ex - w_oft + 1) / bw ;

	for(e=a ; e<b ; e++)
	{
		for(f=c ; f<d ; f++)
		{
			g = e * ALC_AE_DIVISION + f ;
			r_sum = r_sum + r_dat[g] ;
			g_sum = g_sum + g_dat[g] ;
			b_sum = b_sum + b_dat[g] ;
		}
	}
	
	h = (UI_32) (ex - sx + 1) * (ey - sy + 1) ;
	i = (UI_32) ((b - a) * bh) * ((d - c) * bw) ;
	if(i==0) i = 1 ;
	j = (FT_32) h / i ;
	y_sum = (UI_32)(( (k * r_sum) + (l * g_sum) + (m * b_sum) ) * j) ;
	n = h >> 10 ; //9:y=27000,10:51500
	if(n > 0) y_sum = y_sum / n ;
	
	return y_sum ; 
}

void Al_GetAfRegion(TT_AfIfBuf* ppAfIfBuf , UI_16* sx , UI_16* sy, UI_16* ex , UI_16* ey , UI_08 i)
{
	switch(i){
		case 0 :
			*sx = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiHpos ;
			*sy = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiVpos ;
			*ex = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiHpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiHsize - 1 ;
			*ey = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiVpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiVsize - 1 ;
			break ;
		case 1 :
			*sx = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiHpos ;
			*sy = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiVpos ;
			*ex = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiHpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiHsize - 1 ;
			*ey = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiVpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiVsize - 1 ;
			break ;
		case 2 :
			*sx = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiHpos ;
			*sy = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiVpos ;
			*ex = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiHpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiHsize - 1 ;
			*ey = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiVpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiVsize - 1 ;
			break ;
		case 3 :
			*sx = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiHpos ;
			*sy = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiVpos ;
			*ex = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiHpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiHsize - 1 ;
			*ey = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiVpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiVsize - 1 ;
			break ;
		case 4 :
			*sx = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiHpos ;
			*sy = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiVpos ;
			*ex = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiHpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiHsize - 1 ;
			*ey = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiVpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiVsize - 1 ;
			break ;
		case 5 :
			*sx = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiHpos ;
			*sy = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiVpos ;
			*ex = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiHpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiHsize - 1 ;
			*ey = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiVpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiVsize - 1 ;
			break ;
		case 6 :
			*sx = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiHpos ;
			*sy = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiVpos ;
			*ex = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiHpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiHsize - 1 ;
			*ey = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiVpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiVsize - 1 ;
			break ;
		case 7 :
			*sx = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiHpos ;
			*sy = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiVpos ;
			*ex = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiHpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiHsize - 1 ;
			*ey = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiVpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiVsize - 1 ;
			break ;
		case 8 :
			*sx = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiHpos ;
			*sy = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiVpos ;
			*ex = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiHpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiHsize - 1 ;
			*ey = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiVpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiVsize - 1 ;
			break ;
		default :
			*sx = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiHpos ;
			*sy = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiVpos ;
			*ex = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiHpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiHsize - 1 ;
			*ey = ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiVpos + ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiVsize - 1 ;
			break ;
	}
}

void Al_af_area_set(TT_AfIfBuf *ppAfIfBuf , struct alc_win_coord *region) //tes_kano_0820
{
	if(ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode == 8){ //tes_kano_0824 for tap af
		ppAfIfBuf->mttInfo.mttAfWindow.mttAfSingleSpotRoi.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.mttAfSingleSpotRoi.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.mttAfSingleSpotRoi.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.mttAfSingleSpotRoi.muiVsize = region[9].end_y - region[9].start_y + 1;
		
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiVsize = region[9].end_y - region[9].start_y + 1 ;
	}
	else if(ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode == 16){ //tes_kano_0825 for face af
		ppAfIfBuf->mttInfo.mttAfWindow.mttAfSingleSpotRoi.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.mttAfSingleSpotRoi.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.mttAfSingleSpotRoi.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.mttAfSingleSpotRoi.muiVsize = region[9].end_y - region[9].start_y + 1;
		
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiVsize = region[9].end_y - region[9].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiHpos = region[9].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiVpos = region[9].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiHsize = region[9].end_x - region[9].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiVsize = region[9].end_y - region[9].start_y + 1 ;
	}
	else{
		ppAfIfBuf->mttInfo.mttAfWindow.mttAfSingleSpotRoi.muiHpos = region[4].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.mttAfSingleSpotRoi.muiVpos = region[4].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.mttAfSingleSpotRoi.muiHsize = region[4].end_x - region[0].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.mttAfSingleSpotRoi.muiVsize = region[4].end_y - region[0].start_y + 1;
		
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiHpos = region[0].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiVpos = region[0].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiHsize = region[0].end_x - region[0].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti0.muiVsize = region[0].end_y - region[0].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiHpos = region[1].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiVpos = region[1].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiHsize = region[1].end_x - region[1].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti1.muiVsize = region[1].end_y - region[1].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiHpos = region[2].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiVpos = region[2].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiHsize = region[2].end_x - region[2].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti2.muiVsize = region[2].end_y - region[2].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiHpos = region[3].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiVpos = region[3].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiHsize = region[3].end_x - region[3].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti3.muiVsize = region[3].end_y - region[3].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiHpos = region[4].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiVpos = region[4].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiHsize = region[4].end_x - region[4].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti4.muiVsize = region[4].end_y - region[4].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiHpos = region[5].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiVpos = region[5].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiHsize = region[5].end_x - region[5].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti5.muiVsize = region[5].end_y - region[5].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiHpos = region[6].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiVpos = region[6].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiHsize = region[6].end_x - region[6].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti6.muiVsize = region[6].end_y - region[6].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiHpos = region[7].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiVpos = region[7].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiHsize = region[7].end_x - region[7].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti7.muiVsize = region[7].end_y - region[7].start_y + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiHpos = region[8].start_x ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiVpos = region[8].start_y ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiHsize = region[8].end_x - region[8].start_x + 1 ;
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfMult3x3Roi.muiAfWindowMulti8.muiVsize = region[8].end_y - region[8].start_y + 1 ;
	}
}

void Al_SetAfMultiArea(UI_32 sensor_w , UI_32 sensor_h , UI_32 *sx , UI_32 *sy , UI_32 *ex , UI_32 *ey , UI_08 n)
{
	UI_32 cw = 0 ;
	UI_32 ch = 0 ;
	UI_32 bw = 0 ;
	UI_32 bh = 0 ;
	
	cw = sensor_w / 2 ;
	ch = sensor_h / 2 ;
	
	bw = sensor_w / ALC_AE_DIVISION ;
	bh = sensor_h / ALC_AE_DIVISION ;
	
	switch(n){
		case 0 :
			*sx =  cw - (bw * 9) ;
			*sy =  ch - (bh * 9) ;
			break ;
		case 1 :
			*sx =  cw - (bw * 3) ;
			*sy =  ch - (bh * 9) ;
			break ;
		case 2 :
			*sx =  cw + (bw * 3) ;
			*sy =  ch - (bh * 9) ;
			break ;
		case 3 :
			*sx =  cw - (bw * 9) ;
			*sy =  ch - (bh * 3) ;
			break ;
		case 4 :
			*sx =  cw - (bw * 3) ;
			*sy =  ch - (bh * 3) ;
			break ;
		case 5 :
			*sx =  cw + (bw * 3) ;
			*sy =  ch - (bh * 3) ;
			break ;
		case 6 :
			*sx =  cw - (bw * 9) ;
			*sy =  ch + (bh * 3) ;
			break ;
		case 7 :
			*sx =  cw - (bw * 3) ;
			*sy =  ch + (bh * 3) ;
			break ;
		case 8 :
			*sx =  cw + (bw * 3) ;
			*sy =  ch + (bh * 3) ;
			break ;
		default :
			*sx =  cw - (bw * 3) ;
			*sy =  ch - (bh * 3) ;
			break ;
	}
	*ex =  *sx + bw * 6 - 1 ;
	*ey =  *sy + bh * 6 - 1 ;
}

void Al_AF_SetIirCoef(alc_af_handle_t handle , TT_AfIfBuf *ppAfIfBuf)
{
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	
#if 0
		if(ppAfIfBuf->mttAcoAf.hpf_sw != 1){
			ppAfIfBuf->mttAcoAf.hpf_sw = 1 ;
			//LPF default
			af_cxt->sprd_filter.IIR_c[ 0] 		=  16; //20150822 Change
			af_cxt->sprd_filter.IIR_c[ 1] 		=  33;
			af_cxt->sprd_filter.IIR_c[ 2] 		= -14;
			af_cxt->sprd_filter.IIR_c[ 3] 		=  16; //20150822 Change
			af_cxt->sprd_filter.IIR_c[ 4] 		=  32; //20150822 Change
			af_cxt->sprd_filter.IIR_c[ 5] 		=  16; //20150822 Change
			//F Sample2
			af_cxt->sprd_filter.IIR_c[ 6] 		=  114;// 1.7813 * 64 =  114
			af_cxt->sprd_filter.IIR_c[ 7] 		=  -51;//-0.8008 * 64 =  -51
			af_cxt->sprd_filter.IIR_c[ 8] 		=   64;// 1.0000 * 64 =   64
			af_cxt->sprd_filter.IIR_c[ 9] 		= -128;//-2.0000 * 64 = -128
			af_cxt->sprd_filter.IIR_c[10] 		=   64;// 1.0000 * 64 =   64
		}
#else
	if( ppAfIfBuf->mttAdjust.mttFilterControl.msiFixedFilter > 0 ){
		if(ppAfIfBuf->mttInfo.mttAfCameraInfo.muiAfSv < 1820){ //over 10lux
			if(ppAfIfBuf->mttAcoAf.hpf_sw != 1){
				ppAfIfBuf->mttAcoAf.hpf_sw = 1 ;
				//LPF default
				af_cxt->sprd_filter.IIR_c[ 0] 		=  16; //20150822 Change
				af_cxt->sprd_filter.IIR_c[ 1] 		=  33;
				af_cxt->sprd_filter.IIR_c[ 2] 		= -14;
				af_cxt->sprd_filter.IIR_c[ 3] 		=  16; //20150822 Change
				af_cxt->sprd_filter.IIR_c[ 4] 		=  32; //20150822 Change
				af_cxt->sprd_filter.IIR_c[ 5] 		=  16; //20150822 Change
				//F Sample2
				af_cxt->sprd_filter.IIR_c[ 6] 		=  114;// 1.7813 * 64 =  114
				af_cxt->sprd_filter.IIR_c[ 7] 		=  -51;//-0.8008 * 64 =  -51
				af_cxt->sprd_filter.IIR_c[ 8] 		=   64;// 1.0000 * 64 =   64
				af_cxt->sprd_filter.IIR_c[ 9] 		= -128;//-2.0000 * 64 = -128
				af_cxt->sprd_filter.IIR_c[10] 		=   64;// 1.0000 * 64 =   64
				
				if(ppAfIfBuf->mttAdjust.mttFilterControl.msiFixedFilter==1) ppAfIfBuf->mttSinAf.af_wait = 1 ;
			}
		}
		else{
			if(ppAfIfBuf->mttAcoAf.hpf_sw != 2){ //under 10lux
				ppAfIfBuf->mttAcoAf.hpf_sw = 2 ;
				//LPF default
				af_cxt->sprd_filter.IIR_c[ 0] 		=  16; //20150822 Change
				af_cxt->sprd_filter.IIR_c[ 1] 		=  33;
				af_cxt->sprd_filter.IIR_c[ 2] 		= -14;
				af_cxt->sprd_filter.IIR_c[ 3] 		=  16; //20150822 Change
				af_cxt->sprd_filter.IIR_c[ 4] 		=  32; //20150822 Change
				af_cxt->sprd_filter.IIR_c[ 5] 		=  16; //20150822 Change
				//fc=0.1
				af_cxt->sprd_filter.IIR_c[ 6] 		=  73;//-1.1430 * 64 = -73
				af_cxt->sprd_filter.IIR_c[ 7] 		= -26;// 0.4128 * 64 =  26
				af_cxt->sprd_filter.IIR_c[ 8] 		=  41;// 0.6389 * 64 =  41
				af_cxt->sprd_filter.IIR_c[ 9] 		= -82;//-1.2779 * 64 = -82
				af_cxt->sprd_filter.IIR_c[10] 		=  41;// 0.6389 * 64 =  41
				
				if(ppAfIfBuf->mttAdjust.mttFilterControl.msiFixedFilter==1) ppAfIfBuf->mttSinAf.af_wait = 1 ;
			}
		}
		ppAfIfBuf->mttAdjust.mttFilterControl.msiFixedFilter = 0 ;
	}
	else{
		if(ppAfIfBuf->mttAcoAf.hpf_sw != 1){
			ppAfIfBuf->mttAcoAf.hpf_sw = 1 ;
			//LPF default
			af_cxt->sprd_filter.IIR_c[ 0] 		=  16; //20150822 Change
			af_cxt->sprd_filter.IIR_c[ 1] 		=  33;
			af_cxt->sprd_filter.IIR_c[ 2] 		= -14;
			af_cxt->sprd_filter.IIR_c[ 3] 		=  16; //20150822 Change
			af_cxt->sprd_filter.IIR_c[ 4] 		=  32; //20150822 Change
			af_cxt->sprd_filter.IIR_c[ 5] 		=  16; //20150822 Change
			//F Sample2
			af_cxt->sprd_filter.IIR_c[ 6] 		=  114;// 1.7813 * 64 =  114
			af_cxt->sprd_filter.IIR_c[ 7] 		=  -51;//-0.8008 * 64 =  -51
			af_cxt->sprd_filter.IIR_c[ 8] 		=   64;// 1.0000 * 64 =   64
			af_cxt->sprd_filter.IIR_c[ 9] 		= -128;//-2.0000 * 64 = -128
			af_cxt->sprd_filter.IIR_c[10] 		=   64;// 1.0000 * 64 =   64
		}
	}
#endif
}

void Al_AF_Prv_Start_Notice(alc_af_handle_t handle , TT_AfIfBuf *ppAfIfBuf)
{
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	struct alc_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;
	struct alc_win_coord region[25];
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t k = 0;
	uint32_t h1 = 0 ;
	uint32_t h2 = 0 ;
	uint32_t v1 = 0 ;
	uint32_t v2 = 0 ;
	uint32_t face_max_size=0;
	uint32_t face_max_size_temp=0;
	uint32_t face_sx=0;
	uint32_t face_sy=0;
	uint32_t face_ex=0;
	uint32_t face_ey=0;
	uint32_t saf_init=0;
	
	af_ops->cb_set_afm_bypass(handle, 1);	//Turn Off
	ALC_AF_LOGE("DBG_INTERFACE Debug_Start");
	af_ops->cb_set_afm_mode(handle, 1); 	// 1:Continue, 0:single
	//af_ops->cb_set_afm_bypass(handle, 0);	//Turn On

	ALC_AF_LOGE("DBG_INTERFACE Start_Preview");

	if(ppAfIfBuf->mttAcoAf.init == 0){
		af_cxt->sprd_filter.bypass 		= 0; //0;On, 1:Off
		af_cxt->sprd_filter.mode		= 1;
		af_cxt->sprd_filter.source_pos		= 1;
		af_cxt->sprd_filter.shift		= 0;
		af_cxt->sprd_filter.skip_num		= 0;
		af_cxt->sprd_filter.skip_num_clear	= 0;
		af_cxt->sprd_filter.format		=7;
		af_cxt->sprd_filter.iir_bypass		= 1;
	}
	Al_AF_SetIirCoef(handle , ppAfIfBuf);

	af_ops->cb_set_sp_afm_cfg(handle);
	memset(region,0,sizeof(region));

	for (i=0;i<9;i++)
	{
		Al_SetAfMultiArea(af_cxt->sensor_w , af_cxt->sensor_h , &region[i].start_x , &region[i].start_y , &region[i].end_x , &region[i].end_y , i);
	}

	if (ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode == 8 ){ //tes_kano_0825 for tap af
		// for touch
		if((af_cxt->win_pos[0].end_x > af_cxt->win_pos[0].start_x) && (af_cxt->win_pos[0].end_y > af_cxt->win_pos[0].start_y)){
			region[9].start_x = af_cxt->win_pos[0].start_x ;
			region[9].start_y = af_cxt->win_pos[0].start_y ;
			region[9].end_x   = af_cxt->win_pos[0].end_x ;
			region[9].end_y   = af_cxt->win_pos[0].end_y ;
		}
		else{
			region[9].start_x = region[4].start_x ;
			region[9].start_y = region[4].start_y ;
			region[9].end_x   = region[4].end_x ;
			region[9].end_y   = region[4].end_y ;
		}
	}
	else if (ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode == 16 ){ //tes_kano_0825 for face af
		// for face
		if(af_cxt->fd_info.face_num > 1){ //for big size face select
			face_max_size = (af_cxt->fd_info.face_info[0].ex - af_cxt->fd_info.face_info[0].sx) * (af_cxt->fd_info.face_info[0].ey - af_cxt->fd_info.face_info[0].sy);
			for(i=1 ; i<af_cxt->fd_info.face_num ; i++){
				face_max_size_temp = (af_cxt->fd_info.face_info[i].ex - af_cxt->fd_info.face_info[i].sx) * (af_cxt->fd_info.face_info[i].ey - af_cxt->fd_info.face_info[i].sy);
				if(face_max_size_temp > face_max_size){
					j = i;
					face_max_size = face_max_size_temp;
				}
			}
			face_sx = af_cxt->fd_info.face_info[j].sx;
			face_sy = af_cxt->fd_info.face_info[j].sy;
			face_ex = af_cxt->fd_info.face_info[j].ex;
			face_ey = af_cxt->fd_info.face_info[j].ey;
		}
		else{
			face_sx = af_cxt->fd_info.face_info[0].sx;
			face_sy = af_cxt->fd_info.face_info[0].sy;
			face_ex = af_cxt->fd_info.face_info[0].ex;
			face_ey = af_cxt->fd_info.face_info[0].ey;
		}
		if((face_ex > face_sx) && (face_ey > face_sy)){
			// tes_kano_0831 temporary face area x5
			k = 5;
			region[9].start_x = face_sx * k ;
			region[9].start_y = face_sy * k;
			region[9].end_x   = face_ex * k;
			region[9].end_y   = face_ey * k;
			h1 = af_cxt->sensor_w - 100 ;
			h2 = af_cxt->sensor_w - 1 ;
			v1 = af_cxt->sensor_h - 100 ;
			v2 = af_cxt->sensor_h - 1 ;
			// tes_kano_0902 for over area
			if(region[9].start_x > h1){
				region[9].start_x = h1 ;
			}
			if(region[9].end_x > h2){
				region[9].end_x   = h2 ;
			}
			if(region[9].start_y > v1){
				region[9].start_y = 3000 ;
			}
			if(region[9].end_y > v2){
				region[9].end_y   = v2 ;
			}
		}
		else{
			region[9].start_x = region[4].start_x ;
			region[9].start_y = region[4].start_y ;
			region[9].end_x   = region[4].end_x ;
			region[9].end_y   = region[4].end_y ;
		}

	}
	else{
		region[9].start_x = region[4].start_x ;
		region[9].start_y = region[4].start_y ;
		region[9].end_x   = region[4].end_x ;
		region[9].end_y   = region[4].end_y ;
	}
	
	af_ops->cb_set_active_win( handle, 0x03FF);//tes_kano 01FF->03FF
	af_ops->cb_set_sp_afm_win(handle, region);
	
	//tes_kano_0820
	Al_af_area_set(ppAfIfBuf , region);

}

void  Al_AF_Running(alc_af_handle_t handle , TT_AfIfBuf* ppAfIfBuf)//tes_kano_0820
{ // 0 : Fail, 1 : Success
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;

	struct alc_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;

	uint32_t  pos;//for test
	//static uint32_t  firstfr = 0;//for test
	uint32_t  statis[150];
	//static uint32_t  prepos = 0;//for test
	uint32_t  i;
	uint32_t ae_lumi[10] ;//tes_kano_0820
	
	UI_16 sx = 1690 ;
	UI_16 sy = 1269 ;
	UI_16 ex = 2469 ;
	UI_16 ey = 1850 ;
	
//	return;

	ppAfIfBuf->mttInfo.mttAfCameraInfo.muiAfTv = af_cxt->cur_frame_time ;
	//ppAfIfBuf->mttInfo.mttAfCameraInfo.muiAfAv =  ;
	ppAfIfBuf->mttInfo.mttAfCameraInfo.muiAfSv = af_cxt->cur_ae_again ;
	ppAfIfBuf->mttInfo.mttAfCameraInfo.muiAfEv = af_cxt->cur_ae_ev ;
	ppAfIfBuf->mttInfo.mttAfCameraInfo.muiAfFps = af_cxt->cur_fps ;
	ppAfIfBuf->mttInfo.mttAfCameraInfo.muiAfIso = af_cxt->cur_ae_iso ;
	
	if(ppAfIfBuf->mttAcoAf.init == 0){
		ppAfIfBuf->mttAdjust.mttFilterControl.msiFixedFilter = 0 ; //fix hpf coef
		Al_AF_Prv_Start_Notice(handle , ppAfIfBuf); //tes_kano_0820
		af_ops->cb_unlock_ae(handle);
		af_ops->cb_unlock_awb(handle);
	} else {
		if(ppAfIfBuf->mttAcoAf.af_finish == 2){
			ppAfIfBuf->mttAdjust.mttFilterControl.msiFixedFilter = 0 ; //fix hpf coef
			Al_AF_Prv_Start_Notice(handle , ppAfIfBuf); //tes_kano_0825 tap area & filter setting
			ppAfIfBuf->mttAcoAf.af_finish = 0 ;
			ppAfIfBuf->mttAcoAf.ae_unlock_request = 1;
		}
		else if(ppAfIfBuf->mttAcoAf.caf_finish == 1){
			ppAfIfBuf->mttAdjust.mttFilterControl.msiFixedFilter = 0 ; //fix hpf coef
			Al_AF_Prv_Start_Notice(handle , ppAfIfBuf); //tes_kano_0825 tap area & filter setting
			ppAfIfBuf->mttAcoAf.caf_finish = 0 ;
			ppAfIfBuf->mttAcoAf.ae_unlock_request = 1;
		}
		else if(ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode == 0 && ppAfIfBuf->mttAcoAf.af_status == 0){ //tes_kano_0904 for single af
			ppAfIfBuf->mttAdjust.mttFilterControl.msiFixedFilter = 1 ; //change hpf coef by sv
			Al_AF_Prv_Start_Notice(handle , ppAfIfBuf); //tes_kano_0825 & filter setting
			ppAfIfBuf->mttAcoAf.ae_lock_request = 1;
		}
		else if(ppAfIfBuf->mttAdjust.mttFilterControl.msiFixedFilter == 2){
			Al_AF_Prv_Start_Notice(handle , ppAfIfBuf);
		}
#if ALC_AF_DEFOCUS_TEST > 0 //defocus
		else if (ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode == 8){ //tes_kano_0825 for defocus
			if(ppAfIfBuf->mttAcoAf.defocus_status == 1){
				ppAfIfBuf->mttAcoAf.ae_lock_request = 1;
			}
		}
#endif
		//AE/AWB lock or unlock
		if (ppAfIfBuf->mttAcoAf.ae_lock_request == 1){
			if(af_cxt->ae_is_locked == 0){
				af_ops->cb_lock_ae(handle);
				af_ops->cb_lock_awb(handle);
			}
			ppAfIfBuf->mttAcoAf.ae_lock_request = 0;
		}
		else if(ppAfIfBuf->mttAcoAf.ae_unlock_request == 1){
			if(af_cxt->ae_is_locked == 1){
				af_ops->cb_unlock_ae(handle);
				af_ops->cb_unlock_awb(handle);
			}
			ppAfIfBuf->mttAcoAf.ae_unlock_request = 0;
		}

		Al_AF_GetFv(handle,statis);
	}
	
	memset(ae_lumi,0,sizeof(ae_lumi));
	for (i=0;i<9;i++)
	{
		Al_GetAfRegion( ppAfIfBuf , &sx , &sy, &ex , &ey , i) ;
		ae_lumi[i] = Al_GetAfY(af_cxt->sensor_w , af_cxt->sensor_h , sx , sy , ex , ey , &af_cxt->r_info[0] , &af_cxt->g_info[0] , &af_cxt->b_info[0]);
	}

	Al_FocusDataGet(ppAfIfBuf , statis , ae_lumi) ;
	
//	ALC_AF_LOGI("ALC_CAF,%d,%d,%d", ppAfIfBuf->mttAcoAf.caf_status , ppAfIfBuf->mttAcoAf.af_current_position , ppAfIfBuf->mttAcoAf.af_current_data1[0]);
	
#if 1
	//log get for debug
	ppAfIfBuf->mttAcoAf.log_data[7] = af_cxt->ae_is_locked ;
	//ppAfIfBuf->mttAcoAf.log_data[8] = af_cxt->g_info[512] ;
	ppAfIfBuf->mttAcoAf.log_data[8] = af_cxt->r_info[512] ;
	if(af_cxt->sprd_filter.IIR_c[6]==114) ppAfIfBuf->mttAcoAf.log_data[16] = 114 ;
	else if(af_cxt->sprd_filter.IIR_c[6]==73) ppAfIfBuf->mttAcoAf.log_data[16] = 73 ;
	else ppAfIfBuf->mttAcoAf.log_data[16] = 0 ;
	if(af_cxt->sprd_filter.IIR_c[7]==-51) ppAfIfBuf->mttAcoAf.log_data[17] = 51 ;
	else if(af_cxt->sprd_filter.IIR_c[7]==-26) ppAfIfBuf->mttAcoAf.log_data[17] = 26 ;
	else ppAfIfBuf->mttAcoAf.log_data[17] = 0 ;
#endif
	
	//AF_ISR(handle);
	ALC_AF_Running(af_ops , &pos , ppAfIfBuf);//tes_kano_0820

	af_ops->cb_set_motor_pos(handle,pos);// for test
	//prepos = pos;

}

#if 0
//
//
//

#define	PixSizeH			4160
#define	PixSizeV			3120

#define	Hno_MAX				32
#define	Vno_MAX				32

#define	LumKval_R			0.299
#define	LumKval_G			0.587
#define	LumKval_B			0.114

#define	winSize_H			130
#define	winSize_V			97


typedef	struct {
	ushort16_t	XL, YL;
	ushort16_t	XR, YR;
} T_EvaluationWindow;

typedef	struct {
	double	Hiritsu;
} T_WindowInfo;

typedef	struct {
	ushort32_t	r_info[1024];
	ushort32_t	g_info[1024];
	ushort32_t	b_info[1024];
} T_isp_awb_statistic_info;


static	T_WindowInfo	WinInfo[Vno_MAX][Hno_MAX];

static	ushort32_t		AfLuminance[Hno_MAX * Vno_MAX];


//=====================================================================

//=====================================================================
int	AfLuminance_Setup( T_EvaluationWindow *AF_Win )
{
uchar	Hno, Vno;
uchar	EvaWin_LU_Hno, EvaWin_LU_Vno, EvaWin_LU_Hmod, EvaWin_LU_Vmod;
uchar	EvaWin_LD_Hno, EvaWin_LD_Vno, EvaWin_LD_Hmod, EvaWin_LD_Vmod;
uchar	EvaWin_RU_Hno, EvaWin_RU_Vno, EvaWin_RU_Hmod, EvaWin_RU_Vmod;
uchar	EvaWin_RD_Hno, EvaWin_RD_Vno, EvaWin_RD_Hmod, EvaWin_RD_Vmod;

	if( (AF_Win->XL < AF_Win->XR) && (AF_Win->YL < AF_Win->YR) ) {



		for( Vno=0; Vno<Vno_MAX; ++Vno ) {
			for( Hno=0; Hno<Hno_MAX; ++Hno ) {
				WinInfo[Vno][Hno].Hiritsu = 0;
				AfLuminance[(Vno * Vno_MAX) + Hno] = 0;
			}
		}


		EvaWin_LU_Hno  = AF_Win->XL / winSize_H;		EvaWin_LU_Hmod = AF_Win->XL % winSize_H;
		EvaWin_LU_Vno  = AF_Win->YL / winSize_V;		EvaWin_LU_Vmod = AF_Win->YL % winSize_V;
		WinInfo[EvaWin_LU_Vno][EvaWin_LU_Hno].Hiritsu = (double)((winSize_H - EvaWin_LU_Hmod) * (winSize_V - EvaWin_LU_Vmod)) / (double)(winSize_H * winSize_V);

		EvaWin_LD_Hno  = AF_Win->XL / winSize_H;		EvaWin_LD_Hmod = AF_Win->XL % winSize_H;
		EvaWin_LD_Vno  = AF_Win->YR / winSize_V;		EvaWin_LD_Vmod = AF_Win->YR % winSize_V;
		WinInfo[EvaWin_LD_Vno][EvaWin_LD_Hno].Hiritsu = (double)((winSize_H - EvaWin_LD_Hmod) * EvaWin_LU_Vmod) / (double)(winSize_H * winSize_V);

		EvaWin_RU_Hno  = AF_Win->XR / winSize_H;		EvaWin_RU_Hmod = AF_Win->XR % winSize_H;
		EvaWin_RU_Vno  = AF_Win->YL / winSize_V;		EvaWin_RU_Vmod = AF_Win->YL % winSize_V;
		WinInfo[EvaWin_RU_Vno][EvaWin_RU_Hno].Hiritsu = (double)(EvaWin_RU_Hmod * (winSize_V - EvaWin_RU_Vmod)) / (double)(winSize_H * winSize_V);

		EvaWin_RD_Hno  = AF_Win->XR / winSize_H;		EvaWin_RD_Hmod = AF_Win->XR % winSize_H;
		EvaWin_RD_Vno  = AF_Win->YR / winSize_V;		EvaWin_RD_Vmod = AF_Win->YR % winSize_V;
		WinInfo[EvaWin_RD_Vno][EvaWin_RD_Hno].Hiritsu = (double)(EvaWin_RD_Hmod * EvaWin_RD_Vmod) / (double)(winSize_H * winSize_V);


		for( Vno=EvaWin_LU_Vno+1,Hno=EvaWin_LU_Hno; Vno<EvaWin_LD_Vno; ++Vno ) {
			WinInfo[Vno][Hno].Hiritsu = (double)((winSize_H - EvaWin_LU_Hmod) * winSize_V) / (double)(winSize_H * winSize_V);
		}
		for( Vno=EvaWin_RU_Vno+1,Hno=EvaWin_RU_Hno; Vno<EvaWin_RD_Vno; ++Vno ) {
			WinInfo[Vno][Hno].Hiritsu = (double)((winSize_H - EvaWin_RU_Hmod) * winSize_V) / (double)(winSize_H * winSize_V);
		}
		for( Hno=EvaWin_LU_Hno+1,Vno=EvaWin_LU_Vno; Hno<EvaWin_RU_Hno; ++Hno ) {
			WinInfo[Vno][Hno].Hiritsu = (double)((winSize_V - EvaWin_LU_Vmod) * winSize_H) / (double)(winSize_H * winSize_V);
		}
		for( Hno=EvaWin_LD_Hno+1,Vno=EvaWin_LU_Vno; Hno<EvaWin_RD_Hno; ++Hno ) {
			WinInfo[Vno][Hno].Hiritsu = (double)((winSize_V - EvaWin_RU_Vmod) * winSize_H) / (double)(winSize_H * winSize_V);
		}
		
		for( Vno=EvaWin_LU_Vno+1; Vno<EvaWin_RD_Vno; ++Vno ) {
			for( Hno=EvaWin_LU_Hno+1; Hno<(EvaWin_RU_Hno - EvaWin_LU_Hno - 1); ++Hno ) {
				WinInfo[Vno][Hno].Hiritsu = 1;
			}
		}

		if( ((EvaWin_LU_Hno == EvaWin_LD_Hno) && (EvaWin_LU_Vno == EvaWin_LD_Vno)) &&
			((EvaWin_RU_Hno == EvaWin_RD_Hno) && (EvaWin_RU_Vno == EvaWin_RD_Vno)) ) {
			
			for( Vno=0; Vno<Vno_MAX; ++Vno ) {
				for( Hno=0; Hno<Hno_MAX; ++Hno ) {
					WinInfo[Vno][Hno].Hiritsu = 0;
				}
			}
			WinInfo[EvaWin_LU_Vno][EvaWin_LU_Hno].Hiritsu = (double)((AF_Win->XR - AF_Win->XL) * (AF_Win->YR - AF_Win->YL)) / (double)(winSize_H * winSize_V);
		} else if( (EvaWin_LU_Hno < EvaWin_RU_Hno)  &&
				  ((EvaWin_LU_Hno == EvaWin_LD_Hno) && (EvaWin_LU_Vno < EvaWin_LD_Vno)) &&
				  ((EvaWin_RU_Hno == EvaWin_RD_Hno) && (EvaWin_RU_Vno < EvaWin_RD_Vno)) ) {
			
			for( Vno=0; Vno<Vno_MAX; ++Vno ) {
				for( Hno=0; Hno<Hno_MAX; ++Hno ) {
					WinInfo[Vno][Hno].Hiritsu = 0;
				}
			}
			WinInfo[EvaWin_LU_Vno][EvaWin_LU_Hno].Hiritsu = (double)((winSize_H - EvaWin_LU_Hmod) * (EvaWin_LD_Vmod - EvaWin_LU_Vmod)) / (double)(winSize_H * winSize_V);
			WinInfo[EvaWin_RD_Vno][EvaWin_RD_Hno].Hiritsu = (double)((EvaWin_RD_Hmod * (EvaWin_LD_Vmod - EvaWin_LU_Vmod)) * winSize_H) / (double)(winSize_H * winSize_V);
			for( Hno=EvaWin_LU_Hno+1,Vno=EvaWin_LU_Vno; Hno<EvaWin_RU_Hno; ++Hno ) {
				WinInfo[Vno][Hno].Hiritsu = (double)((winSize_V - (EvaWin_LD_Vmod - EvaWin_LU_Vmod)) * winSize_H) / (double)(winSize_H * winSize_V);
			}
		} else if( (EvaWin_LU_Vno == EvaWin_RU_Vno) &&
				  ((EvaWin_LU_Hno == EvaWin_LD_Hno) && (EvaWin_LU_Vno < EvaWin_LD_Vno)) &&
				  ((EvaWin_RU_Hno == EvaWin_RD_Hno) && (EvaWin_RU_Vno < EvaWin_RD_Vno)) ) {
			
			for( Vno=0; Vno<Vno_MAX; ++Vno ) {
				for( Hno=0; Hno<Hno_MAX; ++Hno ) {
					WinInfo[Vno][Hno].Hiritsu = 0;
				}
			}
			WinInfo[EvaWin_LU_Vno][EvaWin_LU_Hno].Hiritsu = (double)((winSize_V - EvaWin_LU_Vmod) * (EvaWin_RD_Hmod - EvaWin_LU_Hmod)) / (double)(winSize_H * winSize_V);
			WinInfo[EvaWin_RD_Vno][EvaWin_RD_Hno].Hiritsu = (double)((EvaWin_RD_Vmod * (EvaWin_RD_Hmod - EvaWin_LD_Hmod)) * winSize_H) / (double)(winSize_H * winSize_V);
			for( Vno=EvaWin_LU_Vno+1,Hno=EvaWin_LU_Hno; Vno<EvaWin_LD_Vno; ++Vno ) {
				WinInfo[Vno][Hno].Hiritsu = (double)((winSize_H - (EvaWin_LD_Hmod - EvaWin_LU_Hmod)) * winSize_V) / (double)(winSize_H * winSize_V);
			}
		}

		return( 0 );
	} else {
		return( -1 );
	}
}

//=====================================================================
// 
//=====================================================================

//=====================================================================
int	AfLuminance_Multiplication( T_isp_awb_statistic_info *af_cxt, ushort32_t *AfLuminance )
{
uchar		Hno, Vno;
ushort16_t	Idx;
ushort32_t	AfLimin;
//#if	defined(WIN32)
//	wchar_t	debugStr[256];
//	swprintf( debugStr, sizeof(debugStr), L"Saturn::AfLuminance_Multiplication() Start\n" );
//	OutputDebugString(debugStr);
//#endif

	if( (af_cxt == NULL) || (AfLuminance == NULL) ) {
		return( -1 );		//
	}

	
	for( Vno=0; Vno<Vno_MAX; ++Vno ) {
		for( Hno=0; Hno<Hno_MAX; ++Hno ) {
			if( WinInfo[Vno][Hno].Hiritsu > 0.0 ) { //
				Idx = (Vno * Vno_MAX) + Hno;
				if( af_cxt->r_info[Idx] > 4095 )	af_cxt->r_info[Idx] = 4095;
				if( af_cxt->g_info[Idx] > 4095 )	af_cxt->g_info[Idx] = 4095;
				if( af_cxt->b_info[Idx] > 4095 )	af_cxt->b_info[Idx] = 4095;
				AfLimin = (ushort32_t)(( (LumKval_R * (double)af_cxt->r_info[Idx]) +
										 (LumKval_G * (double)af_cxt->g_info[Idx]) +
										 (LumKval_B * (double)af_cxt->b_info[Idx]) ) * WinInfo[Vno][Hno].Hiritsu);
				if( AfLimin > 4095 )	AfLimin = 4095;	//
				AfLuminance[Idx] += AfLimin;
				if( AfLuminance[Idx] > 0x7fffffff ) {
					AfLuminance[Idx] = 0x7fffffff;
				}
//#if	defined(WIN32)
//	swprintf( debugStr, sizeof(debugStr), L"Luminance[%d]=%ld (Hiritsu=%lf) (H:%2d , V:%2d)\n", Idx, AfLimin, WinInfo[Vno][Hno].Hiritsu, Hno, Vno );
//	OutputDebugString(debugStr);
//#endif
			}
		}
	}
	return( 0 );
}
#endif


#endif //__ALC_AF_INTERFACE_C__
/* ------------------------------------------------------------------------ */
/* end of this file							    */
/* ------------------------------------------------------------------------ */
