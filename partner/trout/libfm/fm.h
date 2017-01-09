/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ANDROID_FM_INTERFACE_H
#define ANDROID_FM_INTERFACE_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <hardware/hardware.h>

__BEGIN_DECLS

/**
 * The id of this module
 */
#define FM_HARDWARE_MODULE_ID "fm"


struct fm_device_t {
    struct hw_device_t common;

    /**
     * Set the provided lights to the provided values.
     *
     * Returns: 0 on succes, error code on failure.
     */
    int (*getFreq)(struct fm_device_t* dev, int* freq);
    int (*setFreq)(struct fm_device_t* dev, int freq);
    int (*setControl)(struct fm_device_t* dev,int id, int value);
    int (*startSearch)(struct fm_device_t* dev,int freq, int dir, int timeout, int reserve);
    int (*cancelSearch)(struct fm_device_t*dev);
    int (*getRssi)(struct fm_device_t* dev, int* rssi);
};


__END_DECLS

#endif  // ANDROID_LIGHTS_INTERFACE_H

