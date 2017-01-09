
package com.android.messaging.smilplayer.model;

import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;

import com.android.messaging.smilplayer.exception.ContentRestrictionException;
import com.android.messaging.smilplayer.exception.ExceedMessageSizeException;
import com.android.messaging.smilplayer.exception.ResolutionException;
import com.android.messaging.smilplayer.exception.UnsupportContentTypeException;
import com.android.messaging.smilplayer.exception.WarningModeResolutionException;
import com.android.messaging.smilplayer.exception.WarningModeUnsupportTypeException;

import com.google.android.mms.ContentType;
import com.google.android.mms.MmsException;
import com.google.android.mms.pdu.PduBody;
import com.google.android.mms.pdu.PduPart;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;


import com.android.messaging.smilplayer.util.WorkingMessage;
//import com.android.mms.ui.MessageUtils;

public class ExtraMediaContainer {
    private SlideshowModel mSlideshow = null;

    private int mExtraMediaSize = 0;

    private List<MediaModel> mExtraMedias = new ArrayList<MediaModel>();

    private static final String TAG = "ExtraMediaContainer";

    private Context mContext;

    ExtraMediaContainer(SlideshowModel slideshow, Context context) {
        mSlideshow = slideshow;
        mContext = context;
    }

    private static boolean isOSDebug(){
        return true;
    }

    protected List<MediaModel> getExtraMedias() {
        return mExtraMedias;
    }

    protected void setExtraMedias(List<MediaModel> list) {
        Log.i(TAG, "setExtraMedias list=" + list);
        clearExtraMedias();
        mExtraMedias.addAll(list);
		// Fix for bug380610 begin
        int size = 0;
        for (MediaModel media : list) {
            if(media == null){
                continue;
            }
            size += media.getMediaSize();
        }
        setAllExtraMediasSize(size);
        // Fix for bug380610 end
        registerModelChangedObserver(mSlideshow);
    }

    protected boolean addExtraMedia(MediaModel object) {
        Log.i(TAG, "addExtraMedia media=" + object);
        if (object != null) {
            int increaseSize = object.getMediaSize();
            mSlideshow.checkMessageSize(increaseSize);
            if ((object != null) && mExtraMedias.add(object)) {
                object.registerModelChangedObserver(mSlideshow);
                for (IModelChangedObserver observer : mSlideshow.mModelChangedObservers) {
                    object.registerModelChangedObserver(observer);
                }
                int newSize = getAllExtraMediasSize() + increaseSize;
                setAllExtraMediasSize(newSize);
                mSlideshow.notifyModelChanged(true);
                return true;
            }
        }
        return false;
    }

    protected void addExtraMedia(int location, MediaModel media) {
        Log.i(TAG, "addExtraMedia " + "location=" + location + "; media=" + media);
        if (media != null) {
            int increaseSize = media.getMediaSize();
            mExtraMedias.add(location, media);
            media.registerModelChangedObserver(mSlideshow);
            for (IModelChangedObserver observer : mSlideshow.mModelChangedObservers) {
                media.registerModelChangedObserver(observer);
            }
            int newSize = getAllExtraMediasSize() + increaseSize;
            setAllExtraMediasSize(newSize);
            mSlideshow.notifyModelChanged(true);
        }
    }

    protected void clearExtraMedias() {
        unregisterAllModelChangedObserver();
        for (MediaModel media : mExtraMedias) {
            if(media == null){
                continue;
            }
            deleteMediaFile(media);
        }
        setAllExtraMediasSize(0);
        mExtraMedias.clear();
        mSlideshow.notifyModelChanged(true);
    }

    protected MediaModel getExtraMedia(int location) {
        if (location < 0 || location > mExtraMedias.size()) {
            return null;
        }
        return mExtraMedias.get(location);
    }

    protected boolean isEmpty() {
        return mExtraMedias.isEmpty();
    }

