package plugins.sprd.unreadinfo;

import com.android.launcher3.Folder;
import com.android.launcher3.FolderIcon;
import com.android.launcher3.ShortcutAndWidgetContainer;
import com.android.launcher3.SprdUnreadInfoManager;
import com.android.launcher3.LauncherAppState;
import com.android.launcher3.IconCache;
import com.android.launcher3.R;
import com.android.launcher3.Launcher;
import com.android.launcher3.ShortcutInfo;
import com.android.launcher3.AppInfo;
import com.android.launcher3.BubbleTextView;
import com.android.launcher3.FastBitmapDrawable;
import com.android.launcher3.Workspace;

import java.io.File;
import java.io.FileOutputStream;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import android.app.AddonManager;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.UriMatcher;
import android.content.pm.PackageManager;
import android.database.ContentObserver;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Environment;
import android.provider.BaseColumns;
import android.provider.CallLog;
import android.util.Log;
import android.view.View;
import android.Manifest.permission;

import com.android.launcher3.allapps.AllAppsContainerView;
import com.android.launcher3.allapps.AlphabeticalAppsList;
import com.android.launcher3.compat.UserHandleCompat;

// SPRD: bug372523 2014-11-21 Feature show unread mmssms/missed calls info.
public class UnreadInfoManagerAddon extends SprdUnreadInfoManager implements AddonManager.InitialCallback, Runnable {

    private static final String TAG = "UnreadInfoManagerAddon";
    private static final boolean DEBUG = true;

    public static final String EXCEED_TEXT = "99+";
    public static final int MAX_UNREAD_COUNT = 99;
    private static final int STORAGE_PERMISSION_REQUEST_CODE = 2;

    // Apps that show in Launcher workspace or in all apps, currently we only
    // use Mms and Phone, we will show unread info num on their icon.
    private static final ComponentName sMmsComponentName = new ComponentName("com.android.messaging",
            "com.android.messaging.ui.conversationlist.ConversationListActivity");
    private static final ComponentName sDialerComponentName = new ComponentName("com.android.dialer",
            "com.android.dialer.DialtactsActivity");

    private static final  int MATCH_CALL = 1;
    private static final int MATCH_MMSSMS = 2;
    private static final UriMatcher sURIMatcher = new UriMatcher(UriMatcher.NO_MATCH);

    static {
        sURIMatcher.addURI("call_log", "calls", MATCH_CALL);
        sURIMatcher.addURI("mms-sms", null, MATCH_MMSSMS);
    }

    private static final Uri MMSSMS_CONTENT_URI = Uri.parse("content://mms-sms");
    private static final Uri CALLS_CONTENT_URI = CallLog.Calls.CONTENT_URI;
    private static final Uri MMS_CONTENT_URI = Uri.parse("content://mms");
    private static final Uri SMS_CONTENT_URI = Uri.parse("content://sms");

    private static final String MISSED_CALLS_SELECTION =
            CallLog.Calls.TYPE + " = " + CallLog.Calls.MISSED_TYPE + " AND " + CallLog.Calls.NEW + " = 1";

    private Context mContext;
    private WeakReference<Launcher> mLauncherRef;
    private LauncherAppState mAppState;
    private HashMap<ComponentName, UnreadInfo> mUnreadInfoCache = new HashMap<ComponentName, UnreadInfo>();
    private HashMap<ComponentName, Boolean> mUnreadInfoChangedCache = new HashMap<ComponentName, Boolean>();

    // assume the unit is dp
    private float mLargeTextSize = 14;
    private float mMiddleTextSize = 12;
    private float mSmallTextSize = 10;
    private Drawable mBackground;

    private boolean mUpdateUnreadInfoTheFirstTime;
    private IconCache mIconCache;
    private UserHandleCompat mUser;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public UnreadInfoManagerAddon() {
        super(null);
    }

    public void init(LauncherAppState appState) {

        Log.d(TAG, "UnreadInfoManagerAddon");
        mAppState = appState;
        mContext = appState.getContext();

        mLargeTextSize = mContext.getResources().getDimension(R.dimen.unread_info_large_text_size);
        mMiddleTextSize = mContext.getResources().getDimension(R.dimen.unread_info_middle_text_size);
        mSmallTextSize = mContext.getResources().getDimension(R.dimen.unread_info_small_text_size);
        mBackground = mContext.getResources().getDrawable(R.drawable.unread_info_background);

        Thread thread = new IconCacheThread();
        thread.start();

        // register content observer only once for the LauncherApplication
        ContentResolver resolver = mContext.getContentResolver();
        resolver.registerContentObserver(CALLS_CONTENT_URI, true, this);
        resolver.registerContentObserver(MMSSMS_CONTENT_URI, true, this);
        mUpdateUnreadInfoTheFirstTime = true;
    }

