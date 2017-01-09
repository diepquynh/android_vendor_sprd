
package com.sprd.omacp.elements;

import com.sprd.xml.parser.impl.BaseSet;
import com.sprd.xml.parser.prv.OTAKeyDefine;

public class Email extends BaseSet {

    public Email(int nType) {
        super(nType);
    }

    public boolean initEvn() {
        super.initEvn();

        addMainKeySet();
        addOptKeySet();

        return true;
    }

    private void addMainKeySet() {
        addMainKey(OTAKeyDefine.EMAIL_APPLICATION_APPID);
    }

    private void addOptKeySet() {
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_FROM);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_TO_NAPID);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_TO_PROXY);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_PROVIDER_ID);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_APPADD_ADDR);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_PORT_SERVICE);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_PORT_PORTNBR);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_APPADD_ADDRTYPE);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_APPAUTH_AAUTHDATA);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_APPAUTH_AAUTHTYPE);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_APPAUTH_AAUTHLEVEL);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_APPAUTH_AAUTHNAME);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_APPAUTH_AAUTHSECRET);
        // add exchange keys
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_MON);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_TUE);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_WED);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_THU);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_FRI);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_SAT);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_SUN);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_SSL);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_POPUP);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_APPREF);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_PEAKEND);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_DOMAIN);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_CONFLICT);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_PEAKSTART);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_SIGNATURE);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_EMAILADDR);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_TASK_FILTER);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_TASK_ENABLED);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_TASK_REPLACE);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_EMAIL_FILTER);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_PEAKFREQUENCY);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_SYNCWHENROAMING);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_OFFPEAKFREQUENCY);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_CALENDAR_ENABLED);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_CALENDAR_FILTER);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_CALENDAR_REPLACE);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_CONTACT_FILTER);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_CONTACT_ENABLED);
        addOptionKey(OTAKeyDefine.EMAIL_APPLICATION_CONTACT_REPLACE);
    }

}
