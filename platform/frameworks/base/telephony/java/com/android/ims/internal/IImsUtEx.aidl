
package com.android.ims.internal;

import com.android.ims.internal.IImsUtListenerEx;

/**
 * Provides the Ut interface interworking to get/set the supplementary service configuration.
 *
 */
interface IImsUtEx {

    /**
     * Retrieves the configuration of the call forward.
     */
    int setCallForwardingOption(int phoneId, int commandInterfaceCFAction,
            int commandInterfaceCFReason,int serviceClass, String dialingNumber,
            int timerSeconds, String ruleSet);

    /**
     * Updates the configuration of the call forward.
     */
    int getCallForwardingOption(int phoneId, int commandInterfaceCFReason, int serviceClass,
            String ruleSet);

    /**
     * Sets the listener.
     */
    void setListenerEx(int phoneId, IImsUtListenerEx listener);
}
