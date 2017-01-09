
package com.sprd.ota.rule;

import java.util.ArrayList;

public class RuleList extends ArrayList<RuleItem> {
    public RuleList() {
        super();

    }

    public boolean add(RuleItem obj) {
        if (obj == null) {
            return false;
        }
        if (contains(obj.getName())) {
            return false;
        } else {
            return super.add(obj);
        }

    }

    public RuleItem find(String szKeyName) {
        if (szKeyName == null) {
            return getRuleItem();
        }
        for (RuleItem item : this) {
            if (item.equals(szKeyName)) {
                return item;
            }
        }
        return getRuleItem();

    }

    public boolean contains(Object obj) {
        String szKeyName = null;
        if (obj == null) {
            return false;
        } else if (obj instanceof String) {
            szKeyName = (String) obj;
        }

        for (RuleItem item : this) {
            if (item.equals(szKeyName)) {
                return true;
            }
        }
        return false;
    }

    private final RuleItem getRuleItem() {
        return mIns;
    }

    public void Debug() {
        for (RuleItem item : this) {
            item.Debug();
        }
    }

    private static final RuleItem mIns = new RuleItem("", RuleItem.UNKOWN);
}
