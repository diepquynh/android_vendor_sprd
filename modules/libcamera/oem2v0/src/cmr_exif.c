/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cmr_common.h"
#include "cmr_exif.h"




#define ARR_MAX_LEN 100



EXIF_PRIMARY_INFO_T     exif_primary_info;
EXIF_SPECIFIC_INFO_T    exif_specific_info;
EXIF_GPS_INFO_T         exif_gps_info;

/*EXIF_SPECIFIC_INFO_T*/
EXIF_SPEC_DATE_TIME_T   exif_spec_date_time;
EXIF_SPEC_OTHER_T       exif_spec_other;


/*EXIF_PRIMARY_INFO_T*/
static EXIF_PRI_DATA_STRUCT_T exif_pri_data_struct_info = {
	{1, 0}, /*just Orientation valid*/
	1, /*The 0th row is at the visual top of the image, and the 0th column is the visual left-hand side*/
	0
};

static EXIF_PRI_DESC_T exif_pri_desc_info = {
	{0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},/*valid*/
	"Default Date",                  /*Date */
	"Exif_JPEG_420-freed",	         /*ImageDescription*/
	"Spreadtrum-freed",              /*Make */
	"SmartPhone-freed",              /*Model */
	"Software Version v1.1.0",       /*Software*/
	"Artist-freed",                  /*Artist */
	"Copyright(C),Spreadtrum,2014"   /*Copyright*/
};


static EXIF_SPEC_USER_T exif_spec_user_info;

EXIF_SPEC_BASIC_T exif_spec_basic = {
	1,			   /*ColorSpace*/
	{1, 2, 3, 0},  /*ComponentsConfiguration*/
	640,           /*PixelXDimension will be repace after set*/
	480,           /*PixelYDimension will be repace after set*/
};

static cmr_int set_exif_pri_desc(JINF_EXIF_INFO_T *jinf_exif_info_ptr,
                                   EXIF_PRI_DESC_T *p_exif_pri_desc);
static cmr_int set_exif_specific_basic(JINF_EXIF_INFO_T *jinf_exif_info_ptr,
                                       EXIF_SPECIFIC_INFO_T *exif_specific_info,
                                       EXIF_SPEC_BASIC_T exif_spec_basic);
static cmr_int set_exif_specific_user(JINF_EXIF_INFO_T *jinf_exif_info_ptr, EXIF_SPEC_USER_T *exif_spec_user);
static cmr_int set_exif_specific_pic_taking_cond(JINF_EXIF_INFO_T *jinf_exif_info_ptr,
										EXIF_SPEC_PIC_TAKING_COND_T *exif_spec_pic_taking_cond_ptr);
static cmr_int set_exif_primary(JINF_EXIF_INFO_T *jinf_exif_info_ptr,
			                    EXIF_PRI_DATA_STRUCT_T *exif_pri_data);
static cmr_int set_exif_spec_other(JINF_EXIF_INFO_T *jinf_exif_info_ptr,
                                   EXIF_SPEC_OTHER_T *p_exif_spec_other);

static cmr_int set_exif_specific_data_time(JINF_EXIF_INFO_T *jinf_exif_info_ptr,
                                          EXIF_SPEC_DATE_TIME_T *p_image_date_time);
static cmr_int set_exif_gps(JINF_EXIF_INFO_T *jinf_exif_info, EXIF_GPS_INFO_T *p_exif_gps_info);


/*user_time =1 ,use ue*/
cmr_int get_time (cmr_int user_time, char *p_time)
{
	time_t timep = {0};
	struct tm *p;
	static cmr_int loop = 0;

	time(&timep);
	p=localtime(&timep);


	if (!p_time) {
		return -1;
	}

	if (user_time) {
		sprintf(p_time,
		"%04d:%02d:%02d %02d:%02d:%02d",
		(p->tm_year+1900),
		(p->tm_mon+1),
		p->tm_mday,
		p->tm_hour,
		p->tm_min,
		p->tm_sec);

	} else {
#ifdef KERNEL_TIME
			SCI_DATE_T cur_date = {
				0
			};
			SCI_TIME_T cur_time = {
				0
			};
			TM_GetSysDate(&cur_date);
			TM_GetSysTime(&cur_time);
			sprintf(p_time,
				"%04d:%02d:%02d %02d:%02d:%02d",
				cur_date.year,
				cur_date.mon,
				cur_date.mday,
				cur_time.hour,
				cur_time.min,
				cur_time.sec);
#endif
	}
	CMR_LOGV("year_date_time =%s", p_time);

	return 0;
}

