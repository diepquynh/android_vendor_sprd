/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.compatible;

import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.Size;

import com.ucamera.ucam.modules.compatible.Models;
import com.ucamera.ucam.modules.compatible.ResolutionSize;
import com.ucamera.ucam.modules.utils.LogUtils;
import com.ucamera.ucam.modules.utils.Utils;

public class PreviewSize {

    public static String TAG = "PreviewSize";

    private PreviewSize(int cid) {
    }
    public static PreviewSize instance(int cameraId){
        return new PreviewSize(cameraId);
    }

    public ResolutionSize get(Parameters params) {
        if(params != null){
            /*
             * FIX BUG: 6157
             * BUG COMMENT: avoid null pointer exception
             * DATE: 2014-03-26
             */
            Size previeSize = params.getPreviewSize();
            if(previeSize != null) {
                return new ResolutionSize(previeSize.width, previeSize.height);
            }else {
                return null;
            }
        }
        return null;
    }

    public String getString(Parameters params) {
        ResolutionSize rSize = get(params);
        if(rSize != null) {
            return rSize.toString();
        }
        return null;
    }

    public void set(Parameters params, ResolutionSize value) {
        if(Utils.contains(getSupported(params), value)){
            LogUtils.debug(TAG, "set preview size : %dx%d",value.width,value.height);
            params.setPreviewSize(value.width, value.height);
        }
    }

    public ResolutionSize getDefault(Parameters params) {
        return null;
    }

    public List<ResolutionSize> getSupported(Parameters params) {
        ArrayList<String> entrList = split(params == null ? null:params.get("preview-size-values"));
        if (entrList == null || entrList.size() < 1) {
            entrList = new ArrayList<String>();
            if (params != null) {
                entrList.add(params.get("picture-size"));
            }else {
                return null;
            }
        }

        return Utils.getOrderedSizes(stringToSize(entrList));
    }

    public ResolutionSize getBestFor(Parameters params, ResolutionSize size) {
        if (size == null) return null;

        List<ResolutionSize> supported = getSupported(params);
        if (supported == null) return size;
        if (Utils.contains(supported, size)) return size;

        ResolutionSize target = size;
        int dw = size.getWidth();
        for (ResolutionSize s: supported ) {
            if (size.isSameRatio(s)) {
                int diff = Math.abs(size.getWidth() - s.getWidth());
                if (diff < dw) {
                    target = s;
                    dw = diff;
                }
            }
        }
        return target;
    }

    private ArrayList<String> split(String str) {
        if (str == null) return null;
        StringTokenizer tokenizer = new StringTokenizer(str, ",");
        ArrayList<String> substrings = new ArrayList<String>();
        while (tokenizer.hasMoreElements()) {
            substrings.add(tokenizer.nextToken());
        }
        return substrings;
    }

    private List<ResolutionSize> stringToSize(List<String> list){
        List<ResolutionSize> sizeList = new ArrayList<ResolutionSize>();
        for(String s : list){
            /*
             * FIX BUG : 4972
             * FIX COMMENT: avoid null point exception
             * DATE: 2013-10-11
             */
            if(s == null) continue;
            int xIndex = s.indexOf("x");
            if(xIndex > 0){
                int w = Integer.parseInt(s.substring(0, xIndex));
                int h = Integer.parseInt(s.substring(xIndex+1));
                ResolutionSize size = new ResolutionSize(w,h);
                sizeList.add(size);
            }
        }
        return sizeList;
    }
}
