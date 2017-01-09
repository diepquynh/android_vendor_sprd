#ifndef _JPEG_EXIF_HEADER_H_
#define _JPEG_EXIF_HEADER_H_
#if 0
/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/
#include "sci_types.h"

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
    extern   "C"
    {
#endif

#define NEW_INTERFACE

#define LENGTH_TEXT_PIMAGEDESCRIPTION		((uint32) 0x00000100)
#define LENGTH_TEXT_PMAKE						((uint32) 0x00000100)
#define LENGTH_TEXT_PMODEL					((uint32) 0x00000100)
#define LENGTH_TEXT_PARTIST					((uint32) 0x00000100)
#define LENGTH_TEXT_PSOFTWAREVALUE		((uint32) 0x00000100)

#define LENGTH_TEXT_PUSERCOMMENT			((uint32) 0x00000100)
#define LENGTH_TEXT_PCOPYRIGHT				((uint32) 0x00000100)
#define LENGTH_TEXT_PDATETIME				((uint32) 0x00000050)
#define LENGTH_TEXT_PDATETIMEORIGINAL		((uint32) 0x00000050)
#define LENGTH_TEXT_PDATETIMEDIGITILIZED	((uint32) 0x00000050)

/*
 * Data Type struct
 */

/**---------------------------------------------------------------------------*
 **                         Exif related structure		                      *
 **---------------------------------------------------------------------------*/
typedef enum
{
    EXIF_BYTE            = 1,
    EXIF_ASCII           = 2,
    EXIF_SHORT           = 3,
    EXIF_LONG            = 4,
    EXIF_RATIONAL        = 5,
    EXIF_UNDEFINED       = 7,
    EXIF_SLONG           = 9,
    EXIF_SRATIONAL       = 10
}EXIF_TYPE_E;

typedef uint8	EXIF_BYTE_T;
typedef uint8	EXIF_ASCII_T;
typedef uint16	EXIF_SHORT_T;
typedef uint32	EXIF_LONG_T;
typedef uint8	EXIF_UNDEFINED_T;
typedef int32	EXIF_SLONG_T;

typedef struct
{
	uint32		numerator;
	uint32		denominator;
}EXIF_RATIONAL_T;

typedef struct
{
	int32		numerator;
	int32		denominator;
}EXIF_SRATIONAL_T;

typedef struct
{
    EXIF_TYPE_E	type;       //type of the content
	//void 		*ptr;       //content buffer
	char			ptr[256];
	uint32		count;		//count of content by type
    uint32      size;       //size of content buffer by byte
}EXIF_CUSTOM_T;


#define MAX_ASCII_STR_SIZE     256
#define EXIF_VALID              1

//Tags relating to image data structure. Mandatory
typedef struct exif_pri_basic_tag
{
    EXIF_SHORT_T        YCbCrPositioning;
    EXIF_RATIONAL_T     XResolution;
    EXIF_RATIONAL_T     YResolution;
    EXIF_SHORT_T        ResolutionUnit;
}EXIF_PRI_BASIC_T;

//Tags relating to image data structure. Optional
typedef struct exif_pri_data_struct_valid_tag
{
    uint32              Orientation     :1;
    uint32              Reserved        :1;
}EXIF_PRI_DATA_STRUCT_VALID_T;

typedef struct exif_pri_data_struct_tag
{
    EXIF_PRI_DATA_STRUCT_VALID_T    valid;

    EXIF_SHORT_T                    Orientation;
    EXIF_SHORT_T                    Reserved;                   //reserved. ignore
}EXIF_PRI_DATA_STRUCT_T;

//Tags relating to image data characteristics. Optional
typedef struct exif_data_char_valid_tag
{
    uint32        TransferFunction              :1;
    uint32        WhitePoint                    :1;
    uint32        PrimaryChromaticities         :1;
    uint32        YCbCrCoefficients             :1;
    uint32        ReferenceBlackWhite           :1;
}EXIF_PRI_DATA_CHAR_VALID_T;

typedef struct exif_data_char_tag
{
    EXIF_PRI_DATA_CHAR_VALID_T      valid;

    EXIF_SHORT_T        TransferFunction[768];
    EXIF_RATIONAL_T     WhitePoint[2];
    EXIF_RATIONAL_T     PrimaryChromaticities[6];
    EXIF_RATIONAL_T     YCbCrCoefficients[3];
    EXIF_RATIONAL_T     ReferenceBlackWhite[6];
}EXIF_PRI_DATA_CHAR_T;

//Tag relating to image descriptions. Optional
typedef struct exif_pri_desc_valid_tag
{
    uint32        DateTime              :1;
    uint32        ImageDescription      :1;
    uint32        Make                  :1;
    uint32        Model                 :1;
    uint32        Software              :1;
    uint32        Artist                :1;
    uint32        Copyright             :1;
}EXIF_PRI_DESC_VALID_T;

typedef struct exif_pri_desc_tag
{
    EXIF_PRI_DESC_VALID_T   valid;

    EXIF_ASCII_T        DateTime[20];
    EXIF_ASCII_T        ImageDescription[MAX_ASCII_STR_SIZE];
    EXIF_ASCII_T        Make[MAX_ASCII_STR_SIZE];
    EXIF_ASCII_T        Model[MAX_ASCII_STR_SIZE];
    EXIF_ASCII_T        Software[MAX_ASCII_STR_SIZE];
    EXIF_ASCII_T        Artist[MAX_ASCII_STR_SIZE];
    EXIF_ASCII_T        Copyright[MAX_ASCII_STR_SIZE];
}EXIF_PRI_DESC_T;

typedef struct exif_primary_tag
{
    EXIF_PRI_BASIC_T                basic;                         //Mandatory
    EXIF_PRI_DATA_STRUCT_T          *data_struct_ptr;              //Optional. Set NULL to ignore it
    EXIF_PRI_DATA_CHAR_T            *data_char_ptr;                //Optional. Set NULL to ignore it
    EXIF_PRI_DESC_T                 *img_desc_ptr;                 //Optional. Set NULL to ignore it
    uint32                          reserved;                      //reserved
}EXIF_PRIMARY_INFO_T;

//Tags relating to image data characteristics and image configuration. Mandatory
typedef struct exif_spec_basic_tag
{
    EXIF_SHORT_T        ColorSpace;
    EXIF_UNDEFINED_T    ComponentsConfiguration[4];
    EXIF_LONG_T         PixelXDimension;
    EXIF_LONG_T         PixelYDimension;
}EXIF_SPEC_BASIC_T;

//Tags relating to version, read only.
typedef struct exif_spec_version_tag
{
    EXIF_UNDEFINED_T    ExifVersion[4];
    EXIF_UNDEFINED_T    FlashpixVersion[4];
}EXIF_SPEC_VERSION_T;

//Tags relating to image configuration. Optional
typedef struct exif_spec_img_config_valid_tag
{
    uint32        CompressedBitsPerPixel         :1;
}EXIF_SPEC_IMG_CONFIG_VALID_T;

typedef struct exif_spec_img_config_tag
{
    EXIF_SPEC_IMG_CONFIG_VALID_T    valid;
    EXIF_RATIONAL_T                 CompressedBitsPerPixel;
}EXIF_SPEC_IMG_CONFIG_T;

//Tags relating to user information. Optional
typedef struct exif_spec_user_valid_tag
{
    uint32        MakerNote                     :1;
    uint32        UserComment                   :1;
}EXIF_SPEC_USER_VALID_T;

typedef struct exif_spec_user_tag
{
    EXIF_SPEC_USER_VALID_T  	valid;

    EXIF_CUSTOM_T            	MakerNote;
    EXIF_CUSTOM_T            	UserComment;
}EXIF_SPEC_USER_T;

//Tags relating to related file information. Optional
typedef struct exif_spec_related_file_valid_tag
{
    uint32        RelatedSoundFile              :1;
}EXIF_SPEC_RELATED_FILE_VALID_T;

typedef struct exif_spec_related_file_tag
{
    EXIF_SPEC_RELATED_FILE_VALID_T		valid;
    EXIF_ASCII_T						RelatedSoundFile[13];
}EXIF_SPEC_RELATED_FILE_T;

//Tags relating to date and time. Optional
typedef struct exif_spec_date_time_valid_tag
{
    uint32          DateTimeOriginal            :1;
    uint32          DateTimeDigitized           :1;
    uint32          SubSecTime                  :1;
    uint32          SubSecTimeOriginal          :1;
    uint32          SubSecTimeDigitized         :1;
}EXIF_SPEC_DATE_TIME_VALID_T;

typedef struct exif_spec_date_time_tag
{
    EXIF_SPEC_DATE_TIME_VALID_T     valid;

    EXIF_ASCII_T        DateTimeOriginal[20];
    EXIF_ASCII_T        DateTimeDigitized[20];
    EXIF_ASCII_T        SubSecTime[MAX_ASCII_STR_SIZE];
    EXIF_ASCII_T        SubSecTimeOriginal[MAX_ASCII_STR_SIZE];
    EXIF_ASCII_T        SubSecTimeDigitized[MAX_ASCII_STR_SIZE];
}EXIF_SPEC_DATE_TIME_T;

//Tags relating to picture-taking conditions. Optional
typedef struct exif_spec_pic_taking_cond_valid_0_tag
{
    uint32              ExposureTime            :1;
    uint32              FNumber                 :1;
    uint32              ExposureProgram         :1;
    uint32              SpectralSensitivity     :1;
    uint32              ISOSpeedRatings         :1;

    uint32				OECF                    :1;
    uint32		        ShutterSpeedValue       :1;
	uint32			    ApertureValue           :1;
	uint32		        BrightnessValue         :1;
	uint32		        ExposureBiasValue       :1;

	uint32			    MaxApertureValue        :1;
	uint32			    SubjectDistance         :1;
	uint32			    MeteringMode            :1;
	uint32			    LightSource             :1;
	uint32			    Flash                   :1;

	uint32			    FocalLength             :1;
	uint32			    SubjectArea             :1;
	uint32			    FlashEnergy             :1;
	uint32				SpatialFrequencyResponse:1;
	uint32			    FocalPlaneXResolution   :1;

	uint32			    FocalPlaneYResolution   :1;
	uint32			    FocalPlaneResolutionUnit:1;
	uint32			    SubjectLocation         :1;
	uint32			    ExposureIndex           :1;
	uint32			    SensingMethod           :1;

	uint32		        FileSource              :1;
	uint32		        SceneType               :1;
	uint32				CFAPattern              :1;
	uint32			    CustomRendered          :1;
	uint32			    ExposureMode            :1;
	uint32			    WhiteBalance                :1;
	uint32			    DigitalZoomRatio            :1;
	uint32			    FocalLengthIn35mmFilm       :1;
	uint32			    SceneCaptureType            :1;
	uint32			    GainControl                 :1;

	uint32			    Contrast                    :1;
	uint32			    Saturation                  :1;
	uint32			    Sharpness                   :1;
	uint32			    DeviceSettingDescription    :1;
	uint32			    SubjectDistanceRange        :1;
}EXIF_SPEC_PIC_TAKING_COND_VALID_T;

typedef struct exif_spec_pic_taking_cond_tag
{
    EXIF_SPEC_PIC_TAKING_COND_VALID_T     valid;

	EXIF_RATIONAL_T			ExposureTime;
	EXIF_RATIONAL_T			FNumber;
	EXIF_SHORT_T			ExposureProgram;
	EXIF_ASCII_T			SpectralSensitivity[MAX_ASCII_STR_SIZE];
	EXIF_CUSTOM_T				ISOSpeedRatings;	//Modified

	EXIF_CUSTOM_T				OECF;
	EXIF_SRATIONAL_T		ShutterSpeedValue;
	EXIF_RATIONAL_T			ApertureValue;
	EXIF_SRATIONAL_T		BrightnessValue;
	EXIF_SRATIONAL_T		ExposureBiasValue;

	EXIF_RATIONAL_T			MaxApertureValue;
	EXIF_RATIONAL_T			SubjectDistance;
	EXIF_SHORT_T			MeteringMode;
	EXIF_SHORT_T			LightSource;
	EXIF_SHORT_T			Flash;

	EXIF_RATIONAL_T			FocalLength;
	EXIF_CUSTOM_T			    SubjectArea;			// 2 or 3 or 4
	EXIF_RATIONAL_T			FlashEnergy;
	EXIF_CUSTOM_T				SpatialFrequencyResponse;
	EXIF_RATIONAL_T			FocalPlaneXResolution;

	EXIF_RATIONAL_T			FocalPlaneYResolution;
	EXIF_SHORT_T			FocalPlaneResolutionUnit;
	EXIF_SHORT_T			SubjectLocation[2];
	EXIF_RATIONAL_T			ExposureIndex;
	EXIF_SHORT_T			SensingMethod;

	EXIF_UNDEFINED_T		FileSource;
	EXIF_UNDEFINED_T		SceneType;
	EXIF_CUSTOM_T				CFAPattern;
	EXIF_SHORT_T			CustomRendered;
	EXIF_SHORT_T			ExposureMode;

	EXIF_SHORT_T			WhiteBalance;
	EXIF_RATIONAL_T			DigitalZoomRatio;
	EXIF_SHORT_T			FocalLengthIn35mmFilm;
	EXIF_SHORT_T			SceneCaptureType;
	EXIF_SHORT_T			GainControl;

	EXIF_SHORT_T			Contrast;
	EXIF_SHORT_T			Saturation;
	EXIF_SHORT_T			Sharpness;
	EXIF_CUSTOM_T				DeviceSettingDescription;
	EXIF_SHORT_T			SubjectDistanceRange;
}EXIF_SPEC_PIC_TAKING_COND_T;


//Other tags. Optional
typedef struct exif_spec_other_valid_t
{
    uint32			ImageUniqueID               :1;
}EXIF_SPEC_OTHER_VALID_T;

typedef struct exif_spec_other_tag
{
    EXIF_SPEC_OTHER_VALID_T valid;
    EXIF_ASCII_T            ImageUniqueID[33];
}EXIF_SPEC_OTHER_T;

//structure of EXIF-specific information
typedef struct exif_specific_info_tag
{
    EXIF_SPEC_BASIC_T               basic;                      //Mandatory
    EXIF_SPEC_VERSION_T				*version_ptr;				//Optional. Read only. Set NULL to ignore it
    EXIF_SPEC_IMG_CONFIG_T          *img_config_ptr;            //Optional. Set NULL to ignore it
    EXIF_SPEC_USER_T                *user_ptr;                  //Optional. Set NULL to ignore it
    EXIF_SPEC_RELATED_FILE_T        *related_file_ptr;          //Optional. Set NULL to ignore it
    EXIF_SPEC_DATE_TIME_T           *date_time_ptr;             //Optional. Set NULL to ignore it
    EXIF_SPEC_PIC_TAKING_COND_T     *pic_taking_cond_ptr;       //Optional. Set NULL to ignore it
    EXIF_SPEC_OTHER_T               *other_ptr;                 //Optional. Set NULL to ignore it
}EXIF_SPECIFIC_INFO_T;

//structure of GPS information
typedef struct exif_gps_valid_tag
{
	uint32			        GPSVersionID            :1;							//read only
	uint32	            	GPSLatitudeRef          :1;
	uint32		            GPSLatitude             :1;
	uint32		            GPSLongitudeRef         :1;
	uint32		            GPSLongitude            :1;
	uint32			        GPSAltitudeRef          :1;
	uint32		            GPSAltitude             :1;
	uint32		            GPSTimeStamp            :1;
	uint32		            GPSSatellites           :1;
	uint32		            GPSStatus               :1;
	uint32		            GPSMeasureMode          :1;
	uint32		            GPSDOP                  :1;
	uint32		            GPSSpeedRef             :1;
	uint32		            GPSSpeed                :1;
	uint32		            GPSTrackRef             :1;
	uint32		            GPSTrack                :1;
	uint32		            GPSImgDirectionRef      :1;
	uint32		            GPSImgDirection         :1;
	uint32		            GPSMapDatum             :1;
	uint32		            GPSDestLatitudeRef      :1;
	uint32		            GPSDestLatitude         :1;
	uint32		            GPSDestLongitudeRef     :1;
	uint32		            GPSDestLongitude        :1;
	uint32		            GPSDestBearingRef       :1;
	uint32		            GPSDestBearing          :1;
	uint32		            GPSDestDistanceRef      :1;
	uint32		            GPSDestDistance         :1;
	uint32	                GPSProcessingMethod     :1;
	uint32			        GPSAreaInformation      :1;
	uint32		            GPSDateStamp            :1;
	uint32		            GPSDifferential         :1;
}EXIF_GPS_VALID_T;

typedef struct exif_gps_tag
{
    EXIF_GPS_VALID_T                valid;

	EXIF_BYTE_T			            GPSVersionID[4];							//read only
	EXIF_ASCII_T	            	GPSLatitudeRef[2];
	EXIF_RATIONAL_T		            GPSLatitude[3];
	EXIF_ASCII_T		            GPSLongitudeRef[2];
	EXIF_RATIONAL_T		            GPSLongitude[3];
	EXIF_BYTE_T			            GPSAltitudeRef;
	EXIF_RATIONAL_T		            GPSAltitude;
	EXIF_RATIONAL_T		            GPSTimeStamp[3];
	EXIF_ASCII_T		            GPSSatellites[MAX_ASCII_STR_SIZE];
	EXIF_ASCII_T		            GPSStatus[2];
	EXIF_ASCII_T		            GPSMeasureMode[2];
	EXIF_RATIONAL_T		            GPSDOP;
	EXIF_ASCII_T		            GPSSpeedRef[2];
	EXIF_RATIONAL_T		            GPSSpeed;
	EXIF_ASCII_T		            GPSTrackRef[2];
	EXIF_RATIONAL_T		            GPSTrack;
	EXIF_ASCII_T		            GPSImgDirectionRef[2];
	EXIF_RATIONAL_T		            GPSImgDirection;
	EXIF_ASCII_T		            GPSMapDatum[MAX_ASCII_STR_SIZE];
	EXIF_ASCII_T		            GPSDestLatitudeRef[2];
	EXIF_RATIONAL_T		            GPSDestLatitude[3];
	EXIF_ASCII_T		            GPSDestLongitudeRef[2];
	EXIF_RATIONAL_T		            GPSDestLongitude[3];
	EXIF_ASCII_T		            GPSDestBearingRef[2];
	EXIF_RATIONAL_T		            GPSDestBearing;
	EXIF_ASCII_T		            GPSDestDistanceRef[2];
	EXIF_RATIONAL_T		            GPSDestDistance;
    EXIF_CUSTOM_T                      GPSProcessingMethod;
	EXIF_CUSTOM_T			            GPSAreaInformation;
	EXIF_ASCII_T		            GPSDateStamp[11];
	EXIF_SHORT_T		            GPSDifferential;
}EXIF_GPS_INFO_T;

typedef struct exif_interoperability_valid_tag
{
    uint32          InteroperabilityIndex            :1;
}EXIF_INTEROPERABILITY_VALID_T;

typedef struct exif_interoperability_info_t
{
	EXIF_INTEROPERABILITY_VALID_T 			valid;

	EXIF_CUSTOM_T							InteroperabilityIndex;
}EXIF_INTEROPERABILITY_INFO_T;

//structure of EXIF information
typedef struct exif_info_tag
{
    EXIF_PRIMARY_INFO_T             primary;            //Mandatory
    EXIF_SPECIFIC_INFO_T            *spec_ptr;          //Optional. Set NULL to ignore it
    EXIF_GPS_INFO_T                 *gps_ptr;           //Optional. Set NULL to ignore it
    EXIF_INTEROPERABILITY_INFO_T	*inter_ptr;			//Optional. Set NULL to ignore it
}JINF_EXIF_INFO_T;

/**---------------------------------------------------------------------------*
 **                         Function Prototypes
 **---------------------------------------------------------------------------*/


/****************************************************************************/
/* Purpose:	get informaton,such as exif,basic image info...				 	*/
/* Author:	frank.yang														*/
/* Input:      																*/
/*			in_param_ptr	--points to in parameter						*/
/* Output:	info_ptr        --jpeg basic information						*/
/* Return:	operation results												*/
/* Note:    																*/
/****************************************************************************/
//PUBLIC  JINF_RET_E IMGJPEG_GetInfo(JINF_GET_INFO_IN_T *in_param_ptr,
//								 		JINF_INFO_T *info_ptr);

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
    }
#endif
#endif
#endif // _JPEG_EXIF_HEADER_H_



