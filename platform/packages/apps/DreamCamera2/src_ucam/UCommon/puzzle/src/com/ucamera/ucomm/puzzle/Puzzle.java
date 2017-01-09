/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucomm.puzzle;

import android.util.Log;

import com.ucamera.ucomm.puzzle.free.*;
import com.ucamera.ucomm.puzzle.grid.*;
import com.ucamera.ucomm.puzzle.stitch.StitchPuzzle;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Random;

public abstract class Puzzle {
    private static final String TAG = "Puzzle";
    private ArrayList<Method> mPuzzleMethods = new ArrayList<Method>();

    public static final Puzzle EMPTY_PUZZLE = new Puzzle(){
        public PuzzleSpec getSpec() {
            if (mSpec == null) {
                mSpec = PuzzleSpec.create(0);
            }
            return mSpec;
        }
    };

    protected PuzzleSpec mSpec;
    private   Type       mType;
    private   int mCurrentIndex = -1;

    public static Puzzle create(Type type, int num) {
        if (type == Type.STITCH){
            return new StitchPuzzle(num);
        }

        switch (num){
            case 2: return ((type == Type.GRID) ? new GridPuzzle2() : new FreePuzzle2()).init(type);
            case 3: return ((type == Type.GRID) ? new GridPuzzle3() : new FreePuzzle3()).init(type);
            case 4: return ((type == Type.GRID) ? new GridPuzzle4() : new FreePuzzle4()).init(type);
            case 5: return ((type == Type.GRID) ? new GridPuzzle5() : new FreePuzzle5()).init(type);
            case 6: return ((type == Type.GRID) ? new GridPuzzle6() : new FreePuzzle6()).init(type);
            case 7: return ((type == Type.GRID) ? new GridPuzzle7() : new FreePuzzle7()).init(type);
            case 8: return ((type == Type.GRID) ? new GridPuzzle8() : new FreePuzzle8()).init(type);
            case 9: return ((type == Type.GRID) ? new GridPuzzle9() : new FreePuzzle9()).init(type);
            default: throw new UnsupportedOperationException("Puzzle" + num + " is not implemented.");
        }
    }
    public int getMethodCount() {
        return mPuzzleMethods.size();
    }

    private void initMethods() {
        for(Method m: getClass().getDeclaredMethods()){
            PuzzleMethod pm = m.getAnnotation(PuzzleMethod.class);
            if (pm != null && pm.value() == getType()){
                mPuzzleMethods.add(m);
            }
        }
    }

    public Puzzle getPuzzleIndex(int index) {
        int total = mPuzzleMethods.size();
        if (total > 0){
            try{
                mPuzzleMethods.get(index).invoke(this);
            }catch (Exception e) {
                Log.w(TAG,"Error call puzzle method, try to next one!");
            }
        } else {
            Log.w(TAG,"No puzzle method defined");
        }
        return this;
    }

    public Puzzle random(int index){
        ArrayList<Method> puzzleMethods = new ArrayList<Method>();
        for(Method m: getClass().getDeclaredMethods()){
            PuzzleMethod pm = m.getAnnotation(PuzzleMethod.class);
            if (pm != null && pm.value() == getType()){
                puzzleMethods.add(m);
            }
        }

        int total = puzzleMethods.size();
        if (total > 0){
            int randomIndex = new Random().nextInt(total);
            if( randomIndex == mCurrentIndex){
                randomIndex = (randomIndex + 1) % total;
            }
            mCurrentIndex = randomIndex;
            try{
                puzzleMethods.get(mCurrentIndex).invoke(this);
            }catch (Exception e) {
                Log.w(TAG,"Error call puzzle method, try to next one!");
                random(index);
            }
        } else {
            Log.w(TAG,"No puzzle method defined");
        }
        return this;
    }

    public PuzzleSpec getSpec() { return mSpec; }
    public Type getType()       { return mType; }

    public PuzzleSpec.SpecInfo getSpecInfo(int index) {
        return mSpec.getSpecInfo(index);
    }

    private Puzzle init(Type type){
        mType = type;
        if (type == Type.GRID) {
            initMethods();
        }
        return this;
    }

    //////////////////////////////////
    @Target(ElementType.METHOD)
    @Retention(RetentionPolicy.RUNTIME)
    public static @interface PuzzleMethod {
        Type value();
        int weight() default -1;
    }

    public static enum Type {GRID,FREE,STITCH}
}
