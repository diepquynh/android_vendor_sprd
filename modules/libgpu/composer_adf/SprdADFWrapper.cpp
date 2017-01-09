/*
 * Copyright (C) 2010 The Android Open Source Project
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

/*
 *  SprdADFWrapper:: surpport Android ADF.
 *  It pass display data from HWC to Android ADF
 *  Author: zhongjun.chen@spreadtrum.com
*/

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

#include <cutils/log.h>
#include "SprdADFWrapper.h"
#include "dump.h"

static void SprdADFVsyncReport(void *data, int disp, uint64_t timestamp)
{
    SprdADFWrapper *ADF = static_cast<SprdADFWrapper *>(data);
    if (ADF == NULL)
    {
        ALOGE("SprdADFVsyncReport cannot get the SprdADFWrapper reference");
        return;
    }

    const hwc_procs_t *EventProcs = ADF->getEventProcs();
    if (EventProcs && EventProcs->vsync) {
        ALOGI_IF(g_debugFlag,"vsync come.");
        EventProcs->vsync(EventProcs, disp, timestamp);
    }
}

static void SprdADFHotPlugReport(void *data, int disp, bool connected)
{
    SprdADFWrapper *ADF = static_cast<SprdADFWrapper *>(data);
    if (ADF == NULL)
    {
        ALOGE("SprdADFHotPlugReport cannot get the SprdADFWrapper reference");
        return;
    }

    const hwc_procs_t *EventProcs = (ADF->getEventProcs());
    if (EventProcs && EventProcs->hotplug) {
        ALOGI_IF(g_debugFlag,"hotplug come.");
        EventProcs->hotplug(EventProcs, disp, connected);
    }
}

static void SprdADFCustomReport(void *data, int disp, struct adf_event *event)
{
    ALOGI_IF(g_debugFlag,"data:%p,disp:%d,event:%p",data,disp,event);
}

inline void implementBufferFormatConfig(int androidFormat,struct adf_buffer_config *bufferConfig,struct private_handle_t *privateH)
{
    int Bpf = privateH->stride*privateH->height;
    bufferConfig->fd[0]          = privateH->share_fd;
    bufferConfig->offset[0]      = 0;
    bufferConfig->n_planes       = 1;
    bufferConfig->pitch[0]       = privateH->stride*4;
    switch (androidFormat) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
            bufferConfig->format         = DRM_FORMAT_RGBA8888;
            break;
        case HAL_PIXEL_FORMAT_BGRA_8888:
            bufferConfig->format         = DRM_FORMAT_BGRA8888;
            break;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            bufferConfig->format         = DRM_FORMAT_RGBX8888;
            break;
        case HAL_PIXEL_FORMAT_RGB_888:
            bufferConfig->pitch[0]       = privateH->stride*3;
            bufferConfig->format         = DRM_FORMAT_RGB888;
            break;
        case HAL_PIXEL_FORMAT_RGB_565:
            bufferConfig->pitch[0]       = privateH->stride*2;
            bufferConfig->format         = DRM_FORMAT_RGB565;
            break;
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
            bufferConfig->n_planes       = 2;
            bufferConfig->fd[1]          = privateH->share_fd;
            bufferConfig->offset[1]      = Bpf;
            bufferConfig->pitch[0]       = privateH->stride*1;
            bufferConfig->pitch[1]       = bufferConfig->pitch[0];
            bufferConfig->format         = DRM_FORMAT_NV12;
            break;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            bufferConfig->n_planes       = 2;
            bufferConfig->fd[1]          = privateH->share_fd;
            bufferConfig->offset[1]      = Bpf;
            bufferConfig->pitch[0]       = privateH->stride*1;
            bufferConfig->pitch[1]       = bufferConfig->pitch[0];
            bufferConfig->format         = DRM_FORMAT_NV21;
            break;
        case HAL_PIXEL_FORMAT_YCbCr_422_SP:
            bufferConfig->n_planes       = 2;
            bufferConfig->fd[1]          = privateH->share_fd;
            bufferConfig->offset[1]      = Bpf;
            bufferConfig->pitch[0]       = privateH->stride*1;
            bufferConfig->pitch[1]       = bufferConfig->pitch[0];
            bufferConfig->format         = DRM_FORMAT_NV16;
            break;
        case HAL_PIXEL_FORMAT_YCrCb_422_SP:
            bufferConfig->n_planes       = 2;
            bufferConfig->fd[1]          = privateH->share_fd;
            bufferConfig->offset[1]      = Bpf;
            bufferConfig->pitch[0]       = privateH->stride*1;
            bufferConfig->pitch[1]       = bufferConfig->pitch[0];
            bufferConfig->format         = DRM_FORMAT_NV61;
            break;
        case HAL_PIXEL_FORMAT_YV12:
            bufferConfig->n_planes       = 3;
            bufferConfig->fd[1]          = privateH->share_fd;
            bufferConfig->fd[2]          = privateH->share_fd;
            bufferConfig->offset[1]      = Bpf;
            bufferConfig->offset[2]      = Bpf+ALIGN(privateH->stride/2, 16)*privateH->height/2;
            bufferConfig->pitch[0]       = privateH->stride*1;
            bufferConfig->pitch[1]       = ALIGN(privateH->stride/2, 16);
            bufferConfig->pitch[2]       = bufferConfig->pitch[1];
            bufferConfig->format         = DRM_FORMAT_YVU420;
            break;
        case HAL_PIXEL_FORMAT_YCbCr_422_I:
            bufferConfig->pitch[0]       = privateH->stride*2;
            bufferConfig->format         = DRM_FORMAT_YUYV;
            break;
        default:
            ALOGW("SprdADFWrapper::implementBufferFormatConfig() don't supported format:%x",androidFormat);
            bufferConfig->format = adf_fourcc_for_hal_pixel_format(androidFormat);
            break;
    }
}

