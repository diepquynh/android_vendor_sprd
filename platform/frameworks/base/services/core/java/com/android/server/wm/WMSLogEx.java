package com.android.server.wm;

import static com.android.server.wm.WindowManagerDebugConfig.TAG_WM;
import android.util.Slog;
import java.lang.reflect.Field;


public class WMSLogEx extends WMSLog {

    public void dumpWindowStatus(AppWindowToken wtoken) {
        if (wtoken != null) {
            final int N = wtoken.allAppWindows.size();
            for (int i=0; i<N; i++) {
                final WindowState w = wtoken.allAppWindows.get(i);
                final WindowStateAnimator winAnimator = w.mWinAnimator;
                Slog.i(TAG_WM, "Dump window# " + w + "  Status#" + "isFocused: " + w.isFocused()
                        + " hasDrawnLw: " + w.hasDrawnLw() + " canReceiveKeys: " + w.canReceiveKeys()
                        + " isDrawnLw: " + w.isDrawnLw() + " isDrawFinishedLw: " + w.isDrawFinishedLw()
                        + " isAnimatingLw: " + w.isAnimatingLw() + " isDisplayedLw: " + w.isDisplayedLw()
                        + " isReadyForDisplay: " + w.isReadyForDisplay() + " isOnScreen: " + w.isOnScreen()
                        + " isVisibleOrAdding: " + w.isVisibleOrAdding() + " isVisibleNow: " + w.isVisibleNow()
                        + " isWinVisibleLw: " + w.isWinVisibleLw() + " isVisibleLw: " + w.isVisibleLw());
                Slog.i(TAG_WM, "Dump WindowStateAnimator# "  + "  Status# " +
                        " hasDrawnLw: " + winAnimator.isAnimationSet());
            }
        } else {
            Slog.i(TAG_WM, "Dump token is null");
        }
    }

    public void dumpWindowStatus(DisplayContent displayContent) {
        final WindowList windows = displayContent.getWindowList();
        for (int i = windows.size() - 1; i >= 0; i--) {
            final WindowState win = windows.get(i);
            Slog.i( TAG_WM, "Dump Win No." + i
                    + " = " + win
                    + " = " + win.mAppToken
                    + ", flags=0x" + Integer.toHexString(win.mAttrs.flags)
                    + ", canReceive=" + win.canReceiveKeys()
                    + ", ViewVisibility=" + win.mViewVisibility
                    + ", isVisibleOrAdding=" + win.isVisibleOrAdding()
                    + ", DrawState=" + win.mWinAnimator.mDrawState);
            if (!win.isVisibleOrAdding()) {
                String s;
                if (win.mAppToken != null) s = " hiddenRequested=" + win.mAppToken.hiddenRequested;
                else s = " appToken=null";
                Slog.i(TAG_WM, "    mSurfaceController=" + win.mWinAnimator.mSurfaceController
                        + " relayoutCalled=" + win.mRelayoutCalled + " viewVis=" + win.mViewVisibility
                        + " policyVis=" + win.mPolicyVisibility
                        + " policyVisAfterAnim=" + win.mPolicyVisibilityAfterAnim
                        + " attachHid=" + win.mAttachedHidden
                        + " exiting=" + win.mAnimatingExit + " destroying=" + win.mDestroying + s);
            }
        }
    }

    public boolean changeDebugFlag(String[] args) {
        int opti = 0;
        String cmd = null;
        String valueStr = null;
        boolean value = false;
        while (opti < args.length) {
            String opt = args[opti];
            if (opt == null || opt.length() <= 0 || opt.charAt(0) != '-') {
                Slog.w(TAG_WM, "wrong usage of change debug flag");
                return false;
            }
            opti++;

            if ("-set".equals(opt)) {
                if (opti < args.length - 1) {
                    cmd = args[opti++];
                    valueStr = args[opti++];
                    if ("0".equals(valueStr) || "false".equalsIgnoreCase(valueStr)) {
                        value = false;
                    } else if ("1".equals(valueStr) || "true".equalsIgnoreCase(valueStr)) {
                        value = true;
                    } else {
                        Slog.w(TAG_WM, "wrong usage of change debug flag");
                        return false;
                    }
                    break;
                } else {
                    Slog.w(TAG_WM, "wrong usage of change debug flag");
                    return false;
                }
            }
        }

        Class cls = null;
        Field field = null;
        try {
            cls = Class.forName("com.android.server.wm.WindowManagerDebugConfig");
            field = cls.getDeclaredField(cmd);
            boolean access = field.isAccessible();
            field.setAccessible(true);
            field.setBoolean(null, value);
            field.setAccessible(access);
        } catch (Exception e) {
            Slog.w(TAG_WM, "Exception when get WindowManagerDebugConfig");
            return false;
        }
        return true;

    }
}
