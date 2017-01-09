/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.downloadcenter;

import java.util.ArrayList;

/**
 * thumbnails mode
 */
public class ThumbnailsMode {
    //thumbnails action
    private String thumbnailsAction;
    //thumbnails type
    private StringBuffer thumbnailsType;
    //thumbnails group id
    private StringBuffer thumbnailsGroupId;
    //thumbnails density
    private StringBuffer thumbnailsDensity;
    //thumbnails mode list
    private ArrayList<ThumbnailMode> thumbnailsModeList;

    public ThumbnailsMode() {
        thumbnailsType = new StringBuffer();
        thumbnailsGroupId = new StringBuffer();
        thumbnailsDensity = new StringBuffer();
    }

    public String getThumbnailsAction() {
        return thumbnailsAction.toString();
    }

    public void setThumbnailsAction(String thumbnailsAction) {
        this.thumbnailsAction = thumbnailsAction;
    }

    public String getThumbnailsType() {
        return thumbnailsType.toString();
    }

    public void appendThumbnailsType(String thumbnailsType) {
        this.thumbnailsType.append(thumbnailsType);
    }

    public String getThumbnailsGroupId() {
        return thumbnailsGroupId.toString();
    }

    public void appendThumbnailsGroupId(String thumbnailsGroupId) {
        this.thumbnailsGroupId.append(thumbnailsGroupId);
    }

    public String getThumbnailsDensity() {
        return thumbnailsDensity.toString();
    }

    public void appendThumbnailsDensity(String thumbnailsDensity) {
        this.thumbnailsDensity.append(thumbnailsDensity);
    }

    public ArrayList<ThumbnailMode> getThumbnailsModeList() {
        return thumbnailsModeList;
    }

    public void setThumbnailsModeList(ArrayList<ThumbnailMode> thumbnailsModeList) {
        this.thumbnailsModeList = thumbnailsModeList;
    }
}
