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
#include <stdlib.h>
#include "sci_types.h"
#include "jpeg_jfif.h"
#include "jpeg_exif_type.h"
#include "exif_writer.h"
#include "jpeg_stream.h"
#include "sc8830_video_header.h"

#define DEBUG_STR     "%s(L %d), %s: "
#define DEBUG_ARGS    __FILE__,__LINE__,__FUNCTION__

#undef JPEG_PRINT_LOW
#define JPEG_PRINT_LOW(format,...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)
#define JPEG_ALIGN_4(_input) ((((_input) + 3) >> 2) << 2)

/*#define EXIF_DEBUG*/

/*
*@	Name :
*@	Description:	Get string size in which the terminating null character is not included
*@	Author:			Shan.he
*@  Parameters:
*@                  string_ptr:     pointer of string
*@	Note:           size of the string
*/
LOCAL uint32 _GetAsciiStringSize(const uint8 *string_ptr)
{
    uint32 string_size = 0;

    if(PNULL == string_ptr)
    {
		JPEG_PRINT_LOW("[_GetAsciiStringSize] invalid string pointer");
		return string_size;
    }

    while ('\0' != *string_ptr++)
    {
        string_size++;
    }

    return string_size;
}

/*
*@	Name :
*@	Description:	write a IFD
*@	Author:			Shan.he
*@  Parameters:
*@                  context_ptr:    pointer of context
*@                  ifd_info_ptr:   pointer of IFD structure
*@	Note:           return TRUE if successful else return FALSE
*/
LOCAL BOOLEAN Jpeg_WriteIFD(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr,
                          IFD_INFO_T *ifd_info_ptr)
{
	uint32	offset_value  = 0;

    JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, ifd_info_ptr->tag, return FALSE);
    JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, ifd_info_ptr->type, return FALSE);
    JPEG_WRITE_DATA(Jpeg_WriteL, context_ptr, ifd_info_ptr->count, return FALSE);

	offset_value = ifd_info_ptr->value_offset.long_value;

	//do not use the uion to avoid endian issue
	switch (ifd_info_ptr->type)
	{
	case IFD_BYTE:
	case IFD_ASCII:
	case IFD_UNDEFINED:
		if (ifd_info_ptr->count <= 4)
		{
			offset_value = (ifd_info_ptr->value_offset.byte_value[0] << 24)
						| (ifd_info_ptr->value_offset.byte_value[1] << 16)
						| (ifd_info_ptr->value_offset.byte_value[2] << 8)
						| ifd_info_ptr->value_offset.byte_value[3];
		}
		break;

	case IFD_SHORT:
		if (ifd_info_ptr->count <= 2)
		{
			offset_value = (ifd_info_ptr->value_offset.short_value[0] << 16)
						| (ifd_info_ptr->value_offset.short_value[1]);
		}
		break;

	default:
		offset_value = ifd_info_ptr->value_offset.long_value;
		break;
	}

    JPEG_WRITE_DATA(Jpeg_WriteL, context_ptr, offset_value, return FALSE);

    return TRUE;
}

/*
*@	Name :
*@	Description:	write a IFD of the RATIONAL/SRATIONAL type
*@	Author:			Shan.he
*@  Parameters:
*@                  context_ptr:    pointer of context
*@                  ifd_ptr:        pointer of IFD structure
*@                  ifd_offset_ptr: [IN/OUT] pointer of the IFD offset
*@                  value_offset_ptr: [IN/OUT] pointer of the value offset
*@	Note:           return TRUE if successful else return FALSE
*/
LOCAL BOOLEAN Jpeg_WriteRationalIFD(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr,
                                IFD_INFO_T *ifd_ptr,
                                uint32 *ifd_offset_ptr,
                                uint32 *value_offset_ptr)
{
    uint32 ifd_offset       = *ifd_offset_ptr;
    uint32 value_offset     = *value_offset_ptr;
    uint32 i                = 0;
    EXIF_RATIONAL_T *ptr    = NULL;

    if (PNULL == ifd_ptr || PNULL == ifd_ptr->value_ptr)
    {
        return FALSE;
    }

    ptr = (EXIF_RATIONAL_T *)ifd_ptr->value_ptr;

    //write IFD header
    if (!Jpeg_SetWritePos(context_ptr, ifd_offset))
    {
        return FALSE;
    }

    if (!Jpeg_WriteIFD(context_ptr, ifd_ptr))
    {
        return FALSE;
    }

    ifd_offset = Jpeg_GetWritePos(context_ptr);

    if (!Jpeg_SetWritePos(context_ptr, value_offset))
    {
        return FALSE;
    }

    for (i=0; i<ifd_ptr->count; i++)
    {
        JPEG_WRITE_DATA(Jpeg_WriteL, context_ptr, ptr->numerator, return FALSE);
        JPEG_WRITE_DATA(Jpeg_WriteL, context_ptr, ptr->denominator, return FALSE);
        ptr++;
    }

    value_offset = Jpeg_GetWritePos(context_ptr);
	value_offset = JPEG_ALIGN_4(value_offset);

    *ifd_offset_ptr = ifd_offset;
    *value_offset_ptr = value_offset;

    return TRUE;
}

/*
*@	Name :
*@	Description:	write a IFD of the LONG/SLONG type
*@	Author:			Shan.he
*@  Parameters:
*@                  context_ptr:    pointer of context
*@                  ifd_ptr:        pointer of IFD structure
*@                  ifd_offset_ptr: [IN/OUT] pointer of the IFD offset
*@                  value_offset_ptr: [IN/OUT] pointer of the value offset
*@	Note:           return TRUE if successful else return FALSE
*/
LOCAL BOOLEAN Jpeg_WriteLongIFD(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr,
                            IFD_INFO_T *ifd_ptr,
                            uint32 *ifd_offset_ptr,
                            uint32 *value_offset_ptr)
{
    uint32 ifd_offset   = *ifd_offset_ptr;
    uint32 value_offset = *value_offset_ptr;
    uint32 i            = 0;
    EXIF_LONG_T *ptr    = PNULL;

    if (PNULL == ifd_ptr || PNULL == ifd_ptr->value_ptr)
    {
        return FALSE;
    }

    ptr = (EXIF_LONG_T *)ifd_ptr->value_ptr;
    if (ifd_ptr->count <= 1)
    {
        ifd_ptr->value_offset.long_value = *ptr;
    }

    //write IFD header
    if (!Jpeg_SetWritePos(context_ptr, ifd_offset))
    {
        return FALSE;
    }

    if (!Jpeg_WriteIFD(context_ptr, ifd_ptr))
    {
        return FALSE;
    }

    ifd_offset = Jpeg_GetWritePos(context_ptr);

    if (ifd_ptr->count > 1)
    {
        if (!Jpeg_SetWritePos(context_ptr, value_offset))
        {
            return FALSE;
        }

        for (i=0; i<ifd_ptr->count; i++)
        {
            JPEG_WRITE_DATA(Jpeg_WriteL, context_ptr, *ptr, return FALSE);
            ptr++;
        }

        value_offset = Jpeg_GetWritePos(context_ptr);
		value_offset = JPEG_ALIGN_4(value_offset);
    }

    *ifd_offset_ptr = ifd_offset;
    *value_offset_ptr = value_offset;

    return TRUE;
}

/*
*@	Name :
*@	Description:	write a IFD of the SHORT type
*@	Author:			Shan.he
*@  Parameters:
*@                  context_ptr:    pointer of context
*@                  ifd_ptr:        pointer of IFD structure
*@                  ifd_offset_ptr: [IN/OUT] pointer of the IFD offset
*@                  value_offset_ptr: [IN/OUT] pointer of the value offset
*@	Note:           return TRUE if successful else return FALSE
*/
BOOLEAN Jpeg_WriteShortIFD(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr,
                            IFD_INFO_T *ifd_ptr,
                            uint32 *ifd_offset_ptr,
                            uint32 *value_offset_ptr)
{
    uint32 ifd_offset       = *ifd_offset_ptr;
    uint32 value_offset     = *value_offset_ptr;
    uint32 i                = 0;
    EXIF_SHORT_T *ptr       = PNULL;

    if (PNULL == ifd_ptr || PNULL == ifd_ptr->value_ptr)
    {
        return FALSE;
    }

    ptr = (EXIF_SHORT_T *)ifd_ptr->value_ptr;

    if (ifd_ptr->count <= 2)
    {
        ifd_ptr->value_offset.long_value = 0;
        ifd_ptr->value_offset.short_value[0] = *ptr;
        if (2 == ifd_ptr->count)
        {
            ifd_ptr->value_offset.short_value[1] = *(ptr + 1);
        }
    }

    //write IFD header
    if (!Jpeg_SetWritePos(context_ptr, ifd_offset))
    {
        return FALSE;
    }

    if (!Jpeg_WriteIFD(context_ptr, ifd_ptr))
    {
        return FALSE;
    }

    ifd_offset = Jpeg_GetWritePos(context_ptr);

    if (ifd_ptr->count > 2)
    {
        if (!Jpeg_SetWritePos(context_ptr, value_offset))
        {
            return FALSE;
        }

        for (i=0; i<ifd_ptr->count; i++)
        {
            JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, *ptr, return JPEG_FAILED);
            ptr++;
        }

        value_offset = Jpeg_GetWritePos(context_ptr);
		value_offset = JPEG_ALIGN_4(value_offset);
    }

    *ifd_offset_ptr = ifd_offset;
    *value_offset_ptr = value_offset;

    return TRUE;
}

