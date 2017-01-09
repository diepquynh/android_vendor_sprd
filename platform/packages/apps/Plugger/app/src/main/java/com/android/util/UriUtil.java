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
package com.android.util;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.MediaMetadataRetriever;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.MediaStore;

import android.text.TextUtils;


import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import java.net.URLConnection;
import java.util.Arrays;
import java.util.HashSet;

public class UriUtil {
    private static final String SCHEME_SMS = "sms";
    private static final String SCHEME_SMSTO = "smsto";
    private static final String SCHEME_MMS = "mms";
    private static final String SCHEME_MMSTO = "smsto";
    public static final HashSet<String> SMS_MMS_SCHEMES = new HashSet<String>(
        Arrays.asList(SCHEME_SMS, SCHEME_MMS, SCHEME_SMSTO, SCHEME_MMSTO));

    public static final String SCHEME_BUGLE = "bugle";
    public static final HashSet<String> SUPPORTED_SCHEME = new HashSet<String>(
        Arrays.asList(ContentResolver.SCHEME_ANDROID_RESOURCE,
            ContentResolver.SCHEME_CONTENT,
            ContentResolver.SCHEME_FILE,
            SCHEME_BUGLE));

    public static final String SCHEME_TEL = "tel:";

    private static final String MEDIA_STORE_URI_KLP = "com.android.providers.media.documents";

    /**
     * Check if a URI is from the MediaStore
     */
    public static boolean isMediaStoreUri(final Uri uri) {
        final String uriAuthority = uri.getAuthority();
        return TextUtils.equals(ContentResolver.SCHEME_CONTENT, uri.getScheme())
                && (TextUtils.equals(MediaStore.AUTHORITY, uriAuthority) ||
                // KK changed the media store authority name
                TextUtils.equals(MEDIA_STORE_URI_KLP, uriAuthority));
    }

}
