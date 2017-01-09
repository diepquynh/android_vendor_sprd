/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.grid;

import com.ucamera.ucomm.puzzle.PuzzleSpec;

public class GridPuzzle4 extends GridBase {

    public GridPuzzle4() {
        mSpec = PuzzleSpec.create(4);
    }
    /* +-+-+
     * +-+-+
     * +-+-+ (2,2)
     */
    @PuzzleMethod(Type.GRID)
    public void splitX2Y2() {
        mSpec.reset();
        mSpec.set(0, 0, 0, 1, 1);
        mSpec.set(1, 0, 1, 1, 2);
        mSpec.set(2, 1, 0, 2, 1);
        mSpec.set(3, 1, 1, 2, 2);
    }

    /* +-+-+-+-+
     * +-+-+-+-+(4,2)
     */
    @PuzzleMethod(Type.GRID)
    public void splitXXXX() {
        mSpec.reset();
        mSpec.set(0, 0, 0, 1, 2);
        mSpec.set(1, 1, 0, 2, 2);
        mSpec.set(2, 2, 0, 3, 2);
        mSpec.set(3, 3, 0, 4, 2);
    }

    /* +--+
     * +--+
     * +--+
     * +--+ (2,4)
     */
    @PuzzleMethod(Type.GRID)
    public void splitYYYY() {
        mSpec.reset();
        mSpec.set(0, 0, 0, 2, 1);
        mSpec.set(1, 0, 1, 2, 2);
        mSpec.set(2, 0, 2, 2, 3);
        mSpec.set(3, 0, 3, 2, 4);
    }
    /*+-----+
    * +-+-+-+
    * +-+-+-+*/
    @PuzzleMethod(Type.GRID)
    public void splitY1X3() {
        mSpec.reset();
        mSpec.set(0, 0, 0, 3, 2);
        mSpec.set(1, 0, 2, 1, 4);
        mSpec.set(2, 1, 2, 2, 4);
        mSpec.set(3, 2, 2, 3, 4);
    }
    /*+-+-+-+
    * +-+-+-+
    * +-----+*/
    @PuzzleMethod(Type.GRID)
    public void splitY3X1() {
        mSpec.reset();
        mSpec.set(0, 0, 0, 1, 1);
        mSpec.set(1, 1, 0, 2, 1);
        mSpec.set(2, 2, 0, 3, 1);
        mSpec.set(3, 0, 1, 3, 2);
    }
    /*+--+--+
    * +  +--+
    * +  +--+
    * +--+--+*/
    @PuzzleMethod(Type.GRID)
    public void splitX1Y3() {
        mSpec.reset();
        mSpec.set(0, 0, 0, 1, 3);
        mSpec.set(1, 1, 0, 2, 1);
        mSpec.set(2, 1, 1, 2, 2);
        mSpec.set(3, 1, 2, 2, 3);
    }
    /*+--+--+
    * +--+  +
    * +--+  +
    * +--+--+*/
    @PuzzleMethod(Type.GRID)
    public void splitX3Y1() {
        mSpec.reset();
        mSpec.set(0, 0, 0, 1, 1);
        mSpec.set(1, 0, 1, 1, 2);
        mSpec.set(2, 0, 2, 1, 3);
        mSpec.set(3, 1, 0, 2, 3);
    }
    /*
     * 255 * 255
     */
    /*@PuzzleMethod(Type.GRID)
    public void random4(){
        mSpec.reset();
        Random random = new Random();

        int x1 = random.nextInt(155) + 50;
        int y1 = random.nextInt(155) + 50;

        if(random.nextInt(2) == 0){ //same y
            switch (random.nextInt(3)) {
                case 0: {
                       +---+-+
                     * +-+-+-+
                     * +-+---+ (255,255)

                    int x2 = random.nextInt(155) + 50;
                    mSpec.set(0,0,0,x1,y1);
                    mSpec.set(1,x1,0,255,y1);
                    mSpec.set(2,0,y1,x2,255);
                    mSpec.set(3,x2,y1,255,255);
                    break;
                }

                case 1: {
                       +-+-+-+
                     * +-+-+-+
                     * +-----+ (255,255)

                    int x2 = 0;
                    do {
                        x2 = random.nextInt(155) + 50;
                    } while(Math.abs(x1-x2) < 50);

                    int min = Math.min(x1, x2);
                    int max = Math.max(x1, x2);
                    x1 = min;
                    x2 = max;

                    mSpec.set(0, 0,  0,  x1,  y1);
                    mSpec.set(1, x1, 0,  x2,  y1);
                    mSpec.set(2, x2, 0,  255, y1);
                    mSpec.set(3, 0,  y1, 255, 255);
                    break;
                }

                case 2: {
                       +-----+
                     * +-+-+-+
                     * +-+-+-+ (255,255)

                    int x2 = 0;
                    do {
                        x2 = random.nextInt(155) + 50;
                    } while(Math.abs(x1-x2) < 50);

                    int min = Math.min(x1, x2);
                    int max = Math.max(x1, x2);
                    x1 = min;
                    x2 = max;

                    mSpec.set(0,0, 0, 255,y1);
                    mSpec.set(1,0, y1,x1, 255);
                    mSpec.set(2,x1,y1,x2, 255);
                    mSpec.set(3,x2,y1,255,255);
                    break;
                }
            }
        } else { // same x
            switch (random.nextInt(3)) {
                case 0: {
                       +-+-+
                     * | + +
                     * +-+ |
                     * +-+-+ (255,255)

                    int y2 = random.nextInt(155) + 50;
                    mSpec.set(0, 0,  0,  x1,  y1);
                    mSpec.set(1, 0,  y1, x1,  255);
                    mSpec.set(2, x1, 0,  255, y2);
                    mSpec.set(3, x1, y2, 255, 255);
                    break;
                }

                case 1: {
                       +--+--+
                     * +--+  +
                     * +--+  +
                     * +--+--+ (255,255)

                    int y2 = 0;
                    do {
                        y2 = random.nextInt(155) + 50;
                    } while(Math.abs(y1-y2) < 50);

                    int min = Math.min(y1, y2);
                    int max = Math.max(y1, y2);
                    y1 = min;
                    y2 = max;
                    mSpec.set(0, 0, 0,  x1,  y1);
                    mSpec.set(1, 0, y1, x1,  y2);
                    mSpec.set(2, 0, y2, x1,  255);
                    mSpec.set(3, x1,0,  255, 255);
                    break;
                }

                case 2:
                       +--+--+
                     * +  +--+
                     * +  +--+
                     * +--+--+ (255,255)

                    int y2 = 0;
                    do {
                        y2 = random.nextInt(155) + 50;
                    } while(Math.abs(y1-y2) < 50);

                    int min = Math.min(y1, y2);
                    int max = Math.max(y1, y2);
                    y1 = min;
                    y2 = max;
                    mSpec.set(0, 0,  0,  x1,  255);
                    mSpec.set(1, x1, 0,  255, y1);
                    mSpec.set(2, x1, y1, 255, y2);
                    mSpec.set(3, x1, y2, 255, 255);
                    break;
            }
        }
    }*/
}
