
package com.sprd.omacp.transaction;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import android.os.Debug;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;

import com.sprd.process.list2hashmap.MyHashMap;
import com.sprd.process.list2hashmap.OtaMapData;
import com.sprd.xml.parser.prv.Define;
import com.sprd.xml.parser.prv.OmacpUtils;
import com.sprd.xml.parser.prv.PrintLogHelper;

class AppDataConfig {

    public int process(Context context, OtaMapData mapData) {
        Log.d(TAG, "enter AppDataConfig process()");
        if (Define.STATE_OK != apnDataConfig(context, mapData)) {
            Log.d(TAG, "apnDataConfig process error");
            return Define.ERROR_NO_APN;
        }
        if (Define.STATE_OK != emailDataConfig(context, mapData)) {
            Log.d(TAG, "emailDataConfig process error");
        }
        if (Define.STATE_OK != browseDataConfig(context, mapData)) {
            Log.d(TAG, "browseDataConfig process error");
        }
        return Define.STATE_OK;
    }

    // config Email Account data and send broadcast
    private int emailDataConfig(Context context, OtaMapData mapData) {
        int index = 0;
        int easIndex = 0;
        List<MyHashMap> emailDatas = mapData.get(Define.TYPE_EMAIL);
        List<MyHashMap> easDatas = mapData.get(Define.TYPE_EXCHANGE);
        if ((emailDatas == null || emailDatas.isEmpty())
                && (easDatas == null || easDatas.isEmpty())) {
            Log.d(TAG, "emailDatas and easDatas is null or empty");
            return Define.STATE_PARAM_ERROR;
        }
        Intent emailIntent = new Intent(Define.ACTION_EMAIL);
        for (MyHashMap emailMap : emailDatas) {
            ++index;
            Bundle emailBundle = new Bundle();
            Set<String> emailSet = emailMap.keySet();
            for (String key : emailSet) {
                emailBundle.putString(key, emailMap.get(key));
            }
            //if (Debug.isDebug()) {
                Log.d(TAG, "[emailData" + index + "]=[" + emailBundle.toString() + "]");
            //}
            emailIntent.putExtra("emailData" + index, emailBundle);
        }
        for (MyHashMap easMap : easDatas) {
            ++easIndex;
            Bundle easBundle = new Bundle();
            Set<String> easSet = easMap.keySet();
            for (String key : easSet) {
                easBundle.putString(key, easMap.get(key));
            }
            //if (Debug.isDebug()) {
                Log.d(TAG, "[easData" + easIndex + "]=[" + easBundle.toString() + "]");
            //}
            emailIntent.putExtra("easData" + easIndex, easBundle);
        }
        emailIntent.putExtra("email_size", emailDatas.size());
        emailIntent.putExtra("eas_size", easDatas.size());
        emailIntent.putExtra("version", Define.OMA_VERSION);
        context.sendOrderedBroadcast(emailIntent, null);
        Log.d(TAG, "send email boardcast success!");
        index = 0;
        return Define.STATE_OK;
    }

    // config Browse data and send broadcast
    private int browseDataConfig(Context context, OtaMapData mapData) {
        int index = 0;
        List<MyHashMap> bookMarks = mapData.get(Define.TYPE_BOOKMARK);
        List<MyHashMap> startPage = mapData.get(Define.TYPE_STARTPAGE);
        if ((bookMarks == null || bookMarks.isEmpty())
                && (startPage == null || startPage.isEmpty())) {
            Log.d(TAG, "bookMarks and startPage is null");
            return Define.STATE_PARAM_ERROR;
        }
        Intent browseIntent = new Intent(Define.ACTION_BROWSER);

        if (bookMarks != null && !bookMarks.isEmpty()) {
            List<Bundle> bookMarkList = new ArrayList<Bundle>();
            // config bookmark
            for (MyHashMap myHashMap : bookMarks) {
                ++index;
                Bundle browseBundle = new Bundle();
                browseBundle.putString("name", myHashMap.get("resource_name"));
                browseBundle.putString("uri", myHashMap.get("uri"));
                browseIntent.putExtra("bookmark" + index, browseBundle);
                //if (Debug.isDebug()) {
                    Log.d(TAG, "[browseBundle" + index + "]=[" + browseBundle.toString() + "]");
                    PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                            "\n[browseBundle" + index + "]=[" + browseBundle.toString() + "]");
                //}
            }
            browseIntent.putExtra("bookmark_size", bookMarks.size());
        }

