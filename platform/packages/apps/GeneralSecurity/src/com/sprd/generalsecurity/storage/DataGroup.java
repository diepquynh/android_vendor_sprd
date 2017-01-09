package com.sprd.generalsecurity.storage;

import java.util.ArrayList;

import android.util.ArrayMap;
import java.util.Map;
import android.util.Log;
import java.util.Iterator;
import java.io.File;

public class DataGroup {
    private final static String TAG = "DataGroup";
    public ArrayMap<String, Long> mInCacheMap;
    public ArrayMap<String, Long> mExCacheMap;
    public ArrayMap<String, Long> mInRubbishMap;
    public ArrayMap<String, Long> mExRubbishMap;
    public ArrayMap<String, Long> mInApkMap;
    public ArrayMap<String, Long> mExApkMap;
    public ArrayMap<String, Long> mInTmpMap;
    public ArrayMap<String, Long> mExTmpMap;
    public ArrayMap<String, Long> mLargeMap;
    public ArrayMap<String, Long> mExLargeMap;

    public ArrayList<String> mTempKey = new ArrayList<String>();
    public ArrayList<String> mTempValues = new ArrayList<String>();
    private static DataGroup instance;

    public static final int RUBBISH_FILE_BIT = 1;
    public static final int TMP_FILE_BIT = 2;
    public static final int APK_FILE_BIT = 4;
    public static final int LARGE_FILE_BIT = 8;

    public int mFileUpdateBits;

    public static DataGroup getInstance() {
        if (instance == null) {
            instance = new DataGroup();
        }
        return instance;
    }

    private DataGroup() {
        mInCacheMap = new ArrayMap<String, Long>();
        mExCacheMap = new ArrayMap<String, Long>();
        mInRubbishMap = new ArrayMap<String, Long>();
        mExRubbishMap = new ArrayMap<String, Long>();
        mInApkMap = new ArrayMap<String, Long>();
        mExApkMap = new ArrayMap<String, Long>();
        mInTmpMap = new ArrayMap<String, Long>();
        mExTmpMap = new ArrayMap<String, Long>();
        mLargeMap = new ArrayMap<String, Long>();
        mExLargeMap = new ArrayMap<String, Long>();
    }


    public ArrayMap<String, Long> getNeedMap(boolean isExternal, int type) {
        if (!isExternal) {
            if (type == StorageManagement.CACHE_ITEM) {
                return mInCacheMap;
            } else if (type == StorageManagement.RUBBISH_ITEM) {
                return mInRubbishMap;
            } else if (type == StorageManagement.APK_ITEM) {
                return mInApkMap;
            } else if (type == StorageManagement.TMP_ITEM) {
                return  mInTmpMap;
            } else if (type == StorageManagement.LARGE_FILE_ITEM) {
                return mLargeMap;
            }
        } else {
            if (type == StorageManagement.CACHE_ITEM) {
                return mExCacheMap;
            } else if (type == StorageManagement.RUBBISH_ITEM) {
                return mExRubbishMap;
            } else if (type == StorageManagement.APK_ITEM) {
                return mExApkMap;
            } else if (type == StorageManagement.TMP_ITEM) {
                return mExTmpMap;
            } else if (type == StorageManagement.LARGE_FILE_ITEM) {
                return mExLargeMap;
            }
        }

        return null;
    }

    public void destroy() {
        mInCacheMap.clear();
        mExCacheMap.clear();
        mInRubbishMap.clear();
        mExRubbishMap.clear();
        mInApkMap.clear();
        mExApkMap.clear();
        mInTmpMap.clear();
        mExTmpMap.clear();
        mLargeMap.clear();
        mExLargeMap.clear();
        instance = null;
    }

