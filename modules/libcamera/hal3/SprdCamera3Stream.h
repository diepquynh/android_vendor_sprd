/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __SPRDCAMERA3_STREAM_H__
#define __SPRDCAMERA3_STREAM_H__

#include <hardware/camera3.h>
#include "SprdCamera3Mem.h"

namespace sprdcamera {

class SprdCamera3Stream;
/*
typedef struct {
	hal_mem_info_t mem_info;
	buffer_handle_t *buffer_handle;
} hal_buffer_idex_table_t;
*/
typedef struct {
	buffer_handle_t *buffer_handle;
	uint32_t frame_number;
	hal_mem_info_t mem_info;
} hal_buff_list_t;

class SprdCamera3Stream
{
public:
	SprdCamera3Stream(camera3_stream_t *new_stream,
			camera_dimension_t dim, camera_stream_type_t type,
			void *userdata);
	virtual ~SprdCamera3Stream();
	int registerBuffers(uint32_t num_buffers, buffer_handle_t **buffers);
	int getStreamType(camera_stream_type_t* stream_type);
	int getStreamInfo(camera3_stream_t** stream);
	int getStreamSize(int32_t* width, int32_t* height);
	int buffDoneQ(uint32_t frameNumber, buffer_handle_t *buffer);
	// memory optimization, CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT dont need iommu addr
	int buffDoneQ2(uint32_t frameNumber, buffer_handle_t *buffer);
	int buffDoneDQ(uint32_t frameNumber, buffer_handle_t **buffer);
	int buffFirstDoneDQ(uint32_t *frameNumber, buffer_handle_t **buffer);
	int getHeapSize(uint32_t* mm_heap_size);
	int getHeapNum(uint32_t* mm_heap_num);
	int getRegisterBuffPhyAddr(cmr_uint* buff_phy);
	int getRegisterBuffVirAddr(cmr_uint* buff_vir);
	int getQBufListNum(int32_t* buff_num);
	int getRegisterBufListNum(int32_t* buff_num);
	int getQBuffFirstVir(cmr_uint* addr_vir);
	int getQBuffFirstNum(uint32_t* frameNumber);
	int getQBufAddrForNum(uint32_t frameNumber, cmr_uint* addr_vir, cmr_uint* addr_phy);
	int getQBufHandleForNum(uint32_t frameNumber, buffer_handle_t** buff);
	int getQBufNumForVir(uintptr_t addr_vir, uint32_t* frameNumber);
	int getQBufForHandle(buffer_handle_t* buff, cmr_uint* addr_vir, cmr_uint* addr_phy, cmr_uint* priv_data);
	int getQBuffFirstPhy(cmr_uint* addr_phy);
	int getQBuffFirstFd(cmr_uint *priv_data);

private:
	camera_stream_type_t mStreamType;
	int32_t mWidth;
	int32_t mHeight;
	void *mUserChan;
	camera3_stream_t * mCameraStream;

	uint32_t mBuffNum;
//	Vector<hal_buffer_idex_table_t*> mBufferTable;
	Vector<hal_buff_list_t*> mBufferList;

	SprdCamera3GrallocMemory* mMemory;
	Mutex	mLock;

};

}; // namespace sprdcamera

#endif /* __QCAMERA3_STREAM_H__ */
