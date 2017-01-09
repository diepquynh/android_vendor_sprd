/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.free;

import com.ucamera.ucomm.puzzle.Puzzle;
import com.ucamera.ucomm.puzzle.PuzzleSpec;

import java.util.Random;


public class FreePuzzle3 extends Puzzle {

    public FreePuzzle3() {
        mSpec = PuzzleSpec.create(3);
    }
    @PuzzleMethod(Type.FREE)
    public void yxy() {
        mSpec.reset();
        mSpec.setWidthAndHeight(186, 248);
        mSpec.set(0, 68, 16, 155, 127, 0);
        mSpec.set(1, 24, 67, 101, 167, -28);
        mSpec.set(2, 74, 133, 151, 234, 10);
    }
//    @PuzzleMethod(Type.FREE)
//    public void yxx() {
//        mSpec.reset();
//        mSpec.setWidthAndHeight(20, 20);
//        mSpec.set(0, 6, 2, 14, 10, 10);
//        mSpec.set(1, 4, 12, 8, 16, 45);
//        mSpec.set(2, 12, 12, 16, 16, 45);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void xxy() {
//        mSpec.reset();
//        mSpec.setWidthAndHeight(20, 20);
//        mSpec.set(0, 2, 2, 6, 6, 45);
//        mSpec.set(1, 13, 2, 17, 6, 45);
//        mSpec.set(2, 8, 13, 13, 18, 30);
//    }
}
