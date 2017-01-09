/*
 * Copyright (C) 2012 The Android Open Source Project
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

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/time.h>
#include <stdbool.h>
#include <cutils/properties.h>

#define LOG_TAG "InterHotplugPowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define BOOSTPULSE_PATH "/sys/devices/system/cpu/cpufreq/interactive/boostpulse"
#define CPU_IO_IS_BUSY_PATH "/sys/devices/system/cpu/cpufreq/interactive/io_is_busy"
#define CPU_MAX_FREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define CPU_IO_IS_BUSY "1"
#define LOW_POWER_MAX_FREQ "1000000"
#define NORMAL_MAX_FREQ "1300000"

/* < Bug#601097  Add PowerHint interaction subtype for different performance scenes */
// Power hint interaction subtype, keep same with PowerManagerInternal.java
#define INTERACTION_TYPE_LAUNCH (2)

// CPU-hotplug boost
#define CPU_HOTPLUG_BOOSTPULSE_PATH              "/sys/devices/system/cpu/cpuhotplug/boostpulse"
#define CPU_HOTPLUG_BOOSTPULSE_DURATION_PATH     "/sys/devices/system/cpu/cpuhotplug/boostpulse_duration"


// Control properties
enum {
    ENABLE_BOOST= 0,
    ENABLE_TOUCH_BOOST,
    ENABLE_LAUNCH_BOOST,
    ENABLE_FLING_BOOST,
    ENABLE_DDR_BOOST,
    ENABLE_CAM_BOOST,
    FLING_BOOST_LEVEL,
    DDR_BOOST_LEVEL,
    DEBUG_MAX
};

static const char *power_hint_prop_keys[] = {
    "persist.sys.powerHint.enable",
    "persist.sys.powerHint.touch",
    "persist.sys.powerHint.launch",
    "persist.sys.powerHint.fling",
    "persist.sys.powerHint.ddr",
    "persist.sys.powerHint.camera",
    "persist.sys.powerHint.flinglevl",
    "persist.sys.powerHint.ddrlevl",
};

static int power_hint_props[DEBUG_MAX];

/**
 * Get the Power hint properities settings
 */
static void get_power_hint_props() {
    int i= 0;
    char prop[PROPERTY_VALUE_MAX] = {'\0'};

    for(i=0; i<DEBUG_MAX; i++) {
        property_get(power_hint_prop_keys[i], prop, "1");
        power_hint_props[i] = atoi(prop);
        ALOGE("%s = %d\n", power_hint_prop_keys[i], power_hint_props[i]);
    }
}
/* Bug#601097  Add PowerHint interaction subtype for different performance scenes > */

struct interhotplug_power_module {
    struct power_module base;
    pthread_mutex_t lock;
    const char *touchscreen_power_path;
};

static unsigned int vsync_count;
static struct timespec last_touch_boost;
static bool touch_boost;
static bool low_power_mode = false;

static void sysfs_write(const char *path, char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

static void power_init(struct power_module __unused *module)
{
    sysfs_write(CPU_IO_IS_BUSY_PATH,CPU_IO_IS_BUSY);
    /* < Bug#601097  Add PowerHint interaction subtype for different performance scenes */
    get_power_hint_props();
    /* Bug#601097  Add PowerHint interaction subtype for different performance scenes > */
}

static void power_set_interactive(struct power_module __unused *module, int on)
{
#if 0
    ALOGI("power_set_interactive: %d\n", on);

    /*
     * Lower maximum frequency when screen is off.
     * */
    sysfs_write(CPU_MAX_FREQ_PATH,
                (!on || low_power_mode) ? LOW_POWER_MAX_FREQ : NORMAL_MAX_FREQ);

    ALOGI("power_set_interactive: %d done\n", on);
#endif
}

/* < Bug#601097  Add PowerHint interaction subtype for different performance scenes */
#if 0
static int boostpulse_open(struct interhotplug_power_module *pm)
{
    char buf[80];

    pthread_mutex_lock(&pm->lock);

    if (pm->boostpulse_fd < 0) {
        pm->boostpulse_fd = open(BOOSTPULSE_PATH, O_WRONLY);

        if (pm->boostpulse_fd < 0) {
            if (!pm->boostpulse_warned) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error opening %s: %s\n", BOOSTPULSE_PATH, buf);
                pm->boostpulse_warned = 1;
            }
        }
    }

    pthread_mutex_unlock(&pm->lock);
    return pm->boostpulse_fd;
}
#endif
/* Bug#601097  Add PowerHint interaction subtype for different performance scenes > */

static void interhotplug_power_hint(struct power_module *module, power_hint_t hint,
                             void *data)
{
    struct interhotplug_power_module *pm = (struct interhotplug_power_module *) module;
    struct timespec now, diff;
    char buf[80];
    int len;
    /* < Bug#601097  Add PowerHint interaction subtype for different performance scenes */
    char prop[PROPERTY_VALUE_MAX] = {'\0'};
    int subtype = 0;

    if (data != NULL)
        subtype = *(int *)data;
    /* Bug#601097  Add PowerHint interaction subtype for different performance scenes > */

    switch (hint) {
     case POWER_HINT_INTERACTION:
        pthread_mutex_lock(&pm->lock);
        /* < Bug#601097  Add PowerHint interaction subtype for different performance scenes */
        switch (subtype) {
         case INTERACTION_TYPE_LAUNCH:
            if (power_hint_props[ENABLE_LAUNCH_BOOST]) {
                ALOGE("Cpu hotplug boost");
                sysfs_write(CPU_HOTPLUG_BOOSTPULSE_PATH, "4");
            }
            break;
         default:
            break;
        }
        sysfs_write(BOOSTPULSE_PATH, "1");
        pthread_mutex_unlock(&pm->lock);
        /* Bug#601097  Add PowerHint interaction subtype for different performance scenes > */
        break;

     case POWER_HINT_VSYNC:
        break;

    case POWER_HINT_LOW_POWER:
#if 0
        pthread_mutex_lock(&pm->lock);
        if (data)
            sysfs_write(CPU_MAX_FREQ_PATH, LOW_POWER_MAX_FREQ);
        else
            sysfs_write(CPU_MAX_FREQ_PATH, NORMAL_MAX_FREQ);
        low_power_mode = data;
        pthread_mutex_unlock(&pm->lock);
#endif
        break;
    default:
            break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct interhotplug_power_module HAL_MODULE_INFO_SYM = {
    base: {
        common: {
            tag: HARDWARE_MODULE_TAG,
            module_api_version: POWER_MODULE_API_VERSION_0_2,
            hal_api_version: HARDWARE_HAL_API_VERSION,
            id: POWER_HARDWARE_MODULE_ID,
            name: "InterHotplug Power HAL",
            author: "The Android Open Source Project",
            methods: &power_module_methods,
        },

        init: power_init,
        setInteractive: power_set_interactive,
        powerHint: interhotplug_power_hint,
    },

    lock: PTHREAD_MUTEX_INITIALIZER,
};

