
package com.sprd.xml.parser.impl;

import java.util.Collection;
import java.util.HashMap;
import java.util.Set;

import com.sprd.xml.parser.prv.PrintLogHelper;

public class SAXAttribute extends HashMap<String, String> {
    private static final long serialVersionUID = -7060210544600464482L;

    public void Debug() {
        Set<String> szKeyset = keySet();
        for (String item : szKeyset) {
            System.out.print("" + item + " = '" + this.get(item) + "'  ");
            PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).print(
                    "" + item + " = '" + this.get(item) + "'  ");
        }
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

    private boolean mbIgnoreCase = false;
    private static final String TAG = "SAXAttribute";
}
