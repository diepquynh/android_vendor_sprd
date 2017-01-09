
package com.sprd.validationtools;

import android.graphics.Color;
import com.sprd.validationtools.sqlite.EngSqlite;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Gravity;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

public class BaseActivity extends Activity implements OnClickListener{
    private static final String TAG = "BaseActivity";
    private AlertDialog mResultDialog;
    private String mTestname = null;
    private EngSqlite mEngSqlite;
    public static boolean shouldCanceled = true;

    protected Button mPassButton;
    protected Button mFailButton;
    private static final int TEXT_SIZE = 30;
    protected boolean canPass = true;
    protected WindowManager mWindowManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mTestname = this.getIntent().getStringExtra("testname");

        mEngSqlite = EngSqlite.getInstance(this);

        mWindowManager = getWindowManager();
        createButton(true);
        createButton(false);
    }

    public void createButton(boolean isPassButton) {
        int buttonSize = getResources().getDimensionPixelSize(R.dimen.pass_fail_button_size);
        if(isPassButton) {
            mPassButton = new Button(this);
            mPassButton.setText(R.string.text_pass);
            mPassButton.setTextColor(Color.WHITE);
            mPassButton.setTextSize(TEXT_SIZE);
            mPassButton.setBackgroundColor(Color.GREEN);
            mPassButton.setOnClickListener(this);
        }else {
            mFailButton = new Button(this);
            mFailButton.setText(R.string.text_fail);
            mFailButton.setTextColor(Color.WHITE);
            mFailButton.setTextSize(TEXT_SIZE);
            mFailButton.setBackgroundColor(Color.RED);
            mFailButton.setOnClickListener(this);
        }


        WindowManager.LayoutParams lp = new WindowManager.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                WindowManager.LayoutParams.TYPE_APPLICATION,
//                WindowManager.LayoutParams.TYPE_PHONE,
                WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                    | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
                    PixelFormat.TRANSLUCENT);
        lp.gravity = isPassButton?Gravity.LEFT | Gravity.BOTTOM:Gravity.RIGHT | Gravity.BOTTOM;
        lp.flags =LayoutParams.FLAG_NOT_TOUCH_MODAL
                | LayoutParams.FLAG_NOT_FOCUSABLE;
        lp.width=buttonSize;
        lp.height=buttonSize;
        mWindowManager.addView(isPassButton?mPassButton:mFailButton, lp);
    }


    public void storeRusult(boolean isSuccess) {
        Log.d("BaseActivity", "storeResult" + mTestname);
        mEngSqlite.updateDB(mTestname, isSuccess ? Const.SUCCESS : Const.FAIL);
    }

    @Override
    public void finish() {
        removeButton();
        this.setResult(Const.TEST_ITEM_DONE, getIntent());
        super.finish();
    }

    protected void showResultDialog(String content) {
        LinearLayout resultDlgLayout = (LinearLayout) getLayoutInflater().inflate(
                R.layout.result_dlg, null);
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        content = "\n" + content + "\n";
        content += getString(R.string.alert_dialog_test);
        TextView title = (TextView) resultDlgLayout.findViewById(R.id.title);
        title.setText(getResources().getString(R.string.alert_dialog_title));
        TextView message = (TextView) resultDlgLayout.findViewById(R.id.message);
        message.setText(content);
        builder.setView(resultDlgLayout);
        Button passBtn = (Button) resultDlgLayout.findViewById(R.id.positiveButton);
        passBtn.setText(R.string.text_pass); 
        passBtn.setOnClickListener(new View.OnClickListener() {  
            public void onClick(View v) {  
                storeRusult(true);
                finish();
            }  
        }); 
        Button failBtn = (Button) resultDlgLayout.findViewById(R.id.negativeButton);
        failBtn.setText(R.string.text_fail); 
        failBtn.setOnClickListener(new View.OnClickListener() {  
            public void onClick(View v) {  
                storeRusult(false);
                finish();
            }  
        }); 
//        builder.setNegativeButton(getResources().getString(R.string.text_pass),
//                new DialogInterface.OnClickListener() {
//                    @Override
//                    public void onClick(DialogInterface dialog, int which) {
//                        storeRusult(true);
//                        finish();
//                    }
//                });
//        builder.setPositiveButton(getResources().getString(R.string.text_fail),
//                new DialogInterface.OnClickListener() {
//                    @Override
//                    public void onClick(DialogInterface dialog, int which) {
//                        storeRusult(false);
//                        finish();
//                    }
//                });

        if (mResultDialog != null) {
            mResultDialog.cancel();
        }

        mResultDialog = builder.create();
        mResultDialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
            @Override
            public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                if (keyCode == KeyEvent.KEYCODE_BACK
                        && event.getAction() == KeyEvent.ACTION_UP && shouldCanceled) {
                    Intent intent = BaseActivity.this.getIntent();
                    mResultDialog.cancel();

                    BaseActivity.super.finish();
                    BaseActivity.this.startActivityForResult(intent,0);
                    return true;
                }
                shouldCanceled = true;
                return false;
            }
        });
        mResultDialog.setCanceledOnTouchOutside(false);
        if (mResultDialog != null && !mResultDialog.isShowing()) {
            mResultDialog.show();
        }
    }

    protected void removeButton() {
        try{
            mWindowManager.removeView(mPassButton);
            mWindowManager.removeView(mFailButton);
        }catch(Exception e){
            //TODO
        }
    }

    @Override
    public void onBackPressed() {
       // showResultDialog(getString(R.string.alert_finish_test));
        Intent intent = BaseActivity.this.getIntent();
        BaseActivity.this.startActivityForResult(intent,0);
        finish();
    }

    @Override
    public void onClick(View v) {
        // TODO Auto-generated method stub
        if (v == mPassButton) {
            if (canPass) {
                Log.d("onclick", "pass.." + this);
                storeRusult(true);
                finish();
                } else {
                Toast.makeText(this, R.string.can_not_pass, Toast.LENGTH_SHORT).show();
            }
        } else if (v == mFailButton) {
            storeRusult(false);
            finish();
        }
    }
}
