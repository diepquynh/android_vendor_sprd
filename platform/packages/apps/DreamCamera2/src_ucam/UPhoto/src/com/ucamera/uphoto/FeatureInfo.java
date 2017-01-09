/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.graphics.Point;

public class FeatureInfo implements Cloneable{
    public int mMode;
    public int mIntensity;

    //only for deblemish
    private MakeupDeblemish[] mMakeupDeblemishs = null;
    private int deblemish_num = 0;

    public FeatureInfo() {

    }

    public FeatureInfo(int mode) {
        super();
        mMode = mode;
        mMakeupDeblemishs = new MakeupDeblemish[MakeupEngine.MAX_DEBLEMISH_AREA];
        for (int i = 0; i < mMakeupDeblemishs.length; i++) {
            mMakeupDeblemishs[i] = new MakeupDeblemish();
        }
        deblemish_num = 0;
        mIntensity = 0;
    }

    // MakeupEngine.ManageImgae2 will call it from native.
    public int GetIntensity(){
        switch(mMode) {
            case ImageEditConstants.MAKEUP_MODE_SOFTENFACE:
                mIntensity = mSoftenFace;
                break;
            case ImageEditConstants.MAKEUP_MODE_WHITENFACE:
                mIntensity = mWhitenFace;
                break;
            case ImageEditConstants.MAKEUP_MODE_DEBLEMISH:
                mIntensity = mDeblemish;
                break;
            case ImageEditConstants.MAKEUP_MODE_TRIMFACE:
                mIntensity = mTrimFace;
                break;
            case ImageEditConstants.MAKEUP_MODE_BIGEYE:
                mIntensity = mBigEye;
                break;
            default:
                mIntensity = 0;
        }
        return mIntensity;
    }

    public int GetMod(){
        return mMode;
    }

    public void setIntensity(int nInten){
        mIntensity = nInten;
    }

    public boolean setArea(int x, int y){
        if(mMode != ImageEditConstants.MAKEUP_MODE_DEBLEMISH || deblemish_num >= MakeupEngine.MAX_DEBLEMISH_AREA)
            return false;
        mMakeupDeblemishs[deblemish_num].center.x = x;
        mMakeupDeblemishs[deblemish_num].center.y = y;
        deblemish_num++;

        return true;
    }

//    public void setMakeupDeblemishs(MakeupDeblemish[] makeupDeblemish, int deblemishNum) {
//        mMakeupDeblemishs = makeupDeblemish.clone();
//        deblemish_num = deblemishNum;
//    }

    public int GetDeblemishNum() {
        return deblemish_num;
    }

    public Point GetDeblemishArea(int i) {
        if(i<deblemish_num)
            return mMakeupDeblemishs[i].center;
        else
            return null;
    }

    public MakeupDeblemish[] getMakeupDeblemish() {
        return mMakeupDeblemishs;
    }

    public void ReSetDeblemishNum() {
        deblemish_num = 0;
    }

    public void ReSetAllButDeblemish() {
        mSoftenFace = 0;
        mWhitenFace = 0;
        mTrimFace = 0;
        mBigEye = 0;
    }

    public int GetDeblemishRadius(int i) {
        if (mMakeupDeblemishs[i].radius == 0)
            return 6;
        return mMakeupDeblemishs[i].radius;
    }

    public void SetDeblemishRadius(int index, int radius) {
        if(index < MakeupEngine.MAX_DEBLEMISH_AREA) {
            /* FIX BUG : 5848 6203
             * BUG COMMENT : nullpointer
             * DATE : 2014-04-10
             */
            if(mMakeupDeblemishs != null && mMakeupDeblemishs[index] != null) {
                mMakeupDeblemishs[index].radius = radius;
            }
        }
    }

    public class MakeupDeblemish {
        public Point center = new Point();
        public int radius = 0;
    }

    ////////////////////////////////////////////////////////////////////
    public MakeupDeblemish[] getMakeupDeblemishs() {
        return mMakeupDeblemishs;
    }

    public void setMakeupDeblemishs(MakeupDeblemish[] makeupDeblemishs, int debNum) {
        this.mMakeupDeblemishs = makeupDeblemishs;
        mDebNum = debNum;
    }

    public int getSoftenFace() {
        return mSoftenFace;
    }

    public void setSoftenFace(int softenFace) {
        this.mSoftenFace = softenFace;
    }

    public int getWhitenFace() {
        return mWhitenFace;
    }

    public void setWhitenFace(int whitenFace) {
        this.mWhitenFace = whitenFace;
    }

    public int getDeblemish() {
        return mDeblemish;
    }

    public void setDeblemish(int deblemish) {
        this.mDeblemish = deblemish;
    }

    public int getTrimFace() {
        return mTrimFace;
    }

    public void setTrimFace(int trimFace) {
        this.mTrimFace = trimFace;
    }

    public int getBigEye() {
        return mBigEye;
    }

    public void setBigEye(int bigEye) {
        this.mBigEye = bigEye;
    }

    public int getDebNum() {
        return mDebNum;
    }

    public float getDebX() {
        return mMakeupDeblemishs[deblemish_num].center.x;
    }

    public float getDebY() {
        return mMakeupDeblemishs[deblemish_num].center.y;
    }


    public void setDebNum() {
        this.deblemish_num++;
    }

    public void setDebX(float pointX) {

    }

    public void setDebY(float pointY) {

    }

    public void setMode(int mode) {
        mMode = mode;
    }


    private int mSoftenFace = 0;
    private int mWhitenFace = 0;
    private int mDeblemish = 0;
    private int mTrimFace = 0;
    private int mBigEye = 0;
    private int mDebNum = 0;

    public FeatureInfo clone() {
        FeatureInfo newItem = null;
        try {
            newItem = (FeatureInfo) super.clone();
        } catch (CloneNotSupportedException e) {
            e.printStackTrace();
        }

        return newItem;
    }

}
