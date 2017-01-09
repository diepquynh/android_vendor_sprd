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

import org.w3c.dom.events.Event;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
//import android.database.sqlite.SqliteWrapper;
import android.net.Uri;
import android.provider.Telephony.Mms.Part;
import android.util.Log;

import com.android.messaging.smilplayer.exception.UnsupportContentTypeException;
//import com.android.mms.ui.MessageUtils;

import com.google.android.mms.ContentType;
import com.google.android.mms.MmsException;

public class VcalendarModel extends MediaModel {

    public VcalendarModel(Context context, Uri uri) throws MmsException {
        super(context, SmilHelper.ELEMENT_TAG_VCALENDAR, ContentType.TEXT_VCALENDAR, null, uri);
        initModelFromUri(uri);
    }

    public VcalendarModel(Context context, String src, Uri uri) throws MmsException {
        super(context, SmilHelper.ELEMENT_TAG_VCALENDAR, ContentType.TEXT_VCALENDAR, src, uri);
    }

    private boolean isOsDebug(){
        return true;
    }

    private void initModelFromUri(Uri uri) throws UnsupportContentTypeException {
        try {
            String scheme = uri.getScheme();
            if ("content".equals(scheme)) {
                initFromContentUri(uri);
            } else if ("file".equals(scheme)) {
                String path = uri.getPath();
                mSrc = path.substring(path.lastIndexOf('/') + 1);
            }
        }catch(UnsupportContentTypeException ue){
            throw new UnsupportContentTypeException();
        }
        catch (Exception e) {
            Log.e("vcalendar", "Bad URI: " + uri);
        }

        if (mSrc == null || mSrc.equals("")) {
            mSrc = System.currentTimeMillis() + ".vcs";
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
        if (isOsDebug()) {
        Log.d("vcalendar", "mSrc = " + mSrc);
        }
    }

    private void initFromContentUri(Uri uri) throws UnsupportContentTypeException{
        ContentResolver cr = mContext.getContentResolver();
        Cursor c = cr.query(uri, null, null, null, null);
        if (c != null) {
            try {
                if (c.moveToFirst()) {
                    String path;
                    if (isMmsUri(uri)) {
                        path = c.getString(c.getColumnIndexOrThrow(Part._DATA));
                        mSrc = path.substring(path.lastIndexOf('/') + 1);
                    } else {
                        mSrc = c.getString(c.getColumnIndexOrThrow("_display_name"));
                        if(mSrc!=null && !mSrc.endsWith(".vcs")){
                            throw new UnsupportContentTypeException();
                        }
                    }
                }
            }catch(UnsupportContentTypeException e){
               throw new UnsupportContentTypeException();
            }finally {
                c.close();
            }
        }
    }

    @Override
    public void handleEvent(Event evt) {
    }
    public String getVcalendardName(){
        return mSrc;
    }

}
