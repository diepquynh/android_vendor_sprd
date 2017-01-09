/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.free;

import com.ucamera.ucomm.puzzle.Puzzle;
import com.ucamera.ucomm.puzzle.PuzzleSpec;
import com.ucamera.ucomm.puzzle.Puzzle.PuzzleMethod;
import com.ucamera.ucomm.puzzle.Puzzle.Type;

import java.util.Random;


public class FreePuzzle4 extends Puzzle {

    public FreePuzzle4() {
        mSpec = PuzzleSpec.create(4);
    }
    @PuzzleMethod(Type.FREE)
    public void YYxx(){
        mSpec.reset();
        mSpec.setWidthAndHeight(186, 248);
        mSpec.set(0, 24, 18, 111, 119, -30);
        mSpec.set(1, 23, 67, 109, 168, 30);

        mSpec.set(2, 88, 106, 175, 227, 0);
        mSpec.set(3, 19, 130, 106, 231, -15);
    }
//    @PuzzleMethod(Type.FREE)
//    public void xxYY(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(12, 18);
//        mSpec.set(0, 2, 0, 5, 6, 30);
//        mSpec.set(1, 7, 1, 10, 8, 30);
//
//        mSpec.set(2, 2, 9, 5, 16, 30);
//        mSpec.set(3, 7, 8, 10, 14, 30);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void Yxxx(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(8, 8);
//        mSpec.set(0, 1, 2, 4, 6, 0);
//
//        mSpec.set(1, 5, 1, 7, 3, 0);
//        mSpec.set(2, 5, 3, 7, 5, -1);
//        mSpec.set(3, 5, 5, 7, 7, 1);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void xxxY(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(10, 10);
//        mSpec.set(0, 1, 1, 4, 5, -1);
//        mSpec.set(1, 1, 5, 4, 9, 1);
//
//        mSpec.set(2, 6, 1, 9, 5, 5);
//        mSpec.set(3, 6, 6, 9, 9, -2);
//    }
}
