/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.R.integer;
import android.graphics.Color;

import java.util.Random;

import com.ucamera.uphoto.ImageEditControlActivity;

public class BrushConstant {
    //if from gotoGraffiti, the value is five, else is four, contains: Canvas, Clear, Eraser, Auto, Custom
    private static final int PREDEFINE_GRAFFITI_ACTION_ITEM = 4;
    private static final int PredefinedBrushCount = 5;

    public static final int BrushModeRandom = 0;
    public static final int BrushModeSelected = 1;
    //paint line style
    public static final int RainbowBrush = 1;
    public static final int GradientBrush = 2;
    public static final int PenInkBrush = 3;
    public static final int NeonBrush = 4;
//    public static final int SketchBrush = 5;
    public static final int LineBrush = 5;
    public static final int EmbossBrush = 6;
    public static final int EraserBrush = 7;

    //paint brush style
    public static final int DividingLine = 100;

    public static final int RectThinBrush = 101;
    public static final int RectDenseBrush = 102;
    public static final int RectFilamentousBrush = 103;
    public static final int RectBlurBrush = 104;
    public static final int RectThornsBrush = 105;
    public static final int RectOilyBrush = 106;
    public static final int RoundMistBrush = 107;
    public static final int RoundThinBrush = 108;
    public static final int RoundDenseBrush = 109;
    public static final int OvalBrush = 110;
    public static final int RectMeshBrush = 112;
    public static final int RectMistBrush = 113;

    private static final int[] mBrushStyles_dopod = {
        PenInkBrush, RainbowBrush, GradientBrush, NeonBrush, /*SketchBrush,*/ LineBrush, EmbossBrush,
        RectThinBrush, RectDenseBrush, RectFilamentousBrush, RectBlurBrush, RectThornsBrush, RectOilyBrush,
        RoundMistBrush, RoundThinBrush, RoundDenseBrush, OvalBrush, RectMeshBrush, RectMistBrush};
    private static final int[] mBrushStyles_default = {
            PenInkBrush, RainbowBrush, GradientBrush, NeonBrush, /*SketchBrush,*/ LineBrush, EmbossBrush,
            RectThinBrush, RectDenseBrush, RectFilamentousBrush, RectBlurBrush, RectThornsBrush, RectOilyBrush,
            RoundMistBrush, RoundThinBrush, RoundDenseBrush, OvalBrush, RectMeshBrush, RectMistBrush};
    public static final int[] mBrushStyles = ImageEditControlActivity.IsDopod ? mBrushStyles_dopod : mBrushStyles_default;

    private static final String[] CANVAS_COLORS = {"#ffffff", "#000000", "#fe4365", "#fc9d9a", "#f9cdad","#c8c8a9",
        "#83af9b", "#e31100","#cccaa9", "#9e9d83", "#aedd81","#6bc235", "#93e0ff", "#5ca7ba",
        "#17324d", "#994d52", "#d9742b","#e6b450", "#e3e6c3", "#4e1d4c","#576069", "#adc3c0",
        "#b9e3d9","#b3c587"};

    public static int getGraffitiFunctionItems() {
        return PREDEFINE_GRAFFITI_ACTION_ITEM;
    }

    public static int getRandomColor() {
        return Color.parseColor(CANVAS_COLORS[new Random().nextInt(CANVAS_COLORS.length)]);
    }

    public static int getGraffitiActionMaxIndex(boolean showCanvas) {
        return showCanvas ? PREDEFINE_GRAFFITI_ACTION_ITEM : PREDEFINE_GRAFFITI_ACTION_ITEM - 1;
    }

    public static int getGraffitiPredefineAllItems(boolean showCanvas) {
        return showCanvas ? (PREDEFINE_GRAFFITI_ACTION_ITEM + 1) + PredefinedBrushCount : PREDEFINE_GRAFFITI_ACTION_ITEM + PredefinedBrushCount;
    }
}
