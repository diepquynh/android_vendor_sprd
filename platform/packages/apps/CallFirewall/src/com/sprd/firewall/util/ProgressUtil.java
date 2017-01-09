
package com.sprd.firewall.util;

import java.util.Iterator;

import com.sprd.firewall.db.BlackColumns;

import com.sprd.firewall.R;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.database.Cursor;
import android.os.AsyncTask;
import android.util.Log;
import android.widget.Toast;
import android.content.Context;
import android.content.res.Resources;

public class ProgressUtil extends AsyncTask<Void, Integer, Void> {

    public interface ProcessTask {
        void doInBack(ProgressType type);
        void doResult();
    }

    public enum ProgressType {
        NO_TYPE,
        BLACKLIST_ADD,
        BLACKLIST_DEL,
        CALLLOG_DEL,
        SMSLOG_DEL,
    }

    private static final String TAG = "ProgressUtil";

    private ProgressDialog mProgressDialog = null;
    private boolean mProgressRunState = false;

    private ProgressType mProgressType = ProgressType.NO_TYPE;

    private int mMaxNum = 0;
    private Context mContext;
    private int mtitleId;

    ProcessTask mProcessTask;

    public ProgressUtil(Context context, int maxNum, ProgressType type,ProcessTask processTask) {
        mContext = context;
        mMaxNum = maxNum;
        mProgressType = type;
        mProcessTask = processTask;
    }

    public void initProgressDialog(ProgressType progressType) {

        if (progressType == ProgressType.NO_TYPE && mProgressRunState == false) {
            Log.i(TAG, "initProgressDialog return");
            return;
        } else {
            if (mProgressDialog == null) {
                mProgressDialog = new ProgressDialog(mContext);
                Log.i(TAG, "progressDialog create");
            }
            if (mProgressRunState == true) {
                Log.i(TAG, "progressDialog runing");

                mProgressDialog.setTitle(mContext.getResources().getString(mtitleId));
                mProgressDialog.setCancelable(false);
                mProgressDialog.setIndeterminate(false);
                mProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
                mProgressDialog.setMax(mMaxNum);
                if (!mProgressDialog.isShowing()) {
                    mProgressDialog.show();
                }
            }
        }
    }

    public void disMissProgressDailog() {
        Log.i(TAG, "disMissProgressDailog");
        try {

            if ((mProgressDialog != null) && mProgressDialog.isShowing()) {
                mProgressDialog.cancel();
            }
        } catch (final Exception e) {
            // Handle or log or ignore 1141
            Log.i(TAG, "progressDialog.dismiss() filed");
        } finally {
            mProgressDialog = null;
        }
    }

    public void UpdateProgress(int num){

        publishProgress(num);
    }

    @Override
    protected void onPreExecute() {
        super.onPreExecute();
        Log.i(TAG, "onPreExecute");
        mProgressRunState = true;
        initProgressDialog(mProgressType);
    }

    @Override
    protected Void doInBackground(Void... params) {
        // TODO Auto-generated method stub
        Log.i(TAG, "doInBackground mProgressType=" + mProgressType);
        mProcessTask.doInBack(mProgressType);
        return null;
    }

    @Override
    protected void onProgressUpdate(Integer... values) {
        // TODO Auto-generated method stub
        super.onProgressUpdate(values);
        if (mProgressDialog != null) {
            mProgressDialog.setProgress(values[0]);
        }
    }

    @Override
    protected void onPostExecute(Void result) {
        Log.i(TAG, "onPostExecute");
        mProgressType = ProgressType.NO_TYPE;
        mProgressRunState = false;
        disMissProgressDailog();
        mProcessTask.doResult();
    }

    public void setMtitleId(int mtitleId) {
        this.mtitleId = mtitleId;
    }

    public int getmMaxNum() {
        return mMaxNum;
    }

    public void setmMaxNum(int maxNum) {
        this.mMaxNum = maxNum;
    }


    public ProgressType getmProgressType() {
        return mProgressType;
    }

    public void setmProgressType(ProgressType mProgressType) {
        this.mProgressType = mProgressType;
    }

    public boolean ismProgressRun() {
        return mProgressRunState;
    }
}
