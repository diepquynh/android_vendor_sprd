package com.android.server.power;

import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.view.IWindowManager;
import android.util.Slog;
import java.io.File;

public class ShutdownAnimationEx extends ShutdownAnimation {
    private static final String TAG = "ShutdownAnimationEx";

    boolean playShutdownAnimation() {
        File fileDefault = new File("/system/media/shutdownanimation.zip");
        File file = new File("/data/theme/overlay/shutdownanimation.zip");
        boolean hasShutdownAnimation = file.exists() || fileDefault.exists();
        if (!hasShutdownAnimation) {
            return false;
        }
        IWindowManager wm = IWindowManager.Stub.asInterface(ServiceManager.getService("window"));
        try {
            wm.setForceOrientation(0);
        } catch (RemoteException e) {
            Slog.e(TAG, "stop orientation failed!", e);
        }

        String[] bootcmd = {"bootanimation", "shutdown"} ;
        try {
            Slog.i(TAG, "exec the bootanimation ");
            SystemProperties.set("service.bootanim.exit", "0");
            Runtime.getRuntime().exec(bootcmd);
        } catch (Exception e) {
            Slog.e(TAG,"bootanimation command exe err!");
        }
        return true;
    }
}
