/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.free;

import com.ucamera.ucomm.puzzle.Puzzle;
import com.ucamera.ucomm.puzzle.PuzzleSpec;
import com.ucamera.ucomm.puzzle.Puzzle.PuzzleMethod;
import com.ucamera.ucomm.puzzle.Puzzle.Type;

public class FreePuzzle5 extends Puzzle {

    public FreePuzzle5() {
        mSpec = PuzzleSpec.create(5);
    }
    @PuzzleMethod(Type.FREE)
    public void xxYxx(){
        mSpec.reset();
        mSpec.setWidthAndHeight(186, 248);
        mSpec.set(0, 94, 18, 181, 192, 0);
        mSpec.set(1, 15, 9, 92, 110, 10);


        mSpec.set(2, 21, 72, 98, 172, -28);
        mSpec.set(3, 21, 137, 98, 238, 0);

        mSpec.set(4, 97, 134, 174, 235, 10);
    }
//    @PuzzleMethod(Type.FREE)
//    public void xxYxx(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(20, 28);
//        mSpec.set(0, 2, 1, 8, 9, -2);
//        mSpec.set(1, 2, 17, 8, 25, -2);
//
//
//        mSpec.set(2, 12, 1, 18, 9, 0);
//        mSpec.set(3, 12, 18, 18, 24, 2);
//
//        mSpec.set(4, 5, 6, 15, 19, -2);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void xxxYY(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(20, 28);
//        mSpec.set(0, 0, 1, 6, 9, 1);
//        mSpec.set(1, 7, 1, 13, 9, -1);
//        mSpec.set(2, 14, 1, 20, 9, -1);
//
//        mSpec.set(3, 2, 14, 9, 23, -2);
//        mSpec.set(4, 13, 13, 18, 25, 2);
//    }
//
//    @PuzzleMethod(Type.FREE)
//    public void xxYYY(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(8, 8);
//        mSpec.set(0, 1, 1, 3, 3, 1);
//        mSpec.set(1, 5, 0, 7, 3, -40);
//        mSpec.set(2, 3, 3, 5, 6, 5);
//
//        mSpec.set(3, 0, 5, 2, 7, -3);
//        mSpec.set(4, 6, 5, 8, 7, -30);
//    }
}
