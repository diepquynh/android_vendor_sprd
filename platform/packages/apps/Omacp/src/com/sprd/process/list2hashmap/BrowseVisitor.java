
package com.sprd.process.list2hashmap;

import java.util.ArrayList;
import java.util.List;
import android.util.Log;
import java.util.HashMap;
import com.sprd.ota.rule.RuleManager;
import com.sprd.xml.parser.itf.INode;
import com.sprd.xml.parser.itf.IVisitor;
import com.sprd.xml.parser.prv.Define;
import com.sprd.xml.parser.impl.BaseVisitorImpl;

class VisitorParam {
    public MyHashMap getRootHashMap() {
        return mRootHashMap;
    }

    public List<MyHashMap> getBrowserHashMap() {
        return mItemHashMaps;
    }

    // add for bug 518797 begin
    public List<MyHashMap> getOtherHashMap() {
        return mOtherHashMaps;
    }

    // add for bug 518797 end

    /*
     * generate the browse data
     */
    public void build(OtaMapData data) {
        System.out.println("enter build()");
        final List<MyHashMap> bookmarklist = data.get(Define.TYPE_BOOKMARK);
        // add for bug 485744 begin
        List<MyHashMap> startpagelist = data.get(Define.TYPE_STARTPAGE);
        final List<MyHashMap> pxLogicalList = data.get(Define.CHAR_PXLOGICAL);
        // add for bug 485744 end
        // add for bug 518797 begin
        for (MyHashMap myHashMap : getOtherHashMap()) {
            // add otherNodeParams to rootHashMap, will generate An new
            // rootHashMap
            myHashMap.setIgnoreCase(true);
            getRootHashMap().putAll(myHashMap);
            System.out.println("OtherHashMap : " + myHashMap.toString());
        }
        System.out.println("rootHashMap : " + getRootHashMap().toString());
        // add for bug 518797 end
        for (MyHashMap itemsHashMap : mItemHashMaps) {
            itemsHashMap.setIgnoreCase(true);
            if (itemsHashMap.containsValue(Define.BROWSE_STARTPAGE)) {
                MyHashMap insHashMap = new MyHashMap();
                insHashMap.putAll(itemsHashMap);
                insHashMap.putAll(mRootHashMap);
                startpagelist.add(insHashMap);
            } else {
                MyHashMap insHashMap = new MyHashMap();
                insHashMap.putAll(itemsHashMap);
                insHashMap.putAll(mRootHashMap);
                bookmarklist.add(insHashMap);
            }
        }
        // add for bug 485744 begin
        addOtherStartPage(startpagelist, pxLogicalList);
        // add for bug 485744 end
        /*
         * System.out.println("------------build-----------------");
         * System.out.println(bookmarklist); System.out.println(startpagelist);
         * System.out.println("------------build-----------------");
         */
    }

    // add for bug 485744 begin
    private void addOtherStartPage(List<MyHashMap> startPageList, List<MyHashMap> pxLogicalList) {
        if (null == startPageList || null == pxLogicalList) {
            System.out.println("startPageList or pxLogicalList is null");
            return;
        }
        for (MyHashMap myHashMap : pxLogicalList) {
            if (myHashMap.containsKey(Define.PARM_NODE_ATTR_STARTPAGE)) {
                System.out.println("containsKey(Define.PARM_NODE_ATTR_STARTPAGE)");
                if (myHashMap.get(Define.PARM_NODE_ATTR_STARTPAGE) != null) {
                    System.out.println("Define.PARM_NODE_ATTR_STARTPAGE' value is not null");
                    MyHashMap startPageMap = new MyHashMap();
                    startPageMap.put("uri", myHashMap.get(Define.PARM_NODE_ATTR_STARTPAGE));
                    startPageList.add(startPageMap);
                }
            } else {
                System.out.println("not containsKey(Define.PARM_NODE_ATTR_STARTPAGE)");
            }
        }
    }

    // add for bug 485744 end

    // add for 485676 begin
    public String getRuleKey(INode node) {
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

    // add for 485676 end

    private MyHashMap mRootHashMap = new MyHashMap();
    // add for bug 518797 begin
    private List<MyHashMap> mOtherHashMaps = new ArrayList<MyHashMap>();
    // add for bug 518797 end
    private List<MyHashMap> mItemHashMaps = new ArrayList<MyHashMap>();
}

class RootVisitor extends BaseVisitorImpl {
    @Override
    public int visitor(INode node, Object obj) {
        // add for 485676 begin
        String ruleKeyStr = "";
        // add for 485676 end
        VisitorParam param = null;
        if (node == null || obj == null) {
            return IVisitor.VISITOR_ERR_PARAM;
        }

        // add for 485676 begin
        if (!(obj instanceof VisitorParam)) {
            return IVisitor.VISITOR_ERR_PARAM;
        }
        param = (VisitorParam) obj;
        if (node.getParent() != null
                && node.getParent().getName().equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)) {
            ruleKeyStr = param.getRuleKey(node.getParent());
        }
        // add for 485676 end

