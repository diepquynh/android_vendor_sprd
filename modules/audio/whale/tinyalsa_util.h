/*
    Copyright (C) 2012 The Android Open Source Project

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

//#define ALOG_NDEBUG 0

#ifndef _TINYALSA_UTIL_H_
#define _TINYALSA_UTIL_H_
#define LOG_TAG "tinyalsa"
#include <utils/Log.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <sys/mman.h>
#include <sys/time.h>
#include <limits.h>

#include <linux/ioctl.h>
#include <sound/asound.h>
#include "stdint.h"
#ifdef __cplusplus
extern "C" {

#define ALSA_DEVICE_DIRECTORY "/dev/snd/"

#define SND_FILE_CONTROL	ALSA_DEVICE_DIRECTORY "controlC%i"
struct snd_ctl_card_info_t {
    int card;			/* card number */
    int pad;			/* reserved for future (was type) */
    unsigned char id[16];		/* ID of card (user selectable) */
    unsigned char driver[16];	/* Driver name */
    unsigned char name[32];		/* Short name of soundcard */
    unsigned char longname[80];	/* name + info text about soundcard */
    unsigned char reserved_[16];	/* reserved for future (was ID of mixer) */
    unsigned char mixername[80];	/* visual mixer identification */
    unsigned char
    components[128];	/* card components / fine identification, delimited with one space (AC97 etc..) */
};

int get_snd_card_number(const char *card_name);
}
#else

#define ALSA_DEVICE_DIRECTORY "/dev/snd/"

#define SND_FILE_CONTROL	ALSA_DEVICE_DIRECTORY "controlC%i"
struct snd_ctl_card_info_t {
    int card;			/* card number */
    int pad;			/* reserved for future (was type) */
    unsigned char id[16];		/* ID of card (user selectable) */
    unsigned char driver[16];	/* Driver name */
    unsigned char name[32];		/* Short name of soundcard */
    unsigned char longname[80];	/* name + info text about soundcard */
    unsigned char reserved_[16];	/* reserved for future (was ID of mixer) */
    unsigned char mixername[80];	/* visual mixer identification */
    unsigned char
    components[128];	/* card components / fine identification, delimited with one space (AC97 etc..) */
};

int get_snd_card_number(const char *card_name);
#endif
#endif//_TINYALSA_UTIL_H_
