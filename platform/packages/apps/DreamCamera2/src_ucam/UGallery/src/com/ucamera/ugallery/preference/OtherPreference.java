/*
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ugallery.preference;

import android.content.Context;
import android.preference.Preference;
import android.util.AttributeSet;

import com.ucamera.ugallery.UGalleryConst;

public class OtherPreference extends Preference {

    OtherPerenceOnClickListener mListener;
    public OtherPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public interface OtherPerenceOnClickListener{
        public void onClickFeedback();
        public void onClickAboutUs();
        public void onClickShowTips();
    }

    public void setListener(OtherPerenceOnClickListener l){
        mListener = l;
    }

    @Override
    protected void onClick() {
        super.onClick();
        if(mListener == null) return;
        if(getKey().equals(UGalleryConst.KEY_OTHER_FEEDBACK)){
            mListener.onClickFeedback();
        } else if(getKey().equals(UGalleryConst.KEY_OTHER_ABOUTUS)){
            mListener.onClickAboutUs();
        } else if (getKey().equals(UGalleryConst.KEY_SETTING_OTHER_SHOW_TIPS)){
            mListener.onClickShowTips();
        }
    }

}
