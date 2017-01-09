package com.ucamera.ucam.modules.ufilter;

import android.content.Context;
import com.ucamera.ucam.modules.utils.UiUtils;
import com.thundersoft.advancedfilter.TsAdvancedFilterNative;

public class UcamFilterPhotoUtils {
    private final static String TAG = "UcamFilterPhotoUtils";

    public static void initFilter(Context context) {
        UiUtils.initialize(context);
        int[] types = {TsAdvancedFilterNative.ADVANCEDFILTER_FILM, TsAdvancedFilterNative.ADVANCEDFILTER_NOSTALGIA,
                TsAdvancedFilterNative.ADVANCEDFILTER_BLACKWHITE, TsAdvancedFilterNative.ADVANCEDFILTER_FRESH,
                TsAdvancedFilterNative.ADVANCEDFILTER_HOPE, TsAdvancedFilterNative.ADVANCEDFILTER_REFLECTION,
                TsAdvancedFilterNative.ADVANCEDFILTER_INVERT, TsAdvancedFilterNative.ADVANCEDFILTER_1839
                /*************add new filters ********************/
                ,TsAdvancedFilterNative.ADVANCEDFILTER_GREEN,TsAdvancedFilterNative.ADVANCEDFILTER_SKETCH,
                TsAdvancedFilterNative.ADVANCEDFILTER_NEGATIVE,TsAdvancedFilterNative.ADVANCEDFILTER_POSTERIZE
                ,TsAdvancedFilterNative.ADVANCEDFILTER_NOSTALGIA,TsAdvancedFilterNative.ADVANCEDFILTER_RED
                ,TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_LEFT,TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_RIGHT
                ,TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_UP,TsAdvancedFilterNative.ADVANCEDFILTER_SYMMETRY_DOWN
                ,TsAdvancedFilterNative.ADVANCEDFILTER_REVERSAL, TsAdvancedFilterNative.ADVANCEDFILTER_JAPAN
                ,TsAdvancedFilterNative.ADVANCEDFILTER_BRIGHT, TsAdvancedFilterNative.ADVANCEDFILTER_CUTE
                ,TsAdvancedFilterNative.ADVANCEDFILTER_BLUE, TsAdvancedFilterNative.ADVANCEDFILTER_FLOWERINESS
                ,TsAdvancedFilterNative.ADVANCEDFILTER_FLY, TsAdvancedFilterNative.ADVANCEDFILTER_LOTUS
                ,TsAdvancedFilterNative.ADVANCEDFILTER_BLESS, TsAdvancedFilterNative.ADVANCEDFILTER_SPARKLING
                ,TsAdvancedFilterNative.ADVANCEDFILTER_HAPPY
                ,TsAdvancedFilterNative.ADVANCEDFILTER_BLOOM, TsAdvancedFilterNative.ADVANCEDFILTER_MOSAIC
                ,TsAdvancedFilterNative.ADVANCEDFILTER_EDGE
                ,TsAdvancedFilterNative.ADVANCEDFILTER_COLORPENCIL, TsAdvancedFilterNative.ADVANCEDFILTER_GRAYSKETCHPENCIL
                ,TsAdvancedFilterNative.ADVANCEDFILTER_RAINBROW, TsAdvancedFilterNative.ADVANCEDFILTER_AUTUMN
                ,TsAdvancedFilterNative.ADVANCEDFILTER_DUSK, TsAdvancedFilterNative.ADVANCEDFILTER_NEON
                ,TsAdvancedFilterNative.ADVANCEDFILTER_PENCIL1,TsAdvancedFilterNative.ADVANCEDFILTER_PENCIL2
                ,TsAdvancedFilterNative.ADVANCEDFILTER_EMBOSS,TsAdvancedFilterNative.ADVANCEDFILTER_THERMAL
                ,TsAdvancedFilterNative.ADVANCEDFILTER_CRAYON, TsAdvancedFilterNative.ADVANCEDFILTER_ENGRAVING
                ,TsAdvancedFilterNative.ADVANCEDFILTER_KOREA, TsAdvancedFilterNative.ADVANCEDFILTER_AMERICA
                ,TsAdvancedFilterNative.ADVANCEDFILTER_FRANCE, TsAdvancedFilterNative.ADVANCEDFILTER_DESERT
                ,TsAdvancedFilterNative.ADVANCEDFILTER_JIANGNAN,TsAdvancedFilterNative.ADVANCEDFILTER_PEEP1
                ,TsAdvancedFilterNative.ADVANCEDFILTER_PEEP2,TsAdvancedFilterNative.ADVANCEDFILTER_PEEP3
                ,TsAdvancedFilterNative.ADVANCEDFILTER_PEEP4,TsAdvancedFilterNative.ADVANCEDFILTER_PEEP5
                ,TsAdvancedFilterNative.ADVANCEDFILTER_PEEP6,TsAdvancedFilterNative.ADVANCEDFILTER_PEEP7
                ,TsAdvancedFilterNative.ADVANCEDFILTER_PEEP8,TsAdvancedFilterNative.ADVANCEDFILTER_PEEP9
                ,TsAdvancedFilterNative.ADVANCEDFILTER_PEEP10,TsAdvancedFilterNative.ADVANCEDFILTER_PEEP11
                ,TsAdvancedFilterNative.ADVANCEDFILTER_PEEP12,TsAdvancedFilterNative.ADVANCEDFILTER_PEEP13};
        TsAdvancedFilterNative.init(types);
    }
}
