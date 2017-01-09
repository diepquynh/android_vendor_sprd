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

import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

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


public class APCTProcrank extends Activity {

    static final String TAG = "APCTProcrank";

    TextView mProcResultTxt;
    String   mCmd = "procrank";
    String   mOpRest;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.procrank);

        mProcResultTxt = (TextView)findViewById(R.id.txt_procrank_result);
        mProcResultTxt.setText("Geting procrank result, please wait...");
        thread.start();
    }

    @Override
    public void onPause() {
        super.onPause();
        this.finish();
    }

     Thread thread = new Thread()
     {
         public void run()
         {
             while (true)
             {
              try{
                   Process p = Runtime.getRuntime().exec(mCmd);
                   BufferedReader in = new BufferedReader(new InputStreamReader(p.getInputStream()));

                   String line = "";
                   String result = "";
                   int line_count = 0;

                   while ((line = in.readLine()) != null) {
                      result += line + "\n";
                      line_count++;

                      if (line_count % 18 == 0)
                      {
                          break;
                      }
                  }
                  in.close();
                  Message msg = handler.obtainMessage(0, result);
 	              handler.sendMessage(msg);
                  Log.d("bench_procrank",result);
                  result = "";
              }
              catch(Throwable t)
              {
                    t.printStackTrace();
              }
            }
         }
     };

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
                mProcResultTxt.setText(str);
                break;

                default:
                break;
            }
        }
     };
}
