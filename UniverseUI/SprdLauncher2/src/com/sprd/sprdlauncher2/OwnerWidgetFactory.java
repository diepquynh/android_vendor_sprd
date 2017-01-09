/** Created by Spreadtrum */
package com.sprd.sprdlauncher2;

import android.content.Context;

public class OwnerWidgetFactory {
    public static OwnerWidgetInfo getInfo(Context context,String type) {
        if ("clock_widget".equals(type)) {
            //            return new WidgetClock();
        } else if ("contact_widget".equals(type)) {
            return new WidgetContact(context);
        } else if ("clocktwo_widget".equals(type)) {
            //            return new WidgetClockTwo();
        } else {
            return null;
        }
        return null;
    }

    static OwnerWidgetInfo getInfo(Context context,int type) {
        switch (type) {
            case 1:
                //            return new WidgetClock();
            case 2:
                return new WidgetContact(context);
            case 3:
                //            return new WidgetClockTwo();
            default:
                return null;
        }
    }
}
