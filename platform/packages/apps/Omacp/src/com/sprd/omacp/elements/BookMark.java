
package com.sprd.omacp.elements;

import com.sprd.xml.parser.impl.BaseSet;
import com.sprd.xml.parser.prv.OTAKeyDefine;

public class BookMark extends BaseSet {

    public BookMark(int nType) {
        super(nType);
    }

    public boolean initEvn() {
        super.initEvn();

        addMainKeySet();
        addOptKeySet();
        return true;
    }

    private void addMainKeySet() {
        addMainKey(OTAKeyDefine.BOOKMARK_APPLICATION_APPID);
        addMainKey(OTAKeyDefine.BOOKMARK_APPLICATION_RESOURCE_URL);
    }

    private void addOptKeySet() {
        addOptionKey(OTAKeyDefine.BOOKMARK_APPLICATION_NAME);
        addOptionKey(OTAKeyDefine.BOOKMARK_APPLICATION_TO_NAPID);
        addOptionKey(OTAKeyDefine.BOOKMARK_APPLICATION_TO_PROXY);
        addOptionKey(OTAKeyDefine.BOOKMARK_APPLICATION_RESOURCE_NAME);
        addOptionKey(OTAKeyDefine.BOOKMARK_APPLICATION_RESOURCE_STARTPAGE);
        addOptionKey(OTAKeyDefine.BOOKMARK_APPLICATION_RESOURCE_AAUTHTYPE);
        addOptionKey(OTAKeyDefine.BOOKMARK_APPLICATION_RESOURCE_AAUTHNAME);
        addOptionKey(OTAKeyDefine.BOOKMARK_APPLICATION_RESOURCE_AAUTHDATA);
        addOptionKey(OTAKeyDefine.BOOKMARK_APPLICATION_RESOURCE_AAUTHSECRET);
    }
}
