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


/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          Module              DESCRIPTION                             *
 ** 22/09/2013    Hardware Composer   Responsible for processing some         *
 **                                   Hardware layers. These layers comply    *
 **                                   with display controller specification,  *
 **                                   can be displayed directly, bypass       *
 **                                   SurfaceFligner composition. It will     *
 **                                   improve system performance.             *
 ******************************************************************************
 ** File: SprdHWComposer.h            DESCRIPTION                             *
 **                                   comunicate with SurfaceFlinger and      *
 **                                   other class objects of HWComposer       *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/


#ifndef _SPRD_HWCOMPOSER_H
#define _SPRD_HWCOMPOSER_H

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <fcntl.h>
#include <errno.h>

#include <EGL/egl.h>

#include <utils/RefBase.h>
#include <cutils/properties.h>
#include <cutils/atomic.h>
#include <cutils/log.h>

#include "SprdPrimaryDisplayDevice/SprdPrimaryDisplayDevice.h"
#include "SprdVirtualDisplayDevice/SprdVirtualDisplayDevice.h"
#include "SprdExternalDisplayDevice/SprdExternalDisplayDevice.h"
#include "SprdDisplayDevice.h"
#include "SprdUtil.h"

#include "dump.h"

using namespace android;


class SprdHWComposer: public hwc_composer_device_1_t
{
public:
    SprdHWComposer()
        : mPrimaryDisplay(0),
          mExternalDisplay(0),
          mVirtualDisplay(0),
          mFBInfo(0),
          mInitFlag(0),
          mDebugFlag(0),
          mDumpFlag(0)
    {

    }

    ~SprdHWComposer();

    /*
     *  Allocate and initialize the local objects used by HWComposer
     * */
    bool Init();

    /*
     *  Traversal display device, and find layers which comply with display device.
     *  and mark them as HWC_OVERLAY.
     * */
    int prepareDisplays(size_t numDisplays, hwc_display_contents_1_t **displays);

    /*
     *  Post layers to display device.
     * */
    int commitDisplays(size_t numDisplays, hwc_display_contents_1_t **displays);

    /*
     *  Blanks or unblanks a display's screen.
     *  Turns the screen off when blank is nonzero, on when blank is zero.
     * */
    int blank(int disp, int blank);

    /*
     *  Used to retrieve information about the h/w composer.
     * */
    int query(int what, int* value);

    void dump(char *buff, int buff_len);

    /*
     *  returns handles for the configurations available
     *  on the connected display. These handles must remain valid
     *  as long as the display is connected.
     * */
    int getDisplayConfigs(int disp, uint32_t* configs, size_t* numConfigs);

    /*
     *  returns attributes for a specific config of a
     *  connected display. The config parameter is one of
     *  the config handles returned by getDisplayConfigs.
     * */
    int getDisplayAttributes(int disp, uint32_t config, const uint32_t* attributes, int32_t* value);

    /*
     *  returns the index of the configuration that is currently active
     *  on the connected display.  The index is relative to the list of
     *  configuration handles returned by getDisplayConfigs.
     *  If there is no active configuration, -1 shall be returned.
     * */
    int getActiveConfig(int disp);

    /*
     *  instructs the hardware composer to switch to the display
     *  configuration at the given index in the list of configuration
     *  handles returned by getDisplayConfigs.
     * */
    int setActiveConfig(int disp, int index);

    /*
     *  For HWC 1.4 and above, setPowerMode() will be used in place of
     *  blank().
     *  setPowerMode(..., mode)
     *  Sets the display screen's power state.
     *
     *  The expected functionality for the various modes is as follows:
     *  HWC_POWER_MODE_OFF    : Turn the display off.
     *  HWC_POWER_MODE_DOZE   : Turn on the display (if it was previously
     *                          off) and put the display in a low power mode.
     *  HWC_POWER_MODE_NORMAL : Turn on the display (if it was previously
     *                          off), and take it out of low power mode.
     *
     *  The functionality is similar to the blank() command in previous
     *  versions of HWC, but with support for more power states.
     *  The display driver is expected to retain and restore the low power
     *  state of the display while entering and exiting from suspend.
     *
     *  Multiple sequential calls with the same mode value must be supported.
     *
     *  The screen state transition must be be complete when the function
     *  returns.
     *
     *  returns 0 on success, negative on error.
     * */
    int setPowerMode(int disp, int mode);

    /*
     *  Asynchronously update the location of the cursor layer.
     *  Within the standard prepare()/set() composition loop, the client
     *  (surfaceflinger) can request that a given layer uses dedicated cursor
     *  composition hardware by specifiying the HWC_IS_CURSOR_LAYER flag. Only
     *  one layer per display can have this flag set. If the layer is suitable
     *  for the platform's cursor hardware, hwcomposer will return from
     *  prepare() a composition type of HWC_CURSOR_OVERLAY for that layer.
     *  This indicates not only that the client is not responsible for
     *  compositing that layer.
     *  but also that the client can continue to update the position of
     *  that layer after a call to set(). This can reduce the visible latency
     *  of mouse movement to visible, on-screen cursor updates. Calls to
     *  setCursorPositionAsync() may be made from a different thread doing the
     *  prepare()/set() composition loop, but care must be taken to not
     *  interleave calls of setCursorPositionAsync() between calls of
     *  set()/prepare().
     *
     *   Notes:
     *   - Only one layer per display can be specified as a cursor layer with
     *    HWC_IS_CURSOR_LAYER.
     *   - hwcomposer will only return one layer per display as
     *    HWC_CURSOR_OVERLAY
     *    - This returns 0 on success or -errno on error.
     *    - This field is optional for HWC_DEVICE_API_VERSION_1_4 and later.
     *      It should be null for previous versions.
     * */
    int setCursorPositionAsync(int disp, int x_pos, int y_pos);

    /*
     *  Registor a callback from Android Framework.
     * */
    void registerProcs(hwc_procs_t const* procs);

    /*
     *  Control vsync event, enable or disable.
     * */
    bool eventControl(int disp, int enabled);

    /*
     *  pass the number of builtin display vendor support to SurfaceFlinger.
     * */
    int getBuiltInDisplayNum(uint32_t *number);


private:
    SprdPrimaryDisplayDevice  *mPrimaryDisplay;
    SprdExternalDisplayDevice *mExternalDisplay;
    SprdVirtualDisplayDevice  *mVirtualDisplay;
    FrameBufferInfo *mFBInfo;
    DisplayAttributes mDisplayAttributes[MAX_DISPLAYS];
    int mInitFlag;
    int mDebugFlag;
    int mDumpFlag;

    void resetDisplayAttributes();

    int DevicePropertyProbe(size_t numDisplays, hwc_display_contents_1_t **displays);
};

#endif
