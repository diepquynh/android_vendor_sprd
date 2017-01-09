package com.android.callsettings.callbarring;


public class CallBarringInfo {
    public int status;      /*mode, 1 = active, 0 = not active */
    public int reason;      /* fac, from TS 27.007 7.11 "reason" */
    public int serviceClass; /* class, Sum of CommandsInterface.SERVICE_CLASS */
    public String password;      /* "number" from TS 27.007 7.11 */

    public String toString() {
        return super.toString() + (status == 0 ? " not active " : " active ")
            + " reason: " + reason
            + " serviceClass: " + serviceClass;
    }
}

