package plugin.sprd.protectedapp.receiver;

import java.util.ArrayList;
import java.util.List;
import java.util.zip.Inflater;

import plugin.sprd.protectedapp.MainActivity;
import plugin.sprd.protectedapp.R;
import plugin.sprd.protectedapp.db.DatabaseUtil;
import plugin.sprd.protectedapp.model.AppInfoModel;


import android.R.integer;
import android.annotation.SuppressLint;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningAppProcessInfo;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Debug;
import android.os.Debug.MemoryInfo;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Gravity;
import android.view.InflateException;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.FrameLayout;
import android.widget.TextView;

public class SecurityUnlockReceiver extends BroadcastReceiver {
    private static final String TAG = SecurityUnlockReceiver.class.getSimpleName();
    private static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);

    private static final String ACTION_USER_PRESENT = "android.intent.action.USER_PRESENT";
    private static final String SECURITY_KEY_NOTIFICATION = "security_key_notification";
    private static final String SECURIYT_SP = "securiyt_sp";
    private static final int MAX_TASKS = 100;
    private final long BYTES_IN_KILOBYTE = 1024;

    private static final int DIALOG_DEFAULT = -1;
    private static final int DIALOG_DISABLE = 0;
    private static final int DIALOG_ENABLE = 1;

    private ActivityManager mActivityManager;
    private PackageManager mPackageManager;
    private Context mContext;
    private DatabaseUtil mSecurityClearDatabaseUtil;
    private static Dialog dialog = null;

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (DEBUG)
            Log.d(TAG, "Receive action " + action);
        if (ACTION_USER_PRESENT.equals(action)) {
            mContext = context;
            mSecurityClearDatabaseUtil = new DatabaseUtil(mContext);
            mActivityManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
            mPackageManager = context.getPackageManager();
            new SecurityThread().start();
        }
    }

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (DEBUG)
                Log.d(TAG, "handleMessage");
            float[] obj = (float[]) msg.obj;
            showDialog(obj);
        }
    };

    private float[] removeAllTasks(Context context, ArrayList<AppInfoModel> securityList) {
        if (DEBUG)
            Log.d(TAG, "removeAllTasks ");
        if (securityList == null) {
            return null;
        }
        float countApp = 0;
        float countMemory = 0;
        ActivityManager activityManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        List<RunningAppProcessInfo> appProcesses = activityManager.getRunningAppProcesses();
        for (AppInfoModel model : securityList) {
            String pkgName = model.getPackageName();
            for (RunningAppProcessInfo appProcess : appProcesses) {
                if (pkgName.equals(appProcess.processName)
                        && appProcess.importance != RunningAppProcessInfo.IMPORTANCE_FOREGROUND) {
                    countApp++;
                    countMemory += getRunningAppProcessInfo(pkgName);
                    mActivityManager.killBackgroundProcesses(pkgName);
                }
            }
        }
        return new float[] { countApp, countMemory };
    }

    private float getRunningAppProcessInfo(String pkgName) {
        List<ActivityManager.RunningAppProcessInfo> appProcessList = mActivityManager.getRunningAppProcesses();
        float memSize = 0;
        for (ActivityManager.RunningAppProcessInfo appProcessInfo : appProcessList) {
            int pid = appProcessInfo.pid;
            int uid = appProcessInfo.uid;
            String processName = appProcessInfo.processName;
            if (pkgName.equalsIgnoreCase(processName)) {
                int[] myMempid = new int[] { pid };
                Debug.MemoryInfo[] memoryInfo = mActivityManager.getProcessMemoryInfo(myMempid);
                memSize = memoryInfo[0].dalvikPrivateDirty / BYTES_IN_KILOBYTE;
            }
        }
        return memSize;
    }

    private class SecurityThread extends Thread {
        @Override
        public void run() {
            if (DEBUG)
                Log.d(TAG, "SecurityThread run");
            ArrayList<AppInfoModel> securityList = mSecurityClearDatabaseUtil.queryEnableList();
            if (securityList == null || securityList.size() == 0) {
                return;
            }
            float[] obj = removeAllTasks(mContext, securityList);
            Log.d(TAG, "getDefaultNotification: " + getDefaultNotification());
            if (getDefaultNotification() < DIALOG_ENABLE && obj[0] > 0) {
                if (DEBUG)
                    Log.d(TAG, "SecurityThread run sendMessage : " );
                Message msg = new Message();
                msg.obj = obj;
                mHandler.sendMessage(msg);
            }
        }
    }

    private void showDialog(float[] obj) {
        if (DEBUG)
            Log.d(TAG, "showDialog");
        try {
            if (dialog != null && dialog.isShowing()) {
                dialog.dismiss();
            }
            dialog = new AlertDialog.Builder(mContext, R.style.security_dialog).create();
            Window window = dialog.getWindow();
            LayoutParams params = new LayoutParams();
            params.gravity = Gravity.BOTTOM;
            window.setAttributes(params);
            window.setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
            dialog.setCancelable(true);
            dialog.setCanceledOnTouchOutside(true);
            dialog.show();
            dialog.getWindow().setContentView(R.layout.dialog_app);
            TextView textView = (TextView) dialog.findViewById(R.id.clear_dialog_btn);
            textView.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (dialog.isShowing()) {
                        dialog.dismiss();
                    }
                }
            });
            FrameLayout frameLayout = (FrameLayout)dialog.findViewById(R.id.framelayout_arrow);
            frameLayout.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if (dialog != null && dialog.isShowing()) {
                        dialog.dismiss();
                    }
                    Intent intent = new Intent(mContext,MainActivity.class);
                    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    mContext.startActivity(intent);
                }
            });
            TextView textViewTips = (TextView) dialog.findViewById(R.id.clear_tips);
            String info = mContext.getResources().getString(R.string.clear_tips, String.valueOf((int) obj[0]),
                    String.valueOf(obj[1]));
            textViewTips.setText(info);
            CheckBox dlgCheckBox = (CheckBox) dialog.findViewById(R.id.clear_dlg_checkbox);
            dlgCheckBox.setChecked(getDefaultNotification() == DIALOG_ENABLE);
            dlgCheckBox.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton compoundbutton, boolean flag) {
                    setDefaultNotification(flag);
                }
            });
        } catch (Exception e) {
            Log.e(TAG, "Show dialog error : ", e);
        }
    }

    private int getDefaultNotification() {
        if (DEBUG)
            Log.d(TAG, "get Default Notification");
        SharedPreferences prefs = mContext.getSharedPreferences(SECURIYT_SP, Context.MODE_PRIVATE);
        return prefs.getInt(SECURITY_KEY_NOTIFICATION, DIALOG_DEFAULT);
    }

    private void setDefaultNotification(Boolean isNofication) {
        if (DEBUG)
            Log.d(TAG, "set Default Notification" + isNofication);
        SharedPreferences prefs = mContext.getSharedPreferences(SECURIYT_SP, Context.MODE_PRIVATE);
        prefs.edit().putInt(SECURITY_KEY_NOTIFICATION, isNofication ? DIALOG_ENABLE : DIALOG_DISABLE).commit();
    }

    private boolean isBackground(Context context, String pkgName) {
        if (pkgName == null) {
            return false;
        }
        ActivityManager activityManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        List<RunningAppProcessInfo> appProcesses = activityManager.getRunningAppProcesses();
        for (RunningAppProcessInfo appProcess : appProcesses) {
            if (pkgName.equals(appProcess.processName)) {
                if (appProcess.importance != RunningAppProcessInfo.IMPORTANCE_FOREGROUND) {
                    return true;
                }
            }
        }
        return false;
    }
}