SprdADFWrapper:: SprdADFWrapper()
    : mInitFlag(false),
      mNumInterfaces(0),
      mActiveContextCount(0),
      mLayerCount(0),
      mEventProcs(NULL),
      mInterfaceIDs(0),
      mDevice(0),
      mHelper(0),
      mInterfaces(0),
      mDebugFlag(0)
{
}

SprdADFWrapper:: ~SprdADFWrapper()
{
    mInitFlag = false;
    mNumInterfaces = 0;

    deInit();

    if (mInterfaceIDs)
    {
        free(mInterfaceIDs);
        mInterfaceIDs = NULL;
    }

    if (mDevice)
    {
        free(mDevice);
        mDevice = NULL;
    }

    if (mHelper)
    {
        free(mHelper);
        mHelper = NULL;
    }

    if (mInterfaces)
    {
        free(mInterfaces);
        mInterfaces = NULL;
    }
}

bool SprdADFWrapper:: Init()
{
    int *Fds;
    adf_id_t *IDs;
    ssize_t i;

    static const struct adf_hwc_event_callbacks Callbacks = {
        .vsync         = SprdADFVsyncReport,
        .hotplug       = SprdADFHotPlugReport,
        .custom_event  = SprdADFCustomReport,
    };

    mDevice = (struct adf_device *)malloc(sizeof(struct adf_device));
    if (mDevice == NULL)
    {
        ALOGE("SprdADFWrapper:: Init malloc mDevice failed");
        goto ERR0;
    }

    if (adf_devices(&IDs) <= 0)
    {
        ALOGE("SprdADFWrapper:: Init adf_devices failed");
        goto ERR1;
    }

    if (adf_device_open(IDs[0], O_RDWR, mDevice))
    {
        ALOGE("SprdADFWrapper:: Init adf_device_open failed");
        free(IDs);
        goto ERR1;
    }

    free(IDs);
    memset(mFlushContext, 0x00, sizeof(FlushContext) * DEFAULT_DISPLAY_TYPE_NUM);

    if (createADFInterface() < 0)
    {
        ALOGE("SprdADFWrapper:: Init createADFInterface failed");
        goto ERR2;
    }

    Fds = (int *)malloc(sizeof(int) * mNumInterfaces);
    if (Fds == NULL)
    {
        ALOGE("SprdADFWrapper:: Init malloc Fds failed");
        goto ERR3;
    }

    for (i = 0; i < mNumInterfaces; i++)
    {
        Fds[i] = mInterfaces[i].Fd;
    }

    if (adf_hwc_open(Fds, mNumInterfaces, &Callbacks, this, &mHelper))
    {
        ALOGE("SprdADFWrapper:: Init adf_hwc_open failed");
        free(Fds);
        goto ERR3;
    }

    free(Fds);

    mInitFlag = true;

    ALOGI_IF(mDebugFlag, "SprdADFWrapper:: Init success find interface num: %d", mNumInterfaces);

    return true;

ERR3:
    freeADFInterface();
ERR2:
    adf_device_close(mDevice);
ERR1:
    free(mDevice);
    mDevice = NULL;
ERR0:

    return false;
}