cmr_int cmr_exif_init(JINF_EXIF_INFO_T *jinf_exif_info_ptr, setting_get_pic_taking_cb setting_cb, void *priv_data)
{
	cmr_int                       ret =  0;
	EXIF_SPEC_PIC_TAKING_COND_T   *img_sensor_exif_ptr = (EXIF_SPEC_PIC_TAKING_COND_T *)(*setting_cb)(priv_data);


	CMR_LOGV("E");
	CMR_LOGD("flash=%d denominator=%d",img_sensor_exif_ptr->Flash ,img_sensor_exif_ptr->ExposureIndex.denominator);

	if (!jinf_exif_info_ptr) {
		CMR_LOGD("jinf_exif_info_ptr null");
		return -1;
	}


	set_exif_specific_basic(jinf_exif_info_ptr, &exif_specific_info, exif_spec_basic);

	set_exif_specific_user(jinf_exif_info_ptr, &exif_spec_user_info);

	set_exif_specific_pic_taking_cond(jinf_exif_info_ptr, img_sensor_exif_ptr);

	set_exif_specific_data_time(jinf_exif_info_ptr, &exif_spec_date_time);

	set_exif_primary(jinf_exif_info_ptr, &exif_pri_data_struct_info);

	set_exif_spec_other(jinf_exif_info_ptr, &exif_spec_other);

	set_exif_gps(jinf_exif_info_ptr, &exif_gps_info);

	set_exif_pri_desc(jinf_exif_info_ptr, &exif_pri_desc_info);

	CMR_LOGD(" X");

	return ret;
}

cmr_int set_exif_pri_desc(JINF_EXIF_INFO_T *jinf_exif_info_ptr, EXIF_PRI_DESC_T *p_exif_pri_desc)
{
	JINF_EXIF_INFO_T   *p_jinf_exif_info =  jinf_exif_info_ptr;
	char               arr_time[20] = {0};


	CMR_LOGV("E");

	if (!p_jinf_exif_info || !p_exif_pri_desc) {
		CMR_LOGD("jinf_exif_info  or p_exif_pri_desc null !");
		return -1;
	}

	get_time(1, arr_time);

	CMR_LOGV("arr_time =%s", arr_time);

	strcpy((char *)p_exif_pri_desc->DateTime, arr_time);

	*(uint32_t *) &(p_exif_pri_desc->valid) = (uint32_t)0x7F;

	p_jinf_exif_info->primary.img_desc_ptr  = p_exif_pri_desc;

	CMR_LOGV("return 0x%p DateTime=%s", p_exif_pri_desc, p_exif_pri_desc->DateTime);

	CMR_LOGV("X");
	return 0;
}


cmr_int set_exif_specific_basic(JINF_EXIF_INFO_T *jinf_exif_info_ptr,
                             EXIF_SPECIFIC_INFO_T *p_exif_specific_info,
                             EXIF_SPEC_BASIC_T exif_spec_basic)
{
	JINF_EXIF_INFO_T *jinf_exif_info =  jinf_exif_info_ptr;

	CMR_LOGV("E");

	if (!jinf_exif_info) {
		CMR_LOGD("jinf_exif_info null !");
		return  -1;
	}

	jinf_exif_info->spec_ptr        = p_exif_specific_info;
	jinf_exif_info->spec_ptr->basic = exif_spec_basic;

	CMR_LOGV(" X");
	return 0;
}

cmr_int set_exif_specific_user(JINF_EXIF_INFO_T *jinf_exif_info_ptr, EXIF_SPEC_USER_T *p_exif_spec_user)
{
	JINF_EXIF_INFO_T *jinf_exif_info = jinf_exif_info_ptr;

	CMR_LOGV("E");

	if (!jinf_exif_info) {
		CMR_LOGD("jinf_exif_info null !");
		return  -1;
	}

	if (!jinf_exif_info->spec_ptr) {
		CMR_LOGD("jinf_exif_info spec_ptr null !");
		return  -1;
	}

	jinf_exif_info->spec_ptr->user_ptr = p_exif_spec_user;

	CMR_LOGV("X");
	return 0;

}

