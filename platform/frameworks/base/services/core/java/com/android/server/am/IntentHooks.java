/*
 *Copyright © 2016 Spreadtrum Communications Inc.
 */

package com.android.server.am;

import android.app.ActivityThread;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.text.TextUtils;
import android.util.Slog;

import java.util.ArrayList;
import java.util.List;
import static com.android.server.am.ActivityManagerDebugConfig.DEBUG_ALL;

public class IntentHooks {

    public static final int COMPONENT_TYPE_ACTIVITY = 1;
    public static final int COMPONENT_TYPE_SERVICE = 2;
    public static final int COMPONENT_TYPE_BROADCAST = 3;
    public static final int COMPONENT_TYPE_PROVIDER = 4;

    private static final int CONFIG_TYPE_ACTION = 5;
    private static final int CONFIG_TYPE_COMPONENT = 6;

    private static final String TAG = "IntentHooks";
    private static IntentHooks sInstance;
    private PackageManager mPackageManager;
    private List<Hook> mHooks = new ArrayList<Hook>();

    public static IntentHooks getInstance() {
        if (sInstance == null) {
            if(DEBUG_ALL) Slog.d(TAG, "sInstance == null prepare sInstance");
            synchronized (IntentHooks.class) {
                prepare(ActivityThread.currentApplication());
            }
        }
        return sInstance;
    }

    public static void prepare(Context context) {
        if (DEBUG_ALL) Slog.d(TAG, "prepare()");
        // Lazy instialize
        if (sInstance != null) return;
        IntentHooks intentHooks = new IntentHooks();
        intentHooks.mPackageManager = context.getPackageManager();
        intentHooks.parseActionHooks(context);
        intentHooks.parseCompHooks(context);
        sInstance = intentHooks;
    }

    private void parseActionHooks(Context context) {
        parseHooks(context, IntentHooks.CONFIG_TYPE_ACTION);
    }

    private void parseCompHooks(Context context) {
        parseHooks(context, IntentHooks.CONFIG_TYPE_COMPONENT);
    }

    private void parseHooks(Context context, int type) {
        String[] array = null;
        if (type == IntentHooks.CONFIG_TYPE_ACTION) {
            array = context.getResources().getStringArray(
                    com.android.internal.R.array.config_actions_to_hook);
        } else if (type == IntentHooks.CONFIG_TYPE_COMPONENT) {
            array = context.getResources().getStringArray(
                    com.android.internal.R.array.config_components_to_hook);
        }

        if (array == null || array.length <= 0) return;
        if(DEBUG_ALL) Slog.d(TAG, "parseCommen(" + array.length + ")");
        for (int i = 0; i < array.length; i++) {
            String oneLine = array[i];
            if(oneLine == null) continue;
            if(DEBUG_ALL) Slog.d(TAG, "oneLine=" + oneLine);
            String[] strings = oneLine.split(":");
            if (strings.length <= 1) continue;
            Hook oneHook = null;
            if (type == IntentHooks.CONFIG_TYPE_ACTION) {
                if(DEBUG_ALL) Slog.d(TAG, "action0= " + strings[0] + " action1= " + strings[1]);
                oneHook = new ActionHook(strings[0], strings[1]);
            } else if (type == IntentHooks.CONFIG_TYPE_COMPONENT) {
                if(DEBUG_ALL) Slog.d(TAG, "components0= " + strings[0] + " components1= " + strings[1]);
                oneHook = new ComponentHook(strings[0], strings[1]);
            }
            if (oneHook.isValid()) {
                if (mHooks.contains(oneHook)) {
                    if(DEBUG_ALL) Slog.d(TAG, "mHooks contains oneHook already");
                } else {
                    if(DEBUG_ALL) Slog.d(TAG, "add to mHooks");
                    mHooks.add(oneHook);
                }
            }
        }
    }

