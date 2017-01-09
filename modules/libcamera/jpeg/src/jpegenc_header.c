/******************************************************************************
 ** File Name:      JpegEnc_quant.c                                            *
 ** Author:         yi.wang													  *
 ** DATE:           07/12/2007                                                *
 ** Copyright:      2007 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    Initialize the encoder									  *
 ** Note:           None                                                      *
******************************************************************************/
/******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 07/12/2007     yi.wang	         Create.                                  *
******************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8830_video_header.h"
#include "exif_writer.h"

#if !defined(_SIMULATION_)
//#include "os_api.h"
#endif
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C"
    {
#endif

#if defined(JPEG_ENC)
//////////////////////////////////////////////////////////////////////////

/* Emit a marker code */
PUBLIC void PutMarker(JPEG_MARKER_E mark)
{
	JPEGFW_PutC(0xFF);
	JPEGFW_PutC((uint8)mark);
}

PUBLIC void OutPutRstMarker(void)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();

	SCI_ASSERT(jpeg_fw_codec != PNULL);

	JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "BSM,Polling the bsm rfifo can read/write");

	if (jpeg_fw_codec->RST_Count > M_RST7)
	{
		jpeg_fw_codec->RST_Count = M_RST0;
	}
	JPEGFW_PutC(0xFF);
	JPEGFW_PutC(jpeg_fw_codec->RST_Count++);
}

PUBLIC JPEG_RET_E PutAPP0(void)
{
	/*
	* Length of APP0 block	(2 bytes)
	* Block ID			(4 bytes - ASCII "JFIF")
	* Zero byte			(1 byte to terminate the ID string)
	* Version Major, Minor	(2 bytes - 0x01, 0x01)
	* Units			(1 byte - 0x00 = none, 0x01 = inch, 0x02 = cm)
	* Xdpu			(2 bytes - dots per unit horizontal)
	* Ydpu			(2 bytes - dots per unit vertical)
	* Thumbnail X size		(1 byte)
	* Thumbnail Y size		(1 byte)
	*/
	if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE + BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "BSM,Polling the bsm rfifo can read/write"))
	{
		return JPEG_FAILED;
	}

	PutMarker(M_APP0);				/*marker*/
	JPEGFW_PutW((2+4+1+2+1+2+2+1+1));	/*length*/
	JPEGFW_PutC('J');					/*identifier*/
	JPEGFW_PutC('F');
	JPEGFW_PutC('I');
	JPEGFW_PutC('F');
	JPEGFW_PutC(0);
	JPEGFW_PutC(1);					/*major version*/
	JPEGFW_PutC(1);					/*minor version*/
	JPEGFW_PutC(0);					/*pixel size info, no unit*/
	JPEGFW_PutW(1);					/*x_density = 1*/
	JPEGFW_PutW(1);					/*y_density = 1*/
	JPEGFW_PutC(0);					/*no thumbnail image*/
	JPEGFW_PutC(0);

	return JPEG_SUCCESS;
}

PUBLIC JPEG_RET_E JPEG_HWWriteHeadForThumbnail(void)
{
	if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "bsm: polling bsm status"))
	{
		return JPEG_FAILED;
	}

	/*put SOI*/
	PutMarker(M_SOI);

	/*put APP*/
	if(PutAPP0() != JPEG_SUCCESS)
	{
		return JPEG_FAILED;
	}

	/*put quant tbl*/
	if(PutQuantTbl() != JPEG_SUCCESS)
	{
		return JPEG_FAILED;
	}

	/*put SOF0*/
	if(PutSOF0() != JPEG_SUCCESS)
	{
		return JPEG_FAILED;
	}
	/*put huffman tbl*/
	if(PutHuffTbl()!= JPEG_SUCCESS)
	{
		return JPEG_FAILED;
	}

	/*put DRI*/
	if(WriteDRI()!= JPEG_SUCCESS)
	{
		return JPEG_FAILED;
	}
	/*put SOS*/
	if(PutSOS()!= JPEG_SUCCESS)
	{
		return JPEG_FAILED;
	}

	return JPEG_SUCCESS;
}
PUBLIC JPEG_RET_E JPEG_HWWriteTailForThumbnail(void){
	if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "bsm: polling bsm status"))
	{
		return JPEG_FAILED;
	}

	/*put SOI*/
	PutMarker(M_EOI);
	return JPEG_SUCCESS;
}

