package com.sprd.engineermode.telephony.userinfo;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.OutputStreamWriter;
import java.io.IOException;

import android.app.IntentService;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Environment;
import android.util.Log;

import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;

import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.debuglog.slogui.StorageUtil;
import com.sprd.engineermode.utils.IATUtils;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.telephony.TelephonyManager;
import android.telephony.ServiceState;
import android.telephony.PhoneStateListener;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionInfo;

public class NetinfoRecordService extends IntentService{
    private static final String TAG = "NetinfoRecordService";
    String saveFilePath="";
    FileOutputStream out=null;
    OutputStreamWriter osw=null;
    BufferedWriter netinfostatisticsBw=null,netinfoBw=null;
    File netinfostatisticsFile=null,netinfoFile=null;

    private static final int NETWORK_UNKNOW = 0;
    private static final int NETWORK_GSM = 1;
    private static final int NETWORK_TDSCDMA = 2;
    private static final int NETWORK_WCDMA = 3;
    private static final int NETWORK_LTE = 4;

    private static String InternalSavePath = "";
    private static String ExternalSavePath = "";

    NetinfoStatisticsRecord mInfoRecord;
    GsmShowRecord mGsmRecord;
    LteShowRecord mLteRecord;
    TdscdmaShowRecord mTdscdmaRecord;
    WcdmaShowRecord mWcdmaRecord;
    private SharedPreferences preferences,behavior_preferences;
    private SharedPreferences.Editor editor,behavior_editor;
    private SubscriptionManager mSubscriptionManager;
    private TelephonyManager mTelephonyManager;
    private Context mContext;
    private int mPhoneCount;

    private int[] mNetWorkType = {NETWORK_UNKNOW,NETWORK_UNKNOW};
    private boolean[] mListening = {false,false};
    public static int time=0;
    public static String[] serverFreq=new String[]{"0","0"};
    //public static String serverFreq="";

    public NetinfoRecordService(){
        super("NetinfoRecordService");
    }

    @Override
    public void onCreate(){
        super.onCreate();
        Log.d(TAG,"onCreate");
        mContext=this;

        mInfoRecord=new NetinfoStatisticsRecord(this);
        mGsmRecord=new GsmShowRecord(this);
        mLteRecord=new LteShowRecord(this);
        mTdscdmaRecord=new TdscdmaShowRecord(this);
        mWcdmaRecord=new WcdmaShowRecord(this);

        preferences = mContext
                .getSharedPreferences("netinforecord", mContext.MODE_PRIVATE);
        editor = preferences.edit();

        behavior_preferences = mContext
                .getSharedPreferences("behavior_record", mContext.MODE_PRIVATE);
        behavior_editor = behavior_preferences.edit();
        time=Integer.valueOf(preferences.getString("query_time", "8"));

        if(NetInfoRecordActivity.mIsOpenAllNetInfoItems){
            openAllItems();
        }else if(NetInfoRecordActivity.mIsCloseAllNetInfoItems){
            closeAllItems();
        }

        mPhoneCount = TelephonyManager.from(mContext).getPhoneCount();
        mTelephonyManager = (TelephonyManager) TelephonyManager
                .from(mContext);
        mSubscriptionManager = (SubscriptionManager) SubscriptionManager
                .from(mContext);

        if(checkSDCard()){
            saveFilePath=StorageUtil.getExternalStorage()+"/userinfo";
        }else{
            saveFilePath="/storage/emulated/0/userinfo";
        }
        initFilePath();
    }

    private void openAllItems(){
        editor.putString("netinfostatistics_record", "1");
        editor.putString("server_record", "1");
        editor.putString("neighbour_record", "1");
        editor.putString("adjacent_record", "1");
        editor.putString("outfield_record", "1");
        editor.putString("netinfo_record", "1");
        editor.commit();
    }
    private void closeAllItems(){
        editor.putString("netinfostatistics_record", "0");
        editor.putString("server_record", "0");
        editor.putString("neighbour_record", "0");
        editor.putString("adjacent_record", "0");
        editor.putString("outfield_record", "0");
        editor.putString("netinfo_record", "0");
        editor.commit();
    }
    private void initFilePath(){
        //saveFilePath=Environment.getInternalStoragePath().toString()+"/userinfo";
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
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId)
    {
     flags = Service.START_STICKY;
     return super.onStartCommand(intent, flags, startId);
     // return START_REDELIVER_INTENT;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.d(TAG,"onDestroy");
    }
    @Override
    protected void onHandleIntent(Intent intent){
        Log.d(TAG,"onHandleIntent");

        SimpleDateFormat formatter = new SimpleDateFormat ("yyyy-MM-dd");
        Date curDate = new Date(System.currentTimeMillis());//获取当前时间
        String fileName = formatter.format(curDate);
        Log.d(TAG,"file name is "+saveFilePath+"/"+fileName);
        netinfostatisticsFile = new File(saveFilePath+"/"+fileName+"_netinfostatics.csv");
        netinfoFile = new File(saveFilePath+"/"+fileName+"_netinfo.csv");
        createFile();
        mHandler.sendEmptyMessage(1);
    }

