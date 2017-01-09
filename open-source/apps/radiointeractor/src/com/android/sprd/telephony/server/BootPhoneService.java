
package com.android.sprd.telephony.server;

import com.android.sprd.telephony.RadioInteractorHandler;
import com.android.sprd.telephony.RadioInteractorService;
import com.android.sprd.telephony.RadioInteractorNotifier;
import com.android.sprd.telephony.UtilLog;

public class BootPhoneService extends RadioInteractorService {
    @Override
    public void init(RadioInteractorNotifier radioInteractorNotifier,
            RadioInteractorHandler[] radioInteractorHandler) {
        UtilLog.logd(RadioInteractorService.TAG, "BootPhoneService init");
        RadioInteractorProxy.init(this, radioInteractorNotifier, radioInteractorHandler);
    }
}
