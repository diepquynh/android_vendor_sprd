package com.ucamera.ugallery.gallery.privateimage;
/**
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */
import java.io.File;
import java.io.IOException;
import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import com.ucamera.ugallery.R;
import com.ucamera.ugallery.gallery.privateimage.util.PasswordUtils;
public class ImagePassWordDialog extends Dialog implements View.OnClickListener {
    private static final String TAG = "ImagePassWord";
    private Activity mActivity;
    // CID 109274 : UuF: Unused field (FB.UUF_UNUSED_FIELD)
    // private View mContentView;
    private TextView mSetPwd;
    private boolean mIsPwdExist;
    private EditText mEntryPwd;
    private EditText mEntryPwdConfirm;
    private PrivateAbulmCallback mCallback;
    private boolean mIsSetPwd;
    public ImageView mPasswordCancel;
    private TextView mWarningText;
    public interface PrivateAbulmCallback {
        public void controlPrivateAbulmNO();
        public void controlPrivateAbulmOFF();
    }

    public ImagePassWordDialog(Activity activity ,boolean isSetPwd, PrivateAbulmCallback callback) {
        super(activity, android.R.style.Theme_Translucent_NoTitleBar_Fullscreen);
        mActivity = activity;
        mIsSetPwd = isSetPwd;
        mIsPwdExist = PasswordUtils.isPasswordFileExist();
        mCallback = callback;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.password_setting_dialog);
        findViewById(R.id.set_password_complete).setOnClickListener(this);
        mPasswordCancel = (ImageView)findViewById(R.id.set_password_cancel);
        mWarningText = (TextView)findViewById(R.id.tv_warning_set_pwd);
        mPasswordCancel.setOnClickListener(this);
        mEntryPwd = (EditText)findViewById(R.id.et_entry_pwd);
        mEntryPwdConfirm = (EditText)findViewById(R.id.et_entry_pwd_confirm);
        mEntryPwd.setFocusable(true);
        mEntryPwd.setFocusableInTouchMode(true);
        mEntryPwd.requestFocus();
        Timer timer = new Timer();
        timer.schedule(new TimerTask(){
            public void run(){
                InputMethodManager inputManager =
                (InputMethodManager) mEntryPwd.getContext().getSystemService(
                        Context.INPUT_METHOD_SERVICE);
                inputManager.showSoftInput(mEntryPwd, 0);
            }
        },
        300);
        mSetPwd = (TextView)findViewById(R.id.tv_title_set_pwd);
        if (mIsPwdExist) {
            mEntryPwdConfirm.setVisibility(View.INVISIBLE);
            mWarningText.setVisibility(View.INVISIBLE);
            mSetPwd.setText(R.string.text_settings_intput_pwd);
        } else {
            mSetPwd.setText(R.string.text_settings_setpassword);
        }
    }
/*    @Override
    public void setOnDismissListener(OnDismissListener listener) {
        super.setOnDismissListener(listener);
        if(mCallback != null) {
            mCallback.controlPrivateAbulmOFF();
        }
    }*/
    @Override
    public void onBackPressed() {
        if(mIsPwdExist || mIsSetPwd) {
            if(mCallback != null) {
                mCallback.controlPrivateAbulmOFF();
            }
            dismiss();
        } else {
            super.onBackPressed();
        }
    }
    @Override
    public void onClick(View v) {
        switch (v.getId()) {
        case R.id.set_password_complete:
            String pwd = mEntryPwd.getText().toString().trim();
            if(mIsPwdExist) {
                if (TextUtils.isEmpty(pwd)) {
                    Toast.makeText(mActivity, mActivity.getString(R.string.text_settings_no_password), Toast.LENGTH_SHORT).show();
                    return;
                }
                String realpwd = PasswordUtils.readValue(PasswordUtils.KEY_PASSWORD);
                /*
                 * FIX BUG: 6131
                 * BUG COMMENT: avoid null pointer exception
                 * DATE: 2014-03-26
                 */
                if ((PasswordUtils.MD5Encode(pwd).equals(realpwd))) {
                    if(mCallback != null) {
                        mCallback.controlPrivateAbulmNO();
                    }
                    if(mIsSetPwd) {
                        File file = new File(PasswordUtils.PASSWORD_PATH);
                        if(file.exists()) {
                            file.delete();
                        }
                    }
                } else {
                    Toast.makeText(mActivity, mActivity.getString(R.string.text_settings_wrong_password), Toast.LENGTH_SHORT).show();
                    mEntryPwd.setText("");
                    return;
                }
            } else {
                String pwd_confirm = mEntryPwdConfirm.getText().toString().trim();
                if (TextUtils.isEmpty(pwd) || TextUtils.isEmpty(pwd_confirm)) {
                    Toast.makeText(mActivity, mActivity.getString(R.string.text_settings_no_password), Toast.LENGTH_SHORT).show();
                    return;
                }
                if (pwd.equals(pwd_confirm)) {
                    File file = new File(PasswordUtils.PASSWORD_PATH);
                    if(!file.exists()) {
                        try {
                          file.createNewFile();
                      } catch (IOException e) {
                          e.printStackTrace();
                      }
                    }
                    PasswordUtils.saveFile(PasswordUtils.KEY_PASSWORD, PasswordUtils.MD5Encode(pwd));
                }else{
                    Toast.makeText(mActivity, mActivity.getString(R.string.text_settings_different_password), Toast.LENGTH_SHORT).show();
                    mEntryPwd.setText("");
                    mEntryPwdConfirm.setText("");
                    return;
                }
            }
            dismiss();
            break;
        case R.id.set_password_cancel:
            if(mIsPwdExist || mIsSetPwd) {
                if(mCallback != null) {
                    mCallback.controlPrivateAbulmOFF();
                }
                dismiss();
            } else {
                dismiss();
            }
            break;
        default:
            break;
        }
    }
}
