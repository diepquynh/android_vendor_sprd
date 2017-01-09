/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License,  Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,  software
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,  either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include "common.h"
#include "cutils/properties.h"
#include "minui/minui.h"
#include "recovery_ui.h"
#include "battery.h"

#define ENG_FACOTRYMODE_FILE    "/productinfo/factorymode.file"
#define ENG_FACOTRYSYNC_FILE    "/factorysync.file"


extern pthread_mutex_t gBatteryMutex;
void *charge_thread(void *cookie);
void *power_thread(void *cookie);
void *input_thread(void *write_fd);
void backlight_init(void);
int is_exit = 0;

#define MODE_CHECK_MAX 10

int
main(int argc,  char **argv) {
    time_t start = time(NULL);
    int ret,  fd = 0;
    char charge_prop[PROPERTY_VALUE_MAX] = {0};
    int is_factory = 0;
    char factory_prop[10] = {0};
    struct stat s;
    int i = 0;

    LOGD("\n charge start\n");
    ret = battery_status_init();
    if (ret < 0)
        return -1;

    LOGD("\n charge detecting\n");
    property_get("ro.bootmode",  charge_prop,  "");
    LOGD("charge_prop: %s\n",  charge_prop);
    if (!charge_prop[0] || strncmp(charge_prop,  "charge",  6)) {
        LOGE("exit 1\n");
        return EXIT_SUCCESS;
    }

    system("echo 0 > /sys/class/android_usb/android0/enable");

    ui_init();

    LOGD("ui_init\n");

    ui_set_background(BACKGROUND_ICON_NONE);
    ui_show_indeterminate_progress();
    backlight_init();
    pthread_t t_1,  t_2,  t_3;
    ret = pthread_create(&t_1,  NULL,  charge_thread,  NULL);
    if (ret) {
        LOGE("thread:charge_thread creat failed\n");
        return -1;
    }
    ret = pthread_create(&t_2,  NULL,  input_thread,  NULL);
    if (ret) {
        LOGE("thread:input_thread creat failed\n");
        return -1;
    }

    ret = pthread_create(&t_3,  NULL,  power_thread,  NULL);
    if (ret) {
        LOGE("thread: power_thread creat failed\n");
        return -1;
    }

    LOGD("all thread start\n");

    pthread_join(t_1,  NULL);
    pthread_join(t_2,  NULL);
    pthread_join(t_3,  NULL);

    LOGD("charge app exit\n");

    return EXIT_SUCCESS;
}
