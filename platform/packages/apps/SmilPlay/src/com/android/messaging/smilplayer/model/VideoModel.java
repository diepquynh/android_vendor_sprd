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
import org.w3c.dom.smil.ElementTime;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
//import android.database.sqlite.SqliteWrapper;
import android.net.Uri;
import android.provider.MediaStore.Images;
import android.provider.Telephony.Mms.Part;
import android.text.TextUtils;
import android.util.Log;
import android.webkit.MimeTypeMap;
import android.provider.Telephony.Mms.Part;

import com.google.android.mms.ContentType;
import com.google.android.mms.MmsException;

import com.android.messaging.smilplayer.exception.ContentRestrictionException;
import com.android.messaging.smilplayer.util.LogTag;
import com.android.messaging.smilplayer.dom.events.EventImpl;
import com.android.messaging.smilplayer.dom.smil.SmilMediaElementImpl;
import com.android.messaging.smilplayer.util.ItemLoadedCallback;
import com.android.messaging.smilplayer.util.ItemLoadedFuture;
import com.android.messaging.smilplayer.util.ThumbnailManager;
//import com.android.mms.ui.SprdMessageUtils;

/*SPRD:Bug 375488 Add for drm @{*/
import com.android.messaging.smilplayer.drm.DrmModelUtils;
import com.android.messaging.smilplayer.drm.DrmUtilsSprd;
/*@}*/
import com.android.messaging.smilplayer.util.SmilPlayerApp;

public class VideoModel extends RegionMediaModel {
    private static final String TAG = MediaModel.TAG;
    private static final boolean DEBUG = true;
    private static final boolean LOCAL_LOGV = false;
    private ItemLoadedFuture mItemLoadedFuture;

    public VideoModel(Context context, Uri uri, RegionModel region)
            throws MmsException {
        this(context, null, null, uri, region);
        initModelFromUri(uri);
        checkContentRestriction();
    }

    public VideoModel(Context context, String contentType, String src,
            Uri uri, RegionModel region) throws MmsException {
        super(context, SmilHelper.ELEMENT_TAG_VIDEO, contentType, src, uri, region);
    }

    public static boolean isEmailUri(Uri uri) {
        if ( uri == null ) {
            return false;
        }
        String auth = uri.getAuthority();
        if ( auth == null ) {
            return false;
        }
        return auth.startsWith("com.android.email");
    }

    private void initModelFromUri(Uri uri) throws MmsException {
        String scheme = uri.getScheme();
        if ("content".equals(scheme)) {
            initFromContentUri(uri);
        } else if ("file".equals(scheme)) {
            initFromFile(uri);
        }
        /*
         * SPRD:Bug#283192,Replace space symbol with underscores in the attached
         * file name.Because some mmsc have problems with filenames contains
         * space symbol.
         * 
         * @{
         */
        if (mSrc != null) {
            mSrc = mSrc.replaceAll("[\\p{Space}]+", "_");
        }
        /*
         * @}
         */
        /*SPRD:Bug 375488 Add for drm
         *orig: initMediaDuration();
         * @{
         */
        if (!DrmUtilsSprd.getInstance(mContext).isDrmSrc(mSrc)) {
            initMediaDuration();
        }
        /* @} */
    }

    private void initFromFile(Uri uri) {
        // SPRD:
        mSrc = uri.getPath().substring(uri.getPath().lastIndexOf("/") + 1);
        if (mSrc != null) {
            if (mSrc.startsWith(".")) {
                mSrc = mSrc.substring(1);
            }
            int index = mSrc.lastIndexOf(".");
            if (index != -1 && mSrc.endsWith("temp.3gp")) {
                try {
                    StringBuilder sb = new StringBuilder();
                    sb.append(mSrc.substring(0,index));
                    sb.append(System.currentTimeMillis());
                    sb.append(".3gp");
                    mSrc = sb.toString();
                } catch (IndexOutOfBoundsException ex) {
                    if (LOCAL_LOGV) {
                        Log.v(TAG, "Media extension is unknown.");
                    }
                }
            }
        }

        MimeTypeMap mimeTypeMap = MimeTypeMap.getSingleton();
        String extension = MimeTypeMap.getFileExtensionFromUrl(mSrc);
        if (TextUtils.isEmpty(extension)) {
            // getMimeTypeFromExtension() doesn't handle spaces in filenames nor can it handle
            // urlEncoded strings. Let's try one last time at finding the extension.
            int dotPos = -1;
            if(mSrc != null){
                dotPos = mSrc.lastIndexOf('.');
            }
            if (0 <= dotPos) {
                extension = mSrc.substring(dotPos + 1);
            }
        }
        mContentType = mimeTypeMap.getMimeTypeFromExtension(extension);
        /* SPRD: Bug 375488 Add for drm @{ */
        DrmModelUtils.getInstance(mContext).setVideoTypeByExt(this, extension);
        /* @} */
        // It's ok if mContentType is null. Eventually we'll show a toast telling the
        // user the video couldn't be attached.

        if (Log.isLoggable(LogTag.APP, Log.VERBOSE)) {
            Log.v(TAG, "New VideoModel initFromFile created:"
                    + " mSrc=" + mSrc
                    + " mContentType=" + mContentType
                    + " mUri=" + uri);
        }
    }

