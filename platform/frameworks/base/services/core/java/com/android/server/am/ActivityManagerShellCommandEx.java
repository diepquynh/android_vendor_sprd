/*
 * The Spreadtrum Communication Inc. 2016
 */

package com.android.server.am;

import android.os.RemoteException;
import android.util.Slog;

import java.io.PrintWriter;

class ActivityManagerShellCommandEx extends ActivityManagerShellCommand {
    private static final String TAG = "ActivityManager.Shell";

    ActivityManagerShellCommandEx(ActivityManagerService service, boolean dumping) {
        super(service, dumping);
    }

    int runLmkForceStop(PrintWriter pw) {
        if (mInternal != null
                && mInternal instanceof ActivityManagerServiceEx) {
            ActivityManagerServiceEx service = (ActivityManagerServiceEx) mInternal;
            if (service.mLmkTracker != null) {
                String opt = null;
                if ((opt = getNextArgRequired()) != null) {
                    int pid = 0;
                    try {
                        pid = Integer.parseInt(opt);
                    } catch (Exception e) {
                        return -1;
                    }
                    if (pid > 0) {
                        service.mLmkTracker.doLmkForceStop(pid);
                        return 0;
                    }
                }
            } else {
                Slog.e(TAG, "lmk-force-stop failed, lmk tracker is null");
            }
        } else {
            Slog.e(TAG, "lmk-force-stop failed, AmsEx not inherited");
        }
        return -1;
    }

    @Override
    public int onCommand(String cmd) {
        if (cmd == null) {
            return handleDefaultCommands(cmd);
        }
        PrintWriter pw = getOutPrintWriter();
        try {
            switch (cmd) {
                case "lmk-force-stop":
                    return runLmkForceStop(pw);
                default:
                    return super.onCommand(cmd);
            }
        } catch (Exception e) {
            pw.println("Exception: " + e);
        }
        return -1;
    }

}
