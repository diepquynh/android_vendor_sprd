//------------------------------------------------------------------------------
// File: vpuio.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#include "vpuapi.h"
#include "vpuapifunc.h"
#include "vpuio.h"

#define USE_CROP_INFO


//#define FILL_GRAY

//////////////////// DRAM Read/Write helper Function ////////////////////////////
void LoadYuvImageBurstFormat(Uint32 core_idx, Uint8 * src, int picWidth, int picHeight,
    FrameBuffer *fb, int stride, int interLeave, int format, int endian )

{
    int i, y, nY, nCb, nCr;
    int addr;
    int lumaSize, chromaSize, chromaStride, chromaWidth, delta_luma_size, delta_chroma_size;
    Uint8 * puc;
    Uint8 * temp_data;
    BYTE *  pTemp;
    BYTE * srcAddrCb;
    BYTE * srcAddrCr;

    switch (format)
    {
    case FORMAT_420:
        nY = picHeight;
        nCb = nCr = picHeight / 2;
        chromaSize = picWidth * picHeight / 4;
        delta_chroma_size = (((picWidth+15)&~15) * ((picHeight+15)&~15))/4 - chromaSize;
        chromaStride = stride / 2;
        chromaWidth = picWidth / 2;
        break;
    case FORMAT_224:
        nY = picHeight;
        nCb = nCr = picHeight / 2;
        chromaSize = picWidth * picHeight / 2;
        delta_chroma_size = (((picWidth+15)&~15) * ((picHeight+15)&~15))/2 - chromaSize;
        chromaStride = stride;
        chromaWidth = picWidth;
        break;
    case FORMAT_422:
        nY = picHeight;
        nCb = nCr = picHeight;
        chromaSize = picWidth * picHeight / 2;
        delta_chroma_size = (((picWidth+15)&~15) * ((picHeight+15)&~15))/2 - chromaSize;
        chromaStride = stride / 2;
        chromaWidth = picWidth / 2;
        break;
    case FORMAT_444:
        nY = picHeight;
        nCb = nCr = picHeight;
        chromaSize = picWidth * picHeight;
        delta_chroma_size = (((picWidth+15)&~15) * ((picHeight+15)&~15)) - chromaSize;
        chromaStride = stride;
        chromaWidth = picWidth;
        break;
    case FORMAT_400:
        nY = picHeight;
        nCb = nCr = 0;
        chromaSize = picWidth * picHeight / 4;
        delta_chroma_size = (((picWidth+15)&~15) * ((picHeight+15)&~15))/4 - chromaSize;
        chromaStride = stride / 2;
        chromaWidth = picWidth / 2;
		break;
	default:
		nY = picHeight;
		nCb = nCr = picHeight;
		chromaSize = picWidth * picHeight / 2;
		delta_chroma_size = (((picWidth+15)&~15) * ((picHeight+15)&~15))/2 - chromaSize;
		chromaStride = stride / 2;
		chromaWidth = picWidth / 2;
		break;
    }


    puc = src;
    addr = fb->bufY;
    lumaSize = picWidth * picHeight;

    delta_luma_size = ((picWidth+15)&~15) * ((picHeight+15)&~15) - lumaSize;

    if( picWidth == stride) // for fast write
    {
        vdi_write_memory(core_idx, addr, (Uint8 *)( puc ), lumaSize, endian);

        if(delta_luma_size!=0)
        {
            temp_data = osal_malloc(delta_luma_size);
            for(i=0; i<delta_luma_size ; i++)
                temp_data[i] =0;
            vdi_write_memory(core_idx, addr+lumaSize,  temp_data , delta_luma_size, endian);
            osal_free(temp_data);
        }

        if( format == FORMAT_400)
            return;

        if( interLeave == 1 )
        {
            addr = fb->bufCb;
            srcAddrCb = puc + lumaSize;
            srcAddrCr = puc + lumaSize + chromaSize;

            pTemp = osal_malloc(chromaStride*2);
            if (!pTemp)
                return;

            for (y=0; y<nCb; y++) {
                for (i=0; i<chromaStride*2; i+=8) {
                    pTemp[i  ] = *srcAddrCb++;
                    pTemp[i+2] = *srcAddrCb++;
                    pTemp[i+4] = *srcAddrCb++;
                    pTemp[i+6] = *srcAddrCb++;
                    pTemp[i+1] = *srcAddrCr++;
                    pTemp[i+3] = *srcAddrCr++;
                    pTemp[i+5] = *srcAddrCr++;
                    pTemp[i+7] = *srcAddrCr++;
                }
                vdi_write_memory(core_idx, addr+2*chromaStride*y, (Uint8 *)pTemp, chromaStride*2, endian);
            }

            if(delta_chroma_size !=0)
            {
                temp_data = osal_malloc(delta_chroma_size*2);
                for(i=0; i<delta_chroma_size*2 ; i++)
                    temp_data[i] =128;
                vdi_write_memory(core_idx, addr+2*chromaStride*nCb,  temp_data , delta_chroma_size*2, endian);
                osal_free(temp_data);
            }

            osal_free(pTemp);
        }
        else
        {
            puc = src + lumaSize;
            addr = fb->bufCb;
            vdi_write_memory(core_idx, addr, (Uint8 *)puc, chromaSize, endian);

            temp_data = osal_malloc(delta_chroma_size);
            for(i=0; i<delta_chroma_size ; i++)
                temp_data[i] =128;
            vdi_write_memory(core_idx, addr+chromaSize ,  temp_data , delta_chroma_size, endian);
            osal_free(temp_data);

            puc = src + lumaSize + chromaSize;
            addr = fb->bufCr;
            vdi_write_memory(core_idx, addr, (Uint8 *)puc, chromaSize, endian);

            temp_data = osal_malloc(delta_chroma_size);
            for(i=0; i<delta_chroma_size ; i++)
                temp_data[i] =128;
            vdi_write_memory(core_idx, addr+chromaSize ,  temp_data , delta_chroma_size, endian);
            osal_free(temp_data);
        }
    }
    else
    {
        for (y = 0; y < nY; ++y) {
            vdi_write_memory(core_idx, addr + stride * y, (Uint8 *)(puc + y * picWidth), picWidth, endian);
        }


        if( format == FORMAT_400)
            return;

        if( interLeave == 1 )
        {
            addr = fb->bufCb;
            puc = src + lumaSize;
            srcAddrCb = puc + lumaSize;
            srcAddrCr = puc + lumaSize + chromaSize;



            pTemp = (BYTE*)osal_malloc(chromaWidth*2);
            if (!pTemp)
                return;

            for (y=0; y<nCb; y++) {
                for (i=0; i<chromaWidth*2; i+=8) {
                    pTemp[i  ] = *srcAddrCb++;
                    pTemp[i+2] = *srcAddrCb++;
                    pTemp[i+4] = *srcAddrCb++;
                    pTemp[i+6] = *srcAddrCb++;
                    pTemp[i+1] = *srcAddrCr++;
                    pTemp[i+3] = *srcAddrCr++;
                    pTemp[i+5] = *srcAddrCr++;
                    pTemp[i+7] = *srcAddrCr++;
                }
                vdi_write_memory(core_idx, addr+2*chromaStride*y, (Uint8 *)pTemp, chromaWidth*2, endian);
            }
            osal_free(pTemp);
        }
        else
        {
            puc = src + lumaSize;
            addr = fb->bufCb;
            for (y = 0; y < nCb; ++y) {
                vdi_write_memory(core_idx, addr + chromaStride * y, (Uint8 *)(puc + y * chromaWidth), chromaWidth, endian);
            }

            puc = src + lumaSize + chromaSize;
            addr = fb->bufCr;
            for (y = 0; y < nCr; ++y) {
                vdi_write_memory(core_idx, addr + chromaStride * y, (Uint8 *)(puc + y * chromaWidth), chromaWidth, endian);
            }
        }
    }
}