void SprdADFWrapper:: deInit()
{
    struct sigaction OldAction, NewAction;
    OldAction.sa_flags     = SA_RESTART | SA_SIGINFO;
    OldAction.sa_sigaction = DestroyADFSignalHandler;

    NewAction.sa_flags     = SA_RESTART | SA_SIGINFO;
    NewAction.sa_sigaction = DestroyADFSignalHandler;

    /*
     *  Workaround for ADF bug:
     *  pthread_kill maybe cause the precess exit.
     * */
    sigaction(SIGTERM, &NewAction, &OldAction);
    adf_hwc_close(mHelper);
    sigaction(SIGTERM, &OldAction, NULL);

    freeADFInterface();
    adf_device_close(mDevice);

    mInitFlag = false;
}

void SprdADFWrapper:: SetEventProcs(hwc_procs_t const* procs)
{
    if (procs == NULL)
    {
        ALOGE("SprdADFWrapper:: SetEventProcs input procs is NULL");
        return;
    }

    mEventProcs = procs;
}

int SprdADFWrapper:: QueryDisplayInfo(uint32_t *DisplayNum)
{
    int value;

    if (mInitFlag == false)
    {
        ALOGE("func: %s line: %d SprdADFWrapper Need Init first", __func__, __LINE__);
        return -1;
    }

    adf_query_display_types_supported(mHelper, &value);
    if (value & HWC_DISPLAY_EXTERNAL_BIT)
    {
        *DisplayNum = 2;
    }
    else
    {
        *DisplayNum = 1;
    }

    return 0;
}

int SprdADFWrapper:: GetConfigs(int DisplayType, uint32_t *Configs, size_t *NumConfigs)
{
    int ret = -1;

    if (mInitFlag == false)
    {
        ALOGE("func: %s line: %d SprdADFWrapper Need Init first", __func__, __LINE__);
        return -1;
    }

    ret = adf_getDisplayConfigs(mHelper, DisplayType, Configs, NumConfigs);
    if (ret)
    {
        ALOGE("SprdADFWrapper:: GetConfigs error, ret: %d", ret);
        return -1;
    }

    return 0;
}

int SprdADFWrapper:: GetConfigAttributes(int DisplayType, uint32_t Config, const uint32_t *attributes, int32_t *values)
{
    int ret = -1;

    if (mInitFlag == false)
    {
        ALOGE("func: %s line: %d SprdADFWrapper Need Init first", __func__, __LINE__);
        return -1;
    }

    ret = adf_getDisplayAttributes(mHelper, DisplayType, Config, attributes, values);

    return ret;
}

int SprdADFWrapper:: EventControl(int DisplayType, int event, bool enabled)
{
    int ret = -1;

    if (mInitFlag == false)
    {
        ALOGE("func: %s line: %d SprdADFWrapper Need Init first", __func__, __LINE__);
        return -1;
    }

    ret = adf_eventControl(mHelper, DisplayType, event, enabled);

    return ret;
}

int SprdADFWrapper:: Blank(int DisplayType, bool enabled)
{
    int ret = -1;
    if (mInitFlag == false)
    {
        ALOGE("func: %s line: %d SprdADFWrapper Need Init first", __func__, __LINE__);
        return -1;
    }

    ret = adf_blank(mHelper, DisplayType, enabled);

    return ret;
}

int SprdADFWrapper:: Dump(char *buffer)
{
    if (mInitFlag == false)
    {
        ALOGE("func: %s line: %d SprdADFWrapper Need Init first,buffer:%p", __func__, __LINE__,buffer);
        return -1;
    }

    return 0;
}


