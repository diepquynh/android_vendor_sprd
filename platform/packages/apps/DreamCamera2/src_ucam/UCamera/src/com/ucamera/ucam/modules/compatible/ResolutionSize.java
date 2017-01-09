/*
 * Copyright (C) 2010-2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.compatible;

import java.util.List;

import com.ucamera.ucam.modules.utils.Utils;

public class ResolutionSize implements Comparable<ResolutionSize> {
    public int width;
    public int height;
    public double mRatio = 0.0;

    public ResolutionSize(int w,int h){
        width  = w;
        height = h;
        mRatio = calcRatio();
    }

    public ResolutionSize(android.hardware.Camera.Size size){
        /*
         * FIX BUG : 4625
         * BUG COMMENT : avoid null point exception
         * DATE : 2013-07-30
         */
        if(size != null) {
            width  = size.width;
            height = size.height;
            mRatio = calcRatio();
        }
    }

    public ResolutionSize(String size){
        /*
         * FIX BUG : 6037
         * BUG COMMENT : avoid null point exception
         * DATE : 2014-03-04
         */
        int index = size != null ? size.indexOf("x") : -1;
        if(index != -1) {
            width  = Utils.parseIntSafely(size.substring(0, index),0);
            height = Utils.parseIntSafely(size.substring(index+1),0);
            mRatio = calcRatio();
        }
    }


    public boolean isSameRatio(ResolutionSize o) {
        if (o == null) return false;
        return Math.abs(mRatio - o.mRatio) < getMaxRatioDelta();
    }

    private double calcRatio() {
        double ratio;
        if(width > height){
            ratio = (double)width / (double)height;
        }else{
            ratio = (double)height / (double)width;
        }
        return ratio;
    }

    public int getWidth(){
        return width;
    }

    public int getHeight(){
        return height;
    }

    public String toString(){
        return String.format("%dx%d", width,height);
    }

    public double getMaxRatioDelta(){
        if(width > height){
            return 0.035;
        } else {
            return 0.125;
        }
    }

    public String toDisplayString() {
        return String.format("%3s %9s %6s", getSizeString(), toString(), getRatioString());
    }

    public boolean is16x9() {
        if (width == 0 || height == 0) return false;
        return Math.abs(width/16.0 - height/9.0) < 0.01;
    }

    private String getRatioString() {
        return is16x9() ? "(16x9)" : "";
    }

    public String getSizeString() {
        float s = (width * height) / 10000.0f;
        if (s < 10) {
           return  String.format("%.0fK", s*10);
        } else if (s < 95) {
            return  String.format("%.1fM", s/100);
        } else {
                return  String.format("%.0fM", s/100);
        }
    }

    public String toVideoDisplayString() {
        return toString();
    }

    public String getVideoSizeString() {
        if (width == 176 && height == 144 ) return "QCIF";
        if (width == 640 && height == 480 ) return "VGA";
        if (width == 1280 && height == 720 ) return "720P";
        if (width == 1920 && (height == 1080 || height == 1088)) return "1080P";
        return getSizeString();
    }

    @Override
    public int compareTo(ResolutionSize o) {
        int diff = width * height - o.width * o.height;
        if (diff > 0) return 1;
        if (diff < 0) return -1;

        diff = width - o.width;
        if (diff > 0) return 1;
        if (diff < 0) return -1;
        return 0;
    }

    public boolean containOf(List<ResolutionSize> list) {
        for(ResolutionSize size : list) {
            if(this.width == size.width && this.height == size.height){
                return true;
            }
        }
        return false;

    }
}
