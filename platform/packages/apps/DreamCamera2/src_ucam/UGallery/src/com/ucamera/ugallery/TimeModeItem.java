/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ugallery;

import com.ucamera.ugallery.gallery.IImage;

import java.util.ArrayList;

public class TimeModeItem {
    private String mDate;
    private String mWeek;
    private int mCount;
    private int mPictureCount;

    private ArrayList<IImage> mCurDateSelectedList;

    public TimeModeItem(String date, String week, int count, int picCount) {
        mDate = date;
        mWeek = week;
        mCount = count;
        mPictureCount = picCount;

        mCurDateSelectedList = new ArrayList<IImage>();
    }

    public void setDate(String date) {
        mDate = date;
    }

    public String getDate() {
        return mDate;
    }

    public void setWeek(String week) {
        mWeek = week;
    }

    public String getWeek() {
        return mWeek;
    }

    public void setCount(int count) {
        mCount = count;
    }

    public void setPictureCount(int picCount) {
        mPictureCount = picCount;
    }

    public void addPictureCount(int picCount) {
        mPictureCount += picCount;
    }

    public int getCount() {
        return mCount;
    }

    public int getPictureCount() {
        return mPictureCount;
    }

    public void updateItemInDateList(IImage image) {
        if(mCurDateSelectedList.contains(image)) {
            mCurDateSelectedList.remove(image);
        } else {
            mCurDateSelectedList.add(image);
        }
    }

    public void removeItemFromDateList(IImage image) {
        if(mCurDateSelectedList.contains(image)) {
            mCurDateSelectedList.remove(image);
        }
    }

    public void addItemToDateList(IImage image) {
        if(!mCurDateSelectedList.contains(image)) {
            mCurDateSelectedList.add(image);
        }
    }

    public ArrayList<IImage> getCurDateSelectedList() {
        return mCurDateSelectedList;
    }

    public int getCurDateSelectedNum() {
        return mCurDateSelectedList.size();
    }
}
