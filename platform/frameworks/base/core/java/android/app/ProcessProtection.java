/*
 *Copyright © 2016 Spreadtrum Communications Inc.
 */

package android.app;

import android.os.Process;
import android.os.RemoteException;

/**
 * @hide
 */
public class ProcessProtection {
    /**
     * normal bg process
     * @hide
     */
    public static final int  PROCESS_STATUS_IDLE = 0;
    /**
     * running process in bg or foregroud, it's adj adjust to 0
     * @hide
     */
    public static final int  PROCESS_STATUS_RUNNING = 1;
    /**
     * protacted process, it's adj adjust to 2
     * @hide
     */
    public static final int  PROCESS_STATUS_MAINTAIN = 2;
    /**
     * protacted process, this process will be persistent
     * if protected status is it, do not reset status to {@link PROCESS_STATUS_IDLE}
     * @hide
     */
    public static final int  PROCESS_STATUS_PERSISTENT = 3;
    /**
     * Process protect area: in this level, it's adj adjust to 0
     * @hide
     */
    public static final int  PROCESS_PROTECT_CRITICAL = 11;
    /**
     * Process protect area: in this level, it's adj adjust to 2
     * @hide
     */
    public static final int  PROCESS_PROTECT_IMPORTANCE = 12;
    /**
     * Process protect area: in this level, it's adj adjust to 4
     * @hide
     */
    public static final int  PROCESS_PROTECT_NORMAL = 13;
    /**
     * set myslef process protect status
     * @see #PROCESS_STATUS_IDLE
     * @see #PROCESS_STATUS_RUNNING
     * @see #PROCESS_STATUS_MAINTAIN
     * @see #PROCESS_STATUS_PERSISTENT
     * @hide
     */
    public void setSelfProtectStatus(int status) {
        try {
            ActivityManagerNative.getDefault().setProcessProtectStatus(Process.myPid(), status);
        } catch (RemoteException e) {}
    }

}
