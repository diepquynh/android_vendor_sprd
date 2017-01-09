package com.android.messaging.smilplayer.model;

import org.w3c.dom.events.Event;

import android.content.Context;
import android.net.Uri;

import java.io.IOException;
import java.util.HashMap;

import com.google.android.mms.MmsException;

public class FileModel extends MediaModel {

    private String detail;
    public static final String ALL_FILE_TYPE = "*/*";

    public FileModel(Context context, String tag, String contentType, String src, Uri uri)
            throws MmsException {
        super(context, SmilHelper.ELEMENT_TAG_FILE, contentType, src, uri);
    }

    public FileModel(Context context, String contentType, String src, Uri uri) throws MmsException {
        super(context, SmilHelper.ELEMENT_TAG_FILE, contentType, src, uri);
    }

    @Override
    public void handleEvent(Event evt) {
    }

    public String getDetail() {
        return detail;
    }

    public void setDetail(String detail) {
        this.detail = detail;
    }

}
