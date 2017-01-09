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


import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.HashMap;

import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.w3c.dom.events.EventTarget;
import org.w3c.dom.smil.SMILDocument;
import org.w3c.dom.smil.SMILElement;
import org.w3c.dom.smil.SMILLayoutElement;
import org.w3c.dom.smil.SMILMediaElement;
import org.w3c.dom.smil.SMILParElement;
import org.w3c.dom.smil.SMILRegionElement;
import org.w3c.dom.smil.SMILRootLayoutElement;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.SharedPreferences;
import android.net.Uri;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.util.Log;

import com.android.messaging.smilplayer.R;
import com.android.messaging.smilplayer.dom.NodeListImpl;
import com.android.messaging.smilplayer.dom.smil.parser.SmilXmlSerializer;
import com.android.messaging.smilplayer.layout.LayoutManager;

import com.google.android.mms.ContentType;
import com.google.android.mms.MmsException;
import com.google.android.mms.pdu.GenericPdu;
import com.google.android.mms.pdu.PduBody;
import com.google.android.mms.pdu.PduHeaders;
import com.google.android.mms.pdu.PduPart;
//import com.google.android.mms.pdu.PduPersisterSprd;
import com.google.android.mms.pdu.PduPersister;
import com.google.android.mms.pdu.MultimediaMessagePdu;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;

import com.android.messaging.smilplayer.exception.UnsupportContentTypeException;
import com.android.messaging.smilplayer.exception.ContentRestrictionException;
import com.android.messaging.smilplayer.exception.ExceedMessageSizeException;
import com.android.messaging.smilplayer.model.FileModel;

//import com.android.mms.LogTag;
import com.android.messaging.smilplayer.SmilPlayerConfig;
//import com.android.mms.ui.MessageUtils;
//import com.android.mms.ui.MessagingPreferenceActivity;
//import com.android.mms.ui.SprdMessageUtils;

