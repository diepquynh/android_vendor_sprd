/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef RECOVERY_COMMON_H
#define RECOVERY_COMMON_H

#include <stdio.h>
#include <utils/Log.h>

// Initialize the graphics system.
void ui_init();

// Use KEY_* codes from <linux/input.h> or KEY_DREAM_* from "minui/minui.h".
//int ui_wait_key(struct timespec *ntime); // +++++++          // waits for a key/button press, returns the code
//int ui_key_pressed(int key);  // returns >0 if the code is currently pressed
int ui_text_visible();        // returns >0 if text log is currently visible
//void ui_clear_key_queue();

// Write a message to the on-screen log shown with Alt-L (also to stderr).
// The screen is small, and users may need to report these messages to support,
// so keep the output short and not too cryptic.
void ui_print(const char *fmt, ...);

enum {
  BACKGROUND_ICON_NONE,
  BACKGROUND_ICON_INSTALLING,
  BACKGROUND_ICON_ERROR,
  NUM_BACKGROUND_ICONS
};
void ui_set_background(int icon);


//#undef LOG_TAG
//#define LOG_TAG  "FACTORY"
#if 0
#define LOGE(...) fprintf(stderr, "E:" __VA_ARGS__)
#define LOGD(...) fprintf(stderr, "D:" __VA_ARGS__)
#define LOGI(...) fprintf(stderr, "I:" __VA_ARGS__)
#else
#define LOGD(format, ...)  ALOGD(format "\n", ## __VA_ARGS__)
#define LOGE(format, ...)  ALOGE(format "\n", ## __VA_ARGS__)
#define LOGI(format, ...)  ALOGI(format "\n", ## __VA_ARGS__)

#endif


#define STRINGIFY(x) #x
#define EXPAND(x) STRINGIFY(x)


#endif  // RECOVERY_COMMON_H
