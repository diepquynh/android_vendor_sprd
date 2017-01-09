
package com.sprd.xml.parser.impl;

import com.sprd.xml.parser.itf.INode;
import com.sprd.xml.parser.itf.IVisitor;
import com.sprd.xml.parser.prv.Define;

public abstract class BaseVisitorImpl implements IVisitor {

    public boolean isSuccessfully(int nValue) {
        return ((nValue & VISITOR_OK) == VISITOR_OK);// is ( nValue | VISITOR_OK
                                                     // ) == VISITOR_OK ? need
                                                     // confirm
    }

    public boolean isFailure(int nValue) {
        return ((nValue & VISITOR_FAILURE_BASE) == VISITOR_FAILURE_BASE);
    }

    public int getErrorCode(int nReture) {
        return (nReture & VISITOR_MASK);
    }

    public String getErrorDesc(int nCode) {
        return "";
    }

    @Override
    public int startTagHandle(INode node, Object obj) {
        return 0;
    }

    @Override
    public int endTagHandle(INode node, Object obj) {
        return 0;
    }

    public boolean isNeedRename(String key, String[] array) {
        if (array == null || key == null) {
            return false;
        }
        for (String str : array) {
            if (key.equalsIgnoreCase(str)) {
                return true;
            }
        }
        return false;
    }

    public String getRenameKey(INode node, String attrKey) {
        StringBuilder sb = new StringBuilder();
        String fatherKey = node.getParent().getAttribute().get(Define.CHAR_NODE_ATTR_NAME_TYPE);
        String nodeKey = node.getAttribute().get(attrKey);
        sb.append(fatherKey).append("_").append(nodeKey);
        System.out.println("will rename the key : " + sb.toString().toLowerCase());
        return sb.toString().toLowerCase();
    }

    public String[] getRenameArray() {
        return mRenameKeyArray;
    }

    public void setRenameFlag(boolean flag) {
        renameFlag = flag;
    }

    public boolean getRenameFlag() {
        return renameFlag;
    }

    private boolean renameFlag = false;
    private String[] mRenameKeyArray = new String[] {
            //add for bug 518797 begin
            "name", "enabled", "filter", "replace","addr"
            //add for bug 518797 end
    };

}
