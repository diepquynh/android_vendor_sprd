
package com.android.ims.internal;

import android.os.Bundle;

import com.android.ims.internal.ImsCallForwardInfoEx;
import com.android.ims.ImsSsInfo;
import com.android.ims.internal.IImsUt;
import com.android.ims.ImsReasonInfo;

interface IImsUtListenerEx {
    /**
     * Notifies the result of the supplementary service configuration udpate.
     */
    void utConfigurationUpdated(in IImsUt ut, int id);
    void utConfigurationUpdateFailed(in IImsUt ut, int id, in ImsReasonInfo error);

    /**
     * Notifies the result of the supplementary service configuration query.
     */
    void utConfigurationQueried(in IImsUt ut, int id, in Bundle ssInfo);
    void utConfigurationQueryFailed(in IImsUt ut, int id, in ImsReasonInfo error);

    /**
     * Notifies the status of the call barring supplementary service.
     */
    void utConfigurationCallBarringQueried(in IImsUt ut,
            int id, in ImsSsInfo[] cbInfo);

    /**
     * Notifies the status of the call forwarding supplementary service.
     */
    void utConfigurationCallForwardQueried(in IImsUt ut,
            int id, in ImsCallForwardInfoEx[] cfInfo);

    /**
     * Notifies the status of the call waiting supplementary service.
     */
    void utConfigurationCallWaitingQueried(in IImsUt ut,
            int id, in ImsSsInfo[] cwInfo);
}