int SprdADFWrapper:: createADFInterface()
{
    int FirstPrimaryInterface = -1;
    int i, j;

    mNumInterfaces = adf_interfaces(mDevice, &mInterfaceIDs);
    if (mNumInterfaces <=0 )
    {
        ALOGE("SprdADFWrapper:: createADFInterface adf_interface failed mNumInterfaces: %d", mNumInterfaces);
        goto ERR0;
    }

    mInterfaces = (ADF_interface *)calloc(mNumInterfaces, sizeof(ADF_interface));
    if (mInterfaces == NULL)
    {
        ALOGE("SprdADFWrapper:: createADFInterface calloc mInterfaces failed");
        goto ERR1;
    }

    for (i = 0; i < mNumInterfaces; i++)
    {
        mInterfaces[i].Fd = -1;

        mInterfaces[i].Fd = adf_interface_open(mDevice, mInterfaceIDs[i], O_RDWR);
        if (mInterfaces[i].Fd < 0)
        {
            ALOGE("SprdADFWrapper:: createADFInterface adf_interface_open: %d failed", i);
            goto ERR2;
        }

        if (adf_get_interface_data(mInterfaces[i].Fd, &(mInterfaces[i].Data)))
        {
            ALOGE("SprdADFWrapper:: createADFInterface adf_get_interface_data i: %d", i);
            goto ERR2;
        }

        if (mInterfaces[i].Data.flags & ADF_INTF_FLAG_PRIMARY)
        {
            FirstPrimaryInterface = i;
        }

        if (mInterfaces[i].Data.flags & ADF_INTF_FLAG_EXTERNAL)
        {
            mFlushContext[DISPLAY_EXTERNAL].Interface = &mInterfaces[i];
            mFlushContext[DISPLAY_EXTERNAL].InterfaceId = mInterfaceIDs[i];
        }
    }

    /*
     *  Place ADF_INTF_FLAG_PRIMARY interface in the first slot.
     * */
    if (FirstPrimaryInterface > 0)
    {
        adf_id_t tmpId = mInterfaceIDs[0];
        ADF_interface tmpInterface = mInterfaces[0];

        i = FirstPrimaryInterface;

        mInterfaceIDs[0] = mInterfaceIDs[i];
        mInterfaceIDs[i] = tmpId;

        mInterfaces[0] = mInterfaces[i];
        mInterfaces[i] = tmpInterface;
    }

    mFlushContext[DISPLAY_PRIMARY].Interface = &mInterfaces[0];
    mFlushContext[DISPLAY_PRIMARY].InterfaceId = mInterfaceIDs[0];

    for (i = 0; i < mNumInterfaces; i++)
    {
        ADF_interface *pInter = &mInterfaces[i];

        pInter->numEngines = adf_overlay_engines_for_interface(mDevice, mInterfaceIDs[i], &pInter->EngineIDs);
        if (pInter->numEngines <= 0)
        {
            ALOGE("SprdADFWrapper:: createADFInterface interface %d has no overlay engine attached", mInterfaceIDs[i]);
            goto ERR2;
        }

        pInter->Engines = (ADF_overlay_engine *)calloc(pInter->numEngines, sizeof(ADF_overlay_engine));
        if (pInter->Engines == NULL)
        {
            ALOGE("SprdADFWrapper:: createADFInterface calloc Engines i: %d failed", i);
            goto ERR2;
        }

        for (j = 0; j < pInter->numEngines; j++)
        {
            ADF_overlay_engine *pEngine = &(pInter->Engines[j]);
            pEngine->Fd = -1;

            pEngine->Fd = adf_overlay_engine_open(mDevice, pInter->EngineIDs[j],
                                                  O_RDWR);
            if (pEngine->Fd < 0)
            {
                ALOGE("SprdADFWrapper:: createADFInterface adf_overlay_engine_open inter: %d, engine: %d", i, j);
                goto ERR2;
            }

            if (adf_get_overlay_engine_data(pEngine->Fd, &pEngine->Data))
            {
                ALOGE("SprdADFWrapper:: createADFInterface adf_get_overlay_engine_data inter: %d, engine: %d", i, j);
                goto ERR2;
            }
        }
    }

    return 0;

ERR2:
    freeADFInterface();
ERR1:
    free(mInterfaceIDs);
    mInterfaceIDs = NULL;
    mNumInterfaces = 0;
ERR0:
    return -1;
}