        // config startpage
        if (startPage != null && !startPage.isEmpty()) {
            browseIntent.putExtra("startpage", startPage.get(0).get("uri"));
        } else {
            browseIntent.putExtra("startpage", "");
        }
        //if (Debug.isDebug()) {
            Log.d(TAG, "[startpage] = [" + browseIntent.getExtras().getString("startpage") + "]");
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                    "\n[startpage] = [" + browseIntent.getExtras().getString("startpage") + "]");
        //}
        browseIntent.putExtra("version", Define.OMA_VERSION);
        context.sendOrderedBroadcast(browseIntent, null);
        Log.d(TAG, "send browse boardcast success!");
        index = 0;
        return Define.STATE_OK;
    }

    // config Apn data and send broadcast
    private int apnDataConfig(Context context, OtaMapData mapData) {
        int apnIndex = 0;
        // add for bug 483819 begin
        int appIndex = 0;
        // add for bug 483819 end
        int mmsIndex = 0;
        int bootIndex = 0;
        List<MyHashMap> apnList = mapData.get(Define.TYPE_APN);
        List<MyHashMap> mmsList = mapData.get(Define.TYPE_MMS);
        List<MyHashMap> bootList = mapData.get(Define.CHAR_BOOTSTRAP);
        // add for bug 483819 begin
        List<MyHashMap> appList = mapData.get(Define.CHAR_APPLICATION);
        // add for bug 483819 end
        if ((apnList == null || apnList.isEmpty()) && (bootList == null || bootList.isEmpty())) {
            Log.d(TAG, "apnList and bootList is null or empty");
            return Define.STATE_PARAM_ERROR;
        }

        // compatible for email script of HuaWei
        if ((apnList == null || apnList.isEmpty())) {
            for (MyHashMap myHashMap : bootList) {
                if (TextUtils.isEmpty(myHashMap.get(Define.PROXY_ID))) {
                    Log.d(TAG, "bootstarp don't have the element named proxy-id");
                    return Define.STATE_PARAM_ERROR;
                }
            }
            Log.d(TAG, "apnList is empty, but the bootstarp have the element named proxy-id");
            // add for bug 531344 begin
            return Define.ERROR_NO_APN;
            // add for bug 531344 end
        }
        // apn data
        Intent apnIntent = new Intent(Define.ACTION_APN);
        for (MyHashMap apnMap : apnList) {
            ++apnIndex;
            Bundle apnBundle = new Bundle();
            Set<String> apnSet = apnMap.keySet();
            for (String key : apnSet) {
                apnBundle.putString(key, apnMap.get(key));
            }
            apnIntent.putExtra("apn" + apnIndex, apnBundle);
            //if (Debug.isDebug()) {
                Log.d(TAG, "[apnBundle" + apnIndex + "]=[" + apnBundle.toString() + "]");
                PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                        "\n[apnBundle" + apnIndex + "]=[" + apnBundle.toString() + "]");
            //}
        }
        apnIntent.putExtra("apn_size", apnList.size());
        //add for bug 483819 begin
        // mms data
        /*if (mmsList == null || mmsList.isEmpty()) {
            apnIntent.putExtra("mms_size", 0);
        } else {
            for (MyHashMap mmsMap : mmsList) {
                ++mmsIndex;
                Bundle mmsBundle = new Bundle();
                Set<String> mmsSet = mmsMap.keySet();
                for (String key : mmsSet) {
                    mmsBundle.putString(key, mmsMap.get(key));
                }
                apnIntent.putExtra("mms" + mmsIndex, mmsBundle);
                if (Debug.isDebug()) {
                    Log.d(TAG, "[mmsBundle" + mmsIndex + "]=[" + mmsBundle.toString() + "]");
                    PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                            "\n[mmsBundle" + mmsIndex + "]=[" + mmsBundle.toString() + "]");
                }
            }
            apnIntent.putExtra("mms_size", mmsList.size());
        }*/
        // app data
        if (appList == null || appList.isEmpty()) {
            apnIntent.putExtra("app_size", 0);
        } else {
            for (MyHashMap appMap : appList) {
                ++appIndex;
                Bundle appBundle = new Bundle();
                Set<String> appSet = appMap.keySet();
                for (String key : appSet) {
                    appBundle.putString(key, appMap.get(key));
                }
                apnIntent.putExtra("app" + appIndex, appBundle);
                //if (Debug.isDebug()) {
                    Log.d(TAG, "[appBundle" + appIndex + "]=[" + appBundle.toString() + "]");
                    PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                            "\n[appBundle" + appIndex + "]=[" + appBundle.toString() + "]");
                //}
            }
            apnIntent.putExtra("app_size", appList.size());
        }
        // add for bug 483819 end
        // bootstarp data
        if (bootList == null || bootList.isEmpty()) {
            apnIntent.putExtra("bootstarp_size", 0);
        } else {
            apnIntent.putExtra("bootstarp_size", bootList.size());
            for (MyHashMap bootMap : bootList) {
                ++bootIndex;
                Bundle bootBundle = new Bundle();
                Set<String> bootSet = bootMap.keySet();
                for (String key : bootSet) {
                    bootBundle.putString(key, bootMap.get(key));
                }
                apnIntent.putExtra("bootstarp" + bootIndex, bootBundle);
                //if (Debug.isDebug()) {
                    Log.d(TAG, "[bootBundle" + bootIndex + "]=[" + bootBundle.toString() + "]");
                    PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                            "\n[bootBundle" + bootIndex + "]=[" + bootBundle.toString() + "]");
                //}
            }
            apnIntent.putExtra("bootstarp_size", bootList.size());
        }
        apnIntent.putExtra("version", Define.OMA_VERSION);
        apnIntent.putExtra("subid", OmacpUtils.getSubId());
        context.sendOrderedBroadcast(apnIntent, null);
        Log.d(TAG, "send apn boardcast success! the subId = " + OmacpUtils.getSubId());
        apnIndex = 0;
        mmsIndex = 0;
        bootIndex = 0;
        return Define.STATE_OK;
    }

    public final String TAG = "AppDataConfig";
}
