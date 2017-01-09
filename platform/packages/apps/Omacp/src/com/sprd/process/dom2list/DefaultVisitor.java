
package com.sprd.process.dom2list;

import java.util.List;

import android.util.Log;

import com.sprd.xml.parser.impl.BaseVisitorImpl;
import com.sprd.xml.parser.itf.IFindParam;
import com.sprd.xml.parser.itf.INode;
import com.sprd.xml.parser.itf.IVisitor;

// APN EMAIL BOOKMARK MAINPAGE
public class DefaultVisitor extends BaseVisitorImpl {

    @SuppressWarnings("unchecked")
    @Override
    public int visitor(INode node, Object obj) {
        Log.e(TAG, "enter DefaultVisitor visitor()");
        int nRet = 0;

        ListFindParam findParam = null;

        // Check Parameter
        if (node == null) {
            Log.e(TAG, "the node is null");
            return IVisitor.VISITOR_ERR_PARAM;
        }

        if (!(obj instanceof IFindParam)) {
            Log.e(TAG, "obj not instanceof IFindParam");
            return IVisitor.VISITOR_ERR_PARAM;
        } else {
            findParam = (ListFindParam) obj;
        }

        // Check Compare Condition;
        // Node Flag
        if (!node.getName().equalsIgnoreCase(findParam.getFindNodeName())
                && node.getParent().getParent() == null) {
            return IVisitor.VISITOR_CONTINUE;
        }
        if (!node.getName().equalsIgnoreCase(findParam.getFindNodeName())) {
            return IVisitor.VISITOR_BROTHER;
        }

        // AttributeName is not empty
        if (findParam.getFindAttrName() != null
                && !node.getAttribute().containsKey(findParam.getFindAttrName())) {
            return IVisitor.VISITOR_BROTHER;
        }

        // AttributeValue is not empty
        if (findParam.getFindAttrValue() != null) {
            String szAttrValue = node.getAttribute().get(findParam.getFindAttrName());
            if (szAttrValue != null && !szAttrValue.equalsIgnoreCase(findParam.getFindAttrValue())) {
                return IVisitor.VISITOR_BROTHER;
            }
        }

        // add to List
        Object findParamObj = findParam.getUserData();
        if (findParamObj != null && findParamObj instanceof List) {
            ((List<INode>) findParamObj).add(node);
        }

        return IVisitor.VISITOR_BROTHER;

    }

    @Override
    public int endTagHandle(INode node, Object obj) {
        return 0;
    }

    @Override
    public int startTagHandle(INode node, Object obj) {
        return 0;
    }

    private static final String TAG = "DefaultVisitor";
}
