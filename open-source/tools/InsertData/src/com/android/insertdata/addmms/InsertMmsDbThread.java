package com.android.insertdata.addmms;

import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.os.Handler;
import android.util.Log;

public class InsertMmsDbThread implements Runnable {
	static final String TAG = "InsertMmsDbThread";
	private MmsConfig mmsConfig;
	private ProgressDialog insertProgressDialog;
	private Handler mHandler;
	ContentResolver resolver;
	Context context;

	public InsertMmsDbThread(MmsConfig mmsConfig, Context context,
			ContentResolver resolver, ProgressDialog insertProgressDialog,
			Handler handler) {
		this.mmsConfig = mmsConfig;
		this.insertProgressDialog = insertProgressDialog;
		this.mHandler = handler;
		this.resolver = resolver;
		this.context = context;
		
	}

	public void run() {
		long startting = System.currentTimeMillis();
		insertDb();
		long end = System.currentTimeMillis();
		SingleMmsParam.time = (end - startting);
		mHandler.sendEmptyMessage(2);
	}

	public void insertDb() {
		MmsDataGenrat mmsDataGenrat = new MmsDataGenrat(mmsConfig, resolver,context);
		
		insertProgressDialog.setMax(mmsConfig.mmsCount);

		for (int i = 0; i < mmsConfig.mmsCount; i++) {
			if (mmsConfig.isAutoUp){
				long phoneNo = Long.parseLong(mmsConfig.phoneNo);
				phoneNo++;
				mmsConfig.phoneNo = String.valueOf(phoneNo);
			}
			mmsDataGenrat.setCurrent(i);
			mmsDataGenrat.insert();

			++SingleMmsParam.finishCount;
			insertProgressDialog.setProgress(i);
			if (!SingleMmsParam.threadSwitch) {
				Log.d(TAG, "threadSwitch");
				break;
			}
		}
		insertProgressDialog.dismiss();
	}

}
