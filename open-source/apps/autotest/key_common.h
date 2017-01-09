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

#ifndef KEY_COMMON_H
#define KEY_COMMON_H

#include <sys/time.h>
#include <time.h>

enum {
    RL_NA = 0,
    RL_FAIL = 1,
    RL_PASS = 2,
    RL_NS = 3,
};

// Initialize the key.
void key_init();

int key_pass_or_fail(int key);
int pass_or_fail(void);
int ui_wait_key(struct timespec *ntime); // +++++++          // waits for a key/button press, returns the code
int ui_wait_key_sec(int second);
int ui_wait_key_simp(void);
int ui_key_pressed(int key);  // returns >0 if the code is currently pressed
void ui_clear_key_queue();
int ui_read_key(void);
#endif  // KEY_COMMON_H