LOCAL void WriteThumbnailData(APP1_T *app1_t)
{
	uint32 *buf32 = NULL;
	uint8 *buf8 = NULL;
	uint32 i, j;

	buf32 = (uint32 *)app1_t->thumbnail_virt_addr;
	for(i = 0; i < (app1_t->thumbnail_len / 4); i++){
		JPEGFW_PutBits32_II(*buf32++, 32);
	}
	buf8 = (uint8*)buf32;
	for(j = 0; j < (app1_t->thumbnail_len - i * 4); j++){
		JPEGFW_PutC(*buf8++);
	}
	buf32 = NULL;
	buf8 = NULL;
}

#if 1
#define MAX_APP1_SIZE	(64*1024)
PUBLIC JPEG_RET_E PutAPP1(APP1_T *app1_t)
{
	uint8 *app1_buf_ptr;
	uint32 app1_buf_size;
	uint32 app1_size = 0;
	uint32 i=0;
	JPEG_RET_E ret;

	if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE + BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "BSM,Polling the bsm rfifo can read/write"))
	{
		return JPEG_FAILED;
	}

	app1_buf_ptr = ((uint8 *)app1_t->thumbnail_virt_addr+ app1_t->thumbnail_len);
	app1_buf_size = MAX_APP1_SIZE + app1_t->thumbnail_len;

	/* thumbnail use stream_virt_buf[1], after thumbnail, we use this memory as temp buffer */
	if(app1_buf_size + app1_t->thumbnail_len > app1_t->stream_buf_len)
	{
		SCI_TRACE_LOW("[PutAPP1]  failed, not enough tmp buffer, need %x, only have %x\n",
			app1_buf_size, app1_t->stream_buf_len);
		return JPEG_MEMORY_NOT_ENOUGH;
	}

	SCI_TRACE_LOW("[PutAPP1] , app1_buf_ptr = %lx,  app1_buf_size=%x\n", (unsigned long)app1_buf_ptr, app1_buf_size);
#if 0
	//write APP1 to temp buffer
	ret = Jpeg_WriteAPP1(app1_buf_ptr,
	                     app1_buf_size,
	                     app1_t->dc_exif_info_ptr,
	                     app1_t->thumbnail_virt_addr,
	                     app1_t->thumbnail_len,
	                     &app1_size);

	if (JPEG_SUCCESS != ret)
	{
		SCI_TRACE_LOW("[PutAPP1]  failed");
	   	return ret;
	}

	for (i=0; i<app1_size; i++)
	{
		JPEGFW_PutC(*app1_buf_ptr++);
	}
