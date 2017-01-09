package com.sprd.engineermode.telephony.userinfo;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.OutputStreamWriter;
import java.io.IOException;

import android.app.IntentService;
import java.text.SimpleDateFormat;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Environment;
import android.os.SystemProperties;

import java.util.Calendar;
import java.util.Date;
import android.util.Log;

import android.telephony.CellLocation;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.telephony.gsm.GsmCellLocation;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import com.sprd.engineermode.debuglog.slogui.StorageUtil;

public class BehaviorRecordService extends IntentService{
    private static final String TAG = "BehaviorRecordService";
    private Context mContext;
    String saveFilePath="";
    FileOutputStream out=null;
    OutputStreamWriter osw=null;
    BufferedWriter bw=null;
    File file=null;
    private static final int COUNT=1440;
    private static byte[] gpsRecord=new byte[COUNT];
    private static byte[] wifiRecord=new byte[COUNT];
    private static byte[] btRecord=new byte[COUNT];
    private static byte[] dataNetRecord=new byte[COUNT];
    private static byte[] screenRecord=new byte[COUNT];

    private BehaviorInfoRecord mBehaviorInfoRecode;
    private PhoneStateListener mListener;
    TelephonyManager mTelephonyManager;
    private SharedPreferences preferences;
    private SharedPreferences.Editor editor;
    private String mGpsData="",mWifiData="",mBtData="",mNetData="",mScreenData="",mServerFreqData="";
    private String onTime="",offTime="";
    public static int time=0;
    int prevCallState = 0;
    int callNum = 0;

    public BehaviorRecordService(){
        super("BehaviorRecordService");
    }

    @Override
    public void onCreate(){
        super.onCreate();
        Log.d(TAG,"onCreate");
        mContext=this;
        mBehaviorInfoRecode=new BehaviorInfoRecord(this);
        preferences = mContext
                .getSharedPreferences("behavior_record", mContext.MODE_PRIVATE);
        editor = preferences.edit();
        time=Integer.valueOf(preferences.getString("query_time", "2"));

        if(BehaviorInfoRecordActivity.mIsOpenAll){
            openAllItems();
        }

        initFilePath();

        listenPhoneState();
    }

    private void openAllItems(){
        editor.putString("screen_record", "1");
        editor.putString("wifi_record", "1");
        editor.putString("data_record", "1");
        editor.putString("bt_record", "1");
        editor.putString("gps_record", "1");
        editor.putString("call_record", "1");
        editor.putString("server_freq_record", "1");
        editor.commit();
        SystemProperties.set("persist.sys.open.user.record", "true");
    }

    private void initFilePath(){
        saveFilePath="/data/userinfo";
        Log.d(TAG,"file path is:"+saveFilePath);
        try{
            File destDir = new File(saveFilePath);
            if(!destDir.exists()){
                destDir.mkdirs();
                String command="chmod 777 " + saveFilePath;
                Log.d(TAG, command);
                Process p = Runtime.getRuntime().exec(command);
                Log.d(TAG, saveFilePath+" create success");
            }
        }catch(Exception e){
            Log.d(TAG, saveFilePath+" create fail");
            Log.d(TAG,e.getMessage());
        }
        try{
            File Dir = new File(saveFilePath);
            if(!Dir.exists()){
                Log.d(TAG, saveFilePath+" create ssuccess");
            }
        }catch(Exception e){
            Log.d(TAG, saveFilePath+" create fail");
            Log.d(TAG,e.getMessage());
        }
    }

    private void listenPhoneState(){
        mTelephonyManager = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
        mListener = new PhoneStateListener() {

            @Override
            public void onCallStateChanged(int state, String incomingNumber) {
                // TODO Auto-generated method stub
                switch (state) {
                //
                case TelephonyManager.CALL_STATE_OFFHOOK:
//                    if("".equals(onTime)){
//                        onTime=getDate();
//                    }
                    prevCallState = TelephonyManager.CALL_STATE_OFFHOOK;
                    break;
                //
                case TelephonyManager.CALL_STATE_RINGING:
//                    if("".equals(onTime)){
//                        onTime=getDate();
//                    }
                    prevCallState = TelephonyManager.CALL_STATE_RINGING;
                    break;
                //
                case TelephonyManager.CALL_STATE_IDLE:
                    if (prevCallState == TelephonyManager.CALL_STATE_RINGING
                            || prevCallState == TelephonyManager.CALL_STATE_OFFHOOK) {
                        //offTime=getDate();
                        callNum++;
                        Log.i(TAG,"CallNum of:"+"  changed :"+callNum);
                    }
                    prevCallState = TelephonyManager.CALL_STATE_IDLE;
                    break;
                }
                super.onCallStateChanged(state, incomingNumber);
            }
        };
        mTelephonyManager.listen(mListener,
                PhoneStateListener.LISTEN_CALL_STATE);
    }

