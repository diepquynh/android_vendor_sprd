package com.sprd.fileexplorer.util;

// Porting from xingxing
// TODO: MERGE IT INTO FILEUTIL

import java.io.File;
import java.text.Collator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;

import android.provider.MediaStore.MediaColumns;
import android.util.Log;

public class FileSort {
    public static final String TAG = "FileSort";
    public static final String FILE_SORT_KEY = "sort_key";
    public static final int START_SORT = 0;
    public static final int SORT_BY_NAME = 1;
    public static final int SORT_BY_TYPE = 2;
    public static final int SORT_BY_TIME_DESC = 3;
    public static final int SORT_BY_TIME_ASC = 4;
    public static final int SORT_BY_SIZE_DESC = 5;
    public static final int SORT_BY_SIZE_ASC = 6;
    public static final int SORT_BY_NAME_DESC = 7;
    public static final int SORT_BY_TYPE_DESC = 8;

    private static final Object sLock = new Object();
    
    private volatile int mSortType;
    private boolean mIsSorting = false;
    private boolean mEnableSorting = true;

    private Comparator<File> comparator;

    private static FileSort mFileListSort = null;

    public FileSort(int sortType) {
        setSortType(sortType);
    }

    public void stopSort() {
        synchronized (this) {
            mEnableSorting = false;
        }
    }

    public boolean isSorting() {
        return mIsSorting;
    }

    /**
     * Start sort the file List, you <b>*MUST*</b> synchronized fileList before
     * start sort, otherwise may cause ConcurrentModificationException
     * 
     * @param fileList
     *            the file list you want to sort
     */
    public void sort(List<File> fileList) {
        Log.d(TAG, "FileSort:sort");
        synchronized (this) {
           mEnableSorting = true;
        }
        if (null == fileList || fileList.isEmpty()) {
            return;
        }
        ArrayList<File> filesList = new ArrayList<File>();
        ArrayList<File> dirsList = new ArrayList<File>();
        for (File f : fileList) {
            if (f.isDirectory()) {
                dirsList.add(f);
            } else {
                filesList.add(f);
            }
            
        }
        
        mIsSorting = true;
        Log.d(TAG, "begin to sort");
        try {
            Collections.sort(dirsList, comparator);
            Collections.sort(filesList, comparator);
            fileList.clear();
            for (File f : dirsList) {
                fileList.add(f);
            }
            for (File f : filesList) {
                fileList.add(f);
            }
            Log.d(TAG, "sorting done");
        } catch (SortInterruptedException e) {
            Log.d(TAG, "The sort method is interrupted unexpectedly", e);
        }
        mIsSorting = false;
    }
    
    public void sortDefault(List<File> fileList) {
        Log.d(TAG, "FileSort:sort");
        synchronized (this) {
           mEnableSorting = true;
        }
        if (null == fileList) {
            return;
        }
        mIsSorting = true;
        try {
            System.setProperty("java.util.Arrays.useLegacyMergeSort", "true");
            Collections.sort(fileList, comparator);
        } catch (SortInterruptedException e) {
            Log.d(TAG, "The sort method is interrupted unexpectedly", e);
        }
        mIsSorting = false;
    }

    private FileSort() {
        setSortType(SORT_BY_NAME);
        System.setProperty("java.util.Arrays.useLegacyMergeSort", "true");
    }

    public static FileSort getFileListSort() {
        synchronized (sLock) {
            if (mFileListSort == null) {
                mFileListSort = new FileSort();
            }
            return mFileListSort;
        }
    }

    public static FileSort newInstance() {
        return new FileSort();
    }

    public static void fileListSort(List<File> fileList) {
        Collections.sort(fileList, getFileListSort().comparator);
    }