    private void createFile(){
        try{
            if(preferences.getString("netinfostatistics_record","0").equals("1")){
                if(!netinfostatisticsFile.exists()){
                    netinfostatisticsBw = new BufferedWriter(new FileWriter(netinfostatisticsFile,true));
                    String title="TIME,SIM,RESELECT,HANDOVER,ATTACHTIME,DROPTIMES";
                    netinfostatisticsBw.write(title);
                    netinfostatisticsBw.newLine();
                }
            }
            if(preferences.getString("netinfo_record","0").equals("1")){
                if(!netinfoFile.exists()){
                    netinfoBw = new BufferedWriter(new FileWriter(netinfoFile,true));
                    String title="TIME,SIM,ServerCell,NeighbourCell,AdjacentCell_one,AdjacentCell_two,Outfield";
                    netinfoBw.write(title);
                    netinfoBw.newLine();
                    editor.putString("server_freq_sim0", "0");
                    editor.putString("server_freq_sim1", "0");
                    editor.commit();
                }
                serverFreq[0]=preferences.getString("server_freq_sim0","0");
                serverFreq[1]=preferences.getString("server_freq_sim1","0");
            }
        }catch(Exception e){
            Log.d(TAG,e.getMessage());
        }finally{
            try{
                if(netinfostatisticsBw!=null){
                    netinfostatisticsBw.close();
                    netinfostatisticsBw=null;
                }
                if(netinfoBw!=null){
                    netinfoBw.close();
                    netinfoBw=null;
                }
            }catch(Exception e){
                Log.d(TAG,e.getMessage());
            }
        }
    }
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            Log.d(TAG,"handleMessage");
            if (msg.what == 1) {
                SimpleDateFormat formatter = new SimpleDateFormat ("yyyy-MM-dd");
                Date curDate = new Date(System.currentTimeMillis());//获取当前时间
                String fileName = formatter.format(curDate);
                if(checkSDCard()){
                    saveFilePath=StorageUtil.getExternalStorage()+"/userinfo";
                }else{
                    saveFilePath="/storage/emulated/0/userinfo";
                }
                initFilePath();
                Log.d(TAG,"file name is "+saveFilePath+"/"+fileName);
                netinfostatisticsFile = new File(saveFilePath+"/"+fileName+"_netinfostatics.csv");
                netinfoFile = new File(saveFilePath+"/"+fileName+"_netinfo.csv");
                if(!netinfostatisticsFile.exists() || !netinfoFile.exists()){
                    createFile();
                }

                formatter = new SimpleDateFormat ("yyyy-MM-dd HH:mm:ss");
                curDate = new Date(System.currentTimeMillis());//获取当前时间
                String time = formatter.format(curDate);
                try{
                    if(preferences.getString("netinfostatistics_record","0").equals("1")){
                        netinfostatisticsBw = new BufferedWriter(new FileWriter(netinfostatisticsFile,true));
                    }
                    if(preferences.getString("netinfo_record","0").equals("1")){
                        netinfoBw = new BufferedWriter(new FileWriter(netinfoFile,true));
                    }
                    for(int i=0;i<mPhoneCount;i++){
                        if(isSimExist(i)){
                            String sim="SIM"+i;
                            Log.d(TAG,"sim is"+sim);
                            String serverData="",neighbourData="",adjacentData="",adjacentData2="",outfieldData="";
                            if(preferences.getString("netinfostatistics_record","0").equals("1")){
                                Log.d(TAG,"netinfostatistics record");
                                netinfostatisticsBw.write(time+","+sim+","+mInfoRecord.getData(i));
                                netinfostatisticsBw.newLine();
                            }
                            if(preferences.getString("netinfo_record","0").equals("1")){
                                Log.d(TAG,"netinfo record");
                                updateDataNetType(i);

                                if(mNetWorkType[i]==NETWORK_GSM){
                                    if("1".equals(preferences.getString("server_record","0"))){
                                        serverData=mGsmRecord.getServerData(i);
                                    }
                                    if("1".equals(preferences.getString("neighbour_record","0"))){
                                        neighbourData=mGsmRecord.getNeighbourData(i);
                                    }
                                    if("1".equals(preferences.getString("adjacent_record","0"))){
                                        adjacentData=mGsmRecord.getAdjacent3GData(i);
                                        adjacentData2=mGsmRecord.getAdjacent4GData(i);
                                    }
                                    if("1".equals(preferences.getString("outfield_record","0"))){
                                        outfieldData=mGsmRecord.getOutfieldData(i);
                                    }
                                }else if(mNetWorkType[i]==NETWORK_TDSCDMA){
                                    if("1".equals(preferences.getString("server_record","0"))){
                                        serverData=mTdscdmaRecord.getServerData(i);
                                    }
                                    if("1".equals(preferences.getString("neighbour_record","0"))){
                                        neighbourData=mTdscdmaRecord.getNeighbourData(i);
                                    }
                                    if("1".equals(preferences.getString("adjacent_record","0"))){
                                        adjacentData=mTdscdmaRecord.getAdjacent2GData(i);
                                        adjacentData2=mTdscdmaRecord.getAdjacent4GData(i);
                                    }
                                    if("1".equals(preferences.getString("outfield_record","0"))){
                                        outfieldData=mTdscdmaRecord.getOutfieldData(i);
                                    }
                                }else if(mNetWorkType[i]==NETWORK_WCDMA){
                                    if("1".equals(preferences.getString("server_record","0"))){
                                        serverData=mWcdmaRecord.getServerData(i);
                                    }
                                    if("1".equals(preferences.getString("neighbour_record","0"))){
                                        neighbourData=mWcdmaRecord.getNeighbourData(i);
                                    }
                                    if("1".equals(preferences.getString("adjacent_record","0"))){
                                        adjacentData=mWcdmaRecord.getAdjacent2GData(i);
                                        adjacentData2=mWcdmaRecord.getAdjacent4GData(i);
                                    }
                                    if("1".equals(preferences.getString("outfield_record","0"))){
                                        outfieldData=mWcdmaRecord.getOutfieldData(i);
                                    }
                                }else if(mNetWorkType[i]==NETWORK_LTE){
                                    if("1".equals(preferences.getString("server_record","0"))){
                                        serverData=mLteRecord.getServerData(i);
                                    }
                                    if("1".equals(preferences.getString("neighbour_record","0"))){
                                        neighbourData=mLteRecord.getNeighbourData(i);
                                    }
                                    if("1".equals(preferences.getString("adjacent_record","0"))){
                                        adjacentData=mLteRecord.getAdjacent2GData(i);
                                        adjacentData2=mLteRecord.getAdjacent3GData(i);
                                    }
                                    if("1".equals(preferences.getString("outfield_record","0"))){
                                        outfieldData=mLteRecord.getOutfieldData(i);
                                    }
                                }
                                if(mNetWorkType[i] != NETWORK_UNKNOW){
                                    netinfoBw.write(time+","+sim+","+serverData+","+neighbourData+","+adjacentData+","+adjacentData2+","+outfieldData);
                                    netinfoBw.newLine();
                                }
                            }
                        }
                    }

                    int num_sim0=Integer.valueOf(behavior_preferences.getString("server_freq_changes0","0"));
                    int num_sim1=Integer.valueOf(behavior_preferences.getString("server_freq_changes1","0"));
                    if(!serverFreq[0].equals(preferences.getString("server_freq_sim0","0"))){
                        num_sim0++;
                        editor.putString("server_freq_sim0", serverFreq[0]);
                        behavior_editor.putString("server_freq_changes0", String.valueOf(num_sim0));
                    }
                    if(!serverFreq[1].equals(preferences.getString("server_freq_sim1","0"))){
                        num_sim1++;
                        editor.putString("server_freq_sim1", serverFreq[1]);
                        behavior_editor.putString("server_freq_changes1", String.valueOf(num_sim1));
                    }
                    //behavior_editor.putString("server_freq_changes", String.valueOf(num_sim0 > num_sim1 ? num_sim0 : num_sim1));
                    editor.commit();
                    behavior_editor.commit();
                }catch(Exception e){
                    Log.d(TAG,e.getMessage());
                }finally{
                    try{
                        if(netinfostatisticsBw!=null){
                            netinfostatisticsBw.close();
                            netinfostatisticsBw=null;
                        }
                        if(netinfoBw!=null){
                            netinfoBw.close();
                            netinfoBw=null;
                        }
                    }catch(Exception e){
                        Log.d(TAG,e.getMessage());
                    }
                }
                if(NetInfoRecordActivity.mIsCloseAllNetInfoItems){
                    closeAllItems();
                }
                if(NetInfoRecordActivity.mIsOpenNetinfoRecord){
                    Message m = obtainMessage(1);
                    sendMessageDelayed(m, NetinfoRecordService.time*1000);
                }
            }
        }
    };

    private final void updateDataNetType(int phoneId) {

        /* SPRD: modify by bug426493 @{ */
        int netType = TelephonyManager.NETWORK_TYPE_UNKNOWN;
        int[] subId = SubscriptionManager.getSubId(phoneId);
        if (subId != null) {
            if (SubscriptionManager.isValidSubscriptionId(subId[0])) {
               netType = mTelephonyManager.getVoiceNetworkType(subId[0]);
            }
        }

        switch(netType) {
        /* @} */
        case TelephonyManager.NETWORK_TYPE_EDGE:
        case TelephonyManager.NETWORK_TYPE_GPRS:
        case TelephonyManager.NETWORK_TYPE_CDMA:
        case TelephonyManager.NETWORK_TYPE_1xRTT:
        case TelephonyManager.NETWORK_TYPE_IDEN:
        case TelephonyManager.NETWORK_TYPE_GSM:
            mNetWorkType[phoneId] = NETWORK_GSM;
            Log.d(TAG, "Networktype is GSM");
            break;
        case TelephonyManager.NETWORK_TYPE_UMTS:
        case TelephonyManager.NETWORK_TYPE_HSDPA:
        case TelephonyManager.NETWORK_TYPE_HSUPA:
        case TelephonyManager.NETWORK_TYPE_HSPA:
        case TelephonyManager.NETWORK_TYPE_HSPAP:
        case TelephonyManager.NETWORK_TYPE_EHRPD:
        case TelephonyManager.NETWORK_TYPE_EVDO_0:
        case TelephonyManager.NETWORK_TYPE_EVDO_A:
        case TelephonyManager.NETWORK_TYPE_EVDO_B:
            mNetWorkType[phoneId] = check3GNetWork(phoneId);
            if (mNetWorkType[phoneId] == NETWORK_TDSCDMA) {
                Log.d(TAG, "Networktype is TDSCDMA");
            }
            if (mNetWorkType[phoneId] == NETWORK_WCDMA) {
                Log.d(TAG, "Networktype is WCDMA");
            }
            break;
        case TelephonyManager.NETWORK_TYPE_LTE:
            mNetWorkType[phoneId] = NETWORK_LTE;
            Log.d(TAG, "Networktype is LTE");
            break;
        default:
            mNetWorkType[phoneId] = NETWORK_UNKNOW;
            Log.d(TAG, "modem type is UnKnow");
            break;
        }
    }

 // Identify the 3G network type (Mobile and China Unicom)
    private int check3GNetWork(int mSimIndex) {
        String mServerName = "atchannel" + mSimIndex;
        String mATcmd = engconstents.ENG_AT_COPS;
        String mStrTmp = IATUtils.sendATCmd(mATcmd, mServerName);
        if (mStrTmp.contains(IATUtils.AT_OK)) {
            Log.d(TAG, mStrTmp);
            String[] strs = mStrTmp.split("\n");
            Log.d(TAG, strs[0]);
            if (strs[0].contains("46000") || strs[0].contains("46004") || strs[0].contains("00100")) {
                return NETWORK_TDSCDMA;
            } else {
                return NETWORK_WCDMA;
            }
        }
        return NETWORK_WCDMA;
    }

    // phoneId get subid
    public int slotIdToSubId(int phoneId) {
        int subId;
        SubscriptionInfo mSubscriptionInfo = mSubscriptionManager.getActiveSubscriptionInfoForSimSlotIndex(phoneId);
        if (mSubscriptionInfo == null) {
            Log.d(TAG,"mSubscriptionInfo is null");
            subId = SubscriptionManager.getDefaultSubscriptionId();
        } else {
            subId = mSubscriptionInfo.getSubscriptionId();
        }
        return subId;
     }

    private boolean isSimExist(int simIndex) {
        if (TelephonyManager.from(mContext).getSimState(simIndex) == TelephonyManager.SIM_STATE_READY) {
            return true;
        }
        return false;
    }

    protected boolean checkSDCard() {
        if (StorageUtil.getExternalStorageState()) {
            return true;
        }else {
            return false;
        }
    }
}
