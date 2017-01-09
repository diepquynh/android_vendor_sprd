/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.downloadcenter;

/**
 * thumbnail mode
 */
public class ThumbnailMode {
    //thumbnail id
    private StringBuffer thumbnailId;
    //thumbnail name
    private StringBuffer thumbnailName;
    //thunmbnail url
    private StringBuffer thumnailUrl;
    //download url
    private StringBuffer downloadUrl;

    public ThumbnailMode() {
        thumbnailId = new StringBuffer();
        thumbnailName = new StringBuffer();
        thumnailUrl = new StringBuffer();
        downloadUrl = new StringBuffer();
    }

    public String getThumbnailId() {
        return thumbnailId.toString();
    }

    public void appendThumbnailId(String thumbnailId) {
        this.thumbnailId.append(thumbnailId);
    }

    public String getThumbnailName() {
        return thumbnailName.toString();
    }

    public void appendThumbnailName(String thumbnailName) {
        this.thumbnailName.append(thumbnailName);
    }

    public String getThumnailUrl() {
        return thumnailUrl.toString();
    }

    public void appendThumnailUrl(String thumnailUrl) {
        this.thumnailUrl.append(thumnailUrl);
    }

    public String getDownloadUrl() {
        return downloadUrl.toString();
    }

    public void appendDownloadUrl(String downloadUrl) {
        this.downloadUrl.append(downloadUrl);
    }
}
