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

#ifndef __SPRDCAMERA_FLASH_H__
#define __SPRDCAMERA_FLASH_H__

#define SPRD_CAMERA_MAX_NUM_SENSORS 2

#include <hardware/camera_common.h>

#define SPRD_FLASH_CMD_OFF      "0x00"
#define SPRD_FLASH_CMD_ON       "0x11"

enum flash_status {
	SPRD_FLASH_STATUS_OFF,
	SPRD_FLASH_STATUS_ON,
	SPRD_FLASH_STATUS_MAX
};

namespace sprdcamera {

class SprdCamera3Flash {
public:
	static SprdCamera3Flash* getInstance();
	int32_t setFlashMode(const int camera_id, const bool on);
	int32_t reserveFlashForCamera(const int camera_id);
	int32_t releaseFlashFromCamera(const int camera_id);
	//const camera_module_callbacks_t *m_callbacks;
	int32_t set_torch_mode(const char*, bool);
	//External Interface
	static int32_t registerCallbacks(const camera_module_callbacks_t* callbacks);
	static int32_t setTorchMode(const char* cameraIdStr, bool on);
	static int32_t reserveFlash(const int cameraId);
	static int32_t releaseFlash(const int cameraId);

private:
	virtual ~SprdCamera3Flash();
	SprdCamera3Flash();
	SprdCamera3Flash(const SprdCamera3Flash&);
	SprdCamera3Flash& operator=(const SprdCamera3Flash&);
	const camera_module_callbacks_t *m_callbacks;
	int32_t m_flashFds[SPRD_CAMERA_MAX_NUM_SENSORS];
	bool m_flashOn;
	bool m_flashLastStat[SPRD_CAMERA_MAX_NUM_SENSORS];
	bool m_cameraOpen[SPRD_CAMERA_MAX_NUM_SENSORS];
	static SprdCamera3Flash * _instance;
};

}; // namespace sprdcamera

#endif /* __SPRDCAMERA_FLASH_H__ */
