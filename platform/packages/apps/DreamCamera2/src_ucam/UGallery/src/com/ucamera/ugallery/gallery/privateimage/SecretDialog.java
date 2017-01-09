package com.ucamera.ugallery.gallery.privateimage;
/*
 * Copyright (C) 2011,2014 Thundersoft Corporation
 * All rights Reserved
 */
import java.util.ArrayList;

import android.app.Activity;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.widget.TextView;

import com.ucamera.ugallery.ImageGallery;
import com.ucamera.ugallery.MediaScanner;
import com.ucamera.ugallery.R;
import com.ucamera.ugallery.MediaScanner.ScannerFinishCallBack;

public class SecretDialog extends Dialog implements View.OnClickListener{
    private static final String TAG = "SecretDialog";
    // CID 109296 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private Activity mActivity;
    /* SPRD: CID 109173 : UuF: Unused field (FB.UUF_UNUSED_FIELD) @{
    private View mContentView;
    private Handler mHandler;
    private ArrayList<String> mList;
    @} */
    private SecretDialogOnClickListener mListener;
    public interface SecretDialogOnClickListener {
        void secretDialogOnClick();
    }
    public SecretDialog(Activity activity ,SecretDialogOnClickListener listener) {
        super(activity, R.style.SecretDialog);
        // CID 109296 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mActivity = activity;
        mListener = listener;
    }
/*    public SecretDialog(Activity activity ,Handler handler, ArrayList<String> list) {
        super(activity, R.style.SecretDialog);
        mActivity = activity;
        mHandler = handler;
        mList = list;
    }*/
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.secret_dialog);
        findViewById(R.id.secret_dialog_ok).setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
        case R.id.secret_dialog_ok:
           /* ProgressDialog mBackupAllimagesFinishDialog = ProgressDialog.show(mActivity, null, mActivity.getResources().getString(
                    R.string.text_waiting), true, false);
            new MediaScanner(mActivity, new ScannerFinishCallBack() {
                @Override
                public void scannerComplete() {
                    mHandler.sendEmptyMessage(ImageGallery.BACK_UP_ALLIMAGES_FINISH);
                }
            }).scanFile(mList, null);*/
            mListener.secretDialogOnClick();
            break;
        default:
            break;
        }
        dismiss();
    }
    @Override
    public void onBackPressed() {
    }
}
