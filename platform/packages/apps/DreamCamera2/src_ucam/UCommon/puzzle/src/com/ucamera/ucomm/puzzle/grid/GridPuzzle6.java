/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucomm.puzzle.grid;

import com.ucamera.ucomm.puzzle.PuzzleSpec;

public class GridPuzzle6 extends GridBase {

    public GridPuzzle6() {
        mSpec = PuzzleSpec.create(6);
    }

    /* MAYBE rotated
     *  +------+-----+
     *  |      |     |
     *  +------+-----+
     *  |      |     |
     *  +------+-----+
     *  |      |     |
     *  +------+-----+ (2,3)
     */
    @PuzzleMethod(Type.GRID)
    public void grid1() {
        float points[] = {
                0, 0, 1, 1,
                0, 1, 1, 2,
                0, 2, 1, 3,
                1, 0, 2, 1,
                1, 1, 2, 2,
                1, 2, 2, 3
        };
        randomRotate(points, 2, 3);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +---+---+----+
     * |   |   |    |
     * +---+---+----+
     * |   |        |
     * +---+        |
     * |   |        |
     * +---+--------+ (3,3)
     */
    @PuzzleMethod(Type.GRID)
    public void grid2() {
        float points[] = {
                0, 0, 1, 1,
                0, 1, 1, 2,
                0, 2, 1, 3,
                1, 0, 2, 1,
                2, 0, 3, 1,
                1, 1, 3, 3
        };
        randomRotate(points, 3, 3);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +-------+-------+
     * |       |       |
     * +-------+       |
     * |       |       |
     * +-------+---+---|
     * |       |   |   |
     * |       |   |   |
     * |       |   |   |
     * +-------+---+---+ (4,4)
     */
    @PuzzleMethod(Type.GRID)
    public void grid3() {
        float points[] = {
                0, 0, 2, 1,
                0, 1, 2, 2,
                0, 2, 2, 4,
                2, 0, 4, 2,
                2, 2, 3, 4,
                3, 2, 4, 4
        };
        randomRotate(points, 4, 4);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +-------+-------+
     * |       |       |
     * +-------+       |
     * |       |       |
     * +-------+-------+
     * |       |       |
     * +-------+       |
     * |       |       |
     * +-------+-------+ (4,4)
     */
    @PuzzleMethod(Type.GRID)
    public void grid4() {
        float points[] = {
                0, 0, 2, 1,
                0, 1, 2, 2,
                0, 2, 2, 3,
                0, 3, 2, 4,
                2, 0, 4, 2,
                2, 2, 4, 4
        };
        randomRotate(points, 4, 4);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +-------+-------+
     * |       |       |
     * +-------+       |
     * |       |       |
     * +---+---+-------+
     * |   |   |       |
     * |   |   |       |
     * |   |   |       |
     * +---+---+-------+ (4,4)
     */
    @PuzzleMethod(Type.GRID)
    public void grid5() {
        float points[] = {
                0, 0, 2, 1,
                0, 1, 2, 2,
                0, 2, 1, 4,
                1, 2, 2, 4,
                2, 0, 4, 2,
                2, 2, 4, 4
        };
        randomRotate(points, 4, 4);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +-------+----+
     * |       |    |
     * +-------+----+
     * |       |    |
     * +---+---+    |
     * |   |   |    |
     * +---+---+----+ (3,3)
     */
    @PuzzleMethod(Type.GRID)
    public void grid6() {
        float points[] = {
                0, 0, 2, 1,
                0, 1, 2, 2,
                0, 2, 1, 3,
                1, 2, 2, 3,
                2, 0, 3, 1,
                2, 1, 3, 3
        };
        randomRotate(points, 3, 3);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +---+---+----+
     * |   |   |    |
     * |   |   +----+
     * |   +---+    |
     * |   |   +----+
     * |   |   |    |
     * +---+---+----+ (6,6)
     */
    @PuzzleMethod(Type.GRID)
    public void grid7() {
        float points[] = {
                0, 0, 2, 6,
                2, 0, 4, 3,
                2, 3, 4, 6,
                4, 0, 6, 2,
                4, 2, 6, 4,
                4, 4, 6, 6
        };
        randomRotate(points, 6, 6);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +---+---+----+
     * |   |   |    |
     * |   |   +----+
     * +---+   |    |
     * |   |   +----+
     * |   |   |    |
     * +---+---+----+ (6,6)
     */
    @PuzzleMethod(Type.GRID)
    public void grid8() {
        float points[] = {
                0, 0, 2, 3,
                0, 3, 2, 6,
                2, 0, 4, 6,
                4, 0, 6, 2,
                4, 2, 6, 4,
                4, 4, 6, 6
        };
        randomRotate(points, 6, 6);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +------------+
     * |            |
     * +---+---+----+
     * |   |   |    |
     * |   +---+----+
     * |   |   |    |
     * +---+---+----+ (3,3)
     */
    @PuzzleMethod(Type.GRID)
    public void grid9() {
        float points[] = {
                0, 0, 3, 1,
                0, 1, 1, 3,
                1, 1, 2, 2,
                1, 2, 2, 3,
                2, 1, 3, 2,
                2, 2, 3, 3
        };
        randomRotate(points, 3, 3);
        setupSpec(points);
    }
}
