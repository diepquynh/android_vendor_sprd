//------------------------------------------------------------------------------
// File: vpurun.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------
#ifndef _VPU_RUN_H_
#define _VPU_RUN_H_

#include "vpuapi.h"

#ifdef SUPPORT_FFMPEG_DEMUX
#include <libavformat/avformat.h>
#endif

#define	MAX_FILE_PATH	256

//----- Theora Specific Buffer -----------------//
#define SIZE_THO_STREAM                 0x200000   // 1MB*2

#define MAX_PIC_SKIP_NUM 5

typedef struct
{
	char yuvFileName[MAX_FILE_PATH];
	char cmdFileName[MAX_FILE_PATH];
	char bitstreamFileName[MAX_FILE_PATH];
	char huffFileName[MAX_FILE_PATH];
	char cInfoFileName[MAX_FILE_PATH];
	char qMatFileName[MAX_FILE_PATH];
	char qpFileName[MAX_FILE_PATH];
	char cfgFileName[MAX_FILE_PATH];
	int stdMode;
	int picWidth;
	int picHeight;
	int kbps;
	int rotAngle;
	int mirDir;
	int useRot;
	int qpReport;
	int ringBufferEnable;
	int rcIntraQp;
	int outNum;
    int skip_pic_nums[MAX_PIC_SKIP_NUM];
	int instNum;
	int coreIdx;
	int mapType;

      int lineBufIntEn;
#ifdef SUPPORT_FFMPEG_DEMUX
    int en_container;                   //enable container
    int container_frame_rate;           //framerate for container
#else
#endif
	int picQpY;

	int prpYuvFormat;
} EncConfigParam;


typedef struct
{
	char yuvFileName[MAX_FILE_PATH];
	char bitstreamFileName[MAX_FILE_PATH];
	int	bitFormat;
    int avcExtension; // 0:AVC, 1:MVC	// if CDB is enabled 0:MP 1:MVC 2:BP
	int rotAngle;
	int mirDir;
	int useRot;
	int	useDering;
	int outNum;
	int checkeos;
	int mp4DeblkEnable;
	int iframeSearchEnable;
	int skipframeMode;
	int reorder;
	int	mpeg4Class;
	int mapType;
	int seqInitMode;
	int instNum;
	int bitstreamMode;
    int maxWidth;
    int maxHeight;
	int coreIdx;
	int frameDelay;
	int cmd;
	int userFbAlloc;
	int runFilePlayTest;


	int enScaler;
	int scaleWidth;  /** mini-pippen target scale resolution*/
	int scaleHeight;
	int scalerImageFormat;

    int reorderDisable;

} DecConfigParam;


enum {
	STD_AVC_DEC = 0,
	STD_VC1_DEC,
	STD_MP2_DEC,
	STD_MP4_DEC,
	STD_H263_DEC,
	STD_DV3_DEC,
	STD_RVx_DEC,
	STD_AVS_DEC,
	STD_THO_DEC,
	STD_VP3_DEC,
	STD_VP8_DEC,
	STD_MP4_ENC = 12,
	STD_H263_ENC,
	STD_AVC_ENC
};

typedef struct {
	int codecMode;
	int numMulti;
	int saveYuv;
	int  multiMode[MAX_NUM_INSTANCE];
    char multiFileName[MAX_NUM_INSTANCE][MAX_FILE_PATH];
	char multiYuvFileName[MAX_NUM_INSTANCE][MAX_FILE_PATH];
	DecConfigParam decConfig[MAX_NUM_INSTANCE];
	EncConfigParam encConfig[MAX_NUM_INSTANCE];
} MultiConfigParam;


#if defined (__cplusplus)
extern "C" {
#endif

	int DecodeTest(DecConfigParam *param);
#ifdef SUPPORT_FFMPEG_DEMUX
	int FilePlayTest(DecConfigParam *param);
#endif
	int EncodeTest(EncConfigParam *param);
#ifdef SUPPORT_FFMPEG_DEMUX
	int FileTranscodingTest(DecConfigParam *decCfgParam, EncConfigParam *encCfgParam);
#endif
	int MultiInstanceTest(MultiConfigParam	*param);
#if defined (__cplusplus)
}
#endif
#endif	/* _VPU_RUN_H_ */
