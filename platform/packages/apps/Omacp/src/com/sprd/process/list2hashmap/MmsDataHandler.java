
package com.sprd.process.list2hashmap;

import java.util.HashMap;
import java.util.List;

import com.sprd.xml.parser.prv.Define;

public class MmsDataHandler extends BaseHandlerImpl {

    @Override
    public int process(OtaMapData data) {

        return mmsProcess(data);
    }

    private int mmsProcess(OtaMapData data) {
        if (data == null) {
            return Define.STATE_PARAM_ERROR;
        }
        List<MyHashMap> appList = data.get(Define.CHAR_APPLICATION);
        for (HashMap<String, String> mmsMap : appList) {
            String appId = mmsMap.get(Define.APPID);
            if (null == appId) {
                continue;
            }
            if (appId.equalsIgnoreCase(Define.MMS_APPID)) {
                data.put(Define.TYPE_MMS, (MyHashMap) mmsMap);
            }
        }
        return Define.STATE_OK;
    }

    private final String TAG = "MmsHandler";
}
