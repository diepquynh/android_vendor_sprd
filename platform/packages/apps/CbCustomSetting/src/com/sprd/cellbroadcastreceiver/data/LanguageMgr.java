package com.sprd.cellbroadcastreceiver.data;

import java.util.ArrayList;
import java.util.List;

//import com.android.cellbroadcastreceiver.CellBroadcastDatabaseHelper;
import com.sprd.cellbroadcastreceiver.provider.CreateLangViewDefine;
import com.sprd.cellbroadcastreceiver.provider.LangMapTableDefine;
import com.sprd.cellbroadcastreceiver.provider.MncMccTableDefine;
import com.sprd.cellbroadcastreceiver.util.LanguageIds;
import com.sprd.cellbroadcastreceiver.util.Utils;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteFullException;
import android.net.Uri;
import android.util.Log;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionInfo;

public class LanguageMgr extends ArrayList<LanguageItemData> {

    private static final long serialVersionUID = 1L;

    private final String TAG = "LanguageMgr";
    private boolean mNoRecordBySub = false;
    private int mSubId;
    private Context mContext;
    private SubscriptionInfo mSubscriptionInfo;
    private static LanguageMgr mIns;

    private LanguageMgr() {

    }

    private Context getContext() {
        return mContext;
    }

    private int getSubId() {
        return mSubId;
    }

    private SubscriptionInfo getSubscriptionInfo() {
        return mSubscriptionInfo;
    }

    private int getMcc() {
        return 460;
        //return getSubscriptionInfo().getMcc();
    }

    private int getMnc() {
        return 1;
        //return getSubscriptionInfo().getMnc();
    }

    public static LanguageMgr getInstance() {
        if (mIns == null) {
            mIns = new LanguageMgr();
        }
        return mIns;
    }

    public static void releaseInstance() {
        synchronized (LanguageMgr.getInstance()) {
            mIns = null;
        }
    }

    public boolean init(Context context, int sub_id, boolean unexpected) {

        mContext = context;
        mSubId = sub_id;
        mSubscriptionInfo = SubscriptionManager.from(context).getActiveSubscriptionInfo(sub_id);
        return loadFromDb(sub_id, unexpected);
    }

    //modify for bug 543161 begin
    private boolean loadFromDb(int sub_id, boolean unexpected) {
        //modify for bug 542754
        // remove ALL data
        if (!unexpected) {
            log("Activity is closed nomal.");
            clear();
        } else {
            log("Activity is closed unexpected.");
            return true;
        }
        //modify for bug 543161 end

        Cursor cursor = getContext().getContentResolver().query(
                Utils.mViewLangUri,
                CreateLangViewDefine.QUERY_COLUMNS,
                LangMapTableDefine.SUBID + "=" + sub_id + " and "
                        + MncMccTableDefine.MNC + "=" + getMnc() + " and "
                        + MncMccTableDefine.MCC + "=" + getMcc(),
                null,
                LangMapTableDefine.LANG_ID + " ASC");

        if (cursor == null || cursor.getCount() == 0) {
            //modify for bug 549170 begin
            if (cursor != null) {
                cursor.close();
            }
            //modify for bug 549170 end
            log("used default values.");
            mNoRecordBySub = true;
            cursor = getContext().getContentResolver().query(
                    Utils.mViewLangUri,
                    CreateLangViewDefine.QUERY_COLUMNS,
                    MncMccTableDefine.MNC + "=" + getMnc() + " and "
                            + MncMccTableDefine.MCC + "=" + getMcc() + " and "
                            + LangMapTableDefine.SUBID + "=" + "-1",
                    null,
                    LangMapTableDefine.LANG_ID + " ASC");
        } else {//modify for bug 554672
            mNoRecordBySub = false;
        }
        log("--Lang--LoadFromDb--the subId is:" + sub_id
                + " the count is:" + cursor.getCount());

        addDataToArray(cursor);
        cursor.close();
        cursor = null;
        return true;
    }

