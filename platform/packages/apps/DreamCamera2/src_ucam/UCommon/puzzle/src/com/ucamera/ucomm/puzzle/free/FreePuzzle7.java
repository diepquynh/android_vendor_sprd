/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.free;

import com.ucamera.ucomm.puzzle.Puzzle;
import com.ucamera.ucomm.puzzle.PuzzleSpec;
import com.ucamera.ucomm.puzzle.Puzzle.PuzzleMethod;
import com.ucamera.ucomm.puzzle.Puzzle.Type;

public class FreePuzzle7 extends Puzzle {

    public FreePuzzle7() {
        mSpec = PuzzleSpec.create(7);
    }
    @PuzzleMethod(Type.FREE)
    public void xxYYYxx(){
        mSpec.reset();
        mSpec.setWidthAndHeight(186, 248);
        mSpec.set(0, 18, 18, 85, 103, 15);
        mSpec.set(1, 100, 17, 167, 102, -5);
        mSpec.set(2, 118, 91, 173, 161, 5);

        mSpec.set(3, 62, 89, 117, 159, 0);
        mSpec.set(4, 9, 95, 64, 165, -10);
        mSpec.set(5, 18, 148, 85, 233, 15);

        mSpec.set(6, 100, 147, 167, 232, -5);
    }

//    @PuzzleMethod(Type.FREE)
//    public void xxxYxxx(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(20, 20);
//        mSpec.set(0, 0, 0, 6, 8, -1);
//        mSpec.set(1, 7, 0, 13, 8, 0);
//        mSpec.set(2, 14, 0, 20, 8, 1);
//
//        mSpec.set(3, 0, 12, 6, 20, -1);
//        mSpec.set(4, 7, 12, 13, 20, 0);
//        mSpec.set(5, 14, 12, 20, 20, 1);
//
//        mSpec.set(6, 4, 6, 16, 14, 0);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void xxxYxYx(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(20, 28);
//        mSpec.set(0, 0, 1, 6, 9, -2);
//        mSpec.set(1, 0, 10, 6, 18, -1);
//        mSpec.set(2, 0, 19, 6, 27, -2);
//
//        mSpec.set(3, 7, 9, 13, 17, 2);
//
//        mSpec.set(4, 14, 1, 20, 9, 0);
//        mSpec.set(5, 14, 11, 20, 17, 2);
//        mSpec.set(6, 14, 19, 20, 27, -2);
//    }
}