/*
*@	Name :
*@	Description:	write a IFD of the BYTE/ASCII type
*@	Author:			Shan.he
*@  Parameters:
*@                  context_ptr:    pointer of context
*@                  ifd_ptr:        pointer of IFD structure
*@                  ifd_offset_ptr: [IN/OUT] pointer of the IFD offset
*@                  value_offset_ptr: [IN/OUT] pointer of the value offset
*@	Note:           return TRUE if successful else return FALSE
*/
BOOLEAN Jpeg_WriteByteIFD(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr,
                            IFD_INFO_T *ifd_ptr,
                            uint32 *ifd_offset_ptr,
                            uint32 *value_offset_ptr)
{
    uint32 ifd_offset   = *ifd_offset_ptr;
    uint32 value_offset = *value_offset_ptr;
    uint32 i            = 0;
    EXIF_BYTE_T *ptr    = PNULL;

    if (PNULL == ifd_ptr || PNULL == ifd_ptr->value_ptr)
    {
        return FALSE;
    }

    ptr = (EXIF_BYTE_T *)ifd_ptr->value_ptr;

    if (ifd_ptr->count <= 4)
    {
        //copy ifd_ptr->count bytes, deliberately lose break
        i = 0;
        switch (ifd_ptr->count)
        {
        case 4:
            ifd_ptr->value_offset.byte_value[i++] = *ptr++;
        case 3:	 /*lint !e616 !e825 */
            ifd_ptr->value_offset.byte_value[i++] = *ptr++;
        case 2:  /*lint !e616 !e825 */
            ifd_ptr->value_offset.byte_value[i++] = *ptr++;
        case 1:  /*lint !e616 !e825 */
            ifd_ptr->value_offset.byte_value[i++] = *ptr++;
            break;
        default:
            JPEG_PRINT_LOW("Jpeg_WriteByteIFD default!");
            break;
        }
    }

    //write IFD header
    if (!Jpeg_SetWritePos(context_ptr, ifd_offset))
    {
        return FALSE;
    }

    if (!Jpeg_WriteIFD(context_ptr, ifd_ptr))
    {
        return FALSE;
    }

    ifd_offset = Jpeg_GetWritePos(context_ptr);

    if (ifd_ptr->count > 4)
    {
        if (!Jpeg_SetWritePos(context_ptr, value_offset))
        {
            return FALSE;
        }

        for (i=0; i<ifd_ptr->count; i++)
        {
            JPEG_WRITE_DATA(Jpeg_WriteC, context_ptr, *ptr, return JPEG_FAILED);
            ptr++;
        }

        value_offset = Jpeg_GetWritePos(context_ptr);
		value_offset = JPEG_ALIGN_4(value_offset);
    }

    *ifd_offset_ptr = ifd_offset;
    *value_offset_ptr = value_offset;

    return TRUE;
}

/*
*@	Name :
*@	Description:	write interoperability info
*@	Author:			Shan.he
*@  Parameters:
*@                  context_ptr:    pointer of context
*@                  inter_ptr:        pointer of interoperability info structure
*@                  ifh_offset:     offset of the IFH
*@                  begin_offset:   the start position
*@                  end_offset_ptr: [IN/OUT] pointer of the end position value
*@	Note:           return TRUE if successful else return FALSE
*/
LOCAL JPEG_RET_E Jpeg_WriteExifInteroperabilityInfo(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr,
					                                        EXIF_INTEROPERABILITY_INFO_T *inter_ptr,
					                                        uint32 ifh_offset,
					                                        uint32 begin_offset,
					                                        uint32 *end_offset_ptr)
{
    uint16      entries             = 0;
    uint32      ifd_offset          = 0;
    uint32      ifd_value_offset    = 0;
    IFD_INFO_T  ifd_info;//            = {0};
    EXIF_INTEROPERABILITY_VALID_T valid = {0};

    memset(&ifd_info, 0, sizeof(IFD_INFO_T));

    if (PNULL == inter_ptr)
    {
        return JPEG_FAILED;
    }

    //write the max number of the entries temporarily and memorize the location
    entries = 1;
    Jpeg_SetWritePos(context_ptr, begin_offset);
    JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, entries, return JPEG_FAILED);

    ifd_offset = Jpeg_GetWritePos(context_ptr);
    ifd_value_offset = ifd_offset + entries * IFD_HEAD_LENGTH + 4;
	ifd_value_offset = JPEG_ALIGN_4(ifd_value_offset);

    entries = 0;

    valid = inter_ptr->valid;
    /***********************************************************************/
    //write IFD
    /***********************************************************************/

    if (valid.InteroperabilityIndex
        && '\0' != *(inter_ptr->InteroperabilityIndex.ptr)
        && inter_ptr->InteroperabilityIndex.count > 0)
    {
        ifd_info.tag = IFD_INTEROPERABILITYINDEX;
        ifd_info.type = IFD_ASCII;
	    ifd_info.count = inter_ptr->InteroperabilityIndex.count;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)inter_ptr->InteroperabilityIndex.ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    Jpeg_SetWritePos(context_ptr, ifd_offset);

    JPEG_WRITE_DATA(Jpeg_WriteL, context_ptr, 0, return JPEG_FAILED);

    //rewrite the number of entries actually write in
    Jpeg_SetWritePos(context_ptr, begin_offset);
    JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, entries, return JPEG_FAILED);

    *end_offset_ptr = ifd_value_offset;

    return JPEG_SUCCESS;
}



/*
*@	Name :
*@	Description:	write specified-EXIF info
@*	Author:			Shan.he
*@  Parameters:
*@                  context_ptr:    pointer of context
*@                  spec_ptr:       pointer of specified-EXIF info structure
*@                  ifh_offset:     offset of the IFH
*@                  begin_offset:   the start position
*@                  end_offset_ptr: [IN/OUT] pointer of the end position value
*@	Note:           return TRUE if successful else return FALSE
*/
LOCAL JPEG_RET_E Jpeg_WriteExifSpecInfo(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr,
                                        EXIF_SPECIFIC_INFO_T *spec_ptr,
                                        EXIF_INTEROPERABILITY_INFO_T *inter_ptr,
                                        uint32 ifh_offset,
                                        uint32 begin_offset,
                                        uint32 *end_offset_ptr)
{
    uint16      entries             = 0;
    uint32      ifd_offset          = 0;
    uint32      ifd_value_offset    = 0;
    IFD_INFO_T  ifd_info;//            = {0};
	JPEG_RET_E  ret                 = JPEG_SUCCESS;
	uint32 		offset 				= 0;

	EXIF_UNDEFINED_T  exif_version[4] 		= {'0', '2', '2', '0'};
	EXIF_UNDEFINED_T  flashpix_version[4] 	= {'0', '1', '0', '0'};

    EXIF_SPEC_PIC_TAKING_COND_T     *pic_taking_cond_ptr    = PNULL;
    EXIF_SPEC_DATE_TIME_T           *date_time_ptr          = PNULL;
    EXIF_SPEC_IMG_CONFIG_T          *config_ptr             = PNULL;
    EXIF_SPEC_OTHER_T               *other_ptr              = PNULL;
    EXIF_SPEC_RELATED_FILE_T        *rel_file_ptr           = PNULL;
    EXIF_SPEC_BASIC_T               *baisc_ptr              = PNULL;
    EXIF_SPEC_USER_T                *user_ptr               = PNULL;

    if (PNULL == spec_ptr)
    {
        return JPEG_FAILED;
    }

    memset(&ifd_info, 0, sizeof(IFD_INFO_T));

    pic_taking_cond_ptr = spec_ptr->pic_taking_cond_ptr;
    date_time_ptr = spec_ptr->date_time_ptr;
    config_ptr = spec_ptr->img_config_ptr;
    other_ptr = spec_ptr->other_ptr;
    rel_file_ptr = spec_ptr->related_file_ptr;
    user_ptr = spec_ptr->user_ptr;
    baisc_ptr = &spec_ptr->basic;


    //write the max number of the entries temporarily and memorize the location
    entries = 57;
    Jpeg_SetWritePos(context_ptr, begin_offset);
    JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, entries, return JPEG_FAILED);

    ifd_offset = Jpeg_GetWritePos(context_ptr);
    ifd_value_offset = ifd_offset + entries * IFD_HEAD_LENGTH + 4;
	ifd_value_offset = JPEG_ALIGN_4(ifd_value_offset);

    entries = 0;
#ifdef EXIF_DEBUG
    JPEG_PRINT_LOW("pic_taking_cond_ptr = %x, user_ptr=%x \n", (uint32)pic_taking_cond_ptr, (uint32)user_ptr);

    if(NULL != pic_taking_cond_ptr)
    {
		JPEG_PRINT_LOW("%x, %x \n", pic_taking_cond_ptr->FocalLength.numerator, pic_taking_cond_ptr->ExposureProgram);
    }
