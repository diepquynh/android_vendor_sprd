//------------------------------------------------------------------------------
// File: vpuio.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef _VPU_IO_H_
#define _VPU_IO_H_

#include "vputypes.h"

#define DRAM_BUS_WIDTH 8

typedef struct {
	unsigned char * buf;
	void *fp;
	int size;
	int point;
	int random_access_point;
	int count;
	int fillendbs;
} BufInfo;

typedef struct {
    unsigned char*    y_addr;
    unsigned char*    cb_addr;
    unsigned char*    cr_addr;
} FRAME_ADDR;

#if defined (__cplusplus)
extern "C"{
#endif 

	/* Decode Function */
	int FillSdramBurst(Uint32 core_idx, BufInfo * pBufInfo, Uint32 targetAddr, vpu_buffer_t *pVbStream, Uint32 size, int checkeos, int *streameos, int endian );
	void StoreYuvImageBurstLinear(Uint32 core_idx, FrameBuffer *fbSrc, Uint8 *pDst, Rect cropRect, int interLeave, int format, int endian);
	/* Encode Function */
	int ProcessEncodedBitstreamBurst(Uint32 core_idx,  
		osal_file_t fp, 
		int targetAddr, 
		PhysicalAddress bsBufStartAddr, 
		PhysicalAddress bsBufEndAddr,
		int size,
		int endian);
	void LoadYuvImageBurstFormat(Uint32 core_idx, Uint8 * src, int picWidth, int picHeight, FrameBuffer *fb, int stride, int interLeave, int format, int endian );

    void LoadEncodedBitstreamBurst(Uint32 core_idx, unsigned char * buffer, int targetAddr,
        PhysicalAddress bsBufStartAddr, PhysicalAddress bsBufEndAddr,
        int size, int endian );

	void StoreYuvImageBurstFormat_ex(Uint32 core_idx, FrameBuffer *fbSrc, TiledMapConfig mapCfg, Uint8 *pDst, Rect cropRect, int interLeave, int format, int endian);

#if defined (__cplusplus)
}
#endif 

#endif //#ifndef _VPU_IO_H_
