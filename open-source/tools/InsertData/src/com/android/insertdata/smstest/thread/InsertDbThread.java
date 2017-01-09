package com.android.insertdata.smstest.thread;

import java.io.UnsupportedEncodingException;

import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.os.Handler;
import android.provider.Telephony;

import com.android.insertdata.smstest.entity.SingleParam;
import com.android.insertdata.smstest.entity.SmsConfig;


public class InsertDbThread implements Runnable {
	private SmsConfig smsConfig;
	private ProgressDialog insertProgressDialog;
	private Handler mHandler;
	ContentResolver resolver;
	
	
	public InsertDbThread(SmsConfig smsConfig,Context context,ContentResolver resolver,ProgressDialog insertProgressDialog , Handler handler)
	{
		this.smsConfig = smsConfig;
		this.insertProgressDialog = insertProgressDialog;
		this.mHandler = handler;
		this.resolver = resolver;
	}
	public void run() {
		
		long startting = System.currentTimeMillis();
		insertDb();
		long end = System.currentTimeMillis();
		SingleParam.time=(end-startting);
		mHandler.sendEmptyMessage(2);
	}
		
	public void insertDb()
	{
		
		int count = smsConfig.count;
		long phoneNo = Long.parseLong(smsConfig.phoneNo);
//		Long threadId = -1L;
		if(!smsConfig.isThread)
		{
			//threadId = null;
		}
		/*else if(!"".equals(smsConfig.threadId))
		{
			threadId = null;
			threadId = Long.parseLong(smsConfig.threadId);
		}*/
		insertProgressDialog.setMax(count*smsConfig.threadCount/1);
		android.util.Log.e("xxxxxxx","--------insertProgressDialog.setMax(count*smsConfig.threadCount/1) ======== threadCount = "+count*smsConfig.threadCount/1 
		         + ", count ="+count + ", threadCount="+smsConfig.threadCount);
		for(int j=0;j<smsConfig.threadCount;j++)
		{
			phoneNo++;
			for(int i=0;i<count;i++){
				++SingleParam.status;
				insertProgressDialog.setProgress(((j*count)+i+1));
				System.out.println("process==="+((j*count)+i+1));
				if(!SingleParam.threadSwitch){
					return;
				}
				if(smsConfig.isAutoUp)
				{
					phoneNo++;
				}
				smsConfig.phoneNo = String.valueOf(phoneNo);
				smsConfig.subject = (i+1)+" sms of "+(j+1)+" thd(total "+((j*count)+i+1)+" sms)";
				String temp = smsConfig.subject + "  " + smsConfig.smsContent;
				String encoding = System.getProperty("file.encoding");
		        System.out.println("Default System Encoding:" + encoding);
				try {
					String content = new String(temp.getBytes(encoding), "UTF-8");

					if ("1".equals(SmsConfig.boxId)) {
						Telephony.Sms.Inbox.addMessage(resolver, smsConfig.phoneNo,
								content, smsConfig.subject, (long) System
										.currentTimeMillis(), smsConfig.isRead);

					} else if ("2".equals(SmsConfig.boxId)) {
						Telephony.Sms.Sent.addMessage(resolver, smsConfig.phoneNo,
								content, smsConfig.subject, (long) System
										.currentTimeMillis());

					} else if ("3".equals(SmsConfig.boxId)) {
//						Telephony.Sms.Draft.addMessage(resolver, smsConfig.phoneNo,
//								content, smsConfig.subject, (long) System
//										.currentTimeMillis());

					} else if ("4".equals(SmsConfig.boxId)) {

					} else if ("5".equals(SmsConfig.boxId)) {

					} else if ("6".equals(SmsConfig.boxId)) {

					}
				} catch (UnsupportedEncodingException e) {
					e.printStackTrace();
				}
			}

			
		}
		insertProgressDialog.dismiss();
	}

}