void StoreYuvImageBurstLinear(Uint32 core_idx, FrameBuffer *fbSrc, Uint8 *pDst, Rect cropRect, int interLeave, int format, int endian)
{
	int size;
	int y, nY, nCb, nCr;
	int addr;
	int lumaSize, chromaSize, chromaStride, chromaWidth, chromaHeight;
	Uint8 * puc;
	int stride = fbSrc->stride;
	int height = fbSrc->height;
	int width = fbSrc->stride;


	switch (format)
	{
	case FORMAT_420:
		nY = (height+1)/2*2;
		nCb = nCr = (height+1) / 2;
		chromaSize = ((width + 1) / 2) * ((height+1) / 2);
		chromaStride = stride / 2;
		chromaWidth = (width + 1) / 2;
		chromaHeight = nY;
		break;
	case FORMAT_224:
		nY = (height+1)/2*2;
		nCb = nCr = (height+1) / 2;
		chromaSize = (width) * ((height+1) / 2);
		chromaStride = stride;
		chromaWidth = width;
		chromaHeight = nY;
		break;
	case FORMAT_422:
		nY = height;
		nCb = nCr = height;
		chromaSize = ((width + 1)/2) * height ;
		chromaStride = stride / 2;
		chromaWidth = (width + 1) / 2;
		chromaHeight = nY*2;
		break;
	case FORMAT_444:
		nY = height;
		nCb = nCr = height;
		chromaSize = width * height;
		chromaStride = stride;
		chromaWidth = width;
		chromaHeight = nY*2;
		break;
	case FORMAT_400:
		nY = height;
		nCb = nCr = 0;
		chromaSize = 0;
		chromaStride = 0;
		chromaWidth = 0;
		chromaHeight = 0;
		break;
	default:
		nY = height;
		nCb = nCr = height;
		chromaSize = ((width + 1)/2) * height ;
		chromaStride = stride / 2;
		chromaWidth = (width + 1) / 2;
		chromaHeight = nY*2;
		break;
	}


	puc = pDst;
	addr = fbSrc->bufY;


	lumaSize = width * nY;

	size = lumaSize + chromaSize*2;

	if (width == stride)
	{
		vdi_read_memory(core_idx, addr, (Uint8 *)( puc ), lumaSize,  endian);

		if (interLeave) 
		{
			puc = pDst + lumaSize;
			addr = fbSrc->bufCb;
			vdi_read_memory(core_idx, addr, (Uint8 *)( puc ), chromaSize*2,  endian);
		}
		else
		{
			puc = pDst + lumaSize;
			addr = fbSrc->bufCb;
			vdi_read_memory(core_idx, addr, (Uint8 *)( puc ), chromaSize,  endian);

			puc = pDst + lumaSize + chromaSize;
			addr = fbSrc->bufCr;
			vdi_read_memory(core_idx, addr, (Uint8 *)( puc ), chromaSize,  endian);

		}
	}
	else
	{
		for (y = 0; y < nY; ++y) {
			vdi_read_memory(core_idx, addr, (Uint8 *)( puc + y * width ), width,  endian);
		}

		if (interLeave) 
		{
			puc = pDst + lumaSize;
			addr = fbSrc->bufCb;
			for (y = 0; y < (chromaHeight/2); ++y) {
				vdi_read_memory(core_idx, addr + (chromaStride*2)*y, (Uint8 *)(puc + y*(chromaWidth*2)), (chromaWidth*2),  endian);

			}
		}
		else
		{
			puc = pDst + lumaSize;
			addr = fbSrc->bufCb;
			for (y = 0; y < nCb; ++y) {
				vdi_read_memory(core_idx, addr + chromaStride * y, (Uint8 *)(puc + y * chromaWidth), chromaWidth,  endian);
			}

			puc = pDst + lumaSize + chromaSize;
			addr = fbSrc->bufCr;
			for (y = 0; y < nCr; ++y) { 
				vdi_read_memory(core_idx, addr + chromaStride * y, (Uint8 *)(puc + y * chromaWidth), chromaWidth,  endian);
			}
		}

	}
}

