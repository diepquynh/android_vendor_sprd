/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.free;

import com.ucamera.ucomm.puzzle.Puzzle;
import com.ucamera.ucomm.puzzle.PuzzleSpec;

public class FreePuzzle9 extends Puzzle {

    public FreePuzzle9() {
        mSpec = PuzzleSpec.create(9);
    }
    @PuzzleMethod(Type.FREE)
    public void xxxxYxxxx(){
        mSpec.reset();
        mSpec.setWidthAndHeight(186, 248);
        mSpec.set(0, 51, 66, 138, 177, 0);
        mSpec.set(1, 136, 16, 179, 71, 5);
        mSpec.set(2, 93, 16, 166, 71, 0);
        mSpec.set(3, 7, 16, 50, 71, 0);

        mSpec.set(4, 50, 18, 93, 73, -5);

        mSpec.set(5, 136, 175, 179, 230, 5);
        mSpec.set(6, 93, 175, 136, 230, 0);
        mSpec.set(7, 7, 175, 50, 230, 0);
        mSpec.set(8, 50, 177, 93, 232, -5);
    }
//    @PuzzleMethod(Type.FREE)
//    public void xxxxYYxxx(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(18, 18);
//        mSpec.set(0, 1, 1, 4, 4, 30);
//        mSpec.set(1, 5, 1, 8, 4, 30);
//        mSpec.set(2, 9, 1, 12, 4, 30);
//        mSpec.set(3, 13, 1, 16, 4, 30);
//
//        mSpec.set(4, 5, 5, 11, 11, 30);
//
//        mSpec.set(5, 1, 14, 4, 17, 30);
//        mSpec.set(6, 5, 14, 8, 17, 30);
//        mSpec.set(7, 9, 14, 12, 17, 30);
//        mSpec.set(8, 13, 14, 16, 17, 30);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void xxxYYYxxx(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(18, 18);
//        mSpec.set(0, 1, 1, 5, 5, -30);
//        mSpec.set(1, 6, 1, 10, 5, -30);
//        mSpec.set(2, 11, 1, 15, 5, -30);
//
//        mSpec.set(3, 1, 6, 5, 10, 0);
//        mSpec.set(4, 6, 6, 10, 10, 0);
//        mSpec.set(5, 11, 6, 15, 10, 0);
//
//        mSpec.set(6, 1, 13, 5, 17, 30);
//        mSpec.set(7, 6, 13, 10, 17, 30);
//        mSpec.set(8, 11, 13, 15, 17, 30);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void xxxzYzxxx(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(18, 18);
//        mSpec.set(0, 1, 1, 5, 5, 30);
//        mSpec.set(1, 6, 1, 10, 5, 30);
//        mSpec.set(2, 11, 1, 15, 5, 30);
//
//        mSpec.set(3, 1, 6, 6, 11, 30);
//        mSpec.set(4, 13, 5, 17, 10, 30);
//
//        mSpec.set(5, 1, 14, 4, 17, 30);
//        mSpec.set(6, 5, 12, 9, 16, 30);
//        mSpec.set(7, 12, 12, 15, 15, 30);
//
//        mSpec.set(8, 4, 5, 13, 14, 30);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void xxxzzYxxx(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(18, 18);
//        mSpec.set(0, 1, 1, 5, 5, 30);
//        mSpec.set(1, 6, 1, 10, 5, 30);
//        mSpec.set(2, 11, 1, 15, 5, 30);
//
//        mSpec.set(3, 1, 6, 6, 11, 30);
//        mSpec.set(4, 14, 5, 18, 10, 0);
//
//        mSpec.set(5, 1, 14, 4, 17, 30);
//        mSpec.set(6, 6, 13, 10, 17, 30);
//        mSpec.set(7, 12, 12, 15, 15, 30);
//
//        mSpec.set(8, 4, 5, 13, 14, -30);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void xxxxxYYYY(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(18, 18);
//        mSpec.set(0, 3, 0, 7, 4, 0);
//        mSpec.set(1, 2, 4, 6, 8, 0);
//        mSpec.set(2, 3, 7, 8, 11, 0);
//        mSpec.set(3, 1, 10, 5, 14, 0);
//        mSpec.set(4, 2, 13, 7, 18, 0);
//
//        mSpec.set(5, 12, 1, 17, 6, 0);
//        mSpec.set(6, 10, 6, 15, 11, 0);
//        mSpec.set(7, 11, 10, 15, 14, 0);
//        mSpec.set(8, 8, 13, 15, 17, 15);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void xxxxxxxxY(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(18, 18);
//        mSpec.set(0, 0, 10, 5, 13, 0);
//        mSpec.set(1, 12, 1, 17, 7, 0);
//        mSpec.set(2, 7, 10, 10, 13, 0);
//        mSpec.set(3, 14, 7, 18, 10, 0);
//
//        mSpec.set(4, 0, 13, 5, 18, 0);
//        mSpec.set(5, 4, 14, 9, 18, 0);
//        mSpec.set(6, 10, 14, 15, 18, 0);
//        mSpec.set(7, 14, 13, 18, 17, 0);
//
//        mSpec.set(8, 2, 2, 9, 7, 20);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void xxxxYYYYY(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(18, 18);
//        mSpec.set(0, 0, 3, 3, 8, 0);
//        mSpec.set(1, 2, 6, 6, 10, 0);
//        mSpec.set(2, 3, 8, 7, 14, 0);
//        mSpec.set(3, 4, 13, 10, 18, 0);
//
//        mSpec.set(4, 5, 0, 10, 5, 0);
//        mSpec.set(5, 11, 2, 14, 7, 0);
//        mSpec.set(6, 12, 5, 17, 10, 0);
//        mSpec.set(7, 13, 11, 18, 15, 0);
//        mSpec.set(8, 13, 14, 18, 17, 0);
//    }
}