    class IconCacheThread extends Thread{
        @Override
        public void run(){
            Log.d(TAG, "run --------");
            mIconCache = mAppState.getIconCache();
            mUser = UserHandleCompat.myUserHandle();

            Intent intent = new Intent();
            intent.setComponent(sMmsComponentName);
            Bitmap originIcon = mIconCache.getIcon(intent, mUser);
            mUnreadInfoCache.put(sMmsComponentName, new UnreadInfo(-1, originIcon, null));

            intent.setComponent(sDialerComponentName);
            originIcon = mIconCache.getIcon(intent, mUser);
            mUnreadInfoCache.put(sDialerComponentName, new UnreadInfo(-1, originIcon, null));
        }
    }

    public boolean isUnreadinfoOn(){
        return true;
    }

    public void bindLauncher(Launcher launcher) {
        mLauncherRef = new WeakReference<Launcher>(launcher);
    }

    public void terminate() {
        ContentResolver resolver = mContext.getContentResolver();
        resolver.unregisterContentObserver(this);
    }

    /**
     * Note: this method be called in Launcher.onResume to check if there has
     * any unread info need to be updated, then update these unread info all at
     * once.
     */
    public void updateUnreadInfoIfNeeded() {
        Log.d(TAG, "updateUnreadInfoIfNeeded mUnreadInfoChangedCache.size() = "+mUnreadInfoChangedCache.size());
        if (mUnreadInfoChangedCache.size() > 0
                && mLauncherRef != null && mLauncherRef.get() != null) {
            mLauncherRef.get().updateUnreadInfo();
        }
    }

    /**
     * Note: this method is called by Working thread in LauncherModel before load
     * and bind any icon into Workspace or AppsCustomizePagedView.
     *
     * Prepare the unread data and bitmap and then apply these bitmap when
     * new BubbleTextView need to be created.
     *
     * This method should not be called in UI thread for it manipulate database.
     */
    public void prepareUnreadInfo() {
        if (mUpdateUnreadInfoTheFirstTime) {
            mUpdateUnreadInfoTheFirstTime = false;
            for(ComponentName cn : mUnreadInfoCache.keySet()) {
                int unreadNum = loadUnreadInfoCount(cn);
                updateUnreadInfoCache(cn, unreadNum);
            }
        }
    }

    @Override
    public void onChange(boolean selfChange, Uri uri) {
        super.onChange(selfChange, uri);
        if (DEBUG) {
            Log.d(TAG, String.format("onChange: uri=%s selfChange=%b", uri.toString(), selfChange));
        }
        int match = sURIMatcher.match(uri);

        Log.d(TAG, "addon Launcher.getPermissionFlag() = "+Launcher.getPermissionFlag());
        switch (match) {
            case MATCH_CALL:
                if (Launcher.getPermissionFlag()) {
                    asyncGetUnreadInfoAndTriggerUpdate(sDialerComponentName);
                }
                break;
            case MATCH_MMSSMS:
                if (Launcher.getPermissionFlag()) {
                    asyncGetUnreadInfoAndTriggerUpdate(sMmsComponentName);
                }
                break;
        }
    }

    public Bitmap getBitmapWithUnreadInfo(ShortcutInfo info, Bitmap originBmp) {
        ComponentName cn = info.getIntent().getComponent();
        if (originBmp == null) {
            originBmp = info.getIcon(mAppState.getIconCache());
        }
        return getBitmapWithUnreadInfoInternal(cn, originBmp);
    }

    public Bitmap getBitmapWithUnreadInfo(AppInfo info, Bitmap originBmp) {
        ComponentName cn = info.getIntent().getComponent();
        if (originBmp == null) {
            originBmp = info.iconBitmap;
        }
        return getBitmapWithUnreadInfoInternal(cn, originBmp);
    }


