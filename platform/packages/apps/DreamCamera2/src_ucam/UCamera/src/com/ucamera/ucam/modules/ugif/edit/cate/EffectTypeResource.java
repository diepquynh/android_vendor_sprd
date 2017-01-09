/*
 *   Copyright (C) 2010,2012 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ucam.modules.ugif.edit.cate;

import java.util.ArrayList;
import java.util.HashMap;

public class EffectTypeResource {
    private HashMap<Integer, ArrayList<AdapterItem>> mHashMap = new HashMap<Integer, ArrayList<AdapterItem>>();
    private static EffectTypeResource mInstance;

    public static final int EFFECT_LOMO          = 0;
    public static final int EFFECT_HDR           = 1;
    public static final int EFFECT_SKIN          = 2;
    public static final int EFFECT_VIVID_LIGHT   = 3;
    public static final int EFFECT_SKETCH        = 4;
    public static final int EFFECT_COLORFULL     = 5;
    public static final int EFFECT_FUNNY         = 6;
    public static final int EFFECT_NOSTALGIA     = 7;
    public static final int EFFECT_BLACKWHITE    = 8;
    public static final int EFFECT_DEFORM        = 9;

    public static EffectTypeResource getInstance() {
        if(mInstance == null) {
            mInstance = new EffectTypeResource();
        }

        return mInstance;
    }

    public ArrayList<AdapterItem> getEffectItem(int key) {
        return mHashMap.get(key);
    }

    private EffectTypeResource() {
        initResource();
    }

    private void initResource() {
        //LOMO
        ArrayList<AdapterItem> arrayList = getLomo();
        mHashMap.put(EFFECT_LOMO, arrayList);

        //HDR
        arrayList = getHDR();
        mHashMap.put(EFFECT_HDR, arrayList);

        //SKIN
        arrayList = getSkin();
        mHashMap.put(EFFECT_SKIN, arrayList);

        //VIVID LIGHT
        arrayList = getVividLight();
        mHashMap.put(EFFECT_VIVID_LIGHT, arrayList);

        //SKETCH
        arrayList = getSketch();
        mHashMap.put(EFFECT_SKETCH, arrayList);

        //COLOR FULL
        arrayList = getColorFull();
        mHashMap.put(EFFECT_COLORFULL, arrayList);

        //FUNNY
        arrayList = getFunny();
        mHashMap.put(EFFECT_FUNNY, arrayList);

        //NOSTALGIA
        arrayList = getNostalgia();
        mHashMap.put(EFFECT_NOSTALGIA, arrayList);

        //BLACKWHITE
        arrayList = getBlackWhite();
        mHashMap.put(EFFECT_BLACKWHITE, arrayList);

        //DEFORM
        arrayList = getDeform();
        mHashMap.put(EFFECT_DEFORM, arrayList);
    }

    private ArrayList<AdapterItem> getLomo() {
        //0, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150
        ArrayList<AdapterItem> arrayList = new ArrayList<AdapterItem>();
        int[] effects = {0, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150};
        for(int index = 0; index < effects.length; index++) {
            arrayList.add(new AdapterItem(effects[index], "LOMO" + (index + 1)));
        }

        return arrayList;
    }

    private ArrayList<AdapterItem> getHDR() {
        //1, 16, 210, 211, 212, 213, 214
        ArrayList<AdapterItem> arrayList = new ArrayList<AdapterItem>();
        int[] effects = {1, 16, 210, 211, 212, 213, 214};
        for(int index = 0; index < effects.length; index++) {
            arrayList.add(new AdapterItem(effects[index], "HDR" + (index + 1)));
        }

        return arrayList;
    }

    private ArrayList<AdapterItem> getSkin() {
        //200, 201, 202, 203, 204, 205
        ArrayList<AdapterItem> arrayList = new ArrayList<AdapterItem>();
        int[] effects = {200, 201, 202, 203, 204, 205};
        for(int index = 0; index < effects.length; index++) {
            arrayList.add(new AdapterItem(effects[index], "Skin" + (index + 1)));
        }

        return arrayList;
    }

    private ArrayList<AdapterItem> getVividLight() {
        //180, 181, 182, 183, 184, 185, 186, 187
        ArrayList<AdapterItem> arrayList = new ArrayList<AdapterItem>();
        int[] effects = {180, 181, 182, 183, 184, 185, 186, 187};
        for(int index = 0; index < effects.length; index++) {
            arrayList.add(new AdapterItem(effects[index], "Light" + (index + 1)));
        }

        return arrayList;
    }

    private ArrayList<AdapterItem> getSketch() {
        //62, 61, 60, 7, 30, 31, 32, 33, 34, 35, 36, 37
        ArrayList<AdapterItem> arrayList = new ArrayList<AdapterItem>();
        int[] effects = {62, 61, 60, 30, 31, 32, 33, 34, 35, 36, 37};
        for(int index = 0; index < effects.length; index++) {
            arrayList.add(new AdapterItem(effects[index], "Sketch" + (index + 1)));
        }

        return arrayList;
    }

    private ArrayList<AdapterItem> getColorFull() {
        //5, 6, 22, 23, 24, 25, 104, 105, 106, 107, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130
        ArrayList<AdapterItem> arrayList = new ArrayList<AdapterItem>();
        int[] effects = {5, 6, 22, 23, 24, 25, 104, 105, 106, 107, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130};
        for(int index = 0; index < effects.length; index++) {
            arrayList.add(new AdapterItem(effects[index], "Color" + (index + 1)));
        }

        return arrayList;
    }

    private ArrayList<AdapterItem> getFunny() {
        //3, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21
        ArrayList<AdapterItem> arrayList = new ArrayList<AdapterItem>();
        int[] effects = {3, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21};
        for(int index = 0; index < effects.length; index++) {
            arrayList.add(new AdapterItem(effects[index], "Funny" + (index + 1)));
        }

        return arrayList;
    }

    private ArrayList<AdapterItem> getNostalgia() {
        //50, 51, 52, 53, 54, 55, 56, 57
        ArrayList<AdapterItem> arrayList = new ArrayList<AdapterItem>();
        int[] effects = {50, 51, 52, 53, 54, 55, 56, 57};
        for(int index = 0; index < effects.length; index++) {
            arrayList.add(new AdapterItem(effects[index], "Nostalgia" + (index + 1)));
        }

        return arrayList;
    }

    private ArrayList<AdapterItem> getBlackWhite() {
        //4, 2, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170
        ArrayList<AdapterItem> arrayList = new ArrayList<AdapterItem>();
        int[] effects = {4, 2, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170};
        for(int index = 0; index < effects.length; index++) {
            arrayList.add(new AdapterItem(effects[index], "BlackWhite" + (index + 1)));
        }

        return arrayList;
    }

    private ArrayList<AdapterItem> getDeform() {
        //19, 40, 41, 42, 43, 44, 100, 101, 102, 103
        ArrayList<AdapterItem> arrayList = new ArrayList<AdapterItem>();
        int[] effects = {19, 40, 41, 42, 43, 44, 100, 101, 102, 103};
        for(int index = 0; index < effects.length; index++) {
            arrayList.add(new AdapterItem(effects[index], "Deform" + (index + 1)));
        }

        return arrayList;
    }
}
