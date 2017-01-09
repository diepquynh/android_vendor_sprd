/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import java.util.ArrayList;

public class AttributesDefaultHandler extends DefaultHandler {
    private ArrayList<ViewAttributes> mAttrList;
    private ViewAttributes mAttributes;
    private String mPrefixTAG;
    private static final String TAG_ATTR_COLOR = "color";
    private static final String TAG_ATTR_COLOR_TEXT = "text";
    private static final String TAG_ATTR_TEXT_SIZE = "text-size";
    private static final String TAG_ATTR_COLOR_FILL = "fill";
    private static final String TAG_ATTR_COLOR_GRADIENT_END = "gradient-end";
    private static final String TAG_ATTR_COLOR_OUTLINE = "outline";
    private static final String TAG_ATTR_COLOR_STROKE = "stroke";
    private static final String TAG_ATTR_COLOR_SHADOW = "shadow";
    private static final String TAG_ATTR_COLOR_FONT = "font";
    private static final String TAG_ATTR_LABEL_LABEL = "label";
    private static final String TAG_ATTR_LABEL_HEAD = "head";
    private static final String TAG_ATTR_LABEL_TAIL = "tail";

    public ArrayList<ViewAttributes> getAttributesList() {
        return mAttrList;
    }

    @Override
    public void startDocument() throws SAXException {
        super.startDocument();
        mAttrList = new ArrayList<ViewAttributes>();
    }

    @Override
    public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
        super.startElement(uri, localName, qName, attributes);
        if(TAG_ATTR_COLOR.equals(localName)) {
            mAttributes = new ViewAttributes();
        }
        mPrefixTAG = localName;
    }

    @Override
    public void characters(char[] ch, int start, int length) throws SAXException {
        super.characters(ch, start, length);
        if(mPrefixTAG != null) {
            String data = new String(ch, start, length);
            if(TAG_ATTR_COLOR_TEXT.equals(mPrefixTAG)) {
                //title: text color; balloon: text color
                mAttributes.appendTextColor(data);
            } else if(TAG_ATTR_TEXT_SIZE.equals(mPrefixTAG)) {
                mAttributes.appendTextSize(data);
            } else if(TAG_ATTR_COLOR_FILL.equals(mPrefixTAG)) {
                //title: null; balloon: panel fill color
                mAttributes.appendPanelFill(data);
            } /*else if(TAG_ATTR_COLOR_GRADIENT_START.equals(mPrefixTAG)) {
                //title: text gradient; balloon: panel gradient
                mAttributes.appendGradientStart(data);
            }*/ else if(TAG_ATTR_COLOR_GRADIENT_END.equals(mPrefixTAG)) {
                mAttributes.appendGradientEnd(data);
            } else if(TAG_ATTR_COLOR_OUTLINE.equals(mPrefixTAG)) {
                //title: outline color; balloon: panel outline color
                mAttributes.appendOutline(data);
            } else if(TAG_ATTR_COLOR_STROKE.equals(mPrefixTAG)) {
                //title: text outline width; balloon: panel outline width
                mAttributes.appendStrokeWidth(data);
            } else if(TAG_ATTR_COLOR_SHADOW.equals(mPrefixTAG)) {
                //title: null; balloon: panel shadow
                mAttributes.appendPanelShadow(data);
            } else if(TAG_ATTR_COLOR_FONT.equals(mPrefixTAG)) {
                //title: text font; balloon: text font
                mAttributes.appendFont(data);
            } else if(TAG_ATTR_LABEL_LABEL.equals(mPrefixTAG)) {
                mAttributes.appendLabel(data);
            } else if(TAG_ATTR_LABEL_HEAD.equals(mPrefixTAG)) {
                mAttributes.appendLabelHead(data);
            } else if(TAG_ATTR_LABEL_TAIL.equals(mPrefixTAG)) {
                mAttributes.appendLabelTail(data);
            }
        }
    }

    @Override
    public void endElement(String uri, String localName, String qName) throws SAXException {
        super.endElement(uri, localName, qName);
        if(TAG_ATTR_COLOR.equals(localName)) {
            mAttrList.add(mAttributes);
            mAttributes = null;
        }
        mPrefixTAG = null;
    }

    @Override
    public void endDocument() throws SAXException {
        super.endDocument();
    }


}
