package com.dream.camera.settings;

import com.android.camera.settings.Keys;
import com.android.camera2.R;
import com.android.internal.app.ToolbarActionBar;
import com.dream.camera.settings.DataModuleBasic.DataStorageStruct;
import com.dream.camera.settings.DataModuleManager.ResetListener;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.TypedArray;
import android.preference.ListPreference;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;

public class DreamUIPreferenceSettingLayout extends LinearLayout implements
        ResetListener {

    public interface SettingUIListener{
        public void onSettingUIHide();
    }

    public DreamUIPreferenceSettingLayout(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
    }

    public DreamUIPreferenceSettingLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
    }

    public DreamUIPreferenceSettingLayout(Context context, AttributeSet attrs,
            int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        // TODO Auto-generated constructor stub
    }

    public DreamUIPreferenceSettingLayout(Context context, AttributeSet attrs,
            int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        // TODO Auto-generated constructor stub
    }

    Activity mActivity;
    DreamUIPreferenceSettingFragment mCurrentFragment;
    ImageView mReturnButton;
    FrameLayout mSettingFragContainer;
    SettingUIListener mSettingUIListener;


    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mActivity = (Activity) getContext();
        DataModuleManager.getInstance(mActivity).addListener(this);
        mReturnButton = (ImageView) findViewById(R.id.return_image);
        mSettingFragContainer = (FrameLayout) findViewById(R.id.setting_frament_container);
        mReturnButton.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                changeVisibilty(GONE);
            }
        });

    }

    /*SPRD:fix bug607898 fix setting ui when back from home/secure camera, last time pause camera by pressing home @{ */
    public boolean isNeedUpdateModule() {
        return mCurrentFragment != null && mCurrentFragment.getDataSetting() != null 
                    && !mCurrentFragment.getDataSetting().equals(DataModuleManager.getInstance(mActivity).getCurrentDataSetting());
    }

    public void updateModule(SettingUIListener listener) {
        mCurrentFragment.releaseResource();
        mCurrentFragment = new DreamUIPreferenceSettingFragment();
        mActivity.getFragmentManager().beginTransaction()
                .replace(R.id.setting_frament_container, mCurrentFragment)
                .commitAllowingStateLoss();//SPRD:fix bug 611031
        mSettingUIListener = listener;
    }
    /* @} */

    public void changeModule(SettingUIListener listener){

        if(mCurrentFragment != null){
            mCurrentFragment.releaseResource();
        }

        mCurrentFragment = new DreamUIPreferenceSettingFragment();
        mActivity.getFragmentManager().beginTransaction()
                .replace(R.id.setting_frament_container, mCurrentFragment)
                .commitAllowingStateLoss();//SPRD:fix bug 611031
        mSettingUIListener = listener;
    }

    public void changeVisibilty(int visiblty){
        this.setVisibility(visiblty);
        if(visiblty == GONE && mSettingUIListener != null){
            mSettingUIListener.onSettingUIHide();
        }
    }

    @Override
    public void onSettingReset() {
        if(mCurrentFragment != null){
            mCurrentFragment.resetSettings();
        }
        changeVisibilty(GONE);
    }

    public boolean onBackPressed() {
        if(this.getVisibility() == View.VISIBLE){
            changeVisibilty(GONE);
            return true;
        }
        return false;
    }
    public void dialogDismiss(String key){
        if(mCurrentFragment != null){
            mCurrentFragment.dialogDismiss(key);
        }
    }
}
