

package com.android.ims.internal;

import android.os.IBinder;
import android.os.Message;
import android.os.ServiceManager;

/**
 * Provides APIs for IMS services, such as initiating IMS calls, and provides access to
 * the operator's IMS network. This class is the starting point for any IMS actions.
 * You can acquire an instance of it with {@link #getInstance getInstance()}.</p>
 * <p>The APIs in this class allows you to:</p>
 *
 */
public class ImsManagerEx {

    public static final String EXTRA_IMS_CONFERENCE_REQUEST =
            "android.intent.extra.IMS_CONFERENCE_REQUEST";
    public static final String EXTRA_IMS_CONFERENCE_PARTICIPANTS =
            "android.intent.extra.IMS_CONFERENCE_PARTICIPANTS";
    

    public static final String IMS_SERVICE_EX = "ims_ex";
    public static final String IMS_UT_EX = "ims_ut_ex";

    public static final int IMS_UNREGISTERED = 0;
    public static final int IMS_REGISTERED   = 1;

    public static final int HANDOVER_STARTED   = 0;
    public static final int HANDOVER_COMPLETED = 1;
    public static final int HANDOVER_FAILED    = 2;
    public static final int HANDOVER_CANCELED  = 3;

    public static IImsServiceEx getIImsServiceEx(){
        IBinder b = ServiceManager.getService(IMS_SERVICE_EX);
        IImsServiceEx service = null;
        if(b != null){
            service = IImsServiceEx.Stub.asInterface(b);
        }
        return service;
    }

    public static IImsUtEx getIImsUtEx(){
        IBinder b = ServiceManager.getService(IMS_UT_EX);
        IImsUtEx service = null;
        if(b != null){
            service = IImsUtEx.Stub.asInterface(b);
        }
        return service;
    }
}
