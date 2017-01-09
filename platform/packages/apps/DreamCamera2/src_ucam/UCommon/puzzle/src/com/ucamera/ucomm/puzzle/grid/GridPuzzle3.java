/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.grid;

import com.ucamera.ucomm.puzzle.PuzzleSpec;

public class GridPuzzle3 extends GridBase {

    public GridPuzzle3() {
        mSpec = PuzzleSpec.create(3);
    }

    /* +----+
     * +----+
     * +----+
     * +----+ (3,3)
     */
    @PuzzleMethod(value=Type.GRID)
    public void splitXXX() {
        mSpec.reset();
        mSpec.set(0, 0, 0, 1, 3);
        mSpec.set(1, 1, 0, 2, 3);
        mSpec.set(2, 2, 0, 3, 3);
    }

    /* +--+--+--+
     * +--+--+--+ (3,3)
     */
    @PuzzleMethod(value=Type.GRID)
    public void splitYYY() {
        mSpec.reset();
        mSpec.set(0, 0, 0, 3, 1);
        mSpec.set(1, 0, 1, 3, 2);
        mSpec.set(2, 0, 2, 3, 3);
    }

    /* +--+--+
     * +--+  |
     * +--+--+ (3,3)
     */
    @PuzzleMethod(value=Type.GRID)
    public void splitX2X() {
        mSpec.reset();
        mSpec.set(0, 0, 0, 1, 1);
        mSpec.set(1, 0, 1, 1, 2);
        mSpec.set(2, 1, 0, 3, 2);
    }

    /* +--+--+
     * +--+--+
     * +-----+ (3,3)
     */
    @PuzzleMethod(value=Type.GRID)
    public void splitY2Y() {
        mSpec.reset();
        mSpec.set(0, 0, 0, 1, 1);
        mSpec.set(1, 1, 0, 2, 1);
        mSpec.set(2, 0, 1, 2, 3);
    }

    @PuzzleMethod(value=Type.GRID)
    public void split2v1(){
        mSpec.reset();

        int x = 125;
        int y = 125;
        /* +--+--+
         * |  +--+
         * +--+--+ (max,max)
         */
        mSpec.set(0, 0, 0, x, 255);
        mSpec.set(1, x, 0, 255, y);
        mSpec.set(2, x, y, 255, 255);
    }

    @PuzzleMethod(value=Type.GRID)
    public void split1v2(){
        mSpec.reset();

        int x = 125;
        int y = 125;
        /* +-----+
         * +--+--+
         * +--+--+ (max,max)
         */
        mSpec.set(0, 0, 0, 255, y);
        mSpec.set(1, 0, y, x, 255);
        mSpec.set(2, x, y, 255, 255);
    }

    /**
     *  255 * 255
     */
    /*@PuzzleMethod(value=Type.GRID)
    public void random2v1(){
        mSpec.reset();

        Random random = new Random();

        int x = random.nextInt(155) + 50;
        int y = random.nextInt(155) + 50;

        switch (random.nextInt(4)){
            case 0:
                 +--+--+
                 * +--+  |
                 * +--+--+ (max,max)

                mSpec.set(0, 0, 0, x, y);
                mSpec.set(1, 0, y, x, 255);
                mSpec.set(2, x, 0, 255, 255);
                break;
            case 1:
                 +-----+
                 * +--+--+
                 * +--+--+ (max,max)

                mSpec.set(0, 0, 0, 255, y);
                mSpec.set(1, 0, y, x, 255);
                mSpec.set(2, x, y, 255, 255);
                break;
            case 2:
                 +--+--+
                 * |  +--+
                 * +--+--+ (max,max)

                mSpec.set(0, 0, 0, x, 255);
                mSpec.set(1, x, 0, 255, y);
                mSpec.set(2, x, y, 255, 255);
                break;
            case 3:
                 +--+--+
                 * +--+--+
                 * +-----+ (max,max)

                mSpec.set(0, 0, 0, x, y);
                mSpec.set(1, x, 0, 255, y);
                mSpec.set(2, 0, y, 255, 255);
                break;
        }
    }*/
}