int ProcessEncodedBitstreamBurst(Uint32 core_idx, osal_file_t fp, int targetAddr,
	PhysicalAddress bsBufStartAddr, PhysicalAddress bsBufEndAddr,
	int size, int endian )
{
	Uint8 * buffer = 0;
	int room = 0;
	int file_wr_size = 0;

	buffer = (Uint8 *)osal_malloc(size);
	if( ( targetAddr + size ) > (int)bsBufEndAddr )
	{
		room = bsBufEndAddr - targetAddr;
		vdi_read_memory(core_idx, targetAddr, buffer, room,  endian);
		vdi_read_memory(core_idx, bsBufStartAddr, buffer+room, (size-room), endian);
	}
	else
	{
		vdi_read_memory(core_idx, targetAddr, buffer, size, endian); 
	}

	if (fp) {
		file_wr_size = osal_fwrite(buffer, sizeof(Uint8), size, fp);
		osal_fflush(fp);
	}

	osal_free( buffer );

	return file_wr_size;
}


void LoadEncodedBitstreamBurst(Uint32 core_idx, unsigned char * buffer, int targetAddr,
	PhysicalAddress bsBufStartAddr, PhysicalAddress bsBufEndAddr,
	int size, int endian )
{
	int room = 0;

	if( ( targetAddr + size ) > (int)bsBufEndAddr )
	{
		room = bsBufEndAddr - targetAddr;
		vdi_read_memory(core_idx, targetAddr, buffer, room,  endian);
		vdi_read_memory(core_idx, bsBufStartAddr, buffer+room, (size-room), endian);
	}
	else
	{
		vdi_read_memory(core_idx, targetAddr, buffer, size, endian); 
	}
}