#endif
    SCI_TRACE_LOW("[PutAPP1]  Done, app1_size = %x \n", app1_size);

	return JPEG_SUCCESS;
}
#else
PUBLIC JPEG_RET_E PutAPP1(APP1_T *app1_t)
{
	uint32 description_len = strlen(app1_t->image_description) + 1;
	uint32 make_len = strlen(app1_t->make) + 1;
	uint32 model_len = strlen(app1_t->model) + 1;
	uint32 datetime_len = 1;//20
	if(NULL == app1_t->datetime){
		datetime_len = 1;
	}
	else{
		datetime_len = strlen(app1_t->datetime) + 1;
		SCI_TRACE_LOW("datetime: %s.", app1_t->datetime);
	}
	uint32 copyright_len = strlen(app1_t->copyright) + 1;
	uint32 app1_len = 2 + 4 + 2;
	uint32 header_len = 2 + 2 + 4;
	uint32 IFD_0th_num = 12;
	uint32 IFD_Exif_len = 2 + 7 * 12 + 4;
	uint32 IFD_Exif_val_len = 2 * 4;
	uint32 IFD_GPS_len = 0;
	uint32 gps_process_method_len = 0;
	uint32 gps_date_len = 0;
	uint32 IFD_GPS_val_len = 4;
	if((0 == app1_t->Latitude_dd.numerator) && (0 == app1_t->Latitude_mm.numerator) && (0 == app1_t->Latitude_ss.numerator)
		&& (0 == app1_t->Longitude_dd.numerator) && (0 == app1_t->Longitude_mm.numerator) && (0 == app1_t->Longitude_ss.numerator)){
		IFD_GPS_len = 0;
		gps_process_method_len = 0;
		gps_date_len = 0;
		IFD_GPS_val_len = 0;
		IFD_0th_num = 11;//delete the GPS item
		//IFD_0th_len = 2 + 12 * IFD_0th_num + 2;
	}
	else{
		IFD_GPS_len = 2 + 7 * 12 + 4;
		gps_process_method_len = 1;
		gps_date_len = 1;
		if(NULL == app1_t->gps_process_method){
			gps_process_method_len = 1;
		}
		else{
			gps_process_method_len = strlen(app1_t->gps_process_method) + 1 + 8;
			SCI_TRACE_LOW("gps process method: %s.", app1_t->gps_process_method);
		}
		if(NULL == app1_t->gps_date){
			gps_date_len = 1;
		}
		else{
			gps_date_len = strlen(app1_t->gps_date) + 1;
			SCI_TRACE_LOW("gps date: %s.", app1_t->gps_date);
		}
		//IFD_GPS_val_len = 3 * 24 + 4 + gps_process_method_len + gps_date_len;
		IFD_GPS_val_len = 3 * 24 + gps_process_method_len + gps_date_len;
	}
	uint32 IFD_0th_len = 2 + 12 * IFD_0th_num + 4; //2;
	uint32 IFD_0th_val_len = 2 * 8 + description_len + make_len + model_len + datetime_len + copyright_len;
	uint32 IFD_1st_len = 2 + 7 * 12 + 4;
	uint32 IFD_1st_val_len = 2 * 8;
	uint32 thumbnail_len = app1_t->thumbnail_len;
	uint32 i;
	char *tmp_buf = NULL;
	SCI_TRACE_LOW("PutAPP1 E. des: %d, make: %d, model: %d, copy: %d, datetime: %d, process_method: %d, gps_date: %d.", description_len, make_len, model_len, copyright_len, datetime_len, gps_process_method_len, gps_date_len);

	if(VSP_READ_REG_POLL(VSP_BSM_REG_BASE + BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "BSM,Polling the bsm rfifo can read/write"))
	{
		return JPEG_FAILED;
	}

	SCI_TRACE_LOW("app1_len : %d, header_len : %d, IFD_0th_len : %d, IFD_0th_val_len : %d, IFD_Exif_len : %d, IFD_Exif_val_len : %d, IFD_GPS_len : %d, IFD_GPS_val_len : %d, IFD_1st_len : %d, IFD_1st_val_len: %d,thumbnail_len: %d.", app1_len , header_len , IFD_0th_len , IFD_0th_val_len ,
		IFD_Exif_len , IFD_Exif_val_len , IFD_GPS_len , IFD_GPS_val_len , IFD_1st_len , IFD_1st_val_len, thumbnail_len);

	PutMarker(M_APP1);				/*marker*/
	JPEGFW_PutW(app1_len + header_len + IFD_0th_len + IFD_0th_val_len +
		IFD_Exif_len + IFD_Exif_val_len + IFD_GPS_len + IFD_GPS_val_len + IFD_1st_len + IFD_1st_val_len + thumbnail_len);	/*length*/
	JPEGFW_PutC('E');					/*identifier*/
	JPEGFW_PutC('x');
	JPEGFW_PutC('i');
	JPEGFW_PutC('f');
	JPEGFW_PutC(0);
	JPEGFW_PutC(0);
	//header
	JPEGFW_PutC('I');
	JPEGFW_PutC('I');
	JPEGFW_PutW_II(0x002a);
	JPEGFW_PutBits32_II(0x08, 32); //offset
	//0th IFD
	JPEGFW_PutW_II(IFD_0th_num);  //number of interoperability
	JPEGFW_PutW_II(0x010E); //ImageDescription
	JPEGFW_PutW_II(0x0002);
	JPEGFW_PutBits32_II(description_len, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len, 32);
	SCI_TRACE_LOW("PutAPP1 des_addr %d", header_len + IFD_0th_len);
	JPEGFW_PutW_II(0x010F); //make
	JPEGFW_PutW_II(0x0002);
	SCI_TRACE_LOW("PutAPP1 make");
	JPEGFW_PutBits32_II(make_len, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + description_len, 32);
	JPEGFW_PutW_II(0x0110); //model
	JPEGFW_PutW_II(0x0002);
	SCI_TRACE_LOW("PutAPP1 model");
	JPEGFW_PutBits32_II(model_len, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + description_len + make_len, 32);
	JPEGFW_PutW_II(0x0112); //Orientation
	JPEGFW_PutW_II(0x0003);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(app1_t->orientation, 32);
	JPEGFW_PutW_II(0x011A); //XResolution
	JPEGFW_PutW_II(0x0005);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + description_len + make_len + model_len, 32);
	SCI_TRACE_LOW("PutAPP1 XRes: 0x%x.", header_len + IFD_0th_len + description_len + make_len + model_len);
	JPEGFW_PutW_II(0x011B); //YResolution
	JPEGFW_PutW_II(0x0005);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + description_len + make_len + model_len + 8, 32);
	SCI_TRACE_LOW("PutAPP1 YRes: 0x%x.", header_len + IFD_0th_len + description_len + make_len + model_len + 8);
	JPEGFW_PutW_II(0x0128); //ResolutionUnit
	JPEGFW_PutW_II(0x0005);
	JPEGFW_PutBits32_II(0x3, 32);
	JPEGFW_PutBits32_II(0x2, 32);
	SCI_TRACE_LOW("PutAPP1 ResU");
	JPEGFW_PutW_II(0x0132); //DateTime
	JPEGFW_PutW_II(0x0002);
	JPEGFW_PutBits32_II(0x14, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + description_len + make_len + model_len + 8 * 2, 32);
	JPEGFW_PutW_II(0x0213); //YCbCrPositioning
	JPEGFW_PutW_II(0x0003);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(0x2, 32);
	SCI_TRACE_LOW("PutAPP1 YCbCr");
	JPEGFW_PutW_II(0x8298); //copyright
	JPEGFW_PutW_II(0x0002);
	JPEGFW_PutBits32_II(copyright_len, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + description_len + make_len + model_len + 8 * 2 + datetime_len, 32);
	SCI_TRACE_LOW("PutAPP1 copyright");
	JPEGFW_PutW_II(0x8769); //Exif IFD Pointer
	JPEGFW_PutW_II(0x0004);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + description_len + make_len + model_len + 8 * 2 + datetime_len + copyright_len, 32);
	SCI_TRACE_LOW("PutAPP1 Exif IFD");
	if(0 != IFD_GPS_len){
		JPEGFW_PutW_II(0x8825); //GPS IFD Pointer
		JPEGFW_PutW_II(0x0004);
		JPEGFW_PutBits32_II(0x1, 32);
		JPEGFW_PutBits32_II(header_len + IFD_0th_len + IFD_0th_val_len + IFD_Exif_len + IFD_Exif_val_len, 32);
	}

	SCI_TRACE_LOW("Next IFD Offset 0th: %d.", header_len + IFD_0th_len + IFD_0th_val_len + IFD_Exif_len + IFD_Exif_val_len
		+ IFD_GPS_len + IFD_GPS_val_len);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + IFD_0th_val_len + IFD_Exif_len + IFD_Exif_val_len
		+ IFD_GPS_len + IFD_GPS_val_len, 32); //Next IFD Offset
	//Value longer than 4byte of 0th IFD
	tmp_buf = (char *)app1_t->image_description; //ImageDescription
	for(i = 0; i < description_len - 1; i++)
	{
		JPEGFW_PutC(*tmp_buf++);
	}
	JPEGFW_PutC(0x0);
	tmp_buf = (char *)app1_t->make; //Make
	for(i = 0; i < make_len - 1; i++)
	{
		JPEGFW_PutC(*tmp_buf++);
	}
	JPEGFW_PutC(0x0);
	tmp_buf = (char *)app1_t->model; //Model
	for(i = 0; i < model_len - 1; i++)
	{
		JPEGFW_PutC(*tmp_buf++);
	}
	JPEGFW_PutC(0x0);
	JPEGFW_PutBits32_II(0x48, 32); //XResolution Value
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(0x48, 32); //YResolution Value
	JPEGFW_PutBits32_II(0x1, 32);
	if(NULL != app1_t->datetime){
		tmp_buf = (char *)app1_t->datetime; //DateTime
		for(i = 0; i < datetime_len - 1; i++)
		{
			JPEGFW_PutC(*tmp_buf++);
		}
	}
	JPEGFW_PutC(0x0);
	tmp_buf = (char *)app1_t->copyright; //Copyright
	for(i = 0; i < copyright_len - 1; i++)
	{
		JPEGFW_PutC(*tmp_buf++);
	}
	JPEGFW_PutC(0x0);
	//Exif IFD
	JPEGFW_PutW_II(0x7); //Exif IFD Number
	JPEGFW_PutW_II(0x9000); //ExifVersion
	JPEGFW_PutW_II(0x0007);
	JPEGFW_PutBits32_II(0x4, 32);
	JPEGFW_PutC('0');
	JPEGFW_PutC('2');
	JPEGFW_PutC('0');
	JPEGFW_PutC('0');
	JPEGFW_PutW_II(0x9101); //ComponentsConfiguration
	JPEGFW_PutW_II(0x0007);
	JPEGFW_PutBits32_II(0x4, 32);
	JPEGFW_PutBits32_II(0x00030201, 32);
	SCI_TRACE_LOW("PutAPP1 Config.");
	JPEGFW_PutW_II(0x920A); //FocalLength
	JPEGFW_PutW_II(0x0005);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + IFD_0th_val_len + IFD_Exif_len, 32);
	JPEGFW_PutW_II(0xA000); //FlashpixVersion
	JPEGFW_PutW_II(0x0007);
	JPEGFW_PutBits32_II(0x4, 32);
	JPEGFW_PutC('0');
	JPEGFW_PutC('1');
	JPEGFW_PutC('0');
	JPEGFW_PutC('0');
	SCI_TRACE_LOW("PutAPP1 Flash Ver.");
	JPEGFW_PutW_II(0xA001); //ColorSpace
	JPEGFW_PutW_II(0x0003);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutW_II(0xA002); //Pixel X Dimension
	JPEGFW_PutW_II(0x0004);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(app1_t->thumb_width, 32);
	JPEGFW_PutW_II(0xA003); //Pixel Y Dimension
	JPEGFW_PutW_II(0x0004);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(app1_t->thumb_height, 32);
	SCI_TRACE_LOW("iPutAPP1 thumb_height.");
	JPEGFW_PutBits32_II(0x0, 32);
	//Value longer than 4byte of Exif IFD
	JPEGFW_PutBits32_II(app1_t->focal_length.numerator, 32); //Focal Length value
	JPEGFW_PutBits32_II(app1_t->focal_length.denominator, 32);


	//GPS IFiD
	if(0 != IFD_GPS_len){
		JPEGFW_PutW_II(0x7); //GPS IFD Number
		JPEGFW_PutW_II(0x1); //GPS Latitude Ref
		JPEGFW_PutW_II(0x0002);
		JPEGFW_PutBits32_II(0x2, 32);
	if(0 == app1_t->Latitude_ref){
		JPEGFW_PutC('N');
	}
	else{
		JPEGFW_PutC('S');
	}
	JPEGFW_PutC(0x0);
	JPEGFW_PutW_II(0x0);
	JPEGFW_PutW_II(0x2); //Latitude
	JPEGFW_PutW_II(0x0005);
	JPEGFW_PutBits32_II(0x3, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + IFD_0th_val_len + IFD_Exif_len  + IFD_Exif_val_len + IFD_GPS_len, 32);
	SCI_TRACE_LOW("PutAPP1 latitude.");
	JPEGFW_PutW_II(0x3); //GPS Longitude Ref
	JPEGFW_PutW_II(0x0002);
	JPEGFW_PutBits32_II(0x2, 32);
	if(0 == app1_t->Longitude_ref){
		JPEGFW_PutC('E');
	}
	else{
		JPEGFW_PutC('W');
	}
	JPEGFW_PutC(0x0);
	JPEGFW_PutW_II(0x0);
	JPEGFW_PutW_II(0x4); //Longitude
	JPEGFW_PutW_II(0x0005);
	JPEGFW_PutBits32_II(0x3, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + IFD_0th_val_len + IFD_Exif_len + IFD_Exif_val_len + IFD_GPS_len + 6 * 4, 32);
	SCI_TRACE_LOW("PutAPP1 longitude.");
	JPEGFW_PutW_II(0x7); //TimeStamp
	JPEGFW_PutW_II(0x0005);
	JPEGFW_PutBits32_II(0x3, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + IFD_0th_val_len + IFD_Exif_len + IFD_Exif_val_len + IFD_GPS_len + 2 * 6 * 4, 32);
	JPEGFW_PutW_II(0x1B); //ProcessingMethod
	JPEGFW_PutW_II(0x0007);
	JPEGFW_PutBits32_II(gps_process_method_len, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + IFD_0th_val_len + IFD_Exif_len + IFD_Exif_val_len + IFD_GPS_len + 3 * 6 * 4, 32);
	SCI_TRACE_LOW("PutAPP1 method.");
	JPEGFW_PutW_II(0x1D); //DateStamp
	JPEGFW_PutW_II(0x0002);
	JPEGFW_PutBits32_II(gps_date_len, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + IFD_0th_val_len + IFD_Exif_len + IFD_Exif_val_len + IFD_GPS_len + 3 * 6 * 4 + gps_process_method_len, 32);
	SCI_TRACE_LOW("PutAPP1 datestamp.");
	JPEGFW_PutBits32_II(0x0, 32);
	//Value longer than 4byte of GPS IFD
	JPEGFW_PutBits32_II(app1_t->Latitude_dd.numerator, 32); //Latitude value
	JPEGFW_PutBits32_II(app1_t->Latitude_dd.denominator, 32);
	JPEGFW_PutBits32_II(app1_t->Latitude_mm.numerator, 32);
	JPEGFW_PutBits32_II(app1_t->Latitude_mm.denominator, 32);
	JPEGFW_PutBits32_II(app1_t->Latitude_ss.numerator, 32);
	JPEGFW_PutBits32_II(app1_t->Latitude_ss.denominator, 32);
	JPEGFW_PutBits32_II(app1_t->Longitude_dd.numerator, 32); //Longitude value
	SCI_TRACE_LOW("ongitude dd.");
	JPEGFW_PutBits32_II(app1_t->Longitude_dd.denominator, 32);
	JPEGFW_PutBits32_II(app1_t->Longitude_mm.numerator, 32);
	SCI_TRACE_LOW("ongitude mm.");
	JPEGFW_PutBits32_II(app1_t->Longitude_mm.denominator, 32);
	JPEGFW_PutBits32_II(app1_t->Longitude_ss.numerator, 32);
	SCI_TRACE_LOW("ongitude ss.");
	JPEGFW_PutBits32_II(app1_t->Longitude_ss.denominator, 32);
	JPEGFW_PutBits32_II(app1_t->gps_hour, 32); //TimeStamp value
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(app1_t->gps_minuter, 32);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(app1_t->gps_second, 32);
	JPEGFW_PutBits32_II(0x1, 32);
	SCI_TRACE_LOW("process method.");
	if(NULL != app1_t->gps_process_method){
		//0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0
		JPEGFW_PutC(0x41);JPEGFW_PutC(0x53);JPEGFW_PutC(0x43);JPEGFW_PutC(0x49);JPEGFW_PutC(0x49);JPEGFW_PutC(0x0);JPEGFW_PutC(0x0);JPEGFW_PutC(0x0);
		tmp_buf = (char *)app1_t->gps_process_method; //ProcessMethod value
		for(i = 0; i < gps_process_method_len - 1 -8; i++)
		{
			JPEGFW_PutC(*tmp_buf++);
		}
	}

	JPEGFW_PutC(0x0);
	SCI_TRACE_LOW("gps date.");
	if(NULL != app1_t->gps_date){
		tmp_buf = (char *)app1_t->gps_date; //DateStamp value
		for(i = 0; i < gps_date_len - 1; i++)
		{
			JPEGFW_PutC(*tmp_buf++);
		}
	}
		JPEGFW_PutC(0x0);
		SCI_TRACE_LOW("PutAPP1 GPS.");
	}
	SCI_TRACE_LOW("Next IFD Offset Exf: %d.", header_len + IFD_0th_len + IFD_0th_val_len + IFD_Exif_len + IFD_Exif_val_len
		+ IFD_GPS_len + IFD_GPS_val_len);

	// 1st IFD
	JPEGFW_PutW_II(0x7); //Number Of interoperability
	JPEGFW_PutW(0x0103); //Compression
	JPEGFW_PutW(0x0003);
	JPEGFW_PutBits(0x1, 32);
	JPEGFW_PutBits(0x6, 32);
	JPEGFW_PutW_II(0x011A); //XResolution
	JPEGFW_PutW_II(0x0005);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + IFD_0th_val_len +
		IFD_Exif_len + IFD_Exif_val_len + IFD_GPS_len + IFD_GPS_val_len + IFD_1st_len, 32);
	SCI_TRACE_LOW("PutAPP1 XRes.");
	JPEGFW_PutW_II(0x011B); //YResolution
	JPEGFW_PutW_II(0x0005);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + IFD_0th_val_len +
		IFD_Exif_len + IFD_Exif_val_len + IFD_GPS_len + IFD_GPS_val_len + IFD_1st_len + 2 * 4, 32);
	SCI_TRACE_LOW("PutAPP1 YRes.");
	JPEGFW_PutW_II(0x0128); //ResolutionUnit
	JPEGFW_PutW_II(0x0003);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(0x2, 32);
	JPEGFW_PutW_II(0x0201); //JPEGInterchangeFormat
	JPEGFW_PutW_II(0x0004);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(header_len + IFD_0th_len + IFD_0th_val_len +
		IFD_Exif_len + IFD_Exif_val_len + IFD_GPS_len + IFD_GPS_val_len + IFD_1st_len + IFD_1st_val_len, 32);
	JPEGFW_PutW_II(0x0202); //JPEGInterchangeFormatLength
	JPEGFW_PutW_II(0x0004);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(app1_t->thumbnail_len, 32);
	JPEGFW_PutW_II(0x0213); //YCbCrPositiong
	JPEGFW_PutW_II(0x0003);
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(0x2, 32);

	JPEGFW_PutBits32_II(0x0, 32); //Next IFD Offset

	//Value longer than 4byte of 1st IFD
	JPEGFW_PutBits32_II(0x48, 32); //XResolution Value
	JPEGFW_PutBits32_II(0x1, 32);
	JPEGFW_PutBits32_II(0x48, 32); //YResolution Value
	JPEGFW_PutBits32_II(0x1, 32);
	SCI_TRACE_LOW("PutAPP1 XRes val.");

	//Thumbnail Image Data
	if(0 != app1_t->thumbnail_len)
	{
		WriteThumbnailData(app1_t);
	}
	SCI_TRACE_LOW("PutAPP1 X.");
	return JPEG_SUCCESS;
}
#endif