    private void initFromContentUri(Uri uri) throws MmsException {
        ContentResolver cr = mContext.getContentResolver();
        Cursor c = cr.query(uri, null, null, null, null);

        if (c != null) {
            try {
                if (c.moveToFirst()) {
                    String path;
                    if (VideoModel.isMmsUri(uri)) {
                        path = c.getString(c.getColumnIndexOrThrow(Part.FILENAME));
                        if (TextUtils.isEmpty(path)) {
                            path = c.getString(
                                    c.getColumnIndexOrThrow(Part._DATA));
                        }
                        mContentType = c.getString(
                                c.getColumnIndexOrThrow(Part.CONTENT_TYPE));
                        mSrc = path.substring(path.lastIndexOf('/') + 1);
                    } else {
                        try {
                            // Local videos will have a data column
                            path = c.getString(c.getColumnIndexOrThrow(Images.Media.DATA));
                        } catch (IllegalArgumentException e) {
                            // For non-local videos, the path is the uri
                            path = uri.toString();
                        }
                        mSrc = path.substring(path.lastIndexOf('/') + 1);
                        /*SPRD:Bug 375488 Add for drm @{*/
                        DrmModelUtils.getInstance(mContext).setVideoSrc(this,c);
                        /*@}*/
                        try {
                        	if(/*SprdMessageUtils.*/isEmailUri(uri)){
                        		mContentType = cr.getType(uri);
                        	}else{
                        		mContentType = c.getString(c.getColumnIndexOrThrow(
                                    Images.Media.MIME_TYPE));}
                        } catch (Exception e) {
                            throw new MmsException("MIME_TYPE Column is missing");
                        }
                        if (TextUtils.isEmpty(mContentType)) {
                            throw new MmsException("Type of media is unknown.");
                        }
                        if (mContentType.equals(ContentType.VIDEO_MP4)
                                && !(TextUtils.isEmpty(mSrc))) {
                            int index = mSrc.lastIndexOf(".");
                            if (index != -1) {
                                try {
                                    String extension = mSrc.substring(index + 1);
                                    if (!(TextUtils.isEmpty(extension)) &&
                                            (extension.equalsIgnoreCase("3gp") ||
                                                    extension.equalsIgnoreCase("3gpp") ||
                                            extension.equalsIgnoreCase("3g2"))) {
                                        mContentType = ContentType.VIDEO_3GPP;
                                    }
                                } catch (IndexOutOfBoundsException ex) {
                                    if (LOCAL_LOGV) {
                                        Log.v(TAG, "Media extension is unknown.");
                                    }
                                }
                            }
                        }
                        /* SPRD:Bug 375488 Add for drm */
                        DrmModelUtils.getInstance(mContext).setVideoTypeBySrc(
                                this, mSrc);
                        /* @} */

                        if (Log.isLoggable(LogTag.APP, Log.VERBOSE)) {
                            Log.v(TAG, "New VideoModel initFromContentUri created:"
                                    + " mSrc=" + mSrc
                                    + " mContentType=" + mContentType
                                    + " mUri=" + uri);
                        }
                    }
                } else {
                    throw new MmsException("Nothing found: " + uri);
                }
            } finally {
                c.close();
            }
        } else {
            throw new MmsException("Bad URI: " + uri);
        }
    }

    // EventListener Interface
    public void handleEvent(Event evt) {
        String evtType = evt.getType();
        if (LOCAL_LOGV || Log.isLoggable(LogTag.APP, Log.VERBOSE)) {
            Log.v(TAG, "[VideoModel] handleEvent " + evt.getType() + " on " + this);
        }

        MediaAction action = MediaAction.NO_ACTIVE_ACTION;
        if (evtType.equals(SmilMediaElementImpl.SMIL_MEDIA_START_EVENT)) {
            action = MediaAction.START;

            // if the Music player app is playing audio, we should pause that so it won't
            // interfere with us playing video here.
            pauseMusicPlayer();

            mVisible = true;
        } else if (evtType.equals(SmilMediaElementImpl.SMIL_MEDIA_END_EVENT)) {
            action = MediaAction.STOP;
            if (mFill != ElementTime.FILL_FREEZE) {
                mVisible = false;
            }
        } else if (evtType.equals(SmilMediaElementImpl.SMIL_MEDIA_PAUSE_EVENT)) {
            action = MediaAction.PAUSE;
            mVisible = true;
        } else if (evtType.equals(SmilMediaElementImpl.SMIL_MEDIA_SEEK_EVENT)) {
            action = MediaAction.SEEK;
            mSeekTo = ((EventImpl) evt).getSeekTo();
            mVisible = true;
        }

        appendAction(action);
        notifyModelChanged(false);
    }

    protected void checkContentRestriction() throws ContentRestrictionException {
        ContentRestriction cr = ContentRestrictionFactory.getContentRestriction();
        cr.checkVideoContentType(mContentType, mContext);
    }

    @Override
    protected boolean isPlayable() {
        return true;
    }

    public ItemLoadedFuture loadThumbnailBitmap(ItemLoadedCallback callback) {
        ThumbnailManager thumbnailManager = SmilPlayerApp.getThumbnailManager();
        mItemLoadedFuture = thumbnailManager.getVideoThumbnail(getUri(), callback);
        return mItemLoadedFuture;
    }

    public void cancelThumbnailLoading() {
        if (mItemLoadedFuture != null && !mItemLoadedFuture.isDone()) {
            if (Log.isLoggable(LogTag.APP, Log.DEBUG)) {
                Log.v(TAG, "cancelThumbnailLoading for: " + this);
            }
            mItemLoadedFuture.cancel(getUri());
            mItemLoadedFuture = null;
        }
    }
}