#endif
    /***********************************************************************/
    //write IFD
    /***********************************************************************/
    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.ExposureTime)
    {
        ifd_info.tag = IFD_EXPOSURETIME;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->ExposureTime;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.FNumber)
    {
        ifd_info.tag = IFD_FNUMBER;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->FNumber;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.ExposureProgram)
    {
        ifd_info.tag = IFD_EXPOSUREPROGRAM;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->ExposureProgram;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.SpectralSensitivity)
    {
        ifd_info.tag = IFD_SPECTRALSENSITIVITY;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = _GetAsciiStringSize(pic_taking_cond_ptr->SpectralSensitivity) + 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_bytes = ifd_info.count;
        ifd_info.value_ptr = (void *)pic_taking_cond_ptr->SpectralSensitivity;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.ISOSpeedRatings
        && '\0' != *(pic_taking_cond_ptr->ISOSpeedRatings.ptr)
        && pic_taking_cond_ptr->ISOSpeedRatings.count > 0)
    {
        //size of any
        ifd_info.tag = IFD_ISOSPEEDRATINGS;
        ifd_info.type = IFD_SHORT;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.count = pic_taking_cond_ptr->ISOSpeedRatings.count;
        ifd_info.value_ptr = (void *)pic_taking_cond_ptr->ISOSpeedRatings.ptr;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }


    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.OECF
        && '\0' != *(pic_taking_cond_ptr->OECF.ptr)
        && pic_taking_cond_ptr->OECF.count > 0)
    {
        //size of any
        ifd_info.tag = IFD_OECF;
        ifd_info.type = IFD_UNDEFINED;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.count = pic_taking_cond_ptr->OECF.count;
        ifd_info.value_ptr = (void *)pic_taking_cond_ptr->OECF.ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    {
        ifd_info.tag = IFD_EXIFVERSION;
        ifd_info.type = IFD_UNDEFINED;
        ifd_info.count = 4;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)exif_version;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != date_time_ptr && date_time_ptr->valid.DateTimeOriginal)
    {
        ifd_info.tag = IFD_DATETIMEORIGINAL;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 20;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)date_time_ptr->DateTimeOriginal;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != date_time_ptr && date_time_ptr->valid.DateTimeDigitized)
    {
        ifd_info.tag = IFD_DATETIMEDIGITILIZED;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 20;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)date_time_ptr->DateTimeDigitized;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    {
        ifd_info.tag = IFD_COMPONENTSCONFIGURATION;
        ifd_info.type = IFD_UNDEFINED;
        ifd_info.count = 4;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)baisc_ptr->ComponentsConfiguration;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != config_ptr && config_ptr->valid.CompressedBitsPerPixel)
    {
        ifd_info.tag = IFD_COMPRESSEDBITSPERPIXEL;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&config_ptr->CompressedBitsPerPixel;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.ShutterSpeedValue)
    {
        ifd_info.tag = IFD_SHUTTERSPEEDVALUE;
        ifd_info.type = IFD_SRATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->ShutterSpeedValue;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }


    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.ApertureValue)
    {
        ifd_info.tag = IFD_APERTUREVALUE;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->ApertureValue;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.BrightnessValue)
    {
        ifd_info.tag = IFD_BRIGHTNESSVALUE;
        ifd_info.type = IFD_SRATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->BrightnessValue;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.ExposureBiasValue)
    {
        ifd_info.tag = IFD_EXPOSUREBIASVALUE;
        ifd_info.type = IFD_SRATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->ExposureBiasValue;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.MaxApertureValue)
    {
        ifd_info.tag = IFD_MAXAPERTUREVALUE;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->MaxApertureValue;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.SubjectDistance)
    {
        ifd_info.tag = IFD_SUBJECTDISTANCE;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->SubjectDistance;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.MeteringMode)
    {
        //size of any
        ifd_info.tag = IFD_METERINGMODE;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->MeteringMode;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.LightSource)
    {
        //size of any
        ifd_info.tag = IFD_LIGHTSOURCE;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->LightSource;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }


    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.Flash)
    {
        ifd_info.tag = IFD_FLASH;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->Flash;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.FocalLength)
    {
        ifd_info.tag = IFD_FOCALLENGTH;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->FocalLength;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.SubjectArea
        && '\0' != *(pic_taking_cond_ptr->SubjectArea.ptr)
        && pic_taking_cond_ptr->SubjectArea.count > 0)
    {
        ifd_info.tag = IFD_SUBJECTAREA;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = pic_taking_cond_ptr->SubjectArea.count;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)pic_taking_cond_ptr->SubjectArea.ptr;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != user_ptr && '\0' != *(user_ptr->MakerNote.ptr)
        && user_ptr->MakerNote.count > 0)
    {
        ifd_info.tag = IFD_MAKERNOTE;
        ifd_info.type = IFD_UNDEFINED;
        ifd_info.count = user_ptr->MakerNote.count;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)user_ptr->MakerNote.ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != user_ptr && '\0' != *(user_ptr->UserComment.ptr)
        && user_ptr->UserComment.count > 0)
    {
        ifd_info.tag = IFD_USERCOMMENT;
        ifd_info.type = IFD_UNDEFINED;
        ifd_info.count = user_ptr->UserComment.count;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)user_ptr->UserComment.ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != date_time_ptr && date_time_ptr->valid.SubSecTime)
    {
        ifd_info.tag = IFD_SUBSECTIME;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = _GetAsciiStringSize(date_time_ptr->SubSecTime) + 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)date_time_ptr->SubSecTime;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != date_time_ptr && date_time_ptr->valid.SubSecTimeOriginal)
    {
        ifd_info.tag = IFD_SUBSECTIMEORIGINAL;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = _GetAsciiStringSize(date_time_ptr->SubSecTimeOriginal) + 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)date_time_ptr->SubSecTimeOriginal;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != date_time_ptr && date_time_ptr->valid.SubSecTimeDigitized)
    {
        ifd_info.tag = IFD_SUBSECTIMEDIGITILIZED;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = _GetAsciiStringSize(date_time_ptr->SubSecTimeDigitized) + 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)date_time_ptr->SubSecTimeDigitized;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }


    {
        ifd_info.tag = IFD_FLASHPIXVERSION;
        ifd_info.type = IFD_UNDEFINED;
        ifd_info.count = 4;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)flashpix_version;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    {
        ifd_info.tag = IFD_COLORSPACE;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&baisc_ptr->ColorSpace;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    {
        ifd_info.tag = IFD_PIXELXDIMENSION;
        ifd_info.type = IFD_LONG;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&baisc_ptr->PixelXDimension;
#ifdef EXIF_DEBUG
		JPEG_PRINT_LOW("baisc_ptr->PixelXDimension = %d.",baisc_ptr->PixelXDimension);
#endif
        if (!Jpeg_WriteLongIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    {
        ifd_info.tag = IFD_PIXELYDIMENSION;
        ifd_info.type = IFD_LONG;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&baisc_ptr->PixelYDimension;

        if (!Jpeg_WriteLongIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != rel_file_ptr && rel_file_ptr->valid.RelatedSoundFile)
    {
        ifd_info.tag = IFD_RELATEDSOUNDFILE;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 13;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)rel_file_ptr->RelatedSoundFile;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

	if (PNULL != inter_ptr)
    {
        //interoperability IFD pointer
        ifd_value_offset = JPEG_ALIGN_4(ifd_value_offset);
        offset = ifd_value_offset - ifh_offset;

        ifd_info.tag = IFD_INTEROPERABILITYIFDPOINTER;
        ifd_info.type = IFD_LONG;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&offset;       /*lint !e733*/

        if (!Jpeg_WriteLongIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        ret = Jpeg_WriteExifInteroperabilityInfo(context_ptr, inter_ptr, ifh_offset,
												ifd_value_offset, &ifd_value_offset);
        if (JPEG_SUCCESS != ret)
        {
            return ret;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.FlashEnergy)
    {
        ifd_info.tag = IFD_FLASHENERGY;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->FlashEnergy;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.SpatialFrequencyResponse
        && '\0' != *(pic_taking_cond_ptr->SpatialFrequencyResponse.ptr)
        && pic_taking_cond_ptr->SpatialFrequencyResponse.count > 0)
    {
        ifd_info.tag = IFD_SPATIALFREQUENCYRESPONSE;
        ifd_info.type = IFD_UNDEFINED;
        ifd_info.count = pic_taking_cond_ptr->SpatialFrequencyResponse.count;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)pic_taking_cond_ptr->SpatialFrequencyResponse.ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.FocalPlaneXResolution)
    {
        ifd_info.tag = IFD_FOCALPLANEXRESOLUTION;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->FocalPlaneXResolution;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.FocalPlaneYResolution)
    {
        ifd_info.tag = IFD_FOCALPLANEYRESOLUTION;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->FocalPlaneYResolution;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.FocalPlaneResolutionUnit)
    {
        ifd_info.tag = IFD_FOCALPLANERESOLUTIONUNIT;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->FocalPlaneResolutionUnit;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.SubjectLocation)
    {
        ifd_info.tag = IFD_SUBJECTLOCATION;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 2;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)pic_taking_cond_ptr->SubjectLocation;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.ExposureIndex)
    {
        ifd_info.tag = IFD_EXPOSUREINDEX;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->ExposureIndex;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.SensingMethod)
    {
        ifd_info.tag = IFD_SENSINGMETHOD;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset; ;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->SensingMethod;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.FileSource)
    {
        ifd_info.tag = IFD_FILESOURCE;
        ifd_info.type = IFD_UNDEFINED;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset; ;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->FileSource;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.SceneType)
    {
        ifd_info.tag = IFD_SCENETYPE;
        ifd_info.type = IFD_UNDEFINED;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->SceneType;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.CFAPattern
        && '\0' != *(pic_taking_cond_ptr->CFAPattern.ptr)
        && pic_taking_cond_ptr->CFAPattern.count > 0)
    {
        ifd_info.tag = IFD_CFAPATTERN;
        ifd_info.type = IFD_UNDEFINED;
        ifd_info.count = pic_taking_cond_ptr->CFAPattern.count;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)pic_taking_cond_ptr->CFAPattern.ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.CustomRendered)
    {
        ifd_info.tag = IFD_CUSTOMRENDERED;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->CustomRendered;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.ExposureMode)
    {
        ifd_info.tag = IFD_EXPOSUREMODE;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->ExposureMode;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.WhiteBalance)
    {
        ifd_info.tag = IFD_WHITEBALANCE;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->WhiteBalance;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.DigitalZoomRatio)
    {
        ifd_info.tag = IFD_DIGITALZOOMRATIO;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->DigitalZoomRatio;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.FocalLengthIn35mmFilm)
    {
        ifd_info.tag = IFD_FOCALLENGTHIN35MMFILM;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->FocalLengthIn35mmFilm;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.SceneCaptureType)
    {
        ifd_info.tag = IFD_SCENECAPTURETYPE;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->SceneCaptureType;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.GainControl)
    {
        ifd_info.tag = IFD_GAINCONTROL;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->GainControl;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.Contrast)
    {
        ifd_info.tag = IFD_CONTRAST;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->Contrast;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.Saturation)
    {
        ifd_info.tag = IFD_SATURATION;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->Saturation;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.Sharpness)
    {
        ifd_info.tag = IFD_SHARPNESS;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->Sharpness;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.DeviceSettingDescription
        && '\0' != *(pic_taking_cond_ptr->DeviceSettingDescription.ptr)
        && pic_taking_cond_ptr->DeviceSettingDescription.count > 0)
    {
        ifd_info.tag = IFD_DEVICESETTINGDESCRIPTION;
        ifd_info.type = IFD_UNDEFINED;
        ifd_info.count = pic_taking_cond_ptr->DeviceSettingDescription.count;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)pic_taking_cond_ptr->DeviceSettingDescription.ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != pic_taking_cond_ptr && pic_taking_cond_ptr->valid.SubjectDistanceRange)
    {
        ifd_info.tag = IFD_SUBJECTDISTANCERANGE;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&pic_taking_cond_ptr->SubjectDistanceRange;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != other_ptr && other_ptr->valid.ImageUniqueID)
    {
        ifd_info.tag = IFD_IMAGEUNIQUEID;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 33;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&other_ptr->ImageUniqueID;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    Jpeg_SetWritePos(context_ptr, ifd_offset);

    //write 0 to indicate no IFD following
    JPEG_WRITE_DATA(Jpeg_WriteL, context_ptr, 0, return JPEG_FAILED);

    //rewrite the number of entries actually write in
    Jpeg_SetWritePos(context_ptr, begin_offset);
    JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, entries, return JPEG_FAILED);

    *end_offset_ptr = ifd_value_offset;

    return JPEG_SUCCESS;
}

/*****************************************************************************
**	Name :
**	Description:	write GPS info
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:    pointer of context
**                  gps_ptr:        pointer of GPS info structure
**                  ifh_offset:     offset of the IFH
**                  begin_offset:   the start position
**                  end_offset_ptr: [IN/OUT] pointer of the end position value
**	Note:           return TRUE if successful else return FALSE
*****************************************************************************/
LOCAL JPEG_RET_E Jpeg_WriteExifGPSInfo(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr,
                                        EXIF_GPS_INFO_T *gps_ptr,
                                        uint32 ifh_offset,
                                        uint32 begin_offset,
                                        uint32 *end_offset_ptr)
{
    uint16      entries             = 0;
    uint32      ifd_offset          = 0;
    uint32      ifd_value_offset    = 0;
    IFD_INFO_T  ifd_info;//            = {0};
    EXIF_GPS_VALID_T valid;//          = {0};

    memset(&valid, 0, sizeof(EXIF_GPS_VALID_T));
    memset(&ifd_info, 0, sizeof(IFD_INFO_T));

    if (PNULL == gps_ptr)
    {
        return JPEG_FAILED;
    }

    //write the max number of the entries temporarily and memorize the location
    entries = 31;
    Jpeg_SetWritePos(context_ptr, begin_offset);
    JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, entries, return JPEG_FAILED);

    ifd_offset = Jpeg_GetWritePos(context_ptr);
    ifd_value_offset = ifd_offset + entries * IFD_HEAD_LENGTH + 4;
	ifd_value_offset = JPEG_ALIGN_4(ifd_value_offset);

    entries = 0;

    valid = gps_ptr->valid;
    /***********************************************************************/
    //write IFD
    /***********************************************************************/
    if (valid.GPSVersionID)
    {
        ifd_info.tag = IFD_GPSVERSIONID;
        ifd_info.type = IFD_BYTE;
        ifd_info.count = 4;
		ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSVersionID;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSLatitudeRef)
    {
        ifd_info.tag = IFD_GPSLATITUDEREF;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 2;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSLatitudeRef;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSLatitude)
    {
        ifd_info.tag = IFD_GPSLATITUDE;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 3;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSLatitude;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSLongitudeRef)
    {
        ifd_info.tag = IFD_GPSLONGITUDEREF;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 2;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSLongitudeRef;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSLongitude)
    {
        ifd_info.tag = IFD_GPSLONGITUDE;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 3;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSLongitude;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSAltitudeRef)
    {
        ifd_info.tag = IFD_GPSALTITUDEREF;
        ifd_info.type = IFD_BYTE;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&gps_ptr->GPSAltitudeRef;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSAltitude)
    {
        ifd_info.tag = IFD_GPSALTITUDE;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&gps_ptr->GPSAltitude;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSTimeStamp)
    {
        ifd_info.tag = IFD_GPSTIMESTAMP;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 3;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSTimeStamp;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSSatellites)
    {
        ifd_info.tag = IFD_GPSSATELLITES;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = _GetAsciiStringSize(gps_ptr->GPSSatellites) + 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_bytes = ifd_info.count;
        ifd_info.value_ptr = (void *)gps_ptr->GPSSatellites;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSStatus)
    {
        ifd_info.tag = IFD_GPSSTATUS;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 2;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSStatus;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSMeasureMode)
    {
        ifd_info.tag = IFD_GPSMEASUREMODE;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 2;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSMeasureMode;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSDOP)
    {
        ifd_info.tag = IFD_GPSDOP;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&gps_ptr->GPSDOP;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSSpeedRef)
    {
        ifd_info.tag = IFD_GPSSPEEDREF;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 2;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSSpeedRef;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSSpeed)
    {
        ifd_info.tag = IFD_GPSSPEED;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&gps_ptr->GPSSpeed;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSTrackRef)
    {
        ifd_info.tag = IFD_GPSTRACKREF;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 2;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSTrackRef;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSTrack)
    {
        ifd_info.tag = IFD_GPSTRACK;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&gps_ptr->GPSTrack;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSImgDirectionRef)
    {
        ifd_info.tag = IFD_GPSIMGDIRECTIONREF;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 2;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSImgDirectionRef;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSImgDirection)
    {
        ifd_info.tag = IFD_GPSIMGDIRECTION;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&gps_ptr->GPSImgDirection;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSMapDatum)
    {
        ifd_info.tag = IFD_GPSMAPDATUM;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = _GetAsciiStringSize(gps_ptr->GPSMapDatum) + 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_bytes = ifd_info.count;
        ifd_info.value_ptr = (void *)gps_ptr->GPSMapDatum;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSDestLatitudeRef)
    {
        ifd_info.tag = IFD_GPSDESTLATITUDEREF;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 2;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSDestLatitudeRef;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSDestLatitude)
    {
        ifd_info.tag = IFD_GPSDESTLATITUDE;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 3;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSDestLatitude;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSDestLongitudeRef)
    {
        ifd_info.tag = IFD_GPSDESTLONGITUDEREF;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 2;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSDestLongitudeRef;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSDestLatitude)
    {
        ifd_info.tag = IFD_GPSDESTLONGITUDE;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 3;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSDestLongitude;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSDestBearingRef)
    {
        ifd_info.tag = IFD_GPSDESTBEARINGREF;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 2;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSDestBearingRef;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSDestBearing)
    {
        ifd_info.tag = IFD_GPSDESTBEARING;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&gps_ptr->GPSDestBearing;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSDestDistanceRef)
    {
        ifd_info.tag = IFD_GPSDESTDISTANCEREF;
        ifd_info.type = IFD_ASCII;
        ifd_info.count = 2;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSDestDistanceRef;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSDestDistance)
    {
        ifd_info.tag = IFD_GPSDESTDISTANCE;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&gps_ptr->GPSDestDistance;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSProcessingMethod
        && '\0' != *(gps_ptr->GPSProcessingMethod.ptr)
        && gps_ptr->GPSProcessingMethod.count > 0)
    {
        ifd_info.tag = IFD_GPSPROCESSINGMETHOD;
        ifd_info.type = IFD_UNDEFINED;
	    ifd_info.count = gps_ptr->GPSProcessingMethod.count;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSProcessingMethod.ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSAreaInformation
        && '\0' != *(gps_ptr->GPSAreaInformation.ptr)
        && gps_ptr->GPSAreaInformation.count > 0)
    {
        ifd_info.tag = IFD_GPSAREAINFORMATION;
        ifd_info.type = IFD_UNDEFINED;
	    ifd_info.count = gps_ptr->GPSAreaInformation.count;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)gps_ptr->GPSAreaInformation.ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSDateStamp)
    {
        ifd_info.tag = IFD_GPSDATESTAMP;
        ifd_info.type = IFD_ASCII;
	    ifd_info.count = 11;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_bytes = ifd_info.count;
        ifd_info.value_ptr = (void *)gps_ptr->GPSDateStamp;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (valid.GPSDifferential)
    {
        ifd_info.tag = IFD_GPSDIFFERENTIAL;
        ifd_info.type = IFD_SHORT;
	    ifd_info.count = 1;
	    ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&gps_ptr->GPSDifferential;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    Jpeg_SetWritePos(context_ptr, ifd_offset);

    JPEG_WRITE_DATA(Jpeg_WriteL, context_ptr, 0, return JPEG_FAILED);

    //rewrite the number of entries actually write in
    Jpeg_SetWritePos(context_ptr, begin_offset);
    JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, entries, return JPEG_FAILED);

    *end_offset_ptr = ifd_value_offset;

    return  JPEG_SUCCESS;
}

