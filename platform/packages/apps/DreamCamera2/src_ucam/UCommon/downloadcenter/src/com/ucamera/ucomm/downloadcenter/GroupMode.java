/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.downloadcenter;

/**
 * group mode
 */
public class GroupMode implements Comparable<GroupMode>{
    //group id
    private StringBuffer groupId;
    //group name
    private StringBuffer groupName;

    private StringBuffer displayOrder;

    public GroupMode() {
        groupId = new StringBuffer();
        groupName = new StringBuffer();
        displayOrder = new StringBuffer();
    }

    public String getGroupId() {
        return groupId.toString();
    }

    public void appendGroupId(String groupId) {
        this.groupId.append(groupId);
    }

    public String getGroupName() {
        return groupName.toString();
    }

    public void appendGroupName(String groupName) {
        this.groupName.append(groupName);
    }

    public void appendDisplayOrder(String order) {
        this.displayOrder.append(order);
    }

    public String getDisplayOrder() {
        return this.displayOrder.toString();
    }

    @Override
    public int compareTo(GroupMode another) {
        if (another == null)
            return 1;
        int compare = getDisplayOrder().compareTo(another.getDisplayOrder());
        if (compare != 0) {
            return compare;
        }

        compare = getGroupName().compareTo(another.getGroupName());
        if (compare != 0) {
            return compare;
        }

        return 0;
    }
}
