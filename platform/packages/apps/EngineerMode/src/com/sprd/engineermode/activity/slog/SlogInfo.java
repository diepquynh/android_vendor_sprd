package com.sprd.engineermode.activity.slog;

import com.sprd.engineermode.R;
import android.graphics.drawable.Drawable;
import com.sprd.engineermode.core.EngineerModeProtocol;
import com.sprd.engineermode.debuglog.slogui.ISlogService;
import com.sprd.engineermode.core.SlogCore;
import com.sprd.engineermode.debuglog.slogui.SlogAction;
import com.sprd.engineermode.debuglog.slogui.SlogService;
import com.sprd.engineermode.debuglog.slogui.StorageUtil;
import org.apache.http.NameValuePair;
import org.apache.http.message.BasicNameValuePair;
import org.json.JSONObject;
import android.util.*;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.Context;
import android.app.Activity;
import android.os.IBinder;
import android.os.SystemProperties;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.ContextWrapper;



import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

/**
 * Created by SPREADTRUM\zhengxu.zhang on 9/8/15.
 */
public class SlogInfo {

    private static SlogInfo slogInfo = null;
    public static Context x;

    public static SlogInfo self(){
        if(slogInfo==null)
        {
            SlogCore.buildDic();
            slogInfo = new SlogInfo();
            slogInfo.readData();
            slogInfo.slogStartTime = new Timer();
        }
        return slogInfo;
    }

    private String TAG="SlogInfo";

    private List<NameValuePair> switchParams = new ArrayList<NameValuePair>();

    private Timer slogStartTime ;
    private long slogRunningTime;
    private RequestTimerTask timerTask;
    class RequestTimerTask extends TimerTask {
        public void run() {
            slogRunningTime += 1000;
        }
    }

    public void resetStartTime() {
        if(slogStartTime != null) {
            slogStartTime.cancel();
        }
        slogStartTime = new Timer();
        timerTask = new RequestTimerTask();
        slogRunningTime = 0;
        slogStartTime.schedule(timerTask, 1000, 1000);
    }

    public void instanceCloseScene(){
        switchParams.clear();

        switchParams.add(new BasicNameValuePair("SlogCore_MainLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_EventLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_SystemLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_RadioLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_KernelLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_BtHciLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_CrashLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_APCapLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ModemLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_DspLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_WcnLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_GpsLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_CpCapLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ArmLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_DspPcmDataController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ArmPcmDataController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_SimLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_Cm4LogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspOutputController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspPcmDumpLogController","0"));
    }

    public void instanceNormalScene(){
        switchParams.clear();

        switchParams.add(new BasicNameValuePair("SlogCore_MainLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_EventLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_SystemLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_RadioLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_KernelLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_BtHciLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_CrashLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_APCapLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ModemLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_DspLogController","2"));
        switchParams.add(new BasicNameValuePair("SlogCore_WcnLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_GpsLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_CpCapLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ArmLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_DspPcmDataController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ArmPcmDataController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_SimLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_Cm4LogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspOutputController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspPcmDumpLogController","0"));
    }

