/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.grid;

import com.ucamera.ucomm.puzzle.PuzzleSpec;

public class GridPuzzle2 extends GridBase {

    public GridPuzzle2() {
        mSpec = PuzzleSpec.create(2);
    }

    /* +--+--+
     * +--+--+ (4,4)
     */
    @PuzzleMethod(value = Type.GRID)
    public void splitX1X3(){
        mSpec.reset();
        //int x = new Random().nextInt(3) + 1;
        mSpec.set(0,0,0,1,4);
        mSpec.set(1,1,0,4,4);
    }
    @PuzzleMethod(value = Type.GRID)
    public void splitX2X2(){
        mSpec.reset();
        //int x = new Random().nextInt(3) + 1;
        mSpec.set(0,0,0,2,4);
        mSpec.set(1,2,0,4,4);
    }
    @PuzzleMethod(value = Type.GRID)
    public void splitX3X1(){
        mSpec.reset();
        //int x = new Random().nextInt(3) + 1;
        mSpec.set(0,0,0,3,4);
        mSpec.set(1,3,0,4,4);
    }

    /*@PuzzleMethod(value = Type.GRID)
    public void splitXX(){
        mSpec.reset();
        int x = new Random().nextInt(3) + 1;
        mSpec.set(0,0,0,x,4);
        mSpec.set(1,x,0,4,4);
    }*/

    /* +----+
     * +----+
     * +----+ (4,4)
     */
    /*@PuzzleMethod(value = Type.GRID)
    public void splitYY(){
        mSpec.reset();
        int y = new Random().nextInt(3) + 1;
        mSpec.set(0,0,0,4,y);
        mSpec.set(1,0,y,4,4);
    }*/
    @PuzzleMethod(value = Type.GRID)
    public void splitY1Y3(){
        mSpec.reset();
        //int y = new Random().nextInt(3) + 1;
        mSpec.set(0,0,0,4,1);
        mSpec.set(1,0,1,4,4);
    }
    @PuzzleMethod(value = Type.GRID)
    public void splitY2Y2(){
        mSpec.reset();
        //int y = new Random().nextInt(3) + 1;
        mSpec.set(0,0,0,4,2);
        mSpec.set(1,0,2,4,4);
    }
    @PuzzleMethod(value = Type.GRID)
    public void splitY3Y1(){
        mSpec.reset();
        //int y = new Random().nextInt(3) + 1;
        mSpec.set(0,0,0,4,3);
        mSpec.set(1,0,3,4,4);
    }

    /*
     * 255 * 255

    @PuzzleMethod(Type.GRID)
    public void random1v1(){
        mSpec.reset();
        Random random = new Random();
        int x = random.nextInt(155) + 50;
        switch (random.nextInt(2)){
            case 0:
                 +--+--+
                 * +--+--+ (255,255)

                mSpec.set(0, 0, 0, x, 255);
                mSpec.set(1, x, 0, 255, 255);
                break;
            case 1:
                 +---+
                 * +---+
                 * +---+ (255,255)

                mSpec.set(0, 0, 0, 255, x);
                mSpec.set(1, 0, x, 255, 255);
                break;
        }
    }*/
}
