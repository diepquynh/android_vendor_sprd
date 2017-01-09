package plugin.sprd.clearalltask;

import java.lang.reflect.InvocationTargetException;
import java.util.List;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.AddonManager;
import android.content.Context;
import android.content.pm.PackageManager;

import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import com.android.systemui.SystemUIApplication;

import com.android.systemui.recents.RecentsActivity;
import com.android.systemui.recents.RecentsActivityUtils;
import com.android.systemui.recents.views.RecentsView;
import com.android.systemui.recents.views.TaskStackView;
import com.android.systemui.recents.views.TaskView;
import com.android.systemui.statusbar.phone.PhoneStatusBar;
import com.android.systemui.R;

public class RecentsActivityUtilsAddon extends RecentsActivityUtils implements
        AddonManager.InitialCallback {

    public static final String LOG_TAG = "RecentsActivityUtilsAddon";
    private Context mAddonContext;
    private Activity mActivity;
    private PhoneStatusBar mStatusBar;
    private ActivityManager am;
    private PackageManager pm;
    private RecentsView mRecentsView;
    private ImageButton mClearButton;
    private final static String METHOD_TOGGLE_RECENTS = "toggleRecent";
    private final static String METHOD_DISMISS_TASK = "dismissAllTask";

    public RecentsActivityUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    @Override
    public boolean isSupportClearAllTasks() {
        return true;
    }

    public void init(Context context,RecentsView recentsview, ImageButton imagebutton) {
        mAddonContext = context;
        mActivity = (Activity) mAddonContext;
        mStatusBar = ((SystemUIApplication) mActivity.getApplication())
                .getComponent(PhoneStatusBar.class);
        am = (ActivityManager) mActivity
                .getSystemService(Context.ACTIVITY_SERVICE);
        pm = (PackageManager) mActivity.getPackageManager();
        mRecentsView = recentsview;
        mClearButton = imagebutton;
        mClearButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        removeAllTasks();
                    }
                }).start();
            }
        });
    }

    Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (mRecentsView == null) {
                return;
            }
            int childCount = mRecentsView.getChildCount();
            if (childCount == 0)
                if (mStatusBar != null) {
                    toggleRecents();
                } else {
                    mActivity.finish();
                }
            for (int i = 0; i < childCount; i++) {
                View tsv = mRecentsView.getChildAt(i);
                if (!(tsv instanceof TaskStackView))
                    continue;
                int count = ((TaskStackView) tsv).getChildCount();
                if (count == 0)
                    continue;
                for (int j = 0; j < count; j++) {
                    View tv = ((TaskStackView) tsv).getChildAt(j);
                    if (!(tv instanceof TaskView))
                        continue;
                    dismissAllTask((TaskView) tv);
                }
            }
            if (mStatusBar != null) {
                //toggleRecents();
            } else {
                mActivity.finish();
            }
        }
    };

    public void toggleRecents() {
        try {
            mStatusBar.getClass().getMethod(METHOD_TOGGLE_RECENTS)
                .invoke(mStatusBar);
        } catch (NoSuchMethodException ex) {
            Log.i(LOG_TAG, "executeInit NoSuchMethodException " + ex);
        } catch (IllegalArgumentException ex) {
            Log.i(LOG_TAG, "executeInit IllegalArgumentException  " + ex);
        } catch (IllegalAccessException ex) {
            Log.i(LOG_TAG, "executeInit IllegalAccessException  " + ex);
        } catch (InvocationTargetException ex) {
            Log.i(LOG_TAG, "executeInit InvocationTargetException  " + ex);
        }
    }

    public void dismissAllTask(TaskView taskview) {
        try {
            taskview.getClass().getMethod(METHOD_DISMISS_TASK)
                .invoke(taskview);
        } catch (NoSuchMethodException ex) {
            Log.i(LOG_TAG, "executeInit NoSuchMethodException " + ex);
        } catch (IllegalArgumentException ex) {
            Log.i(LOG_TAG, "executeInit IllegalArgumentException  " + ex);
        } catch (IllegalAccessException ex) {
            Log.i(LOG_TAG, "executeInit IllegalAccessException  " + ex);
        } catch (InvocationTargetException ex) {
            Log.i(LOG_TAG, "executeInit InvocationTargetException  " + ex);
        }
    }

    private static final int MAX_TASKS = 100;

    private void removeAllTasks() {
        final List<ActivityManager.RecentTaskInfo> recentTasks = am
                .getRecentTasks(MAX_TASKS,ActivityManager.RECENT_IGNORE_HOME_STACK_TASKS);
        for (int i = 0; i < recentTasks.size(); ++i) {
            final ActivityManager.RecentTaskInfo recentInfo = recentTasks
                    .get(i);
            android.util.Log.w(LOG_TAG, "recentInfo:"
                    + recentInfo.baseIntent.getComponent().getPackageName());
            am.removeTask(recentInfo.persistentId);
        }
        mHandler.sendEmptyMessage(0);
    }
}
