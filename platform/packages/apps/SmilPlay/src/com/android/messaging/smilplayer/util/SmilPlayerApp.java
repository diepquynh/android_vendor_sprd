/*
 * Copyright (C) 2008 Esmertec AG.
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

package com.android.messaging.smilplayer.util;

import java.util.List;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.drm.DrmManagerClient;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.os.Handler;


import com.android.messaging.smilplayer.layout.LayoutManager;
import com.android.messaging.smilplayer.util.ThumbnailManager;
import com.android.messaging.smilplayer.util.LogTag;

public class SmilPlayerApp/* extends Application */{
    public static final String LOG_TAG = LogTag.TAG;
	
    private static SmilPlayerApp sSmilPlayerApp = null;
    private static ThumbnailManager mThumbnailManager;
    private static DrmManagerClient mDrmManagerClient;
    private static Context sContext;
    private static boolean mInited;


    private SmilPlayerApp(Context context) {
	 sContext = context;
	 mInited = true;
        mThumbnailManager = new ThumbnailManager(context);
	 mDrmManagerClient = new DrmManagerClient(context);
    }

    synchronized public static SmilPlayerApp init(Context context) {
        if (sSmilPlayerApp != null){
             sSmilPlayerApp = new SmilPlayerApp(context);
        }
        return sSmilPlayerApp;
    }

    public static ThumbnailManager getThumbnailManager() {
	 if (!mInited){
             return null;
	 }
        return mThumbnailManager;
    }

    public static DrmManagerClient getDrmManagerClient() {
	 if (!mInited){
             return null;
	 }
        return mDrmManagerClient;
    }
}
