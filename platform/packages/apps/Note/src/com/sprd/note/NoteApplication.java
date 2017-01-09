
package com.sprd.note;

import android.app.Application;
import android.content.Context;
import android.util.Log;

import com.sprd.note.data.NoteDataManager;
import com.sprd.note.data.NoteDataManagerImpl;

public class NoteApplication extends Application {
    private static final String TAG = "NoteApplication";

    NoteDataManager mDataManager = null;

    public synchronized NoteDataManager getNoteDataManager(Context context) {
        if (mDataManager == null) {
            mDataManager = NoteDataManagerImpl.getNoteDataManger(context);
            mDataManager.initData(context);
        }
        return mDataManager;
    }

    @Override
    public void onTerminate() {
        super.onTerminate();
        Log.i(TAG, "onTerminate");
    }
}
