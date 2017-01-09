package com.sprd.engineermode;

import android.content.Context;
import android.preference.SwitchPreference;
import android.util.AttributeSet;
import android.view.View;

public class EMSwitchPreference extends SwitchPreference {
    
    public EMSwitchPreference(Context context, AttributeSet attrs){
        super(context, attrs);
    }
    
    @Override
    protected void onBindView(View view){
        super.onBindView(view);
    }
    
    @Override
    protected void onClick() {
        super.onClick();

    }
}