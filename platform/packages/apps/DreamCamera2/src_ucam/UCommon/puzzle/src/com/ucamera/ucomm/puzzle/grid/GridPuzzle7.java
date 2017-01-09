/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucomm.puzzle.grid;

import com.ucamera.ucomm.puzzle.PuzzleSpec;

public class GridPuzzle7 extends GridBase {

    public GridPuzzle7() {
        mSpec = PuzzleSpec.create(7);
    }

    /* MAYBE rotated
     *  +---+------+---+
     *  |   |      |   |
     *  +---+      +---+
     *  |   |      |   |
     *  +---+      +---+
     *  |   |      |   |
     *  +---+------+---+(4,3)
     */
    @PuzzleMethod(Type.GRID)
    public void split() {
        float points[] = {
                0, 0, 1, 1,
                0, 1, 1, 2,
                0, 2, 1, 3,
                1, 0, 3, 3,
                3, 0, 4, 1,
                3, 1, 4, 2,
                3, 2, 4, 3
        };
        randomRotate(points, 4, 3);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +---+-------+
     * |   |       |
     * +---+---+---+
     * |   |   |   |
     * +---+---+---+
     * |       |   |
     * +-------+---+ (3,3)
     */
    @PuzzleMethod(Type.GRID)
    public void grid3() {
        float points[] = {
                0, 0, 1, 1,
                0, 1, 1, 2,
                0, 2, 2, 3,
                2, 2, 3, 3,
                2, 1, 3, 2,
                1, 0, 3, 1,
                1, 1, 2, 2
        };
        randomRotate(points, 3, 3);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +---+-------+
     * |   |       |
     * +---+---+---+
     * |   |   |   |
     * |   +---+---+
     * |   |   |   |
     * +---+---+---+ (3,3)
     */
    @PuzzleMethod(Type.GRID)
    public void grid4() {
        float points[] = {
                0, 1, 1, 3,
                0, 0, 1, 1,
                1, 0, 3, 1,
                2, 1, 3, 2,
                2, 2, 3, 3,
                1, 2, 2, 3,
                1, 1, 2, 2
        };
        randomRotate(points, 3, 3);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +---+-------+---+
     * |   |       |   |
     * |   +-------+   |
     * |   |       |   |
     * +---+       +---+
     * |   |       |   |
     * |   +-------+   |
     * |   |       |   |
     * +---+-------+---+ (4,4)
     */
    @PuzzleMethod(Type.GRID)
    public void grid5(){
        float points[] = {
                0, 0, 1, 2,
                0, 2, 1, 4,
                1, 3, 3, 4,
                3, 2, 4, 4,
                3, 0, 4, 2,
                1, 0, 3, 1,
                1, 1, 3, 3
        };
        randomRotate(points, 4, 4);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +---+---+-------+
     * |   |   |       |
     * |   |   +-------+
     * |   |   |       |
     * +---+---+       |
     * |   |   |       |
     * |   |   +-------+
     * |   |   |       |
     * +---+---+-------+ (4,4)
     */
    @PuzzleMethod(Type.GRID)
    public void grid6(){
        float points[] = {
                0, 0, 1, 2,
                0, 2, 1, 4,
                1, 0, 2, 2,
                1, 2, 2, 4,
                2, 0, 4, 1,
                2, 1, 4, 3,
                2, 3, 4, 4
        };
        randomRotate(points, 4, 4);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +---+-------+
     * |   |       |
     * +---+---+---+
     * |       |   |
     * +---+---+---+
     * |   |   |   |
     * +---+---+---+ (3,3)
     */
    @PuzzleMethod(Type.GRID)
    public void grid7() {
        float points[] = {
                0, 0, 1, 1,
                0, 1, 2, 2,
                0, 2, 1, 3,
                1, 0, 3, 1,
                2, 1, 3, 2,
                1, 2, 2, 3,
                2, 2, 3, 3
        };
        randomRotate(points, 3, 3);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +-------+-------+
     * |       |       |
     * |       +-------+
     * |       |       |
     * +---+---+-------+
     * |   |   |       |
     * |   |   +-------+
     * |   |   |       |
     * +---+---+-------+ (4,4)
     */
    @PuzzleMethod(Type.GRID)
    public void grid8(){
        float points[] = {
                0, 0, 2, 2,
                0, 2, 1, 4,
                1, 2, 2, 4,
                2, 0, 4, 1,
                2, 1, 4, 2,
                2, 2, 4, 3,
                2, 3, 4, 4
        };
        randomRotate(points, 4, 4);
        setupSpec(points);
    }

    /* MAYBE rotated
     * +---+---+-------+
     * |   |   |       |
     * +---+---+       |
     * |   |   |       |
     * +---+---+-------+
     * |       |       |
     * |       |       |
     * |       |       |
     * +-------+-------+ (4,4)
     */
    @PuzzleMethod(Type.GRID)
    public void grid9(){
        float points[] = {
                0, 0, 1, 1,
                0, 1, 1, 2,
                0, 2, 2, 4,
                1, 0, 2, 1,
                1, 1, 2, 2,
                2, 0, 4, 2,
                2, 2, 4, 4
        };
        randomRotate(points, 4, 4);
        setupSpec(points);
    }
}
