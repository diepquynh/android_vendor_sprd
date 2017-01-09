
package com.sprd.process.dom2list;

import java.util.List;

import com.sprd.xml.parser.itf.IFindParam;
import com.sprd.xml.parser.itf.INode;

public class ListFindParam implements IFindParam {

    public void setUserData(Object obj) {
        mObject = obj;
    }

    public Object getUserData() {
        return mObject;
    }

    public ListFindParam(int nType, String nodeName, String attrName, String attrValue) {
        mnType = nType;
        mNodeName = nodeName;
        mAttrName = attrName;
        mAttrValue = attrValue;
    }

    public String getFindAttrValue() {
        return mAttrValue;
    }

    public void setFindAttrValue(String attrValue) {
        mAttrValue = attrValue;
    }

    public String getFindNodeName() {
        return mNodeName;
    }

    public void setFindNodeName(String nodeName) {
        mNodeName = nodeName;
    }

    public String getFindAttrName() {
        return mAttrName;
    }

    public void setFindAttrName(String attrName) {
        mAttrName = attrName;
    }

    public int getType() {
        return mnType;
    }

    public void Debug() {
        System.out.println("\nbefore----------- type : " + getType() + " " + getFindAttrValue()
                + "-------------\n");
        if (null == getUserData()) {
            System.out.println("the UserData is null");
            return;
        }
        if (!(getUserData() instanceof List)) {
            System.out.println("UserData is not instance of List");
            return;
        }
        for (INode node : (List<INode>) getUserData()) {
            node.Debug();
        }
        System.out.println("\n----------- type : " + getType() + " " + getFindAttrValue()
                + "-------------end\n");
    }

    private int mnType;
    private Object mObject = null;
    private String mNodeName = null;
    private String mAttrName = null;
    private String mAttrValue = null;
    private static final String TAG = "FindParam";

}
