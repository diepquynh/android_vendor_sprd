
package com.sprd.process.list2hashmap;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import com.sprd.process.dom2list.OtaListData;
import com.sprd.xml.parser.impl.BaseVisitorImpl;
import com.sprd.xml.parser.itf.IFindParam;
import com.sprd.xml.parser.itf.INode;
import com.sprd.xml.parser.itf.IVisitor;
import com.sprd.xml.parser.prv.Define;

public class List2MapMgr {

    private List2MapMgr() {
        mMapData = new OtaMapData();
        mDepthVisitor = new DepthVisitor();
        mBrowseVisitor = new BrowseVisitor();
    }

    synchronized public static List2MapMgr getInstance() {
        if (mInstance == null) {
            mInstance = new List2MapMgr();
        }
        return mInstance;
    }

    synchronized public static void releaseIns() {
        if (mInstance != null) {
            mInstance = null;
        }
    }

    public int list2MapProcess(OtaListData listData) {
        for (IFindParam param : PARAM) {
            getMapProcess(param, listData.get(param.getType()));
        }
        return Define.STATE_OK;
    }

    public int getMapProcess(IFindParam param, List<INode> list) {

        if (param == null || list == null || getDepthVisitor() == null) {
            System.out.println(TAG + " the param or list is null");
            return Define.STATE_ERROR;
        }
        if (!(param instanceof MapFindParam)) {
            System.out.println(TAG + "param is not instanceof MapFindParam");
        }
        for (INode iNode : list) {
            HashMap<String, String> insHashMap = new MyHashMap();
            getOtaMapData().put(param.getType(), (MyHashMap) insHashMap);
            param.setUserData(insHashMap);
            if (isBrowseNode(iNode)) {
                getBrowseVisitor().setRenameFlag(((MapFindParam) param).getRenameFlag());
                iNode.visitor(iNode, getOtaMapData(), getBrowseVisitor());
            }
            // add for bug 483819 begin
            iNode.visitor(iNode, param, getDepthVisitor());
            // add for bug 483819 end
        }
        return Define.STATE_OK;
    }

    private boolean isBrowseNode(INode node) {
        if (null == node) {
            System.out.println("enter isBrowseNode(), node is null");
            return false;
        }
        for (INode iNode : node.getChild()) {
            if (iNode.getAttribute() != null) {
                iNode.getAttribute().setIgnoreCase(true);
            }

            if (iNode.getAttribute().containsValue(Define.APPID)
                    && iNode.getAttribute().containsValue(Define.BROWSE_APPID)) {
                return true;
            }
        }
        return false;
    }

    public void setDepthVisitor(IVisitor visitor) {
        mDepthVisitor = (DepthVisitor) visitor;
    }

    public IVisitor getDepthVisitor() {
        return mDepthVisitor;
    }

    public void setBrowseVisitor(IVisitor visitor) {
        mBrowseVisitor = (BaseVisitorImpl) visitor;
    }

    public BaseVisitorImpl getBrowseVisitor() {
        return mBrowseVisitor;
    }

    public OtaMapData getOtaMapData() {
        return mMapData;
    }

    public void clearData() {
        System.out.println("clear All MapData");
        getOtaMapData().clear();
        releaseIns();
    }

    public void Debug() {
        for (IFindParam param : PARAM) {
            param.Debug();
        }
    }

    public void setParamList(List<String> paramlist) {
        if (null == paramlist) {
            mParamList = new ArrayList<String>();
        }
        mParamList = paramlist;
    }

    public List<String> getParamList() {
        return mParamList;
    }

    private List<String> mParamList = null;
    private OtaMapData mMapData = null;
    private IVisitor mDepthVisitor = null;
    private BaseVisitorImpl mBrowseVisitor = null;
    private static List2MapMgr mInstance = null;
    private final String TAG = "List2MapMgr";

    /* Define findParam of the HashMap */
    private IFindParam PARAM[] = {
            new MapFindParam(Define.CHAR_NAPDEF, true, Arrays.asList(new String[] {
                    "parm", "name", "value"
            })), new MapFindParam(Define.CHAR_PXLOGICAL, true, Arrays.asList(new String[] {
                    "parm", "name", "value"
            })), new MapFindParam(Define.CHAR_BOOTSTRAP, true, Arrays.asList(new String[] {
                    "parm", "name", "value"
            })), new MapFindParam(Define.CHAR_APPLICATION, true, Arrays.asList(new String[] {
                    "parm", "name", "value"
            })), new MapFindParam(Define.CHAR_CLIENTIDENTITY, true, Arrays.asList(new String[] {
                    "parm", "name", "value"
            })), new MapFindParam(Define.CHAR_VENDORCONFIG, true, Arrays.asList(new String[] {
                    "parm", "name", "value"
            })), new MapFindParam(Define.CHAR_PORT, true, Arrays.asList(new String[] {
                    "parm", "name", "value"
            })), new MapFindParam(Define.CHAR_ACCESS, true, Arrays.asList(new String[] {
                    "parm", "name", "value"
            }))
    };

}