    public boolean updateAppinfoUnreadInfo(AppInfo info) {
        BubbleTextView bubble = new BubbleTextView(mContext);
        Bitmap b = getBitmapWithUnreadInfo(info, null);
        FastBitmapDrawable topDrawable = mLauncherRef.get().createIconDrawable(b);
        bubble.setTag(info);
        bubble.setCompoundDrawables(null, topDrawable, null, null);
        ComponentName cn = info.getIntent().getComponent();
        return hasComponentUnreadInfoChanged(cn);
    }

    public boolean updateBubbleTextViewUnreadInfo(BubbleTextView bubble) {
        if (bubble != null) {
            Object tag = bubble.getTag();
            ComponentName cn = null;
            if (tag instanceof ShortcutInfo) {
                ShortcutInfo info = (ShortcutInfo) tag;
                Bitmap b = getBitmapWithUnreadInfo(info, null);
                if(mLauncherRef != null && mLauncherRef.get() != null){
                    FastBitmapDrawable iconDrawable = mLauncherRef.get().createIconDrawable(b);
                    iconDrawable.setGhostModeEnabled(info.isDisabled != 0);
                    bubble.setCompoundDrawables(null, iconDrawable, null, null);
                }
                cn = info.getIntent().getComponent();
            } else if (tag instanceof AppInfo) {
                AppInfo info = (AppInfo) tag;
                Bitmap b = getBitmapWithUnreadInfo(info, null);
                if(mLauncherRef != null && mLauncherRef.get() != null){
                    FastBitmapDrawable topDrawable = mLauncherRef.get().createIconDrawable(b);
                    bubble.setCompoundDrawables(null, topDrawable, null, null);
                }
                cn = info.getIntent().getComponent();
            }
            return hasComponentUnreadInfoChanged(cn);
        }
        return false;
    }

    private Bitmap getBitmapWithUnreadInfoInternal(ComponentName cn, Bitmap origin) {
        if (mUnreadInfoCache.containsKey(cn)) {
            UnreadInfo unreadInfo = mUnreadInfoCache.get(cn);
            if (unreadInfo != null && unreadInfo.mIconWithNum != null) {
                return unreadInfo.mIconWithNum;
            }
        }
        return origin;
    }

    private void saveBitmap(Bitmap bitMap, String mapName) {
        File file = new File("/storage/emulated/0/DCIM/Camera/" + mapName);
        if(file.exists()){
            file.delete();
        }
        FileOutputStream out;
        try{
            out = new FileOutputStream(file);
            if(bitMap.compress(Bitmap.CompressFormat.PNG, 90, out)){
                out.flush();
                out.close();
            }
        }catch(Exception e){
            Log.i(TAG, "exception" + e);
        }
    }

    private boolean hasComponentUnreadInfoChanged(ComponentName cn) {
        if (mUnreadInfoChangedCache.containsKey(cn)) {
            return mUnreadInfoChangedCache.get(cn);
        }
        return false;
    }

    private void asyncGetUnreadInfoAndTriggerUpdate(final ComponentName cn) {
        new AsyncTask<Void, Void, Integer>() {
            @Override
            protected Integer doInBackground(Void... params) {
                return loadUnreadInfoCount(cn);
            }

            @Override
            protected void onPostExecute(Integer unreadNum) {
                if (mUnreadInfoCache.containsKey(cn)
                        && unreadNum != mUnreadInfoCache.get(cn).mUnreadInfoNum) {
                    boolean updated = updateUnreadInfoCache(cn, unreadNum);
                    if (updated && mLauncherRef != null && mLauncherRef.get() != null) {
                        mUnreadInfoChangedCache.put(cn, true);
                        mLauncherRef.get().updateUnreadInfo();
                    }
                }
            }
        }.execute();
    }

    private int loadUnreadInfoCount(ComponentName component) {
        if (component.equals(sMmsComponentName)) {
            return getUnreadMessageCount();
        } else if (component.equals(sDialerComponentName)) {
            return getMissedCallCount();
        } else {
            return 0;
        }
    }

