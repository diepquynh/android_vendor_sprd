package com.sprd.engineermode.telephony.userinfo;

import android.content.Context;
import android.location.LocationManager;
import android.net.ConnectivityManager;
import android.bluetooth.BluetoothAdapter;
import android.os.PowerManager;
import android.net.NetworkInfo;
import android.util.Log;

public class BehaviorInfoRecord{
    private static final String TAG = "BehaviorInfoRecord";
    private Context mContext;
    public BehaviorInfoRecord(Context context){
        Log.d(TAG,"constructor");
        mContext=context;
    }

  //GPS接口
    /**
    * @Title: gpsIsOpend
    * @Description: TODO 获取gps状态
    * @param: @return
    * @return: boolean
    * @throws
    */
    public boolean gpsIsOpend() {
         return ((LocationManager) mContext.getSystemService("location"))
                     .isProviderEnabled("gps");
    }

    //wifi接口
    /**
    * @Title: wifiIsLinked
    * @Description: TODO 获取wifi连接状态
    * @param: @return
    * @return: boolean
    * @throws
    */
    public boolean wifiIsLinked() {
        return ((ConnectivityManager) mContext.getSystemService("connectivity"))
                .getNetworkInfo(1).isConnected();
    }

    //BT接口
    /**
    * @Title: isBluetoothOpend
    * @Description: TODO 获取蓝牙设备的状态
    * @param: @return
    * @return: boolean
    * @throws
    */
    public boolean isBluetoothOpend() {
        BluetoothAdapter blueadapter = BluetoothAdapter.getDefaultAdapter();
        if (blueadapter == null) {// 没有蓝牙设备
            return false;
        }
        return blueadapter.isEnabled();
    }

    //数据连接接口
    /**
    * @Title: dataNetIsOpen
    * @Description: TODO 获取数据连接状态
    * @param: @return
    * @return: boolean
    * @throws
    */
    public boolean dataNetIsOpen() {
        NetworkInfo localNetworkInfo = ((ConnectivityManager) mContext
                .getSystemService("connectivity")).getNetworkInfo(0);
        boolean bool = false;
        if (localNetworkInfo != null)
            bool = localNetworkInfo.isConnected();
        return bool;
    }

    //获取屏幕状态
    /**
    * @Title: isScreenOn
    * @Description: TODO 获取屏幕状态
    * @param: @return
    * @return: boolean
    * @throws
    */
    public boolean isScreenOn() {
        PowerManager pm = (PowerManager)mContext.getSystemService(Context.POWER_SERVICE);
        return pm.isScreenOn();
    }
}
