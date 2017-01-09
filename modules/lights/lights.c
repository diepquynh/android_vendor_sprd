/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 *
 * you may not use this file except in compliance with the License.
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
#define LOG_NDEBUG 0
*/
#define LOG_TAG "LIGHTS"

#include <cutils/log.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hardware/lights.h>

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
struct led_prop {
    const char *filename;
    int fd;
};
struct led {
    struct led_prop brightness;
    struct led_prop high_time;
    struct led_prop low_time;
    struct led_prop rising_time;
    struct led_prop falling_time;
    struct led_prop on_off;
};

/*all lights*/
enum {
    RED_LED,
    GREEN_LED,
    BLUE_LED,
    LCD_BACKLIGHT,
    BUTTONS_LED,
    NUM_LEDS,
};

/*light nodes*/
struct led leds[NUM_LEDS] = {
    [RED_LED] = {
             .brightness = {"/sys/class/leds/red/brightness", -1},
             .high_time = {"/sys/class/leds/red_bl/high_time", -1},
             .low_time = {"/sys/class/leds/red_bl/low_time", -1},
             .rising_time = {"/sys/class/leds/red_bl/rising_time", -1},
             .falling_time = {
                      "/sys/class/leds/red_bl/falling_time",
                      -1},
             .on_off = {"/sys/class/leds/red_bl/on_off", -1},
             },
    [GREEN_LED] = {
               .brightness = {"/sys/class/leds/green/brightness", -1},
               .high_time = {"/sys/class/leds/green_bl/high_time", -1},
               .low_time = {"/sys/class/leds/green_bl/low_time", -1},
               .rising_time = {
                       "/sys/class/leds/green_bl/rising_time",
                       -1},
               .falling_time = {
                    "/sys/class/leds/green_bl/falling_time",
                    -1},
               .on_off = {"/sys/class/leds/green_bl/on_off", -1},
               },
    [BLUE_LED] = {
              .brightness = {"/sys/class/leds/blue/brightness", -1},
              .high_time = {"/sys/class/leds/blue_bl/high_time", -1},
              .low_time = {"/sys/class/leds/blue_bl/low_time", -1},
              .rising_time = {
                      "/sys/class/leds/blue_bl/rising_time",
                      -1},
              .falling_time = {
                       "/sys/class/leds/blue_bl/falling_time",
                       -1},
              .on_off = {"/sys/class/leds/blue_bl/on_off", -1},
              },
    [LCD_BACKLIGHT] = {
               .brightness = {
                      "/sys/class/backlight/sprd_backlight/brightness",
                      -1},
               },
    [BUTTONS_LED] = {
             .brightness = {
                    "/sys/class/leds/keyboard-backlight/brightness",
                    -1},
             },
};

void init_g_lock(void) {
    pthread_mutex_init(&g_lock, NULL);
}

/*init one node*/
static int init_prop(struct led_prop *prop) {
    int fd;

    prop->fd = -1;
    if (!prop->filename)
        return 0;
    fd = open(prop->filename, O_RDWR);
    if (fd < 0) {
        ALOGE("init_prop: %s cannot be opened (%s)\n", prop->filename,
              strerror(errno));
        return -errno;
    }

    prop->fd = fd;
    return 0;
}

/*close node*/
static void close_prop(struct led_prop *prop) {
    int fd;

    if (prop->fd > 0)
        close(prop->fd);
}

/*init all nodes*/
void init_globals(void) {
    int i;

    for (i = 0; i < NUM_LEDS; ++i) {
        init_prop(&leds[i].brightness);
        init_prop(&leds[i].high_time);
        init_prop(&leds[i].low_time);
        init_prop(&leds[i].rising_time);
        init_prop(&leds[i].falling_time);
        init_prop(&leds[i].on_off);
    }
}

static int write_int(struct led_prop *prop, int value) {
    int fd;
    static int already_warned;

    already_warned = 0;

    ALOGE("file:%s, func:%s, path=%s, value=%d\n", __FILE__, __func__,
          prop->filename, value);
    fd = open(prop->filename, O_RDWR);

    if (fd >= 0) {
        char buffer[20];
        int bytes = snprintf(buffer, sizeof(buffer), "%d\n", value);
        int amt = write(fd, buffer, bytes);

        close(fd);

        return amt == -1 ? -errno : 0;
    }

    if (already_warned == 0) {
        ALOGE("file:%s, func:%s, failed to open %s,fd = %d\n",
              __FILE__, __func__, prop->filename, fd);
        already_warned = 1;
    }
    return -errno;
}

