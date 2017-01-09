package com.android.carrierconfig;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.os.Build;
import android.os.PersistableBundle;
import android.service.carrier.CarrierIdentifier;
import android.service.carrier.CarrierService;
import android.telephony.CarrierConfigManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;

import com.android.internal.util.FastXmlSerializer;

public class DefaultCarrierConfigServiceEx extends CarrierService {

    private static final String TAG = "DefaultCarrierConfigServiceEx";

    private XmlPullParserFactory mFactory;

    public DefaultCarrierConfigServiceEx() {
        Log.d(TAG, "Service created");
        mFactory = null;
    }

    /**
     * Returns per-network overrides for carrier configuration.
     *
     * This returns a carrier config bundle appropriate for the given network by reading data from
     * files in our assets folder. First we look for a file named after the MCC+MNC of {@code id}
     * and then we read res/xml/vendor.xml. Both files may contain multiple bundles with filters on
     * them. All the matching bundles are flattened to return one carrier config bundle.
     */
    @Override
    public PersistableBundle onLoadConfig(CarrierIdentifier id) {
        Log.d(TAG, "Config being fetched");

        if (id == null) {
            return null;
        }


        PersistableBundle config = null;
        try {
            synchronized (this) {
                if (mFactory == null) {
                    mFactory = XmlPullParserFactory.newInstance();
                }
            }

            XmlPullParser parser = mFactory.newPullParser();
            String fileName = "carrier_config_" + id.getMcc() + id.getMnc() + ".xml";
            parser.setInput(getApplicationContext().getAssets().open(fileName), "utf-8");
            config = readConfigFromXml(parser, id);
        }
        catch (IOException | XmlPullParserException e) {
            Log.d(TAG, e.toString());
            // We can return an empty config for unknown networks.
            config = new PersistableBundle();
        }


        // Treat vendor.xml as if it were appended to the carrier config file we read.
        XmlPullParser vendorInput = getApplicationContext().getResources().getXml(R.xml.vendor);
        try {
            PersistableBundle vendorConfig = readConfigFromXml(vendorInput, id);
            config.putAll(vendorConfig);
        }
        catch (IOException | XmlPullParserException e) {
            Log.e(TAG, e.toString());
        }

        /**
         *  The vendor_ex.xml only exist when build overlay resource follow feature board.
         *  For example the CMCC product,if you need modify the a boolean value,add the value in the the vendor_ex.xml.
         *   path : vendor/sprd/feature_configs/cmcc/overlay/packages/apps/CarrierConfig/res/xml/vendor_ex.xml
         */
        try {
            Resources resource = getApplicationContext().getResources();
            int resId = resource.getIdentifier("vendor_ex", "xml", getPackageName());
            XmlPullParser vendorExInput =resource.getXml(resId);
            PersistableBundle vendorConfigEx = readConfigFromXml(vendorExInput, id);
            config.putAll(vendorConfigEx);
        }
        catch (IOException | XmlPullParserException |NotFoundException e) {
            Log.e(TAG, e.toString());
        }
        return config;
    }

    /**
     * Parses an XML document and returns a PersistableBundle.
     *
     * <p>This function iterates over each {@code <carrier_config>} node in the XML document and
     * parses it into a bundle if its filters match {@code id}. The format of XML bundles is defined
     * by {@link PersistableBundle#restoreFromXml}. All the matching bundles will be flattened and
     * returned as a single bundle.</p>
     *
     * <p>Here is an example document. The second bundle will be applied to the first only if the
     * GID1 is ABCD.
     * <pre>{@code
     * <carrier_config_list>
     *     <carrier_config>
     *         <boolean name="voicemail_notification_persistent_bool" value="true" />
     *     </carrier_config>
     *     <carrier_config gid1="ABCD">
     *         <boolean name="voicemail_notification_persistent_bool" value="false" />
     *     </carrier_config>
     * </carrier_config_list>
     * }</pre></p>
     *
     * @param parser an XmlPullParser pointing at the beginning of the document.
     * @param id the details of the SIM operator used to filter parts of the document
     * @return a possibly empty PersistableBundle containing the config values.
     */
    static PersistableBundle readConfigFromXml(XmlPullParser parser, CarrierIdentifier id)
            throws IOException, XmlPullParserException {
        PersistableBundle config = new PersistableBundle();
        Log.d(TAG, "read config from xml" );

        if (parser == null) {
          return config;
        }

        // Iterate over each <carrier_config> node in the document and add it to the returned
        // bundle if its filters match.
        PersistableBundle defaultConfig = new PersistableBundle();
        int event;
        while (((event = parser.next()) != XmlPullParser.END_DOCUMENT)) {
            if (event == XmlPullParser.START_TAG && "carrier_config".equals(parser.getName())) {
                /* SPRD: [Bug516965] Ignore fragment without attribute if any fragment with attribute matches. */
                if (parser.getAttributeCount() == 0) {
                    defaultConfig.putAll(PersistableBundle.restoreFromXml(parser));
                } else {
                    // Skip this fragment if it has filters that don't match.
                    if (!checkFilters(parser, id)) {
                        continue;
                    }
                    PersistableBundle configFragment = PersistableBundle.restoreFromXml(parser);
                    config.putAll(configFragment);
                }
            }
        }

        if (!id.isNetworkPreferred() && config.isEmpty()) {
            config.putAll(defaultConfig);
        }
        /* @} */

        return config;
    }

