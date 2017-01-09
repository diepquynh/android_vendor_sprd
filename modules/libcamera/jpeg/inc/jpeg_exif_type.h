/******************************************************************************
 ** File Name:		jpeg_exif.h                                               *
 ** Author:			Shan.He		                                          		*
 ** DATE:			10/24/2009                                                *
 ** Copyright:      2008 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:	Refer to "Exchangeable image file formator digital		  *
 					still cameras: Exif Version 2.2" for more details.        *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** -------------------------------------------------------------------------*
 ** DATE              NAME             DESCRIPTION                          *
 ** 2009-10-24		  Shan.He		   Created								*
 ******************************************************************************/


#ifndef _JPEG_EXIF_TYPE_H_
#define _JPEG_EXIF_TYPE_H_

#include "sci_types.h"
#include "jpeg_exif_header.h"

#define EXIF_BIG_ENDIAN                     0x4D4D
#define EXIF_LITTLE_ENDIAN                  0x4949
#define EXIF_IFH_FIX_VALUE                  0x002a
#define IFD_HEAD_LENGTH                     12

#define IFD_BYTE 		EXIF_BYTE
#define IFD_ASCII		EXIF_ASCII
#define IFD_SHORT		EXIF_SHORT
#define IFD_LONG		EXIF_LONG
#define IFD_RATIONAL	EXIF_RATIONAL
#define IFD_UNDEFINED	EXIF_UNDEFINED
#define IFD_SLONG		EXIF_SLONG
#define IFD_SRATIONAL	EXIF_SRATIONAL

//EXIF IFD tags
typedef enum
{
	//GPS tag ID
	IFD_GPSVERSIONID					= 0x00,
	IFD_GPSLATITUDEREF					= 0x01,
	IFD_GPSLATITUDE						= 0x02,
	IFD_GPSLONGITUDEREF					= 0x03,
	IFD_GPSLONGITUDE					= 0x04,
	IFD_GPSALTITUDEREF					= 0x05,
	IFD_GPSALTITUDE						= 0x06,
	IFD_GPSTIMESTAMP					= 0x07,
	IFD_GPSSATELLITES					= 0x08,
	IFD_GPSSTATUS						= 0x09,
	IFD_GPSMEASUREMODE					= 0x0a,
	IFD_GPSDOP							= 0x0b,
	IFD_GPSSPEEDREF						= 0x0c,
	IFD_GPSSPEED						= 0x0d,
	IFD_GPSTRACKREF						= 0x0e,
	IFD_GPSTRACK						= 0x0f,


	IFD_GPSIMGDIRECTIONREF				= 0x10,
	IFD_GPSIMGDIRECTION					= 0x11,
	IFD_GPSMAPDATUM						= 0x12,
	IFD_GPSDESTLATITUDEREF				= 0x13,
	IFD_GPSDESTLATITUDE					= 0x14,
	IFD_GPSDESTLONGITUDEREF				= 0x15,
	IFD_GPSDESTLONGITUDE				= 0x16,
	IFD_GPSDESTBEARINGREF				= 0x17,
	IFD_GPSDESTBEARING					= 0x18,
	IFD_GPSDESTDISTANCEREF				= 0x19,
	IFD_GPSDESTDISTANCE					= 0x1a,

	IFD_GPSPROCESSINGMETHOD				= 0x1b,
	IFD_GPSAREAINFORMATION				= 0x1c,
	IFD_GPSDATESTAMP					= 0x1d,
	IFD_GPSDIFFERENTIAL					= 0x1e,

    //IFD attribution
	IFD_IMAGEWIDTH                      = 0x100,
	IFD_IMAGELENGTH                     = 0x101,
    IFD_COMPRESSION                     = 0x103,    //only used in IFD of thumbnail jpeg
    IFD_IMAGEDESCRIPTION                = 0x10e,
    IFD_MAKE                            = 0x10f,
    IFD_MODEL                           = 0x110,
    IFD_ORIENTATION                     = 0x112,
    IFD_XRESOLUTION                     = 0x11A,
    IFD_YRESOLUTION                     = 0x11B,
    IFD_RESOLUTIONUNIT                  = 0x128,
    IFD_TRANSFERFUNCTION                = 0x12D,
    IFD_SOFTWARE                        = 0x131,
    IFD_DATETIME                        = 0x132,
    IFD_ARTIST                          = 0x13B,
    IFD_WHITEPOINT                      = 0x13E,
    IFD_PRIMARYCHROMATICITIES           = 0x13F,
    IFD_JPEGINTERCHANGEFORMAT           = 0x201,   //only used in IFD of thumbnail jpeg
    IFD_JPEGINTERCHANGEFORMATLENGTH     = 0x202,   //only used in IFD of thumbnail jpeg
    IFD_YCBCRCOEFFICIENTS				= 0x211,
    IFD_YCBCRPOSITION                   = 0x213,
    IFD_REFERENCEBLACKWHITE				= 0x214,
    IFD_COPYRIGHT                       = 0x8298,
    IFD_EXIFIFDPOINTER                  = 0x8769,
    IFD_GPSIFDPOINTER                   = 0x8825,

    //EXIF IFD
    IFD_EXPOSURETIME                    = 0x829A,
    IFD_FNUMBER                         = 0x829D,
    IFD_EXPOSUREPROGRAM                 = 0x8822,
    IFD_SPECTRALSENSITIVITY				= 0x8824,
    IFD_ISOSPEEDRATINGS                 = 0x8827,
    IFD_OECF							= 0x8828,

    IFD_EXIFVERSION                     = 0x9000,
    IFD_DATETIMEORIGINAL                = 0x9003,
    IFD_DATETIMEDIGITILIZED             = 0x9004,
    IFD_COMPONENTSCONFIGURATION         = 0x9101,
    IFD_COMPRESSEDBITSPERPIXEL          = 0x9102,
    IFD_SHUTTERSPEEDVALUE				= 0x9201,
    IFD_APERTUREVALUE					= 0x9202,
    IFD_BRIGHTNESSVALUE					= 0x9203,
    IFD_EXPOSUREBIASVALUE               = 0x9204,
    IFD_MAXAPERTUREVALUE               	= 0x9205,
    IFD_SUBJECTDISTANCE					= 0x9206,
    IFD_METERINGMODE                    = 0x9207,
    IFD_LIGHTSOURCE                     = 0x9208,
    IFD_FLASH                           = 0x9209,
    IFD_FOCALLENGTH                     = 0x920A,
    IFD_SUBJECTAREA						= 0x9214,
    IFD_MAKERNOTE						= 0x927C,
    IFD_USERCOMMENT                     = 0x9286,
    IFD_SUBSECTIME						= 0x9290,
    IFD_SUBSECTIMEORIGINAL				= 0x9291,
    IFD_SUBSECTIMEDIGITILIZED			= 0x9292,

    IFD_FLASHPIXVERSION                	= 0xA000,
    IFD_COLORSPACE                      = 0xA001,
    IFD_PIXELXDIMENSION                 = 0xA002,
    IFD_PIXELYDIMENSION                 = 0xA003,
    IFD_RELATEDSOUNDFILE				= 0xA004,
    IFD_INTEROPERABILITYIFDPOINTER      = 0xA005,

	IFD_FLASHENERGY						= 0xA20B,
	IFD_SPATIALFREQUENCYRESPONSE		= 0xA20C,
	IFD_FOCALPLANEXRESOLUTION		 	= 0xA20E,
	IFD_FOCALPLANEYRESOLUTION			= 0xA20F,
	IFD_FOCALPLANERESOLUTIONUNIT		= 0xA210,
	IFD_SUBJECTLOCATION					= 0xA214,
	IFD_EXPOSUREINDEX					= 0xA215,
	IFD_SENSINGMETHOD					= 0xA217,

    IFD_FILESOURCE                      = 0xA300,
    IFD_SCENETYPE                       = 0xA301,
    IFD_CFAPATTERN						= 0xA302,

    IFD_CUSTOMRENDERED                  = 0xA401,
    IFD_EXPOSUREMODE                    = 0xA402,
    IFD_WHITEBALANCE                    = 0xA403,
	IFD_DIGITALZOOMRATIO				= 0xA404,
	IFD_FOCALLENGTHIN35MMFILM			= 0xA405,
    IFD_SCENECAPTURETYPE                = 0xA406,
	IFD_GAINCONTROL						= 0xA407,
	IFD_CONTRAST						= 0xA408,
	IFD_SATURATION						= 0xA409,
	IFD_SHARPNESS						= 0xA40A,
	IFD_DEVICESETTINGDESCRIPTION		= 0xA40B,
	IFD_SUBJECTDISTANCERANGE			= 0xA40C,

	IFD_IMAGEUNIQUEID					= 0xa420,

    //interoperability IFD
    IFD_INTEROPERABILITYINDEX           = 0x0001,
	IFD_INTEROPERABILITYVERSION         = 0x0002,
	IFD_APP3_ISP_INFO                   = 0XCFFF
}EXIF_IFD_TAG;