    private int getUnreadMessageCount() {
        int unreadSms = 0;
        int unreadMms = 0;

        Cursor cursor = null;
        ContentResolver resolver = mContext.getContentResolver();

        // get unread sms count
        cursor = resolver.query(SMS_CONTENT_URI, new String[] { BaseColumns._ID },
                "type = 1 AND read = 0", null, null);
        if (cursor != null) {
            unreadSms = cursor.getCount();
            cursor.close();
            if (DEBUG) {
                Log.i(TAG, String.format("getUnreadMessageCount unreadSms=%d", unreadSms));
            }
        }

        // get unread mms count
        cursor = resolver.query(MMS_CONTENT_URI, new String[] { BaseColumns._ID },
                "msg_box = 1 AND read = 0 AND ( m_type = 130 OR m_type = 132 ) AND thread_id > 0",
                null, null);

        if (cursor != null) {
            unreadMms = cursor.getCount();
            cursor.close();
            if (DEBUG) {
                Log.i(TAG, String.format("getUnreadMessageCount unreadMms=%d", unreadMms));
            }
        }
        return (unreadSms + unreadMms);
    }

    private int getMissedCallCount() {
        int missedCalls = 0;

        Cursor cursor = null;
        ContentResolver resolver = mContext.getContentResolver();

        cursor = resolver.query(CALLS_CONTENT_URI, new String[] { BaseColumns._ID },
                MISSED_CALLS_SELECTION, null, null);
        if (cursor != null) {
            missedCalls = cursor.getCount();
            cursor.close();
            if (DEBUG) {
                Log.i(TAG, String.format("getMissedCallCount missedCalls=%d", missedCalls));
            }
        }
        return missedCalls;
    }

    private Bitmap getOringIcon(ComponentName cn) {
        Intent intent = new Intent();
        intent.setComponent(cn);
        return mIconCache.getIcon(intent, mUser);
    }

    private boolean updateUnreadInfoCache(ComponentName cn, int unreadNum) {
        if (mUnreadInfoCache.containsKey(cn)) {
            UnreadInfo info = mUnreadInfoCache.get(cn);
            if(!(info.mUnreadInfoNum > MAX_UNREAD_COUNT && unreadNum > MAX_UNREAD_COUNT)){
                if (unreadNum == 0) {
                    // restore origin icon
                    if (info.mOriginIcon == null) {
                        info.mIconWithNum = getOringIcon(cn);
                    } else {
                        info.mIconWithNum = info.mOriginIcon;
                    }
                } else {
                    if (info.mOriginIcon != null) {
                        info.mIconWithNum = createBitmapWithUnreadInfo(info.mOriginIcon, unreadNum);
                    }
                }
                info.mUnreadInfoNum = unreadNum;
                return true;
            }
        }
        return false;
    }

    private Bitmap createBitmapWithUnreadInfo(Bitmap origin, int unreadNum) {
        Paint paint = new Paint();
        paint.setAntiAlias(true);
        paint.setColor(Color.WHITE);

        String finalText;
        if (unreadNum <= MAX_UNREAD_COUNT) {
            finalText = String.valueOf(unreadNum);
        } else {
            finalText = EXCEED_TEXT;
        }
        switch (finalText.length()) {
        case 1:
            paint.setTextSize(mLargeTextSize);
            break;
        case 2:
            paint.setTextSize(mMiddleTextSize);
            break;
        default:
            paint.setTextSize(mSmallTextSize);
            break;
        }

        int bgWidth = mBackground.getIntrinsicWidth();
        int bgHeight = mBackground.getIntrinsicHeight();

        Rect textBounds = new Rect();
        paint.getTextBounds(finalText, 0, finalText.length(), textBounds);

        int textHeight = textBounds.height();

        // Why we not use textBounds.width() as textWidth?
        // After test on devices, use the measured width is more precise to center
        // the number in the background circle. May be it is the red circle not in
        // then center of the background resource 'R.drawable.unread_info_background'
        // cause this problem.
        int textWidth = (int) paint.measureText(finalText, 0, finalText.length());

        // TODO: if textWidth >= bgWidth or textHeight >= bgHeight,
        // if textWidth >= circleWidth or textHeight >= circleHeight,
        // we must reduce the font size until fit the previous condition.

        // Why we multiply bgHeight by 0.71
        // Because the red circle's height in the background bitmap occupied 71%.
        // If the background resource changed, the percentage here also need to
        // be change.
        int circleHeight = (int) (bgHeight * 0.71);

        Canvas canvas = new Canvas();
        Bitmap compoundBmp = Bitmap.createBitmap(bgWidth, bgHeight, Bitmap.Config.ARGB_8888);
        canvas.setBitmap(compoundBmp);
        mBackground.setBounds(0, 0, bgWidth, bgHeight);
        mBackground.draw(canvas); // draw background

        int x = (bgWidth - textWidth) / 2;

        // Why we add circleHeight by 1 pixel?
        // As we can see from the background bitmap resource file, there has 1 pixel
        // between the top of the red circle and the top of the background bitmap.
        // With xhdpi resource bitmap, it is 2 pixel, but we can not detect which
        // resource be used easily, so we always plus 1 pixel when calculate the
        // y position to make the number be drawn in the center of the red circle.
        //
        // It is the background bitmap cause this problem for the red circle not
        // in the center of the background. The guy who design the background resource
        // bitmap is to be blame.
        int y = (circleHeight + 1 ) - (circleHeight - textHeight) / 2;

        canvas.drawText(finalText, x, y, paint);

        Bitmap finalBitmap = origin.copy(Bitmap.Config.ARGB_8888, true);
        canvas.setBitmap(finalBitmap);
        canvas.drawBitmap(compoundBmp, 0, 0, null);

        compoundBmp.recycle();
        return finalBitmap;
    }

