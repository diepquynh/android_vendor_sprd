/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.downloadcenter;

import java.util.ArrayList;
import java.util.Collections;

/**
 * groups mode
 */
public class GroupsMode {
    //group action contains list-groups
    private String groupAction;
    //type contains frame(1) and decor(2)
    private StringBuffer groupType;
    //mode list
    private ArrayList<GroupMode> modeList;

    public GroupsMode() {
        groupType = new StringBuffer();
    }

    public String getGroupAction() {
        return groupAction;
    }

    public void setGroupAction(String groupAction) {
        this.groupAction = groupAction;
    }

    public String getGroupType() {
        return groupType.toString();
    }

    public void appendGroupType(String groupType) {
        this.groupType.append(groupType);
    }

    public ArrayList<GroupMode> getModeList() {
        return modeList;
    }

    public void setModeList(ArrayList<GroupMode> modeList) {
        this.modeList = modeList;
        if (this.modeList != null) {
            Collections.sort(this.modeList);
        }
    }
}
