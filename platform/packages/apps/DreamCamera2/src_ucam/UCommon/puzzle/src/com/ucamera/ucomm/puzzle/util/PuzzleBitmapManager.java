/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.util;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import android.graphics.Bitmap;

import com.ucamera.ucomm.puzzle.PuzzleUtils;
import com.ucamera.ucomm.puzzle.free.FreePuzzleView;

public class PuzzleBitmapManager {
    private Map<Integer, Bitmap> mPuzzleBitmapMaps = new HashMap<Integer, Bitmap>();
    private PuzzleBitmapManager() {}
    private static PuzzleBitmapManager mInstance;
    public static PuzzleBitmapManager getInstance() {
        if(mInstance == null) {
            mInstance = new PuzzleBitmapManager();
        }
        return mInstance;
    }
    public void addBitmap(int index, Bitmap bitmap) {
        mPuzzleBitmapMaps.put(index, bitmap);
    }
    public boolean containsBitmap(int index) {
        return mPuzzleBitmapMaps.containsKey(index);
    }
    public Bitmap getBitmap(int index) {
        return mPuzzleBitmapMaps.get(index);
    }
    public void updateBitmap(int index, Bitmap bitmap) {
        if(mPuzzleBitmapMaps != null && mPuzzleBitmapMaps.containsKey(index)) {
            if(!mPuzzleBitmapMaps.get(index).isRecycled()) {
                PuzzleUtils.recyleBitmap(mPuzzleBitmapMaps.get(index));
            }
            mPuzzleBitmapMaps.remove(index);
            mPuzzleBitmapMaps.put(index, bitmap);
        }
    }
    public void swapBmp(int x1, int x2) {
        if(mPuzzleBitmapMaps != null) {
            Bitmap bmp = mPuzzleBitmapMaps.get(x1);
            mPuzzleBitmapMaps.remove(x1);
            mPuzzleBitmapMaps.put(x1, mPuzzleBitmapMaps.get(x2));
            mPuzzleBitmapMaps.remove(x2);
            mPuzzleBitmapMaps.put(x2, bmp);
        }
    }
    public void recycleUnusedBms(ArrayList<Integer> unusedNums) {
        for(Integer uri : unusedNums) {
            if(mPuzzleBitmapMaps.containsKey(uri)) {
                PuzzleUtils.recyleBitmap(mPuzzleBitmapMaps.get(uri));
                try {
                    mPuzzleBitmapMaps.remove(uri);
                } catch(UnsupportedOperationException e){}
            }
        }
    }
    public void recycleBm() {
        if(mPuzzleBitmapMaps != null) {
            for(Bitmap bmp : mPuzzleBitmapMaps.values()) {
                if(bmp != null && !bmp.isRecycled()) {
                    bmp.recycle();
                }
            }
            mPuzzleBitmapMaps.clear();
        }
    }
    public void clear() {
        mPuzzleBitmapMaps.clear();
    }
}
