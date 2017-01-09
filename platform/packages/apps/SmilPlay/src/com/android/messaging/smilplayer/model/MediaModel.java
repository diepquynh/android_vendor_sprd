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

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.nio.channels.FileChannel;

import org.w3c.dom.events.EventListener;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.media.MediaMetadataRetriever;
import android.net.Uri;
import android.util.Log;

import com.google.android.mms.MmsException;
import com.google.android.mms.ContentType;

import com.android.messaging.smilplayer.util.LogTag;

import com.android.messaging.smilplayer.SmilPlayerConfig;


// TODO: remove dependency for SDK build

public abstract class MediaModel extends Model implements EventListener {
    protected static final String TAG = LogTag.TAG;

    protected Context mContext;
    protected int mBegin;
    protected int mDuration;
    protected String mTag;
    public String mSrc;
    public String mContentType;
    private Uri mUri;
    private byte[] mData;
    protected short mFill;
    protected int mSize;
    protected int mSeekTo;
    protected boolean mMediaResizeable;

    private final ArrayList<MediaAction> mMediaActions;
    public static enum MediaAction {
        NO_ACTIVE_ACTION,
        START,
        STOP,
        PAUSE,
        SEEK,
    }

    public MediaModel(Context context, String tag, String contentType,
            String src, Uri uri) throws MmsException {
        mContext = context;
        mTag = tag;
        mContentType = contentType;
        mSrc = src;
        mUri = uri;
        initMediaSize();
        mMediaActions = new ArrayList<MediaAction>();
    }

    public MediaModel(Context context, String tag, String contentType,
            String src, byte[] data) {
        if (data == null) {
            throw new IllegalArgumentException("data may not be null.");
        }

        mContext = context;
        mTag = tag;
        mContentType = contentType;
        mSrc = src;
        mData = data;
        mSize = data.length;
        mMediaActions = new ArrayList<MediaAction>();
    }

    public int getBegin() {
        return mBegin;
    }

    public void setBegin(int begin) {
        mBegin = begin;
        notifyModelChanged(true);
    }

    public int getDuration() {
        return mDuration;
    }

    public void setDuration(int duration) {
        if (isPlayable() && (duration < 0)) {
            // 'indefinite' duration, we should try to find its exact value;
            try {
                initMediaDuration();
            } catch (MmsException e) {
                // On error, keep default duration.
                Log.e(TAG, e.getMessage(), e);
                return;
            }
        } else {
            mDuration = duration;
        }
        notifyModelChanged(true);
    }

    public String getTag() {
        return mTag;
    }

    public String getContentType() {
        return mContentType;
    }

    /**
     * Get the URI of the media.
     *
     * @return The URI of the media.
     */
    public Uri getUri() {
        return mUri;
    }

    public byte[] getData() {
        if (mData != null) {
            byte[] data = new byte[mData.length];
            System.arraycopy(mData, 0, data, 0, mData.length);
            return data;
        }
        return null;
    }

    /**
     * @param uri the mUri to set
     */
    void setUri(Uri uri) {
        mUri = uri;
    }

    /**
     * @return the mSrc
     */
    public String getSrc() {
        return mSrc;
    }

    // SPRD: 
    public void setSrc(String src) {
        this.mSrc = src;
    }

    /**
     * @return the mFill
     */
    public short getFill() {
        return mFill;
    }

    /**
     * @param fill the mFill to set
     */
    public void setFill(short fill) {
        mFill = fill;
        notifyModelChanged(true);
    }

    /**
     * @return whether the media is resizable or not. For instance, a picture can be resized
     * to smaller dimensions or lower resolution. Other media, such as video and sounds, aren't
     * currently able to be resized.
     */
    public boolean getMediaResizable() {
        return mMediaResizeable;
    }

    /**
     * @return the size of the attached media
     */
    public int getMediaSize() {
        return mSize;
    }

    public boolean isText() {
        return mTag.equals(SmilHelper.ELEMENT_TAG_TEXT)
                && mContentType != null
                && (mContentType.equals(ContentType.TEXT_PLAIN));
    }

    /* SPRD: Support html type attachment for bug 261203 @{ */
    public boolean isHtml() {
        return mTag.equals(SmilHelper.ELEMENT_TAG_FILE)
                && mContentType != null
                && mContentType.equals(ContentType.TEXT_HTML);
    }
    /* @} */

