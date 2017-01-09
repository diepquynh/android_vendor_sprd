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

#define LOG_TAG "SprdemandPowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

/*
 * for /sys/devices/system/cpu/cpufreq/sprdemand/boostpulse
 *
 * write it will get max-1 cpu frequency & more cpu cores.
 * the allowed value range from 1 to 4(CONFIG_NR_CPUS)
 * - 1: max-1 freq and 1 core
 * - 2: max-1 freq and 2 core
 * - 3: max-1 freq and 3 core
 * - 4: max-1 freq and 4 core
 */
#define BOOSTPULSE_PATH "/sys/devices/system/cpu/cpufreq/sprdemand/boostpulse"
#define CPU_MAX_FREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define LOW_POWER_MAX_FREQ "1000000"
#define NORMAL_MAX_FREQ "1300000"

struct sprdemand_power_module {
    struct power_module base;
    pthread_mutex_t lock;
    int boostpulse_fd;
    int boostpulse_warned;
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

static int boostpulse_open(struct sprdemand_power_module *pm)
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

static void sprdemand_power_hint(struct power_module *module, power_hint_t hint,
                             void *data)
{
    struct sprdemand_power_module *pm = (struct sprdemand_power_module *) module;
    struct timespec now, diff;
    char buf[80];
    int len;

    switch (hint) {
     case POWER_HINT_INTERACTION:
        if (boostpulse_open(pm) >= 0) {
            pthread_mutex_lock(&pm->lock);
            len = write(pm->boostpulse_fd, "4", 1);
            if (len < 0) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error writing to %s: %s\n", BOOSTPULSE_PATH, buf);
            }
            pthread_mutex_unlock(&pm->lock);
        }

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

struct sprdemand_power_module HAL_MODULE_INFO_SYM = {
    base: {
        common: {
            tag: HARDWARE_MODULE_TAG,
            module_api_version: POWER_MODULE_API_VERSION_0_2,
            hal_api_version: HARDWARE_HAL_API_VERSION,
            id: POWER_HARDWARE_MODULE_ID,
            name: "Sprdemand Power HAL",
            author: "The Android Open Source Project",
            methods: &power_module_methods,
        },

        init: power_init,
        setInteractive: power_set_interactive,
        powerHint: sprdemand_power_hint,
    },

    lock: PTHREAD_MUTEX_INITIALIZER,
    boostpulse_fd: -1,
    boostpulse_warned: 0,
};

