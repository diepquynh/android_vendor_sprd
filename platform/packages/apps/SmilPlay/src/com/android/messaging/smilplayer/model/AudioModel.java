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

import java.util.HashMap;
import java.util.Map;
import org.w3c.dom.events.Event;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
//import android.database.sqlite.SqliteWrapper;
import android.net.Uri;
import android.provider.MediaStore.Audio;
import android.provider.Telephony.Mms.Part;
import android.text.TextUtils;
import android.util.Log;
import android.webkit.MimeTypeMap;
import com.android.messaging.smilplayer.util.LogTag;
//import com.android.mms.ui.SprdMessageUtils;

import com.android.messaging.smilplayer.exception.ContentRestrictionException;
import com.android.messaging.smilplayer.dom.events.EventImpl;
import com.android.messaging.smilplayer.dom.smil.SmilMediaElementImpl;
import com.google.android.mms.MmsException;
/*SPRD:Bug 375488 Add for drm @*/
import com.android.messaging.smilplayer.drm.DrmModelUtils;
/*@}*/

public class AudioModel extends MediaModel {
    private static final String TAG = MediaModel.TAG;
    private static final boolean DEBUG = false;
    private static final boolean LOCAL_LOGV = false;

    private final HashMap<String, String> mExtras;

    public AudioModel(Context context, Uri uri, boolean checkForWarning) throws MmsException {
        this(context, null, null, uri);
        initModelFromUri(uri);
        checkContentRestriction(checkForWarning);
    }

    public AudioModel(Context context, String contentType, String src, Uri uri) throws MmsException {
        super(context, SmilHelper.ELEMENT_TAG_AUDIO, contentType, src, uri);
        mExtras = new HashMap<String, String>();
    }

