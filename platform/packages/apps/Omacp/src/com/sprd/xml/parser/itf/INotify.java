
package com.sprd.xml.parser.itf;

import java.util.List;

public interface INotify {

    public void SetCallBack(INotify cbf, Object obj);

    public int SetParameter(String szName, Object obj);

    public int OnNotify(int nMsg, int nValue, long lValue, Object obj, List<Object> lstObj);
}
