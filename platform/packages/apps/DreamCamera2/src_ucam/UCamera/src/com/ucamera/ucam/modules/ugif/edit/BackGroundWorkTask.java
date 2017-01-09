/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit;

import com.ucamera.ucam.compatible.Models;

import android.app.ProgressDialog;
import android.content.Context;
import android.os.AsyncTask;
import android.os.AsyncTask.Status;
import android.view.View;
import android.view.Window;

public class BackGroundWorkTask {
    private static WorkTask mTask = null;
    private static ProgressDialog mDialog = null;

    public interface OnTaskFinishListener {
        public void afterTaskFinish();
    }

    public static void processTask(Context context, String message, Runnable runnable,
            OnTaskFinishListener listener) {
        if (mDialog != null && mDialog.isShowing()
                || (mTask != null && mTask.getStatus() == Status.RUNNING)) {
            return;
        }

        mDialog = new ProgressDialog(context);
        mDialog.requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
        mDialog.setMessage(message);
        mDialog.setIndeterminate(false);
        mDialog.setCancelable(false);
        mDialog.show();
        if(Models.getModel().equals(Models.AMAZON_KFTT)) {
            mDialog.getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
        }
        OnTaskFinishListener tempListener = listener;
        if (listener == null) {
            if (context instanceof GifEditActivity) {
                tempListener = (OnTaskFinishListener) context;
            }
        }
        mTask = new WorkTask(runnable, tempListener);
        mTask.execute();
    }

    private static void dismissDialog() {
        if (mDialog != null && mDialog.getWindow() != null) {
            try{
                mDialog.dismiss();
            } catch(RuntimeException e){
            }
            mDialog = null;
        }
        mTask = null;
    }

    public static class WorkTask extends AsyncTask<Void, Void, Void> {
        Runnable mRunnable = null;
        OnTaskFinishListener mListener;

        private WorkTask(Runnable runnable, OnTaskFinishListener listener) {
            mRunnable = runnable;
            mListener = listener;
        }

        @Override
        protected void onPreExecute() {
        }

        @Override
        protected Void doInBackground(Void... params) {
            mRunnable.run();
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            if (mListener != null) {
                mListener.afterTaskFinish();
            }
            dismissDialog();
        }

        @Override
        protected void onCancelled() {
            dismissDialog();
        }
    }
}
