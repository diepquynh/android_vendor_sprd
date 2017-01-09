
package com.sprd.validationtools.itemstest;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemProperties;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageButton;

import com.sprd.validationtools.R;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.Const;

public class KeyTestActivity extends BaseActivity {
    private static final String TAG = "KeyTestActivity";
    private ImageButton mHomeButton;
    private ImageButton mMenuButton;
    private ImageButton mBackButton;
    private ImageButton mVolumeUpButton;
    private ImageButton mVolumeDownButton;
    private ImageButton mCameraButton;
    private byte keyPressedFlag = 0;
    private byte keySupportFlag = 0;
    private boolean isHideCamera = false;
    private AlertDialog mCameraConfirmDialog;
    public Handler mHandler = new Handler();
    public boolean shouldBack = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.key_test);
        setTitle(R.string.key_test);
        mHomeButton = (ImageButton) findViewById(R.id.home_button);
        mMenuButton = (ImageButton) findViewById(R.id.menu_button);
        mBackButton = (ImageButton) findViewById(R.id.back_button);
        mVolumeUpButton = (ImageButton) findViewById(R.id.volume_up_button);
        mVolumeDownButton = (ImageButton) findViewById(R.id.volume_down_button);
        mCameraButton = (ImageButton) findViewById(R.id.camera_button);
        showHasCameraDialog();
    }

    protected void showHasCameraDialog() {

        Dialog cameraKeyDialog = new AlertDialog.Builder(this)
                .setIconAttribute(android.R.attr.alertDialogIcon)
                .setTitle(this.getString(R.string.has_camera_title))
                .setCancelable(false)
                .setNegativeButton(R.string.has_camera_key, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        // TODO Auto-generated method stub
                        isHideCamera = false;
                        showKey(isHideCamera);
                        dialog.dismiss();
                    }
                })
                .setPositiveButton(R.string.no_camera_key, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        // TODO Auto-generated method stub
                        isHideCamera = true;
                        showKey(isHideCamera);
                        dialog.dismiss();
                    }
                }).create();
        cameraKeyDialog.show();
    }

    private void showKey(boolean hideCamera) {
        if (Const.isHomeSupport(this)) {
            mHomeButton.setVisibility(View.VISIBLE);
            keySupportFlag |= 1;
        }else {
/* SPRD: modify 20140529 Spreadtrum of 305634 MMI test,lack of button which is on the right 0f "Home" @{ */
            mHomeButton.setVisibility(View.GONE);
/* @} */
        }
        if (Const.isBackSupport(this)) {
            mBackButton.setVisibility(View.VISIBLE);
            keySupportFlag |= 2;
        }else {
/* SPRD: modify 20140529 Spreadtrum of 305634 MMI test,lack of button which is on the right 0f "Home" @{ */
            mBackButton.setVisibility(View.GONE);
/* @} */
        }
        if (Const.isMenuSupport(this)) {
            mMenuButton.setVisibility(View.VISIBLE);
            keySupportFlag |= 4;
        }else {
            mMenuButton.setVisibility(View.GONE);
        }
        if (Const.isCameraSupport() && !hideCamera) {
            mCameraButton.setVisibility(View.VISIBLE);
            keySupportFlag |= 8;
        } else if (hideCamera) {
            mCameraButton.setVisibility(View.GONE);
        }
        if (Const.isVolumeUpSupport()) {
            mVolumeUpButton.setVisibility(View.VISIBLE);
            keySupportFlag |= 16;
        }
        if (Const.isVolumeDownSupport()) {
            mVolumeDownButton.setVisibility(View.VISIBLE);
            keySupportFlag |= 32;
        }
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (KeyEvent.KEYCODE_HOME == keyCode) {
            mHomeButton.setPressed(true);
            keyPressedFlag |= 1;
        } else if (KeyEvent.KEYCODE_BACK == keyCode) {
            if (!mBackButton.isPressed()) {
                mBackButton.setPressed(true);
                keyPressedFlag |= 2;
            } else {
                return super.onKeyDown(keyCode, event);
            }
        } else if (KeyEvent.KEYCODE_APP_SWITCH == keyCode) {
            mMenuButton.setPressed(true);
            keyPressedFlag |= 4;
        } else if (KeyEvent.KEYCODE_CAMERA == keyCode) {
            mCameraButton.setPressed(true);
            keyPressedFlag |= 8;
        } else if (KeyEvent.KEYCODE_VOLUME_UP == keyCode) {
            mVolumeUpButton.setPressed(true);
            keyPressedFlag |= 16;
        } else if (KeyEvent.KEYCODE_VOLUME_DOWN == keyCode) {
            mVolumeDownButton.setPressed(true);
            keyPressedFlag |= 32;
        }
        if (keySupportFlag == keyPressedFlag) {
            BaseActivity.shouldCanceled = false;
            //showResultDialog(getString(R.string.key_test_info));
            storeRusult(true);
            mHandler.postDelayed(new Runnable(){
                @Override
                public void run() {
                    finish();
                }
            }, 100);
        }
        return true;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        return super.onKeyUp(keyCode, event);
    }
}
