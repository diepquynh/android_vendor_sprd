
package com.sprd.omacp.transaction;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import com.sprd.omacp.elements.ElementsMgr;
import com.sprd.process.dom2list.Dom2ListMgr;
import com.sprd.process.list2hashmap.ApnDataHandler;
import com.sprd.process.list2hashmap.BaseHandlerImpl;
import com.sprd.process.list2hashmap.EmailDataHandler;
import com.sprd.process.list2hashmap.ExchangeDataHandler;
import com.sprd.process.list2hashmap.List2MapMgr;
import com.sprd.process.list2hashmap.MapProcessManager;
import com.sprd.process.list2hashmap.MmsDataHandler;
import com.sprd.process.list2hashmap.MyHashMap;
import com.sprd.xml.parser.impl.DefaultHandler;
import com.sprd.xml.parser.itf.ISet;
import com.sprd.xml.parser.prv.DebugVisitor;
import com.sprd.xml.parser.prv.Define;
import com.sprd.xml.parser.prv.PrintLogHelper;

import android.content.Context;
import android.content.Intent;
import android.os.Debug;
import android.util.Log;

/*
 * 
 * */
public class OtaOmaManager {

    private OtaOmaManager() {
    }

    public OtaOmaManager(Intent intent, int serviceId) {
        PrintLogHelper.setFilePath(LOG_PATH);
        mAppDataConfig = new AppDataConfig();
        mOtaParser = new OtaDefaultParser();
        mOtaPreParser = new OtaPreParser(intent, serviceId);

    }

    public int parse(Context context, String inputPin) {
        Log.e(TAG, "in OmacpManager.apk enter parse()");
        // add for bug 524981 begin
        int ret = getOtaPreParser().securityCheck(context, inputPin);
        // add for bug 524981 end
        if (ret == Define.STATE_OK) {
            // we should setData and setMimeType before parse it.
            if (getOtaPreParser().getData() != null && getOtaPreParser().getMimeType() != null) {
                getOtaParser().setData(getOtaPreParser().getData());
                getOtaParser().setMimeType(getOtaPreParser().getMimeType());
            }
            // parse data and generate DOM tree
            if (getOtaParser().parse(context) != Define.STATE_OK) {
                Log.d(TAG, "getOtaParser() parse error");
            }

            // debug :
            Log.d(TAG, "<<<----------------before Debug visitor---------------------");
            // DefaultHandler.getNodeIns().getRoot().Debug();
            // DebugVisitor.VisitorParam param = new
            // DebugVisitor.VisitorParam();
            // DebugVisitor dv = new DebugVisitor();
            // DefaultHandler.getNodeIns().visitor(DefaultHandler.getInstance().getRoot(),
            // param, dv);
            Log.d(TAG, "-------------------end Debug visitor--------------------->>>");

            // DOM tree to list
            int ndom2ListRet = dom2List();
            if (ndom2ListRet != Define.STATE_OK) {
                Log.d(TAG, "dom2List() return : " + ndom2ListRet);
                return Define.STATE_ERROR;
            }

            // List to HashMap
            int list2MapRet = list2Map();
            if (list2MapRet != Define.STATE_OK) {
                Log.d(TAG, "list2Map() return : " + list2MapRet);
                return Define.STATE_ERROR;
            }

            // check the elements of OMA xml script
            Log.d(TAG, "<<<-------------------before checkKeys---------------------");
            checkKeys();
            Log.d(TAG, "-------------------end checkKeys------------------------>>>");
            // Apps data config
            int nRet = getAppDataConfig().process(context,
                    List2MapMgr.getInstance().getOtaMapData());
            if (nRet != Define.STATE_OK) {
                Log.d(TAG, "getAppDataConfig process return STATE : " + nRet);
                ret = nRet;
            }
            // Debug if the version is userDebug
            //if (Debug.isDebug()) {
                Debug();
            //}
            // clear All data
            clearData();
        }
        if (DEBUG) {
            Log.d(TAG, "parse() securityCheck ret:" + getOtaPreParser().decodeErrorType(ret));
        }

        return ret;
    }

    private int dom2List() {
        return Dom2ListMgr.getInstance().dom2ListProcess(DefaultHandler.getNodeIns());
    }

    private int list2Map() {
        int nRet = List2MapMgr.getInstance().list2MapProcess(
                Dom2ListMgr.getInstance().getOtaListData());
        if (nRet != Define.STATE_OK) {
            System.out.println("list2Map return STATE_ERROR");
            return nRet;
        }
        List<BaseHandlerImpl> handlerList = new ArrayList<BaseHandlerImpl>();
        handlerList.add(new EmailDataHandler());
        handlerList.add(new MmsDataHandler());
        handlerList.add(new ApnDataHandler());
        handlerList.add(new ExchangeDataHandler());
        for (BaseHandlerImpl baseHandler : handlerList) {
            MapProcessManager.getInstance().setBaseHandler(baseHandler);
            nRet = MapProcessManager.getInstance().mapDataProcess(
                    List2MapMgr.getInstance().getOtaMapData());
        }
        return nRet;
    }

