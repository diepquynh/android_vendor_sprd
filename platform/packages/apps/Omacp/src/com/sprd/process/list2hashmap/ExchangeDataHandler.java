
package com.sprd.process.list2hashmap;

import java.util.HashMap;
import java.util.List;

import com.sprd.xml.parser.prv.Define;

public class ExchangeDataHandler extends BaseHandlerImpl {

    @Override
    public int process(OtaMapData data) {

        return exchangeDataProcess(data);
    }

    private int exchangeDataProcess(OtaMapData data) {
        if (data == null) {
            return Define.STATE_PARAM_ERROR;
        }
        List<MyHashMap> appList = data.get(Define.CHAR_APPLICATION);
        for (HashMap<String, String> easMap : appList) {
            String appId = easMap.get(Define.APPID);
            if (null == appId) {
                continue;
            }
            if (isExchangeApp(appId)) {
                data.put(Define.TYPE_EXCHANGE, (MyHashMap) easMap);
            }
        }
        return Define.STATE_OK;
    }

    private boolean isExchangeApp(String appId) {
        if (!appId.equalsIgnoreCase(Define.POP_DEFAULT) & !appId.equalsIgnoreCase(Define.POP_SSL)
                & !appId.equalsIgnoreCase(Define.IMAP_DEFAULT)
                & !appId.equalsIgnoreCase(Define.IMAP_SSL)
                & !appId.equalsIgnoreCase(Define.SMTP_DEFAULT)
                & !appId.equalsIgnoreCase(Define.SMTP_SSL)
                & !appId.equalsIgnoreCase(Define.BROWSE_APPID)
                & !appId.equalsIgnoreCase(Define.MMS_APPID)) {
            return true;
        }
        return false;
    }

    private final String TAG = "ExchangelHandler";
}