    @Override
    public void onDestroy() {
        Log.d(TAG,"onDestroy");
        super.onDestroy();
    }

    @Override
    protected void onHandleIntent(Intent intent){
        Log.d(TAG,"onHandleIntent");

        try{
            SimpleDateFormat formatter = new SimpleDateFormat ("yyyy-MM-dd");
            Date curDate = new Date(System.currentTimeMillis());//获取当前时间
            String fileName = formatter.format(curDate);
            file = new File(saveFilePath+"/"+fileName+"_behavior.csv");
            Log.d(TAG,"file name is "+saveFilePath+"/"+fileName);

            if(!file.exists()){
                for(int i=0;i<COUNT;i++){
                    gpsRecord[i]=48;
                    wifiRecord[i]=48;
                    btRecord[i]=48;
                    dataNetRecord[i]=48;
                    screenRecord[i]=48;
                }
                callNum=0;
                editor.putString("gps_data", "");
                editor.putString("wifi_data", "");
                editor.putString("bt_data", "");
                editor.putString("net_data", "");
                editor.putString("screen_data", "");
                editor.putString("call_num","0");
                editor.putString("server_freq_changes","0");
                editor.putString("server_freq_changes0","0");
                editor.putString("server_freq_changes1","0");
                //editor.putString("server_freq","0");
                //editor.putString("call_data","");
                editor.commit();
            }else{
                String gps=preferences.getString("gps_data", "");
                String wifi=preferences.getString("wifi_data", "");
                String bt=preferences.getString("bt_data", "");
                String dataNet=preferences.getString("net_data", "");
                String screen=preferences.getString("screen_data", "");
                String call_num=preferences.getString("call_num", "");
//                String server_freq_changes=preferences.getString("server_freq_changes", "0");
                //String call_data=preferences.getString("call_data", "");

                if("".equals(gps)){
                    for(int i=0;i<COUNT;i++){
                        gpsRecord[i]=48;
                    }
                }else{
                    gpsRecord=gps.getBytes();
                }

                if("".equals(wifi)){
                    for(int i=0;i<COUNT;i++){
                        wifiRecord[i]=48;
                    }
                }else{
                    wifiRecord=wifi.getBytes();
                }
                if("".equals(bt)){
                    for(int i=0;i<COUNT;i++){
                        btRecord[i]=48;
                    }
                }else{
                    btRecord=bt.getBytes();
                }
                if("".equals(dataNet)){
                    for(int i=0;i<COUNT;i++){
                        dataNetRecord[i]=48;
                    }
                }else{
                    dataNetRecord=dataNet.getBytes();
                }
                if("".equals(screen)){
                    for(int i=0;i<COUNT;i++){
                        screenRecord[i]=48;
                    }
                }else{
                    screenRecord=screen.getBytes();
                }
                if("".equals(call_num)){
                    callNum=0;
                }else{
                    callNum=Integer.valueOf(call_num);
                }
//                if("".equals(call_data)){
//                    mCallData="";
//                }else{
//                    mCallData=call_data;
//                }
            }
        }catch(Exception e){
            Log.d(TAG,e.getMessage());
        }
        mHandler.sendEmptyMessage(1);
    }

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            Log.d(TAG,"handleMessage");
            if (msg.what == 1) {
                if(false == SystemProperties.getBoolean("persist.sys.open.user.record",false)){
                    return;
                }
                SimpleDateFormat formatter = new SimpleDateFormat ("yyyy-MM-dd");
                Date curDate = new Date(System.currentTimeMillis());//获取当前时间
                String fileName = formatter.format(curDate);
                file = new File(saveFilePath+"/"+fileName+"_behavior.csv");
                if(!file.exists()){
                    for(int i=0;i<COUNT;i++){
                        gpsRecord[i]=48;
                        wifiRecord[i]=48;
                        btRecord[i]=48;
                        dataNetRecord[i]=48;
                        screenRecord[i]=48;
                    }
                    callNum=0;
                    editor.putString("gps_data", "");
                    editor.putString("wifi_data", "");
                    editor.putString("bt_data", "");
                    editor.putString("net_data", "");
                    editor.putString("screen_data", "");
                    editor.putString("call_num","0");
                    editor.putString("server_freq_changes","0");
                    editor.putString("server_freq_changes0","0");
                    editor.putString("server_freq_changes1","0");
                    //editor.putString("server_freq","0");
                    //editor.putString("call_data","");
                    editor.commit();
                }
                String time = getDate();
                try{
                    bw = new BufferedWriter(new FileWriter(file,false));
                    String[] str=time.split(" ");
                    int index=0;
                    if(str.length == 2){
                        String[] str1=str[1].split(":");
                        if(str1.length == 3){
                            index=60*Integer.valueOf(str1[0])+Integer.valueOf(str1[1]);
                        }
                    }
                    if(index >= 0){
                        bw.write("ITEM"+","+"TIME"+","+"DATA"+","+"OTHER");
                        bw.newLine();
                        if("1".equals(preferences.getString("gps_record","0"))){
                            if(mBehaviorInfoRecode.gpsIsOpend()){
                                gpsRecord[index-1]=49;
                            }
                            mGpsData=new String(gpsRecord);
                            editor.putString("gps_data", mGpsData);
                            bw.write("gps"+","+time+","+mGpsData);
                            bw.newLine();
                        }
                        if("1".equals(preferences.getString("wifi_record","0"))){
                            if(mBehaviorInfoRecode.wifiIsLinked()){
                                wifiRecord[index-1]=49;
                            }
                            mWifiData=new String(wifiRecord);
                            editor.putString("wifi_data", mWifiData);
                            bw.write("wifi"+","+time+","+mWifiData);
                            bw.newLine();
                        }
                        if("1".equals(preferences.getString("bt_record","0"))){
                            if(mBehaviorInfoRecode.isBluetoothOpend()){
                                btRecord[index-1]=49;
                            }
                            mBtData=new String(btRecord);
                            editor.putString("bt_data", mBtData);
                            bw.write("bt"+","+time+","+mBtData);
                            bw.newLine();
                        }
                        if("1".equals(preferences.getString("data_record","0"))){
                            if(mBehaviorInfoRecode.dataNetIsOpen()){
                                dataNetRecord[index-1]=49;
                            }
                            mNetData=new String(dataNetRecord);
                            editor.putString("net_data", mNetData);
                            bw.write("dataNet"+","+time+","+mNetData);
                            bw.newLine();
                        }
                        if("1".equals(preferences.getString("screen_record","0"))){
                            if(mBehaviorInfoRecode.isScreenOn()){
                                screenRecord[index-1]=49;
                            }
                            mScreenData=new String(screenRecord);
                            editor.putString("screen_data", mScreenData);
                            bw.write("screen"+","+time+","+mScreenData);
                            bw.newLine();
                        }
                    }
                    if("1".equals(preferences.getString("call_record","0"))){
//                        if(!"".equals(onTime)){
//                            mCallData+="\"Num:"+(callNum+1)+" startTime:"+onTime+"  endTime:";
//                            onTime="";
//                        }
//                        if(!"".equals(offTime)){
//                            mCallData+="Num:"+callNum+" startTime:"+onTime+"  endTime:"+offTime+"\r\n";
//                            Log.d(TAG,"call data:"+mCallData);
//                            onTime="";
//                            offTime="";
//                        }
                        editor.putString("call_num", String.valueOf(callNum));
                        //editor.putString("call_data", mCallData);
                        bw.write("call"+","+time+","+callNum);
                        bw.newLine();
                    }
                    if("1".equals(preferences.getString("server_freq_record","0"))){
                        bw.write("server freq change times"+","+time+","+"\""+"SIM0:"+preferences.getString("server_freq_changes0", "0")+"\r\n"+
                                "SIM1:"+preferences.getString("server_freq_changes1", "0")+"\""+","+
                                (Integer.valueOf(preferences.getString("server_freq_changes0", "0"))+
                                Integer.valueOf(preferences.getString("server_freq_changes1", "0"))));
                        bw.newLine();
                    }
                    editor.commit();
                }catch(Exception e){
                    Log.d(TAG,e.getMessage());
                }finally{
                    try{
                        if(bw!=null){
                            bw.close();
                        }
                    }catch(Exception e){
                        Log.d(TAG,e.getMessage());
                    }
                }
                if(BehaviorInfoRecordActivity.mIsOpenBehaviorRecord){
                    Message m = obtainMessage(1);
                    sendMessageDelayed(m, BehaviorRecordService.time*1000);
                }
            }
        }
    };

    private String getDate(){
        String time="";
        SimpleDateFormat formatter = new SimpleDateFormat ("yyyy-MM-dd HH:mm:ss");
        Date curDate = new Date(System.currentTimeMillis());//获取当前时间
        time = formatter.format(curDate);
        return time;
    }

}
