
package com.spreadtrum.sgps;

import java.io.File;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

import android.os.SystemProperties;
import android.util.Log;

public class SgpsUtils {

    private static final String TAG = SgpsUtils.class.getSimpleName();

    public static final String GPS_CSR_CG_CONFIG_FILE = "/data/cg/supl/supl.xml";
    public static final String GPS_CSR_GNSS_CONFIG_FILE = "/data/gnss/supl/supl.xml";
    public static final String GPS_CSR_CONFIG_FIL_FOR_GE2 = "/data/gnss/config/config.xml";

    /* Gps chip type is ge or ge2 , default type is ge */
    private static final String DEFAULT_GPS_CHIP_TYPE = "ge";
    public static final String GPS_CHIP_TYPE = SystemProperties.get("ro.wcn.gpschip",
            DEFAULT_GPS_CHIP_TYPE);

    /**
     * get GPS version info from configuration file
     * <p/>
     * ge: file is /data/cg/supl/supl.xml , key is VERSION; ge2: file is
     * /data/gnss/config/config.xml, key is GE2-VERSION
     */

    public static String getGPSVersionInfo() {
        String gpsVersion = "UNKNOWN";
        if ("ge".equals(GPS_CHIP_TYPE)) {
            gpsVersion = getValueFromXML(GPS_CSR_CG_CONFIG_FILE, "PROPERTY", "VERSION");
        } else if ("ge2".equals(GPS_CHIP_TYPE)) {
            gpsVersion = getValueFromXML(GPS_CSR_CONFIG_FIL_FOR_GE2, "PROPERTY", "GE2-VERSION");
        } else {
            Log.d(TAG, "No match for version " + GPS_CHIP_TYPE);
        }
        return gpsVersion;
    }

    /**
     * get value from configuration file.
     * 
     * @param filepath the file to read from
     * @param key which to get
     * @return get string value by key
     */

    private static String getValueFromXML(String filepath, String elementName, String key) {
        String gpsVersion = "UNKNOWN";
        Log.d(TAG, "filepath-> " + filepath + ", element name-> " + elementName + ", key-> " + key);
        try {
            DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
            DocumentBuilder db = dbf.newDocumentBuilder();
            File gpsConfig = new File(filepath);
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
            Log.e(TAG, "Exception ->" + e);
            e.printStackTrace();
        }
        return gpsVersion;
    }
}
