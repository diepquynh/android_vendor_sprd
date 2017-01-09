
package com.sprd.ota.rule;

public class RuleItem {
    public RuleItem(String szName, int nType) {
        mnType = nType;
        mszName = szName;
    }

    public String getName() {
        return mszName;
    }

    public int getType() {
        return mnType;
    }

    public boolean equals(Object obj) {
        String objStr = null;
        if (obj == null) {
            return false;
        } else if (obj instanceof String) {
            objStr = (String) obj;
            // return (getName().compareTo(objStr) == 0);
            return getName().equalsIgnoreCase(objStr);
        }
        return false;
    }

    public void Debug() {
        System.out.println("RuleItem name = [" + getName() + "]    type = [" + getType() + "]");
    }

    private String mszName;
    private final int mnType;

    public static final int UNKOWN = 0x00000000;

    public static final int ONE = 0x00000001;
    public static final int ZeroOrOne = 0x00000002;
    public static final int ZeroOrMore = 0x00000004;
    public static final int OneOrMore = 0x00000008;

}
