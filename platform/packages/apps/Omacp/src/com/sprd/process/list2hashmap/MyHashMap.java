
package com.sprd.process.list2hashmap;

import java.util.Set;
import java.util.HashMap;
import java.util.Collection;

public class MyHashMap extends HashMap<String, String> {

    public final void setUserData(Object obj) {
        mObject = obj;
    }

    public Object getUserData() {
        return mObject;
    }

    public boolean containsKey(Object key) {
        Set<String> keysetSet = null;
        String szTempKey = null;
        if (!IsIgnoreCase()) {
            return super.containsKey(key);
        } else if (!(key instanceof String)) {
            return super.containsKey(key);
        }

        szTempKey = (String) key;
        keysetSet = super.keySet();
        for (String szItem : keysetSet) {
            if (szTempKey.equalsIgnoreCase(szItem)) {
                return true;
            }
        }
        return false;
    }

    public boolean containsValue(Object value) {
        String szTempValue = null;
        if (!IsIgnoreCase()) {
            return super.containsKey(value);
        } else if (!(value instanceof String)) {
            return super.containsKey(value);
        }
        szTempValue = (String) value;
        Collection<String> setvalue = super.values();
        for (String szItem : setvalue) {
            if (szTempValue.equalsIgnoreCase(szItem)) {
                return true;
            }
        }
        return false;
    }

    public void setIgnoreCase(boolean bIgnoreCase) {
        mbIgnoreCase = bIgnoreCase;
    }

    public boolean IsIgnoreCase() {
        return mbIgnoreCase;
    }

    private Object mObject = null;
    private boolean mbIgnoreCase = false;
}