    private void Debug() {
        DebugVisitor.VisitorParam param = new DebugVisitor.VisitorParam();
        DebugVisitor dv = new DebugVisitor();
        DefaultHandler.getNodeIns().visitor(DefaultHandler.getInstance().getRoot(), param, dv);
        List2MapMgr.getInstance().getOtaMapData().Debug();
    }

    private void checkKeys() {
        // check the elements of OMA xml
        HashMap<Integer, ISet> elemMap = ElementsMgr.CreateInstance().getList();
        // check APN
        System.out.println("\n<<<------------------Check APN Elements begin----------------------");
        for (MyHashMap myHashMap : List2MapMgr.getInstance().getOtaMapData().get(Define.TYPE_APN)) {
            System.out.println("<<<----------------------item------------------------");
            List2MapMgr.getInstance().getOtaMapData().DebugHashMap(myHashMap);
            System.out.println("\n-----------------------item---------------------->>>");
            int nRet = ElementsMgr.CreateInstance().getList().get(ISet.OTA_APN)
                    .check(myHashMap.keySet());
            System.out.println("\nnRet = " + Integer.toHexString(nRet));
        }
        System.out.println("\n-------------------Check APN Elements end----------------------->>>");

        System.out
                .println("\n<<<------------------Check Email Elements begin----------------------");
        for (MyHashMap myHashMap : List2MapMgr.getInstance().getOtaMapData().get(Define.TYPE_EMAIL)) {
            System.out.println("<<<----------------------item------------------------");
            List2MapMgr.getInstance().getOtaMapData().DebugHashMap(myHashMap);
            System.out.println("\n-----------------------item---------------------->>>");
            int nRet = ElementsMgr.CreateInstance().getList().get(ISet.OTA_EMAIL)
                    .check(myHashMap.keySet());
            System.out.println("\nnRet = " + Integer.toHexString(nRet));
        }
        System.out
                .println("\n-------------------Check Email Elements end-------------------------->>>");

        System.out
                .println("\n<<<------------------Check BookMark Elements begin----------------------");
        for (MyHashMap myHashMap : List2MapMgr.getInstance().getOtaMapData()
                .get(Define.TYPE_BOOKMARK)) {
            System.out.println("<<<----------------------item------------------------");
            List2MapMgr.getInstance().getOtaMapData().DebugHashMap(myHashMap);
            System.out.println("\n-----------------------item---------------------->>>");
            int nRet = ElementsMgr.CreateInstance().getList().get(ISet.OTA_BOOKMARK)
                    .check(myHashMap.keySet());
            System.out.println("\nnRet = " + Integer.toHexString(nRet));
        }
        System.out
                .println("\n-------------------Check BookMark Elements end---------------   ----->>>");

        System.out
                .println("\n<<<------------------Check StartPage Elements begin----------------------");
        for (MyHashMap myHashMap : List2MapMgr.getInstance().getOtaMapData()
                .get(Define.TYPE_STARTPAGE)) {
            System.out.println("<<<----------------------item------------------------");
            List2MapMgr.getInstance().getOtaMapData().DebugHashMap(myHashMap);
            System.out.println("\n-----------------------item---------------------->>>");
            int nRet = ElementsMgr.CreateInstance().getList().get(ISet.OTA_STARTPAGE)
                    .check(myHashMap.keySet());
            System.out.println("\nnRet = " + Integer.toHexString(nRet));
        }
        System.out
                .println("\n----------------Check StartPage Elements end--------------------------->>>");
        System.out
                .println("\n<<<------------------Check Exchange Elements begin----------------------");
        for (MyHashMap myHashMap : List2MapMgr.getInstance().getOtaMapData()
                .get(Define.TYPE_EXCHANGE)) {
            System.out.println("<<<----------------------item------------------------");
            List2MapMgr.getInstance().getOtaMapData().DebugHashMap(myHashMap);
            System.out.println("\n-----------------------item---------------------->>>");
            int nRet = ElementsMgr.CreateInstance().getList().get(ISet.OTA_EMAIL)
                    .check(myHashMap.keySet());
            System.out.println("\nnRet = " + Integer.toHexString(nRet));
        }
        System.out
                .println("\n----------------Check Exchange Elements end--------------------------->>>");
    }

    public boolean requiredInput() {
        return getOtaPreParser().requiredInput();
    }

    public void handleError(int flag, Context context) {
        getOtaPreParser().handleError(flag, context);
    }

    public void clearData() {
        List2MapMgr.getInstance().clearData();
        Dom2ListMgr.getInstance().clearData();
        DefaultHandler.getInstance().clearData();
    }

    private OtaPreParser getOtaPreParser() {
        return mOtaPreParser;
    }

    private OtaDefaultParser getOtaParser() {
        return mOtaParser;
    }

    private AppDataConfig getAppDataConfig() {
        return mAppDataConfig;
    }

    private final String TAG = "OtaManager";

    private OtaPreParser mOtaPreParser = null;
    private OtaDefaultParser mOtaParser = null;
    private AppDataConfig mAppDataConfig = null;
    private final boolean DEBUG = true;
    private final String LOG_PATH = "/data/data/com.sprd.omacp/otaLog.txt";

}
