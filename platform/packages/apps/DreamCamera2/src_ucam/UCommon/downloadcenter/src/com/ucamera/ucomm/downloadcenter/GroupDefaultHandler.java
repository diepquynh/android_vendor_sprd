/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.downloadcenter;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import java.util.ArrayList;

/**
 * parser xml by sax
 */
public class GroupDefaultHandler extends DefaultHandler {
    //group list
    private ArrayList<GroupMode> mGroupList;
    //groups mode
    private GroupsMode mGroupsMode;
    //group mode
    private GroupMode mGroupMode;
    //prefix tag
    private String mPrefixTAG;
    private static final String TAG_LIST_GROUP = "list-groups";
    private static final String TAG_GROUPS = "groups";
    private static final String TAG_GROUPS_TYPE = "type";
    private static final String TAG_GROUP_GROUP = "group";
    private static final String TAG_GROUP_ID = "id";
    private static final String TAG_GROUP_NAME = "name";
    private static final String TAG_GROUP_ORDER = "order";

    public GroupsMode getGroupsMode() {
        return mGroupsMode;
    }

    /**
     * start to parser document
     */
    public void startDocument() throws SAXException {
        super.startDocument();
        mGroupList = new ArrayList<GroupMode>();
    }

    /**
     * start to parser node element
     */
    public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
        super.startElement(uri, localName, qName, attributes);
        if(TAG_LIST_GROUP.equals(localName)) {
            mGroupsMode = new GroupsMode();
            mGroupsMode.setGroupAction(localName);
        }
        if(TAG_GROUP_GROUP.equals(localName)) {
            mGroupMode = new GroupMode();
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
            if(TAG_GROUPS_TYPE.equals(mPrefixTAG)) {
                mGroupsMode.appendGroupType(data);
            } else if(TAG_GROUP_ID.equals(mPrefixTAG)) {
                mGroupMode.appendGroupId(data);
            } else if(TAG_GROUP_NAME.equals(mPrefixTAG)) {
                mGroupMode.appendGroupName(data);
            } else if (TAG_GROUP_ORDER.equals(mPrefixTAG)) {
                mGroupMode.appendDisplayOrder(data);
            }
        }
    }

    /**
     * end to parser node element
     */
    public void endElement(String uri, String localName, String qName) throws SAXException {
        super.endElement(uri, localName, qName);
        if(TAG_GROUPS.equals(localName)) {
            mGroupsMode.setModeList(mGroupList);
        } else if(TAG_GROUP_GROUP.equals(localName)) {
            mGroupList.add(mGroupMode);
            mGroupMode = null;
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
