
package com.sprd.process.dom2list;

import android.util.Log;

import com.sprd.xml.parser.itf.INode;
import com.sprd.xml.parser.prv.Define;
import com.sprd.xml.parser.itf.IVisitor;
import com.sprd.xml.parser.itf.IFindParam;

public class Dom2ListMgr {

    private Dom2ListMgr() {
        mData = new OtaListData();
        mDefaultVisitor = new DefaultVisitor();
    }

    synchronized public static Dom2ListMgr getInstance() {
        if (mInstance == null) {
            mInstance = new Dom2ListMgr();
        }
        return mInstance;
    }

    synchronized public static void releaseInstance() {
        if (mInstance != null) {
            mInstance = null;
        }
    }

    public int dom2ListProcess(INode node) {
        for (IFindParam findParam : PARAM) {
            getFindList(findParam, node);
        }
        return Define.STATE_OK;
    }

    private int getFindList(IFindParam param, INode node) {
        Log.e("jordan", "enter getFindList() in Dom2List");
        param.setUserData(getOtaListData().get(param.getType()));

        if (param == null || node == null || getVisitor() == null) {
            Log.e("jordan", "param is null in  getFindList()");
            return Define.STATE_ERROR;
        }
        return node.visitor(node.getRoot(), param, getVisitor());
    }

    public void setVisitor(IVisitor visitor) {
        mDefaultVisitor = visitor;
    }

    private IVisitor getVisitor() {
        return mDefaultVisitor;
    }

    public OtaListData getOtaListData() {
        return mData;
    }

    public void clearData() {
        getOtaListData().clear();
        releaseInstance();
    }

    public void Debug() {
        for (IFindParam param : PARAM) {
            param.Debug();
        }
    }

    private OtaListData mData = null;
    private IVisitor mDefaultVisitor = null;
    private static Dom2ListMgr mInstance = null;
    private final String TAG = "Dom2ListMgr";

    /* Define findParam of the primary node */
    private static IFindParam PARAM[] = {
            new ListFindParam(Define.CHAR_NAPDEF, "characteristic", "type", "NAPDEF"),
            new ListFindParam(Define.CHAR_PXLOGICAL, "characteristic", "type", "PXLOGICAL"),
            new ListFindParam(Define.CHAR_BOOTSTRAP, "characteristic", "type", "BOOTSTRAP"),
            new ListFindParam(Define.CHAR_APPLICATION, "characteristic", "type", "APPLICATION"),
            new ListFindParam(Define.CHAR_CLIENTIDENTITY, "characteristic", "type",
                    "CLIENTIDENTITY"),
            new ListFindParam(Define.CHAR_VENDORCONFIG, "characteristic", "type", "VENDORCONFIG"),
            new ListFindParam(Define.CHAR_PORT, "characteristic", "type", "PORT"),
            new ListFindParam(Define.CHAR_ACCESS, "characteristic", "type", "ACCESS")
    };

}