PUBLIC JPEG_RET_E PutSOF0(void)
{
	uint8 i = 0;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();

	SCI_ASSERT(jpeg_fw_codec != PNULL);

	if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE + BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "BSM,Polling the bsm rfifo can read/write"))
	{
		return JPEG_FAILED;
	}

	/*
	* Length of SOF head(2 byte)
	* Sample precision(1 byte)
	* Image Height(2 byte)
	* Image Width (2 byte)
	* Component number(1 byte)
	* Component id(1 byte)
	* Sample ratio (1 byte)
	* Q table id( 1 byte)
	*/

	PutMarker(M_SOF0);				/*put SOF0 marker*/
	JPEGFW_PutW(3*3+2+5+1);			/*length*/
	JPEGFW_PutC(8);					/*sample precision*/
	JPEGFW_PutW(jpeg_fw_codec->height);	/*height*/
	JPEGFW_PutW(jpeg_fw_codec->width);	/*width*/
	JPEGFW_PutC(3);					/*component number*/

	for (i=0; i<3; i++)
	{
		JPEGFW_PutC((uint8)(i+1));				/*put component id, from 1 to 3*/
		JPEGFW_PutC((uint8)((jpeg_fw_codec->ratio[i].h_ratio<<4)|(jpeg_fw_codec->ratio[i].v_ratio)));
		JPEGFW_PutC((uint8)(jpeg_fw_codec->tbl_map[i].quant_tbl_id));
	}

	return JPEG_SUCCESS;
}

