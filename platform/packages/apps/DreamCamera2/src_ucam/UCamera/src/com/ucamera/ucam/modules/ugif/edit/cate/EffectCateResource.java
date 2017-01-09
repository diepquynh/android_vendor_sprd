/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucam.modules.ugif.edit.cate;

import java.util.ArrayList;
import android.content.Context;
import com.android.camera2.R;

public class EffectCateResource {
    // CID 109348 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private Context mContext;
    private ArrayList<AdapterItem> mArrayList = new ArrayList<AdapterItem>();
    private static EffectCateResource mInstance;

    public static EffectCateResource getInstance(Context context) {
        if(mInstance == null) {
            mInstance = new EffectCateResource(context);
        }

        return mInstance;
    }

    public ArrayList<AdapterItem> getCateResource() {
        return mArrayList;
    }

    private EffectCateResource(Context context) {
        // CID 109348 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mContext = context;
        initResource();
    }

    private void initResource() {
//        //LOMO
//        mArrayList.add(new AdapterItem(R.drawable.ugif_edit_effect_cate_lomo, mContext.getString(R.string.ugif_edit_text_effect_lomo)));
//
//        //HDR
//        mArrayList.add(new AdapterItem(R.drawable.ugif_edit_effect_cate_hdr, mContext.getString(R.string.ugif_edit_text_effect_hdr)));
//
//        //SKIN
//        mArrayList.add(new AdapterItem(R.drawable.ugif_edit_effect_cate_skin, mContext.getString(R.string.ugif_edit_text_effect_skin)));
//
//        //VIVID LIGHT
//        mArrayList.add(new AdapterItem(R.drawable.ugif_edit_effect_cate_light, mContext.getString(R.string.ugif_edit_text_effect_lightcolor)));
//
//        //SKETCH
//        mArrayList.add(new AdapterItem(R.drawable.ugif_edit_effect_cate_sketch, mContext.getString(R.string.ugif_edit_text_effect_sketch)));
//
//        //COLOR FULL
//        mArrayList.add(new AdapterItem(R.drawable.ugif_edit_effect_cate_color, mContext.getString(R.string.ugif_edit_text_effect_colorfull)));
//
//        //FUNNY
//        mArrayList.add(new AdapterItem(R.drawable.ugif_edit_effect_cate_fun, mContext.getString(R.string.ugif_edit_text_effect_funny)));
//
//        //NOSTALGIA
//        mArrayList.add(new AdapterItem(R.drawable.ugif_edit_effect_cate_nostalgia, mContext.getString(R.string.ugif_edit_text_effect_nostalgia)));
//
//        //BLACKWHITE
//        mArrayList.add(new AdapterItem(R.drawable.ugif_edit_effect_cate_bw, mContext.getString(R.string.ugif_edit_text_effect_bw)));
//
//        //DEFORM
//        mArrayList.add(new AdapterItem(R.drawable.ugif_edit_effect_cate_deform, mContext.getString(R.string.ugif_edit_text_effect_deform)));
    }
}