    /**
     * Checks to see if an XML node matches carrier filters.
     *
     * <p>This iterates over the attributes of the current tag pointed to by {@code parser} and
     * checks each one against {@code id} or {@link Build.DEVICE}. Attributes that are not specified
     * in the node will not be checked, so a node with no attributes will always return true. The
     * supported filter attributes are,
     * <ul>
     *   <li>mcc: {@link CarrierIdentifier#getMcc}</li>
     *   <li>mnc: {@link CarrierIdentifier#getMnc}</li>
     *   <li>gid1: {@link CarrierIdentifier#getGid1}</li>
     *   <li>gid2: {@link CarrierIdentifier#getGid2}</li>
     *   <li>spn: {@link CarrierIdentifier#getSpn}</li>
     *   <li>device: {@link Build.DEVICE}</li>
     * </ul>
     * </p>
     *
     * @param parser an XmlPullParser pointing at a START_TAG with the attributes to check.
     * @param id the carrier details to check against.
     * @return false if any XML attribute does not match the corresponding value.
     */
    static boolean checkFilters(XmlPullParser parser, CarrierIdentifier id) {
        /*
         * SPRD: [bug475223] special for network preferred config such as ecclist. Only need to match
         * attribute named "network_preferred" and do not care others. @{
         */
        if (id.isNetworkPreferred()) {
            return "true".equals(parser.getAttributeValue(null, "network_preferred"));
        }
        /* @} */
        boolean result = true;
        for (int i = 0; i < parser.getAttributeCount(); ++i) {
            String attribute = parser.getAttributeName(i);
            String value = parser.getAttributeValue(i);
            switch (attribute) {
                case "mcc":
                    result = result && value.equals(id.getMcc());
                    break;
                case "mnc":
                    result = result && value.equals(id.getMnc());
                    break;
                case "gid1":
                    result = result && value.equals(id.getGid1());
                    break;
                case "gid2":
                    result = result && value.equals(id.getGid2());
                    break;
                case "spn":
                    result = result && value.equals(id.getSpn());
                    break;
                case "device":
                    result = result && value.equals(Build.DEVICE);
                    break;
                // SPRD: [bug475223] add for mvno match type PNN.
                case "pnn":
                    result = result && value.equals(id.getPnn());
                    break;
                // SPRD： [bug475223] add for mvno match type IMSI.
                case "imsi":
                    result = result && imsiMatches(value, id.getImsi());
                    break;
                default:
                    Log.e(TAG, "Unknown attribute " + attribute + "=" + value);
                    result = false;
                    break;
            }
        }
        return result;
    }

    /**
     * SPRD: [bug475223] mvno match : IMSI Carrier is MVNO if the given two imsis can match
     * each other.
     */
    private static boolean imsiMatches(String imsiXML, String imsiSIM) {
        int len = imsiXML.length();
        int idxCompare = 0;

        if (len <= 0)
            return false;
        if (len > imsiSIM.length())
            return false;

        for (int idx = 0; idx < len; idx++) {
            char c = imsiXML.charAt(idx);
            if ((c == 'x') || (c == 'X') || (c == imsiSIM.charAt(idx))) {
                continue;
            } else {
                return false;
            }
        }
        return true;
    }
}