//IFD info
typedef struct ifd_info_tag
{
    uint16      tag;                //tag to identify the IFD
    uint16      type;               //value type of the IFD
    uint32      count;              //value count, do not equal the bytes of the value
    union
    {
        uint8   byte_value[4];
        uint16  short_value[2];
        uint32  long_value;
    }value_offset;                  //value or offset being far away from the IFH
    uint32      value_bytes;        //value bytes of the IFD

    void        *value_ptr;         //value pointer
}IFD_INFO_T;

//tags relating to the offset information
typedef struct exif_pri_offset_valid_tag
{
    uint32        JPEGInterchangeFormat         :1;
    uint32        JPEGInterchangeFormatLenght   :1;
    uint32        ExifIFDPointer                :1;
    uint32        GPSInfoIFDPointer             :1;
    uint32        NextIFDPointer                :1;
}EXIF_PRI_OFFSET_VALID_T;

typedef struct exif_pri_offset_tag
{
    EXIF_PRI_OFFSET_VALID_T         valid;

    EXIF_LONG_T                     JPEGInterchangeFormat;
    EXIF_LONG_T                     JPEGInterchangeFormatLenght;
    EXIF_LONG_T                     ExifIFDPointer;
    EXIF_LONG_T                     GPSInfoIFDPointer;
    EXIF_LONG_T                     NextIFDPointer;
}EXIF_PRI_OFFSET_T;

typedef struct exif_spec_offset_valid_tag
{
    uint32        InteroperabilityIFDPointer     :1;
}EXIF_SPEC_OFFSET_VALID_T;

typedef struct exif_spec_offset_tag
{
	EXIF_SPEC_OFFSET_VALID_T	valid;

	EXIF_LONG_T					InteroperabilityIFDPointer;
}EXIF_SPEC_OFFSET_T;

#endif