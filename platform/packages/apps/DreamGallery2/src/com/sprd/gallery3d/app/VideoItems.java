/**
 * Created By Spreadst
 * */

package com.sprd.gallery3d.app;

public class VideoItems {
    private int id;
    private String displayName;
    private String duration;
    private String date_modified;
    private long width;
    private long height;
    private long size;
    private String url;

    public VideoItems(int id, String displayName, String duration,
            String date_modified, long width, long height, long size, String url) {
        super();
        this.id = id;
        this.displayName = displayName;
        this.duration = duration;
        this.date_modified = date_modified;
        this.width = width;
        this.height = height;
        this.size = size;
        this.url = url;
    }

    public String getDisplayName() {
        return displayName;
    }

    public void setDisplayName(String displayName) {
        this.displayName = displayName;
    }

    public String getDuration() {
        return duration;
    }

    public void setDuration(String duration) {
        this.duration = duration;
    }

    public String getDate_modified() {
        return date_modified;
    }

    public void setDate_modified(String date_modified) {
        this.date_modified = date_modified;
    }

    public long getWidth() {
        return width;
    }

    public void setWidth(long width) {
        this.width = width;
    }

    public long getHeight() {
        return height;
    }

    public void setHeight(long height) {
        this.height = height;
    }

    public long getSize() {
        return size;
    }

    public void setSize(long size) {
        this.size = size;
    }

    public String getUrl() {
        return url;
    }

    public void setUrl(String url) {
        this.url = url;
    }

    public int getId() {
        return id;
    }

    public void setId(int id) {
        this.id = id;
    }
}
