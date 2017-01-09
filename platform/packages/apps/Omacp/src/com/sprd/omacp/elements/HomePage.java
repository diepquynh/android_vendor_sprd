
package com.sprd.omacp.elements;

import com.sprd.xml.parser.impl.BaseSet;
import com.sprd.xml.parser.prv.OTAKeyDefine;

public class HomePage extends BaseSet {

    public HomePage(int nType) {
        super(nType);
    }

    public boolean initEvn() {
        super.initEvn();

        addMainKeySet();
        addOptKeySet();
        return true;
    }

    private void addMainKeySet() {
        addMainKey(OTAKeyDefine.HOMEPAGE_APPLICATION_APPID);
        addMainKey(OTAKeyDefine.HOMEPAGE_APPLICATION_RESOURCE_URL);
    }

    private void addOptKeySet() {
        addOptionKey(OTAKeyDefine.HOMEPAGE_APPLICATION_NAME);
        addOptionKey(OTAKeyDefine.HOMEPAGE_APPLICATION_TO_NAPID);
        addOptionKey(OTAKeyDefine.HOMEPAGE_APPLICATION_TO_PROXY);
        addOptionKey(OTAKeyDefine.HOMEPAGE_APPLICATION_RESOURCE_NAME);
        addOptionKey(OTAKeyDefine.HOMEPAGE_APPLICATION_RESOURCE_STARTPAGE);
        addOptionKey(OTAKeyDefine.HOMEPAGE_APPLICATION_RESOURCE_AAUTHTYPE);
        addOptionKey(OTAKeyDefine.HOMEPAGE_APPLICATION_RESOURCE_AAUTHNAME);
        addOptionKey(OTAKeyDefine.HOMEPAGE_APPLICATION_RESOURCE_AAUTHDATA);
        addOptionKey(OTAKeyDefine.HOMEPAGE_APPLICATION_RESOURCE_AAUTHSECRET);
    }
}
