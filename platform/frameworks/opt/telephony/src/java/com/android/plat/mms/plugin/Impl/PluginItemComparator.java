
package com.android.plat.mms.plugin.Impl;

import java.util.Comparator;
import com.android.plat.mms.plugin.Impl.PluginItem;

public class PluginItemComparator implements Comparator<PluginItem> {

    @Override
    public int compare(PluginItem object1, PluginItem object2) {
        return object1.getPriority() - object2.getPriority();
    }
}