        node.getAttribute().setIgnoreCase(true);
        if (node.getName().equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)
                && node.getAttribute().containsValue(Define.CHAR_NODE_ATTR_APPLICATION)) {
            return IVisitor.VISITOR_CONTINUE;
        }

        // jump the resource node
        // add for bug 518797 begin
        if (node.getName().equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)
                && !node.getAttribute().containsValue(Define.CHAR_NODE_ATTR_APPLICATION)) {
            return IVisitor.VISITOR_BROTHER;
        }
        // add for bug 518797 end
        String key = node.getAttribute().get(Define.PARM_NODE_ATTR_NAME).toLowerCase();
        // add for bug 483895 begin
        String value = node.getAttribute().get(Define.PARM_NODE_ATTR_VALUE);
        // add for bug 483895 end
        if (isNeedRename(key, getRenameArray()) && getRenameFlag()) {
            key = getRenameKey(node, Define.PARM_NODE_ATTR_NAME);
        }

        // add for 485676 begin
        boolean containRuleKey = false;
        boolean setsContainKey = false;
        if (RuleManager.createInstance().getRuleMaps() != null
                && RuleManager.createInstance().getRuleMaps().get(ruleKeyStr) != null) {
            System.out.println("-------------has RuleMap and has RuleSets-------------");
            containRuleKey = RuleManager.createInstance().getRuleMaps().containsKey(ruleKeyStr);
            setsContainKey = RuleManager.createInstance().getRuleMaps().get(ruleKeyStr)
                    .contains(key);
        }

        System.out.println("containRuleKey = " + containRuleKey + ", setsContainKey = "
                + setsContainKey);
        if (containRuleKey && setsContainKey && param.getRootHashMap().containsKey(key)) {
            // 1~* or 0~* not cover
            System.out.println("----------------------------add M params, the key = " + key
                    + "-------------------");
            int keyCount = 0;
            for (String keyItem : param.getRootHashMap().keySet()) {
                if (keyItem.contains(key)) {
                    keyCount++;
                }
            }
            System.out.println("keyCount = " + keyCount);
            param.getRootHashMap().put(key + keyCount, value);

        } else if (param.getRootHashMap().containsKey(key)) {
            // whether contains the key,if yes, do nothing
            // return IVisitor.VISITOR_CONTINUE;
        } else {
            // save hashMap data to UserData first time
            param.getRootHashMap().put(key, value);
        }
        // add for 485676 end
        // param.getRootHashMap().put(key,value);
        return IVisitor.VISITOR_BROTHER;
    }
}

class ResourceVisitor extends BaseVisitorImpl {

    @Override
    public int visitor(INode node, Object obj) {

        VisitorParam param = null;
        if (node == null || obj == null) {
            return IVisitor.VISITOR_ERR_PARAM;
        }
        // Application node continue visit
        node.getAttribute().setIgnoreCase(true);
        if (node.getName().equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)
                && node.getAttribute().containsValue(Define.CHAR_NODE_ATTR_APPLICATION)) {
            return IVisitor.VISITOR_CONTINUE;
        }
        // jump parm node
        // add for bug 518797 begin
        if (!node.getName().equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)
                || !node.getAttribute().containsValue(Define.CHAR_NODE_ATTR_RESOURCE)) {
            return IVisitor.VISITOR_BROTHER;
        }
        // add for bug 518797 end

        if (!(obj instanceof VisitorParam)) {
            return IVisitor.VISITOR_ERR_PARAM;
        }
        param = (VisitorParam) obj;
        // add All Childs
        MyHashMap ins = new MyHashMap();
        List<INode> childNodes = node.getChild();
        for (INode itemNodes : childNodes) {
            // System.out.println(" \r\n debug Attribute");
            // itemNodes.getAttribute().Debug();
            // add for bug 485676 begin
            if (itemNodes.getAttribute().get(Define.PARM_NODE_ATTR_NAME) == null) {
                Log.e("ResourceVisitor", "resource error !");
                return IVisitor.VISITOR_ERR_PARAM;
            }
            // add for bug 485676 end
            String key = itemNodes.getAttribute().get(Define.PARM_NODE_ATTR_NAME).toLowerCase();

            if (isNeedRename(key, getRenameArray()) && getRenameFlag()) {
                key = getRenameKey(itemNodes, Define.PARM_NODE_ATTR_NAME).toLowerCase();
            }
            if (itemNodes.getAttribute().containsKey(Define.PARM_NODE_ATTR_VALUE)) {
                // add for bug 483895 begin
                String value = itemNodes.getAttribute().get(Define.PARM_NODE_ATTR_VALUE);
                // add for bug 483895 end
                // add for 485676 begin
                if (!ins.containsKey(key)) {
                    ins.put(key, value);
                }
            } else {
                // startpage has no value attribute
                if (!ins.containsKey(key)) {
                    ins.put(key, "");
                }
                // add for 485676 end
            }
        }
        param.getBrowserHashMap().add(ins);

        return IVisitor.VISITOR_BROTHER;
    }
}

