package com.sprd.engineermode.activity.slog;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import com.sprd.engineermode.R;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import android.os.Handler;
import com.sprd.engineermode.core.SlogCore;
import android.graphics.Color;
import android.app.ProgressDialog;


/**
 * Created by SPREADTRUM\zhengxu.zhang on 9/6/15.
 */
public class SceneActivity extends Activity implements View.OnClickListener{

    private ArrayList<Button> btnScene = new ArrayList<Button>();
    private TextView title;
    private static final String TAG = "SLOG_SCENE";
    private static final int SCENE_HANDLER_0 = 0;
    private static final int SCENE_HANDLER_1 = 1;
    private ProgressDialog mProgressDialog;
    private Handler mUiThread = new Handler();




    @Override
    public void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);
        setContentView(R.layout.layout_slog_scene);
        SlogInfo.x = SceneActivity.this;
        btnScene.add((Button) findViewById(R.id.btn_slog_normal_scene));
        btnScene.add((Button)findViewById(R.id.btn_slog_data_scene));
        btnScene.add((Button)findViewById(R.id.btn_slog_voice_scene));
        btnScene.add((Button)findViewById(R.id.btn_slog_modem_scene));
        btnScene.add((Button)findViewById(R.id.btn_slog_wcn_scene));
        btnScene.add((Button)findViewById(R.id.btn_slog_sim_scene));
        btnScene.add((Button)findViewById(R.id.btn_slog_custom_scene));

        if (SlogCore.isYlogOpen()) {
            for(Button btn:btnScene) {
                btn.setEnabled(true);
                btn.setOnClickListener(this);
                btn.setBackgroundColor(Color.GRAY);
            }
        } else {
            for(Button btn:btnScene) {
                btn.setEnabled(false);
                btn.setBackgroundColor(Color.GRAY);
            }
        }

    }

    @Override
    public void onResume(){
        super.onResume();
        if(SlogInfo.self().slog_tmp!= SlogInfo.SceneStatus.close)
            return;
        switch (SlogInfo.self().getSceneStatus()){
            case normal:
                btnScene.get(0).setBackgroundColor(Color.GREEN);
                break;
            case data:
                btnScene.get(1).setBackgroundColor(Color.GREEN);
                break;
            case voice:
                btnScene.get(2).setBackgroundColor(Color.GREEN);
                break;
            case modem:
                btnScene.get(3).setBackgroundColor(Color.GREEN);
                break;
            case wcn:
                btnScene.get(4).setBackgroundColor(Color.GREEN);
                break;
            case sim:
                btnScene.get(5).setBackgroundColor(Color.GREEN);
                break;
            case customer:
                for(Button btn:btnScene){
                    btn.setBackgroundColor(Color.GRAY);
                }
            	btnScene.get(6).setBackgroundColor(Color.GREEN);
            	break;
        }

    }

    /* SPRD: Bug 564177 set the log scene during capture the armlog, EngineerMode is no responding @{ */
    private void showProgressDialog() {
        Log.d(TAG,"showProgressDialog");
        mUiThread.post(new Runnable() {
            @Override
            public void run() {
                mProgressDialog = ProgressDialog.show(SceneActivity.this,
                        getResources().getString(R.string.scene_switching),
                        getResources().getString(R.string.scene_switching_wait), true,
                        false);
            }
        });
    }

    private void dismissProgressDialog() {
        Log.d(TAG, "dismissProgressDialog");
        mUiThread.post(new Runnable() {
            @Override
            public void run() {
                if (mProgressDialog != null) {
                    mProgressDialog.dismiss();
                }
            }
        });
    }
    public Handler dismissProgressBarHandler = new Handler() {

        public void handleMessage(Message msg) {
            switch (msg.what) {
                case SCENE_HANDLER_0:
                    final String valueStr = (String) msg.obj;
                    dismissProgressDialog();
                    for(Button btn:btnScene){
                        btn.setBackgroundColor(Color.GRAY);
                        Log.d(TAG,"setBackgroundColor(Color.GRAY)");
                    }
                    btnScene.get(Integer.parseInt(valueStr)).setBackgroundColor(Color.GREEN);
                    Log.d(TAG,"setBackgroundColor(Color.GREEN)");
                    break;
                case SCENE_HANDLER_1:
                    dismissProgressDialog();
                    for(Button btn:btnScene){
                        btn.setBackgroundColor(Color.GRAY);
                    }
                    break;
            }
        }
    };
    /* @} */

    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.btn_slog_normal_scene:
                Log.d(TAG,"open normalLog 00");
                showProgressDialog();
                new Thread(new Runnable() {
                    public void run() {
                        if(SlogInfo.self().getSceneStatus()== SlogInfo.SceneStatus.normal){
                            SlogInfo.self().closeScene();
                            SlogInfo.self().setSceneStatus(SlogInfo.SceneStatus.close);
                            Message DismissProgressBar = dismissProgressBarHandler.obtainMessage(SCENE_HANDLER_1);
                            dismissProgressBarHandler.sendMessage(DismissProgressBar);
                            return;
                        }
                        SlogInfo.self().openNormalScene();
                        Message DismissProgressBar = dismissProgressBarHandler.obtainMessage(SCENE_HANDLER_0,"0");
                        dismissProgressBarHandler.sendMessage(DismissProgressBar);
                    }
                }).start();

                break;
            case R.id.btn_slog_data_scene:
                Log.d(TAG,"open dataLog 00");
                showProgressDialog();
                new Thread(new Runnable() {
                    public void run() {
                        if(SlogInfo.self().getSceneStatus()== SlogInfo.SceneStatus.data){
                            SlogInfo.self().closeScene();
                            SlogInfo.self().setSceneStatus(SlogInfo.SceneStatus.close);
                            Message DismissProgressBar = dismissProgressBarHandler.obtainMessage(SCENE_HANDLER_1);
                            dismissProgressBarHandler.sendMessage(DismissProgressBar);
                            return;
                        }
                        SlogInfo.self().openDataScene();
                        Message DismissProgressBar = dismissProgressBarHandler.obtainMessage(SCENE_HANDLER_0,"1");
                        dismissProgressBarHandler.sendMessage(DismissProgressBar);
                    }
                }).start();
                break;
            case R.id.btn_slog_voice_scene:
                Log.d(TAG,"open voiceLog 00");
                showProgressDialog();
                new Thread(new Runnable() {
                    public void run() {
                        if(SlogInfo.self().getSceneStatus()== SlogInfo.SceneStatus.voice){
                            SlogInfo.self().closeScene();
                            SlogInfo.self().setSceneStatus(SlogInfo.SceneStatus.close);
                            Message DismissProgressBar = dismissProgressBarHandler.obtainMessage(SCENE_HANDLER_1);
                            dismissProgressBarHandler.sendMessage(DismissProgressBar);
                            return;
                        }
                        SlogInfo.self().openVoiceScene();
                        Message DismissProgressBar = dismissProgressBarHandler.obtainMessage(SCENE_HANDLER_0,"2");
                        dismissProgressBarHandler.sendMessage(DismissProgressBar);
                    }
                }).start();
                break;
            case R.id.btn_slog_modem_scene:
                Log.d(TAG,"open modemLog 00");
                showProgressDialog();
                new Thread(new Runnable() {
                    public void run() {
                        if(SlogInfo.self().getSceneStatus()== SlogInfo.SceneStatus.modem){
                            SlogInfo.self().closeScene();
                            SlogInfo.self().setSceneStatus(SlogInfo.SceneStatus.close);
                            Message DismissProgressBar = dismissProgressBarHandler.obtainMessage(SCENE_HANDLER_1);
                            dismissProgressBarHandler.sendMessage(DismissProgressBar);
                            return;
                        }
                        SlogInfo.self().openModemScene();
                        Message DismissProgressBar = dismissProgressBarHandler.obtainMessage(SCENE_HANDLER_0,"3");
                        dismissProgressBarHandler.sendMessage(DismissProgressBar);
                    }
                }).start();
                break;
            case R.id.btn_slog_wcn_scene:
                Log.d(TAG,"open wcnLog 00");
                showProgressDialog();
                new Thread(new Runnable() {
                    public void run() {
                        if(SlogInfo.self().getSceneStatus()== SlogInfo.SceneStatus.wcn){
                            SlogInfo.self().closeScene();
                            SlogInfo.self().setSceneStatus(SlogInfo.SceneStatus.close);
                            Message DismissProgressBar = dismissProgressBarHandler.obtainMessage(SCENE_HANDLER_1);
                            dismissProgressBarHandler.sendMessage(DismissProgressBar);
                            return;
                        }
                        SlogInfo.self().openWcnScene();
                        Message DismissProgressBar = dismissProgressBarHandler.obtainMessage(SCENE_HANDLER_0,"4");
                        dismissProgressBarHandler.sendMessage(DismissProgressBar);
                    }
                }).start();
                break;
            case R.id.btn_slog_sim_scene:
                Log.d(TAG,"open simLog 00");
                showProgressDialog();
                new Thread(new Runnable() {
                    public void run() {
                        if(SlogInfo.self().getSceneStatus()== SlogInfo.SceneStatus.sim){
                            SlogInfo.self().closeScene();
                            SlogInfo.self().setSceneStatus(SlogInfo.SceneStatus.close);
                            Message DismissProgressBar = dismissProgressBarHandler.obtainMessage(SCENE_HANDLER_1);
                            dismissProgressBarHandler.sendMessage(DismissProgressBar);
                            return;
                        }
                        SlogInfo.self().openSimScene();
                        Message DismissProgressBar = dismissProgressBarHandler.obtainMessage(SCENE_HANDLER_0,"5");
                        dismissProgressBarHandler.sendMessage(DismissProgressBar);
                    }
                }).start();
                break;

            case R.id.btn_slog_custom_scene:
                Log.d(TAG,"slog_custom_scene");
                //Intent iCustomer = new Intent(this,SlogUISettings.class);
                //startActivity(iCustomer);
                Intent iOpenUserDefined = new Intent(this,UserDefinedActivity.class);
                startActivity(iOpenUserDefined);
                break;
        }
    }

    public Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg){
            switch (msg.what){

                case 0:

                    break;
                case 1:

                    break;

            }
        }

    };





}
