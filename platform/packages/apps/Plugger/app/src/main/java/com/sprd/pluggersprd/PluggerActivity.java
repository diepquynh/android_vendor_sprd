package com.sprd.pluggersprd;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.TextView;
import android.support.v4.content.ContextCompat;
import android.support.v4.app.ActivityCompat;

import com.android.sms.MmsFailureException;
import com.android.sms.MmsSender;

import java.util.List;
import com.sprd.pluggersprd.R;
import android.telephony.TelephonyManager;
import android.content.Context;

public class PluggerActivity extends Activity {

    private static final String TAG = "PluggerActivity";
    private static final int CLOSE_ALERTDIALOG = 1;
    private static final int CLOSE_PLUGGER_ACTIVITY = 2;
    private static final int NO_SERVICE = 3;

    AlertDialog.Builder builder;
    AlertDialog mAlertDialog;
    private static final int FontSize = 24;
    private static final int PERMISSIONS_REQUEST_READ_PHONE_STATE = 5;
    private static final int PERMISSIONS_REQUEST_SEND_SMS = 6;
    private static final int PERMISSIONS_READ_EXTERNAL_STORAGE = 7;
   private Context mContext;

    private  Handler mainHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case CLOSE_PLUGGER_ACTIVITY:
//                    if (mAlertDialog != null && mAlertDialog.isShowing()) {
//                        mAlertDialog.dismiss();
//                    }
                    finish();
                    break;
                case CLOSE_ALERTDIALOG:
//                    if (mAlertDialog != null && mAlertDialog.isShowing()) {
//                        mAlertDialog.dismiss();
//                    }
                    mainHandler.removeMessages(CLOSE_ALERTDIALOG);
                    mainHandler.sendEmptyMessageDelayed(CLOSE_PLUGGER_ACTIVITY, 500);
                    break;
                case NO_SERVICE:
//                    showDialog("NO SERVICE...");
                    mainHandler.removeMessages(NO_SERVICE);
                    mainHandler.sendEmptyMessageDelayed(CLOSE_PLUGGER_ACTIVITY, 500);
                    break;
                default:
                    break;
            }
        }
    };

//    private void showDialog(String text) {
//        builder = new AlertDialog.Builder(PluggerActivity.this, AlertDialog.THEME_HOLO_LIGHT);
//        builder.setTitle(text);
//        builder.setIcon(R.mipmap.ic_plugger);
//        builder.setInverseBackgroundForced(true);
//        mAlertDialog = builder.create();
//        mAlertDialog.show();
//        mAlertDialog.setCanceledOnTouchOutside(false);
//        mainHandler.sendEmptyMessageDelayed(CLOSE_ALERTDIALOG, 2000);
//        setDialogFontSize(mAlertDialog, FontSize);
//    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.activity_plugger);

        Intent intent = getIntent();
        Bundle extras = intent.getExtras();
        if (extras == null) {
            Log.v(TAG, "from gallery extra is null");
            finish();
            return;
        }
        if(null == intent.getType()|| ("*/*".equals(intent.getType()))){
            Log.v(TAG, "from gallery intent type is " + intent.getType());
            finish();
            return;
        }
        mContext = this;
        Intent pluggerIntent = new Intent();
        pluggerIntent.putExtras(extras);
        pluggerIntent.setType(intent.getType());
        pluggerIntent.setAction(PluggerReceiver.PLUGGER_SEND_MMS_NOSAVE);
        setIntent(pluggerIntent);

        checkReadPhoneStatePermission();
    }

//    @Override
//    public boolean onCreateOptionsMenu(Menu menu) {
//        // Inflate the menu; this adds items to the action bar if it is present.
//        getMenuInflater().inflate(R.menu.menu_plugger, menu);
//        return true;
//    }

