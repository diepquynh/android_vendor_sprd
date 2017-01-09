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
import android.widget.Spinner;
import android.widget.ArrayAdapter;

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
import android.os.Environment;

public class APCTRom extends Activity {

    static final String TAG = "APCTRom";

    TextView mRwSizeTxt;
    TextView mRwDirTxt;
    TextView mRwResultTxt;
    EditText   mRwSizeEt;
    Spinner   mDirSpinner;
    Button      mStartBtn;
    String      mCmd;
    String      mOpRest;
    private static final String ROM_BIN_FILE = "bonnie";
    private static final String[] mDir={"/data/data/com.sprd.APCT", "/mnt/sdcard"};
    private ArrayAdapter<String> mSpinnerAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.rom);

        try {
        InputStream in = getAssets().open(ROM_BIN_FILE);

        OutputStream out = new FileOutputStream(new File("/data/data/com.sprd.APCT",ROM_BIN_FILE));

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
        Runtime.getRuntime().exec("/system/bin/chmod 744 /data/data/com.sprd.APCT/bonnie");
        }catch(Throwable t)  {
        }

        mRwSizeTxt   = (TextView)findViewById(R.id.txt_rw_size);
        mRwDirTxt  = (TextView)findViewById(R.id.txt_rw_dir);
        mRwResultTxt = (TextView)findViewById(R.id.txt_rw_result);

        mRwSizeEt = (EditText)findViewById(R.id.et_rw_size);
        mDirSpinner = (Spinner)findViewById(R.id.spinner_rw_dir);

        mSpinnerAdapter = new ArrayAdapter<String>(this,android.R.layout.simple_spinner_item,mDir);

        mSpinnerAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

        mDirSpinner.setAdapter(mSpinnerAdapter);

        mDirSpinner.setVisibility(View.VISIBLE);

        mStartBtn = (Button)findViewById(R.id.StartBn);

        mStartBtn.setOnClickListener(new Button.OnClickListener(){
             public void onClick(View v)
             {
                 boolean isEmpty = false;
                 Log.d("bonnie", "dir = " + mDirSpinner.getSelectedItem() + " size=" + mRwSizeEt.getText());

                 InputMethodManager imm = (InputMethodManager) getBaseContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                 imm.hideSoftInputFromWindow(v.getWindowToken(), 0);

                 if (    "".equals(mRwSizeEt.getText().toString())
                     || "".equals(mDirSpinner.getSelectedItem().toString()))
                 {
                     Toast.makeText(APCTRom.this, "dir/file size cann't be empty!", Toast.LENGTH_LONG).show();
                 }
                 else
                 {
                     mRwResultTxt.setText("Calculating, Please wait...");
                     mCmd = "/data/data/com.sprd.APCT/bonnie -d " +  mDirSpinner.getSelectedItem().toString() + " -s " +  
                                                                                                       mRwSizeEt.getText().toString();

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
                                      result +=  line + "\n";
                                      Log.d("bonnie",result);
                                   }
                                   in.close();
                 		    mOpRest = mDirSpinner.getSelectedItem() + " RW Rate is:\n" + result;
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