    private boolean addDataToArray(Cursor cursor) {
        if (cursor == null || cursor.getCount() == 0) {
            return false;
        }

        try {
            cursor.moveToFirst();
            do {
                LanguageItemData itemData = new LanguageItemData(cursor,
                        this.size());
                getInstance().add(itemData);
            } while (cursor.moveToNext());
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public ArrayList<LanguageItemData> filterToLangAdapter(
            ArrayList<LanguageItemData> list) {
        if (list == null) {
            return null;
        } else {
            list.clear();
        }
        int size = this.size();
        log("the size of LanguageMgr is:"+size);
        log("the detail of LanguageMgr is:"+this.toString());
        for (int i = 0; i < size; i++) {
            if (get(i).getEnabled() == 0) {
                continue;
            } else {
                list.add(get(i));
                log("i is:"+i+" and add language:"+ get(i).getDescription()+"\n");
            }
        }
        return list;
    }

    public ArrayList<Integer> filterToLangChooseAdapter(ArrayList<Integer> list) {
        if (list == null) {
            return null;
        } else {
            list.clear();
        }
        int size = this.size();
        log("the size of LanguageMgr is:"+size);
        log("the detail of LanguageMgr is:"+this.toString());
        for (int i = 0; i < size; i++) {
            if (get(i).getShow() == 0) {
                continue;
            } else {
                list.add(get(i).getIndexOfArray());
            }
        }
        return list;
    }

    public boolean SaveDataToDB() {
        bulkUpdate(Utils.mLangUri, getSubId());
        //every language need to base a value and set ATCommand
        Utils.sendLangAT(mContext, getLanguageId(), getSubId(), true);
        return true;
    }

    //filter those bits which need to open
    private ArrayList<Integer> getLanguageId(){
        
        int codeScheme = 0;
        int size = getInstance().size()<LanguageIds.MAX_LANG ? getInstance().size():LanguageIds.MAX_LANG;
        ArrayList<Integer> languageId = new ArrayList<Integer>();
        int singleId = 0xffff;
        for (int i = 0; i < size; i++) {
            codeScheme |= get(i).getEnabled() << get(i).getLanguageBit()-1;
            if (get(i).getEnabled()==1) {
                singleId = LanguageIds.LANGUAGE_ID[get(i).getLanguageBit()-1];
                log("getLang langId by:" 
                        + getContext().getString(LanguageIds.LangMap[get(i).getLanguageBit()-1])
                        +" and singleId is:"+singleId);
                languageId.add(singleId);
            }
        }
        log("--getLanguageId--codeScheme is:"+codeScheme);
        return languageId;
    }

    private boolean bulkUpdate(Uri uri, int sub_id) {
        ///*
        //CellBroadcastDatabaseHelper dbHelper = new CellBroadcastDatabaseHelper(
         //       getContext());
        //dbHelper.getWritableDatabase().beginTransaction();
        try {
            int size = this.size();
            for (int i = 0; i < size; i++) {
                ContentValues cv = new ContentValues();
                cv.put(LangMapTableDefine.ENABLE, get(i).getEnabled());
                if (mNoRecordBySub) {
                    log("--insertToDB--");
                    cv.put(LangMapTableDefine.LANG_ID, get(i).getLanguageBit());
                    cv.put(LangMapTableDefine.MNC_MCC_ID, get(i).getMncMccId());
                    cv.put(LangMapTableDefine.SUBID, sub_id);
                    cv.put(LangMapTableDefine.SHOW, get(i).getShow());
                    getContext().getContentResolver().insert(Utils.mLangBulkUri, cv);
                } else {
                    getContext().getContentResolver().update(
                            Utils.mLangBulkUri, cv,
                            LangMapTableDefine._ID + "=" + get(i).getId(), null);
                }

            }
           // dbHelper.getWritableDatabase().setTransactionSuccessful();
        } catch(SQLiteFullException e){
            Log.e(TAG,"bulkUpdate SQLiteFullException: "+ e,new Throwable());
        }
        finally {
            //dbHelper.getWritableDatabase().endTransaction();
        }
        //getContext().getContentResolver().notifyChange(uri, null);
       // */
        return true;
    }

    private void log(String string){
        Log.d(TAG, string);
    }

    @Override
    public String toString(){
        String s = null;
        for (int i = 0; i < LanguageMgr.getInstance().size(); i++) {
            s += "i is:"+i+" ,Description is:"+get(i).getDescription()+" and showed:"+get(i).getShow()+"\n";
        }
        return s;
    }
}
