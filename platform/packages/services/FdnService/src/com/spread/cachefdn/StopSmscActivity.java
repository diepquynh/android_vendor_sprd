package com.spread.cachefdn;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.Button;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import com.spread.cachefdn.R;

public class StopSmscActivity extends Activity{
    private Button mConfirm ,mCancle;
    private Intent mIntent;
    private int mSubId;
    private String mSmcsNumber;
    private String mSimLable;

    public static final String SUB_ID_EXTRA = "com.android.phone.settings.SubscriptionInfoHelper.SubscriptionId";
    public static final String SUB_LABEL_EXTRA = "com.android.phone.settings.SubscriptionInfoHelper.SubscriptionLabel";
    // SubscriptionInfo subscription
    // subscription.getDisplayName().toString()

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        setContentView(R.layout.smsc_dialog_activity);

        //getWindow().setFlags(LayoutParams.FLAG_NOT_TOUCH_MODAL, LayoutParams.FLAG_NOT_TOUCH_MODAL);


        //initUiWidget();
        mIntent = getIntent();
        if(mIntent != null){
            mSubId = mIntent.getIntExtra(SUB_ID_EXTRA, -1);
            mSmcsNumber = mIntent.getStringExtra("smcsNumber");
            mSimLable = mIntent.getStringExtra(SUB_LABEL_EXTRA);
            Log.i("FdnService", "StopSmscActivity---->mSmcsNumber      :"+mSmcsNumber +","
                    +"mSimLable =" + mSimLable);
        }
        show();
    }

//    private void initUiWidget() {
//        mConfirm = (Button) findViewById(R.id.confirm);
//        mCancle = (Button) findViewById(R.id.cancle);
//
//        mConfirm.setOnClickListener(this);
//        mCancle.setOnClickListener(this);
//
//    }

    private void show() {
        AlertDialog.Builder editDialog = new AlertDialog.Builder(this);
        editDialog.setTitle(R.string.fdn_add_dialog_title)
                .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        startToFdnUiForAdd();
                        finishActivity();
                    }
                })
                .setNegativeButton(android.R.string.cancel,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                                int which) {
                                finishActivity();
                            }
                        })
                .setOnDismissListener(new DialogInterface.OnDismissListener() {

                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        finishActivity();
                    }
                }).show();
    }

    private void startToFdnUiForAdd(){
        Intent it = new Intent();
        it.setComponent(new ComponentName("com.android.callsettings",
                "com.android.callsettings.fdnsettings.FdnSetting"));
        if(mSubId > 0){
            it.putExtra(SUB_ID_EXTRA, mSubId);
        }
        if(!TextUtils.isEmpty(mSimLable)){
            it.putExtra(SUB_LABEL_EXTRA, mSimLable);
        }
        if(!TextUtils.isEmpty(mSmcsNumber)){
            it.putExtra("smcsNumber", mSmcsNumber);
        }
        this.startActivity(it);
    }

//    @Override
//    public void onClick(View v) {
//        switch (v.getId()) {
//        case R.id.confirm:
//            startToFdnUiForAdd();
//            break;
//        case R.id.cancle:
//            break;
//        }
//        finish();
//    }
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK
                 && event.getRepeatCount() == 0) {
             return true;
         }
         return super.onKeyDown(keyCode, event);
    }

    private void finishActivity() {
        StopSmscActivity.this.finish();
    }

}

