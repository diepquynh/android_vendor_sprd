/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.internal.telephony.imsphone;

import com.android.internal.telephony.Phone;
import com.android.ims.ImsCall;
import android.telephony.Rlog;
import android.telephony.DisconnectCause;



public class ImsPhoneConnectionEx extends ImsPhoneConnection{

    /** This is probably an MT call */
    public ImsPhoneConnectionEx(Phone phone, ImsCall imsCall, ImsPhoneCallTracker ct,
            ImsPhoneCall parent, boolean isUnknown){
        super(phone,imsCall,ct,parent,isUnknown);

    }

    /** This is an MO call, created when dialing */
    public ImsPhoneConnectionEx(Phone phone, String dialString, ImsPhoneCallTracker ct,
            ImsPhoneCall parent, boolean isEmergency) {
        super(phone,dialString,ct,parent,isEmergency);
    }

    /** Called when the connection has been disconnected */
    @Override
    public boolean onDisconnect(int cause) {
        Rlog.d("ImsPhoneConnectionEx", "onDisconnect: cause=" + cause+ " mCause="+mCause);
        if (mCause != DisconnectCause.LOCAL|| cause == DisconnectCause.INCOMING_REJECTED) mCause = cause;
        return onDisconnect();
    }
}
