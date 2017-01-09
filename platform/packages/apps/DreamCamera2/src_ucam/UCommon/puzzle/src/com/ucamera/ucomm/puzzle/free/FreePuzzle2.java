/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.free;

import com.ucamera.ucomm.puzzle.Puzzle;
import com.ucamera.ucomm.puzzle.PuzzleSpec;

public class FreePuzzle2 extends Puzzle {

    public FreePuzzle2() {
        mSpec = PuzzleSpec.create(2);
    }

//    @PuzzleMethod(Type.FREE)
//    public void topbottom(){
//         mSpec.reset();
//         mSpec.setWidthAndHeight(186, 248);
//         mSpec.set(0, 24, 26, 135, 169, -15);
//         mSpec.set(1, 77, 104, 162, 214, 15);
//    }

    @PuzzleMethod(Type.FREE)
    public void leftright(){
        mSpec.reset();
        mSpec.setWidthAndHeight(186, 248);
        mSpec.set(0, 22, 85, 133, 228, -15);
        mSpec.set(1, 76, 16, 161, 126, 15);
    }
//
//    @PuzzleMethod(Type.FREE)
//    public void leftcenter(){
//        mSpec.reset();
//        mSpec.setWidthAndHeight(191, 255);
//        mSpec.set(0, 27, 22, 128, 165, -15);
//        mSpec.set(1, 75, 114, 150, 224, 15);
//    }
}
