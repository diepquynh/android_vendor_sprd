/** Create by Spreadst */
package com.android.sprdlauncher2;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.FontMetrics;
import android.graphics.Rect;
import android.os.AsyncTask;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import com.android.sprdlauncher2.R;
import java.lang.ref.WeakReference;
import java.util.ArrayList;

public class UnreadLoader extends BroadcastReceiver {
    private static final String TAG = "UnreadLoader";
    private static final String TAG_UNREADSHORTCUTS = "unreadshortcuts";
    public final static String ACTION_MISSED_CALL_COUNT = "com.android.call.MISSED_CALL";
    public final static String ACTION_MISSED_CALL_CANCEL = "com.android.call.MISSED_CALL_CANCEL";
    public final static String EXTRA_MESSAGE_COUNT = "newMessagecount";
    public final static String ACTION_UNREAD_MESSAGE_COUNT = "com.android.mms.NEW_MASSAGE_RECEVICE_COUNT";
    public final static String ACTION_DELETE_UNREAD_MESSAGE_COUNT = "com.android.mms.DELETE_UNREAD_MESSAGE_COUNT";
    static String EXTRA_MISSED_CALL_KEY ="EXTRA_MISSED_CALL_KEY";
    private static int sUnreadSupportShortcutsNum = 0;
    private static final Object LOG_LOCK = new Object();
    private static Paint mPaint = new Paint();
    private static final String INFINITY_UNICODE = "\u221e";

    private static final int MAX_SIZE = 100;

    private static float mFewTextSize;

    private static float mManytextSize;

    private static final int mOffsetY = 3;
    private static final Canvas sCanvas = new Canvas();
    private Context mContext;
    public static int unReadMmsCount;
    public static int missCallCount;
    public static ComponentName mmsComponentName;
    public static ComponentName phoneComponentName;

    private WeakReference<LauncherModel.Callbacks> mCallbacks;

    public UnreadLoader(Context context) {
        mContext = context;
        mmsComponentName = new ComponentName("com.android.mms",
                "com.android.mms.ui.ConversationList");
        phoneComponentName = new ComponentName("com.android.dialer",
                "com.android.dialer.DialtactsActivity");
    }

    @Override
    public void onReceive(final Context context, final Intent intent) {
        final String action = intent.getAction();
        /* SPRD : fix bug297395 When receive unreadinfo num, first save @{ */
        unReadMmsCount = intent.getIntExtra(EXTRA_MESSAGE_COUNT, 0);
        missCallCount = intent.getIntExtra(EXTRA_MISSED_CALL_KEY, 0);
        if(action.equals(ACTION_UNREAD_MESSAGE_COUNT)) {
            saveUnreadInfoNum(mmsComponentName ,unReadMmsCount);
        }else if(action.equals(ACTION_MISSED_CALL_COUNT)) {
            saveUnreadInfoNum(phoneComponentName ,missCallCount);
        }
        if(mCallbacks != null){
            final LauncherModel.Callbacks callbacks = mCallbacks.get();
            if (action.equals(ACTION_UNREAD_MESSAGE_COUNT)) {
                Log.d(TAG,"unReadMmsCount="+unReadMmsCount);
                if(callbacks != null){ // SPRD : fix bug204290 null pointer
                    callbacks.bindComponentUnreadChanged(mmsComponentName, unReadMmsCount);
                }
            }
            if (action.equals(ACTION_MISSED_CALL_COUNT)) {
                Log.d(TAG,"missCallCount="+missCallCount);
                if(callbacks != null){// SPRD : fix bug204290 null pointer
                    callbacks.bindComponentUnreadChanged(phoneComponentName, missCallCount);
                }
            }
        }
        /* @} */
    }
    /**
     * SPRD : fix bug297395 save unreadinfo num
     * @param component
     * @param unreadNum
     */
    public void saveUnreadInfoNum(ComponentName component, int unreadNum) {
        SharedPreferences mUnreadInfoPreference = (SharedPreferences) mContext
                .getSharedPreferences(Launcher.UNREADINFO,
                        Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = mUnreadInfoPreference.edit();
        editor.putInt(component.getPackageName(), unreadNum);
        editor.commit();
    }
    /** @} */

    /**
     * Set this as the current Launcher activity object for the loader.
     */
    public void initialize(LauncherModel.Callbacks callbacks) {
        mCallbacks = new WeakReference<LauncherModel.Callbacks>(callbacks);
    }
    static void drawUnreadEventIfNeed(Context context, Canvas canvas, View icon) {
    }
    static Bitmap getBitmapWithNum(Context context,ItemInfo info,Bitmap origin){
        //Bitmap bitmapWithNum = Bitmap.createBitmap(origin, 0, 0, origin.getWidth(), origin.getHeight());
        Bitmap bitmapWithNum = origin.copy(Bitmap.Config.ARGB_8888, true);
        Canvas canvas  = new Canvas(bitmapWithNum);
        canvas.drawBitmap(UnreadLoader.drawSubBitmap(context, info.unreadNum), 0,0,new Paint());
        return bitmapWithNum;
    }
    static Bitmap drawSubBitmap(Context context,int drawText) {
        mPaint.setAntiAlias(true);
        mPaint.setColor(Color.WHITE);
        DisplayMetrics metrics = context.getResources().getDisplayMetrics();
        final float density = metrics.density;
        mFewTextSize = 14 * density;
        mManytextSize = 18 * density;
        String endstr = drawText < MAX_SIZE ? ("" + drawText) : INFINITY_UNICODE;
        if (drawText < MAX_SIZE) {
            mPaint.setTextSize(mFewTextSize);
        } else {
            mPaint.setTextSize(mManytextSize);
        }
        Drawable mIcon = context.getResources().getDrawable(R.drawable.ic_count_dial_unlock);

        int width = mIcon.getIntrinsicWidth();
        int height = mIcon.getIntrinsicHeight();

        int textWidth = (int) mPaint.measureText(endstr, 0, endstr.length());
        if (textWidth > width) {
            width = textWidth + 6;
        }

        final Bitmap thumb = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        final Canvas canvas = sCanvas;
        canvas.setBitmap(thumb);
        Rect r = new Rect(0, 0, width, height);
        mIcon.setBounds(r);
        mIcon.draw(canvas);
        int x = (int) ((width - textWidth) * 0.5);
        FontMetrics fm = mPaint.getFontMetrics();
        int y = (int) ((height - (fm.descent - fm.ascent)) * 0.5 - fm.ascent);
        canvas.drawText(endstr, x, y - mOffsetY, mPaint);
        return thumb;
    }
}
