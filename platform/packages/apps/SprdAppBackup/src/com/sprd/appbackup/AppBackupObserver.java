package com.sprd.appbackup;

import android.content.ContentResolver;
import android.content.Context;
import android.database.ContentObserver;
import android.net.Uri;
import android.os.Handler;
import android.provider.CalendarContract.Events;
import android.provider.ContactsContract;
import android.provider.MediaStore.Images;

public class AppBackupObserver extends ContentObserver {
    public static final int CONTENT_CHANGE = 100;
    public static final int UPDATE_INTERVAL = 600;
    
    Handler mHandler;
    ContentResolver mContentResolver;

    public AppBackupObserver(Context context,Handler handler) {
        super(handler);
        mHandler = handler;
        mContentResolver = context.getContentResolver();
    }

    @Override
    public void onChange(boolean selfChange) {
        super.onChange(selfChange);
        mHandler.removeMessages(CONTENT_CHANGE);
        mHandler.sendEmptyMessageDelayed(CONTENT_CHANGE, UPDATE_INTERVAL);
    }

    public void registerObservers(){
        mContentResolver.registerContentObserver(Uri.parse("content://mms-sms"),true,this);
        mContentResolver.registerContentObserver(ContactsContract.Contacts.CONTENT_URI,true,this);
        mContentResolver.registerContentObserver(Events.CONTENT_URI,true,this);
        mContentResolver.registerContentObserver(Images.Media.EXTERNAL_CONTENT_URI,true,this);
        mContentResolver.registerContentObserver(Images.Media.INTERNAL_CONTENT_URI,true,this);
    }
    
    public void unregisterObservers(){
        mContentResolver.unregisterContentObserver(this);
    }
}
