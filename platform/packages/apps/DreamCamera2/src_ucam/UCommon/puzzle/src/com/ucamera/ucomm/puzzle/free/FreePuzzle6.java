/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.free;

import com.ucamera.ucomm.puzzle.Puzzle;
import com.ucamera.ucomm.puzzle.PuzzleSpec;
import com.ucamera.ucomm.puzzle.Puzzle.PuzzleMethod;
import com.ucamera.ucomm.puzzle.Puzzle.Type;

public class FreePuzzle6 extends Puzzle {

    public FreePuzzle6() {
        mSpec = PuzzleSpec.create(6);
    }
    @PuzzleMethod(Type.FREE)
    public void xxxYYY(){
        mSpec.reset();
        mSpec.setWidthAndHeight(186, 248);
        mSpec.set(0, 23, 40, 90, 125, -5);
        mSpec.set(1, 29, 123, 96, 208, -15);
        mSpec.set(2, 107, 14, 162, 84, 5);


        mSpec.set(3, 107, 64, 162, 134, 0);
        mSpec.set(4, 112, 117, 167, 187, 5);
        mSpec.set(5, 107, 154, 162, 224, -10);
    }
//    @PuzzleMethod(Type.FREE)
//    public void xxxYYY(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(20, 28);
//        mSpec.set(0, 2, 1, 8, 9, -2);
//        mSpec.set(1, 2, 10, 8, 18, -1);
//        mSpec.set(2, 2, 19, 8, 27, -2);
//
//
//        mSpec.set(3, 12, 1, 18, 9, 0);
//        mSpec.set(4, 12, 11, 18, 17, 2);
//        mSpec.set(5, 12, 19, 18, 27, -2);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void xxxYxx(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(20, 28);
//        mSpec.set(0, 0, 1, 6, 9, -2);
//        mSpec.set(1, 0, 10, 6, 18, -1);
//        mSpec.set(2, 0, 19, 6, 27, -2);
//
//
//        mSpec.set(3, 7, 0, 18, 19, -3);
//        mSpec.set(4, 8, 20, 13, 25, 2);
//        mSpec.set(5, 14, 19, 20, 27, 2);
//    }
}
