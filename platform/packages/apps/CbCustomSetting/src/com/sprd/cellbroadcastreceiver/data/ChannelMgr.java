package com.sprd.cellbroadcastreceiver.data;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

//import com.android.cellbroadcastreceiver.CellBroadcastDatabaseHelper;
import com.sprd.cellbroadcastreceiver.data.itf.IModify;
import com.sprd.cellbroadcastreceiver.data.itf.ISoundSetting;
import com.sprd.cellbroadcastreceiver.data.itf.SoundSettingImpl;
import com.sprd.cellbroadcastreceiver.provider.ChannelTableDefine;
import com.sprd.cellbroadcastreceiver.util.Utils;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.telephony.SmsManager;
import android.util.Log;

public class ChannelMgr extends ArrayList<ChannelItemData> {

    private ChannelMgr() {

    }

    public static ChannelMgr getInstance() {
        if (mIns == null) {
            mIns = new ChannelMgr();
        }
        return mIns;
    }

    public static void releaseInstance() {
        synchronized (ChannelMgr.getInstance()) {
            mIns = null;
        }
    }

    private Context getContext() {
        return mContext;
    }

    private int getSubID() {
        return mSubId;
    }

    private ArrayList<ContentValues> getInsertValues() {
        return insertValues;
    }

    private ArrayList<Integer> getDeleteValuesIndex() {
        return deleteValuesIndex;
    }

    private HashMap getUpdateValues() {
        return updateValues;
    }

    public boolean init(Context context, int sub_id, boolean unexpected) {
//        if (getSubID() == sub_id) {
//            return true;
//        }
        mContext = context;
        mSubId = sub_id;
        return loaderFromDb(unexpected);
    }