void SprdADFWrapper:: freeADFInterface()
{
    ssize_t i, j;

    for (i = 0; i < mNumInterfaces; i++)
    {
        ADF_interface *pI = &(mInterfaces[i]);

        adf_free_interface_data(&pI->Data);

        free(pI->EngineIDs);

        if (pI->Fd < 0)
        {
            break;
        }
        close(pI->Fd);

        for (j = 0; j < pI->numEngines; j++)
        {
            ADF_overlay_engine *pE = &(pI->Engines[j]);

            if (pE->Fd < 0)
            {
                break;
            }
            close(pE->Fd);

            adf_free_overlay_engine_data(&pE->Data);
        }
        free(pI->Engines);
    }

    free(mInterfaces);
    free(mInterfaceIDs);
}

void SprdADFWrapper:: DestroyADFSignalHandler(int Signal, siginfo_t *SigInfo, void *user)
{
    ALOGI_IF(g_debugFlag, "Signal:%d,SigInfo:%p,user:%p",Signal,SigInfo,user);
}

int SprdADFWrapper:: AddFlushData(int DisplayType, SprdHWLayer **list, int LayerCount)
{
    int i = 0;
    FlushContext *ctx = NULL;
    queryDebugFlag(&mDebugFlag);

    if ((DisplayType < 0) || (DisplayType > DEFAULT_DISPLAY_TYPE_NUM))
    {
        ALOGE("SprdADFWrapper:: AddFlushData DisplayType is invalidate");
        return -1;
    }

    if (list == NULL || LayerCount <= 0)
    {
        ALOGE("SprdADFWrapper:: AddFlushData context para error");
        return -1;
    }

    ctx = &(mFlushContext[DisplayType]);
    ctx->LayerCount = LayerCount;
    ctx->LayerList = list;
    ctx->DisplayType = DisplayType;
    ctx->user = NULL;
    ctx->Active = true;

    mLayerCount += ctx->LayerCount;
    mActiveContextCount++;

    ALOGI_IF(mDebugFlag, "SprdADFWrapper:: AddFlushData disp: %d, LayerCount: %d", DisplayType, LayerCount);

    return 0;
}

int SprdADFWrapper:: PostDisplay(DisplayTrack *tracker)
{
    int ret = -1;
    int i = 0;
    int j = 0;
    int interfaceNum = 0;
    int32_t currentIndex = 0;
    struct adf_buffer_config *BufferConfig;
    struct sprd_adf_post_custom_data *custom = NULL;
    struct sprd_adf_hwlayer_custom_data *adfLayers = NULL;
    int iCustomDataSize = 0;

    if (tracker == NULL)
    {
        ALOGE("SprdADFWrapper:: PostDisplay input para error");
        ret = -1;
        goto EXT0;
    }

    if (mLayerCount <= 0)
    {
        ALOGE("SprdADFWrapper:: PostDisplay No Layer should be displayed");
        ret = -1;
        goto EXT0;
    }

    BufferConfig = (struct adf_buffer_config *)calloc(mLayerCount, sizeof(struct adf_buffer_config));
    if (BufferConfig == NULL)
    {
        ALOGE("SprdADFWrapper:: PostDisplay calloc adf_buffer_config failed");
        ret = -1;
        goto EXT0;
    }

    iCustomDataSize = sizeof(struct sprd_adf_post_custom_data) + sizeof(struct sprd_adf_hwlayer_custom_data) * mLayerCount;
    custom = (struct sprd_adf_post_custom_data *)calloc(1, iCustomDataSize);
    if (!custom)
    {
         ALOGE("SprdADFWrapper:: PostDisplay calloc adfLayers failed");
         ret = -1;
         goto EXT1;
    }
    memset(custom, 0x00, iCustomDataSize);
    adfLayers = &custom->hwlayers[0];

    mActiveContextCount = (mActiveContextCount > DEFAULT_DISPLAY_TYPE_NUM)
                           ? DEFAULT_DISPLAY_TYPE_NUM : mActiveContextCount;

    for (i = 0; i < mActiveContextCount; i++)
    {
        adf_id_t overlayEngineId;
        FlushContext *ctx = getFlushContext(i);

        if (ctx->Active == false)
        {
            continue;
        }

        if (ctx->LayerCount <= 0)
        {
            continue;
        }
#if 1
        if (attachOverlayEngineToInterface(ctx, &overlayEngineId))
        {
            ALOGE("SprdADFWrapper:: PostDisplay attachOverlayEngineToInterface failed");
            ret = -1;
            goto EXT2;
        }
        ctx->OverlayEngineId = overlayEngineId;
#else
        ctx->OverlayEngineId = 0;
#endif
        for (j = 0; j < ctx->LayerCount; j++)
        {
            int32_t index = currentIndex + j;
            adf_id_t overlayEngineId = 0;
            SprdHWLayer *l = ctx->LayerList[j];

            if (implementBufferConfig(l, overlayEngineId,
                                      &(BufferConfig[index])))
            {
                ALOGE("SprdADFWrapper:: PostDisplay implementBufferConfig failed");
                ret = -1;
                continue;
            }

            implementCustomConfig(ctx, l, &(adfLayers[index]),index);

            ALOGI_IF(mDebugFlag, "SprdADFWrapper:: PostDisplay config %dth layer", index);
        }
        currentIndex += ctx->LayerCount;
        interfaceNum++;
    }

    custom->num_interfaces  = interfaceNum;

    static int f_c=0;
    ALOGI_IF(mDebugFlag, "adf_device_post() %d times, interfaceNum:%d, Layer count: %d",
		f_c++,interfaceNum, mLayerCount);

    ret = adf_device_post(mDevice, mInterfaceIDs, interfaceNum,
                          BufferConfig, mLayerCount,
                          custom, iCustomDataSize);
    if (ret < 0)
    {
        ALOGE("SprdADFWrapper:: PostDisplay adf_device_post error: %d", ret);
        goto EXT3;
    }

    tracker->releaseFenceFd = ret;
    tracker->retiredFenceFd = custom->retire_fence; // ? fill later;

    ALOGI_IF(mDebugFlag, "<05> af adf_device_post() return rel_fd:%d,retire_fd:%d,",
                          tracker->releaseFenceFd, tracker->retiredFenceFd);

    ret = 0;

EXT3:
    detachOverlayEngineFromInterfaces(&BufferConfig);
EXT2:
     free(custom);
     custom = NULL;
EXT1:
    free(BufferConfig);
    BufferConfig = NULL;
EXT0:
     mLayerCount = 0;
     mActiveContextCount = 0;
     invalidateFlushContext();

     return ret;
}