PUBLIC JPEG_RET_E PutSOS(void)
{
	/*
	* Length of scan head(2 byte)
	* Component number in scane(1 byte)
	* Csj scan component selector (1 byte)
	* Tdj DC entroy coding table ,Taj AC entropy coding table, (1 byte)
	* Start of spectral predictor selection (1 byte)
	* End of spectral predictor selection  (1 byte)
	* 0         (1 byte)
	*/

	uint8 i = 0;

	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();
	SCI_ASSERT(jpeg_fw_codec != PNULL);

	if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE + BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "BSM,Polling the bsm rfifo can read/write"))
	{
		return JPEG_FAILED;
	}

	PutMarker(M_SOS);			/*put marker*/
	JPEGFW_PutW(2*3+2+1+3);		/*scan head length*/
	JPEGFW_PutC(3);				/*component num*/

	for(i=0; i<3; i++)
	{
		JPEGFW_PutC((uint8)(i+1));			/*put component selector for YUV*/
		JPEGFW_PutC((uint8)((jpeg_fw_codec->tbl_map[i].dc_huff_tbl_id<<4)|jpeg_fw_codec->tbl_map[i].ac_huff_tbl_id));
	}

	JPEGFW_PutC(0);				/*Spectral selection start */
	JPEGFW_PutC(63);				/*Spectral selection end */
	JPEGFW_PutC(0);				/*Successive approximation */

	return JPEG_SUCCESS;
}

