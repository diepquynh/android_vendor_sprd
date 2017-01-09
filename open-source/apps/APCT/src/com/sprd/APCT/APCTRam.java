package com.sprd.APCT;

import com.sprd.APCT.R;
import java.util.ArrayList;
import java.util.List;
import android.content.Context;
import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.view.View.OnClickListener;
import android.view.inputmethod.InputMethodManager;

import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import java.util.HashMap;        
import java.util.Map;

import android.provider.Settings;
import android.content.ContentResolver;
import android.util.Log;
import android.content.Intent;
import java.io.*;
import android.os.SystemClock;
import android.view.MotionEvent;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;

public class APCTRam extends Activity {
    static final String TAG = "APCTRam";
    TextView mPromptTxt;
    TextView mRwSizeTxt;
    TextView mRwCountTxt;
    TextView mRwProcessTxt;
    TextView mRwResultTxt;
    EditText mRwSizeEt;
    EditText mRwCountEt;
    EditText mRwProcessEt;
    Button   mStartBtn;
    String   mCmd;
    String   mOpRest;
    private static final String RAM_BIN_FILE = "copybw";

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.ram);

        try {
        InputStream in = getAssets().open(RAM_BIN_FILE);

        OutputStream out = new FileOutputStream(new File("/data/data/com.sprd.APCT",RAM_BIN_FILE));

        byte[] buf = new byte[2048];
        int len;

        while ((len = in.read(buf)) >= 0 ) {
             out.write(buf, 0, len);
        }
        out.close();
        in.close();
        }catch (IOException e) {
        }

        try {
        Runtime.getRuntime().exec("/system/bin/chmod 744 /data/data/com.sprd.APCT/copybw");
        }catch(Throwable t)  {
        }

        //mPromptTxt       = (TextView)findViewById(R.id.txt_prompt);
        mRwSizeTxt   = (TextView)findViewById(R.id.txt_rw_size);
        mRwCountTxt  = (TextView)findViewById(R.id.txt_rw_count);
        mRwProcessTxt = (TextView)findViewById(R.id.txt_rw_process);
        mRwResultTxt = (TextView)findViewById(R.id.txt_rw_result);

        mRwSizeEt = (EditText)findViewById(R.id.et_rw_size);
        mRwCountEt = (EditText)findViewById(R.id.et_rw_count);
        mRwProcessEt = (EditText)findViewById(R.id.et_rw_process);

        mStartBtn = (Button)findViewById(R.id.StartBn);

        mStartBtn.setOnClickListener(new Button.OnClickListener(){
             public void onClick(View v)
             {
                 boolean isEmpty = false;

                 Log.d("copybw", "size = " + mRwSizeEt.getText() + " count=" + mRwCountEt.getText() + " process=" +  mRwProcessEt.getText());

                 InputMethodManager imm = (InputMethodManager) getBaseContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                 imm.hideSoftInputFromWindow(v.getWindowToken(), 0);

                 if (    "".equals(mRwSizeEt.getText().toString())
                     || "".equals(mRwCountEt.getText().toString())
                     || "".equals(mRwProcessEt.getText().toString()))
                 {
                     Toast.makeText(APCTRam.this, "size/count/process cann't be empty!", Toast.LENGTH_LONG).show();
                 }
                 else
                 {
                     mRwResultTxt.setText("Calculating RAM performance, Please wait...");
                     mCmd = "/data/data/com.sprd.APCT/copybw -c " +  mRwSizeEt.getText() + " -n " +  
                                                                                                       mRwCountEt.getText()  + " -p " + mRwProcessEt.getText();

                     Thread thread = new Thread()
                     {
                         public void run()
                         {
                              try{
                                   Process p = Runtime.getRuntime().exec(mCmd);
                                   BufferedReader in = new BufferedReader(new InputStreamReader(p.getInputStream()));

                                   String line = "";
                                   String result = "";

                                   while ((line = in.readLine()) != null) {
                                      result += line + "\n";
                                      Log.d("copybw",result);
                                   }
                                   in.close();
                 		    mOpRest = "RAM Throughput Rate is:\n" + result;
                                Message msg = handler.obtainMessage(0, mOpRest);
                 	           handler.sendMessage(msg);
                              } 
                              catch(Throwable t)
                              {
                                    t.printStackTrace();
                              }
                 	 }
                     };
                     thread.start();
                 }
             }
         });
    }

    public Handler handler = new Handler()
    {
        @SuppressWarnings("unchecked")
        @Override
        public void handleMessage(Message msg)
        {
            String str;

            switch (msg.what)
            {
                case 0:
                str = (String)msg.obj;
                mRwResultTxt.setText(str);
                break;

                default:
                break;
            }
        }
     };
}