    // SPRD:  porting:vcard/vcalendar @{
    public boolean isVcard() {
        return mTag.equals(SmilHelper.ELEMENT_TAG_VCARD) && mContentType != null
                && mContentType.equals(ContentType.TEXT_VCARD);
    }

    public boolean isVcalendar() {
        return mTag.equals(SmilHelper.ELEMENT_TAG_VCALENDAR) && mContentType != null
                && mContentType.equals(ContentType.TEXT_VCALENDAR);
    }
    // @}

    public boolean isImage() {
        return mTag.equals(SmilHelper.ELEMENT_TAG_IMAGE);
    }

    public boolean isVideo() {
        return mTag.equals(SmilHelper.ELEMENT_TAG_VIDEO);
    }

    public boolean isAudio() {
        return mTag.equals(SmilHelper.ELEMENT_TAG_AUDIO);
    }

    protected void initMediaDuration() throws MmsException {
        if (mUri == null) {
            throw new IllegalArgumentException("Uri may not be null.");
        }

        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        int duration = 0;
        try {
            retriever.setDataSource(mContext, mUri);
            String dur = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION);
            if (dur != null) {
                duration = Integer.parseInt(dur);
            }
            /* SPRD: Modify for bug 303158
             * orig: mDuration = duration; @{ */
            if (duration < (duration / 1000) *1000 + 100) {
                mDuration = (int)Math.floor(duration / 1000f) *1000;
                return;
            }
            mDuration = (int)Math.ceil(duration / 1000f) *1000;
             /* @} */
        } catch (Exception ex) {
            Log.e(TAG, "MediaMetadataRetriever failed to get duration for " + mUri.getPath(), ex);
            throw new MmsException(ex);
        } finally {
            retriever.release();
        }
    }

    private void initMediaSize() throws MmsException {
        ContentResolver cr = mContext.getContentResolver();
        InputStream input = null;
        try {
            input = cr.openInputStream(mUri);
			//SPRD: Add for bug#231450
            if (input == null) {
                throw new MmsException();
            }
            if (input instanceof FileInputStream) {
                // avoid reading the whole stream to get its length
                FileInputStream f = (FileInputStream) input;
                FileChannel channel = f.getChannel();
                if(channel != null){
                    mSize = (int) channel.size();
                }
                if(mSize == 0){
                    if(input != null){
                        while (-1 != input.read()) {
                            mSize++;
                        }
                    }
                }
                // sometimes mSize will be zero here. It's tempting to count the
                // bytes as the code
                // does below, but that turns out to be very slow. We'll deal
                // with a zero size
                // when we resize the media.
            } else {
                while (-1 != input.read()) {
                    mSize++;
                }
            }

        } catch (IOException e) {
            // Ignore
            Log.e(TAG, "IOException caught while opening or reading stream", e);
            if (e instanceof FileNotFoundException) {
                throw new MmsException(e.getMessage());
            }
        } catch (NullPointerException e) {
            Log.e(TAG, "IOException caught while opening or reading stream", e);
        }catch (SecurityException se) {//add bug443444
            Log.e(TAG, "SecurityException caught while opening stream", se);
        } finally {
            if (null != input) {
                try {
                    input.close();
                } catch (IOException e) {
                    // Ignore
                    Log.e(TAG, "IOException caught while closing stream", e);
                }
            }
        }
    }

    public static boolean isMmsUri(Uri uri) {
        return uri.getAuthority().startsWith("mms");
    }

    public int getSeekTo() {
        return mSeekTo;
    }

    public void appendAction(MediaAction action) {
        mMediaActions.add(action);
    }

    public MediaAction getCurrentAction() {
        if (0 == mMediaActions.size()) {
            return MediaAction.NO_ACTIVE_ACTION;
        }
        return mMediaActions.remove(0);
    }

    protected boolean isPlayable() {
        return false;
    }

    protected void pauseMusicPlayer() {
        if (Log.isLoggable(LogTag.APP, Log.VERBOSE)) {
            Log.d(TAG, "pauseMusicPlayer");
        }

        AudioManager am = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
        am.requestAudioFocus(null, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
    }

    /**
     * If the attached media is resizeable, resize it to fit within the byteLimit. Save the
     * new part in the pdu.
     * @param byteLimit the max size of the media attachment
     * @throws MmsException
     */
    protected void resizeMedia(int byteLimit, long messageId) throws MmsException {
    }

    /* SPRD:Bug 271830,add for Mms DRM feature develop. @{ */
    private boolean isDrmType() {
        return mContentType.startsWith("application/vnd.oma.drm");
    }
    /* @} */
}
