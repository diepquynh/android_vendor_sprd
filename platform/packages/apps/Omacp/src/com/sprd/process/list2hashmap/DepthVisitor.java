
package com.sprd.process.list2hashmap;

import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import com.sprd.xml.parser.impl.BaseVisitorImpl;
import com.sprd.xml.parser.itf.IFindParam;
import com.sprd.xml.parser.itf.INode;
import com.sprd.xml.parser.itf.IVisitor;
import com.sprd.xml.parser.prv.Define;
import com.sprd.ota.rule.RuleManager;

public class DepthVisitor extends BaseVisitorImpl {

    @SuppressWarnings("unchecked")
    @Override
    public int visitor(INode node, Object obj) {
        int nRet = 0;
        IFindParam mapFindParam = null;
        // add for 485676 begin
        String ruleKeyStr = "";
        // add for 485676 end
        if (node == null || obj == null) {
            return IVisitor.VISITOR_ERR_PARAM;
        }
        if (!(obj instanceof IFindParam)) {
            System.out.println(TAG + "obj is not instanceof IFindParam");
            return IVisitor.VISITOR_ERR_PARAM;
        }
        // add for 485676 begin
        if (node.getParent() != null
                && node.getParent().getName().equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)) {
            ruleKeyStr = getRuleKey(node.getParent());
        }
        // add for 485676 end
        mapFindParam = (MapFindParam) obj;
        String attrName1 = ((MapFindParam) mapFindParam).getAttrList().get(1);
        String attrName2 = ((MapFindParam) mapFindParam).getAttrList().get(2);
        // check nodeName
        if (!node.getName().equalsIgnoreCase(((MapFindParam) mapFindParam).getNodeName())) {
            return IVisitor.VISITOR_CONTINUE;
        }
        // check AttrKey1
        if (attrName1 != null && !node.getAttribute().containsKey(attrName1)) {
            return IVisitor.VISITOR_CONTINUE;
        }
        // check AttrKey2
        /*
         * if (attrName2 != null && !node.getAttribute().containsKey(attrName2)
         * &&
         * !node.getAttribute().containsValue(Define.PARM_NODE_ATTR_STARTPAGE))
         * { return IVisitor.VISITOR_CONTINUE; }
         */
        // some node don't have value parameter ,if not, value = ""
        String key = node.getAttribute().get(attrName1).toLowerCase();
        String value = "";
        if (attrName2 != null && node.getAttribute().containsKey(attrName2)) {
            // add for bug 484306 begin
            value = node.getAttribute().get(attrName2);
            // add for bug 484306 end
        }
        if (isNeedRename(key, ((MapFindParam) mapFindParam).getRenameArray())
                && ((MapFindParam) mapFindParam).getRenameFlag()) {
            key = getRenameKey(node, attrName1);
        }
        // add for 485676 begin
        boolean containRuleKey = false;
        boolean setsContainKey = false;
        if (!ruleKeyStr.isEmpty()) {
            System.out.println("---------the ruleKeyStr = " + ruleKeyStr);
            if (RuleManager.createInstance().getRuleMaps().get(ruleKeyStr) == null) {
                System.out
                        .println("----------RuleManager.createInstance().getRuleMaps().get(ruleKeyStr) == null");
            } else {
                System.out
                        .println("RuleManager.createInstance().getRuleMaps().get(ruleKeyStr) != null");
            }
        }
        if (RuleManager.createInstance().getRuleMaps() != null
                && RuleManager.createInstance().getRuleMaps().get(ruleKeyStr) != null) {
            System.out.println("-------------has RuleMap and has RuleSets-------------");
            containRuleKey = RuleManager.createInstance().getRuleMaps().containsKey(ruleKeyStr);
            setsContainKey = RuleManager.createInstance().getRuleMaps().get(ruleKeyStr)
                    .contains(key);
        }

        System.out.println("containRuleKey = " + containRuleKey + ", setsContainKey = "
                + setsContainKey);
        if (containRuleKey && setsContainKey
                && ((HashMap<String, String>) mapFindParam.getUserData()).containsKey(key)) {
            // 1~* or 0~* not cover
            System.out.println("----------------------------add M params, the key = " + key
                    + "-------------------");
            int keyCount = 0;
            for (String keyItem : ((HashMap<String, String>) mapFindParam.getUserData()).keySet()) {
                if (keyItem.contains(key)) {
                    keyCount++;
                }
            }
            System.out.println("keyCount = " + keyCount);
            ((HashMap<String, String>) mapFindParam.getUserData()).put(key + keyCount, value);

        } else if (((HashMap<String, String>) mapFindParam.getUserData()).containsKey(key)) {
            // whether contains the key,if yes, do nothing
            return IVisitor.VISITOR_CONTINUE;
        } else {
            // save hashMap data to UserData first time
            ((HashMap<String, String>) mapFindParam.getUserData()).put(key, value);
        }
        // add for 485676 end
        return IVisitor.VISITOR_CONTINUE;
    }

    private String getRuleKey(INode node) {
        INode tmpNode = node;
        String ruleKey = "";
        if (tmpNode == null) {
            System.out.println("----------------node is null---------------------");
            return "";
        }
        if (!tmpNode.getName().equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)) {
            System.out
                    .println("-----------------node name is not equals to CHARACTERISTIC-------------------");
            return "";
        }
        if (tmpNode.getParent() != null
                && !tmpNode.getParent().getName().equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)) {
            ruleKey = tmpNode.getAttribute().get("type");
        }
        while (tmpNode.getParent() != null
                && tmpNode.getParent().getName().equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)) {
            if (ruleKey.isEmpty()) {
                ruleKey = tmpNode.getParent().getAttribute().get("type") + "/"
                        + tmpNode.getAttribute().get("type");
            } else {
                ruleKey = tmpNode.getParent().getAttribute().get("type") + "/" + ruleKey;
            }
            tmpNode = tmpNode.getParent();
        }
        // System.out.println("enter getRuleKey, and the ruleKey = " + ruleKey);
        return ruleKey;
    }

    private final String TAG = "DepthVisitor";

}