    protected MediaModel removeExtraMedia(int location) {
        MediaModel media = mExtraMedias.remove(location);
        Log.i(TAG, "removeExtraMedia " + "location=" + location + "; media=" + media);
        if (media != null) {
            int newSize = getAllExtraMediasSize() - media.getMediaSize();
            setAllExtraMediasSize(newSize);
            media.unregisterAllModelChangedObservers();
            mSlideshow.notifyModelChanged(true);
            deleteMediaFile(media);
        }
        return media;
    }

    protected boolean removeExtraMedia(MediaModel media) {
        if ((media != null) && mExtraMedias.remove(media)) {
            int newSize = getAllExtraMediasSize() - media.getMediaSize();
            setAllExtraMediasSize(newSize);
            media.unregisterAllModelChangedObservers();
            mSlideshow.notifyModelChanged(true);
            deleteMediaFile(media);
            return true;
        }
        return false;
    }

    protected MediaModel setExtraMedia(int location, MediaModel media) {
        MediaModel old = mExtraMedias.get(location);
        if (null != media) {
            int removeSize = 0;
            int addSize = media.getMediaSize();
            if (null != old) {
                removeSize = old.getMediaSize();
            }
            if (addSize > removeSize) {
                mSlideshow.checkMessageSize(addSize - removeSize);
            }

            old = mExtraMedias.set(location, media);
            if (old != null) {
                old.unregisterAllModelChangedObservers();
            }
            media.registerModelChangedObserver(mSlideshow);
            for (IModelChangedObserver observer : mSlideshow.mModelChangedObservers) {
                media.registerModelChangedObserver(observer);
            }
            int newSize = getAllExtraMediasSize() + (addSize - removeSize);
            setAllExtraMediasSize(newSize);
            mSlideshow.notifyModelChanged(true);
        }
        if (old != null) //added by 4645 2014-12-23 for coverity
            deleteMediaFile(old);
        return old;
    }

    protected int size() {
        return mExtraMedias.size();
    }

    protected int getAllExtraMediasSize() {
        return mExtraMediaSize;
    }

    protected void setAllExtraMediasSize(int size) {
        if (size >= 0) {
            mExtraMediaSize = size;
        } else {
            mExtraMediaSize = 0;
        }
        Log.i(TAG, "setAllExtraMediasSize mExtraMediaSize=" + mExtraMediaSize);
    }