// add for bug 518797 begin
/*
 * parse the characteristic node which is not RESOURCE , for example :
 * <characteristic type="APPADDR"> <parm name="ADDR" value=""/>
 * </characteristic>
 */
class OtherNodeVisitor extends BaseVisitorImpl {

    @Override
    public int visitor(INode node, Object obj) {
        // add for 485676 begin
        String ruleKeyStr = "";
        // add for 485676 end
        VisitorParam param = null;
        if (node == null || obj == null) {
            return IVisitor.VISITOR_ERR_PARAM;
        }
        if (!(obj instanceof VisitorParam)) {
            return IVisitor.VISITOR_ERR_PARAM;
        }
        param = (VisitorParam) obj;

        // Application node continue visit
        node.getAttribute().setIgnoreCase(true);
        if (node.getName().equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)
                && node.getAttribute().containsValue(Define.CHAR_NODE_ATTR_APPLICATION)) {
            return IVisitor.VISITOR_CONTINUE;
        }
        // jump parm node
        // add for 485676 begin
        if (!node.getName().equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)
                || node.getAttribute().containsValue(Define.CHAR_NODE_ATTR_RESOURCE)) {
            return IVisitor.VISITOR_BROTHER;
        }
        // add for 485676 end

        // add All Childs
        MyHashMap ins = new MyHashMap();
        List<INode> childNodes = node.getChild();
        for (INode itemNodes : childNodes) {
            // System.out.println(" \r\n debug Attribute");
            // itemNodes.getAttribute().Debug();
            // add for 485676 begin
            String key = "";
            String value = "";
            if (itemNodes.getName().equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)) {
                for (INode item : itemNodes.getChild()) {
                    if (item.getAttribute().get(Define.PARM_NODE_ATTR_NAME) != null) {
                        key = item.getAttribute().get(Define.PARM_NODE_ATTR_NAME).toLowerCase();
                        if (item.getAttribute().containsKey(Define.PARM_NODE_ATTR_VALUE)) {
                            value = item.getAttribute().get(Define.PARM_NODE_ATTR_VALUE);
                        }
                        // System.out.println("-----key1 = " + key +
                        // ", value1 = " + value);
                        if (item.getParent() != null
                                && item.getParent().getName()
                                        .equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)) {
                            ruleKeyStr = param.getRuleKey(item.getParent());
                        }
                        boolean containRuleKey = false;
                        boolean setsContainKey = false;
                        if (RuleManager.createInstance().getRuleMaps() != null
                                && RuleManager.createInstance().getRuleMaps().get(ruleKeyStr) != null) {
                            System.out
                                    .println("-------------has RuleMap and has RuleSets-------------");
                            containRuleKey = RuleManager.createInstance().getRuleMaps()
                                    .containsKey(ruleKeyStr);
                            setsContainKey = RuleManager.createInstance().getRuleMaps()
                                    .get(ruleKeyStr).contains(key);
                        }

                        System.out.println("containRuleKey = " + containRuleKey
                                + ", setsContainKey = " + setsContainKey);
                        if (containRuleKey && setsContainKey && ins.containsKey(key)) {
                            // 1~* or 0бл* not cover
                            System.out
                                    .println("----------------------------add M params, the key = "
                                            + key + "-------------------");
                            int keyCount = 0;
                            for (String keyItem : ins.keySet()) {
                                if (keyItem.contains(key)) {
                                    keyCount++;
                                }
                            }
                            System.out.println("keyCount = " + keyCount);
                            ins.put(key + keyCount, value);

                        } else if (!ins.containsKey(key)) {
                            // save hashMap data to UserData first time,do
                            // nothing if contains key
                            ins.put(key, value);
                        }

                    }
                }
            }
            if (itemNodes.getParent() != null
                    && itemNodes.getParent().getName()
                            .equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)) {
                ruleKeyStr = param.getRuleKey(itemNodes.getParent());
            }
            // add for 485676 end
            if (itemNodes.getAttribute().get(Define.PARM_NODE_ATTR_NAME) != null) {
                key = itemNodes.getAttribute().get(Define.PARM_NODE_ATTR_NAME).toLowerCase();
            }

            if (isNeedRename(key, getRenameArray()) && getRenameFlag()) {
                key = getRenameKey(itemNodes, Define.PARM_NODE_ATTR_NAME).toLowerCase();
            }
            if (itemNodes.getAttribute().containsKey(Define.PARM_NODE_ATTR_VALUE)) {
                value = itemNodes.getAttribute().get(Define.PARM_NODE_ATTR_VALUE);
            }
            // add for 485676 begin
            boolean containRuleKey = false;
            boolean setsContainKey = false;
            if (RuleManager.createInstance().getRuleMaps() != null
                    && RuleManager.createInstance().getRuleMaps().get(ruleKeyStr) != null) {
                System.out.println("-------------has RuleMap and has RuleSets-------------");
                containRuleKey = RuleManager.createInstance().getRuleMaps().containsKey(ruleKeyStr);
                setsContainKey = RuleManager.createInstance().getRuleMaps().get(ruleKeyStr)
                        .contains(key);
            }

            System.out.println("containRuleKey = " + containRuleKey + ", setsContainKey = "
                    + setsContainKey);
            if (containRuleKey && setsContainKey && ins.containsKey(key)) {
                // 1~* or 0бл* not cover
                System.out.println("----------------------------add M params, the key = " + key
                        + "-------------------");
                int keyCount = 0;
                for (String keyItem : ins.keySet()) {
                    if (keyItem.contains(key)) {
                        keyCount++;
                    }
                }
                System.out.println("keyCount = " + keyCount);
                ins.put(key + keyCount, value);

            } else if (!ins.containsKey(key)) {
                // save hashMap data to UserData first time,do nothing if
                // contains key
                ins.put(key, value);
            }
            // add for 485676 end
        }
        param.getOtherHashMap().add(ins);

        return IVisitor.VISITOR_BROTHER;
    }
}

