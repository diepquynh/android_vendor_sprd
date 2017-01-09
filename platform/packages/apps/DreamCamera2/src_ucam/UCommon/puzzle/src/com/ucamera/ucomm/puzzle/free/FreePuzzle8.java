/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.free;

import com.ucamera.ucomm.puzzle.Puzzle;
import com.ucamera.ucomm.puzzle.PuzzleSpec;
import com.ucamera.ucomm.puzzle.Puzzle.PuzzleMethod;
import com.ucamera.ucomm.puzzle.Puzzle.Type;

public class FreePuzzle8 extends Puzzle {

    public FreePuzzle8() {
        mSpec = PuzzleSpec.create(8);
    }
    @PuzzleMethod(Type.FREE)
    public void xYxYYxYx(){
        mSpec.reset();
        mSpec.setWidthAndHeight(186, 248);
        mSpec.set(0, 18, 18, 85, 103, 15);
        mSpec.set(1, 100, 17, 167, 102, -5);
        mSpec.set(2, 130, 97, 179, 152, 5);

        mSpec.set(3, 93, 97, 136, 152, 0);
        mSpec.set(4, 7, 97, 50, 152, 0);
        mSpec.set(5, 50, 99, 93, 154, -5);

        mSpec.set(6, 18, 148, 85, 233, 15);
        mSpec.set(7, 100, 147, 167, 232, -5);
    }
//    @PuzzleMethod(Type.FREE)
//    public void xYxYYxYx(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(16, 16);
//        mSpec.set(0, 1, 0, 5, 5, 1);
//        mSpec.set(1, 6, 1, 10, 4, 0);
//        mSpec.set(2, 11, 0, 15, 5, -1);
//
//        mSpec.set(3, 1, 10, 5, 15, 2);
//        mSpec.set(4, 6, 10, 10, 15, 2);
//        mSpec.set(5, 11, 12, 15, 15, -2);
//
//        mSpec.set(6, 3, 5, 8, 11, -3);
//        mSpec.set(7, 9, 5, 14, 11, 3);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void xxxYxxxx(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(16, 16);
//        mSpec.set(0, 1, 0, 5, 5, 1);
//        mSpec.set(1, 6, 1, 10, 4, 0);
//        mSpec.set(2, 11, 0, 15, 5, -1);
//
//        mSpec.set(3, 0, 11, 3, 15, 2);
//        mSpec.set(4, 4, 11, 7, 15, 2);
//        mSpec.set(5, 8, 13, 11, 15, -2);
//        mSpec.set(6, 12, 10, 16, 16, -3);
//
//        mSpec.set(7, 4, 4, 12, 13, 5);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void xxxXxxxx(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(17, 17);
//        mSpec.set(0, 2, 0, 6, 5, 0);
//        mSpec.set(1, 2, 4, 6, 9, -1);
//        mSpec.set(2, 2, 8, 6, 13, 1);
//        mSpec.set(3, 2, 14, 6, 16, -2);
//
//        mSpec.set(4, 11, 0, 15, 5, 0);
//        mSpec.set(5, 11, 4, 15, 9, 1);
//        mSpec.set(6, 11, 8, 15, 13, -1);
//        mSpec.set(7, 11, 12, 15, 17, 1);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void YxxxxxYY(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(15, 15);
//        mSpec.set(0, 0, 5, 2, 8, 1);
//        mSpec.set(1, 4, 5, 6, 8, 0);
//        mSpec.set(2, 8, 5, 10, 8, -1);
//        mSpec.set(3, 12, 5, 14, 8, 1);
//
//        mSpec.set(4, 2, 0, 7, 6, 3);
//
//        mSpec.set(5, 10, 1, 14, 4, 1);
//
//        mSpec.set(6, 2, 9, 7, 15, -3);
//        mSpec.set(7, 8, 9, 13, 15, 3);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void YxxxxYYY(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(13, 12);
//        mSpec.set(0, 0, 0, 3, 4, 30);
//        mSpec.set(1, 3, 0, 6, 4, 30);
//        mSpec.set(2, 6, 0, 10, 4, 30);
//        mSpec.set(3, 10, 0, 13, 4, 30);
//
//        mSpec.set(4, 2, 3, 8, 10, 30);
//
//        mSpec.set(5, 0, 8, 4, 12, 30);
//        mSpec.set(6, 4, 8, 7, 12, 30);
//        mSpec.set(7, 8, 8, 13, 12, 30);
//    }
}
