/** Create by Spreadst */

package com.spreadst.lockscreen;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;

import org.xmlpull.v1.XmlPullParserException;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.XmlResourceParser;
import android.provider.Settings;
import android.util.Log;

public class Tools {

    private static int DEFAULT_LOCKSCREEN = 0;

    public static int getLockViewID(Context context) {

        return Settings.System.getInt(context.getContentResolver(),
                Constants.SYSTEM_SETTINGS_LOCKSTYLE, DEFAULT_LOCKSCREEN);
    }

    public static ArrayList<HashMap> parserXml(int resId, Resources res) {

        ArrayList<HashMap> elsList = new ArrayList<HashMap>();
        HashMap elsMapInfo = null;
        XmlResourceParser xrp = res.getXml(resId);
        try {
            while (xrp.getEventType() != XmlResourceParser.END_DOCUMENT) {
                if (xrp.getEventType() == XmlResourceParser.START_TAG) {
                    String tagname = xrp.getName();
                    if (tagname.endsWith(Constants.LOCKSCREEN_TAG_NAME)) {
                        Log.d("Tools", "xrp.getAttributeCount() = " + xrp.getAttributeCount());
                        elsMapInfo = new HashMap();
                        for (int i = 0; i < xrp.getAttributeCount(); i++) {
                            elsMapInfo.put(xrp.getAttributeName(i),
                                    xrp.getAttributeValue(i));
                        }
                        elsList.add(elsMapInfo);
                    }
                } else if (xrp.getEventType() == XmlResourceParser.END_TAG) {
                } else if (xrp.getEventType() == XmlResourceParser.TEXT) {
                }
                xrp.next();
            }
        } catch (XmlPullParserException e) {

        } catch (IOException e) {
            e.printStackTrace();
        }

        return elsList;

    }

}
