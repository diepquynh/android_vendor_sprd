
package com.android.plat.mms.plugin.Impl;

import java.util.HashMap;
import java.util.List;

import com.android.plat.mms.plugin.Interface.INotify;


public class BaseUserMessage extends NotifyStatus implements INotify {

    private INotify mcbf;
    private Object mObj;
    private HashMap<String, Object> mMapParamter;

    protected BaseUserMessage()
    {
        mcbf = null;
        mObj = null;
        mMapParamter = new HashMap<String, Object>();
    }

    public void SetCallBack(INotify cbf, Object obj) {
        mcbf = cbf;
        mObj = obj;
    }

    public int SetParameter(String szName, Object obj) {
        int nRet = PARA_NOT_EXIST;
        if (null != GetUserParamterByName(szName)) {
            nRet = PARA_EXIST;
        }
        if (obj == null)
            return (PARA_ERROR | FAILURE);

        if (GetUserParamter().put(szName, obj) == null)
            return (nRet | FAILURE);
        else
            return (nRet | SUCC);
    }

    public int OnNotify(int nMsg, int nValue, long lValue, Object obj,
            List<Object> lstObj) {
        return FAILURE;
    }

    // export interface to child class
    protected INotify GetCallBack() {
        return mcbf;
    }

    protected Object GetCbfParamter() {
        return mObj;
    }

    protected Object GetUserParamterByName(String szName) {
        if (GetUserParamter().containsKey(szName))
            return GetUserParamter().get(szName);
        else
            return null;
    }

    protected HashMap<String, Object> GetUserParamter() {
        return mMapParamter;
    }

}
