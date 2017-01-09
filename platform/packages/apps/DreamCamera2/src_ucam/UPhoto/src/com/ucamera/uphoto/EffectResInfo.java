/**
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.util.SparseIntArray;

import java.util.HashMap;


public class EffectResInfo {

    private final static String FILTER_TYPE_FILM = "effect_film.fs";
    private final static String FILTER_TYPE_KOREA = "effect_korea.fs";
    private final static String FILTER_TYPE_JAPAN = "effect_japan.fs";
    private final static String FILTER_TYPE_JIANGNAN = "effect_jiangnan.fs";
    private final static String FILTER_TYPE_AMERICA = "effect_america.fs";
    private final static String FILTER_TYPE_FRANCE = "effect_france.fs";
    private final static String FILTER_TYPE_DESERT = "effect_desert.fs";

    private final static String FILTER_TYPE_AUTUMN = "effect_autumn.fs";
    private final static String FILTER_TYPE_RAINBOW = "effect_rainbrown.fs";
    private final static String FILTER_TYPE_FLOWERINESS = "effect_floweriness.fs";
    private final static String FILTER_TYPE_FLY = "effect_fly.fs";
    private final static String FILTER_TYPE_LOTUS = "effect_lotus.fs";
    private final static String FILTER_TYPE_BLESS = "effect_bless.fs";
    private final static String FILTER_TYPE_SPARKLING = "effect_sparkling.fs";
    private final static String FILTER_TYPE_HOPE = "effect_hope.fs";
    private final static String FILTER_TYPE_HAPPY = "effect_happy.fs";
    private final static String FILTER_TYPE_BLOOM = "effect_bloom.fs";
    private final static String FILTER_TYPE_COLORFUL = "effect_colorful.fs";
    private final static String FILTER_TYPE_BRIGHT = "effect_bright.fs";
    private final static String FILTER_TYPE_CUTE = "effect_cute.fs";
    private final static String FILTER_TYPE_BLUE = "effect_blue.fs";
    private final static String FILTER_TYPE_GRAYSKETCH = "effect_pencil.fs";
    private final static String FILTER_TYPE_COLORPENCIL = "filter_103_colorpencil.fs";
    private final static String FILTER_TYPE_DREAM = "effect_dream.fs";
    private final static String FILTER_TYPE_FRESH = "effect_fresh.fs";
    private final static String FILTER_TYPE_INSTAGRAM_1 = "filter_041_instagram1.fs";
    private final static String FILTER_TYPE_INSTAGRAM_3 = "filter_043_instagram3.fs";

    private static HashMap<String, Integer> mFilterMap = new HashMap<String, Integer>();
    private static SparseIntArray mNeedRotatedTable = new SparseIntArray();
    static {
        mFilterMap.put(FILTER_TYPE_FILM, R.drawable.res_instagram_lomocolor);
        mFilterMap.put(FILTER_TYPE_KOREA, R.drawable.res_instagram_lomocolor);
        mFilterMap.put(FILTER_TYPE_JAPAN, R.drawable.res_instagram_lomocolor);
        mFilterMap.put(FILTER_TYPE_JIANGNAN, R.drawable.res_instagram_lomocolor);
        mFilterMap.put(FILTER_TYPE_AMERICA, R.drawable.res_instagram_lomocolor);
        mFilterMap.put(FILTER_TYPE_FRANCE, R.drawable.res_instagram_lomocolor);
        mFilterMap.put(FILTER_TYPE_DESERT, R.drawable.res_instagram_lomocolor);
        mFilterMap.put(FILTER_TYPE_INSTAGRAM_1, R.drawable.res_instagram_lomocolor);
        mFilterMap.put(FILTER_TYPE_INSTAGRAM_3, R.drawable.res_instagram_lomocolor);

        mFilterMap.put(FILTER_TYPE_AUTUMN, R.drawable.autumn_1);
        mFilterMap.put(FILTER_TYPE_RAINBOW, R.drawable.rainbow_1);
        mFilterMap.put(FILTER_TYPE_FLOWERINESS, R.drawable.effect_floweriness_1);
        mFilterMap.put(FILTER_TYPE_FLY, R.drawable.effect_fly_1);
        mFilterMap.put(FILTER_TYPE_LOTUS, R.drawable.effect_lotus_1);
        mFilterMap.put(FILTER_TYPE_BLESS, R.drawable.effect_bless_1);
        mFilterMap.put(FILTER_TYPE_SPARKLING, R.drawable.effect_sparkling_1);
        mFilterMap.put(FILTER_TYPE_HOPE, R.drawable.effect_hope_1);
        mFilterMap.put(FILTER_TYPE_HAPPY, R.drawable.effect_happy_1);
        mFilterMap.put(FILTER_TYPE_BLOOM, R.drawable.effect_bloom_1);
        mFilterMap.put(FILTER_TYPE_COLORFUL, R.drawable.effect_colorful_1);
        mFilterMap.put(FILTER_TYPE_BRIGHT, R.drawable.effect_bright_1);
        mFilterMap.put(FILTER_TYPE_CUTE, R.drawable.effect_cute_1);
        mFilterMap.put(FILTER_TYPE_BLUE, R.drawable.effect_blue_1);
        mFilterMap.put(FILTER_TYPE_GRAYSKETCH, R.drawable.effect_pencil_1);
        mFilterMap.put(FILTER_TYPE_COLORPENCIL, R.drawable.effect_colorpencil_1);
        mFilterMap.put(FILTER_TYPE_DREAM, R.drawable.dream_1);
        mFilterMap.put(FILTER_TYPE_FRESH, R.drawable.effect_fresh_1);

        /*
         * drawable as resource texture, need to rotate, in the table
         */
        mNeedRotatedTable.put(R.drawable.effect_floweriness_1, 0);
        mNeedRotatedTable.put(R.drawable.effect_fly_1, 0);
        mNeedRotatedTable.put(R.drawable.effect_lotus_1, 0);
        mNeedRotatedTable.put(R.drawable.effect_bless_1, 0);
        mNeedRotatedTable.put(R.drawable.effect_sparkling_1, 0);
        mNeedRotatedTable.put(R.drawable.effect_hope_1, 0);
        mNeedRotatedTable.put(R.drawable.effect_happy_1, 0);
        mNeedRotatedTable.put(R.drawable.effect_bloom_1, 0);
        mNeedRotatedTable.put(R.drawable.effect_colorful_1, 0);
        mNeedRotatedTable.put(R.drawable.effect_bright_1, 0);
        mNeedRotatedTable.put(R.drawable.effect_cute_1, 0);
        mNeedRotatedTable.put(R.drawable.effect_blue_1, 0);
        mNeedRotatedTable.put(R.drawable.effect_pencil_1, 0);
        mNeedRotatedTable.put(R.drawable.effect_colorpencil_1, 0);
        mNeedRotatedTable.put(R.drawable.effect_colorful_1, 0);
        mNeedRotatedTable.put(R.drawable.dream_1, 0);
    }

    // whether the shader need a resource texture
    public static boolean isNeedResourceTexture(String effectid) {
        return mFilterMap.containsKey(effectid);
    }

    public static int getDrawableID(String effectid) {
        if (isNeedResourceTexture(effectid)) {
            return mFilterMap.get(effectid).intValue();
        }

        throw new RuntimeException("The effect " + effectid + " has no resource!");
    }

    public static boolean isResourceDrawableRotated(int drawableId) {
        return mNeedRotatedTable.indexOfKey(drawableId) >= 0;
    }
}
