/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.downloadcenter;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import android.util.Log;

import java.util.ArrayList;

/**
 * parser xml by sax
 */
public class ThumbnailDefaultHandler extends DefaultHandler {
    //thumbnail list
    private ArrayList<ThumbnailMode> mThumbnailList;
    //thumbnails mode
    private ThumbnailsMode mThumbnailsMode;
    //thumbnail mode
    private ThumbnailMode mThumbnailMode;
    //prefix tag
    private String mPrefixTAG;
    private static final String TAG_LIST_RESOURCES = "list-resources";
    private static final String TAG_THUMBNAILS = "resources";
    private static final String TAG_THUMBNAILS_TYPE = "type";
    private static final String TAG_THUMBNAILS_GROUP_ID = "group-id";
    private static final String TAG_THUMBNAILS_DENSITY = "density";
    private static final String TAG_THUMBNAIL_THUMBNAIL = "resource";
    private static final String TAG_THUMBNAIL_ID = "id";
    private static final String TAG_THUMBNAIL_NAME = "name";
    private static final String TAG_THUMBNAIL_THUMB_URL = "thumb-url";
    private static final String TAG_THUMBNAIL_DOWNLOAD_RUL = "download-url";

    public ThumbnailsMode getThumbnailsMode() {
        return mThumbnailsMode;
    }

    /**
     * start to parser document
     */
    public void startDocument() throws SAXException {
        super.startDocument();
        mThumbnailList = new ArrayList<ThumbnailMode>();
    }

    /**
     * start to parser node element
     */
    public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
        super.startElement(uri, localName, qName, attributes);
        if(TAG_LIST_RESOURCES.equals(localName)) {
            mThumbnailsMode = new ThumbnailsMode();
            mThumbnailsMode.setThumbnailsAction(localName);
        }
        if(TAG_THUMBNAIL_THUMBNAIL.equals(localName)) {
            mThumbnailMode = new ThumbnailMode();
        }
        mPrefixTAG = localName;
    }

    /**
     * get the node data
     */
    public void characters(char[] ch, int start, int length) throws SAXException {
        super.characters(ch, start, length);
        if(mPrefixTAG != null) {
            String data=new String(ch, start, length);
            if(TAG_THUMBNAILS_TYPE.equals(mPrefixTAG)) {
                mThumbnailsMode.appendThumbnailsType(data);
            } else if(TAG_THUMBNAILS_GROUP_ID.equals(mPrefixTAG)) {
                mThumbnailsMode.appendThumbnailsGroupId(data);
            } else if(TAG_THUMBNAILS_DENSITY.equals(mPrefixTAG)) {
                mThumbnailsMode.appendThumbnailsDensity(data);
            } else if(TAG_THUMBNAIL_ID.equals(mPrefixTAG)) {
                mThumbnailMode.appendThumbnailId(data);
            } else if(TAG_THUMBNAIL_NAME.equals(mPrefixTAG)) {
                mThumbnailMode.appendThumbnailName(data);
            } else if(TAG_THUMBNAIL_THUMB_URL.equals(mPrefixTAG)) {
                mThumbnailMode.appendThumnailUrl(data);
            } else if(TAG_THUMBNAIL_DOWNLOAD_RUL.equals(mPrefixTAG)) {
                mThumbnailMode.appendDownloadUrl(data);
            }
        }
    }

    /**
     * end to parser node element
     */
    public void endElement(String uri, String localName, String qName) throws SAXException {
        super.endElement(uri, localName, qName);
        if(TAG_THUMBNAILS.equals(localName))  {
            mThumbnailsMode.setThumbnailsModeList(mThumbnailList);
        } else if(TAG_THUMBNAIL_THUMBNAIL.equals(localName)) {
            mThumbnailList.add(mThumbnailMode);
            mThumbnailMode = null;
        }

        mPrefixTAG = null;
    }

    /**
     * end to parser document
     */
    public void endDocument() throws SAXException {
        super.endDocument();
    }
}
