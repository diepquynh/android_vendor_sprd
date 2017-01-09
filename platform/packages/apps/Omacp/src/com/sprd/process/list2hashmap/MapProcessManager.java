
package com.sprd.process.list2hashmap;

import com.sprd.xml.parser.prv.Define;

public class MapProcessManager {

    private MapProcessManager() {
    }

    synchronized public static MapProcessManager getInstance() {
        if (null == mInstance) {
            mInstance = new MapProcessManager();
        }
        return mInstance;
    }

    synchronized public void releaseIns() {
        if (mInstance != null) {
            mInstance = null;
        }
    }

    public int mapDataProcess(OtaMapData mapData) {
        if (null == mapData) {
            return Define.STATE_ERROR;
        }
        getBaseHandler().process(mapData);
        return Define.STATE_OK;
    }

    public void setBaseHandler(BaseHandlerImpl handler) {
        mHandler = handler;
    }

    public BaseHandlerImpl getBaseHandler() {
        return mHandler;
    }

    private BaseHandlerImpl mHandler = null;
    private final String TAG = "MapProcessManager";
    private static MapProcessManager mInstance = null;
}