    protected int addExtraPduParts(PduBody pb) {
        int partCount = 0;
        for (MediaModel media : mExtraMedias) {
            if(media == null){
                continue;
            }
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
                    String contentId = (index == -1) ? location : location.substring(0, index);
                    if (contentId != null && !contentId.isEmpty()) {
                        part.setContentId(contentId.getBytes());
                    }
                }
            }
            if (media.isText()) {
                part.setData(((TextModel) media).getText().getBytes());
            } else if (media.isImage() || media.isVideo() || media.isAudio() || media.isVcard()
                    || media.isVcalendar()) {
                part.setDataUri(media.getUri());
            } else {
                Log.w(TAG, "Unsupport media: " + media);
            }
            pb.addPart(part);
            partCount++;
        }
        return partCount;
    }

    public static MediaModel createMediaModel(PduPart part, Context context) {
        MediaModel media = null;
        byte[] bytes = part.getContentType();
        if (null != bytes) {
            String contentType = new String(bytes);
            Uri uri = part.getDataUri();
            String contentLocation = null;
            if (null == part.getContentLocation()) {
                contentLocation = "";
            } else {
                contentLocation = new String(part.getContentLocation());
            }
            if (isOSDebug()) {
                Log.d(TAG, "createMediaModel contentType == " + contentType
                        + " contentLocation == " + contentLocation + " uri:" + uri);
            }
            if (!ContentType.APP_SMIL.equalsIgnoreCase(contentType)) {
                if (null != uri) {
                    try {
                        if (SmilHelper.containsTag(part, SmilHelper.ELEMENT_TAG_VCALENDAR_FILE)
                                || SmilHelperSprd.isVCalendar(part)) {
                            media = new VcalendarModel(context, part.generateLocation(),
                                    part.getDataUri());
                        } else if (SmilHelper.containsTag(part, SmilHelper.EXTENDS_TAG_VCARD)
                                || SmilHelperSprd.isVcard(part)) {
                            media = new VcardModel(context, part.getDataUri());
                        } else {
                            media = new FileModel(context, null, contentType, contentLocation, uri);
                        }
                        if (isOSDebug()) {
                            Log.d(TAG, "createMediaModel MediaModel media.mContentType:"
                                    + media.mContentType + " media.mSrc:" + media.mSrc);
                        }
                    } catch (MmsException e) {
                        Log.e(TAG, "createMediaModel MmsException caught while creating FileModel",
                                e);
                    }
                }
            }
        }
        return media;
    }

    protected void registerModelChangedObserver(IModelChangedObserver observer) {
        for (MediaModel media : mExtraMedias) {
            //bug 368289 delete for signature function begin
            if (media != null) {
                media.registerModelChangedObserver(observer);
            }
            // bug 368289 delete for signature function end
        }
    }

    protected void unregisterModelChangedObserver(IModelChangedObserver observer) {
        for (MediaModel media : mExtraMedias) {
            //bug 378977 begin
            if(media != null){
                media.unregisterModelChangedObserver(observer);
            }
            //bug 378977 end
        }
    }

    protected void unregisterAllModelChangedObserver() {
        for (MediaModel media : mExtraMedias) {
            //bug 378977 begin
            if(media != null){
                media.unregisterAllModelChangedObservers();
            }
            //bug 378977 end
        }
    }

    protected void sync(PduBody pb) {
        for (MediaModel media : mExtraMedias) {
		    if(media == null){
                continue;
            }
            PduPart part = pb.getPartByContentLocation(media.getSrc());
            if (part != null) {
                media.setUri(part.getDataUri());
            }
        }
    }

    protected void checkContentType(ContentRestriction cr) throws ContentRestrictionException {
        for (MediaModel media : mExtraMedias) {
		    if(media == null){
                continue;
            }
            cr.checkCGFContentType(media.getContentType(), mContext);
        }
    }

    protected void checkResolution(ContentRestriction cr, LayoutModel layout)
            throws ContentRestrictionException, MmsException {
        for (MediaModel media : mExtraMedias) {
		    if(media == null){
                continue;
            }
            try {
                cr.checkImageContentType(media.getContentType(), mContext);
            } catch (Exception e) {
                continue;
            }
            ImageModel im = new ImageModel(mContext, media.getUri(), layout.getImageRegion());
            // cr.checkResolution(im.getWidth(), im.getHeight());
        }
    }

    protected void renameMediaNameIfNeeded(Set hashSet) {
        for (int i = 0; i < size(); i++) {
            MediaModel media = mExtraMedias.get(i);
			if(media == null){
                continue;
            }
            if (isOSDebug()) {
                Log.d(TAG,
                        "Media which is going to be added into HashSet. Media Name = "
                                + media.getSrc());
            }
            rename(hashSet, media, i + 1);
            hashSet.add(media.getSrc());
        }
    }

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

    public static boolean isExtraMedia(MediaModel media) {
        if (media.isAudio() || media.isImage() || media.isText() || media.isVideo()) {
            return false;
        }
        return true;
    }

    public static boolean isExtraMediaType(int type) {
        switch (type) {
            case WorkingMessage.AUDIO:
            case WorkingMessage.IMAGE:
            case WorkingMessage.TEXT:
            case WorkingMessage.VIDEO:
                return false;
        }
        return true;
    }

    public int openPartFiles(ContentResolver cr, HashMap<Uri, InputStream> openedFiles) {
        int count = 0;
        for (MediaModel media : mExtraMedias) {
            if (media == null ||media.isText()) {
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
        return count;
    }

    private void addMediaList(ArrayList<CharSequence> mediaNameList, ArrayList<MediaModel> mediaList) {
        for (MediaModel media : mExtraMedias) {
		    if (media == null) {
                continue;
            }
            mediaNameList.add(media.getSrc());
            mediaList.add(media);
        }
    }

    protected int addExtraMedia(int type, Uri uri) {
        int result = WorkingMessage.OK;
        MediaModel media = null;
        try {
            if (type == WorkingMessage.VCARD) {
                media = new VcardModel(mContext, uri);
            } else if (type == WorkingMessage.VCALENDAR) {
                media = new VcalendarModel(mContext, uri);
            } else if (type == WorkingMessage.OTHER_FILE) {
                String path = uri.getPath();
                if (TextUtils.isEmpty(path)) {
                    int start = path.lastIndexOf("/");
                    if (start != -1) {
                        String src = path.substring(start);
                        media = new FileModel(mContext, FileModel.ALL_FILE_TYPE, src, uri);
                    } else {
                        Log.e(TAG, "addExtraMedia: FileModel path = " + path);
                        result = WorkingMessage.UNKNOWN_ERROR;
                    }
                } else {
                    Log.e(TAG, "addExtraMedia: FileModel path empty");
                    result = WorkingMessage.UNKNOWN_ERROR;
                }
            } else {
                result = WorkingMessage.UNSUPPORTED_TYPE;
            }
            if (media == null) {
                Log.e(TAG, "addExtraMedia: media=null");
                result = WorkingMessage.UNKNOWN_ERROR;
            }
            if (result == WorkingMessage.OK) {
                addExtraMedia(media);
            }
        } catch (MmsException e) {
            Log.e(TAG, "addExtraMedia:", e);
            result = WorkingMessage.UNKNOWN_ERROR;
        } catch (UnsupportContentTypeException e) {
            Log.e(TAG, "addExtraMedia:", e);
            result = WorkingMessage.UNSUPPORTED_TYPE;
        } catch (ExceedMessageSizeException e) {
            Log.e(TAG, "addExtraMedia:", e);
            result = WorkingMessage.MESSAGE_SIZE_EXCEEDED;
        } catch (ResolutionException e) {
            Log.e(TAG, "addExtraMedia:", e);
            result = WorkingMessage.IMAGE_TOO_LARGE;
        } catch (WarningModeUnsupportTypeException e) {
            Log.e(TAG, "addExtraMedia:", e);
            result = WorkingMessage.WARNING_MODE_UNSUPPORTED_TYPE;
        } catch (WarningModeResolutionException e) {
            Log.e(TAG, "addExtraMedia:", e);
            result = WorkingMessage.WARNING_MODE_IMAGE_TOO_LARGE;
        } catch (ContentRestrictionException e) {
            result = WorkingMessage.UNSUPPORTED_TYPE;
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "IllegalArgumentException:", e);
            result = WorkingMessage.URI_ISNULL;
        }
        return result;
    }

    protected int getAttachmentType() {
        boolean hasVcard = false;
        boolean hasVcalender = false;
        boolean hasOtherFile = false;
        for (MediaModel media : getExtraMedias()) {
			if (media == null) {
                continue;
            }
            if (media.isVcard()) {
                hasVcard = true;
                continue;
            }
            if (media.isVcalendar()) {
                hasVcalender = true;
                continue;
            }
            hasOtherFile = true;
        }

        if (hasVcard && !hasVcalender && !hasOtherFile) {
            return WorkingMessage.VCARD;
        } else if (!hasVcard && hasVcalender && !hasOtherFile) {
            return WorkingMessage.VCALENDAR;
        } else if (!hasVcard && !hasVcalender && hasOtherFile) {
            return WorkingMessage.OTHER_FILE;
        } else if (hasVcard || hasVcalender || hasOtherFile) {
            return WorkingMessage.EXTRAMEDIAS;
        }
        return WorkingMessage.TEXT;
    }

    private boolean deleteMediaFile(MediaModel media) {
        if (media == null) //added by 4645 2014-12-23 for coverity
            return true;
        try {
            String path = media.getUri().getPath();
            if (path != null && path.startsWith(mContext.getFilesDir().getPath())) {
                File file = new File(path);
                Log.e(TAG, "delete media file: " + path);
                file.delete();
            }
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
        return true;
    }
}
