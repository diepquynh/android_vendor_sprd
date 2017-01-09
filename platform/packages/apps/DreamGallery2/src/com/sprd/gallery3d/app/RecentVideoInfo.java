/**
 * Created By Spreadst
 * */

package com.sprd.gallery3d.app;

public class RecentVideoInfo{
    private int id;
    private String title;
    private String uri;
    private String bookmark;
    private String duration;
    private String date;

    public RecentVideoInfo(int id, String title, String uri, String bookmark, String duration,
            String date) {
        super();
        this.id = id;
        this.title = title;
        this.uri = uri;
        this.bookmark = bookmark;
        this.duration = duration;
        this.date = date;
    }
    public int getId() {
        return id;
    }
    public void setId(int id) {
        this.id = id;
    }
    public String getTitle() {
        return title;
    }
    public void setTitle(String title) {
        this.title = title;
    }
    public String getUri() {
        return uri;
    }
    public void setUri(String uri) {
        this.uri = uri;
    }
    public String getBookmark() {
        return bookmark;
    }
    public void setBookmark(String bookmark) {
        this.bookmark = bookmark;
    }
    public String getDuration() {
        return duration;
    }
    public void setDuration(String duration) {
        this.duration = duration;
    }
    public String getDate() {
        return date;
    }
    public void setDate(String date) {
        this.date = date;
    }
    @Override
    public String toString() {
        return "RecentVideoInfo [id=" + id + ", title=" + title + ", uri=" + uri + ", bookmark="
                + bookmark + ", duration=" + duration + ", date=" + date + "]";
    }
}
