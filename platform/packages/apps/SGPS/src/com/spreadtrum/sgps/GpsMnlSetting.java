
package com.spreadtrum.sgps;

import android.os.SystemProperties;

import android.util.Log;

import java.io.File;
import java.util.ArrayList;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

public class GpsMnlSetting {

    private static final String TAG = "SGPS/Mnl_Setting";
    public static final String PROP_VALUE_0 = "0";
    public static final String PROP_VALUE_1 = "1";
    public static final String PROP_VALUE_2 = "2";

    public static final String KEY_DEBUG_DBG2SOCKET = "debug.dbg2socket";
    public static final String KEY_DEBUG_NMEA2SOCKET = "debug.nmea2socket";
    public static final String KEY_DEBUG_DBG2FILE = "debug.dbg2file";
    public static final String KEY_DEBUG_DEBUG_NMEA = "debug.debug_nmea";
    public static final String KEY_BEE_ENABLED = "BEE_enabled";
    public static final String KEY_TEST_MODE = "test.mode";
    public static final String KEY_SUPLLOG_ENABLED = "SUPPLOG_enabled";

    private static final String MNL_PROP_NAME = "persist.radio.mnl.prop";
    private static final String GPS_CHIP_PROP = "gps.gps.version";

    private static final String DEFAULT_MNL_PROP = "0001100";
    private static ArrayList<String> sKeyList = null;

    public static String getChipVersion(String defaultValue) {
        String chipVersion = SystemProperties.get(GPS_CHIP_PROP);
        if (null == chipVersion || chipVersion.isEmpty()) {
            chipVersion = defaultValue;
        }
        return chipVersion;
    }

    public static String getGPSVersion(String path, String elementName,
            String key) {
        String gpsVersion = "UNKNOWN";
        Log.d(TAG, "path->" + path + ",element->" + elementName + ",key->"
                + key);
        try {
            DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
            DocumentBuilder db = dbf.newDocumentBuilder();
            File gpsConfig = new File(path);
            Document doc = db.parse(gpsConfig);

            NodeList nodeList = doc.getElementsByTagName(elementName);
            for (int i = 0; i < nodeList.getLength(); i++) {
                Element element = (Element) nodeList.item(i);
                if (element.getAttribute("NAME").equals(key)) {
                    gpsVersion = element.getAttribute("VALUE");
                    break;
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Exception->" + e);
            e.printStackTrace();
        }
        return gpsVersion;
    }

    public static void setMnlProp(String key, String value) {
        Log.v(TAG, "setMnlProp: " + key + " " + value);
        String prop = SystemProperties.get(MNL_PROP_NAME);
        if (null == sKeyList) {
            initList();
        }
        int index = sKeyList.indexOf(key);
        if (index != -1) {
            if (null == prop || prop.isEmpty()) {
                prop = DEFAULT_MNL_PROP;
            }
            if (prop.length() > index) {
                char[] charArray = prop.toCharArray();
                charArray[index] = value.charAt(0);
                String newProp = String.valueOf(charArray);
                SystemProperties.set(MNL_PROP_NAME, newProp);
                Log.v(TAG, "setMnlProp newProp: " + newProp);
            }
        }
    }

    public static String getMnlProp(String key, String defaultValue) {
        String result = defaultValue;
        String prop = SystemProperties.get(MNL_PROP_NAME);
        if (null == sKeyList) {
            initList();
        }
        int index = sKeyList.indexOf(key);
        Log.v(TAG, "getMnlProp: " + prop);
        if (null == prop || prop.isEmpty() || -1 == index
                || index >= prop.length()) {
            result = defaultValue;
        } else {
            char c = prop.charAt(index);
            result = String.valueOf(c);
        }
        Log.v(TAG, "getMnlProp result: " + result);
        return result;
    }

    private static void initList() {
        sKeyList = new ArrayList<String>();
        sKeyList.add(KEY_DEBUG_DBG2SOCKET);
        sKeyList.add(KEY_DEBUG_NMEA2SOCKET);
        sKeyList.add(KEY_DEBUG_DBG2FILE);
        sKeyList.add(KEY_DEBUG_DEBUG_NMEA);
        sKeyList.add(KEY_BEE_ENABLED);
        sKeyList.add(KEY_TEST_MODE);
        sKeyList.add(KEY_SUPLLOG_ENABLED);
    }

}