/*****************************************************************************
**	Name :
**	Description:	write the exif primary information
**	Author:			Shan.he
**  Parameters:
**                  exif_info_ptr:    exif info
**                  begin_offset:     writing position
**                  ifh_offset:       IFH position
**                  end_offset_ptr:   pointer of end position when writing done,
**                                    output parameter
**	Note:           return JPEG_SUCESS if successful
*****************************************************************************/
LOCAL JPEG_RET_E Jpeg_WriteExifIFD(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr,
                                    EXIF_PRIMARY_INFO_T *primary_ptr,
                                    EXIF_SPECIFIC_INFO_T *spec_ptr,
                                    EXIF_GPS_INFO_T *gps_ptr,
                                    EXIF_INTEROPERABILITY_INFO_T *inter_ptr,
                                    EXIF_THUMBNAIL_INFO_T *thumbnail_ptr,
                                    BOOLEAN is_next_ifd_exist,
                                    uint32 ifh_offset,
                                    uint32 begin_offset,
                                    uint32 *end_offset_ptr)
{
    uint16      entries             = 0;
    uint32      ifd_offset          = 0;
    uint32      ifd_value_offset    = 0;
    IFD_INFO_T  ifd_info;//            = {0};
    uint32      entries_offset      = 0;
    uint32      thumbnail_ifd_offset= 0;
    JPEG_RET_E  ret                 = JPEG_SUCCESS;

    EXIF_PRI_DATA_STRUCT_T  *data_struct_ptr;
    EXIF_PRI_DATA_CHAR_T    *data_char_ptr;
    EXIF_PRI_DESC_T         *img_desc_ptr;
    EXIF_LONG_T thumbnail_offset = 0;
    EXIF_LONG_T thumbnail_size = 0;
    EXIF_LONG_T offset = 0;

    if (PNULL == primary_ptr)
    {
        return JPEG_FAILED;
    }

	data_struct_ptr    = primary_ptr->data_struct_ptr;
    data_char_ptr      = primary_ptr->data_char_ptr;
    img_desc_ptr       = primary_ptr->img_desc_ptr;

    memset(&ifd_info, 0, sizeof(IFD_INFO_T));

    //write the max number of the entries temporarily and memorize the location
    entries = 24;
    Jpeg_SetWritePos(context_ptr, begin_offset);
    JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, entries, return JPEG_FAILED);
    entries_offset = begin_offset;

    ifd_offset = Jpeg_GetWritePos(context_ptr);
    ifd_value_offset = ifd_offset + entries * IFD_HEAD_LENGTH + 4;
	ifd_value_offset = JPEG_ALIGN_4(ifd_value_offset);

    entries = 0;
    /***********************************************************************/
    //write IFD
    /***********************************************************************/
    if (PNULL != data_struct_ptr && data_struct_ptr->valid.Reserved)
    {
        ifd_info.tag = IFD_COMPRESSION;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
		ifd_info.value_offset.long_value = 0;
        ifd_info.value_offset.short_value[0] = data_struct_ptr->Reserved;
        ifd_info.value_ptr = (void *)&data_struct_ptr->Reserved;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != img_desc_ptr && img_desc_ptr->valid.ImageDescription)
    {
        void *desc_ptr = img_desc_ptr->ImageDescription;

        ifd_info.tag = IFD_IMAGEDESCRIPTION;
        ifd_info.type = IFD_ASCII;
        //add an NULL char
        ifd_info.count = _GetAsciiStringSize(desc_ptr) + 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_bytes = ifd_info.count;
        ifd_info.value_ptr = desc_ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != img_desc_ptr && img_desc_ptr->valid.Make)
    {
        void *desc_ptr = img_desc_ptr->Make;

        ifd_info.tag = IFD_MAKE;
        ifd_info.type = IFD_ASCII;
        //add an NULL char
        ifd_info.count = _GetAsciiStringSize(desc_ptr) + 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_bytes = ifd_info.count;
        ifd_info.value_ptr = desc_ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != img_desc_ptr && img_desc_ptr->valid.Model)
    {
        void *desc_ptr = img_desc_ptr->Model;

        ifd_info.tag = IFD_MODEL;
        ifd_info.type = IFD_ASCII;
        //add an NULL char
        ifd_info.count = _GetAsciiStringSize(desc_ptr) + 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_bytes = ifd_info.count;
        ifd_info.value_ptr = desc_ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != data_struct_ptr && data_struct_ptr->valid.Orientation)
    {
        ifd_info.tag = IFD_ORIENTATION;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
		ifd_info.value_offset.long_value = 0;
        ifd_info.value_offset.short_value[0] = data_struct_ptr->Orientation;
        ifd_info.value_ptr = (void *)&data_struct_ptr->Orientation;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    {
        ifd_info.tag = IFD_XRESOLUTION;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
		ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&primary_ptr->basic.XResolution;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    {
        ifd_info.tag = IFD_YRESOLUTION;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&primary_ptr->basic.YResolution;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    {
        ifd_info.tag = IFD_RESOLUTIONUNIT;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
		ifd_info.value_offset.long_value = 0;
        ifd_info.value_offset.short_value[0] = primary_ptr->basic.ResolutionUnit;
        ifd_info.value_ptr = (void *)&primary_ptr->basic.ResolutionUnit;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != data_char_ptr && data_char_ptr->valid.TransferFunction)
    {
        ifd_info.tag = IFD_TRANSFERFUNCTION;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 3 * 256;
		ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&data_char_ptr->TransferFunction;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != img_desc_ptr && img_desc_ptr->valid.Software)
    {
        void *desc_ptr = (void *)img_desc_ptr->Software;

        ifd_info.tag = IFD_SOFTWARE;
        ifd_info.type = IFD_ASCII;
        //add an NULL char
        ifd_info.count = _GetAsciiStringSize(desc_ptr) + 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = desc_ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != img_desc_ptr && img_desc_ptr->valid.DateTime)
    {
        ifd_info.tag = IFD_DATETIME;
        ifd_info.type = IFD_ASCII;
        //add an NULL char
        ifd_info.count = 20;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)img_desc_ptr->DateTime;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != img_desc_ptr && img_desc_ptr->valid.Artist)
    {
        void *desc_ptr = (void *)img_desc_ptr->Artist;

        ifd_info.tag = IFD_ARTIST;
        ifd_info.type = IFD_ASCII;
        //add an NULL char
        ifd_info.count = _GetAsciiStringSize(desc_ptr) + 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = desc_ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != primary_ptr->data_char_ptr && data_char_ptr->valid.WhitePoint)/*lint !e613*/
    {
        ifd_info.tag = IFD_WHITEPOINT;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 2;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)data_char_ptr->WhitePoint;/*lint !e613*/

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != data_char_ptr && data_char_ptr->valid.PrimaryChromaticities)
    {
        ifd_info.tag = IFD_PRIMARYCHROMATICITIES;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 6;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)data_char_ptr->PrimaryChromaticities;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != thumbnail_ptr && PNULL != thumbnail_ptr->stream_buf_ptr
        && thumbnail_ptr->stream_buf_size > 0)
    {
        thumbnail_offset = 0;

        //just write the tag temporarily for unknown of the thumbnail offset
        ifd_info.tag = IFD_JPEGINTERCHANGEFORMAT;
        ifd_info.type = IFD_LONG;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&thumbnail_offset;

        thumbnail_ifd_offset = ifd_offset;

        if (!Jpeg_WriteLongIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != thumbnail_ptr && PNULL != thumbnail_ptr->stream_buf_ptr
        && thumbnail_ptr->stream_buf_size > 0)
    {
        thumbnail_size = thumbnail_ptr->stream_buf_size;
        ifd_info.tag = IFD_JPEGINTERCHANGEFORMATLENGTH;
        ifd_info.type = IFD_LONG;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&thumbnail_size;

        if (!Jpeg_WriteLongIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != data_char_ptr && data_char_ptr->valid.YCbCrCoefficients)
    {
        ifd_info.tag = IFD_YCBCRCOEFFICIENTS;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 3;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)data_char_ptr->YCbCrCoefficients;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    {
        ifd_info.tag = IFD_YCBCRPOSITION;
        ifd_info.type = IFD_SHORT;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&primary_ptr->basic.YCbCrPositioning;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    {
        ifd_info.tag = IFD_IMAGEWIDTH;
        ifd_info.type = IFD_LONG;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&primary_ptr->basic.ImageWidth;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    {
        ifd_info.tag = IFD_IMAGELENGTH;
        ifd_info.type = IFD_LONG;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&primary_ptr->basic.ImageLength;

        if (!Jpeg_WriteShortIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }
    if (PNULL != data_char_ptr && data_char_ptr->valid.ReferenceBlackWhite)
    {
        ifd_info.tag = IFD_REFERENCEBLACKWHITE;
        ifd_info.type = IFD_RATIONAL;
        ifd_info.count = 6;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)data_char_ptr->ReferenceBlackWhite;

        if (!Jpeg_WriteRationalIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != img_desc_ptr && img_desc_ptr->valid.Copyright)
    {
        void *desc_ptr = (void *)img_desc_ptr->Copyright;

        ifd_info.tag = IFD_COPYRIGHT;
        ifd_info.type = IFD_ASCII;
        //add an NULL char
        ifd_info.count = _GetAsciiStringSize(desc_ptr) + 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)desc_ptr;

        if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        entries++;
    }

    if (PNULL != spec_ptr)
    {
        offset = 0;
        ifd_value_offset = JPEG_ALIGN_4(ifd_value_offset);
        offset = ifd_value_offset - ifh_offset;

        ifd_info.tag = IFD_EXIFIFDPOINTER;
        ifd_info.type = IFD_LONG;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&offset;

        if (!Jpeg_WriteLongIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        ret = Jpeg_WriteExifSpecInfo(context_ptr, spec_ptr, inter_ptr, ifh_offset,
                                        ifd_value_offset, &ifd_value_offset);
        if (JPEG_SUCCESS != ret)
        {
            return ret;
        }

        entries++;
    }
#ifdef EXIF_DEBUG
    JPEG_PRINT_LOW("gps_ptr = %x \n", (uint32)gps_ptr);
#endif
    if (PNULL != gps_ptr)
    {
        offset = 0;
        ifd_value_offset = JPEG_ALIGN_4(ifd_value_offset);
        offset = ifd_value_offset - ifh_offset;

        ifd_info.tag = IFD_GPSIFDPOINTER;
        ifd_info.type = IFD_LONG;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
        ifd_info.value_ptr = (void *)&offset;

        if (!Jpeg_WriteLongIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }

        ret = Jpeg_WriteExifGPSInfo(context_ptr, gps_ptr, ifh_offset,
                                    ifd_value_offset, &ifd_value_offset);
        if (JPEG_SUCCESS != ret)
        {
            return ret;
        }

        entries++;
    }
#ifdef EXIF_DEBUG
    JPEG_PRINT_LOW("thumbnail_ptr=0x%x.",(uint32_t)thumbnail_ptr);
#endif
    /*write thumbnail image*/
    if (PNULL != thumbnail_ptr && PNULL != thumbnail_ptr->stream_buf_ptr
        && thumbnail_ptr->stream_buf_size > 0)
    {
        thumbnail_offset = ifd_value_offset - ifh_offset;

        Jpeg_SetWritePos(context_ptr, ifd_value_offset);

        if (!Jpeg_WriteBlock(context_ptr, thumbnail_ptr->stream_buf_ptr,
                                thumbnail_ptr->stream_buf_size))
        {
            return JPEG_FAILED;
        }

        ifd_value_offset = Jpeg_GetWritePos(context_ptr);

        //rewrite the thumbnail offset ifd
        Jpeg_SetWritePos(context_ptr, thumbnail_ifd_offset);
        ifd_info.tag = IFD_JPEGINTERCHANGEFORMAT;
        ifd_info.type = IFD_LONG;
        ifd_info.count = 1;
        ifd_info.value_offset.long_value = thumbnail_offset;
        ifd_info.value_ptr = (void *)&thumbnail_offset;

        if (!Jpeg_WriteLongIFD(context_ptr, &ifd_info, &thumbnail_ifd_offset, &ifd_value_offset))
        {
            return JPEG_FAILED;
        }
    }
#ifdef EXIF_DEBUG
    JPEG_PRINT_LOW("write next IFD pointer.");
#endif
    /*write next IFD pointer*/
    Jpeg_SetWritePos(context_ptr, ifd_offset);
    if (is_next_ifd_exist)
    {
        JPEG_WRITE_DATA(Jpeg_WriteL, context_ptr, ifd_value_offset - ifh_offset, return JPEG_FAILED);
    }
    else
    {
        JPEG_WRITE_DATA(Jpeg_WriteL, context_ptr, 0, return JPEG_FAILED);
    }

    //rewrite the number of entries actually write in
    Jpeg_SetWritePos(context_ptr, begin_offset);
    JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, entries, return JPEG_FAILED);

    *end_offset_ptr = ifd_value_offset;
#ifdef EXIF_DEBUG
	JPEG_PRINT_LOW("end.");
#endif
    return JPEG_SUCCESS;
}



/*****************************************************************************
**	Name :
**	Description:	write the IFH
**	Author:			Shan.he
**  Parameters:
**                  exif_info_ptr:    pointer of EXIF info structure
**                  begin_offset:     writing position
**                  end_offset_ptr:   pointer of end position when writing done,
**                                    output parameter
**	Note:           return JPEG_SUCESS if successful
*****************************************************************************/
LOCAL JPEG_RET_E Jpeg_WriteIFH(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr,
                               uint32 begin_offset,
                               uint32 *end_offset_ptr)
{
    uint32 ifd0_offset = 8;

    Jpeg_SetWritePos(context_ptr, begin_offset);

    JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, EXIF_BIG_ENDIAN, return JPEG_FAILED);
    JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, EXIF_IFH_FIX_VALUE, return JPEG_FAILED);
    JPEG_WRITE_DATA(Jpeg_WriteL, context_ptr, ifd0_offset, return JPEG_FAILED);

    *end_offset_ptr = Jpeg_GetWritePos(context_ptr);

    return JPEG_SUCCESS;
}

/*****************************************************************************
**	Name :
**	Description:	write the SOI marker
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:      pointer of context structure
**                  begin_offset:     writing position
**                  end_offset_ptr:   pointer of end position when writing done,
**                                    output parameter
**	Note:           return JPEG_SUCESS if successful
*****************************************************************************/
LOCAL JPEG_RET_E Jpeg_WriteSOIMarker(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr,
                                        uint32 begin_offset,
                                        uint32 *end_offset_ptr)
{
    Jpeg_SetWritePos(context_ptr, begin_offset);
    JPEG_WRITE_DATA(Jpeg_WriteC, context_ptr, M_MARKER, return JPEG_FAILED);
    JPEG_WRITE_DATA(Jpeg_WriteC, context_ptr, M_SOI, return JPEG_FAILED);
    *end_offset_ptr = Jpeg_GetWritePos(context_ptr);

    return JPEG_SUCCESS;
}

/*****************************************************************************
**	Name :
**	Description:	write the the APP1 marker
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:      pointer of context structure
**                  begin_offset:     writing position
**                  end_offset_ptr:   pointer of end position when writing done,
**                                    output parameter
**                  app1_length:      length of the whole app1 exclued the APP1 marker
**	Note:           return JPEG_SUCESS if successful
*****************************************************************************/
LOCAL JPEG_RET_E Jpeg_WriteAPP1Header(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr, uint32 begin_offset,
                                            uint32 *end_offset_ptr, uint16 app1_length)
{
    uint32  i           = 0;
    uint8   exif_id[6]  = "Exif";

    Jpeg_SetWritePos(context_ptr, begin_offset);
    JPEG_WRITE_DATA(Jpeg_WriteC, context_ptr, M_MARKER, return JPEG_FAILED);
    JPEG_WRITE_DATA(Jpeg_WriteC, context_ptr, M_APP1, return JPEG_FAILED);
    JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, app1_length, return JPEG_FAILED);

    for (i=0; i<6; i++)
    {
        JPEG_WRITE_DATA(Jpeg_WriteC, context_ptr, exif_id[i], return JPEG_FAILED);
    }

    *end_offset_ptr = Jpeg_GetWritePos(context_ptr);

    return JPEG_SUCCESS;
}