PUBLIC JPEG_RET_E PutQuantTbl(void)
{
	uint8 i = 0, k = 0;
	const uint8 *quant = NULL;
	const uint8 *zigzag = jpeg_fw_zigzag_order;
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();

	SCI_ASSERT(jpeg_fw_codec != PNULL);

	/*we have two table, one for luminance, the other for chroma*/
	for(i=0; i < 2; i++)
	{
		/*
		* Length of table(2 byte)
		* Table header (1 byte)
		* Table content (64 byte)
		*/
		if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "BSM,Polling the bsm rfifo can read/write"))
		{
			return JPEG_FAILED;
		}

		PutMarker(M_DQT);			/*put marker*/
		JPEGFW_PutW(64 + 1 + 2);		/*length*/
		JPEGFW_PutC(i);				/*table id, precision = 8*/
		quant = jpeg_fw_codec->quant_tbl[i];
		JPEG_ASSERT(quant!=0);

		if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "BSM,Polling the bsm rfifo can read/write"))
		{
			return JPEG_FAILED;
		}

		for (k=0; k<32; k++)
		{
			JPEGFW_PutC(quant[zigzag[k]]);
		}

		if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "BSM,Polling the bsm rfifo can read/write"))
		{
			return JPEG_FAILED;
		}

		for (k=32; k<64; k++)
		{
			JPEGFW_PutC(quant[zigzag[k]]);
		}
	}

	return JPEG_SUCCESS;
}

