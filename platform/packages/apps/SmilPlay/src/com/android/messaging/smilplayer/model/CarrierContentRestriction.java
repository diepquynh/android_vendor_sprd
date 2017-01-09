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

import java.util.ArrayList;

import android.content.ContentResolver;
import android.content.Context;
import android.util.Log;

import com.google.android.mms.ContentType;
import com.android.messaging.smilplayer.exception.WarningModeResolutionException;
import com.android.messaging.smilplayer.exception.WarningModeUnsupportTypeException;
import com.android.messaging.smilplayer.exception.ContentRestrictionException;
import com.android.messaging.smilplayer.exception.ExceedMessageSizeException;
import com.android.messaging.smilplayer.util.LogTag;

import com.android.messaging.smilplayer.exception.ResolutionException;
import com.android.messaging.smilplayer.exception.UnsupportContentTypeException;

//import com.android.mms.ui.MessagingPreferenceActivity;
import com.android.messaging.smilplayer.SmilPlayerConfig;

public class CarrierContentRestriction implements ContentRestriction {
    private static final ArrayList<String> sSupportedImageTypes;
    private static final ArrayList<String> sSupportedAudioTypes;
    private static final ArrayList<String> sSupportedVideoTypes;
    private static final ArrayList<String> sGCFTypes;
    private static final boolean DEBUG = true;

    static {
        sSupportedImageTypes = ContentType.getImageTypes();
        sSupportedAudioTypes = ContentType.getAudioTypes();
        sSupportedVideoTypes = ContentType.getVideoTypes();
        sGCFTypes = new ArrayList<String>();
        sGCFTypes.add(ContentType.IMAGE_PNG);
        sGCFTypes.add(ContentType.AUDIO_MP3);
        sGCFTypes.add(ContentType.AUDIO_MPEG3);
        sGCFTypes.add(ContentType.AUDIO_MPEG);
        sGCFTypes.add(ContentType.AUDIO_MPEG);
        sGCFTypes.add(ContentType.AUDIO_X_MP3);
        sGCFTypes.add(ContentType.AUDIO_X_MPEG3);
        sGCFTypes.add(ContentType.AUDIO_X_MPEG);
        sGCFTypes.add(ContentType.AUDIO_X_MPG);
        sGCFTypes.add(ContentType.AUDIO_IMELODY);
        sGCFTypes.add(ContentType.AUDIO_X_WAV);
    }

    private boolean MessagingPreferenceActivityIsWaringMode(Context context){
         return true;
    }
    private boolean MessagingPreferenceActivityIsRestrictedMode(Context context){
         return false;
    }

    public CarrierContentRestriction() {
    }

    public void checkMessageSize(int messageSize, int increaseSize, ContentResolver resolver)
            throws ContentRestrictionException {
        if (DEBUG) {
            Log.d(LogTag.APP, "CarrierContentRestriction.checkMessageSize messageSize: " +
                        messageSize + " increaseSize: " + increaseSize +
                        " SmilPlayerConfig.getMaxMessageSize: " + SmilPlayerConfig.getMaxMessageSize());
        }
        if ( (messageSize < 0) || (increaseSize < 0) ) {
            throw new ContentRestrictionException("Negative message size"
                    + " or increase size");
        }
        int newSize = messageSize + increaseSize;
        // Fixed for bug206239 begin
        //if ( (newSize < 0) || (newSize > SmilPlayerConfig.getMaxMessageSize() - SlideshowModel.SLIDESHOW_SLOP) ) {
        if ( (newSize < 0) || (newSize > SmilPlayerConfig.getMaxMessageSize()) ){
        // Fixed for bug206239 end
            throw new ExceedMessageSizeException("Exceed message size limitation");
        }
    }

    public void checkResolution(int width, int height, Context context) throws ContentRestrictionException {
        checkResolution(width, height, context, false);
    }
    public void checkResolution(int width, int height, Context context, boolean checkForWarning) throws ContentRestrictionException {
        if ( (width > SmilPlayerConfig.getMaxImageWidth() || height > SmilPlayerConfig.getMaxImageHeight())
                && (height > SmilPlayerConfig.getMaxImageWidth() || width > SmilPlayerConfig.getMaxImageHeight()) ) {
            if ( MessagingPreferenceActivityIsWaringMode(context) ) {
                if ( checkForWarning) {
                    throw new WarningModeResolutionException("content resolution exceeds restriction.");
                }
            } else {
                throw new ResolutionException("content resolution exceeds restriction.");
            }
        }
    }

    public void checkImageContentType(String contentType, Context context) throws ContentRestrictionException {
        checkImageContentType(contentType, context, false);
    }
    public void checkImageContentType(String contentType, Context context, boolean checkForWarning)
            throws ContentRestrictionException {
        if (null == contentType) {
            throw new ContentRestrictionException("Null content type to be check");
        }
        checkCGFContentType(contentType, context, checkForWarning);
        if (!sSupportedImageTypes.contains(contentType)) {
            throw new UnsupportContentTypeException("Unsupported image content type : "
                    + contentType);
        }
    }

    public void checkAudioContentType(String contentType, Context context) throws ContentRestrictionException {
        checkAudioContentType(contentType, context, false);
    }
    public void checkAudioContentType(String contentType, Context context, boolean checkForWarning)
            throws ContentRestrictionException {
        if (null == contentType) {
            throw new ContentRestrictionException("Null content type to be check");
        }
        checkCGFContentType(contentType, context, checkForWarning);
        if (!sSupportedAudioTypes.contains(contentType)) {
            throw new UnsupportContentTypeException("Unsupported audio content type : "
                    + contentType);
        }
    }

    public void checkVideoContentType(String contentType, Context context)
            throws ContentRestrictionException {
        if (null == contentType) {
            throw new ContentRestrictionException("Null content type to be check");
        }
        checkCGFContentType(contentType, context, false);
        if (!sSupportedVideoTypes.contains(contentType)) {
            throw new UnsupportContentTypeException("Unsupported video content type : "
                    + contentType);
        }
    }

    // SPRD: 
    public void checkCGFContentType(String contentType, Context context) throws ContentRestrictionException {
        checkCGFContentType(contentType, context, false);
    }
    public void checkCGFContentType(String contentType, Context context, boolean checkForWarning)
            throws ContentRestrictionException {
        if (null == contentType) {
            throw new ContentRestrictionException("Null content type to be check");
        }
        if (sGCFTypes.contains(contentType)) {
            if ( MessagingPreferenceActivityIsRestrictedMode(context) ) {
                throw new UnsupportContentTypeException("Unsupported CGF content type : "
                        + contentType);
            } else if ( checkForWarning && MessagingPreferenceActivityIsWaringMode(context) ) {
                throw new WarningModeUnsupportTypeException("Unsupported CGF content type : "
                        + contentType);
            }
        }
    }
}