    public long getTotalSize(boolean isExternal, int type) {
        if (!isExternal) {
            if (type == StorageManagement.CACHE_ITEM) {
                return getCategorySize(mInCacheMap);
            } else if (type == StorageManagement.RUBBISH_ITEM) {
                return getCategorySize(mInRubbishMap);
            } else if (type == StorageManagement.APK_ITEM) {
                return getCategorySize(mInApkMap);
            } else if (type == StorageManagement.TMP_ITEM) {
                return  getCategorySize(mInTmpMap);
            } else if (type == StorageManagement.LARGE_FILE_ITEM) {
                return  getCategorySize(mLargeMap);
            }
        } else {
            if (type == StorageManagement.CACHE_ITEM) {
                return getCategorySize(mExCacheMap);
            } else if (type == StorageManagement.RUBBISH_ITEM) {
                return getCategorySize(mExRubbishMap);
            } else if (type == StorageManagement.APK_ITEM) {
                return getCategorySize(mExApkMap);
            } else if (type == StorageManagement.TMP_ITEM) {
                return getCategorySize(mExTmpMap);
            } else if (type == StorageManagement.LARGE_FILE_ITEM) {
                return  getCategorySize(mExLargeMap);
            }
        }
        return 0;
    }

    long mAPKCategorySize;
    long mExAPKCategorySize;
    long mTempCategorySize;
    long mExTempCategorySize;
    long mRubbishCategorySize;
    long mExRubbishCategorySize;
    long mLargeFileCategorySize;
    long mExLargeFileCategorySize;
    long mUniqueLargeFileSize;
    long mExUniqueLargeFileSize; //large file size that not contained in APK, tmp category.

    public void updateSize(int updateType, boolean isExternal) {
        if (!isExternal) {
            if ((updateType & RUBBISH_FILE_BIT) > 0) {
                mRubbishCategorySize = getTotalSize(isExternal, StorageManagement.RUBBISH_ITEM);
            }
            if ((updateType & TMP_FILE_BIT) > 0) {
                mTempCategorySize = getTotalSize(isExternal, StorageManagement.TMP_ITEM);
            }
            if ((updateType & APK_FILE_BIT) > 0) {
                mAPKCategorySize = getTotalSize(isExternal, StorageManagement.APK_ITEM);
            }
            if ((updateType & LARGE_FILE_BIT) > 0) {
                mLargeFileCategorySize = getTotalSize(isExternal, StorageManagement.LARGE_FILE_ITEM);
            }
        } else {
            if ((updateType & RUBBISH_FILE_BIT) > 0) {
                mExRubbishCategorySize = getTotalSize(isExternal, StorageManagement.RUBBISH_ITEM);
            }
            if ((updateType & TMP_FILE_BIT) > 0) {
                mExTempCategorySize = getTotalSize(isExternal, StorageManagement.TMP_ITEM);
            }
            if ((updateType & APK_FILE_BIT) > 0) {
                mExAPKCategorySize = getTotalSize(isExternal, StorageManagement.APK_ITEM);
            }
            if ((updateType & LARGE_FILE_BIT) > 0) {
                mExLargeFileCategorySize = getTotalSize(isExternal, StorageManagement.LARGE_FILE_ITEM);
            }
        }
    }

    private long getCategorySize(ArrayMap<String,Long> map) {
        long size = 0;

        if (map == mExLargeMap) {
            mExUniqueLargeFileSize = 0;
        } else if (map == mLargeMap) {
            mUniqueLargeFileSize = 0;
        }

        for (Iterator<Map.Entry<String, Long>> it =map.entrySet().iterator(); it.hasNext(); ) {
            Map.Entry<String, Long> entry = it.next();
            String key = entry.getKey();

            File f = new File(key);
            if (f.exists()) {
                size += f.length();
                if (map == mExLargeMap) {
                    if (StorageManagement.isLargeFileUnique(f)) {
                        mExUniqueLargeFileSize += f.length();
                    }
                } else if (map == mLargeMap) {
                    if (StorageManagement.isLargeFileUnique(f)) {
                        mUniqueLargeFileSize += f.length();
                    }
                }
            } else {
                it.remove();
            }
        }

        return size;
    }
}