LOCAL JPEG_RET_E PutOneHuffTbl(uint8 index, const uint8 *bits, const uint8 *huffval)
{
	uint16 count = 0, i = 0, j = 0, length = 0;
	uint16 int_count = 0;
	uint16 left_count = 0;

	JPEG_ASSERT(bits != 0);
	JPEG_ASSERT(huffval != 0);

	/*get the symbol count*/
	for(i=1, count=0; i<=16; i++)
	{
		count += bits[i];
	}
	JPEG_ASSERT(count <= 256);

	/*get the data length*/
	length = 2 + 1 + 16 + count;

	/*
	*  length of table(2 byte)
	*  table id(1 byte)
	*  bits table(16 byte)
	*  huff value table
	*/
	if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "BSM,Polling the bsm rfifo can read/write"))
	{
		return JPEG_FAILED;
	}

	PutMarker(M_DHT);			/*put marker*/
	JPEGFW_PutW(length);			/*length*/
	JPEGFW_PutC(index);			/*table id*/

	for(i = 1; i <= 16; i++)		/*put bits*/
	{
		JPEGFW_PutC(bits[i]);
	}

	int_count = count/32;
	left_count = count%32;

	for(i = 0; (int_count--) > 0; )
	{
		if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "BSM,Polling the bsm rfifo can read/write"))
		{
			return JPEG_FAILED;
		}

		for(j = 0; j < 32; j++,i++)
		{
			JPEGFW_PutC(huffval[i]);
		}
	}

	if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "BSM,Polling the bsm rfifo can read/write"))
	{
		return JPEG_FAILED;
	}

	for( ;(left_count--) > 0; i++)
	{
		JPEGFW_PutC(huffval[i]);
	}

	return JPEG_SUCCESS;
}