// add for bug 518797 end

public class BrowseVisitor extends BaseVisitorImpl {

    @Override
    public int visitor(INode node, Object obj) {
        System.out.println("enter BrowseVisitor visitor()");
        VisitorParam param = null;
        int nRet = IVisitor.VISITOR_FAILURE;

        if (node == null || obj == null) {
            return IVisitor.VISITOR_ERR_PARAM;
        }
        if (!(obj instanceof OtaMapData)) {
            System.out.println(TAG + "obj is not instanceof IFindParam");
            return IVisitor.VISITOR_ERR_PARAM;
        }

        if (node.getName().equalsIgnoreCase(Define.CHAR_NODE_CHARACTERISTIC)
                && node.getAttribute().containsValue(Define.CHAR_NODE_ATTR_APPLICATION)) {
            return IVisitor.VISITOR_CONTINUE;
        }

        final INode currentNode = node;
        param = new VisitorParam();

        getRootVisitor().setRenameFlag(getRenameFlag());
        getResourceVisitor().setRenameFlag(getRenameFlag());
        // add for bug 518797 begin
        getOtherNodeVisitor().setRenameFlag(getRenameFlag());
        // add for bug 518797 end
        nRet = currentNode.visitor(currentNode, param, getRootVisitor());
        if (nRet != IVisitor.VISITOR_BROTHER && nRet != IVisitor.VISITOR_CMP) {
            System.out.println("root return  [0x" + Integer.toHexString(nRet) + "]");
            return nRet;
        }

        nRet = currentNode.visitor(currentNode, param, getResourceVisitor());
        if (nRet != IVisitor.VISITOR_BROTHER && nRet != IVisitor.VISITOR_CMP) {
            System.out.println("resource return [0x" + Integer.toHexString(nRet) + "]");
            return nRet;
        }
        // add for bug 518797 begin
        nRet = currentNode.visitor(currentNode, param, getOtherNodeVisitor());
        if (nRet != IVisitor.VISITOR_BROTHER && nRet != IVisitor.VISITOR_CMP) {
            System.out.println("resource return [0x" + Integer.toHexString(nRet) + "]");
            return nRet;
        }
        // add for bug 518797 end

        param.build((OtaMapData) obj);

        return IVisitor.VISITOR_CMP;
    }

    private static final RootVisitor getRootVisitor() {
        return mRootVisitor;
    }

    private static final ResourceVisitor getResourceVisitor() {
        return mResourceVisitor;
    }

    // add for bug 518797 begin
    private static final OtherNodeVisitor getOtherNodeVisitor() {
        return mOtherNodeVisitor;
    }

    // add for bug 518797 end

    private final String TAG = "BrowseVisitor";
    private static final RootVisitor mRootVisitor = new RootVisitor();
    private static final ResourceVisitor mResourceVisitor = new ResourceVisitor();
    // add for bug 518797 begin
    private static final OtherNodeVisitor mOtherNodeVisitor = new OtherNodeVisitor();
    // add for bug 518797 end
}