    public void resetComponentsUnreadInfoChangedValue() {
        mUnreadInfoChangedCache.clear();
    }

    @Override
    public void run() {
        Log.d(TAG, "run----");
        // TODO Auto-generated method stub
        mIconCache = mAppState.getIconCache();
        mUser = UserHandleCompat.myUserHandle();

        Intent intent = new Intent();
        intent.setComponent(sMmsComponentName);
        Bitmap originIcon = mIconCache.getIcon(intent, mUser);
        mUnreadInfoCache.put(sMmsComponentName, new UnreadInfo(-1, originIcon, null));

        intent.setComponent(sDialerComponentName);
        originIcon = mIconCache.getIcon(intent, mUser);
        mUnreadInfoCache.put(sDialerComponentName, new UnreadInfo(-1, originIcon, null));
    }

    private class UnreadInfo {
        int mUnreadInfoNum;
        Bitmap mOriginIcon;
        Bitmap mIconWithNum;

        public UnreadInfo(int num, Bitmap origin, Bitmap withNum) {
            mUnreadInfoNum = num;
            mOriginIcon = origin;
            mIconWithNum = withNum;
        }
    }

    public void updateUnreadInfo(Workspace wp) {
        ArrayList<ShortcutAndWidgetContainer> childrenLayouts = wp.getAllShortcutAndWidgetContainers();
        for (ShortcutAndWidgetContainer container: childrenLayouts) {
            int count = container.getChildCount();
            for (int i = 0; i < count; i++) {
                View childV = container.getChildAt(i);
                if (childV instanceof BubbleTextView) {
                    //LauncherAppState.getInstance().getUnreadInfoManager()
                        //.updateBubbleTextViewUnreadInfo((BubbleTextView) childV);
                    updateBubbleTextViewUnreadInfo((BubbleTextView) childV);
                }
                if (childV instanceof FolderIcon) {
                    boolean updated = false;
                    FolderIcon folderIcon = (FolderIcon) childV;
                    Folder folder = folderIcon.getFolder();
                    if (folder == null) {
                        continue;
                    }
                    ArrayList<View> bubbles = folder.getItemsInReadingOrder();
                    for (View bubble : bubbles) {
                        if (bubble instanceof BubbleTextView) {
                            /*if (LauncherAppState.getInstance().getUnreadInfoManager()
                                    .updateBubbleTextViewUnreadInfo((BubbleTextView) bubble)) {*/
                            if(updateBubbleTextViewUnreadInfo((BubbleTextView) bubble)) {
                                updated = true;
                            }
                        }
                    }
                    if (updated) {
                        folderIcon.invalidate();
                    }
                }
            }
        }
    }

    public void updateUnreadInfo(Launcher mLauncher,AlphabeticalAppsList mApps,AllAppsContainerView mAllAppsContainerView) {
        List<AppInfo> apps = new ArrayList<AppInfo>();
        for (int i = 0; i < mApps.getSize(); i++) {
            AppInfo app = (AppInfo)mApps.getApps().get(i);
            BubbleTextView bubble = new BubbleTextView(mLauncher);
            bubble.setTag(app);
            if (bubble instanceof BubbleTextView) {
                /*LauncherAppState.getInstance().getUnreadInfoManager()
                        .updateBubbleTextViewUnreadInfo((BubbleTextView) bubble);*/
                updateBubbleTextViewUnreadInfo((BubbleTextView) bubble);
            }
            apps.add(app);
        }
        mAllAppsContainerView.updateApps(apps);
    }
}