/*****************************************************************************
**	Name :
**	Description:	write the the APP1
**	Author:			Shan.he
**  Parameters:
**                  target_buf:       pointer of exif structure
**                  target_buf_size:  pointer of target buffer
**                  write_buf_size:   size of target buffer
**                  thumbnail_buf_ptr:thumbnail buffer pointer
**                  thumbnail_size:   thumbnail size
**                  app1_size_ptr:    pointer of APP1 size. output structure
**	Note:           return JPEG_SUCESS if successful
*****************************************************************************/
PUBLIC JPEG_RET_E Jpeg_WriteAPP1(uint8 *target_buf,
                                 uint32 target_buf_size,
                                 JINF_EXIF_INFO_T *exif_info_ptr,
                                 uint8 *thumbnail_buf_ptr,
                                 uint32 thumbnail_size,
                                 uint32 *app1_size_ptr)
{
    JPEG_WRITE_STREAM_CONTEXT_T context;//        = {0};
    JPEG_RET_E              ret             = JPEG_SUCCESS;
    uint32                  begin_offset    = 0;
    uint32                  end_offset      = 0;
    uint32                  ifh_offset      = 0;
    uint16                  app1_length     = 0;
    BOOLEAN                 is_ifd1_exist   = FALSE;

    EXIF_PRIMARY_INFO_T     ifd1_primary;//    = {0};
    EXIF_PRIMARY_INFO_T     *primary_ptr    = PNULL;
    EXIF_SPECIFIC_INFO_T    *spec_ptr       = PNULL;
    EXIF_GPS_INFO_T         *gps_ptr        = PNULL;
	EXIF_INTEROPERABILITY_INFO_T *inter_ptr	= PNULL;
    EXIF_THUMBNAIL_INFO_T   *thumbnail_ptr  = PNULL;
    EXIF_THUMBNAIL_INFO_T   thumbnail_info;// = {0};
    EXIF_PRI_DATA_STRUCT_T pri_data_struct;//g = {0};
#ifdef EXIF_DEBUG
    JPEG_PRINT_LOW("[Jpeg_WriteAPP1] start 1, %x, %x, %x, %x, %x \n", (uint32)target_buf,
		target_buf_size, (uint32)exif_info_ptr, (uint32)thumbnail_buf_ptr, thumbnail_size);
#endif
    if (PNULL == exif_info_ptr)
    {
        JPEG_PRINT_LOW("[Jpeg_WriteAPP1] exif_info_ptr is NULL \n");
        return JPEGE_INVALID_ARGUMENT;
    }

    if (NULL == target_buf || target_buf_size < 10)
    {
	JPEG_PRINT_LOW("[Jpeg_WriteAPP1] target buffer is not enough 0x%lx, %d",
							(unsigned long)target_buf, target_buf_size);
        return JPEG_MEMORY_NOT_ENOUGH;
    }

    memset(&context, 0, sizeof(JPEG_WRITE_STREAM_CONTEXT_T));
    memset(&ifd1_primary, 0, sizeof(EXIF_PRIMARY_INFO_T));
    memset(&thumbnail_info, 0, sizeof(EXIF_THUMBNAIL_INFO_T));
    memset(&pri_data_struct, 0, sizeof(EXIF_PRI_DATA_STRUCT_T));

    context.write_buf = target_buf;
    context.write_buf_size = target_buf_size;
    context.write_ptr = context.write_buf;
#if 1
    begin_offset = 10;  //reserve 10 bytes for APP1 marker, size and ID
    ret = Jpeg_WriteIFH(&context, begin_offset, &end_offset);
    if (JPEG_SUCCESS != ret)
    {
    	JPEG_PRINT_LOW("[Jpeg_WriteAPP1] Jpeg_WriteIFH failed");
        return ret;
    }

    ifh_offset = begin_offset;
    begin_offset = end_offset;
    primary_ptr = &exif_info_ptr->primary;
    spec_ptr = exif_info_ptr->spec_ptr;
    gps_ptr = exif_info_ptr->gps_ptr;
	inter_ptr = exif_info_ptr->inter_ptr;
    thumbnail_ptr = PNULL;

    if (PNULL != thumbnail_buf_ptr && thumbnail_size > 0)
    {
        is_ifd1_exist = TRUE;
    }

    ret = Jpeg_WriteExifIFD(&context, primary_ptr, spec_ptr, gps_ptr,
                            inter_ptr, thumbnail_ptr, is_ifd1_exist,ifh_offset,
                            begin_offset, &end_offset);

    if (JPEG_SUCCESS != ret)
    {
        JPEG_PRINT_LOW("[Jpeg_WriteAPP1] write IFD0 failed");
        return ret;
    }
#ifdef EXIF_DEBUG
	JPEG_PRINT_LOW("test 0");
#endif
    if (is_ifd1_exist)
    {
        BOOLEAN is_next_ifd_exist = FALSE;
#ifdef EXIF_DEBUG
		JPEG_PRINT_LOW("test 1");
#endif
        begin_offset = end_offset;

        thumbnail_ptr = &thumbnail_info;
        thumbnail_info.stream_buf_ptr = thumbnail_buf_ptr;
        thumbnail_info.stream_buf_size = thumbnail_size;

        ifd1_primary.basic.ResolutionUnit = 2;
        ifd1_primary.basic.XResolution.numerator = 72;
        ifd1_primary.basic.XResolution.denominator = 1;
        ifd1_primary.basic.YResolution.numerator = 72;
        ifd1_primary.basic.YResolution.denominator = 1;
        ifd1_primary.basic.YCbCrPositioning = 1;

		ifd1_primary.data_struct_ptr = &pri_data_struct;

        pri_data_struct.Orientation = 1;
        pri_data_struct.valid.Orientation = EXIF_VALID;
        pri_data_struct.Reserved = 6;
        pri_data_struct.valid.Reserved = EXIF_VALID;

        primary_ptr = &ifd1_primary;
        spec_ptr = PNULL;
        gps_ptr = PNULL;
		inter_ptr = PNULL;
        thumbnail_ptr = &thumbnail_info;

        thumbnail_info.stream_buf_ptr = thumbnail_buf_ptr;
        thumbnail_info.stream_buf_size = thumbnail_size;

        ret = Jpeg_WriteExifIFD(&context, primary_ptr, spec_ptr, gps_ptr, inter_ptr,
                                thumbnail_ptr, is_next_ifd_exist,ifh_offset,
                                begin_offset, &end_offset);

        if (JPEG_SUCCESS != ret)
        {
            JPEG_PRINT_LOW("[Jpeg_WriteAPP1] write IFD1 failed");
            return ret;
        }
    }
#ifdef EXIF_DEBUG
	JPEG_PRINT_LOW("test 2");
#endif
    app1_length = (uint16)end_offset - 2;   //without app1 marker
    *app1_size_ptr = app1_length + 2;

    begin_offset = 0;
    ret = Jpeg_WriteAPP1Header(&context, begin_offset, &end_offset, app1_length);
    if (JPEG_SUCCESS != ret)
    {
    	JPEG_PRINT_LOW("[Jpeg_WriteAPP1] Jpeg_WriteAPP1Header failed");
        return ret;
    }
#endif
#ifdef EXIF_DEBUG
	JPEG_PRINT_LOW("test 3");
#endif
    return JPEG_SUCCESS;
}

