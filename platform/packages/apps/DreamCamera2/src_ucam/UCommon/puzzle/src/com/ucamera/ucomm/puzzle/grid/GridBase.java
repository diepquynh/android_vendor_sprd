/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.grid;

import android.graphics.Matrix;
import android.util.Log;

import com.ucamera.ucomm.puzzle.Puzzle;

import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Random;

public abstract class GridBase extends Puzzle {
    private HashMap<Integer, Method> mHashMap = null;
    protected void randomRotate(float[] points, int x, int y){
        final int r = new Random().nextInt(4);
        Matrix m = new Matrix();
        m.setRotate(r * 90);
        switch (r){
            case 0: break;
            case 1: m.postTranslate(y,0); break;
            case 2: m.postTranslate(x,y); break;
            case 3: m.postTranslate(0,x); break;
        }
        m.mapPoints(points);
    }

    protected void randomMirror(float[] points, int x, int y) {
        Matrix m = new Matrix();
        final int random = new Random().nextInt(3);
        switch (random){
            case 0: break;
            case 1: {
                m.setScale(-1.0f, 1.0f);
                m.postTranslate(x, 0);
                break;
            }
            case 2:{
                m.setScale(1.0f, -1.0f);
                m.postTranslate(0, y);
                break;
            }
        }
        m.mapPoints(points);
    }

    protected void setupSpec(float[] points){
        mSpec.reset();
        for ( int i=0; i < mSpec.length(); i++ ){
            mSpec.set(i, (int)Math.min(points[4*i  ],points[4*i+2]),
                         (int)Math.min(points[4*i+1],points[4*i+3]),
                         (int)Math.max(points[4*i  ],points[4*i+2]),
                         (int)Math.max(points[4*i+1],points[4*i+3]));
        }
    }

    @Override
    public Puzzle random(int index) {
        if (mHashMap == null) {
            mHashMap = new HashMap<Integer, Method>();
            for(Method m: getClass().getDeclaredMethods()){
                PuzzleMethod pm = m.getAnnotation(PuzzleMethod.class);
                if (pm != null && pm.value() == getType() && pm.weight() != -1){
                    mHashMap.put(pm.weight(), m);
                }
            }
        }

        if (index >= mHashMap.size() || index < 0) {
            return super.random(index);
        }

        try{
            mHashMap.get(index).invoke(this);
        }catch (Exception e) {
            Log.w("GridBase","Error call puzzle method, try to next one!");
            super.random(index);
        }

        return this;
    }
}
