package com.android.sprdlauncher2;

import android.content.Context;
import android.content.IntentFilter;

/**
 * SPRD Receiver operation @{
 */
public class SprdReceiverOperation {

    /**
     * register receiver for UnreadLoader
     * @param context
     * @return
     */
    public static UnreadLoader registerReceiverUnreadLoader(Context context){
        UnreadLoader unreadLoader = new UnreadLoader(context);
        IntentFilter unReadfilter = new IntentFilter();
        unReadfilter.addAction(UnreadLoader.ACTION_MISSED_CALL_COUNT);
        unReadfilter.addAction(UnreadLoader.ACTION_MISSED_CALL_CANCEL);
        unReadfilter.addAction(UnreadLoader.ACTION_UNREAD_MESSAGE_COUNT);
        unReadfilter.addAction(UnreadLoader.ACTION_DELETE_UNREAD_MESSAGE_COUNT);
        context.registerReceiver(unreadLoader, unReadfilter);
        return unreadLoader;
    }

    /**
     * unregister receiver for UnreadLoader
     * @param context
     * @param mUnreadLoader
     */
    public static void unregisterReceiverUnreadLoader(Context context, UnreadLoader unreadLoader){
        if(unreadLoader !=null)
            context.unregisterReceiver(unreadLoader);
    }

    /**
     * initialize for UnreadLoader
     * @param launcher
     * @param mUnreadLoader
     */
    public static void initializeUnreadLoader(Launcher launcher, UnreadLoader unreadLoader){
        unreadLoader.initialize(launcher);
    }
}
/** @} */
