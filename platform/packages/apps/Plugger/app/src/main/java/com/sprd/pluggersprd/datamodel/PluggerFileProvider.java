/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.sprd.pluggersprd.datamodel;

import android.content.Context;
import android.net.Uri;
import android.util.Log;

import java.io.File;

/**
 * A very simple content provider that can serve mms files from our cache directory.
 */
public class PluggerFileProvider extends FileProvider {
    private static final String TAG = "PluggerFileProvider";


    static final String AUTHORITY = "com.sprd.pluggersprd.datamodel.PluggerFileProvider";
    private static final String RAW_MMS_DIR = "rawmms";
    static Context mContext;

    /**
     * Returns a uri that can be used to access a raw mms file.
     *
     * @return the URI for an raw mms file
     */
    public static Uri buildRawMmsUri(final Context context) {
        final Uri uri = buildFileUri(AUTHORITY, null);
        final File file = getFile(context,uri.getPath());
        if (!ensureFileExists(file)) {
            Log.e(TAG, "Failed to create temp file " + file.getAbsolutePath());
        }
        Log.d(TAG, "buildRawMmsUri  file path =" + file.getAbsolutePath());
        mContext=context;
        return uri;
    }

    @Override
    File getFile(final String path, final String extension) {
        return getFile(mContext,path);
    }




    public static File getFile(final Context context ,final Uri uri) {
        return getFile(context,uri.getPath());
    }

    private static File getFile(final Context context ,final String path) {
        //final Context context = Factory.get().getApplicationContext();
        return new File(getDirectory(context), path + ".dat");
    }

    private static File getDirectory(final Context context) {
        return new File(context.getCacheDir(), RAW_MMS_DIR);
    }
}