//    @Override
//    public boolean onOptionsItemSelected(MenuItem item) {
//        // Handle action bar item clicks here. The action bar will
//        // automatically handle clicks on the Home/Up button, so long
//        // as you specify a parent activity in AndroidManifest.xml.
//        int id = item.getItemId();
//
//        //noinspection SimplifiableIfStatement
//        if (id == R.id.action_settings) {
//            return true;
//        }
//
//        return super.onOptionsItemSelected(item);
//    }
//    @Override
//    protected void onResume(){
//        super.onResume();
//    }
private int getActiveSubId_old() {
    int subId = -1;
    SubscriptionManager subscriptionManager = SubscriptionManager.from(PluggerActivity.this);
    List<SubscriptionInfo> subscriptionInfo = subscriptionManager.getActiveSubscriptionInfoList();
    if ( null!=subscriptionInfo ){
        subId = subscriptionInfo.get(0).getSubscriptionId();
    }
    return subId;
}
private int getActiveSubId() {
    int subId = -1;
   // boolean supportsM =
    //        android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP;//21.Android5.0.1
    SubscriptionManager subscriptionManager = SubscriptionManager.from(PluggerActivity.this);
    List<SubscriptionInfo> subscriptionInfo = subscriptionManager.getActiveSubscriptionInfoList();
    if ( null!=subscriptionInfo ){
        subId = subscriptionInfo.get(0).getSubscriptionId();
    }
    return subId;
}
    public void checkReadPhoneStatePermission(){
        int hasReadPhoneStatePermission = ContextCompat.checkSelfPermission(PluggerActivity.this, Manifest.permission.READ_PHONE_STATE);
        if (hasReadPhoneStatePermission != PackageManager.PERMISSION_GRANTED) {
            if (!ActivityCompat.shouldShowRequestPermissionRationale(PluggerActivity.this, Manifest.permission.READ_PHONE_STATE)) {
                showMessageOKCancel("You need to allow access to Setting",
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                ActivityCompat.requestPermissions(PluggerActivity.this, new String[]{Manifest.permission.READ_PHONE_STATE},PERMISSIONS_REQUEST_READ_PHONE_STATE);
                                dialog.dismiss();
                            }
                        });
                return ;
            }
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_PHONE_STATE},PERMISSIONS_REQUEST_READ_PHONE_STATE);
            return;
        }
        //permisson is PERMISSION_GRANTED, doing somthing
        if (getActiveSubId() == -1) {
            Log.v(TAG, "has no active sim card");
            finish();
            return;
        }
        checkReadExternalStoragePermission();
    }
    public void checkReadExternalStoragePermission(){
        int hasSendSmsPermission = ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE);
        if (hasSendSmsPermission != PackageManager.PERMISSION_GRANTED) {
            if (!ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.READ_EXTERNAL_STORAGE)) {
                showMessageOKCancel("You need to allow access to Setting",
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                ActivityCompat.requestPermissions(PluggerActivity.this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},  PERMISSIONS_READ_EXTERNAL_STORAGE);
                                dialog.dismiss();
                            }
                        });
                return ;
            }
            ActivityCompat.requestPermissions(PluggerActivity.this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, PERMISSIONS_READ_EXTERNAL_STORAGE);
            return;
        }
        checkSendSmsPermission();
    }
    public void checkSendSmsPermission(){
        int hasSendSmsPermission = ContextCompat.checkSelfPermission(this, Manifest.permission.SEND_SMS);
        if (hasSendSmsPermission != PackageManager.PERMISSION_GRANTED) {
            if (!ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.SEND_SMS)) {
                showMessageOKCancel("You need to allow access to Setting",
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                ActivityCompat.requestPermissions(PluggerActivity.this, new String[]{Manifest.permission.SEND_SMS}, PERMISSIONS_REQUEST_SEND_SMS);
                                dialog.dismiss();
                            }
                        });
                return ;
            }
            ActivityCompat.requestPermissions(PluggerActivity.this, new String[]{Manifest.permission.SEND_SMS}, PERMISSIONS_REQUEST_SEND_SMS);
            return;
        }
        sendBroadcast(getIntent());
        finish();
    }
    private void showMessageOKCancel(String message, DialogInterface.OnClickListener okListener) {
        builder = new AlertDialog.Builder(this);
        builder.setMessage(message)
                .setPositiveButton("OK", okListener)
                .setNegativeButton("Cancel", null);
        mAlertDialog = builder.create();
        mAlertDialog.show();
    }
    //   @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case PERMISSIONS_REQUEST_READ_PHONE_STATE: {
                // If request is cancelled, the result arrays are empty.
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    if (getActiveSubId() == -1) {
                        Log.v(TAG, "has no active sim card");
                        finish();
                        return;
                    }
                    Log.i(TAG, "user granted the READ_PHONE_STATE permission!");
                    checkReadExternalStoragePermission();
                } else {
                    Log.i(TAG, "user denied the READ_PHONE_STATE permission!");
                    finish();
                }
                return;
            }
            case PERMISSIONS_REQUEST_SEND_SMS: {
                // If request is cancelled, the result arrays are empty.
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Log.i(TAG, "user granted the SEND_SMS permission!");
                    sendBroadcast(getIntent());
                    finish();
                } else {
                    Log.i(TAG, "user denied the SEND_SMS permission!");
                    finish();
                }
                return;
            }
            case PERMISSIONS_READ_EXTERNAL_STORAGE: {
                // If request is cancelled, the result arrays are empty.
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Log.i(TAG, "user granted the READ_EXTERNA_STORAGE permission!");
                    checkSendSmsPermission();
                } else {
                    Log.i(TAG, "user denied the READ_EXTERNA_STORAGE permission!");
                    finish();
                }
                return;
            }
        }
    }

    private void setDialogFontSize(Dialog dialog, int size) {
        Window window = dialog.getWindow();
        View view = window.getDecorView();
        setViewFontSize(view, size);
    }

    private void setViewFontSize(View view, int size) {
        if (view instanceof ViewGroup) {
            ViewGroup parent = (ViewGroup) view;
            int count = parent.getChildCount();
            for (int i = 0; i < count; i++) {
                setViewFontSize(parent.getChildAt(i), size);
            }
        } else if (view instanceof TextView) {
            TextView textview = (TextView) view;
            textview.setTextSize(size);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mAlertDialog != null && mAlertDialog.isShowing()) {
            Log.v(TAG, "onPause send dialog dismiss");
            mAlertDialog.dismiss();
        }
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();
        Log.v(TAG, "onBackPressed ");
        if (isFinishing() || isDestroyed()) {
            ;
        } else if (mAlertDialog != null && mAlertDialog.isShowing()) {
            Log.v(TAG, "onBackPressed send dialog dismiss.");
            mAlertDialog.dismiss();
        }
    }
}