    //modify for bug 543161 begin
    public boolean loaderFromDb(boolean unexpected) {
        if (!unexpected) {
            log("Activity is closed nomal.");
            clear();
        } else {
            log("Activity is closed unexpected.");
            return true;
        }
        //modify for bug 543161 end

        // load Data From Disk
        Cursor cursor = getContext().getContentResolver().query(Utils.mChannelUri,
                ChannelTableDefine.QUERY_COLUMNS,
                ChannelTableDefine.SUB_ID + "=" + getSubID(), null, null);
        log("--Channel--LoadFromDb--the subId is:"+getSubID()+" the count is:"+ cursor.getCount());
        addAllData(cursor);
        cursor.close();
        cursor = null;
        return true;
    }

    
    private boolean addAllData(Cursor cursor) {
        if (cursor == null || (cursor.getCount() ==0)) {
            return false;
        }
     
        
        try {
            cursor.moveToFirst();
            do {
                log("add to the MgrList");
                ChannelItemData insChannelItemData = new ChannelItemData(
                        cursor, size()/*,getSoundSetting()*/);
                getInstance().add(insChannelItemData);
            } while (cursor.moveToNext());
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public ArrayList<ChannelItemData> filterDelRecord(ArrayList<ChannelItemData> list) {
        if (list == null) {
            return null;
        } else {
            list.clear();
        }
        log("the size of Mgr is:"+ size());
        for (int i=0; i<this.size(); i++) {
            if (get(i).getFlag() == IModify.OP_DELETE) {
                continue;
            } else {
                list.add(get(i));
            }
        }

        return list;
    }

    public ArrayList<ChannelItemData> filterDelRecord() {
        ArrayList<ChannelItemData> list = new ArrayList<ChannelItemData>();
        for (int i=0; i<this.size(); i++) {
            if (get(i).getFlag() == IModify.OP_DELETE) {
                continue;
            } else {
                list.add(get(i));
            }
        }
        return list;
    }

    public boolean addNewData(int subId, Integer nCid, String szChannelName,
            boolean bEnable, boolean bSave, ISoundSetting mSoundSetting) {
        if (nCid == null || szChannelName == null || szChannelName.isEmpty()) {
            return false;
        }

        ChannelItemData ins = new ChannelItemData(size());
        //ins.setSinglePhoneSound(getSoundSetting());
        ins.setSubId(subId);
        ins.setChannelId(nCid);
        ins.setChannelName(szChannelName);
        ins.setEnabled(bEnable ? 1 : 0);
        ins.setSave(bSave ? 1 : 0);
        ins.setFlag(IModify.OP_INSERT);
        ins.setSinglePhoneSound(mSoundSetting);
        super.add(ins);
        return true;
    }

    public boolean deleteDataByIndex(int nIndex) {
        if (nIndex < 0 || nIndex >= size()) {
            return false;
        }
        if (get(nIndex).getIndexOfArray() != nIndex) {
            throw new RuntimeException(
                    "deleteDataByIndex Parameter Index Error");
        }
        get(nIndex).setFlag(IModify.OP_DELETE);
        return true;
    }

    public boolean updateDataByIndex(int nIndex, Integer nCid,
            String szChannelName, boolean bEnable, boolean bSave, ISoundSetting mSoundSetting) {
        if (nIndex < 0 || nIndex >= size()) {
            return false;
        }

        if (get(nIndex).getIndexOfArray() != nIndex) {
            throw new RuntimeException(
                    "updateDataByIndex Parameter Index Error");
        }

        if (get(nIndex).getID() != -1) {
            get(nIndex).setFlag(IModify.OP_UPDATE);
        }
        get(nIndex).setChannelId(nCid);
        get(nIndex).setChannelName(szChannelName);
        get(nIndex).setEnabled(bEnable ? 1 : 0);
        get(nIndex).setSave(bSave ? 1 : 0);
        get(nIndex).setSinglePhoneSound(mSoundSetting);
        return true;
    }

    private boolean sendATCommand(){
        int countInsert = getInsertValues().size();
        int countDelete = getDeleteValuesIndex().size();
        int countUpdate = getUpdateValues().size();
        for (int i = 0; i < countInsert; i++) {
            if((int)getInsertValues().get(i).get(ChannelTableDefine.ENABLE) == 1){
                log("Send ATCommand. Enabled new channel.");
                Utils.sendATCmd(mContext,
                        (int)getInsertValues().get(i).get(ChannelTableDefine.CHANNEL_ID),
                        1,
                        (int)getInsertValues().get(i).get(ChannelTableDefine.SUB_ID),
                        Utils.SET_CHANNEL);
            }
        }

        Iterator iterator = getUpdateValues().entrySet().iterator();
        while (iterator.hasNext()) {
            log("Send ATCommand. Update channel state.");
            Map.Entry<Integer, ContentValues> entry = (Map.Entry<Integer, ContentValues>) iterator.next();
            int _id = entry.getKey();
            ContentValues cv = entry.getValue();

            Utils.sendATCmd(mContext,
                    (int)cv.get(ChannelTableDefine.CHANNEL_ID),
                    (int)cv.get(ChannelTableDefine.ENABLE),
                    (int)cv.get(ChannelTableDefine.SUB_ID),
                    Utils.SET_CHANNEL);

            log("Send ATCommand.the channel_id is:"+cv.get(ChannelTableDefine.CHANNEL_ID));
        }

        for (int i = 0; i < countDelete; i++) {
            if (this.get((int)getDeleteValuesIndex().get(i)).getID() != -1) {
                log("Send ATCommand. Disabled channel.");
                Utils.sendATCmd(mContext,
                        this.get((int)getDeleteValuesIndex().get(i)).getChannelId(),
                        0,
                        this.get((int)getDeleteValuesIndex().get(i)).getSubId(),
                        Utils.SET_CHANNEL);
            }
        }
        return true;
    }



    public boolean SaveDataToDB() {
        synchronized(ChannelMgr.getInstance()){
            int size = this.size();
            for (int i = 0; i < size; i++) {
                ContentValues value = new ContentValues(10);
                value.put(ChannelTableDefine.CHANNEL_ID, get(i).getChannelId());
                value.put(ChannelTableDefine.CHANNEL_NAME, get(i).getChannelName());
                value.put(ChannelTableDefine.SUB_ID, get(i).getSubId());
                value.put(ChannelTableDefine.ENABLE, get(i).getEnabled());
                value.put(ChannelTableDefine.SAVE, get(i).getSave());
                value.put(ChannelTableDefine.MCC, get(i).getMcc());
                value.put(ChannelTableDefine.MNC, get(i).getMnc());

                //the feilds bellow is used when the ringtone is set by channel
                value.put(ChannelTableDefine.VIBRATE, get(i).getSoundSetting().isVibrate()?1:0);
                value.put(ChannelTableDefine.SOURND_URI, get(i).getSoundSetting().getSoundURI());                
                value.put(ChannelTableDefine.NOTIFICATION, get(i).getSoundSetting().isNotification()?1:0);

                log("the flag is:"+ get(i).getFlag());
                switch (get(i).getFlag()) {
                case IModify.OP_INSERT:
                    getInsertValues().add(value);
                    break;
                case IModify.OP_DELETE:
                    //if the _id is -1 means that this record is not in the DB, so stored the index of those need
                    //saved in the MgrList
                    if(get(i).getID() != -1){
                        getDeleteValuesIndex().add(get(i).getIndexOfArray());
                    }
                    break;
                case IModify.OP_UPDATE:
                    getUpdateValues().put(get(i).getID(), value);
                    break;

                default:
                    break;
                }
            }
            log("the size of InsertValues, UpdateValues DeleteValues and MgrLis is:"+ getInsertValues().size()+
                    " "+ getUpdateValues().size()+" "+ getDeleteValuesIndex().size()+ " "+this.size());
            // bluk insert
            if (getInsertValues().size() != 0) {
                ContentValues[] values = (ContentValues[])getInsertValues().toArray(new ContentValues[getInsertValues().size()]);
                getContext().getContentResolver().bulkInsert(Utils.mChannelUri, values);
            }
            // bluk update
            if (getUpdateValues().size() != 0) {
                bulkUpdate(Utils.mChannelUri, getUpdateValues());
            }
            // bluk Delete
            if (getDeleteValuesIndex().size() != 0) {
                bulkDelete(Utils.mChannelUri, getDeleteValuesIndex());
            }
            //send AT command
            sendATCommand();
        }
        releaseInstance();
        return true;
    }

    private int bulkUpdate(Uri uri, HashMap map) {
        int numOfValues =map.size();
        //CellBroadcastDatabaseHelper dbHelper=new CellBroadcastDatabaseHelper(getContext());
        //dbHelper.getWritableDatabase().beginTransaction();
        try {
            Iterator iterator = map.entrySet().iterator();
            while (iterator.hasNext()) {
                Map.Entry<Integer, ContentValues> entry = (Map.Entry<Integer, ContentValues>) iterator.next();
                
                int _id = entry.getKey();
                ContentValues cv = entry.getValue();
                int count = 0;
                count = getContext().getContentResolver().update(Utils.mChannelBulkUri, cv,
                        ChannelTableDefine._ID + "=" + _id, null);
                log("update single record.the _id is:"+_id);
            }
            //dbHelper.getWritableDatabase().setTransactionSuccessful();
        } finally {
            //dbHelper.getWritableDatabase().endTransaction();
        }
        //*/
        //getContext().getContentResolver().notifyChange(uri, null);
        return numOfValues;
    }

    public int bulkDelete(Uri uri, ArrayList<Integer> values){
        int count = values.size();
        ///*
        //CellBroadcastDatabaseHelper dbHelper=new CellBroadcastDatabaseHelper(getContext());
        //dbHelper.getWritableDatabase().beginTransaction();
        try {
            for (int i = 0; i < count; i++) {
                getContext().getContentResolver().delete(Utils.mChannelBulkUri,
                        ChannelTableDefine._ID + "=" + ChannelMgr.getInstance().get(values.get(i)).getID(),
                        null);
            }
            //dbHelper.getWritableDatabase().setTransactionSuccessful();
        } finally {
           // dbHelper.getWritableDatabase().endTransaction();
        }
       // */
       // getContext().getContentResolver().notifyChange(uri, null);
        return count;
    }

    private void log(String string){
        Log.d(TAG, string);
    }

    public boolean addAll(Collection<? extends ChannelItemData> data) {
        throw new RuntimeException(
                "System DO NOT Implements addAll(Collection <? extends ChannelItemData>)  ");
    }

    public boolean addAll(int nIndex, Collection<? extends ChannelItemData> data) {
        throw new RuntimeException(
                "System DO NOT Implements addAll(int nIndex, Collection <? extends ChannelItemData>)  ");
    }

    public ISoundSetting getSoundSetting() {
        return mobjSoundSetting;
    }

    private int mSubId;
    private Context mContext;
    private String TAG = "ChannelMgr";
    
    private ArrayList<ContentValues> insertValues = new ArrayList<ContentValues>();
    private HashMap updateValues = new HashMap();
    private ArrayList<Integer> deleteValuesIndex = new ArrayList<Integer>();

    private static ChannelMgr mIns = null;

    private static final long serialVersionUID = 1L;

    public static String INDEX_OF_MGR = "indexOfChannelMgr";
    
    
    private ISoundSetting  mobjSoundSetting = null; 
}