public class SlideshowModel extends Model
        implements List<SlideModel>, IModelChangedObserver {
    private static final String TAG = "SlideshowModel";

    private ArrayList<CharSequence> mMediaNameList = new ArrayList<CharSequence>();
    private ArrayList<MediaModel> mMediaList = new ArrayList<MediaModel>();
    private final LayoutModel mLayout;
    private final ArrayList<SlideModel> mSlides;
    public ArrayList<FileModel> mFiles;
    private SMILDocument mDocumentCache;
    private PduBody mPduBodyCache;
    private int mCurrentMessageSize;    // This is the current message size, not including
                                        // attachments that can be resized (such as photos)
    private int mTotalMessageSize;      // This is the computed total message size
    private Context mContext;
    private int lengthSignature;

    // amount of space to leave in a slideshow for text and overhead.
    public static final int SLIDESHOW_SLOP = 1024;

    private SlideshowModel(Context context) {
        mLayout = new LayoutModel();
        mSlides = new ArrayList<SlideModel>();
        mContext = context;
        setSingatureLength();
    }

    private static boolean isOSDebug(){
         return true;
    }

    private final static String MessagingPreferenceActivitySIGNATURE_ENABLE =  "pref_key_signature_enable";
    private final static String MessagingPreferenceActivitySIGNATURE_INPUT =  "pref_key_signature_input";
    private boolean MessagingPreferenceActivityIsRestrictedMode(Context context){
       return true;
    }

    private SlideshowModel (
            LayoutModel layouts, ArrayList<SlideModel> slides,
            SMILDocument documentCache, PduBody pbCache,
            Context context) {
        mLayout = layouts;
        mSlides = slides;
        mContext = context;

        mDocumentCache = documentCache;
        mPduBodyCache = pbCache;
        for (SlideModel slide : mSlides) {
            increaseMessageSize(slide.getSlideSize());
            slide.setParent(this);
        }

        setSingatureLength();
    }

    public static SlideshowModel createNew(Context context) {
        return new SlideshowModel(context);
    }

    public static SlideshowModel createFromMessageUri(
            Context context, Uri uri) throws MmsException {
        return createFromPduBody(context, getPduBody(context, uri));
    }

    public static SlideshowModel createFromPduBody(Context context, PduBody pb) throws MmsException {
        SMILDocument document = SmilHelper.getDocument(pb);
        // fix bug 598897 start
        int parCount = document.getBody().getChildNodes().getLength();
        for (int i = 0; i < parCount;) {
            int ParChildCount = document.getBody().getChildNodes().item(i)
                    .getChildNodes().getLength();
            Log.i(TAG, "===========createFromPduBody===1==ParChildCount: "
                    + ParChildCount);
            for (int j = 0; j < ParChildCount;) {
                String deepChildName = document.getBody().getChildNodes()
                        .item(i).getChildNodes().item(j).getNodeName();
                Log.i(TAG, "==========createFromPduBody======2====deepChildName: "
                        + deepChildName);
                if ("ref".equalsIgnoreCase(deepChildName) || "/ref".equalsIgnoreCase(deepChildName)) {
                    document.getBody()
                            .getChildNodes()
                            .item(i)
                            .removeChild(
                                    document.getBody().getChildNodes().item(i)
                                            .getChildNodes().item(j));
                    ParChildCount--; // =
                                     // document.getBody().getChildNodes().item(i).getChildNodes().getLength();
                    Log.i(TAG, "========createFromPduBody===3====ParChildCount:  "
                            + ParChildCount);
                } else {
                    j++;
                }
            }
            if (document.getBody().getChildNodes().item(i).getChildNodes()
                    .getLength() == 0) {
                document.getBody().removeChild(
                        document.getBody().getChildNodes().item(i));
                parCount--; // = document.getBody().getChildNodes().getLength();
                Log.i(TAG, "==========createFromPduBody====4===parCount: " + parCount);
            } else {
                i++;
            }
        }
        // fix bug 598897 end

        // Create root-layout model.
        SMILLayoutElement sle = document.getLayout();
        SMILRootLayoutElement srle = sle.getRootLayout();
        int w = srle.getWidth();
        int h = srle.getHeight();
        if ((w == 0) || (h == 0)) {
            w = LayoutManager.getInstance().getLayoutParameters().getWidth();
            h = LayoutManager.getInstance().getLayoutParameters().getHeight();
            srle.setWidth(w);
            srle.setHeight(h);
        }
        RegionModel rootLayout = new RegionModel(
                null, 0, 0, w, h);

        // Create region models.
        ArrayList<RegionModel> regions = new ArrayList<RegionModel>();
        NodeList nlRegions = sle.getRegions();
        int regionsNum = nlRegions.getLength();

        for (int i = 0; i < regionsNum; i++) {
            SMILRegionElement sre = (SMILRegionElement) nlRegions.item(i);
            RegionModel r = new RegionModel(sre.getId(), sre.getFit(),
                    sre.getLeft(), sre.getTop(), sre.getWidth(), sre.getHeight(),
                    sre.getBackgroundColor());
            regions.add(r);
        }
        LayoutModel layouts = new LayoutModel(rootLayout, regions);

        // SPRD: Add for support other file.
        ArrayList<FileModel> fileModels = new ArrayList<FileModel>();

        // Create slide models.
        SMILElement docBody = document.getBody();
        NodeList slideNodes = docBody.getChildNodes();
        int slidesNum = slideNodes.getLength();
        ArrayList<SlideModel> slides = new ArrayList<SlideModel>(slidesNum);
        //Fix for Bug 447934 begin
        List<MediaModel> extraMedias = new ArrayList<MediaModel>();
        //Fix for Bug 447934 end
        int totalMessageSize = 0;

        for (int i = 0; i < slidesNum; i++) {
            // FIXME: This is NOT compatible with the SMILDocument which is
            // generated by some other mobile phones.
            SMILParElement par = (SMILParElement) slideNodes.item(i);
            /* SPRD: Add for slideshow duration @{ */
            int parDuration = (int) (par.getDur() * 1000);
            if (parDuration == 0) {
                parDuration = SmilPlayerConfig.getMinimumSlideElementDuration();
                par.setDur(parDuration);
                parDuration = parDuration * 1000;
            }
            /* @} */

            // Create media models for each slide.
            NodeList mediaNodes = par.getChildNodes();
            int mediaNum = mediaNodes.getLength();
            ArrayList<MediaModel> mediaSet = new ArrayList<MediaModel>(mediaNum);

            for (int j = 0; j < mediaNum; j++) {
                SMILMediaElement sme = (SMILMediaElement) mediaNodes.item(j);
                try {
                    MediaModel media = MediaModelFactory.getMediaModel(
                            context, sme, layouts, pb);
                    if (isOSDebug()) {
                        Log.d(TAG, "mediaNum["+i+"] media.mContentType == "+media.mContentType +
                                " media.mTag == "+media.mTag+" media.mSrc:"+media.mSrc);
                    }

                    /*
                    * This is for slide duration value set.
                    * If mms server does not support slide duration.
                    */
                    if (!SmilPlayerConfig.getSlideDurationEnabled()) {
                        int mediadur = media.getDuration();
                        float dur = par.getDur();
                        if (dur == 0) {
                            mediadur = SmilPlayerConfig.getMinimumSlideElementDuration() * 1000;
                            media.setDuration(mediadur);
                        }

                        if ((int)mediadur / 1000 != dur) {
                            String tag = sme.getTagName();

                            if (ContentType.isVideoType(media.mContentType)
                              || tag.equals(SmilHelper.ELEMENT_TAG_VIDEO)
                              || ContentType.isAudioType(media.mContentType)
                              || tag.equals(SmilHelper.ELEMENT_TAG_AUDIO)) {
                                /*
                                * add 1 sec to release and close audio/video
                                * for guaranteeing the audio/video playing.
                                * because the mmsc does not support the slide duration.
                                */
                                if ((int)dur != 0) {
                                    media.setDuration((int)dur * 1000);
                                }
                            } else {
                                /*
                                * If a slide has an image and an audio/video element
                                * and the audio/video element has longer duration than the image,
                                * The Image disappear before the slide play done. so have to match
                                * an image duration to the slide duration.
                                */
                                if ((int)mediadur / 1000 < dur) {
                                    media.setDuration((int)dur * 1000);
                                } else {
                                    if ((int)dur != 0) {
                                        media.setDuration((int)dur * 1000);
                                    } else {
                                        par.setDur((float)mediadur / 1000);
                                    }
                                }
                            }
                        }
                    }
                    //Fix for Bug 447934 begin
                    if(ExtraMediaContainer.isExtraMedia(media)){
                        extraMedias.add(media);
                    }else{
                    SmilHelper.addMediaElementEventListeners(
                            (EventTarget) sme, media);
                    mediaSet.add(media);
                    totalMessageSize += media.getMediaSize();
                    }
                    //Fix for Bug 447934 end
                } catch (IOException e) {
                    Log.e(TAG, e.getMessage(), e);
                } catch (IllegalArgumentException e) {
                    Log.e(TAG, e.getMessage(), e);
                } catch (UnsupportContentTypeException e) {
                    Log.e(TAG, e.getMessage(), e);
                }
            }

            SlideModel slide = new SlideModel((int) (par.getDur() * 1000), mediaSet);
            slide.setFill(par.getFill());
            SmilHelper.addParElementEventListeners((EventTarget) par, slide);
            slides.add(slide);
        }

        /* SPRD: Add for support other file. @{*/
        //Fix for Bug 447934 begin
        synchronized (pb){
            for (PduPart part : pb.getParts()) {
                String contentType = new String(part.getContentType());
                if (!part.findBySmil && !ContentType.APP_SMIL.equalsIgnoreCase(contentType)) {
                    MediaModel media = ExtraMediaContainer.createMediaModel(part, context);
                    if(media == null){
                        continue;
                    }
                    if(ExtraMediaContainer.isExtraMedia(media)){
                        extraMedias.add(media);
                    }
					Log.i(TAG,"slidshowModel sync pb extraMedias add media:"+media+" contentType:"+contentType);
                }
            }
        }
		//Fix for Bug 447934 end
        /* @} */

        SlideshowModel slideshow = new SlideshowModel(layouts, slides, document, pb, context);
        //Fix for Bug 447934 begin
        if (!extraMedias.isEmpty()) {
            slideshow.getExtraMediaContainer().setExtraMedias(extraMedias);
        }
        //Fix for Bug 447934 end
        /* @} */

        slideshow.mTotalMessageSize = totalMessageSize;
        slideshow.registerModelChangedObserver(slideshow);
        return slideshow;
    }

    public PduBody toPduBody() {
        if (mPduBodyCache == null) {
            // SPRD: Rename media name if it is duplicated before create Smil Document.
            renameMediaNameIfNeeded();
            mDocumentCache = SmilHelper.getDocument(this);
            mPduBodyCache = makePduBody(mDocumentCache);
        }
        return mPduBodyCache;
    }

    private PduBody makePduBody(SMILDocument document) {
        PduBody pb = new PduBody();

        //boolean hasVcardOrVcalendar = false; add for bug 447934
        synchronized (mSlides) {
            for (SlideModel slide : mSlides) {
                for (MediaModel media : slide) {
                    PduPart part = new PduPart();

                    if (media.isText()) {
                        TextModel text = (TextModel) media;
                        // Don't create empty text part.
                        if (TextUtils.isEmpty(text.getText())) {
                            continue;
                        }
                        // Set Charset if it's a text media.
                        part.setCharset(text.getCharset());
                    }

                    // Set Content-Type.
                    part.setContentType(media.getContentType().getBytes());

                    String src = media.getSrc();
                    String location;
                    /*SPRD: Add this for bug 218358 @{*/
                    if (src != null) {
                        boolean startWithContentId = src.startsWith("cid:");
                        if (startWithContentId) {
                            location = src.substring("cid:".length());
                        } else {
                            location = src;
                        }

                        // Set Content-Location.
                        part.setContentLocation(location.getBytes());
                        // Set Content-Id.
                        if (startWithContentId) {
                            // Keep the original Content-Id.
                            part.setContentId(location.getBytes());
                        } else {
                            int index = location.lastIndexOf(".");
                            String contentId = (index == -1) ? location : location.substring(0,
                                    index);
                            if (contentId != null && !contentId.isEmpty()) {
                                part.setContentId(contentId.getBytes());
                            }
                        }
                    }
                    /* @} */
                    /* add for bug 447934
                    * Bug#270871,Sometimes vcalendar's content-type is 'application/oct-stream'.
                    * Bug#271098,Sometimes vcard's content-type is 'application/oct-stream'
                    * @{
                    if (media.isVcard() || media.isVcalendar()|| SmilHelper.isVcard(part) || SmilHelper.isVCalendar(part)) {
                        hasVcardOrVcalendar = true;
                    }
                    */
                    if (media.isText()) {
                        part.setData(((TextModel) media).getText().getBytes());
                    //Fix for Bug 447934 begin
                    } else if (media.isImage() || media.isVideo() || media.isAudio()){
                    //Fix for Bug 447934 end

                        part.setDataUri(media.getUri());
                    } else {
                        Log.w(TAG, "Unsupport media: " + media);
                    }

                    pb.addPart(part);
                }
            }
        }
        //Fix for Bug 447934 begin
        getExtraMediaContainer().addExtraPduParts(pb);
        //Fix for Bug 447934 end
        // Create and insert SMIL part(as the first part) into the PduBody.
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        SmilXmlSerializer.serialize(document, out);
        PduPart smilPart = new PduPart();
        smilPart.setContentId("smil".getBytes());
        smilPart.setContentLocation("smil.xml".getBytes());
        smilPart.setContentType(ContentType.APP_SMIL.getBytes());
        smilPart.setData(out.toByteArray());
        //Fix for Bug 447934 begin
        pb.addPart(0, smilPart);
        //Fix for Bug 447934 end


        return pb;
    }

    public HashMap<Uri, InputStream> openPartFiles(ContentResolver cr) {
        HashMap<Uri, InputStream> openedFiles = null;     // Don't create unless we have to

        /* SPRD: Modify for bug#270981, ConcurrentModificationException
         * @orig
         *     for (SlideModel slide : mSlides) {
         * @{
         * */
        CopyOnWriteArrayList<SlideModel> safeList = new CopyOnWriteArrayList(mSlides);
        for(SlideModel slide : safeList){
        /* @} */
            for (MediaModel media : slide) {
                if (media.isText()) {
                    continue;
                }
                Uri uri = media.getUri();
                InputStream is;
                try {
                    is = cr.openInputStream(uri);
                    if (is != null) {
                        if (openedFiles == null) {
                            openedFiles = new HashMap<Uri, InputStream>();
                        }
                        openedFiles.put(uri, is);
                    }
                } catch (FileNotFoundException e) {
                    Log.e(TAG, "openPartFiles couldn't open: " + uri, e);
                }
            }
        }
        //Fix for Bug 447934 begin
        getExtraMediaContainer().openPartFiles(cr,openedFiles);
        //Fix for Bug 447934 end
        return openedFiles;
    }

    public PduBody makeCopy() {
        return makePduBody(SmilHelper.getDocument(this));
    }

    public SMILDocument toSmilDocument() {
        if (mDocumentCache == null) {
            mDocumentCache = SmilHelper.getDocument(this);
        }
        return mDocumentCache;
    }

    public static PduBody getPduBody(Context context, Uri msg) throws MmsException {
        PduPersister p = PduPersister.getPduPersister(context);
        GenericPdu pdu = p.load(msg);

        int msgType = pdu.getMessageType();
        if ((msgType == PduHeaders.MESSAGE_TYPE_SEND_REQ)
                || (msgType == PduHeaders.MESSAGE_TYPE_RETRIEVE_CONF)) {
            return ((MultimediaMessagePdu) pdu).getBody();
        } else {
            throw new MmsException();
        }
    }

    public void setCurrentMessageSize(int size) {
	    // Fix for bug 447934 begin
        size -= (lengthSignature + getExtraMediaContainer().getAllExtraMediasSize());
        if (size < 0) {
            size = 0;
        }
        // Fix for bug 447934 end
        mCurrentMessageSize = size;
    }

    // getCurrentMessageSize returns the size of the message, not including resizable attachments
    // such as photos. mCurrentMessageSize is used when adding/deleting/replacing non-resizable
    // attachments (movies, sounds, etc) in order to compute how much size is left in the message.
    // The difference between mCurrentMessageSize and the maxSize allowed for a message is then
    // divided up between the remaining resizable attachments. While this function is public,
    // it is only used internally between various MMS classes. If the UI wants to know the
    // size of a MMS message, it should call getTotalMessageSize() instead.
    public int getCurrentMessageSize() {
        //Fix for Bug 447934 begin
        return mCurrentMessageSize + lengthSignature
                + getExtraMediaContainer().getAllExtraMediasSize() ;
        //Fix for Bug 447934 end
    }

    // getTotalMessageSize returns the total size of the message, including resizable attachments
    // such as photos. This function is intended to be used by the UI for displaying the size of the
    // MMS message.
    public int getTotalMessageSize() {
        // Fix for Bug 447934 begin
        return mTotalMessageSize + lengthSignature
                + getExtraMediaContainer().getAllExtraMediasSize();
        // Fix for Bug 447934 end
    }

    public void increaseMessageSize(int increaseSize) {
        if (increaseSize > 0) {
            mCurrentMessageSize += increaseSize;
        }
    }

    public void decreaseMessageSize(int decreaseSize) {
        if (decreaseSize > 0) {
            mCurrentMessageSize -= decreaseSize;
        }
    }

    public LayoutModel getLayout() {
        return mLayout;
    }

    //
    // Implement List<E> interface.
    //
    public boolean add(SlideModel object) {
        if (object != null) { // changed by 4645 2014-12-24 for coverity
            int increaseSize = object.getSlideSize();
            checkMessageSize(increaseSize);

            if (mSlides.add(object)) {
                increaseMessageSize(increaseSize);
                object.registerModelChangedObserver(this);
                for (IModelChangedObserver observer : mModelChangedObservers) {
                    object.registerModelChangedObserver(observer);
                }
                notifyModelChanged(true);
                return true;
            }
        }
        return false;
    }

    public boolean addAll(Collection<? extends SlideModel> collection) {
        throw new UnsupportedOperationException("Operation not supported.");
    }

    public void clear() {
        if (mSlides.size() > 0) {
            for (SlideModel slide : mSlides) {
                slide.unregisterModelChangedObserver(this);
                for (IModelChangedObserver observer : mModelChangedObservers) {
                    slide.unregisterModelChangedObserver(observer);
                }
            }
            mCurrentMessageSize = 0;
            mSlides.clear();
            notifyModelChanged(true);
        }
    }

    public boolean contains(Object object) {
        return mSlides.contains(object);
    }

    public boolean containsAll(Collection<?> collection) {
        return mSlides.containsAll(collection);
    }

    public boolean isEmpty() {
        return mSlides.isEmpty();
    }

    public Iterator<SlideModel> iterator() {
        return mSlides.iterator();
    }

    public boolean remove(Object object) {
        if ((object != null) && mSlides.remove(object)) {
            SlideModel slide = (SlideModel) object;
            decreaseMessageSize(slide.getSlideSize());
            slide.unregisterAllModelChangedObservers();
            notifyModelChanged(true);
            return true;
        }
        return false;
    }

    public boolean removeAll(Collection<?> collection) {
        throw new UnsupportedOperationException("Operation not supported.");
    }

    public boolean retainAll(Collection<?> collection) {
        throw new UnsupportedOperationException("Operation not supported.");
    }

    public int size() {
        return mSlides.size();
    }

    public Object[] toArray() {
        return mSlides.toArray();
    }

    public <T> T[] toArray(T[] array) {
        return mSlides.toArray(array);
    }

    public void add(int location, SlideModel object) {
        if (object != null) {
            int increaseSize = object.getSlideSize();
            /* SPRD:Bug#289416,Mms occur exception when add Slideshow.
             * orig: checkMessageSize(increaseSize); @{ */
            checkMessageSize(increaseSize + SmilPlayerConfig.getPreSaveSlideSize());
              /* }@ */
            /* SPRD: add for bug307151 of monkey issue @{ */
            if (mSlides.size() >= location) {

            mSlides.add(location, object);
            increaseMessageSize(increaseSize);
            object.registerModelChangedObserver(this);
            for (IModelChangedObserver observer : mModelChangedObservers) {
                object.registerModelChangedObserver(observer);
            }
            notifyModelChanged(true);
            }
            /* @} */
        }
    }

    public boolean addAll(int location,
            Collection<? extends SlideModel> collection) {
        throw new UnsupportedOperationException("Operation not supported.");
    }

    public SlideModel get(int location) {
        return (location >= 0 && location < mSlides.size()) ? mSlides.get(location) : null;
    }

    public int indexOf(Object object) {
        return mSlides.indexOf(object);
    }

    public int lastIndexOf(Object object) {
        return mSlides.lastIndexOf(object);
    }

    public ListIterator<SlideModel> listIterator() {
        return mSlides.listIterator();
    }

    public ListIterator<SlideModel> listIterator(int location) {
        return mSlides.listIterator(location);
    }

    public SlideModel remove(int location) {
        SlideModel slide = mSlides.remove(location);
        if (slide != null) {
            decreaseMessageSize(slide.getSlideSize());
            slide.unregisterAllModelChangedObservers();
            notifyModelChanged(true);
        }
        return slide;
    }

    public SlideModel set(int location, SlideModel object) {
        SlideModel slide = mSlides.get(location);
        if (null != object) {
            int removeSize = 0;
            int addSize = object.getSlideSize();
            if (null != slide) {
                removeSize = slide.getSlideSize();
            }
            if (addSize > removeSize) {
                checkMessageSize(addSize - removeSize);
                increaseMessageSize(addSize - removeSize);
            } else {
                decreaseMessageSize(removeSize - addSize);
            }
        }

        slide =  mSlides.set(location, object);
        if (slide != null) {
            slide.unregisterAllModelChangedObservers();
        }

        if (object != null) {
            object.registerModelChangedObserver(this);
            for (IModelChangedObserver observer : mModelChangedObservers) {
                object.registerModelChangedObserver(observer);
            }
        }

        notifyModelChanged(true);
        return slide;
    }

    public List<SlideModel> subList(int start, int end) {
        return mSlides.subList(start, end);
    }

    @Override
    protected void registerModelChangedObserverInDescendants(
            IModelChangedObserver observer) {
        mLayout.registerModelChangedObserver(observer);

        for (SlideModel slide : mSlides) {
            slide.registerModelChangedObserver(observer);
        }
        //Fix for Bug 447934 begin
        getExtraMediaContainer().registerModelChangedObserver(observer);
        //Fix for Bug 447934 end
    }

    @Override
    protected void unregisterModelChangedObserverInDescendants(
            IModelChangedObserver observer) {
        mLayout.unregisterModelChangedObserver(observer);

        for (SlideModel slide : mSlides) {
            slide.unregisterModelChangedObserver(observer);
        }
        //Fix for Bug 447934 begin
        getExtraMediaContainer().unregisterModelChangedObserver(observer);
        //Fix for Bug 447934 end
    }

    @Override
    protected void unregisterAllModelChangedObserversInDescendants() {
        mLayout.unregisterAllModelChangedObservers();

        for (SlideModel slide : mSlides) {
            slide.unregisterAllModelChangedObservers();
        }
        //Fix for Bug 447934 begin
        getExtraMediaContainer().unregisterAllModelChangedObserver();
        //Fix for Bug 447934 end
    }

    public void onModelChanged(Model model, boolean dataChanged) {
        if (dataChanged) {
            mDocumentCache = null;
            mPduBodyCache = null;
        }
    }

    public void sync(PduBody pb) {
        for (SlideModel slide : mSlides) {
            for (MediaModel media : slide) {
                PduPart part = pb.getPartByContentLocation(media.getSrc());
                if (part != null) {
                    media.setUri(part.getDataUri());
                }
            }
        }
        //Fix for Bug 447934 begin
        getExtraMediaContainer().sync(pb);
        //Fix for Bug 447934 end
    }

    public void checkMessageSize(int increaseSize) throws ContentRestrictionException {
        ContentRestriction cr = ContentRestrictionFactory.getContentRestriction();
        // Fixed for bug206239 begin
        //cr.checkMessageSize(mCurrentMessageSize, increaseSize, mContext.getContentResolver());
        cr.checkMessageSize(getTotalMsgSizeWithAllHead(), increaseSize, mContext.getContentResolver());
        // Fixed for bug 206239 end
    }
    // Fixed for bug206239 begin
    public int getTotalSlideCount() {
        return mSlides.size();
    }
    public int getTotalMsgSizeWithSlideHead() {
        return (getCurrentMessageSize()+getTotalSlideCount()*SmilPlayerConfig.getPreSaveSlideSize());
    }
    public int getTotalMsgSizeWithAllHead() {
        return (SmilPlayerConfig.getMessageWithPduHeadSize(getTotalMsgSizeWithSlideHead()));
    }
    public int getTotalMsgSizeWithAllHead(int size) {
        return (SmilPlayerConfig.getMessageWithPduHeadSize(size+getTotalSlideCount()*SmilPlayerConfig.getPreSaveSlideSize()));
    }
    // Fixed for bug 206239 end

    /**
     * Determines whether this is a "simple" slideshow.
     * Criteria:
     * - Exactly one slide
     * - Exactly one multimedia attachment, but no audio
     * - It can optionally have a caption
    */
    public boolean isSimple() {
        // There must be one (and only one) slide.
        if (size() != 1)
            return false;

        SlideModel slide = get(0);
        //Fix for Bug 447934 begin
        // The slide must have an image or video,
        // but not two of them together.
        if (!(slide.hasImage() ^ slide.hasVideo()))
            return false;
        //Fix for Bug 447934 end
        // No audio allowed.
        if (slide.hasAudio())
            return false;

        return true;
    }

    /**
     * Make sure the text in slide 0 is no longer holding onto a reference to the text
     * in the message text box.
     */
    public void prepareForSend() {
        if (size() == 1) {
            TextModel text = get(0).getText();
            if (text != null) {
                text.cloneText();
            }
        }
    }

    /**
     * Resize all the resizeable media objects to fit in the remaining size of the slideshow.
     * This should be called off of the UI thread.
     *
     * @throws MmsException, ExceedMessageSizeException
     */
    public void finalResize(Uri messageUri) throws MmsException, ExceedMessageSizeException {

        // Figure out if we have any media items that need to be resized and total up the
        // sizes of the items that can't be resized.
        int resizableCnt = 0;
        int fixedSizeTotal = 0;
        for (SlideModel slide : mSlides) {
            for (MediaModel media : slide) {
                if (media.getMediaResizable()) {
                    ++resizableCnt;
                } else {
                    fixedSizeTotal += media.getMediaSize();
                }
            }
        }
        //Fix for Bug 447934 begin
        fixedSizeTotal += getExtraMediaContainer().getAllExtraMediasSize();
        //Fix for Bug 447934 end
        if (Log.isLoggable(TAG, Log.VERBOSE)) {
            Log.v(TAG, "finalResize: original message size: " + getCurrentMessageSize() +
                    " getMaxMessageSize: " + SmilPlayerConfig.getMaxMessageSize() +
                    " fixedSizeTotal: " + fixedSizeTotal);
        }
        if (resizableCnt > 0) {
            int remainingSize = SmilPlayerConfig.getMaxMessageSize() - fixedSizeTotal - SLIDESHOW_SLOP;
            if (remainingSize <= 0) {
                throw new ExceedMessageSizeException("No room for pictures");
            }
            long messageId = ContentUris.parseId(messageUri);
            int bytesPerMediaItem = remainingSize / resizableCnt;
            // Resize the resizable media items to fit within their byte limit.
            for (SlideModel slide : mSlides) {
                for (MediaModel media : slide) {
                    if (media.getMediaResizable()) {
                        media.resizeMedia(bytesPerMediaItem, messageId);
                    }
                }
            }
            // One last time through to calc the real message size.
            int totalSize = 0;
            for (SlideModel slide : mSlides) {
                for (MediaModel media : slide) {
                    totalSize += media.getMediaSize();
                }
            }
            if (Log.isLoggable(TAG, Log.VERBOSE)) {
                Log.v(TAG, "finalResize: new message size: " + totalSize);
            }

            if (totalSize > SmilPlayerConfig.getMaxMessageSize()) {
                throw new ExceedMessageSizeException("After compressing pictures, message too big");
            }
            setCurrentMessageSize(totalSize);

            onModelChanged(this, true);     // clear the cached pdu body
            PduBody pb = toPduBody();
            // This will write out all the new parts to:
            //      /data/data/com.android.providers.telephony/app_parts
            // and at the same time delete the old parts.
            PduPersister.getPduPersister(mContext).updateParts(messageUri, pb, null);
        }
    }

    /**
     * SPRD: Add for support mms creation mode. @{
     * @throws ContentRestrictionException
     */
    public void checkContentType() throws ContentRestrictionException {
        if ( !MessagingPreferenceActivityIsRestrictedMode(mContext) ) {
            return;
        }
        ContentRestriction cr = ContentRestrictionFactory.getContentRestriction();
        for (SlideModel slide : mSlides) {
            for (MediaModel media : slide) {
                cr.checkCGFContentType(media.getContentType(), mContext);
            }
        }
		//Fix for Bug 447934 begin
        getExtraMediaContainer().checkContentType(cr);
        //Fix for Bug 447934 end
    }
    /** @} */

    /**
     * SPRD: Add for support other file. @{
     *
     * @return
     */
    public boolean hasOtherFile() {
        if (null == mFiles || mFiles.size() == 0) {
            return false;
        }
        return true;
    }
    /** @} */

    /**
     * SPRD: Add for support other file. @{
     * @param files
     */
    public void setOtherFiles(ArrayList<FileModel> files){
        if(null != files){
            mFiles = files;
        }else{
            mFiles = new ArrayList<FileModel>();
        }
    }
    /** @} */

    /**
     * SPRD: Add for support mms creation mode. @{
     * @throws ContentRestrictionException
     * @throws MmsException
     */
    public void checkResolution() throws ContentRestrictionException, MmsException {
        if (!MessagingPreferenceActivityIsRestrictedMode(mContext)) {
            return;
        }
        ContentRestriction cr = ContentRestrictionFactory.getContentRestriction();
        for (SlideModel slide : mSlides) {
            for (MediaModel media : slide) {
                if (media instanceof ImageModel) {
                    ImageModel im = (ImageModel)media;
                    cr.checkResolution(im.getWidth(), im.getHeight(), mContext);
                }
            }
        }
        //Fix for Bug 447934 begin
        getExtraMediaContainer().checkResolution(cr, mLayout);
        //Fix for Bug 447934 end

    }
    /** @} */

    /**
     * SPRD: Add for mms compose func. @{
     * If the media name is duplicated with the old ones, we have to rename it
     * before the Smil Document created.
     */
    private void renameMediaNameIfNeeded() {
        if (isOSDebug()) {
            Log.d(TAG,
                    "Begin to check if there is duplicated media names. The current slides size :"
                            + mSlides.size());
        }
        Set hashSet = new HashSet();
        for (int i = 0; i < mSlides.size(); i++) {
            for (MediaModel media : mSlides.get(i)) {
                if (isOSDebug()) {
                    Log.d(TAG, "Media which is going to be added into HashSet. Media Name = "
                            + media.getSrc());
                }
                // SPRD: "i+1" is current slide number.
                rename(hashSet, media, i+1);
                hashSet.add(media.getSrc());
            }
        }
        //Fix for Bug 447934 begin
        getExtraMediaContainer().renameMediaNameIfNeeded(hashSet);
        //Fix for Bug 447934 end
    }
    /** @} */

    /**
     * SPRD: Add for mms compose func. @{
     * We have to call this function recursively to make sure there is
     * no duplicated media names in current Slideshow.
     * @param hashset
     * @param mediamodel
     */
    private void rename(Set hashset, MediaModel mediamodel, int currentSlideNum) {
        if (hashset.contains(mediamodel.getSrc())) {
            String mediaRename = currentSlideNum + mediamodel.getSrc();
            mediamodel.setSrc(mediaRename);
            if (isOSDebug()) {
                Log.d(TAG, "It has existed, rename it as : " + mediaRename);
            }
            // Rename recursively if it has existed in the HashSet.
            rename(hashset, mediamodel, currentSlideNum);
        }
    }
    /** @} */

    //Fix for Bug 433743 begin
    public boolean hasVdatas() {
        boolean hasVcalendar = false;
        boolean hasVcard = false;
        for (MediaModel media : getExtraMedias()) {
            if (media.isVcalendar()) {
                hasVcalendar = true;
            }
            if (media.isVcard()) {
                hasVcard = true;
            }
        }
        return hasVcalendar && hasVcard;
    }

    public boolean hasVcard() {
        boolean result = false;
        for (MediaModel media : getExtraMedias()) {
            if (media.isVcard()) {
                result = true;
                break;
            }
        }
        return result;
    }

	public boolean hasVcalendar() {
        boolean result = false;
        for (MediaModel media : getExtraMedias()) {
            if (media.isVcalendar()) {
                result = true;
                break;
            }
        }
        return result;
    }
    //Fix for Bug 433743 end



    /* add for bug   447934
     SPRD: add for bug 252067 end
     SPRD: add for bug 264624 @{
    public void makeVcardModelFirst() {
        int slideCount = mSlides != null ? mSlides.size() : 0;
        if (slideCount > 0) {
            int vcardIndex;
            for (vcardIndex = 0; vcardIndex < slideCount; vcardIndex++) {
                SlideModel sm = get(vcardIndex);
                if (sm != null && sm.hasVcard()) {
                    break;
                }
            }
            if (vcardIndex == 0 || vcardIndex == slideCount)
                return;
            SlideModel sm = get(vcardIndex);
            for (; vcardIndex > 0; vcardIndex--) {
                mSlides.set(vcardIndex, get(vcardIndex - 1));
            }
            mSlides.set(0, sm);
        }
    }
    */

    /* SPRD: Save html type attachment size is 0kb, fixed for bug267576. @{ */
    public ArrayList<MediaModel> getMediaList() {
        if (!mMediaList.isEmpty()) {
            return mMediaList;
        }
        addMediaList();
        return mMediaList;
    }
    public ArrayList<CharSequence> getMediaNameList() {
        if (!mMediaNameList.isEmpty()) {
            return mMediaNameList;
        }
        addMediaList();
        return mMediaNameList;
    }
    private void addMediaList() {
        for (SlideModel slideModel : mSlides) {
            for (MediaModel media : slideModel) {
                if (!media.isText()) {
                    mMediaNameList.add(media.getSrc());
                    mMediaList.add(media);
                }
            }
        }
        //Fix for Bug 447934 begin
        for (MediaModel media : getExtraMedias()) {
            if (!media.isText()) {
                mMediaNameList.add(media.getSrc());
                mMediaList.add(media);
            }
        }
        //Fix for Bug 447934 end
    }
    /* @} */

    /* SPRD: Set singature length, fixed for bug343706. @{ */
    private void setSingatureLength() {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(mContext);
        boolean signatureEnabled = sp.getBoolean(MessagingPreferenceActivitySIGNATURE_ENABLE,
                false);
        String signatureText = sp.getString(MessagingPreferenceActivitySIGNATURE_INPUT, "");

        if (signatureEnabled && signatureText != null) {
            lengthSignature = signatureText.getBytes().length;
        }
    }
    /* @} */

    //Fix for Bug 447934 begin
    private ExtraMediaContainer mExtraMediaContainer;

    protected ExtraMediaContainer getExtraMediaContainer() {
        if (mExtraMediaContainer == null) {
            mExtraMediaContainer = new ExtraMediaContainer(this, mContext);
        }
        return mExtraMediaContainer;
    }

    public List<MediaModel> getExtraMedias() {
        return getExtraMediaContainer().getExtraMedias();
    }

    protected void setExtraMedias(List<MediaModel> list) {
        getExtraMediaContainer().setExtraMedias(list);
    }

    public boolean addExtraMedia(MediaModel object) {
        return getExtraMediaContainer().addExtraMedia(object);
    }

    public void addExtraMedia(int location, MediaModel media) {
        getExtraMediaContainer().addExtraMedia(location, media);
    }

    public void clearExtraMedias() {
        getExtraMediaContainer().clearExtraMedias();
    }

    public MediaModel getExtraMedia(int location) {
        return getExtraMediaContainer().getExtraMedia(location);
    }

    public boolean isExtraMediaEmpty() {
        return getExtraMediaContainer().isEmpty();
    }

    public MediaModel removeExtraMedia(int location) {
        return getExtraMediaContainer().removeExtraMedia(location);
    }

    public boolean removeExtraMedia(MediaModel media) {
        return getExtraMediaContainer().removeExtraMedia(media);
    }

    public MediaModel setExtraMedia(int location, MediaModel media) {
        return getExtraMediaContainer().setExtraMedia(location, media);
    }

    public int getExtraMediaCount() {
        return getExtraMediaContainer().size();
    }

    public int addExtraMedia(int type, Uri uri) {
        return getExtraMediaContainer().addExtraMedia(type, uri);
    }

    public int getExtraMediaType() {
        return getExtraMediaContainer().getAttachmentType();
    }
    //Fix for Bug 447934 end
}
