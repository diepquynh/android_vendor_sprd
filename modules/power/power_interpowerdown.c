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

#define LOG_TAG "InterPowerdownHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define INTER_BOOSTPULSE_PATH1 "/sys/devices/system/cpu/cpu0/cpufreq/interactive/boostpulse"
#define INTER_BOOSTPULSE_PATH2 "/sys/devices/system/cpu/cpu4/cpufreq/interactive/boostpulse"
#define GTS_BOOSTPULSE_PATH "/sys/kernel/hmp/packing_boostpulse"
#define INTER_BOOSTPULSE_DURATION_PATH1 "/sys/devices/system/cpu/cpu0/cpufreq/interactive/boostpulse_duration"
#define INTER_BOOSTPULSE_DURATION_PATH2 "/sys/devices/system/cpu/cpu4/cpufreq/interactive/boostpulse_duration"
#define GTS_BOOSTPULSE_DURATION_PATH "/sys/kernel/hmp/boostpulse_duration"
#define INTER_BOOST_PATH1 "/sys/devices/system/cpu/cpu0/cpufreq/interactive/boost"
#define INTER_BOOST_PATH2 "/sys/devices/system/cpu/cpu4/cpufreq/interactive/boost"
#define GTS_BOOST_PATH "/sys/kernel/hmp/boost"
/*
 * FIX ME, use /dev/null replace real cpufreq interface
 * #define CPU_MAX_FREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
 */
#define CPU_MAX_FREQ_PATH "/dev/null"
#define LOW_POWER_MAX_FREQ "1000000"
#define NORMAL_MAX_FREQ "1300000"
#define STATE_ON "state=1"
#define STATE_OFF "state=0"
#define STATE_HDR_ON "state=2"
#define STATE_HDR_OFF "state=3"

struct boostpulse_fd_t {
    int inter_boostpulse_fd1;
    int gts_boostpulse_fd;
    int inter_boostpulse_warned1;
    int inter_boostpulse_warned2;
    int gts_boostpulse_warned;
};
struct interpowerdown_power_module {
    struct power_module base;
    pthread_mutex_t lock;
    struct boostpulse_fd_t boostpulse_fd;
    const char *touchscreen_power_path;
};

static unsigned int vsync_count;
static struct timespec last_touch_boost;
static bool touch_boost;
static bool low_power_mode = false;

static void sysfs_write(const char *path, char *s) {
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

static void power_init(struct power_module __unused *module) {
    sysfs_write(INTER_BOOSTPULSE_DURATION_PATH1, "500000");
}

static void power_set_interactive(struct power_module __unused *module, int on) {
    ALOGV("power_set_interactive: %d\n", on);

    /*
     * Lower maximum frequency when screen is off.
     * */
    sysfs_write(CPU_MAX_FREQ_PATH,
                (!on || low_power_mode) ? LOW_POWER_MAX_FREQ : NORMAL_MAX_FREQ);

    ALOGV("power_set_interactive: %d done\n", on);
}

static void boostpulse_open(struct interpowerdown_power_module *pm) {
    char buf[80];

    pthread_mutex_lock(&pm->lock);

    if (pm->boostpulse_fd.inter_boostpulse_fd1 < 0) {
        pm->boostpulse_fd.inter_boostpulse_fd1 = open(INTER_BOOSTPULSE_PATH1, O_WRONLY);

        if (pm->boostpulse_fd.inter_boostpulse_fd1 < 0) {
            if (!pm->boostpulse_fd.inter_boostpulse_warned1) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error opening %s: %s\n", INTER_BOOSTPULSE_PATH1, buf);
                pm->boostpulse_fd.inter_boostpulse_warned1 = 1;
            }
        }
    }

    if (pm->boostpulse_fd.gts_boostpulse_fd < 0) {
        pm->boostpulse_fd.gts_boostpulse_fd = open(GTS_BOOSTPULSE_PATH, O_WRONLY);

        if (pm->boostpulse_fd.gts_boostpulse_fd < 0) {
            if (!pm->boostpulse_fd.gts_boostpulse_warned) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error opening %s: %s\n", GTS_BOOSTPULSE_PATH, buf);
                pm->boostpulse_fd.gts_boostpulse_warned = 1;
            }
        }
    }

    pthread_mutex_unlock(&pm->lock);
}

static void interpowerdown_power_hint(struct power_module *module, power_hint_t hint,
                             void *data) {
    struct interpowerdown_power_module *pm = (struct interpowerdown_power_module *) module;
    struct timespec now, diff;
    char buf[80];
    int len;

    switch (hint) {
    case POWER_HINT_INTERACTION:
        boostpulse_open(pm);
        pthread_mutex_lock(&pm->lock);
        if (pm->boostpulse_fd.inter_boostpulse_fd1 >= 0) {
            len = write(pm->boostpulse_fd.inter_boostpulse_fd1, "1", 1);
            if (len < 0) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error writing to %s: %s\n", INTER_BOOSTPULSE_PATH1, buf);
            }
        }

        if (pm->boostpulse_fd.gts_boostpulse_fd >= 0) {
            len = write(pm->boostpulse_fd.gts_boostpulse_fd, "1", 1);
            if (len < 0) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error writing to %s: %s\n", GTS_BOOSTPULSE_PATH, buf);
            }
        }
        pthread_mutex_unlock(&pm->lock);
        break;

    case POWER_HINT_VSYNC:
        break;

    case POWER_HINT_VIDEO_ENCODE:
        ALOGI("POWER_HINT_VIDEO_ENCODE: %s", (char *)data);
        pthread_mutex_lock(&pm->lock);
        if (data) {
            if (!strncmp(data, STATE_ON, sizeof(STATE_ON))) {
                /* Video encode started */
                sysfs_write(GTS_BOOST_PATH, "1");
            } else if (!strncmp(data, STATE_OFF, sizeof(STATE_OFF))) {
                /* Video encode stopped */
                sysfs_write(GTS_BOOST_PATH, "0");
            }
        }
        pthread_mutex_unlock(&pm->lock);
        break;

    case POWER_HINT_LOW_POWER:
        pthread_mutex_lock(&pm->lock);
        if (data)
            sysfs_write(CPU_MAX_FREQ_PATH, LOW_POWER_MAX_FREQ);
        else
            sysfs_write(CPU_MAX_FREQ_PATH, NORMAL_MAX_FREQ);
        low_power_mode = data;
        pthread_mutex_unlock(&pm->lock);
        break;
    default:
        break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct interpowerdown_power_module HAL_MODULE_INFO_SYM = {
    base: {
        common: {
            tag: HARDWARE_MODULE_TAG,
            module_api_version: POWER_MODULE_API_VERSION_0_2,
            hal_api_version: HARDWARE_HAL_API_VERSION,
            id: POWER_HARDWARE_MODULE_ID,
            name: "Spreadtrum Power HAL",
            author: "The Android Open Source Project",
            methods: &power_module_methods,
        },

        init: power_init,
        setInteractive: power_set_interactive,
        powerHint: interpowerdown_power_hint,
    },

    lock: PTHREAD_MUTEX_INITIALIZER,
    boostpulse_fd: {
        inter_boostpulse_fd1: -1,
        gts_boostpulse_fd: -1,
        inter_boostpulse_warned1: 0,
        inter_boostpulse_warned2: 0,
        gts_boostpulse_warned: 0,
    },
};

