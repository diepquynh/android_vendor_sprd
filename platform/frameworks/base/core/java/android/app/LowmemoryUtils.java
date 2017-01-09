package android.app;

import android.os.RemoteException;

/**
 *  SPRD:This file is add for Lowmemory case.In order to
 *       Provide some particular API for application.
 *       Its the extension of {@link android.app.ActivityMAnager}
 *  @hide
 */
public class LowmemoryUtils {

    //SPRD: add for kill-stop front app process when phone call is incoming @{
    public static final int KILL_STOP_FRONT_APP = 1;

    public static final int KILL_CONT_STOPPED_APP = 0;

    public static final int CANCEL_KILL_STOP_TIMEOUT = 2;

    public static void killStopFrontApp(int func) {
        try {
            ActivityManagerNativeEx.getDefault().killStopFrontApp(func);
        } catch (RemoteException e) {
            // System dead, we will be dead too soon!
        }
    }
    //@}
}

