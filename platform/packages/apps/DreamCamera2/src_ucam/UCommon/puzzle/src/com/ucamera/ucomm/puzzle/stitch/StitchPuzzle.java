/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.stitch;

import com.ucamera.ucomm.puzzle.Puzzle;
import com.ucamera.ucomm.puzzle.PuzzleSpec;

public class StitchPuzzle extends Puzzle {

    public StitchPuzzle(int size){
        mSpec = PuzzleSpec.create(size);
        for (int i=0; i < mSpec.length(); i++){
            mSpec.set(i, 0, i, 1, i+1);
        }
    }

    @PuzzleMethod(Type.STITCH)
    @Override public Puzzle random(int index){
        return this;
    }
}
