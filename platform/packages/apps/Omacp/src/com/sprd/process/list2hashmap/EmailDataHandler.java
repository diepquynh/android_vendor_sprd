
package com.sprd.process.list2hashmap;

import java.util.HashMap;
import java.util.List;

import com.sprd.xml.parser.prv.Define;

public class EmailDataHandler extends BaseHandlerImpl {

    @Override
    public int process(OtaMapData data) {

        return emailProcess(data);
    }

    private int emailProcess(OtaMapData data) {
        if (data == null) {
            return Define.STATE_PARAM_ERROR;
        }
        List<MyHashMap> appList = data.get(Define.CHAR_APPLICATION);
        for (HashMap<String, String> emailMap : appList) {
            String appId = emailMap.get(Define.APPID);
            if (null == appId) {
                continue;
            }
            if (appId.equalsIgnoreCase(Define.POP_DEFAULT)
                    || appId.equalsIgnoreCase(Define.POP_SSL)
                    || appId.equalsIgnoreCase(Define.IMAP_DEFAULT)
                    || appId.equalsIgnoreCase(Define.IMAP_SSL)
                    || appId.equalsIgnoreCase(Define.SMTP_DEFAULT)
                    || appId.equalsIgnoreCase(Define.SMTP_SSL)) {
                data.put(Define.TYPE_EMAIL, (MyHashMap) emailMap);
            }
        }
        return Define.STATE_OK;
    }

    private final String TAG = "EmailHandler";
}