    private void initModelFromUri(Uri uri) throws MmsException {
        if(uri != null){
            String scheme = uri.getScheme();
            if ("content".equals(uri.getScheme())) {
                initFromContentUri(uri);
            } else if ("file".equals(scheme)) {
                initFromFile(uri);
            }
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
        /* SPRD: Bug#284516 orig: initMediaDuration(); @{*/
        if (!"audio/dcf".equals(mContentType)) {
            initMediaDuration();
        }
        /* @} */
    }

    public static boolean isDownloadsUri(Uri uri) {
        return uri.getAuthority().startsWith("com.android.providers.downloads.documents");
    }

    private void initFromContentUri(Uri uri) throws MmsException {
        ContentResolver cr = mContext.getContentResolver();
        Cursor c = cr.query(uri, null, null, null, null);
        if (c != null) {
            try {
                if (c.moveToFirst()) {
                    String path = null;
                    boolean isFromMms = isMmsUri(uri);
                    // SPRD:Bug#276889
                    boolean isFromDownloads = isDownloadsUri(uri);

                    // FIXME We suppose that there should be only two sources
                    // of the audio, one is the media store, the other is
                    // our MMS database.
                    if (isFromMms) {
                        path = c.getString(c.getColumnIndexOrThrow(Part._DATA));
                        mContentType = c.getString(c.getColumnIndexOrThrow(Part.CONTENT_TYPE));
                    } 
                    /* SPRD:Bug#276889 @{ */
                    else if(isFromDownloads){
                        mSrc = c.getString(c.getColumnIndexOrThrow("_display_name"));
                        mContentType = c.getString(c.getColumnIndexOrThrow("mime_type"));
                        /* SPRD:Bug 375488 Add for drm @{ */
                        DrmModelUtils.getInstance(mContext).setAudioTypeAndSrc(
                                this, mSrc);
                        /* @} */
                    } 
                    /* @} */
                    else {
                        path = c.getString(c.getColumnIndexOrThrow(Audio.Media.DATA));
                        try{
                            mContentType = c.getString(c.getColumnIndexOrThrow(
                                    Audio.Media.MIME_TYPE));
                        }catch(IllegalArgumentException e){
                            mContentType = c.getString(c.getColumnIndexOrThrow("mimetype"));
                        }
                        
                        try {
                            // Get more extras information which would be useful
                            // to the user.
                            String album = c.getString(c
                                    .getColumnIndexOrThrow("album"));
                            if (!TextUtils.isEmpty(album)) {
                                mExtras.put("album", album);
                            }
                        } catch (IllegalArgumentException e) {
                            Log.e(TAG, "no album");
                        }

                        try {
                            String artist = c.getString(c
                                    .getColumnIndexOrThrow("artist"));
                            if (!TextUtils.isEmpty(artist)) {
                                mExtras.put("artist", artist);
                            }
                        } catch (IllegalArgumentException e) {
                            Log.e(TAG, "no artist");
                        }
                    }
                    if(path != null){
                        mSrc = path.substring(path.lastIndexOf('/') + 1);
                    }
                    /* SPRD:Bug 375488 Add for drm @{ */
                    DrmModelUtils.getInstance(mContext).setAudioTypeBySrc(this,
                            mSrc);
                    /* @} */
                    if (TextUtils.isEmpty(mContentType)) {
                        throw new MmsException("Type of media is unknown.");
                    }

                    if (LOCAL_LOGV) {
                        Log.v(TAG, "New AudioModel created:"
                                + " mSrc=" + mSrc
                                + " mContentType=" + mContentType
                                + " mUri=" + uri
                                + " mExtras=" + mExtras);
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

    public void stop() {
        appendAction(MediaAction.STOP);
        notifyModelChanged(false);
    }

    public void handleEvent(Event evt) {
        String evtType = evt.getType();
        if (LOCAL_LOGV) {
            Log.v(TAG, "Handling event: " + evtType + " on " + this);
        }

        MediaAction action = MediaAction.NO_ACTIVE_ACTION;
        if (evtType.equals(SmilMediaElementImpl.SMIL_MEDIA_START_EVENT)) {
            action = MediaAction.START;
            // if the Music player app is playing audio, we should pause that so it won't
            // interfere with us playing audio here.
            pauseMusicPlayer();
        } else if (evtType.equals(SmilMediaElementImpl.SMIL_MEDIA_END_EVENT)) {
            action = MediaAction.STOP;
        } else if (evtType.equals(SmilMediaElementImpl.SMIL_MEDIA_PAUSE_EVENT)) {
            action = MediaAction.PAUSE;
        } else if (evtType.equals(SmilMediaElementImpl.SMIL_MEDIA_SEEK_EVENT)) {
            action = MediaAction.SEEK;
            mSeekTo = ((EventImpl) evt).getSeekTo();
        }

        appendAction(action);
        notifyModelChanged(false);
    }

    public Map<String, ?> getExtras() {
        return mExtras;
    }

    protected void checkContentRestriction(boolean checktForWarning) throws ContentRestrictionException {
        ContentRestriction cr = ContentRestrictionFactory.getContentRestriction();
        cr.checkAudioContentType(mContentType, mContext, checktForWarning);
    }

    @Override
    protected boolean isPlayable() {
        return true;
    }
    
    /* SPRD:Bug#276884 @{ */
    public AudioModel(Context context, Uri uri, boolean checkForWarning, int type) throws MmsException {
        this(context, null, null, uri);
        mMediatype = type;
        initModelFromUri(uri);
        checkContentRestriction(checkForWarning);
    }
    private int mMediatype;
    public static final int AUDIO = 3;
    /* @} */
    private void initFromFile(Uri uri) {
        String path = uri.getPath();
        mSrc = path.substring(path.lastIndexOf('/') + 1);
        MimeTypeMap mimeTypeMap = MimeTypeMap.getSingleton();
        String extension = MimeTypeMap.getFileExtensionFromUrl(mSrc);
        if (TextUtils.isEmpty(extension)) {
            // getMimeTypeFromExtension() doesn't handle spaces in filenames nor
            // can it handle
            // urlEncoded strings. Let's try one last time at finding the
            // extension.
            int dotPos = mSrc.lastIndexOf('.');
            if (0 <= dotPos) {
                extension = mSrc.substring(dotPos + 1);
            }
        }
        mContentType = mimeTypeMap.getMimeTypeFromExtension(extension.toLowerCase());
        /* SPRD:Bug#276884 @{ */
        if ((mMediatype == AUDIO) && (extension.toLowerCase().equals("3gpp"))) {
            mContentType = "audio/3gpp";
        }
        /* @} */
        /* SPRD:Bug 375488 Add for drm @{ */
        DrmModelUtils.getInstance(mContext).setAudioTypeByExt(this, extension);
        /* @} */
        // It's ok if mContentType is null. Eventually we'll show a toast
        // telling the
        // user the audio couldn't be attached.

        if (Log.isLoggable(LogTag.APP, Log.VERBOSE)) {
            Log.v(TAG, "New AudioModel initFromFile created:"
                    + " mSrc=" + mSrc
                    + " mContentType=" + mContentType
                    + " mUri=" + uri);
        }
    }
}