PUBLIC JPEG_RET_E PutHuffTbl(void)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();
	SCI_ASSERT(jpeg_fw_codec != PNULL);

	/*put lum dc tbl*/
	if(PutOneHuffTbl(
		0x00,
		jpeg_fw_codec->dc_huff_tbl[JPEG_FW_LUM_ID].bits,
		jpeg_fw_codec->dc_huff_tbl[JPEG_FW_LUM_ID].huffval
		) != JPEG_SUCCESS)
		{
			return JPEG_FAILED;
		}

	/*put lum ac tbl*/
	if(PutOneHuffTbl(
		0x10,
		jpeg_fw_codec->ac_huff_tbl[JPEG_FW_LUM_ID].bits,
		jpeg_fw_codec->ac_huff_tbl[JPEG_FW_LUM_ID].huffval
		) != JPEG_SUCCESS)
		{
			return JPEG_FAILED;
		}

	/*put chr dc tbl*/
	if(PutOneHuffTbl(
		0x01,
		jpeg_fw_codec->dc_huff_tbl[JPEG_FW_CHR_ID].bits,
		jpeg_fw_codec->dc_huff_tbl[JPEG_FW_CHR_ID].huffval
		) != JPEG_SUCCESS)
		{
			return JPEG_FAILED;
		}

	/*put chr ac tbl*/
	if(PutOneHuffTbl(
		0x11,
		jpeg_fw_codec->ac_huff_tbl[JPEG_FW_CHR_ID].bits,
		jpeg_fw_codec->ac_huff_tbl[JPEG_FW_CHR_ID].huffval
		) != JPEG_SUCCESS)
		{
			return JPEG_FAILED;
		}

	return JPEG_SUCCESS;
}

PUBLIC JPEG_RET_E WriteDRI(void)
{
	JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();

	SCI_ASSERT(jpeg_fw_codec != PNULL);

	if(JPG_READ_REG_POLL(JPG_BSM_REG_BASE+BSM_RDY_OFFSET, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "BSM,Polling the bsm rfifo can read/write"))
	{
		return JPEG_FAILED;
	}

	if(jpeg_fw_codec->restart_interval)
	{
		PutMarker(M_DRI);		/*put marker*/
		JPEGFW_PutW(4);			/*length*/
		JPEGFW_PutW((uint16) jpeg_fw_codec->restart_interval);			/*interval*/
	}

	return JPEG_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
#endif //JPEG_ENC
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
