
package com.sprd.process.list2hashmap;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
import com.sprd.xml.parser.itf.IFindParam;

public class MapFindParam implements IFindParam {

    public MapFindParam(int nType, boolean renameFlag, List<String> list) {

        mType = nType;
        mRenameFlag = renameFlag;
        setAttrList(list);
        if (mlistAttr == null) {
            mlistAttr = new ArrayList<String>();
            mlistAttr.add(DEFAULT_ATTR_NAME);
        }
        setNodeName(list.get(0));
    }

    @Override
    public int getType() {
        return mType;
    }

    @Override
    public void Debug() {
        System.out.println("\nbefore----------- type : " + getType() + "-------------\n");
        if (null == getUserData()) {
            System.out.println(TAG + "the UserData is null");
            return;
        }
        if (!(getUserData() instanceof HashMap)) {
            System.out.println(TAG + "UserData is not instance of List");
            return;
        }

        Set<String> szKeyset = ((HashMap<String, String>) getUserData()).keySet();
        System.out.println("the size of HashMap = " + szKeyset.size());
        for (String item : szKeyset) {
            System.out.print("" + item + " = "
                    + ((HashMap<String, String>) getUserData()).get(item) + " ");
        }
        System.out.println("\n----------- type : " + getType() + "-------------end\n");
    }

    @Override
    public void setUserData(Object obj) {
        mObject = obj;
    }

    @Override
    public Object getUserData() {
        return mObject;
    }

    public void setRenameFlag(boolean flag) {
        mRenameFlag = flag;
    }

    public void setNodeName(String szNodeName) {
        mNodeName = szNodeName;
    }

    public String getNodeName() {
        return mNodeName;
    }

    public void setAttrList(List<String> list) {
        mlistAttr = list;
    }

    public List<String> getAttrList() {
        return mlistAttr;
    }

    public boolean getRenameFlag() {
        return mRenameFlag;
    }

    public String[] getRenameArray() {
        return mRenameKeyArray;
    }

    private int mType;
    private String mNodeName = null;
    private List<String> mlistAttr;
    private boolean mRenameFlag = false;
    private Object mObject = null;
    private final String TAG = "MapFindParam";
    private static final String DEFAULT_ATTR_NAME = "name";
    private String[] mRenameKeyArray = new String[] {
            "name", "enabled", "filter", "replace"
    };

}