/*****************************************************************************
**	Name :
**	Description:	write the exif information of APP3
**	Author:			Shan.he
**  Parameters:
**                  exif_info_ptr:    exif info
**                  begin_offset:     writing position
**                  ifh_offset:       IFH position
**                  end_offset_ptr:   pointer of end position when writing done,
**                                    output parameter
**	Note:           return JPEG_SUCESS if successful
*****************************************************************************/
LOCAL JPEG_RET_E Jpeg_Write_APP3_ExifIFD(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr,
                                    EXIF_ISP_INFO_T *exif_isp_info,
                                    BOOLEAN is_next_ifd_exist,
                                    uint32 ifh_offset,
                                    uint32 begin_offset,
                                    uint32 *end_offset_ptr)
{
	uint16      entries             = 0;
	uint32      ifd_offset          = 0;
	uint32      ifd_value_offset    = 0;
	IFD_INFO_T  ifd_info;//            = {0};
	uint32      entries_offset      = 0;
	JPEG_RET_E  ret                 = JPEG_SUCCESS;

	EXIF_LONG_T offset = 0;

	if (PNULL == exif_isp_info) {
		JPEG_PRINT_LOW("PNULL == exif_isp_info, return failed!\n");
		return JPEG_FAILED;
	}

	memset(&ifd_info, 0, sizeof(IFD_INFO_T));

	//write the max number of the entries temporarily and memorize the location
	entries = 2;
	Jpeg_SetWritePos(context_ptr, begin_offset);
	JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, entries, return JPEG_FAILED);
	entries_offset = begin_offset;

	ifd_offset = Jpeg_GetWritePos(context_ptr);
	ifd_value_offset = ifd_offset + entries * IFD_HEAD_LENGTH + 4;
	ifd_value_offset = JPEG_ALIGN_4(ifd_value_offset);
	exif_isp_info->is_exif_validate = entries;
	entries = 0;
	/***********************************************************************/
	//write IFD
	/***********************************************************************/
	exif_isp_info->exif_check.app_head = EXIF_APP3;
	exif_isp_info->exif_check.status = APP3_STATUS;

	if (PNULL != exif_isp_info) {
		ifd_info.tag = IFD_APP3_ISP_INFO;
		ifd_info.type = EXIF_BYTE;
		ifd_info.count = sizeof(EXIF_ISP_INFO_T);
		ifd_info.value_offset.long_value = ifd_value_offset - ifh_offset;
		ifd_info.value_bytes = ifd_info.count;
		ifd_info.value_ptr = exif_isp_info;
		JPEG_PRINT_LOW("Jpeg_Write_APP3_ExifIFD: count = 0x%x!!!\n", ifd_info.value_bytes );
		if (!Jpeg_WriteByteIFD(context_ptr, &ifd_info, &ifd_offset, &ifd_value_offset)) {
			return JPEG_FAILED;
		}
		entries++;
	}

	/*write next IFD pointer*/
	Jpeg_SetWritePos(context_ptr, ifd_offset);
	if (is_next_ifd_exist) {
		JPEG_WRITE_DATA(Jpeg_WriteL, context_ptr, ifd_value_offset - ifh_offset, return JPEG_FAILED);
	} else {
		JPEG_WRITE_DATA(Jpeg_WriteL, context_ptr, 0, return JPEG_FAILED);
	}

	//rewrite the number of entries actually write in
	Jpeg_SetWritePos(context_ptr, begin_offset);
	JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, entries, return JPEG_FAILED);

	*end_offset_ptr = ifd_value_offset;
	JPEG_PRINT_LOW("end.");
	return JPEG_SUCCESS;
}