int SprdADFWrapper:: attachOverlayEngineToInterface(FlushContext *ctx, adf_id_t *overlayEngineId)
{
    int i = 0;
    int err = 0;
    ADF_interface *pI = getInterface(ctx->DisplayType);
    adf_id_t interfaceId = getInterfaceId(ctx->DisplayType);

    for (i = 0; pI != NULL && i < pI->numEngines; i++)
    {
        //ADF_overlay_engine *pE = &(pI->Engines[i]);

        err = adf_device_attach(mDevice, pI->EngineIDs[i], interfaceId);

        if (err == -EALREADY)
        {
            ALOGI_IF(mDebugFlag, "attachOverlayEngine engine maybe busy i: %d", i);
            continue;
        }

        if (err)
        {
            ALOGE("SprdADFWrapper:: attachOverlayEngineToInterface invalid config");
            goto EXT0;
        }
        *overlayEngineId = pI->EngineIDs[i];
        break;
    }

EXT0:
    return err;
}

void SprdADFWrapper:: detachOverlayEngineFromInterfaces(struct adf_buffer_config **bufferConfig)
{
    int i = 0;
    ALOGI_IF(mDebugFlag, "bufferConfig %p", bufferConfig);

    for (i = 0; i < mActiveContextCount; i++)
    {
        FlushContext *ctx = getFlushContext(i);

        adf_device_detach(mDevice, ctx->OverlayEngineId,
                          getInterfaceId(ctx->DisplayType));
    }
}


