/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucomm.puzzle.grid;

import com.ucamera.ucomm.puzzle.PuzzleSpec;

import java.util.Random;

public class GridPuzzle9 extends GridBase {

    public GridPuzzle9() {
        mSpec = PuzzleSpec.create(9);
    }

    /*
     * +---+---+---+
     * |   |   |   |
     * +---+---+---+
     * |   |   |   |
     * +---+---+---+
     * |   |   |   |
     * +---+---+---+ (max,max){max=3~5}
     */
    @PuzzleMethod(Type.GRID)
    public void grid() {
        int max = new Random().nextInt(3) + 3;
        float points[] = {
                0, 0, 1, 1,
                0, 1, 1, (max - 1),
                0, (max - 1), 1, max,
                1, 0, (max - 1), 1,
                1, 1, (max - 1), (max - 1),
                1, (max - 1), (max - 1), max,
                (max - 1), 0, max, 1,
                (max - 1), 1, max, (max - 1),
                (max - 1), (max - 1), max, max
        };
        setupSpec(points);
    }

    /*
     * +---+---+---+---+
     * |   |   |   |   |
     * +---+---+---+---+
     * |               |
     * |               |
     * +---+---+---+---+
     * |   |   |   |   |
     * +---+---+---+---+ (4,4)
     */
    @PuzzleMethod(Type.GRID)
    public void grid1() {
        float points[] = {
                0, 0, 1, 1,
                1, 0, 2, 1,
                2, 0, 3, 1,
                3, 0, 4, 1,
                3, 3, 4, 4,
                2, 3, 3, 4,
                1, 3, 2, 4,
                0, 3, 1, 4,
                0, 1, 4, 3
        };
        randomRotate(points, 4, 4);
        setupSpec(points);
    }

    /*
     * +---+---+-------+
     * |   |   |       |
     * +---+---+       |
     * |   |   |       |
     * +---+---+       |
     * |   |   |       |
     * +---+---+       |
     * |   |   |       |
     * +---+---+-------+ (4,4)
     */
    @PuzzleMethod(Type.GRID)
    public void grid2() {
        float points[] = {
                0, 0, 1, 1,
                0, 1, 1, 2,
                0, 2, 1, 3,
                0, 3, 1, 4,
                1, 0, 2, 1,
                1, 1, 2, 2,
                1, 2, 2, 3,
                1, 3, 2, 4,
                2, 0, 4, 4
        };
        randomRotate(points, 4, 4);
        setupSpec(points);
    }

    /*
     * +---+---------------+
     * |   |               |
     * +---+               |
     * |   |               |
     * +---+---+---+---+---+
     * |   |   |   |   |   |
     * +---+   |   |   |   |
     * |   |   |   |   |   |
     * +---+---+---+---+---+ (5,4)
     */
    @PuzzleMethod(Type.GRID)
    public void grid3() {
        float points[] = {
                0, 0, 1, 1,
                0, 1, 1, 2,
                0, 2, 1, 3,
                0, 3, 1, 4,
                1, 0, 5, 2,
                1, 2, 2, 4,
                2, 2, 3, 4,
                3, 2, 4, 4,
                4, 2, 5, 4
        };
        randomRotate(points, 5, 4);
        setupSpec(points);
    }

    /*
     * +---+---+---+---+
     * |           |   |
     * +---+---+---+---+
     * |       |   |   |
     * +---+---+---+   |
     * |   |   |   |   |
     * +---+   +   |   |
     * |   |   |   |   |
     * +---+---+---+---+ (4,4)
     */
    @PuzzleMethod(Type.GRID)
    public void grid4() {
        float points[] = {
                0, 0, 3, 1,
                3, 0, 4, 1,
                3, 1, 4, 4,
                0, 1, 2, 2,
                2, 1, 3, 2,
                2, 2, 3, 4,
                0, 2, 1, 3,
                1, 2, 2, 4,
                0, 3, 1, 4
        };
        randomRotate(points, 4, 4);
        setupSpec(points);
    }

    /*
     * +---+---+-------+
     * |   |   |       |
     * +---+---+       |
     * |   |   |       |
     * +---+---+-------+
     * |   |   |       |
     * +---+   |       |
     * |   |   |       |
     * +---+---+-------+ (4,4)
     */
    @PuzzleMethod(Type.GRID)
    public void grid5() {
        float points[] = {
                0, 0, 1, 1,
                0, 1, 1, 2,
                0, 2, 1, 3,
                0, 3, 1, 4,
                1, 0, 2, 1,
                1, 1, 2, 2,
                1, 2, 2, 4,
                2, 0, 4, 2,
                2, 2, 4, 4
        };
        randomRotate(points, 4, 4);
        setupSpec(points);
    }

    /*
     * +---+---+-------+
     * |   |   |       |
     * +---+---+       |
     * |   |   |       |
     * +---+---+-------+
     * |   |   |       |
     * |   +---+       |
     * |   |   |       |
     * +---+---+-------+ (4,4)
     */
    @PuzzleMethod(Type.GRID)
    public void grid6() {
        float points[] = {
                0, 0, 1, 1,
                0, 1, 1, 2,
                0, 2, 1, 4,
                1, 0, 2, 1,
                1, 1, 2, 2,
                1, 2, 2, 3,
                1, 3, 2, 4,
                2, 0, 4, 2,
                2, 2, 4, 4
        };
        randomRotate(points, 4, 4);
        setupSpec(points);
    }

    /*
     * +---+---+---+
     * |   |   |   |
     * +---+   |   |
     * |   +---+   |
     * +---+   +---+
     * |   |   |   |
     * +---+---+   |
     * |   |   |   |
     * +---+---+---+ (12,12)
     */
    @PuzzleMethod(Type.GRID)
    public void grid7() {
        float points[] = {
                0, 0, 4, 6,
                0, 6, 4, 12,
                4, 0, 8, 4,
                4, 4, 8, 8,
                4, 8, 8, 12,
                8, 0, 12, 3,
                8, 3, 12, 6,
                8, 6, 12, 9,
                8, 9, 12, 12
        };
        randomRotate(points, 12, 12);
        setupSpec(points);
    }

    /*
     * +---+---+---+
     * |   |   |   |
     * +---+   |   |
     * |   +   +---+
     * +---+---+   |
     * |   |   |   |
     * +---+   +---+
     * |   |   |   |
     * +---+---+---+ (12,12)
     */
    @PuzzleMethod(Type.GRID)
    public void grid8() {
        float points[] = {
                0, 0, 4, 3,
                0, 3, 4, 6,
                0, 6, 4, 9,
                0, 9, 4, 12,
                4, 0, 8, 6,
                4, 6, 8, 12,
                8, 0, 12, 4,
                8, 4, 12, 8,
                8, 8, 12, 12
        };
        randomRotate(points, 12, 12);
        setupSpec(points);
    }
}