/*****************************************************************************
**	Name :
**	Description:	write the the APP3 marker
**	Author:			Shan.he
**  Parameters:
**                  context_ptr:      pointer of context structure
**                  begin_offset:     writing position
**                  end_offset_ptr:   pointer of end position when writing done,
**                                    output parameter
**                  app3_length:      length of the whole app3 exclued the APP3 marker
**	Note:           return JPEG_SUCESS if successful
*****************************************************************************/
LOCAL JPEG_RET_E Jpeg_WriteAPP3Header(JPEG_WRITE_STREAM_CONTEXT_T *context_ptr, uint32 begin_offset,
                                            uint32 *end_offset_ptr, uint16 app3_length)
{
	uint32  i           = 0;
	uint8   exif_id[6]  = "APP3";
	JPEG_PRINT_LOW("Jpeg_WriteAPP3Header :E!!!begin_offset = %d, app3_length = %d\n", begin_offset, app3_length);

	Jpeg_SetWritePos(context_ptr, begin_offset);
	JPEG_WRITE_DATA(Jpeg_WriteC, context_ptr, M_MARKER, return JPEG_FAILED);
	JPEG_WRITE_DATA(Jpeg_WriteC, context_ptr, M_APP3, return JPEG_FAILED);
	JPEG_WRITE_DATA(Jpeg_WriteW, context_ptr, app3_length, return JPEG_FAILED);

	for (i=0; i<6; i++) {
		JPEG_WRITE_DATA(Jpeg_WriteC, context_ptr, exif_id[i], return JPEG_FAILED);
	}

	*end_offset_ptr = Jpeg_GetWritePos(context_ptr);
	JPEG_PRINT_LOW("Jpeg_WriteAPP3Header :X!!!\n");
	return JPEG_SUCCESS;
}

/*****************************************************************************
**	Name :
**	Description:	write the the APP3
**	Author:			Shan.he
**  Parameters:
**                  target_buf:       pointer of exif structure
**                  target_buf_size:  pointer of target buffer
**                  write_buf_size:   size of target buffer
**                  thumbnail_buf_ptr:thumbnail buffer pointer
**                  thumbnail_size:   thumbnail size
**                  app3_size_ptr:    pointer of APP3 size. output structure
**	Note:           return JPEG_SUCESS if successful
*****************************************************************************/
PUBLIC JPEG_RET_E Jpeg_WriteAPP3(uint8 *target_buf,
                                 uint32 target_buf_size,
                                     EXIF_ISP_INFO_T *exif_isp_info,
                                 uint32 *app3_size_ptr)
{
	JPEG_WRITE_STREAM_CONTEXT_T context;//        = {0};
	JPEG_RET_E              ret             = JPEG_SUCCESS;
	uint32                  begin_offset    = 0;
	uint32                  end_offset      = 0;
	uint32                  ifh_offset      = 0;
	uint16                  app3_length     = 0;
	BOOLEAN                 is_ifd1_exist   = FALSE;

#ifdef EXIF_DEBUG
	JPEG_PRINT_LOW("[WriteAPP3] start 1, %x, %x, %x,  \n", (uint32)target_buf,
	target_buf_size, (uint32)exif_isp_info );
#endif
	if (PNULL == exif_isp_info) {
		JPEG_PRINT_LOW("[WriteAPP3] exif_info_ptr is NULL \n");
		return JPEGE_INVALID_ARGUMENT;
	}

	if (NULL == target_buf || target_buf_size < 10) {
		JPEG_PRINT_LOW("[WriteAPP3] target buffer is not enough 0x%lx, %d",
							(unsigned long)target_buf, target_buf_size);
		return JPEG_MEMORY_NOT_ENOUGH;
	}

	memset(&context, 0, sizeof(JPEG_WRITE_STREAM_CONTEXT_T));
	context.write_buf = target_buf;
	context.write_buf_size = target_buf_size;
	context.write_ptr = context.write_buf;

	begin_offset = 10;  //reserve 10 bytes for APP3 marker, size and ID
	ret = Jpeg_WriteIFH(&context, begin_offset, &end_offset);
	if (JPEG_SUCCESS != ret) {
		JPEG_PRINT_LOW("[WriteAPP3] Jpeg_WriteIFH failed");
		return ret;
	}

	ifh_offset = begin_offset;
	begin_offset = end_offset;

	ret = Jpeg_Write_APP3_ExifIFD(&context, exif_isp_info, is_ifd1_exist,ifh_offset, begin_offset, &end_offset);
	if (JPEG_SUCCESS != ret) {
		JPEG_PRINT_LOW("[WriteAPP3] write IFD0 failed");
		return ret;
	}

	if (is_ifd1_exist) {
		BOOLEAN is_next_ifd_exist = FALSE;
		begin_offset = end_offset;
		ret = Jpeg_Write_APP3_ExifIFD(&context, exif_isp_info, is_next_ifd_exist,ifh_offset, begin_offset, &end_offset);
		if (JPEG_SUCCESS != ret) {
			JPEG_PRINT_LOW("[WriteAPP3] write IFD1 failed");
			return ret;
		}
	}

	app3_length = (uint16)end_offset ;   //without app3 marker
	*app3_size_ptr = app3_length ;
	if (app3_length > 2) {
		app3_length -= 2;//APP3 length do not include app3 header   FF E3
	} else {
		app3_length = 0;
	}
	begin_offset = 0;
	ret = Jpeg_WriteAPP3Header(&context, begin_offset, &end_offset, app3_length);
	if (JPEG_SUCCESS != ret) {
		JPEG_PRINT_LOW("[WriteAPP3] Jpeg_WriteAPP3Header failed");
		return ret;
	}

	return JPEG_SUCCESS;
	}
