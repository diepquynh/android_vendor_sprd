
package com.sprd.note.data;

import java.util.ArrayList;

import com.sprd.note.utils.LogUtils;

import android.database.MatrixCursor;
import android.os.Debug;
import android.text.TextUtils;
import android.util.Log;

public class CustomSorter {

    private static final String TAG = "CustomSorter";
    private static final boolean DEBUG = LogUtils.DEBUG;

    private ArrayList<Data> mDataSet = new ArrayList<Data>();
    private String mTarget;

    public CustomSorter(String target) {
        mTarget = target;
    }

    private class Data {
        int id;
        String text1;
        int query;
        int index;
        float weight;
        private static final int COLUMES = 3;

        private Data(int _id, String _text, int _query) {
            id = _id;
            text1 = _text;
            query = _query;
            index = text1.indexOf(mTarget);
            weight = geterateWeight();
        }

        private float geterateWeight() {
            if (TextUtils.isEmpty(mTarget)) {
                return 0f;
            }
            final int tarLen = mTarget.length();
            int idx = 0;
            int tmp;
            tmp = text1.indexOf(mTarget);
            if (tmp == -1)
                return 0f;
            for (;;) {
                if (tmp >= 0) {
                    idx++;
                    tmp = text1.indexOf(mTarget, tmp + tarLen);
                } else {
                    return (idx * tarLen) * 1.0f / text1.length();
                }
            }
        }
    }

    synchronized void add(int id, String text1, int query) {
        Log.v(TAG, "add  id-->" + id + "  text1-->" + text1 + "   query-->" + query);
        if (DEBUG) {
            Log.e(TAG, "add:" + id + " " + text1 + " " + query);
        }
        Data d = new Data(id, text1, query);
        if (mDataSet.isEmpty()) {
            mDataSet.add(d);
            return;
        } else {
            final int index = d.index;
            if (index < 0) {
                mDataSet.add(d);
                return;
            } else {
                final int count = mDataSet.size();
                for (int i = 0; i < count; i++) {
                    if (mDataSet.get(i).index > index) {
                        mDataSet.add(i, d);
                        return;
                    } else if (mDataSet.get(i).index == index) {
                        for (int j = i; j < count; j++) {
                            if (mDataSet.get(j).index == index) {
                                if (mDataSet.get(j).weight >= d.weight) {

                                } else {
                                    mDataSet.add(j, d);
                                    return;
                                }
                            } else {
                                mDataSet.add(j, d);
                                return;
                            }
                        }
                    } else {
                        ;
                    }
                    if (i == count - 1) {
                        mDataSet.add(d);
                        return;
                    }
                }
            }
        }
    }

    public synchronized MatrixCursor dumpToCorsor(String[] columes) {
        if (columes.length != Data.COLUMES) {
            return null;
        }
        MatrixCursor matrix = new MatrixCursor(columes);
        for (Data d : mDataSet) {
            matrix.newRow().add(d.id).add(d.text1).add(d.query);
        }
        matrix.move(-1);
        if (DEBUG) {
            dump();
        }
        return matrix;
    }

    public void dump() {
        for (Data d : mDataSet) {
            Log.e(TAG, d.text1);
        }
    }
}