cmr_int set_exif_specific_pic_taking_cond(JINF_EXIF_INFO_T *jinf_exif_info_ptr ,
						   EXIF_SPEC_PIC_TAKING_COND_T *exif_spec_pic_taking_cond_ptr)
{
	JINF_EXIF_INFO_T *jinf_exif_info = jinf_exif_info_ptr;

	CMR_LOGV("E");

	if (!jinf_exif_info) {
		CMR_LOGD("jinf_exif_info null !");
		return  -1;
	}

	if (!jinf_exif_info->spec_ptr) {
		CMR_LOGD("jinf_exif_info->spec_ptr null !");
		return  -1;
	}

	jinf_exif_info->spec_ptr->pic_taking_cond_ptr = exif_spec_pic_taking_cond_ptr;

	CMR_LOGV("X");
	return 0;
}

cmr_int set_exif_primary(JINF_EXIF_INFO_T *jinf_exif_info_ptr,
						EXIF_PRI_DATA_STRUCT_T *p_exif_pri_data)
{
	JINF_EXIF_INFO_T *jinf_exif_info = jinf_exif_info_ptr;

	if (!jinf_exif_info) {
		CMR_LOGD("jinf_exif_info null !");
		return  -1;
	}

	CMR_LOGV("E");

	jinf_exif_info->primary.basic.YCbCrPositioning        = 1;
	jinf_exif_info->primary.basic.XResolution.numerator   = 72;
	jinf_exif_info->primary.basic.XResolution.denominator = 1;
	jinf_exif_info->primary.basic.YResolution.numerator   = 72;
	jinf_exif_info->primary.basic.YResolution.denominator = 1;
	jinf_exif_info->primary.basic.ResolutionUnit          = 2;
	jinf_exif_info->primary.data_struct_ptr               = p_exif_pri_data;

	p_exif_pri_data->valid.Orientation = 1;
	p_exif_pri_data->Orientation       = 1;

	CMR_LOGV("X");
	return 0;
}

cmr_int set_exif_spec_other(JINF_EXIF_INFO_T *jinf_exif_info_ptr, EXIF_SPEC_OTHER_T *p_exif_spec_other)
{
	JINF_EXIF_INFO_T   *jinf_exif_info = jinf_exif_info_ptr;
	char               arr_time[ARR_MAX_LEN] = {0};
	cmr_int            ret = 0;

	CMR_LOGV("E");

	if (!jinf_exif_info) {
		CMR_LOGD("jinf_exif_info null !");
		return  -1;
	}

	ret = get_time(1, arr_time);

	p_exif_spec_other->valid.ImageUniqueID = 1;

	if (strlen(arr_time) < 33)
		strncpy((char *)p_exif_spec_other->ImageUniqueID, arr_time, strlen(arr_time));

	CMR_LOGV("%s",p_exif_spec_other->ImageUniqueID);

	jinf_exif_info->spec_ptr->other_ptr = p_exif_spec_other;

	CMR_LOGV("X");
	return 0;
}

cmr_int set_exif_gps(JINF_EXIF_INFO_T *jinf_exif_info, EXIF_GPS_INFO_T *p_exif_gps_info)
{

	if (!jinf_exif_info) {
		CMR_LOGD("jinf_exif_info null !");
		return  -1;
	}

	jinf_exif_info->gps_ptr = p_exif_gps_info;

	return	0;
}


cmr_int set_exif_specific_data_time(JINF_EXIF_INFO_T *jinf_exif_info_ptr,
                                   EXIF_SPEC_DATE_TIME_T *p_image_date_time)
{
	JINF_EXIF_INFO_T   *jinf_exif_info = jinf_exif_info_ptr;
	char               arr_time[20] = {0};


	if (!jinf_exif_info || !p_image_date_time) {
		CMR_LOGE("param null");
		return  -1;
	}

	/*process time*/
	get_time(1, arr_time);

	strcpy((char *)p_image_date_time->DateTimeOriginal, arr_time);
	strcpy((char*)p_image_date_time->DateTimeDigitized, arr_time);
	p_image_date_time->valid.DateTimeOriginal  = 1;
	p_image_date_time->valid.DateTimeDigitized = 1;
	p_image_date_time->valid.SubSecTime = 1;
	p_image_date_time->valid.SubSecTimeDigitized = 1;
	p_image_date_time->valid.SubSecTimeOriginal = 1;
	strcpy((char *)p_image_date_time->SubSecTime, "10");
	strcpy((char *)p_image_date_time->SubSecTimeDigitized, "20");
	strcpy((char *)p_image_date_time->SubSecTimeOriginal, "100");

	CMR_LOGV("%s",p_image_date_time->DateTimeOriginal);
	if (NULL != jinf_exif_info->spec_ptr)
		jinf_exif_info->spec_ptr->date_time_ptr = p_image_date_time;

	return 0;
}


