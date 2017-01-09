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

#define LOG_TAG "SprdCamera3Stream"
//#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <utils/Errors.h>
#include "SprdCamera3HWI.h"
#include "SprdCamera3Stream.h"
#include "SprdCamera3Channel.h"
#include "gralloc_priv.h"

using namespace android;

namespace sprdcamera {

SprdCamera3Stream::SprdCamera3Stream(camera3_stream_t *new_stream,
						camera_dimension_t dim, camera_stream_type_t type,
						void *userdata)
{
	mCameraStream = new_stream;
	mWidth = dim.width;
	mHeight = dim.height;
	mStreamType = type;
	mUserChan = userdata;
	mBuffNum = 0;

	mMemory = new SprdCamera3GrallocMemory();
	if (NULL == mMemory) {
		HAL_LOGE("object NULL");
	}
}

SprdCamera3Stream::~SprdCamera3Stream()
{
	if(mMemory){
		delete mMemory;
		mMemory = NULL;
	}
}

int SprdCamera3Stream::registerBuffers(uint32_t num_buffers, buffer_handle_t **buffers)
{
	return NO_ERROR;
}

int SprdCamera3Stream::getStreamType(camera_stream_type_t* stream_type)
{
	*stream_type = mStreamType;

	return NO_ERROR;
}

int SprdCamera3Stream::getStreamInfo(camera3_stream_t** stream)
{
	*stream = mCameraStream;

	return NO_ERROR;
}

int SprdCamera3Stream::getStreamSize(int32_t* width, int32_t* height)
{
	*width = mWidth;
	*height = mHeight;

	return NO_ERROR;
}

int SprdCamera3Stream::buffDoneQ2(uint32_t frameNumber, buffer_handle_t *buffer)
{
	Mutex::Autolock l(mLock);
	int ret = NO_ERROR;
	hal_buff_list_t* buff_hal = new hal_buff_list_t;

	hal_mem_info_t* buf_mem_info = &(buff_hal->mem_info);
	if(buff_hal == NULL) {
		HAL_LOGE("ERROR: buff_hal is 0x%lx", buff_hal);
		return BAD_VALUE;
	}

	if(mMemory == NULL) {
		HAL_LOGE("mMemory is NULL");
		delete buff_hal;
		return BAD_VALUE;
	}

	buff_hal->buffer_handle = buffer;
	ret = mMemory->map2(buffer, buf_mem_info);
	if(ret != NO_ERROR) {
		HAL_LOGE("buffer queue Done Q buffer(0x%lx) error", buffer);
		mBuffNum++;
		buff_hal->buffer_handle = buffer;
		buff_hal->frame_number = frameNumber;
		buf_mem_info->addr_vir = NULL;
		buf_mem_info->addr_phy = NULL;
		mBufferList.add(buff_hal);
		ret = NO_ERROR;
	} else {
		HAL_LOGV("addr_phy = 0x%lx, addr_vir = 0x%lx, size = 0x%lx, mStreamType = %d", buf_mem_info->addr_phy, buf_mem_info->addr_vir, buf_mem_info->size, mStreamType);
		mBuffNum++;
		buff_hal->buffer_handle = buffer;
		buff_hal->frame_number = frameNumber;
		HAL_LOGD("Fnumber %d, handle 0x%lx, SType %d",buff_hal->frame_number,buffer, mStreamType);
		mBufferList.add(buff_hal);
	}

	return ret;
}

int SprdCamera3Stream::buffDoneQ(uint32_t frameNumber, buffer_handle_t *buffer)
{
	Mutex::Autolock l(mLock);
	int ret = NO_ERROR;
	hal_buff_list_t* buff_hal = new hal_buff_list_t;

	hal_mem_info_t* buf_mem_info = &(buff_hal->mem_info);
	if(buff_hal == NULL) {
		HAL_LOGE("ERROR: buff_hal is 0x%lx", buff_hal);
		return BAD_VALUE;
	}

	if(mMemory == NULL) {
		HAL_LOGE("mMemory is NULL");
		delete buff_hal;
		return BAD_VALUE;
	}

	buff_hal->buffer_handle = buffer;
	ret = mMemory->map(buffer, buf_mem_info);
	if(ret != NO_ERROR) {
		HAL_LOGE("buffer queue Done Q buffer(0x%lx) error", buffer);
		mBuffNum++;
		buff_hal->buffer_handle = buffer;
		buff_hal->frame_number = frameNumber;
		buf_mem_info->addr_vir = NULL;
		buf_mem_info->addr_phy = NULL;
		mBufferList.add(buff_hal);
		ret = NO_ERROR;
	} else {
		HAL_LOGV("addr_phy = 0x%lx, addr_vir = 0x%lx, size = 0x%lx, mStreamType = %d", buf_mem_info->addr_phy, buf_mem_info->addr_vir, buf_mem_info->size, mStreamType);
		mBuffNum++;
		buff_hal->buffer_handle = buffer;
		buff_hal->frame_number = frameNumber;
		HAL_LOGD("Fnumber %d, handle 0x%lx, SType %d",buff_hal->frame_number,buffer, mStreamType);
		mBufferList.add(buff_hal);
	}

	return ret;
}

int SprdCamera3Stream::buffDoneDQ(uint32_t frameNumber, buffer_handle_t **buffer)
{
	Mutex::Autolock l(mLock);
	int ret = NO_ERROR;
	Vector<hal_buff_list_t*>::iterator iter;

	if(mMemory == NULL) {
		HAL_LOGE("mMemory is NULL");
		return BAD_VALUE;
	}

	for (iter=mBufferList.begin();iter!=mBufferList.end();iter++)
	{
		if((*iter) && (*iter)->frame_number == frameNumber)
		{
			*buffer = (*iter)->buffer_handle;
			mMemory->unmap((*iter)->buffer_handle, &((*iter)->mem_info));
			HAL_LOGD("Fnumber %d, SType %d",(*iter)->frame_number, mStreamType);
			delete *iter;
			mBufferList.erase(iter);

			return ret;
		}
	}

	return BAD_VALUE;
}

int SprdCamera3Stream::buffFirstDoneDQ(uint32_t *frameNumber, buffer_handle_t **buffer)
{
	Mutex::Autolock l(mLock);
	Vector<hal_buff_list_t*>::iterator iter;

	if(mMemory == NULL) {
		HAL_LOGE("mMemory is NULL");
		return BAD_VALUE;
	}

	if(mBufferList.size()) {
		iter = mBufferList.begin();

		*buffer = (*iter)->buffer_handle;
		*frameNumber = (uint32_t)((*iter)->frame_number);
		mMemory->unmap((*iter)->buffer_handle, &((*iter)->mem_info));

		HAL_LOGD("buffer queue First Done DQ frame_number = %d, mStreamType = %d",(*iter)->frame_number, mStreamType);

		delete *iter;
		mBufferList.erase(iter);

		return NO_ERROR;
	}

	return BAD_VALUE;
}

int SprdCamera3Stream::getHeapSize(uint32_t* mm_heap_size)
{
	Vector<hal_buff_list_t*>::iterator iter=mBufferList.begin();
	const private_handle_t *priv_handle = NULL;

	if((*iter) == NULL) {
		HAL_LOGE("stream has no buffer");
		return BAD_VALUE;
	}

	priv_handle = reinterpret_cast<const private_handle_t *>(*((*iter)->buffer_handle));
	*mm_heap_size = priv_handle->size;

	return NO_ERROR;
}

int SprdCamera3Stream::getHeapNum(uint32_t* mm_heap_num)
{
	//*mm_heap_num = mBufferTable.size();

	return NO_ERROR;
}

int SprdCamera3Stream::getRegisterBuffPhyAddr(cmr_uint* buff_phy)
{
	/*cmr_uint* buff_phy_p = buff_phy;
	for (Vector<hal_buffer_idex_table_t*>::iterator iter=mBufferTable.begin();iter!=mBufferTable.end();iter++)
	{
		if(*iter){
			*buff_phy_p = (cmr_uint)((*iter)->mem_info.addr_phy);
			buff_phy_p++;
		}
	}*/

	return NO_ERROR;
}

int SprdCamera3Stream::getRegisterBuffVirAddr(cmr_uint* buff_vir)
{
	/*cmr_uint* buff_vir_p = buff_vir;
	for (Vector<hal_buffer_idex_table_t*>::iterator iter=mBufferTable.begin();iter!=mBufferTable.end();iter++)
	{
		if(*iter){
			*buff_vir_p = (cmr_uint)((*iter)->mem_info.addr_vir);
			buff_vir_p++;
		}
	}*/

	return NO_ERROR;
}

int SprdCamera3Stream::getQBufListNum(int32_t* buff_num)
{
	Mutex::Autolock l(mLock);
	*buff_num = mBufferList.size();
	return NO_ERROR;
}

int SprdCamera3Stream::getRegisterBufListNum(int32_t* buff_num)
{
	*buff_num = mBuffNum;
	return NO_ERROR;
}

int SprdCamera3Stream::getQBuffFirstVir(cmr_uint* addr_vir)
{
	Mutex::Autolock l(mLock);
	Vector<hal_buff_list_t*>::iterator iter;

	if(mBufferList.size()) {
		iter = mBufferList.begin();

		*addr_vir = (cmr_uint)((*iter)->mem_info.addr_vir);
		return NO_ERROR;
	}

	return BAD_VALUE;
}

int SprdCamera3Stream::getQBuffFirstPhy(cmr_uint* addr_phy)
{
	Mutex::Autolock l(mLock);
	Vector<hal_buff_list_t*>::iterator iter;
	if(mBufferList.size()) {
		iter = mBufferList.begin();
		*addr_phy = (cmr_uint)((*iter)->mem_info.addr_phy);
		return NO_ERROR;
	}
	return BAD_VALUE;
}

int SprdCamera3Stream::getQBuffFirstFd(cmr_uint* priv_data)
{
	Mutex::Autolock l(mLock);
	Vector<hal_buff_list_t*>::iterator iter;
	if(mBufferList.size()) {
		iter = mBufferList.begin();
		*priv_data = (cmr_uint)((*iter)->mem_info.fd);
		return NO_ERROR;
	}
	return BAD_VALUE;
}

int SprdCamera3Stream::getQBuffFirstNum(uint32_t* frameNumber)
{
	Mutex::Autolock l(mLock);
	Vector<hal_buff_list_t*>::iterator iter;

	if(mBufferList.size()) {
		iter = mBufferList.begin();

		*frameNumber = (uint32_t)((*iter)->frame_number);
		return NO_ERROR;
	}

	return BAD_VALUE;
}

int SprdCamera3Stream::getQBufAddrForNum(uint32_t frameNumber, cmr_uint* addr_vir, cmr_uint* addr_phy)
{
	Mutex::Autolock l(mLock);
	int ret = NO_ERROR;
	Vector<hal_buff_list_t*>::iterator iter;

	for (iter=mBufferList.begin();iter!=mBufferList.end();iter++)
	{
		if((*iter) && (*iter)->frame_number == frameNumber)
		{
			*addr_vir = (cmr_uint)((*iter)->mem_info.addr_vir);
			*addr_phy = (cmr_uint)((*iter)->mem_info.addr_phy);
			return ret;
		}
	}

	return BAD_VALUE;
}

int SprdCamera3Stream::getQBufHandleForNum(uint32_t frameNumber, buffer_handle_t** buff)
{
	Mutex::Autolock l(mLock);
	int ret = NO_ERROR;
	Vector<hal_buff_list_t*>::iterator iter;

	for (iter=mBufferList.begin();iter!=mBufferList.end();iter++)
	{
		if((*iter) && (*iter)->frame_number == frameNumber)
		{
			*buff = (*iter)->buffer_handle;
			return ret;
		}
	}
	return BAD_VALUE;
}
int SprdCamera3Stream::getQBufNumForVir(uintptr_t addr_vir, uint32_t* frameNumber)
{
	Mutex::Autolock l(mLock);
	int ret = NO_ERROR;
	Vector<hal_buff_list_t*>::iterator iter;
	for (iter=mBufferList.begin();iter!=mBufferList.end();iter++)
	{
		if((*iter) && (*iter)->mem_info.addr_vir == (void*)addr_vir)
		{
			*frameNumber = (*iter)->frame_number;
			return ret;
		}
	}

	return BAD_VALUE;
}

int SprdCamera3Stream::getQBufForHandle(buffer_handle_t* buff, cmr_uint* addr_vir, cmr_uint* addr_phy, cmr_uint* priv_data)
{
	Mutex::Autolock l(mLock);
	int ret = NO_ERROR;
	Vector<hal_buff_list_t*>::iterator iter;

	for (iter=mBufferList.begin();iter!=mBufferList.end();iter++)
	{
		if((*iter) && (*iter)->buffer_handle == buff)
		{
			*addr_vir = (cmr_uint)((*iter)->mem_info.addr_vir);
			*addr_phy = (cmr_uint)((*iter)->mem_info.addr_phy);
			*priv_data = (cmr_uint)((*iter)->mem_info.fd);
			return ret;
		}
	}

	return BAD_VALUE;
}

}; // namespace sprdcamera