static int rgb_to_brightness(struct light_state_t const *state) {
    int color = state->color & 0x00ffffff;

    return ((77 * ((color >> 16) & 0x00ff))
        + (150 * ((color >> 8) & 0x00ff)) +
        (29 * (color & 0x00ff))) >> 8;
}

static int set_light_backlight(struct light_device_t *dev,
                   struct light_state_t const *state) {
    int err = 0;
    int brightness = rgb_to_brightness(state);
    if (dev)
        ALOGD("file:%s, func:%s, brightness=%d\n", __FILE__, __func__,
              brightness);
    if (NULL == leds[LCD_BACKLIGHT].brightness.filename) {
        ALOGE("file:%s, func:%s, unsupported light!\n", __FILE__,
              __func__);
        return -EINVAL;
    }

    pthread_mutex_lock(&g_lock);
    err = write_int(&leds[LCD_BACKLIGHT].brightness, brightness);
    pthread_mutex_unlock(&g_lock);
    return err;
}

static int is_lit(struct light_state_t const *state) {
    return state->color & 0x00ffffff;
}

static int set_light_keyboard(struct light_device_t *dev,
                  struct light_state_t const *state) {
    int err = 0;
    int on = is_lit(state);

    ALOGD("file:%s, func:%s, on=%d\n", __FILE__, __func__, on);
    if (NULL == BUTTONS_LED) {
        ALOGE("file:%s, func:%s, unsupported light!\n", __FILE__,
              __func__);
        return -EINVAL;
    }

    pthread_mutex_lock(&g_lock);
    err = write_int(&leds[BUTTONS_LED].brightness, on ? 255 : 0);
    pthread_mutex_unlock(&g_lock);
    return err;
}

static int set_light_buttons(struct light_device_t *dev,
                 struct light_state_t const *state) {
    int err = 0;
    int on = is_lit(state);

    ALOGD("file:%s, func:%s, on=%d\n", __FILE__, __func__, on);
    if (NULL == leds[BUTTONS_LED].brightness.filename) {
        ALOGE("file:%s, func:%s, unsupported light!\n", __FILE__,
              __func__);
        return -EINVAL;
    }

    pthread_mutex_lock(&g_lock);
    err = write_int(&leds[BUTTONS_LED].brightness, on ? (on & 0xff) : 0);
    pthread_mutex_unlock(&g_lock);
    return err;
}

static int close_lights(struct light_device_t *dev) {
    int i;

    ALOGV("file:%s, func:%s\n", __FILE__, __func__);

    for (i = 0; i < NUM_LEDS; ++i) {
        close_prop(&leds[i].brightness);
        close_prop(&leds[i].high_time);
        close_prop(&leds[i].low_time);
        close_prop(&leds[i].rising_time);
        close_prop(&leds[i].falling_time);
        close_prop(&leds[i].on_off);
    }
    if (dev)
        free(dev);
    return 0;
}

