
package com.sprd.contacts.common.util;

import java.util.HashMap;

import android.database.Cursor;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

public class MultiContactDataCacheUtils implements Parcelable {

    private static final String TAG = MultiContactDataCacheUtils.class.getSimpleName();

    public interface CacheMode {
        public static final int SINGLE = 0;
        public static final int MULTI = 1;
    }

    private HashMap<Long, HashMap<String, String>> mMultiCacheData = new HashMap<Long, HashMap<String, String>>();
    private HashMap<Long, String> mCacheData = new HashMap<Long, String>();

    private int mode;
    private String mMainColumnIndex = new String();
    private String mMinorColumnIndex = new String();

    public MultiContactDataCacheUtils() {
        this.mode = CacheMode.SINGLE;
    }

    public MultiContactDataCacheUtils(int mode) {
        this.mode = mode;
    }

    public MultiContactDataCacheUtils(int mode, String mainIndex, String minorIndex) {
        this.mode = mode;
        this.mMainColumnIndex = mainIndex;
        this.mMinorColumnIndex = minorIndex;
    }

    public static final Parcelable.Creator<MultiContactDataCacheUtils> CREATOR = new Parcelable.Creator<MultiContactDataCacheUtils>() {
        public MultiContactDataCacheUtils createFromParcel(Parcel source) {
            return new MultiContactDataCacheUtils(source);
        }

        public MultiContactDataCacheUtils[] newArray(int size) {
            return new MultiContactDataCacheUtils[size];
        }
    };

    private MultiContactDataCacheUtils(Parcel source) {
        mMainColumnIndex = source.readString();
        mMinorColumnIndex = source.readString();
        mode = source.readInt();
        mMultiCacheData = source.readHashMap(HashMap.class.getClassLoader());
        mCacheData = source.readHashMap(HashMap.class.getClassLoader());
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(mMainColumnIndex);
        dest.writeString(mMinorColumnIndex);
        dest.writeInt(mode);
        dest.writeMap(mMultiCacheData);
        dest.writeMap(mCacheData);
    }

    public void setColumnIndex(String mainIndex) {
        setColumnIndex(mainIndex, null);
    }

    public void setColumnIndex(String mainIndex, String minorIndex) {
        this.mMainColumnIndex = mainIndex;
        this.mMinorColumnIndex = minorIndex;
    }

    public void addCacheItem(Cursor cursor, Long contactId) {
        if (cursor == null || cursor.getCount() == 0 || mMainColumnIndex == null || contactId == -1) {
            Log.d(TAG, "cursor is null or count is 0");
            return;
        }
        if (cursor.getColumnIndex(mMainColumnIndex) == -1) {
            return;
        }

        switch (mode) {
            case CacheMode.MULTI:
                HashMap<String, String> result = new HashMap<String, String>();
                result.put(cursor.getString(cursor.getColumnIndex(mMainColumnIndex)),
                        cursor.getString(cursor.getColumnIndex(mMinorColumnIndex)));
                mMultiCacheData.put(contactId, result);
                break;

            case CacheMode.SINGLE:
                mCacheData
                        .put(contactId, cursor.getString(cursor.getColumnIndex(mMainColumnIndex)));
                break;

            default:
                Log.d(TAG, "this mode " + mode + " is not supported");
                break;
        }

    }

    public void addCacheItem(Cursor cursor) {
        if (cursor == null || cursor.getCount() == 0 || mMainColumnIndex == null) {
            Log.d(TAG, "cursor is null or count is 0");
            return;
        }
        Long contactId = cursor.getLong(0);
        if (contactId == -1) {
            return;
        }
        addCacheItem(cursor, contactId);
    }

    public void clear() {
        mCacheData.clear();
        mMultiCacheData.clear();
    }

    public void remove(Long contactId) {

        switch (mode) {
            case CacheMode.SINGLE:
                mCacheData.remove(contactId);
                break;

            case CacheMode.MULTI:
                mMultiCacheData.remove(contactId);

            default:
                break;
        }
    }

    public HashMap<Long, String> getCache() {
        if (CacheMode.SINGLE == mode) {
            return mCacheData;
        }
        Log.d(TAG, "getCache but mode is not correct, current mode is " + mode);
        return mCacheData;
    }

    public HashMap<Long, HashMap<String, String>> getMultiCache() {
        if (CacheMode.MULTI == mode) {
            return mMultiCacheData;
        }
        Log.d(TAG, "getMultiCache but mode is not correct, current mode is " + mode);
        return mMultiCacheData;
    }

    public void setModel(int mode, String mainIndex, String minorIndex) {
        this.mode = mode;
        this.mMainColumnIndex = mainIndex;
        this.mMinorColumnIndex = minorIndex;
    }

    public boolean containsCache(Long contactId) {
        if (CacheMode.SINGLE == mode) {
            return mCacheData.containsKey(contactId);
        } else if (CacheMode.MULTI == mode) {
            return mMultiCacheData.containsKey(contactId);
        } else {
            Log.d(TAG, "contains cache and mode is not supported");
            return false;
        }
    }
}