int SprdADFWrapper:: implementBufferConfig(SprdHWLayer *l, adf_id_t overlayEngineId, struct adf_buffer_config *bufferConfig)
{
    int format = -1;
    struct private_handle_t *privateH = NULL;
    if (l == NULL)
    {
        ALOGE("SprdADFWrapper:: implementBufferConfig input para invalid");
        return -1;
    }

    privateH = l->getBufferHandle();
    if (privateH == NULL)
    {
        ALOGE("SprdADFWrapper:: implementBufferConfig buffer handle error");
        return -1;
    }

    format = l->getLayerFormat();
    for (int k = 0; k < ADF_MAX_PLANES; k++)
    {
        bufferConfig->fd[k] = -1;
    }
    bufferConfig->overlay_engine = ~0U;
    bufferConfig->w              = privateH->width;
    bufferConfig->h              = privateH->height;
    bufferConfig->acquire_fence  = l->getAcquireFence();
    bufferConfig->overlay_engine = overlayEngineId;
    implementBufferFormatConfig(format,bufferConfig,privateH);

    ALOGI_IF(mDebugFlag, "SprdADFWrapper:: implementBufferConfig w:%d, h:%d,stride: %d, format: %d, bufFd: %d, acfence:%d, OVEnId: %d",
                         bufferConfig->w, bufferConfig->h, privateH->stride,
                         format, privateH->share_fd,
                         bufferConfig->acquire_fence, bufferConfig->overlay_engine);

    return 0;
}

void SprdADFWrapper:: implementCustomConfig(FlushContext *ctx, SprdHWLayer *l, struct sprd_adf_hwlayer_custom_data *adfLayer, int32_t index)
{
    struct private_handle_t *privateH = NULL;

    if (ctx == NULL || l == NULL || adfLayer == NULL)
    {
        ALOGE("SprdADFWrapper:: implementCustomConfig input para is NULL");
        return;
    }

    privateH = l->getBufferHandle();
    if (privateH == NULL)
    {
        ALOGE("SprdADFWrapper:: implementCustomConfig buffer handle error");
        return;
    }

    //struct sprdRect *src = l->getSprdSRCRect();
    struct sprdRect *fb  = l->getSprdFBRect();

    adfLayer->interface_id       = getInterfaceId(ctx->DisplayType);
    adfLayer->hwlayer_id         = (l->getLayerType() == LAYER_OSD) ? 0x00 : 0x01 ;
    adfLayer->buffer_id          = index;
    adfLayer->alpha              = l->getPlaneAlpha();
    adfLayer->dst_x              = fb->x;
    adfLayer->dst_y              = fb->y;
    adfLayer->dst_w              = fb->w;
    adfLayer->dst_h              = fb->h;
    adfLayer->blending           = convertToADFBlendMode(l->getBlendMode());
    adfLayer->rotation           = convertToADFTransform(l->getTransform());
    adfLayer->scale              = 0; // ?
    adfLayer->compression        = 0; // ?

    ALOGI_IF(mDebugFlag, "SprdADFWrapper:: implementCustomConfig interfaceId:%d,hwlayerid: %d, bufFd: %d, planeAlpha:%d, dst(x:%d,y:%d,w:%d,h:%d) blending: %x, transform: %d, scale: %d, compression: %d",
                         adfLayer->interface_id, adfLayer->hwlayer_id, adfLayer->buffer_id,
                         adfLayer->alpha, adfLayer->dst_x, adfLayer->dst_y, adfLayer->dst_w,
                         adfLayer->dst_h, adfLayer->blending, adfLayer->rotation,
                         adfLayer->scale, adfLayer->compression);
}

void SprdADFWrapper:: invalidateFlushContext()
{
    int i = 0;

    for (i = 0; i < mActiveContextCount; i++)
    {
        FlushContext *ctx = getFlushContext(i);

        ctx->LayerCount          = 0;
        ctx->LayerList           = NULL;
        ctx->DisplayType         = -1;
        ctx->Active              = false;
        ctx->OverlayEngineId     = -1;
    }
}

SprdADFWrapper::ADF_interface *SprdADFWrapper:: getInterface(int displayType)
{
    ADF_interface *pI = NULL;

    switch (displayType)
    {
        case DISPLAY_PRIMARY:
            pI = mFlushContext[DISPLAY_PRIMARY].Interface;
            break;
        case DISPLAY_EXTERNAL:
            pI = mFlushContext[DISPLAY_EXTERNAL].Interface;
            break;
    }

    return pI;
}

adf_id_t SprdADFWrapper:: getInterfaceId(int displayType)
{
     adf_id_t id = -1;

    switch (displayType)
    {
        case DISPLAY_PRIMARY:
            id = mFlushContext[DISPLAY_PRIMARY].InterfaceId;
            break;
        case DISPLAY_EXTERNAL:
            id = mFlushContext[DISPLAY_EXTERNAL].InterfaceId;
            break;
    }

    return id;
}