int FillSdramBurst(Uint32 core_idx, BufInfo *pBufInfo, Uint32 targetAddr,
	vpu_buffer_t *pVbStream,
	Uint32 size,  int checkeos,
	int *streameos, int endian )
{
	int room;
	//Uint8 *pBuf                    = (Uint8 *)pVbStream->virt_addr;
	PhysicalAddress bsBufStartAddr = pVbStream->phys_addr;
	PhysicalAddress bsBufEndAddr   = pVbStream->phys_addr+pVbStream->size;
	int offset         = targetAddr - bsBufStartAddr;
    int             readlen        = 0;


	pBufInfo->count = 0;

	if (checkeos == 1 && (pBufInfo->point >= pBufInfo->size))
	{
		*streameos = 1;
		return 0;
	}

	if (offset<0)				return -1;
	if (offset>(int)pVbStream->size)	return -1;


	if ((pBufInfo->size - pBufInfo->point) < (int)size)
		pBufInfo->count = (pBufInfo->size - pBufInfo->point);
	else
		pBufInfo->count = size;

    if (pBufInfo->random_access_point >= 0)
	{
		osal_fseek(pBufInfo->fp, pBufInfo->random_access_point, (int)SEEK_SET);
		pBufInfo->point = pBufInfo->random_access_point;
		pBufInfo->random_access_point = -1;
	}
	readlen = osal_fread(pBufInfo->buf, pBufInfo->count, sizeof( Uint8 ), pBufInfo->fp);

    //VLOG(ERR, "FillSdramBurst: pBufInfo->count=%d, readlen=%d\n", pBufInfo->count, readlen);
    //VLOG(ERR, "FillSdramBurst: targetAddr=%x, bsBufEndAddr=%x\n", targetAddr, bsBufEndAddr);

	if( (targetAddr+pBufInfo->count) > bsBufEndAddr) //
	{   
		room   = bsBufEndAddr - targetAddr;
		//write to physical address
		vdi_write_memory(core_idx, targetAddr, pBufInfo->buf, room, endian);
		vdi_write_memory(core_idx, bsBufStartAddr, pBufInfo->buf+room, (pBufInfo->count-room), endian);

	}
	else
	{
		//write to physical address
		vdi_write_memory(core_idx, targetAddr, pBufInfo->buf, pBufInfo->count, endian);
	}

	pBufInfo->point += pBufInfo->count;
	return pBufInfo->count;

}

