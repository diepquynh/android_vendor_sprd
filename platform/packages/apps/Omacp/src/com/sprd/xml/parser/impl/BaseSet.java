
package com.sprd.xml.parser.impl;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

import com.sprd.xml.parser.itf.ISet;

public abstract class BaseSet implements ISet {

    public BaseSet(int nType) {
        mnType = nType;
    }

    public boolean initEvn() {
        mMainKey = null;
        mOptionKey = null;

        mMainKey = new HashSet<String>();
        mOptionKey = new HashSet<String>();
        return true;
    }

    @Override
    public int check(Set<String> obj) {
        if (obj == null || obj.size() <= 0) {
            return ER_PARAM;
        }

        if (!obj.containsAll(getMainKey())) {
            System.out.println(getMainKey());
            // print error;
            ErrorPrint(obj, getMainKey());
            return ER_LOSE_MAIN_KEY;

        }

        if (!getOptionKey().containsAll(obj)) {
            // print warning
            WarningPrint(obj, getMainKey());
            return OK_LOSE_OPT_KEY;
        }

        return BASE_OK;
    }

    private Set<String> getMainKey() {
        return mMainKey;
    }

    private Set<String> getOptionKey() {
        return mOptionKey;
    }

    protected void addMainKey(String szKey) {
        if (szKey != null) {
            mMainKey.add(szKey);
            mOptionKey.add(szKey);
        }
    }

    protected void addOptionKey(String szKey) {
        if (szKey != null) {
            mOptionKey.add(szKey);
        }
    }

    private void ErrorPrint(Set<String> srcSet, Set<String> desSet) {
        System.out.println("enter ErrorPrint()");
        desSet = new HashSet<String>();
        for (String key : getMainKey()) {
            if (!srcSet.contains(key)) {
                desSet.add(key);
            }
        }
        PrintHashMapSet("ErrorPrint : the omited main Elements :", desSet);
        desSet.clear();
        desSet = null;
    }

    private void WarningPrint(Set<String> srcSet, Set<String> desSet) {
        System.out.println("enter WarningPrint()");
        desSet = new HashSet<String>();
        for (String key : srcSet) {
            if (!getOptionKey().contains(key)) {
                desSet.add(key);
            }
        }
        PrintHashMapSet("Warning : different Elements :", desSet);
        desSet.clear();
        desSet = null;
    }

    public int getType() {
        return mnType;
    }

    public void debug() {
        System.out.println("Debug :[" + getClass().getSimpleName() + "] ");
        System.out.println("======= mnType=[" + getType() + "]============>>>");
        System.out.println("======= Elements List============>>>");
        // main
        PrintHashMapSet("Main Key", getMainKey());
        // option
        PrintHashMapSet("Option Key", getOptionKey());
        System.out.println("\r\n");
        System.out.println("\r\n");
        for (String mparam : getMainKey()) {

        }

    }

    private void PrintHashMapSet(String szTag, Set<String> set) {
        if (set == null)
            return;
        System.out.println("Debug : ======= " + szTag + "============>>>");
        for (String szItem : set) {
            System.out.print(szItem + "\t");
        }
    }

    private int mnType;
    private Set<String> mMainKey;
    private Set<String> mOptionKey;

}