    public void instanceDataScene(){
        switchParams.clear();

        switchParams.add(new BasicNameValuePair("SlogCore_MainLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_EventLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_SystemLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_RadioLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_KernelLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_BtHciLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_CrashLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_APCapLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_ModemLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_DspLogController","2"));
        switchParams.add(new BasicNameValuePair("SlogCore_WcnLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_GpsLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_CpCapLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_ArmLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_DspPcmDataController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ArmPcmDataController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_SimLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_Cm4LogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspOutputController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspPcmDumpLogController","0"));
    }
    public void instanceVoiceScene(){
        switchParams.clear();

        switchParams.add(new BasicNameValuePair("SlogCore_MainLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_EventLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_CrashLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_SystemLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_RadioLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_KernelLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_BtHciLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_APCapLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ModemLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_DspLogController","2"));
        switchParams.add(new BasicNameValuePair("SlogCore_WcnLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_GpsLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_CpCapLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ArmLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_DspPcmDataController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_ArmPcmDataController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_SimLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_Cm4LogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspOutputController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspPcmDumpLogController","0"));
    }
    public void instanceModemScene(){
        switchParams.clear();

        switchParams.add(new BasicNameValuePair("SlogCore_MainLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_EventLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_CrashLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_SystemLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_RadioLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_KernelLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_BtHciLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_APCapLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ModemLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_DspLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_WcnLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_GpsLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_CpCapLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_ArmLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_DspPcmDataController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ArmPcmDataController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_SimLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_Cm4LogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspOutputController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspPcmDumpLogController","0"));
    }
    public void instanceWcnScene(){
        switchParams.clear();

        switchParams.add(new BasicNameValuePair("SlogCore_MainLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_EventLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_CrashLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_SystemLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_RadioLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_KernelLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_BtHciLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_APCapLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ModemLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_DspLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_WcnLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_GpsLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_CpCapLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ArmLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_DspPcmDataController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ArmPcmDataController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_SimLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_Cm4LogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspOutputController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspPcmDumpLogController","0"));
    }
    public void instanceSimScene(){
        switchParams.clear();

        switchParams.add(new BasicNameValuePair("SlogCore_MainLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_EventLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_CrashLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_SystemLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_RadioLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_KernelLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_BtHciLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_APCapLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ModemLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_DspLogController","2"));
        switchParams.add(new BasicNameValuePair("SlogCore_WcnLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_GpsLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_CpCapLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ArmLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_DspPcmDataController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_ArmPcmDataController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_SimLogController","1"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspLogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_Cm4LogController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspOutputController","0"));
        switchParams.add(new BasicNameValuePair("SlogCore_AGDspPcmDumpLogController","0"));
    }

    public void closeScene(){
        Log.d(TAG,"close Scene");
        instanceCloseScene();

        //sceneStatus = SceneStatus.close;
        slog_tmp = sceneStatus;
        sendCmd();
    }

    public void openNormalScene(){
        Log.d(TAG,"open normal Scene2");
        instanceNormalScene();
        Log.d(TAG,"open normal Scene3");

        sceneStatus = SceneStatus.normal;
        slog_tmp = SceneStatus.close;
        sendCmd();
        resetStartTime();
    }

    public void openDataScene(){
        Log.d(TAG,"open data Scene2");
        instanceDataScene();
        Log.d(TAG,"open data Scene3");

        sceneStatus = SceneStatus.data;
        slog_tmp = SceneStatus.close;
        sendCmd();
        resetStartTime();
    }

    public void openVoiceScene(){
        Log.d(TAG,"open voice Scene2");
        instanceVoiceScene();
        Log.d(TAG,"open voice Scene3");

        sceneStatus = SceneStatus.voice;
        slog_tmp = SceneStatus.close;
        sendCmd();
        resetStartTime();
    }

    public void openModemScene(){
        Log.d(TAG,"open modem Scene2");
        instanceModemScene();
        Log.d(TAG,"open modem Scene3");

        sceneStatus = SceneStatus.modem;
        slog_tmp = SceneStatus.close;
        sendCmd();
        resetStartTime();
    }

    public void openWcnScene(){
        Log.d(TAG,"open wcn Scene2");
        instanceWcnScene();
        Log.d(TAG,"open wcn Scene3");
        sceneStatus = SceneStatus.wcn;
        slog_tmp = SceneStatus.close;
        sendCmd();
        resetStartTime();
    }

    public void openSimScene(){
        Log.d(TAG,"open sim Scene2");
        instanceSimScene();
        Log.d(TAG,"open sim Scene3");
        sceneStatus = SceneStatus.sim;
        slog_tmp = SceneStatus.close;
        sendCmd();
        resetStartTime();
    }

    public void openCustomerScene(){
        sceneStatus = SceneStatus.customer;
        slog_tmp = SceneStatus.close;
        commitCustomerOrder();
        resetStartTime();
    }

    public void open(SceneStatus s){
        switch (s){
            case close:
                // SPRD: Bug 557464 normal log button display is not same with scence activity
                //open(slog_tmp);
                break;
            case normal:
                openNormalScene();
                break;
            case data:
                openDataScene();
                break;
            case voice:
                openVoiceScene();
                break;
            case modem:
                openModemScene();
                break;
            case wcn:
                openWcnScene();
                break;
            case sim:
                openSimScene();
                break;
            case customer:
            	openCustomerScene();
        }
    }

    public String toString(){
        switch (sceneStatus){
            case close:
                return "close";
            case normal:
                return "normal";
            case data:
                return "data";
            case voice:
                return "voice";
            case modem:
                return "modem";
            case wcn:
                return "wcn";
            case sim:
                return "sim";
            case customer:
            	return "customer";
        }
        return "close";
    }

    public void openUserDefinedScene(){
        switchParams.clear();
    }

    private void sendCmd() {
        try {
            for (NameValuePair pair : switchParams) {
                Log.d(TAG, pair.getName() + pair.getValue());
                EngineerModeProtocol.sendCommand(pair.getName(), pair.getValue());
            }
            writeData();
        } catch (Exception e) {
            Log.d(TAG, "Exception is " + e);
        }
    }

    private SceneStatus sceneStatus;

    public SceneStatus getSceneStatus(){
        return sceneStatus;
    }

    public void setSceneStatus(SceneStatus sceneStatus){
        this.sceneStatus = sceneStatus;
    }

    public enum SceneStatus{
        close ,
        normal ,
        data,
        voice,
        modem,
        wcn,
        sim,
        customer
    }

    public String getSceneInfo(SceneStatus s){
        switch (s){
            case close:
                // SPRD: Bug 557464 normal log button display is not same with scence activity
                //return getSceneInfo(slog_tmp);
                return x.getString(R.string.slog_info_no);
            case normal:
            	if(SlogCore.getGnssStatus()){
                    return "main log, event log, system log, crash log, radio log, kernel log, bt hci log, modem log, dsp log, wcn log, gnss log";
            	}else{
                    return "main log, event log, system log, crash log, radio log, kernel log, bt hci log, modem log, dsp log, wcn log";
            	}
            case data:
                return "main log, event log, system log, crash log, radio log, kernel log, apcap log, modem log, dsp log, cpcap log";
            case voice:
                return "main log, event log, system log, crash log, radio log, kernel log, modem log, dsp log, dsp pcm data, arm pcm data";
            case modem:
                return "Output Modem Log from Uart!";
            case wcn:
                return "Output WCN Log from Uart!";
            case sim:
                return "main log, event log, system log, crash log, radio log, kernel log, modem log, dsp log, SIM drive Log";
            case customer:
            {
            	String info  = "";
            	boolean isMore=false;
            	for(NameValuePair pair:SlogCore.DIC_LOG_NAME){

            		if(getCustomerDefined(pair.getName()).equals("1")){
                		if(isMore){
                			info+=", ";
                		}
            			info+=pair.getValue();
            			isMore=true;
            		}
            		
            	}
                return info;
            }
        }
        return "nothing";
    }
    public String getTitle(SceneStatus s){
        switch (s){
            case close:
                // SPRD: Bug 557464 normal log button display is not same with scence activity
                //return getTitle(slog_tmp);
                return x.getString(R.string.slog_no_scene);
            case normal:
                return x.getString(R.string.slog_normal_scene);
            case data:
                return x.getString(R.string.slog_data_scene);
            case voice:
                return x.getString(R.string.slog_voice_scene);
            case modem:
                return x.getString(R.string.slog_modem_scene);
            case wcn:
                return x.getString(R.string.slog_wcn_scene);
            case sim:
                return x.getString(R.string.slog_sim_scene);
            case customer:
            	return x.getString(R.string.slog_custom_scene);
        }
        return "nothing";
    }

    public long getSlogStartTime(){
        return slogRunningTime;
    }

    public  SharedPreferences mySharedPreferences;
    private  SharedPreferences mCustomerSharedPreferences;

    public void writeData()
    {
        Log.d("slog","写入场景");
        mySharedPreferences = x.getSharedPreferences("test", Activity.MODE_PRIVATE);
        SharedPreferences.Editor editor = mySharedPreferences.edit();

        if(slog_tmp!=SceneStatus.close){
            Log.d("slog_scene","xie name:"+"close");
            editor.putString("slog_scene", "close");
        }
        else{
            editor.putString("slog_scene", SlogInfo.self().toString());
            Log.d("slog_scene","xie name:"+SlogInfo.self().toString());
        }
        editor.commit();
    }
    public String readData()
    {
        Log.d("slog","读取场景");
        mySharedPreferences = x.getSharedPreferences("test",Activity.MODE_PRIVATE);
        String name = mySharedPreferences.getString("slog_scene", "");
        Log.d("slog_scene","du name:"+name);
        if(name.equals("close")){
            sceneStatus = SceneStatus.close;
            slog_tmp = SceneStatus.normal;
            instanceCloseScene();
        }else if(name.equals("data")){
            sceneStatus = SceneStatus.data;
            instanceDataScene();
        }else if(name.equals("normal")){
            sceneStatus = SceneStatus.normal;
            instanceNormalScene();
        }else if(name.equals("voice")){
            sceneStatus = SceneStatus.voice;
            instanceVoiceScene();
        }else if(name.equals("modem")){
            sceneStatus = SceneStatus.modem;
            instanceModemScene();
        }else if(name.equals("sim")){
            sceneStatus = SceneStatus.sim;
            instanceSimScene();
        }else if(name.equals("wcn")){
            sceneStatus = SceneStatus.wcn;
            instanceWcnScene();
        }else if(name.equals("customer")){
        	sceneStatus = SceneStatus.customer;
        	SlogCore.SlogCore_GetDspLogStatus();
        	loadCustomerOrder();
        	commitCustomerOrder();
        }else if(SystemProperties.get("ro.build.type").equalsIgnoreCase("user")){
            sceneStatus = SceneStatus.close;
            slog_tmp = SceneStatus.normal;
            instanceCloseScene();
        }else{
            sceneStatus = SceneStatus.normal;
            instanceNormalScene();
        }
        return name;
    }

    public SceneStatus slog_tmp= SceneStatus.close;
    
    public List<NameValuePair> customer = new ArrayList<NameValuePair>();
    
    public List<NameValuePair> customerTmp = new ArrayList<NameValuePair>();
    
    public String getCustomer(String name){
        for(NameValuePair pair:customerTmp){
        	if(pair.getName().equals(name)){
        		return pair.getValue();
        	}
        }
        Log.d(TAG, String.format("%s , this arg not existed return 0 ",name));
        return "0";
    }
    
    public String getCustomerDefined(String name){
        for(NameValuePair pair:customer){
        	if(pair.getName().equals(name)){
        		return pair.getValue();
        	}
        }
        Log.d(TAG, String.format("%s , this arg not existed return 0 ",name));
        return "0";
    }
    
    public void setCustomer(String name,String value){
    	int i = 0;
        for(NameValuePair pair:customerTmp){
        	if(pair.getName().equals(name)){
        		customerTmp.set(i, new BasicNameValuePair(name,value));
        		Log.d(TAG,String.format("set:%s value:%s",name,value));
        		return;
        	}
        	++i;
        }
        customerTmp.add(new BasicNameValuePair(name,value));
        Log.d(TAG,String.format("add:%s value:%s",name,value));
    }
    
    public void commitCustomerOrder(){
    	Log.d(TAG, "user commit order:");
    	customer = new ArrayList<NameValuePair>(customerTmp);
    	saveCustomerOrder();
        for(NameValuePair pair:customer){
            Log.d(TAG,pair.getName()+pair.getValue());
            EngineerModeProtocol.sendCommand(pair.getName(), pair.getValue());
        }
        slog_tmp = SceneStatus.close;
        writeData();
    }
    
    public void saveCustomerOrder(){
    	mCustomerSharedPreferences = x.getSharedPreferences("customerOrder",Activity.MODE_PRIVATE);
        SharedPreferences.Editor editor = mCustomerSharedPreferences.edit();
        editor.clear();
    	for(NameValuePair pair:customer){
            Log.d(TAG,"save customer order:"+pair.getName()+pair.getValue());
            editor.putString(pair.getName(), pair.getValue());
            
        }
    	editor.commit();
    }
    
    public void loadCustomerOrder(){
    	mCustomerSharedPreferences = x.getSharedPreferences("customerOrder",Activity.MODE_PRIVATE);
        instanceCloseScene();
        customerTmp.clear();
    	for(NameValuePair pair:switchParams){
            Log.d(TAG,"load customer order:"+pair.getName()+
            		"value:"+mCustomerSharedPreferences.getString(pair.getName(), "0"));
            customerTmp.add(new BasicNameValuePair(pair.getName(),
            		mCustomerSharedPreferences.getString(pair.getName(), "0")));
        }
    	customer = new ArrayList<NameValuePair>(customerTmp);
    }
    /** Start the slog daemon, and then slog can work. */
    private static final void startSlogDaemon() {
     // If the daemon started, no need to set it again.
        if (SystemProperties.getBoolean("persist.sys.slog.enabled", false)) return;
        Log.v("duke", "start daemon now");
        new File("/data/local/tmp/slog").mkdir();
        SystemProperties.set("persist.sys.slog.enabled", "1");
    }
    
    
}
