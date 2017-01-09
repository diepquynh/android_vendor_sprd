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

package com.android.messaging.smilplayer.model;

import android.database.Cursor;
//import android.database.sqlite.SqliteWrapper;
import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;
import android.provider.Telephony.Mms.Part;
import android.util.Log;

import com.google.android.mms.ContentType;
import com.google.android.mms.MmsException;

import org.w3c.dom.events.Event;


public class VcardModel extends MediaModel {
    public VcardModel(Context context, Uri uri) throws MmsException {
        super(context, SmilHelper.ELEMENT_TAG_VCARD, ContentType.TEXT_VCARD, null, uri);
        initModelFromUri(uri);
    }

    public VcardModel(Context context, String src, Uri uri) throws MmsException {
        super(context, SmilHelper.ELEMENT_TAG_VCARD, ContentType.TEXT_VCARD, src, uri);
    }

    private void initModelFromUri(Uri uri) {
        try {
            String scheme = uri.getScheme();
            if ("content".equals(scheme)) {
                initFromContentUri(uri);
            } else if ("file".equals(scheme)) {
                String path = uri.getPath();
                mSrc = path.substring(path.lastIndexOf('/') + 1);
            }
        } catch (Exception e) {
            Log.d("vcard", "Bad URI: " + uri);
        }

        if (mSrc == null || mSrc.equals("")) {
            mSrc = System.currentTimeMillis() + ".vcf";
        }
        /*
         * SPRD:Bug#283192,Replace space symbol with underscores in the attached
         * file name.Because some mmsc have problems with filenames contains
         * space symbol.
         *
         * @{
         */
        mSrc = mSrc.replaceAll("[\\p{Space}]+", "_");
        /*
         * @}
         */
        Log.d("vcard", "mSrc = " + mSrc);
    }

    private void initFromContentUri(Uri uri) {
        ContentResolver cr = mContext.getContentResolver();
        Cursor c = cr.query(uri, null, null, null, null);
        if (c != null) {
            try {
                if (c.moveToFirst()) {
                    if (isMmsUri(uri)) {
                        // SPRD: use Part.CONTENT_LOCATION instead of PART_XXXXXXXXXXXX to display vcard name.
                        mSrc = c.getString(c.getColumnIndexOrThrow(Part.CONTENT_LOCATION));
                    } else {
                        mSrc = c.getString(c.getColumnIndexOrThrow("_display_name"));
                    }
                }
            } finally {
                c.close();
            }
        }
    }

    // EventListener Interface
    public void handleEvent(Event evt) {
    }
    public String getVcardName(){
        return mSrc;
    }

}