/*breath led*/
static int set_breath_light(struct light_device_t *dev,
                struct light_state_t const *state) {
    int len;
    int value;
    unsigned int colorRGB;
    unsigned int colorR;
    unsigned int colorG;
    unsigned int colorB;
    unsigned int ms_unit = 125, ontime_regv = 0, offtime_regv =
        0, rise_regv = 0, high_regv = 0, fall_regv = 0, low_regv = 0;

    colorRGB = state->color & 0xFFFFFF;
    /* white */
    if (colorRGB == 0xFFFFFF)
        colorRGB = 0x0;

    colorR = (colorRGB >> 16) & 0x00ff;
    colorG = (colorRGB >> 8) & 0x00ff;
    colorB = colorRGB & 0x00ff;

    pthread_mutex_lock(&g_lock);

    switch (state->flashMode) {
    case LIGHT_FLASH_HARDWARE:
    case LIGHT_FLASH_TIMED:
        ontime_regv = state->flashOnMS / ms_unit;
        offtime_regv = state->flashOffMS / ms_unit;

        rise_regv = ontime_regv * 2 / 3;
        high_regv = ontime_regv / 3;
        fall_regv = offtime_regv / 5;
        low_regv = offtime_regv * 4 / 5;
        break;

    case LIGHT_FLASH_NONE:
    default:
        ontime_regv = 0;
        offtime_regv = 0;
        break;

        ALOGD("set_led_state colorRGB=%08X, unknown mode %d\n",
              colorRGB, state->flashMode);
    }
    if (ontime_regv > 0 && offtime_regv > 0) {
        if (colorR) {
            write_int(&leds[RED_LED].rising_time, rise_regv);
            write_int(&leds[RED_LED].high_time, high_regv);
            write_int(&leds[RED_LED].falling_time, fall_regv);
            write_int(&leds[RED_LED].low_time, low_regv);
            write_int(&leds[RED_LED].on_off, 1);
        } else {    /*off */
            write_int(&leds[RED_LED].on_off, 0);
        }
        if (colorG) {
            write_int(&leds[GREEN_LED].rising_time, rise_regv);
            write_int(&leds[GREEN_LED].high_time, high_regv);
            write_int(&leds[GREEN_LED].falling_time, fall_regv);
            write_int(&leds[GREEN_LED].low_time, low_regv);
            write_int(&leds[GREEN_LED].on_off, 1);
        } else {    /*off */
            write_int(&leds[GREEN_LED].on_off, 0);
        }
        if (colorB) {
            write_int(&leds[BLUE_LED].rising_time, rise_regv);
            write_int(&leds[BLUE_LED].high_time, high_regv);
            write_int(&leds[BLUE_LED].falling_time, fall_regv);
            write_int(&leds[BLUE_LED].low_time, low_regv);
            write_int(&leds[BLUE_LED].on_off, 1);
        } else {    /*off */
            write_int(&leds[BLUE_LED].on_off, 0);
        }
    } else {
        if (colorR) {
            write_int(&leds[RED_LED].brightness, colorR);
        } else {    /*off */
            write_int(&leds[RED_LED].brightness, 0);
            write_int(&leds[RED_LED].on_off, 0);
        }
        if (colorG) {
            write_int(&leds[GREEN_LED].brightness, colorG);
        } else {    /*off */
            write_int(&leds[GREEN_LED].brightness, 0);
            write_int(&leds[GREEN_LED].on_off, 0);
        }
        if (colorB) {
            write_int(&leds[BLUE_LED].brightness, colorB);
        } else {    /*off */
            write_int(&leds[BLUE_LED].brightness, 0);
            write_int(&leds[BLUE_LED].on_off, 0);
        }
    }

    pthread_mutex_unlock(&g_lock);
    return 0;
}

/* LEDs */
static int set_light_leds_notifications(struct light_device_t *dev,
                    struct light_state_t const *state) {
    ALOGE("file:%s, func:%s, unsupported light!\n", __FILE__, __func__);
    set_breath_light(dev, state);
    return -EINVAL;
}

static int set_light_leds_attention(struct light_device_t *dev,
                    struct light_state_t const *state) {
    ALOGE("file:%s, func:%s, unsupported light!\n", __FILE__, __func__);
    return -EINVAL;
}

static int open_lights(const struct hw_module_t *module, char const *name,
               struct hw_device_t **device) {
    int (*set_light)(struct light_device_t *dev,
              struct light_state_t const *state);

    ALOGV("file:%s, func:%s name=%s\n", __FILE__, __func__, name);

    if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
        set_light = set_light_backlight;
    else if (0 == strcmp(LIGHT_ID_KEYBOARD, name))
        set_light = set_light_keyboard;
    else if (0 == strcmp(LIGHT_ID_BUTTONS, name))
        set_light = set_light_buttons;
    else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name))
        set_light = set_light_leds_notifications;
    else if (0 == strcmp(LIGHT_ID_ATTENTION, name))
        set_light = set_light_leds_attention;
    else if (0 == strcmp(LIGHT_ID_BATTERY, name))
        set_light = set_light_leds_notifications;
    else
        return -EINVAL;

    pthread_once(&g_init, init_g_lock);

    /*pthread_once(&g_init, init_globals); */
    struct light_device_t *dev = malloc(sizeof(struct light_device_t));

    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t *)module;
    dev->common.close = (int (*)(struct hw_device_t *))close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t *)dev;

    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open = open_lights,
};

/*
 * The lights Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "lights Module",
    .author = "Google, Inc.",
    .methods = &lights_module_methods,
};
