
package com.sprd.xml.parser.prv;

import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.media.AudioManager;
import android.net.Uri;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.text.TextUtils;
import android.util.Log;
//add for bug 498188 begin
import com.sprd.omacp.R;
//add for bug 498188 end

public class OmacpUtils {
    public static void setSubId(int subId) {
        mSubId = subId;
    }

    public static int getSubId() {
        return mSubId;
    }

    public static void registerMsgNf(Context context) {
        Log.d(TAG, "register Message Notification");
        //modified for bug 498188 begin
        Notification.Builder builder = new Notification.Builder(context);
        builder.setSound(Uri.parse(NOTIFICATIONURI_STRING));
        builder.setDefaults(Notification.DEFAULT_LIGHTS);
        builder.setSmallIcon(R.drawable.ic_launcher);
        // add for bug 518831 begin
        builder.setContentTitle("OTA");
        builder.setContentText(context.getString(R.string.OTAConfig_title));
        // add for bug 518831 end
        AudioManager audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        NotificationManager nm = (NotificationManager) context
                .getSystemService(Context.NOTIFICATION_SERVICE);
        nm.notify(NOTIFICATION_ID, builder.getNotification());
        //modified for bug 498188 end

        // wake the screen,when the message received.
        PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        WakeLock wl = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK
                | PowerManager.ACQUIRE_CAUSES_WAKEUP, "OTAMsgNotification");
        wl.acquire(WAKETIME);
    }
    //add for bug 530106 begin
    public static void clearMsgNf(Context context) {
        Log.d(TAG, "clear Message Notification begin");
        AudioManager audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        NotificationManager nm = (NotificationManager) context
                .getSystemService(Context.NOTIFICATION_SERVICE);
        nm.cancel(NOTIFICATION_ID);
        Log.d(TAG, "clear  Message Notification end");
    }
    //add for bug 530106 end
    /**
     * Converts a byte array into a String of hexadecimal characters.
     *
     * @param bytes an array of bytes
     *
     * @return hex string representation of bytes array
     */
    //add for upgrade to Android N
    public static String bytesToHexString(byte[] bytes) {
        if (bytes == null)
            return null;
        StringBuilder ret = new StringBuilder(2 * bytes.length);
        for (int i = 0; i < bytes.length; i++) {
            int b;
            b = 0x0f & (bytes[i] >> 4);
            ret.append("0123456789abcdef".charAt(b));
            b = 0x0f & bytes[i];
            ret.append("0123456789abcdef".charAt(b));
        }
        return ret.toString();
    }
    //add for upgrade to Android N

    private static Integer mSubId = null;
    private static final int WAKETIME = 10000;
    private static final int NOTIFICATION_ID = 110;
    private static final String TAG = "OmacpUtils";
    private static final String NOTIFICATIONURI_STRING = "content://settings/system/notification_sound";
}