/*****************************************************************************
**	Name :
**	Description:	and the EXIF info and write the output jpeg to the memory
**	Author:			Shan.he
**  Parameters:
**                  in_param_ptr:      pointer of the input parameter
**                  out_param_ptr:     pointer of the output parameter
**	Note:           return JPEG_SUCESS if successful
*****************************************************************************/
LOCAL JPEG_RET_E JPEG_AddExifToMemory(JINF_WEXIF_IN_PARAM_T *in_param_ptr,
                                      JINF_WEXIF_OUT_PARAM_T *out_param_ptr)
{
    uint8   *app1_buf_ptr   = NULL;
    uint32  app1_buf_size   = 0;
    uint32  app1_size       = 0;
    uint8   *target_buf_ptr = NULL;
    uint32  free_buf_size   = 0;
    JPEG_RET_E  ret         = JPEG_SUCCESS;
	uint32_t i = 0;
    uint8   *app3_buf_ptr           = NULL;
    uint32  app3_buf_size           = 0;
    uint32  app3_size               = 0;
    app3_buf_ptr = in_param_ptr->temp_exif_isp_buf_ptr;
    app3_buf_size = in_param_ptr->temp_exif_isp_buf_size;

    app1_buf_ptr = in_param_ptr->temp_buf_ptr;
    app1_buf_size = in_param_ptr->temp_buf_size;

    //write APP1 to temp buffer
    ret = Jpeg_WriteAPP1(app1_buf_ptr,
							app1_buf_size,
							in_param_ptr->exif_info_ptr,
							in_param_ptr->thumbnail_buf_ptr,
							in_param_ptr->thumbnail_buf_size,
							&app1_size);
#ifdef EXIF_DEBUG
	JPEG_PRINT_LOW("Jpeg_WriteAPP1 end.ret = %d.",ret);
#endif
    if (JPEG_SUCCESS != ret) {
    	JPEG_PRINT_LOW("[JPEG_AddExifToMemory] Jpeg_WriteAPP1 failed");
        return ret;
    }

    free_buf_size = (unsigned long)in_param_ptr->src_jpeg_buf_ptr
                        - (unsigned long)in_param_ptr->target_buf_ptr;

    if (free_buf_size < app1_size) {
    	JPEG_PRINT_LOW("[JPEG_AddExifToMemory] free buffer size is not enought \
							free buffer = %d, app1_size = %d",free_buf_size, app1_size);
        return JPEG_MEMORY_NOT_ENOUGH;
    }
	if (NULL != in_param_ptr->exif_isp_info && NULL != app3_buf_ptr && app3_buf_size > 0) {
	    ret = Jpeg_WriteAPP3(app3_buf_ptr,
								app3_buf_size,
								in_param_ptr->exif_isp_info,
								&app3_size);
	    JPEG_PRINT_LOW("[JPEG_AddExifToMemory] :after Jpeg_WriteAPP3, ret = %d,\
							app3_size = %d\n", ret, app3_size);
		if (JPEG_SUCCESS == ret) {
		    free_buf_size = (unsigned long)in_param_ptr->src_jpeg_buf_ptr
		                        - (unsigned long)in_param_ptr->target_buf_ptr - app1_size;

		    JPEG_PRINT_LOW("free_buf_size = %d, app1_size = %d,  app3_size = %d\n",
								free_buf_size, app1_size, app3_size);
		    if (free_buf_size < app3_size) {
				app3_size = 0;
			}
		} else {
			app3_size = 0;
	    }
	} else {
		app3_size = 0;
	}
    target_buf_ptr = in_param_ptr->src_jpeg_buf_ptr - app1_size - app3_size;

    out_param_ptr->output_buf_ptr = target_buf_ptr;
    out_param_ptr->output_size = app1_size + app3_size + in_param_ptr->src_jpeg_size;

    //write SOI marker
    *target_buf_ptr++ = M_MARKER;
    *target_buf_ptr++ = M_SOI;

    memcpy(target_buf_ptr, app1_buf_ptr, app1_size);
	if(app3_size > 0) {
	    target_buf_ptr = target_buf_ptr + app1_size;
	    memcpy(target_buf_ptr, app3_buf_ptr, app3_size);
	}
	JPEG_PRINT_LOW("end.");

    return JPEG_SUCCESS;
}

/*****************************************************************************
**	Name :
**	Description:	and the EXIF info and write the output jpeg to the file
**	Author:			Shan.he
**  Parameters:
**                  in_param_ptr:      pointer of the input parameter
**                  out_param_ptr:     pointer of the output parameter
**	Note:           return JPEG_SUCESS if successful
*****************************************************************************/
LOCAL JPEG_RET_E JPEG_AddExifToFile(JINF_WEXIF_IN_PARAM_T *in_param_ptr)
{
    uint8   *app1_buf_ptr           = NULL;
    uint32  app1_buf_size           = 0;
    uint32  app1_size               = 0;
    uint8   soi_marker[2]           = {M_MARKER, M_SOI};
    uint32  write_offset            = 0;
    uint32  write_size              = 0;
    uint32  actual_write_size       = 0;
    uint8   *write_ptr              = NULL;
    JPEG_RET_E  ret                 = JPEG_SUCCESS;
    uint8   *app3_buf_ptr           = NULL;
    uint32  app3_buf_size           = 0;
    uint32  app3_size               = 0;
    app3_buf_ptr = in_param_ptr->temp_exif_isp_buf_ptr;
    app3_buf_size = in_param_ptr->temp_exif_isp_buf_size;

    app1_buf_ptr = in_param_ptr->temp_buf_ptr;
    app1_buf_size = in_param_ptr->temp_buf_size;

    //write APP1 to temp buffer
    ret = Jpeg_WriteAPP1(app1_buf_ptr,
							app1_buf_size,
							in_param_ptr->exif_info_ptr,
							in_param_ptr->thumbnail_buf_ptr,
							in_param_ptr->thumbnail_buf_size,
							&app1_size);

    if (JPEG_SUCCESS != ret) {
    	JPEG_PRINT_LOW("[JPEG_AddExifToFile] Jpeg_WriteAPP1 failed");
        return ret;
    }

    //writer JPEG SOI marker
    write_offset = 0;
    write_size = 2;
    write_ptr = soi_marker;
    if (!in_param_ptr->wrtie_file_func(write_ptr, write_offset, write_size,
                                        &actual_write_size)
        || write_size != actual_write_size) {
    	JPEG_PRINT_LOW("[JPEG_AddExifToFile] soi marker failed, write_size= %d,\
							actual_write_size = %d", write_size, actual_write_size);
        return JPEG_FAILED;
    }

    //write the APP1
    write_offset += actual_write_size;

    write_size = app1_size;
    write_ptr = app1_buf_ptr;
    if (!in_param_ptr->wrtie_file_func(write_ptr, write_offset, write_size,
                                        &actual_write_size)
        || write_size != actual_write_size) {
    	JPEG_PRINT_LOW("[JPEG_AddExifToFile] write app1 failed, write_size= %d,\
							actual_write_size = %d", write_size, actual_write_size);
        return JPEG_FAILED;
    }
    ret = Jpeg_WriteAPP3(app3_buf_ptr,
							app3_buf_size,
							in_param_ptr->exif_isp_info,
							&app3_size);
    if (JPEG_SUCCESS == ret) {
		if (app3_size > 0) {
			write_offset += actual_write_size;
			write_size = app3_size;
			write_ptr = app3_buf_ptr;
			if (!in_param_ptr->wrtie_file_func(write_ptr, write_offset, write_size,
												&actual_write_size)
				|| write_size != actual_write_size) {
		    	JPEG_PRINT_LOW("[JPEG_AddExifToFile] write src jpeg failed, write_size= %d,\
								actual_write_size = %d", write_size, actual_write_size);
        		return JPEG_FAILED;
    		}
		}
    } else {
		JPEG_PRINT_LOW("[JPEG_AddExifToFile] Jpeg_WriteAPP3 failed");
	}
    //wrtie the main JPEG
    write_offset += actual_write_size;

    write_size = in_param_ptr->src_jpeg_size - JPEG_HEADER_BYTE;
    write_ptr = in_param_ptr->src_jpeg_buf_ptr + JPEG_HEADER_BYTE;   //exclude SOI marker
    if (!in_param_ptr->wrtie_file_func(write_ptr, write_offset, write_size,
                                        &actual_write_size)
        || write_size != actual_write_size) {
    	JPEG_PRINT_LOW("[JPEG_AddExifToFile] write src jpeg failed, write_size= %d,\
							actual_write_size = %d", write_size, actual_write_size);
        return JPEG_FAILED;
    }

    return JPEG_SUCCESS;
}


#if 1
/*****************************************************************************
**	Name :
**	Description:	write the EXIF
**	Author:			Shan.he
**  Parameters:
**                  in_param_ptr:      pointer of the input parameter
**                  out_param_ptr:     pointer of the output parameter
**	Note:           return JINF_SUCCESS if successful
*****************************************************************************/
JINF_RET_E IMGJPEG_WriteExif(JINF_WEXIF_IN_PARAM_T *in_param_ptr,
							        JINF_WEXIF_OUT_PARAM_T *out_param_ptr)
{
    JPEG_RET_E ret = JPEG_SUCCESS;


	if (NULL == in_param_ptr) {
		JPEG_PRINT_LOW("input parameter pointer is NULL!");
		return JPEGE_INVALID_ARGUMENT;
	}
	if ((NULL == in_param_ptr->exif_info_ptr)
		&&(NULL == in_param_ptr->src_jpeg_buf_ptr)&&(NULL ==in_param_ptr->temp_buf_ptr )) {
		JPEG_PRINT_LOW("input parameter error!");
		return JPEGE_INVALID_ARGUMENT;
	}
	JPEG_PRINT_LOW("[EXIFW_AddExif] src buf = 0x%lx, size = %d, thumb  buf = 0x%lx, size = %d,\
						temp buf = 0x%lx, size = %d, write file = 0x%lx",
						(unsigned long)in_param_ptr->src_jpeg_buf_ptr, in_param_ptr->src_jpeg_size,
						(unsigned long)in_param_ptr->thumbnail_buf_ptr, in_param_ptr->thumbnail_buf_size,
						(unsigned long)in_param_ptr->temp_buf_ptr, in_param_ptr->temp_buf_size,
						(unsigned long)in_param_ptr->wrtie_file_func);

    if (PNULL != in_param_ptr->wrtie_file_func) {
        ret = JPEG_AddExifToFile(in_param_ptr);
    } else {
		if(NULL == in_param_ptr->target_buf_ptr) {
			JPEG_PRINT_LOW("error:target buffer is NULL.!");
			return JPEGE_INVALID_ARGUMENT;
		}

		if ((unsigned long)in_param_ptr->src_jpeg_buf_ptr <= (unsigned long)in_param_ptr->target_buf_ptr) {
			JPEG_PRINT_LOW("[IMGJPEG_WriteExif] src jpeg buffer address must be bigger than thumbnail jpeg buffer adder");
			return JINF_MEMORY_NOT_ENOUGH;
		}

        ret = JPEG_AddExifToMemory(in_param_ptr, out_param_ptr);
    }
#ifdef EXIF_DEBUG
    JPEG_PRINT_LOW("end.");
#endif
    return JPEG_SUCCESS == ret ? JINF_SUCCESS : JINF_FAILED;
}

/*****************************************************************************
**	Name :
**	Description:	format the date/time string
**	Author:			Shan.he
**  Parameters:
**                  date_ptr:      	pointer of the date parameter
**                  time_ptr:    	pointer of the time parameter
**					date_time_ptr: 	pointer of the date time string buffer
**					date_time_size:	size of the data time string buffer, should be at least 20 bytes
**	Note:           return JINF_SUCCESS if successful
*****************************************************************************/
JINF_RET_E IMGJPEG_FormatDateTime(JINF_DATE_T *date_ptr, JINF_TIME_T *time_ptr,
										  EXIF_ASCII_T *date_time_ptr, uint32 date_time_size)
{
	if (date_time_size < 20)
	{
		return JINF_MEMORY_NOT_ENOUGH;
	}

	sprintf((char *)date_time_ptr, "%04d:%02d:%02d %02d:%02d:%02d", date_ptr->year, date_ptr->month, date_ptr->day,
				time_ptr->hour, time_ptr->minute, time_ptr->second);

	return JINF_SUCCESS;
}
#endif



