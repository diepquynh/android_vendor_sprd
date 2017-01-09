package com.android.server.am;

import android.app.ActivityManager;
import android.app.ActivityManagerNative;
import android.app.AppGlobals;
import android.app.INotificationManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.AppAs;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.BitmapFactory;
import android.os.FileUtils;
import android.os.Handler;

import android.os.RemoteException;
import android.os.UserHandle;
import android.os.Binder;
import android.provider.Settings;
import android.net.Uri;
import android.util.Slog;
import android.util.Xml;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.RemoteViews;
import com.android.internal.content.PackageMonitor;
import com.android.internal.util.FastXmlSerializer;
import com.android.internal.R;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Iterator;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public class AppAsLaunchStatus {
    private static final String TAG = "AppAsLaunchStatus";
    private static final String APPASLAUNCHRECEIVER= "android.settings.APPASLAUNCHRECEIVER";
    private static final String ENABLE_TAG = "enable";
    private static final String PACKAGE_NAME_TAG = "packagename";
    private static final String LABEL_NAME_TAG = "labelName";
    private static final String NOTIFICATION_ID_TAG = "notiId";
    private final static String APP_AS_LAUNCH_ENABLE = "appaslaunch_enable_";

    HashMap<String, AppAs.AppAsData> mAppAsLaunchMap = new HashMap<String, AppAs.AppAsData>();
    ArrayList<AppAs.AppAsRecord> mAppAsLaunchRecord = new ArrayList<AppAs.AppAsRecord>();
    Context mContext;
    Handler mHandler;
    Handler mUiHandler;
    private ActivityManagerService mService;
    // id for different app notification
    private int notificationId = 278;
    // add notified apps into mAALNotiList
    private ArrayList<String> mAALNotiList = new ArrayList<String>();

    /* SPRD:modify for Bug 653843 associated start interface shows uninstalling app infos. @{ */
    private myPackageMonitor mPackageMonitor = null;
    private class myPackageMonitor extends PackageMonitor {
        @Override
        public void onPackagesUnavailable(String[] packageName) {
            for (String name: packageName) {
                Slog.d(TAG, "onPackagesUnavailable name = "+name);
                removeAppAsRecordByName(name);
                removeAppAsLaunchMapByName(name);
            }
            scheduleUpdateAppAsFile();
        }

        @Override
        public void onPackageRemoved(String packageName, int uid) {
            Slog.d(TAG, "onPackageRemoved packageName = " + packageName
                    + ", uid = "+uid);
            removeAppAsRecordByName(packageName);
            removeAppAsLaunchMapByName(packageName);
            scheduleUpdateAppAsFile();
            }
    };
    /* @} */

    AppAsLaunchStatus(Context context, Handler handler, Handler uiHandler){
        mContext = context;
        mHandler = handler;
        mUiHandler = uiHandler;
        mService = (ActivityManagerService) ActivityManagerNative.getDefault();
    }

    public List<AppAs.AppAsData> getAppAsLauncherList(String packageName){
        if(packageName.isEmpty()) {
            Slog.e(TAG,"packageName is null");
            return null;
        }

        if (mAppAsLaunchMap == null) {
            Slog.e(TAG,"mAppAsLaunchMap is null");
            return null;
        }
        AppAs.AppAsData m = null;
        synchronized (mAppAsLaunchMap) {
            m = mAppAsLaunchMap.get(packageName);
        }
        if (m == null) {
            return null;
        }

        ArrayList<AppAs.AppAsData> mList = m.getAllList();
        return mList;
    }

    public List<AppAs.AppAsData> getAppAsList(){
        if (mAppAsLaunchMap == null) {
            Slog.e(TAG,"mAppAsLaunchMap is null");
            return null;
        }

        ArrayList<AppAs.AppAsData> mList = new ArrayList<AppAs.AppAsData>();
        synchronized (mAppAsLaunchMap) {
            Iterator iter = mAppAsLaunchMap.entrySet().iterator();
            while(iter.hasNext()) {
                Map.Entry entry = (Map.Entry)iter.next();
                AppAs.AppAsData key = (AppAs.AppAsData)entry.getValue();
                mList.add(key);
            }
        }
        return mList;
    }

    public void setAppAsEnable(int flag, String packageName, String basePackage){
        synchronized (mAppAsLaunchMap) {
            if (mAppAsLaunchMap == null) {
                Slog.e(TAG,"mAppAsLaunchMap is null");
                return ;
            }

            if (packageName == null){
                return ;
            }
            if (basePackage == null) {
                basePackage = packageName;
            }
            AppAs.AppAsData m = mAppAsLaunchMap.get(basePackage);
            if (m == null) {
                return ;
            }
            ArrayList<AppAs.AppAsData> mList = m.getAllList();

            if (basePackage.equals(packageName)) {
                m.setEnabled((flag == 1)?true:false);
                for(AppAs.AppAsData app:mList) {
                    app.setEnabled((flag == 1)?true:false);
                }
            } else {
                /* SPRD:modify for Bug 653875 AS switches confused with item AS switches open or close. @{ */
                // isAllCallingEnabled is enabled count for mList apps.
                int isAllCallingEnabled = 0;
                for(AppAs.AppAsData app:mList) {
                    if(packageName.equals(app.getPackage())) {
                        app.setEnabled((flag == 1)?true:false);
                        //break;
                    }
                    if (app.getEnabled()) {
                        isAllCallingEnabled++;
                    }
                }
                // set basePackage app enable or disable with isAllCallingEnabled != 0.
                m.setEnabled(isAllCallingEnabled != 0);
                /* @} */
            }

            // if disabled basePackage app first time, save data into settings_system.xml.
            // keep this data for send notification only once with never disabled.
            if (!m.getEnabled()) {
                int packageNotiId = Settings.System.getInt(
                        mContext.getContentResolver(),
                        APP_AS_LAUNCH_ENABLE + basePackage, -1);
                if (packageNotiId == -1) {
                    Settings.System.putInt(mContext.getContentResolver(),
                            APP_AS_LAUNCH_ENABLE + packageName, m.getEnabled() ? 1 : 0);
                }
            }

            Slog.d(TAG,"setappasenable flag = " + flag + ", packageName = "
                    + packageName + ", basePackage = " + basePackage);
            scheduleUpdateAppAsFile();
            return ;
        }
    }

    public int getAppAsStatus(String packageName, String basePackage){
        if (mAppAsLaunchMap == null) {
            Slog.e(TAG,"mAppAsLaunchMap is null");
            return 1;
        }

        AppAs.AppAsData m = mAppAsLaunchMap.get(basePackage);
        if (m == null) {
            return 1;
        }
        ArrayList<AppAs.AppAsData> mList = m.getAllList();
        if (basePackage.equals(packageName)) {
            return m.getEnabled()?1:0;
        } else {
            for(AppAs.AppAsData app:mList) {
                if(packageName.equals(app.getPackage())) {
                    return app.getEnabled()?1:0;
                }
            }
        }
        return 1;
    }

    private File getUserAppAsStateFile(int userId) {
        // TODO: Implement a cleaner solution when adding tests.
        // This instead of Environment.getUserSystemDirectory(userId) to support testing.
        File userDir = new File(new File("/data/system", "users"), Integer.toString(userId));
        return new File(userDir, "AppAsStatus.xml");
    }
    private static final String TAG_ASLAUNCH = "aslaunch";
    private static final String ATTR_BASEPKG = "basepkg";
    private static final String TAG_PKG = "package";
    private static final String TAG_ASLAUNCHPKG = "callingpkg";
    public void writeAppAsData(int userId){
        synchronized (mAppAsLaunchMap) {
            File userPackagesStateFile = getUserAppAsStateFile(userId);
            new File(userPackagesStateFile.getParent()).mkdirs();
            if (userPackagesStateFile.exists()) {
                userPackagesStateFile.delete();
                Slog.d(TAG, "Preserving older app as file");
            }
            BufferedOutputStream str = null;
            try {
                final FileOutputStream fstr = new FileOutputStream(userPackagesStateFile);
                str = new BufferedOutputStream(fstr);

                final XmlSerializer serializer = new FastXmlSerializer();
                serializer.setOutput(str, StandardCharsets.UTF_8.name());
                serializer.startDocument(null, true);
                serializer.setFeature("http://xmlpull.org/v1/doc/features.html#indent-output", true);
                serializer.startTag(null, TAG_ASLAUNCH);

                Iterator iter = mAppAsLaunchMap.entrySet().iterator();
                while(iter.hasNext()) {
                    Map.Entry entry = (Map.Entry)iter.next();
                    String key = (String)entry.getKey();
                    AppAs.AppAsData val = (AppAs.AppAsData)entry.getValue();
                    serializer.startTag(null, TAG_PKG);
                    val.saveToXml(serializer);
                    ArrayList<AppAs.AppAsData> mList = val.getAllList();
                    for(AppAs.AppAsData app:mList) {
                        serializer.startTag(null, TAG_ASLAUNCHPKG);
                        app.saveToXml(serializer);
                        serializer.endTag(null, TAG_ASLAUNCHPKG);
                    }
                    serializer.endTag(null, TAG_PKG);
                }
                serializer.endTag(null, TAG_ASLAUNCH);
                serializer.endDocument();
                serializer.flush();
                FileUtils.sync(fstr);
                str.close();
                FileUtils.setPermissions(userPackagesStateFile.toString(),
                    FileUtils.S_IRUSR|FileUtils.S_IWUSR
                    |FileUtils.S_IRGRP|FileUtils.S_IWGRP,
                    -1, -1);
            } catch(IOException e1){
                Slog.e(TAG,"write file get IOException");
            } catch (XmlPullParserException e2) {
                Slog.e(TAG,"write file get XmlPullParserException");
            } finally {
                if (str != null) {
                    try {
                        str.close();
                    }catch(IOException e) {
                    }
                }
            }
        }
    }

    void readAppAsState(int userId) {
        synchronized (mAppAsLaunchMap) {
            FileInputStream str = null;
            File userPackagesStateFile = getUserAppAsStateFile(userId);
            if (!userPackagesStateFile.exists()) {
                Slog.e(TAG,"no app as file");
                return;
            }
            try {
                str = new FileInputStream(userPackagesStateFile);
                final XmlPullParser parser = Xml.newPullParser();
                parser.setInput(str, StandardCharsets.UTF_8.name());
                int event;
                AppAs.AppAsData base = null;
                while (((event = parser.next()) != XmlPullParser.END_DOCUMENT)) {
                    final String name = parser.getName();
                    boolean startPackage = true;
                    if (event == XmlPullParser.START_TAG) {
                        if (TAG_PKG.equals(name)) {
                            base = new AppAs.AppAsData();
                            base = base.restoreFromXml(parser);
                        } else if(TAG_ASLAUNCHPKG.equals(name)) {
                            AppAs.AppAsData app = new AppAs.AppAsData();
                            app = app.restoreFromXml(parser);
                            base.addList(app);
                        }
                    }

                    if (event == XmlPullParser.END_TAG) {
                        if (TAG_PKG.equals(name)) {
                            if (base != null) {
                                mAppAsLaunchMap.put(base.getPackage(), base);
                                base = null;
                            }
                        }
                    }
                }
                str.close();
            } catch(IOException e1){
                Slog.e(TAG,"read file get IOException");
            } catch (XmlPullParserException e2) {
                Slog.e(TAG,"read file get XmlPullParserException");
            } finally {
                if (str != null) {
                    try{
                        str.close();
                    }catch(IOException e) {
                    }
                }
            }
        }
    }
    public void updateAppAsRecord(String base, String calling, boolean flag) {
        synchronized(mAppAsLaunchRecord){
            boolean exist = false;
            for(AppAs.AppAsRecord app:mAppAsLaunchRecord) {
                if(calling.equals(app.getCallingPackage()) && base.equals(app.getBasePackage()) && (!(flag^app.getEnabled()))) {
                    app.increaseCount();
                    mAppAsLaunchRecord.remove(app);
                    mAppAsLaunchRecord.add(0,app);
                    exist = true;
                    break;
                }
            }
            if (!exist) {
                AppAs.AppAsRecord m = new AppAs.AppAsRecord(base, calling, flag);
                m.increaseCount();
                mAppAsLaunchRecord.add(0,m);
            }
        }
    }
    public List<AppAs.AppAsRecord> getAppAsRecordList() {
        return mAppAsLaunchRecord;
    }
    private boolean startingActivityByHomeApp (String base) {
        if(mService.mHomeProcess != null && mService.mHomeProcess.info != null
            && mService.mHomeProcess.info.packageName != null
            && mService.mHomeProcess.info.packageName.equals(base)) {
            return true;
        }
        return false;
    }
    public boolean addListAndJudgeAllowLocked(String base, String calling, String reason){
        /* SPRD:modify for Bug 653843 associated start interface shows uninstalling app infos. @{ */
        if (mPackageMonitor == null) {
            mPackageMonitor = new myPackageMonitor();
            mPackageMonitor.register(mContext, null, true);
        }
        /* @} */

        boolean allow = true;
        try {
            allow = addListAndJudgeAllowInnerLocked(base, calling, reason);
        } catch (Exception e) {}
        return allow;
    }
    private boolean addListAndJudgeAllowInnerLocked(String base, String calling, String reason){
        if (base == null || calling == null) {
            return true;
        }

        Slog.d(TAG,"Start Proc : "+base+", callingPackage = "+calling+", reason = "+reason);
        if( reason!= null && reason.equals("start-activity") && startingActivityByHomeApp(base)) {
            return true;
        }
        String label = "";
        String callingLabel = "";
        ApplicationInfo mAppInfo = null;
        ApplicationInfo mCallingAppInfo = null;
        long origId = 0;
        try{
            origId = Binder.clearCallingIdentity();
            PackageManager pm = mContext.getPackageManager();
            mAppInfo = pm.getApplicationInfo(base, 0);
            mCallingAppInfo = pm.getApplicationInfo(calling, 0);
            if ((mAppInfo.flags&ApplicationInfo.FLAG_SYSTEM) != 0 || (mCallingAppInfo.flags&ApplicationInfo.FLAG_SYSTEM)!=0) {
                return true;
            } else if (!base.equals(calling) && !base.contains(mContext.getResources().getText(
                     com.android.internal.R.string.special_pkg))) {
                label = mAppInfo.loadLabel(pm).toString();
                callingLabel = mCallingAppInfo.loadLabel(pm).toString();
            } else {
                return true;
            }
        } catch(PackageManager.NameNotFoundException e){
            Slog.e(TAG,"package name not found");
            return true;
        } finally {
            Binder.restoreCallingIdentity(origId);
        }
        synchronized (mAppAsLaunchMap) {
            //add app as launch
            AppAs.AppAsData m = mAppAsLaunchMap.get(base);
            boolean exist = false;
            int index = 0;
            if (m == null) {
                m = new AppAs.AppAsData(base,true);
            }
            ArrayList<AppAs.AppAsData> mList = m.getAllList();
            if (mList != null) {
                for(AppAs.AppAsData app:mList) {
                    if(calling.equals(app.getPackage())) {
                        exist = true;
                        break;
                    }
                    index++;
                }
            }
            m.increaseCount();
            m.setLastTickTime(System.currentTimeMillis());
            if(!exist) {
                AppAs.AppAsData mAppAsData = new AppAs.AppAsData(calling,true,1);
                m.addList(mAppAsData);
                m.setEnabled(true);
                mAppAsLaunchMap.put(base, m);
                index = m.getListSize() - 1;
            } else {
                m.getListItem(index).increaseCount();
                m.getListItem(index).setLastTickTime(System.currentTimeMillis());
            }
            scheduleUpdateAppAsFile();

            boolean enable = m.getEnabled() && m.getListItem(index).getEnabled();
            updateAppAsRecord(base, calling, enable);

            // get this app whether disabled before.
            int packageNotiId = Settings.System.getInt(
                    mContext.getContentResolver(),
                    APP_AS_LAUNCH_ENABLE + m.getPackage(), -1);

            // if app as never enabled or disabled by person, and app as is enabled,
            // and never showed notification before after boot, then send nofitications.
            if ((packageNotiId == -1)&& enable
                    && !mAALNotiList.contains(m.getPackage())) {
                sendNotification(m.getPackage(), label, callingLabel, enable);
            }
            return enable;
        }
    }

    private void scheduleUpdateAppAsFile() {
        mHandler.post(new Runnable() {
            public void run() {
                writeAppAsData(UserHandle.myUserId());
            }
        });
    }
    void sendNotification(String labelPackageName, String label,
             String callingLabel, boolean enable) {
        mHandler.post(new Runnable() {
            public void run() {
                sendNotificationCustom(labelPackageName, label,
                        callingLabel, enable);
            }
        });
    }

    // SPRD: add remoteviews for notification.
    // add image like checkbox style to control AS enable or disable quiklly.
    private void sendNotificationCustom(String labelPackageName,
              String label, String callingLabel, boolean enable) {
        INotificationManager inm = NotificationManager.getService();
        if (inm == null) {
            Slog.w(TAG, "NotificationManager is null.");
            return;
        }

        // intent for go to associate start page.
        Intent intent = new Intent();
        intent.setAction("LAUNCH_APP_AS_LIST");
        PendingIntent contentIntent = PendingIntent.getActivity(
                mContext, notificationId,intent, Notification.FLAG_AUTO_CANCEL);

        // description of this notification
        String title = mContext.getString(R.string.app_as_launch);
        String text = mContext.getString(R.string.app_as_launch_warning,
                enable ? mContext.getString(R.string.app_as_launch_allow)
                : mContext.getString(R.string.app_as_launch_forbidden),
                label, callingLabel);
        String notification_summery = mContext.getString(
                R.string.notification_summery,
                enable ? mContext.getString(R.string.notification_close)
                : mContext.getString(R.string.notification_open));

        // custom remote view for notification
        RemoteViews remoteViews = new RemoteViews(mContext.getPackageName(),
                R.layout.layout_custom_notifycation);
        remoteViews.setImageViewResource(R.id.noti_icon,
                R.drawable.ic_settings_appaslaunch);
        remoteViews.setImageViewResource(R.id.btn_app_as_launch,
                enable ? R.drawable.appas_btn_check_on
                : R.drawable.appas_btn_check_off);
        remoteViews.setTextViewText(R.id.noti_title, title);
        remoteViews.setTextViewText(R.id.noti_description, text);
        remoteViews.setTextViewText(R.id.noti_hint, notification_summery);


        // send broadcast to set app as launch enable/disable quicklly for right large icon.
        Intent imageintent = new Intent();
        imageintent.setAction(APPASLAUNCHRECEIVER);
        imageintent.putExtra(ENABLE_TAG, !enable);
        imageintent.putExtra(PACKAGE_NAME_TAG, labelPackageName);
        imageintent.putExtra(LABEL_NAME_TAG, label);
        imageintent.putExtra(NOTIFICATION_ID_TAG, notificationId);
        PendingIntent imagePendingIntent = PendingIntent
                .getBroadcast(mContext, notificationId, imageintent, 0);
        remoteViews.setOnClickPendingIntent(
                R.id.btn_app_as_launch, imagePendingIntent);

        // build notification.
        Notification.Builder builder = new Notification.Builder(mContext);
        builder.setSmallIcon(R.drawable.ic_settings_appaslaunch);
        builder.setContent(remoteViews);
        builder.setContentIntent(contentIntent);

        try {
            // send notification.
            int[] outId = new int[1];
            inm.enqueueNotificationWithTag("android", "android", null,
                notificationId,
                builder.build(), outId, UserHandle.myUserId());
            // add labelPackageName into mAALNotiList for keeping this app notification shows once after boot.
            mAALNotiList.add(labelPackageName);
            notificationId++;
        } catch (RuntimeException e) {
            Slog.w(TAG,
                "Error showing notification for heavy-weight app", e);
        } catch (RemoteException e) {
        }

    }

    /* SPRD:modify for Bug 653843 associated start interface shows uninstalling app infos. @{ */
    private void removeAppAsLaunchMapByName(String name) {
        synchronized (mAppAsLaunchMap) {
            mAppAsLaunchMap.remove(name);

            Iterator iter = mAppAsLaunchMap.entrySet().iterator();
            ArrayList<String> mRmList = new ArrayList<String>();
            while(iter.hasNext()) {
                Map.Entry entry = (Map.Entry)iter.next();
                AppAs.AppAsData key = (AppAs.AppAsData)entry.getValue();
                String pkgName = key.getPackage();

                // mRmList for removing base key if mList is empty.
                for (AppAs.AppAsData item: key.getAllList()) {
                    String itemName = item.getPackage();
                    if (name.equals(itemName)) {
                        key.getAllList().remove(item);
                        Slog.e(TAG,"mAppAsLaunchMap remove mRmItemList pkgName = "+item.getPackage());
                        if (key.getAllList().size() == 0) {
                            mRmList.add(pkgName);
                        } else {
                            int count = 0;
                            for (AppAs.AppAsData data: key.getAllList()) {
                                if (data.getEnabled()) {
                                    count ++;
                                }
                            }
                            key.setEnabled(count != 0);
                        }
                        break;
                    }
                }
            }

            // remove base key
            for (String pkgName: mRmList) {
                mAppAsLaunchMap.remove(pkgName);
            }
        }
    }

    private void removeAppAsRecordByName(String name) {
        synchronized(mAppAsLaunchRecord){
            ArrayList<AppAs.AppAsRecord> mRmRecord = new ArrayList<AppAs.AppAsRecord>();
            for(AppAs.AppAsRecord records: mAppAsLaunchRecord) {
                if (name.equals(records.getCallingPackage())
                        || name.equals(records.getBasePackage())) {
                    mRmRecord.add(records);
                }
            }
            for(AppAs.AppAsRecord app : mRmRecord) {
                mAppAsLaunchRecord.remove(app);
            }
        }
    }
    /* @} */
}
