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

import com.android.messaging.smilplayer.util.LogTag;
//import com.android.mms.ui.SprdMessageUtils;
import com.google.android.mms.ContentType;
import com.google.android.mms.MmsException;
import com.google.android.mms.pdu.PduBody;
import com.google.android.mms.pdu.PduPart;
//import com.sprd.mms.model.FileModel;

import org.w3c.dom.smil.SMILMediaElement;
import org.w3c.dom.smil.SMILRegionElement;
import org.w3c.dom.smil.SMILRegionMediaElement;
import org.w3c.dom.smil.Time;
import org.w3c.dom.smil.TimeList;

import android.content.Context;
import android.util.Log;

import java.io.IOException;

import com.android.messaging.smilplayer.SmilPlayerConfig;

public class MediaModelFactory {
    private static final String TAG = LogTag.TAG;

    public static MediaModel getMediaModel(Context context,
            SMILMediaElement sme, LayoutModel layouts, PduBody pb)
            throws IOException, IllegalArgumentException, MmsException {
        String tag = sme.getTagName();
        String src = sme.getSrc();
        PduPart part = findPart(pb, src);

        if (sme instanceof SMILRegionMediaElement) {
            return getRegionMediaModel(
                    context, tag, src, (SMILRegionMediaElement) sme, layouts, part);
        } else {
            return getGenericMediaModel(
                    context, tag, src, sme, part, null);
        }
    }

    private static PduPart findPart(PduBody pb, String src) {
        PduPart part = null;

        if (src != null) {
            src = unescapeXML(src);
            if (src.startsWith("cid:")) {
                part = pb.getPartByContentId("<" + src.substring("cid:".length()) + ">");
            } else {
                part = pb.getPartByName(src);
                if (part == null) {
                    part = pb.getPartByFileName(src);
                    if (part == null) {
                        part = pb.getPartByContentLocation(src);
                        if (part == null) {
                            int extensionPos = src.lastIndexOf(".");
                            if (extensionPos > 0) {
                                src = src.substring(0, extensionPos);
                                part = pb.getPartByContentLocation(src);
                            }
                        }
                        if (part == null) {
                            part = pb.getPartByContentId("<" + src + ">");
                        }
                    }
                }
            }
        }

        if (part != null) {
            part.findBySmil = true;//modify by bug 447934
            return part;
        }

        throw new IllegalArgumentException("No part found for the model.");
    }

    private static String unescapeXML(String str) {
        return str.replaceAll("&lt;","<")
            .replaceAll("&gt;", ">")
            .replaceAll("&quot;","\"")
            .replaceAll("&apos;","'")
            .replaceAll("&amp;", "&");
    }

    private static MediaModel getRegionMediaModel(Context context,
            String tag, String src, SMILRegionMediaElement srme,
            LayoutModel layouts, PduPart part) throws IOException, MmsException {
        SMILRegionElement sre = srme.getRegion();
        if (sre != null) {
            RegionModel region = layouts.findRegionById(sre.getId());
            if (region != null) {
                return getGenericMediaModel(context, tag, src, srme, part, region);
            }
        } else {
            String rId = null;

            if (tag.equals(SmilHelper.ELEMENT_TAG_TEXT)) {
                rId = LayoutModel.TEXT_REGION_ID;
            } else {
                rId = LayoutModel.IMAGE_REGION_ID;
            }

            RegionModel region = layouts.findRegionById(rId);
            if (region != null) {
                return getGenericMediaModel(context, tag, src, srme, part, region);
            }
        }

        throw new IllegalArgumentException("Region not found or bad region ID.");
    }

    // When we encounter a content type we can't handle, such as "application/vnd.smaf", instead
    // of throwing an exception and crashing, insert an empty TextModel in its place.
    private static MediaModel createEmptyTextModel(Context context,  RegionModel regionModel)
            throws IOException {
        return new TextModel(context, ContentType.TEXT_PLAIN, null, regionModel);
    }