    public void hookIntentIfNeeded(Intent intent, int componentType) {
        if(DEBUG_ALL) Slog.d(TAG, "hookIntentIfNeeded() intent: " + intent);
        Intent storedIntent = new Intent(intent);
        boolean hooked = false;
        for (int i = 0; i < mHooks.size(); i++) {
            Hook oneHook = mHooks.get(i);
            if (oneHook.shouldHook(intent)) {
                if(DEBUG_ALL) Slog.d(TAG, "souldHook " + intent);
                oneHook.hook(intent);
                hooked = true;
                break;
            }
        }
        if (!hooked) return;
        List<ResolveInfo> list = null;
        switch (componentType) {
            case IntentHooks.COMPONENT_TYPE_ACTIVITY:
                list = mPackageManager.queryIntentActivities(intent,
                        PackageManager.GET_INTENT_FILTERS);
                break;
            case IntentHooks.COMPONENT_TYPE_SERVICE:
                list = mPackageManager.queryIntentServices(intent,
                        PackageManager.GET_INTENT_FILTERS);
                break;
            case IntentHooks.COMPONENT_TYPE_BROADCAST:
                list = mPackageManager.queryBroadcastReceivers(intent,
                        PackageManager.GET_INTENT_FILTERS);
                break;
            case IntentHooks.COMPONENT_TYPE_PROVIDER:
                break;
            default:
        }

        if (list == null || list.isEmpty() || list.size() == 0) {
            intent = storedIntent;
        }
        storedIntent = null;
    }

    class ActionHook implements Hook {

        private String mHookAction;
        private String mReplaceAction;

        public ActionHook(String hookAction, String replaceAction) {
            mHookAction = hookAction.trim();
            mReplaceAction = replaceAction.trim();
        }

        public boolean equals(Object obj) {
            if (obj instanceof ActionHook) {
                ActionHook a = (ActionHook) obj;
                return this.mHookAction.equals(a.mHookAction)
                        && this.mReplaceAction.equals(a.mReplaceAction);
            }
            return super.equals(obj);
        }

        @Override
        public boolean shouldHook(Intent intent) {
            String intentAction = intent.getAction();
            boolean isHook = mHookAction.equals(intentAction)
                    && !(intent.getBooleanExtra("extra_not_hook_action", false));
            return isHook;
        }

        @Override
        public Intent hook(Intent intent) {
            if(DEBUG_ALL) Slog.d(TAG, "hook in ActionHook " + intent + " mReplaceAction: " + mReplaceAction);
            intent.setAction(mReplaceAction);
            if(DEBUG_ALL) Slog.d(TAG, "after set action: " + intent);
            return intent;
        }

        @Override
        public boolean isValid() {
            return !(TextUtils.isEmpty(mHookAction) || TextUtils.isEmpty(mReplaceAction));
        }
    }

    class ComponentHook implements Hook {

        private ComponentName mHookCompName;
        private ComponentName mReplaceCompName;
        private boolean mOk;

        public ComponentHook(String hookComponentStr, String replaceComponentStr) {
            String[] hookArray = hookComponentStr.trim().split("/");
            String[] replaceArray = replaceComponentStr.trim().split("/");
            if (hookArray.length == 2 && replaceArray.length == 2) {
                mHookCompName = new ComponentName(hookArray[0], hookArray[1]);
                mReplaceCompName = new ComponentName(replaceArray[0], replaceArray[1]);
                mOk = true;
            } else {
                mOk = false;
            }
        }

        public boolean equals(Object obj) {
            if (obj instanceof ComponentHook) {
                ComponentHook c = (ComponentHook) obj;
                return this.mHookCompName.equals(c.mHookCompName)
                        && this.mReplaceCompName.equals(c.mReplaceCompName);
            }
            return super.equals(obj);
        }

        @Override
        public boolean shouldHook(Intent intent) {
            ComponentName intentComponent = intent.getComponent();
            boolean isHook = mHookCompName.toShortString().equals(intentComponent.toShortString())
                    && !(intent.getBooleanExtra("extra_not_hook_action", false));
            return isHook;
        }

        @Override
        public Intent hook(Intent intent) {
            return intent.setComponent(mReplaceCompName);
        }

        @Override
        public boolean isValid() {
            return mOk;
        }
    }

    interface Hook {
        /**
         * When handle intent, check intent passed whether to be hook
         *
         * @param intent The intent you will check
         * @return whether to hook
         */
        boolean shouldHook(Intent intent);

        /**
         * Hook intent here.
         *
         * @param The intent to hook
         * @return hooked intent to return
         */
        Intent hook(Intent intent);

        /**
         * Check whether this hook is valid
         *
         * @return whether check is ok.
         */
        boolean isValid();
    }
}
