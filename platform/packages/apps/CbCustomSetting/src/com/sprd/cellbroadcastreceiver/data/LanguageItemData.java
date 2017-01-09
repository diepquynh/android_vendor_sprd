package com.sprd.cellbroadcastreceiver.data;

import android.database.Cursor;
import android.util.Log;

import com.sprd.cellbroadcastreceiver.data.itf.ModifyImp;
import com.sprd.cellbroadcastreceiver.provider.CreateLangViewDefine;

public class LanguageItemData extends ModifyImp implements CreateLangViewDefine {

    public LanguageItemData(Cursor cursor, int nIndexOfArray) {
        mCursor = cursor;
        mIndexOfArray = nIndexOfArray;
        init(mCursor);
    }

    public LanguageItemData(int nIndexOfArray) {
        mIndexOfArray = nIndexOfArray;
    }

    private Boolean init(Cursor cursor) {
        if (cursor == null || cursor.getCount() == 0) {
            Log.d(TAG, "--init LanguageList--the cursor is empty.");
            return false;
        }
        Log.d(TAG, "--init--the count is:" + cursor.getCount());
        try {
            setId(cursor.getInt(cursor.getColumnIndex(INDEX_ID)));
            setEnabled(cursor.getInt(cursor.getColumnIndex(INDEX_ENABLED)));
            setShow(cursor.getInt(cursor.getColumnIndex(INDEX_SHOW)));
            setLanguageBit(cursor.getInt(cursor.getColumnIndex(INDEX_LANGUAGE_ID)));
            setDescription(cursor.getString(cursor.getColumnIndex(INDEX_DESCRIPTION)));
            setMncMccId(cursor.getInt(cursor.getColumnIndex(INDEX_MNCMCCID)));
            return true;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    public void setEnabled(int i) {
        mEnabled = i;
    }

    public void setShow(int i) {
        mShow = i;
    }

    public void setLanguageBit(int bit) {
        mLanguageBit = bit;
    }

    public void setDescription(String s) {
        mDescription = s;
    }

    public void setId(int id) {
        _id = id;
    }

    public void setMncMccId(int mncmccId){
        mncmcc_id = mncmccId;
    }

    public int getMncMccId() {
        return mncmcc_id;
    }

    public int getId() {
        return _id;
    }

    public int getEnabled() {
        return mEnabled;
    }

    public int getShow() {
        return mShow;
    }

    public int getLanguageBit() {
        return mLanguageBit;
    }

    private Cursor getCursor() {
        return mCursor;
    }

    public String getDescription() {
        return mDescription;
    }

    public final int getIndexOfArray() {
        return mIndexOfArray;
    }

    private final String TAG = "LangItemData";

    private Cursor mCursor;
    private int _id;
    private int mShow;
    private int mEnabled;
    private int mLanguageBit;
    private int mncmcc_id;
    private final int mIndexOfArray;
    private String mDescription;
}