    private static MediaModel getGenericMediaModel(Context context,
            String tag, String src, SMILMediaElement sme, PduPart part,
            RegionModel regionModel) throws IOException, MmsException {
        byte[] bytes = part.getContentType();
        if (bytes == null) {
            throw new IllegalArgumentException(
                    "Content-Type of the part may not be null.");
        }

        String contentType = new String(bytes);
        /* SPRD:Bug 271830,add for Mms DRM feature develop. @{ */
        boolean isDrmType = contentType.startsWith("application/vnd.oma.drm");
        /* @} */
        MediaModel media = null;
        if (tag.equals(SmilHelper.ELEMENT_TAG_TEXT)) {
            if (contentType.equals(ContentType.TEXT_PLAIN) /*|| contentType
                    .equals(ContentType.TEXT_HTML)*/) {
                media = new TextModel(context, contentType, src,
                        part.getCharset(), part.getData(), regionModel);
            } else if (SmilHelper.containsTag(part, SmilHelper.ELEMENT_TAG_VCALENDAR_FILE)
            /* SPRD:Bug#270871,Sometimes vcalendar's content-type is 'application/oct-stream'. @{*/
            || SmilHelper.isVCalendar(part)) {/*@}*/
                media = new VcalendarModel(context, src, part.getDataUri());
            } else if (SmilHelper.containsTag(part, SmilHelper.EXTENDS_TAG_VCARD)
            /* SPRD:Bug#271098,Sometimes vcard's content-type is 'application/oct-stream'.@{*/      
            || SmilHelper.isVcard(part)) {
                media = new VcardModel(context, part.getDataUri());
            } else if (contentType.equals(ContentType.TEXT_HTML)) {
                media = new FileModel(context, null, contentType, src, part.getDataUri());
            } else {
                throw new IllegalArgumentException("Unsupported TAG: " + tag);
            }
        } else if (tag.equals(SmilHelper.ELEMENT_TAG_IMAGE)) {
            media = new ImageModel(context, contentType, src,
                    part.getDataUri(), regionModel);
        } else if (tag.equals(SmilHelper.ELEMENT_TAG_VIDEO)) {
            media = new VideoModel(context, contentType, src,
                    part.getDataUri(), regionModel);
        } else if (tag.equals(SmilHelper.ELEMENT_TAG_AUDIO)) {
            media = new AudioModel(context, contentType, src,
                    part.getDataUri());
        } else if (tag.equals(SmilHelper.ELEMENT_TAG_VCARD_FILE)) {
            media = new VcardModel(context, part.getDataUri());
        }else if (tag.equals(SmilHelper.ELEMENT_TAG_REF)) {
            //bug 447934 begin
            if (SmilHelper.containsTag(part, SmilHelper.EXTENDS_TAG_VCARD)
                    || SmilHelper.isVcard(part)) {
                media = new VcardModel(context, part.getDataUri());
            }else if(SmilHelper.containsTag(part, SmilHelper.ELEMENT_TAG_VCALENDAR_FILE)
                    || SmilHelper.isVCalendar(part)){
                media = new VcalendarModel(context, src, part.getDataUri());
            }else if (contentType.equals(ContentType.TEXT_HTML)) {
               media = new FileModel(context, null, contentType, src, part.getDataUri());//bug 434098 end
            }else if (ContentType.isTextType(contentType)) {
            //if (ContentType.isTextType(contentType)) {
            //bug 447934 end
                media = new TextModel(context, contentType, src,
                        part.getCharset(), part.getData(), regionModel);
            } else if (ContentType.isImageType(contentType)) {
                media = new ImageModel(context, contentType, src,
                        part.getDataUri(), regionModel);
            } else if (ContentType.isVideoType(contentType)) {
                media = new VideoModel(context, contentType, src,
                        part.getDataUri(), regionModel);
            } else if (ContentType.isAudioType(contentType)) {
                media = new AudioModel(context, contentType, src,
                        part.getDataUri());
            } else {
                Log.d(TAG, "[MediaModelFactory] getGenericMediaModel Unsupported Content-Type: "
                        + contentType);
                media = createEmptyTextModel(context, regionModel);
            }
            Log.i(TAG, "[MediaModelFactory] getGenericMediaModel media"+ media);
        } else {
            throw new IllegalArgumentException("Unsupported TAG: " + tag);
        }

        // Set 'begin' property.
        int begin = 0;
        TimeList tl = sme.getBegin();
        if ((tl != null) && (tl.getLength() > 0)) {
            // We only support a single begin value.
            Time t = tl.item(0);
            begin = (int) (t.getResolvedOffset() * 1000);
        }
        /* SPRD: Add this for NUllPointException of bug 226476 @{ */
        if (null == media) {
            throw new IllegalArgumentException(
                    "error:can not create generic MediaModel.");
        }
        /* @} */
        media.setBegin(begin);

        // Set 'duration' property.
        int duration = (int) (sme.getDur() * 1000);
        if (duration <= 0) {
            tl = sme.getEnd();
            if ((tl != null) && (tl.getLength() > 0)) {
                // We only support a single end value.
                Time t = tl.item(0);
                if (t.getTimeType() != Time.SMIL_TIME_INDEFINITE) {
                    duration = (int) (t.getResolvedOffset() * 1000) - begin;

                    if (duration == 0 &&
                            (media instanceof AudioModel || media instanceof VideoModel)) {
                        duration = SmilPlayerConfig.getMinimumSlideElementDuration();
                        if (Log.isLoggable(LogTag.APP, Log.VERBOSE)) {
                            Log.d(TAG, "[MediaModelFactory] compute new duration for " + tag +
                                    ", duration=" + duration);
                        }
                    }
                }
            }
        }
        /*SPRD:fix fix bug 315715 @{*/
        if(!"audio/dcf".equals(contentType)){
            media.setDuration(duration);
        }
        /*@}*/

        if (!SmilPlayerConfig.getSlideDurationEnabled()) {
            /**
             * Because The slide duration is not supported by mmsc,
             * the device has to set fill type as FILL_FREEZE.
             * If not, the media will disappear while rotating the screen
             * in the slide show play view.
             */
            media.setFill(sme.FILL_FREEZE);
        } else {
            // Set 'fill' property.
            media.setFill(sme.getFill());
        }
        return media;
    }
}