    public static String getOrderStr(int order) {
        String ret = null;
        switch (order) {
        case SORT_BY_NAME:
            ret = MediaColumns.TITLE + " collate nocase, " + MediaColumns.DATA + " collate nocase";
            break;
        case SORT_BY_NAME_DESC:
            ret = MediaColumns.TITLE + " collate nocase desc, " + MediaColumns.DATA+ " collate nocase desc";
            break;
        case SORT_BY_TYPE:
            ret = MediaColumns.MIME_TYPE + ", " + MediaColumns.TITLE + ", " + MediaColumns.DATA;
            break;
        case SORT_BY_TYPE_DESC:
            ret = MediaColumns.MIME_TYPE + " desc, " + MediaColumns.TITLE + " desc, " +  MediaColumns.DATA+ " desc";
            break;
        case SORT_BY_TIME_DESC:
            ret = MediaColumns.DATE_MODIFIED+ " desc, " + MediaColumns.TITLE + " desc, " +  MediaColumns.DATA+ " desc";
            break;
        case SORT_BY_TIME_ASC:
            ret = MediaColumns.DATE_MODIFIED + ", " + MediaColumns.TITLE + ", " + MediaColumns.DATA;
            break;
        case SORT_BY_SIZE_DESC:
            ret = MediaColumns.SIZE + " desc, " + MediaColumns.TITLE + " desc, " +  MediaColumns.DATA+ " desc";
            break;
        case SORT_BY_SIZE_ASC:
            ret = MediaColumns.SIZE + ", " + MediaColumns.TITLE + ", " + MediaColumns.DATA;
            break;
        }
        return ret;
    }

    public void setSortType(int sortType) {
        if (sortType > SORT_BY_TYPE_DESC || sortType < SORT_BY_NAME) {
            sortType = SORT_BY_NAME;
            Log.e("FileSort", "the argument: sortType = " + sortType + " out of border");
        }
        mSortType = sortType;
        switch (mSortType) {
            case SORT_BY_NAME:
                comparator = new Comparator<File>() {

                    @Override
                    public int compare(File lhs, File rhs) {
                        if (!mEnableSorting) {
                            throw new SortInterruptedException("SORT_BY_NAME");
                        }
                        Collator myCollator = Collator.getInstance(java.util.Locale.CHINA);
                        int ret = myCollator.compare(lhs.getName().toLowerCase(), rhs.getName().toLowerCase());
                        return ret;
                    }

                };
                break;
            case SORT_BY_NAME_DESC:
                comparator = new Comparator<File>() {

                    @Override
                    public int compare(File lhs, File rhs) {
                        if (!mEnableSorting) {
                            throw new SortInterruptedException("SORT_BY_NAME_DESC");
                        }
                        Collator myCollator = Collator.getInstance(java.util.Locale.CHINA);
                        int ret = myCollator.compare(lhs.getName().toLowerCase(), rhs.getName().toLowerCase());
                        return -ret;
                    }

                };
                break;
            case SORT_BY_TYPE:
                comparator = new Comparator<File>() {

                    @Override
                    public int compare(File lhs, File rhs) {
                        if (!mEnableSorting) {
                            throw new SortInterruptedException("SORT_BY_TYPE");
                        }

                        String lhsName = lhs.getName();
                        String rhsName = rhs.getName();
                        String lhsType = getFileType(lhsName);
                        String rhsType = getFileType(rhsName);
                        int ret = lhsType.compareTo(rhsType);
                        if (ret == 0) {
                            ret = lhsName.compareTo(rhsName);
                        }
                        if (ret == 0) {
                            ret = lhs.getPath().compareTo(rhs.getPath());
                        }
                        Log.i(TAG, "compare " + lhs.getPath() + " AND " + rhs.getPath() + ", retutn " + ret);
                        return ret;
                    }
                };
                break;
            case SORT_BY_TYPE_DESC:
                comparator = new Comparator<File>() {

                    @Override
                    public int compare(File lhs, File rhs) {
                        if (!mEnableSorting) {
                            throw new SortInterruptedException("SORT_BY_TYPE_DESC");
                        }
                        String lhsName = lhs.getName();
                        String rhsName = rhs.getName();
                        String lhsType = getFileType(lhsName);
                        String rhsType = getFileType(rhsName);
                        int ret = lhsType.compareTo(rhsType);
                        if (ret == 0) {
                            ret = lhsName.compareTo(rhsName);
                        }
                        return -ret;
                    }

                };
            break;
        case SORT_BY_TIME_ASC:
            comparator = new Comparator<File>() {

                @Override
                public int compare(File lhs, File rhs) {
                    if (!mEnableSorting) {
                        throw new SortInterruptedException("SORT_BY_TIME_ASC");
                    }
                    return compareTime(lhs, rhs, false);
                }

            };
            break;
        case SORT_BY_TIME_DESC:
            comparator = new Comparator<File>() {

                @Override
                public int compare(File lhs, File rhs) {
                    if (!mEnableSorting) {
                        throw new SortInterruptedException("SORT_BY_TIME_DESC");
                    }
                    return -compareTime(lhs, rhs, true);
                }

            };
            break;
        case SORT_BY_SIZE_ASC:
            comparator = new Comparator<File>() {
                @Override
                public int compare(File lhs, File rhs) {
                    if (!mEnableSorting) {
                        throw new SortInterruptedException("SORT_BY_SIZE_ASC");
                    }
                    return compareSize(lhs, rhs, false);
                }
            };
            break;
        case SORT_BY_SIZE_DESC:
            comparator = new Comparator<File>() {

                @Override
                public int compare(File lhs, File rhs) {
                    if (!mEnableSorting) {
                        throw new SortInterruptedException("SORT_BY_SIZE_DESC");
                    }
                    return -compareSize(lhs, rhs, true);
                }

            };
            break;
        }
    }

