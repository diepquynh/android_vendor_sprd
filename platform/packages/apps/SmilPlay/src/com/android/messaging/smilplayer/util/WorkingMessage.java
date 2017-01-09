/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.android.messaging.smilplayer.util;


public class WorkingMessage {
    private static final String TAG = "WorkingMessage";
    private static final boolean DEBUG = true;

    public static final String ACTION_SENDING_SMS = "android.intent.action.SENDING_SMS";

    public static final String EXTRA_SMS_MESSAGE = "android.mms.extra.MESSAGE";
    public static final String EXTRA_SMS_RECIPIENTS = "android.mms.extra.RECIPIENTS";
    public static final String EXTRA_SMS_THREAD_ID = "android.mms.extra.THREAD_ID";


    // States that can require us to save or send a message as MMS.
    private static final int RECIPIENTS_REQUIRE_MMS = (1 << 0); // 1
    private static final int HAS_SUBJECT = (1 << 1); // 2
    private static final int HAS_ATTACHMENT = (1 << 2); // 4
    private static final int LENGTH_REQUIRES_MMS = (1 << 3); // 8
    private static final int FORCE_MMS = (1 << 4); // 16
    private static final int MULTIPLE_RECIPIENTS = (1 << 5); // 32


    // Errors from setAttachment()
    public static final int OK = 0;
    public static final int UNKNOWN_ERROR = -1;
    public static final int MESSAGE_SIZE_EXCEEDED = -2;
    public static final int UNSUPPORTED_TYPE = -3;
    public static final int IMAGE_TOO_LARGE = -4;
    public static final int WARNING_MODE_UNSUPPORTED_TYPE = -5;
    public static final int WARNING_MODE_IMAGE_TOO_LARGE = -6;
    public static final int LIMIT_NUMBERS_OF_SLIDES = -7;
    public static final int URI_ISNULL = -8;
    // Attachment types
    public static final int TEXT = 0;
    public static final int IMAGE = 1;
    public static final int VIDEO = 2;
    public static final int AUDIO = 3;
    public static final int SLIDESHOW = 4;
    public static final int VCARD = 5;
    public static final int VCALENDAR = 6;
    public static final int OTHER_FILE = 7;
    public static final int EXTRAMEDIAS = 8;//add for Bug 447934

    public static final String SAVE_MSG_URI_KEY = "pref_msg_uri_key";
}
