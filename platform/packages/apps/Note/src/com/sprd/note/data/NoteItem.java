
package com.sprd.note.data;

import java.util.Date;

import android.content.Context;
import android.text.format.DateFormat;
import com.sprd.note.R;
import com.sprd.note.utils.StringUtils;

public class NoteItem {
    public static final int MAX_NUM = 14;
    public static final int TITLE_ONE_LINE_MAX_NUM = 7;

    public int _id;
    public String content;
    public long date;
    public boolean isFileFolder;
    public int parentFolderId;

    public String title;

    // used in delete mode or moveable mode.
    public boolean isSelected = false;

    public NoteItem(String content, long date, boolean isFileFolder, int parentFolder) {
        this.content = content;
        this.date = date;
        this.isFileFolder = isFileFolder;
        this.parentFolderId = parentFolder;
        this.title = "";

    }

    public NoteItem(int parentFolder) {
        this("", new Date().getTime(),false, parentFolder);
    }

    public NoteItem() {
        this(-1);// no parent
    }

    public String getContent() {
        return content == null ? "" : content;
    }
    
    public String getTitle() {
        return title == null ? "" : title;
    }

    public String getShortTitle() {
        if (title == null || title.equals("")) {
            return getShortContext();
        }
        return title;
    }

    public String getShortContext() {
        if (getContent().length() > MAX_NUM) {
            return getContent().substring(0, MAX_NUM - 1) + "...";
        } else {
            return getContent();
        }
    }

    public String getShowTitle() {
        if (title == null || title.equals("")) {
            return StringUtils.replaceBlank(getContent());
        }
        return StringUtils.replaceBlank(title);
    }

    public String getDate(Context context) {
        java.text.DateFormat dateFormat = DateFormat.getDateFormat(context);
        String cDate = dateFormat.format(new java.util.Date(date));
        return cDate;
    }

    public String getTime(Context context) {
        java.text.DateFormat timeFormat = DateFormat.getTimeFormat(context);//is24HourFormat, "h:mm a"
        String cTime = timeFormat.format(new java.util.Date(date));
        return cTime;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + _id;
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        NoteItem other = (NoteItem) obj;
        if (_id != other._id)
            return false;
        return true;
    }

    public String getShareTitle(Context context) {//share
        if (title == null || title.equals("")) {
            return context.getString(R.string.export_no_content);
        }
        return title;
    }

    public String getShareContent(Context context) {
        if (content == null || content.equals("")) {
            return context.getString(R.string.export_no_content);
        }
        return content;
    }

    @Override
    public String toString() {
        return "NoteItem [_id=" + _id + ", content=" + content + "]";
    }
}
