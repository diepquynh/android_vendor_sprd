/*
 * Copyright (C) 2008 The Android Open Source Project
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
#ifndef _EXIF_WRITER_H_
#define _EXIF_WRITER_H_


#include "sci_types.h"
#include "jpeg_exif_header.h"
#include "jpeg_fw_def.h"
#include "isp_exif.h"
#define JPEG_HEADER_BYTE	2
#ifdef __cplusplus
    extern   "C"
    {
#endif


typedef struct
{
	uint16	year;
	uint16	month;
	uint16	day;
}JINF_DATE_T;

typedef struct
{
	uint16	hour;
	uint16	minute;
	uint16	second;
}JINF_TIME_T;

typedef struct
{
    JINF_EXIF_INFO_T        *exif_info_ptr;
    EXIF_ISP_INFO_T         *exif_isp_info;
    uint8                   *temp_exif_isp_buf_ptr;
    uint32                  temp_exif_isp_buf_size;
    uint8                   *src_jpeg_buf_ptr;
    uint32                  src_jpeg_size;
    uint8                   *thumbnail_buf_ptr;
    uint32                  thumbnail_buf_size;
    uint8                   *temp_buf_ptr;          //should be bigger than (thumbnail_buf_size + 20K)
    uint32                  temp_buf_size;
    JINF_WRITE_FILE_FUNC    wrtie_file_func;
	uint8					*target_buf_ptr;
	uint32					target_buf_size;
}JINF_WEXIF_IN_PARAM_T;

typedef struct
{
    uint8                   *output_buf_ptr;
    uint32                  output_size;
}JINF_WEXIF_OUT_PARAM_T;

typedef struct thumbnail_info_tag
{
    uint32                          width;
    uint32                          height;
    uint32                          offset;             //read only.
    uint32                          size;
    uint8                           *stream_buf_ptr;   //Optional when read thumbnail. Set NULL to ignore it
    uint32                          stream_buf_size;
}EXIF_THUMBNAIL_INFO_T;

typedef enum
{
	JINF_SUCCESS = 0,
	JINF_STATUS_BUSY,
	JINF_INVALID_ARGUMENT,
	JINF_MEMORY_NOT_ENOUGH,
	JINF_MEMORY_MALLOC_FAILED,
	JINF_MEMORY_FREE_FAILED,
	JINF_PARSE_EXIF_ERROR,
	JINF_HEAD_INFO_ERROR,
	JINF_OUTPUT_BUFFER_OVERFLOW,
	JINF_FORMAT_UNSUPPORTED,
	JINF_HARDWARE_ERROR,
	JINF_READ_FILE_ERROR,
	JINF_WRITE_FILE_ERROR,
	JINF_PROGRESSIVE_JPEG_IN_FILE,
	JINF_SCALING_FACTOR_UNSUPPORTED,
	JINF_SCALING_ERROR,
	JINF_FAILED = 0xff
}JINF_RET_E;


JPEG_RET_E Jpeg_WriteAPP1(uint8 *target_buf,
                                 uint32 target_buf_size,
                                 JINF_EXIF_INFO_T *exif_info_ptr,
                                 uint8 *thumbnail_buf_ptr,
                                 uint32 thumbnail_size,
                                 uint32 *app1_size_ptr);

/*
*@	Name :
*@	Description:	add the EXIF
*@	Author:			Shan.he
*@  Parameters:
*@                  in_param_ptr:      pointer of the input parameter
*@                  out_param_ptr:     pointer of the output parameter
*@	Note:           return JINF_SUCCESS if successful
*/
JINF_RET_E IMGJPEG_WriteExif(JINF_WEXIF_IN_PARAM_T *in_param_ptr,
										 JINF_WEXIF_OUT_PARAM_T *out_param_ptr);

/*
*@	Name :
*@	Description:	format the date/time string
*@	Author:			Shan.he
*@  Parameters:
*@                  date_ptr:      	pointer of the date parameter
*@                  time_ptr:    	pointer of the time parameter
*@					date_time_ptr: 	pointer of the date time string buffer
*@					date_time_size:	size of the data time string buffer, should be at least 20 bytes
*@	Note:           return JINF_SUCCESS if successful
*/
/*PUBLIC JINF_RET_E IMGJPEG_FormatDateTime(JINF_DATE_T *date_ptr, JINF_TIME_T *time_ptr,
										  EXIF_ASCII_T *date_time_ptr, uint32 date_time_size);*/
#ifdef __cplusplus
    }
#endif

#endif // _EXIF_WRITER_H_
