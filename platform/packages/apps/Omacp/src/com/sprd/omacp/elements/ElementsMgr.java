
package com.sprd.omacp.elements;

import java.util.HashMap;

import com.sprd.xml.parser.itf.ISet;

public class ElementsMgr {

    private ElementsMgr() {
        InitEnv();
    }

    synchronized public static ElementsMgr CreateInstance() {
        if (mIns == null) {
            mIns = new ElementsMgr();
        }
        return mIns;
    }

    synchronized public static void ReleaseInstance() {
        if (mIns != null) {
            mIns = null;
        }
    }

    private void InitEnv() {
        for (int nItem : LIST) {
            ISet ins = OtaConfigFactory.CreateInstanceByType(nItem);
            if (ins != null) {
                ins.initEvn();
                mnList.put(nItem, ins);
            }
        }
    }

    public ISet getSetByType(int ntype) {
        if (getList().containsKey(ntype)) {
            return getList().get(ntype);
        } else {
            // log
            System.out.println("=====================================================");
            System.out.println("Debug : ======= " + "getSetByType()"
                    + "============>>> has no the type you had put in");
            System.out.println("=====================================================");
            System.out.println("\r\n");
            return null;
        }
    }

    public HashMap<Integer, ISet> getList() {
        return mnList;
    }

    private static ElementsMgr mIns = null;
    private final HashMap<Integer, ISet> mnList = new HashMap<Integer, ISet>();
    private final int LIST[] = {
            ISet.OTA_APN, ISet.OTA_EMAIL, ISet.OTA_BOOKMARK, ISet.OTA_STARTPAGE
    };
}