    public int getSortType() {
        return mSortType;
    }

    private String getFileType(String fileName) {
        String ret = "";
        int index = fileName.lastIndexOf(".");
        if (index != -1) {
            ret = fileName.substring(index + 1).toLowerCase(Locale.US);
        }
        return ret;
    }

    private int compareTime(File lhs, File rhs, boolean isDes) {
        int ret = (int) (lhs.lastModified() - rhs.lastModified());
        if (lhs.lastModified() > rhs.lastModified()) {
            ret = 1;
        } else if (lhs.lastModified() < rhs.lastModified()) {
            ret = -1;
        } else {
            ret = 0;
        }
        if (ret == 0) {
            Log.i(TAG, "time is equal, compare the name");
            Collator myCollator = Collator
                    .getInstance(java.util.Locale.CHINA);
            ret = myCollator.compare(lhs.getName().toLowerCase(), rhs
                    .getName().toLowerCase());
            if (ret == 0) {
                Log.i(TAG, "if time and name is equal, copmare the file");
                ret = rhs.compareTo(rhs);
            }
        }
        return ret;
    }

    private int compareSize(File lhs, File rhs, boolean isDes) {
        int ret = (int) (lhs.length() - rhs.length());
        if (ret == 0) {
            Log.i(TAG, "time is equal, compare the name");
            Collator myCollator = Collator
                    .getInstance(java.util.Locale.CHINA);
            ret = myCollator.compare(lhs.getName().toLowerCase(), rhs
                    .getName().toLowerCase());
            if (ret == 0) {
                Log.i(TAG, "time and name is equal, compare the file");
                ret = lhs.compareTo(rhs);
            }
        }
        return ret;
    }
    
    public static int getSelectItemByType(int type){
        switch(type){
        case SORT_BY_TYPE:
        case SORT_BY_TYPE_DESC:
            return 1;
        case SORT_BY_NAME:
        case SORT_BY_NAME_DESC:
            return 0;
        case SORT_BY_SIZE_ASC:
        case SORT_BY_SIZE_DESC:
            return 3;
        case SORT_BY_TIME_DESC:
        case SORT_BY_TIME_ASC:
            return 2;
        default:
            return 0;
        }
    }

    private static class SortInterruptedException extends RuntimeException {

        private static final long serialVersionUID = 1L;

        public SortInterruptedException(String message) {
            super(message);
        }
    }
}
