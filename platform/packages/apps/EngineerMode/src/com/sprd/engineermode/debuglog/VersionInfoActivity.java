
package com.sprd.engineermode.debuglog;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.Activity;
import android.net.LocalSocketAddress;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.ServiceManager;
import android.os.RemoteException;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.engineermode.*;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.utils.SocketUtils;
import com.sprd.engineermode.utils.EMFileUtils;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

import java.io.File;

import com.sprd.engineermode.EngineerModeActivity;
import android.os.SystemProperties;

public class VersionInfoActivity extends Activity {
    private static final String TAG = "VersionInfoActivity";
    /*BEGIN BUG559596 zhijie.yang 2016/05/05 modify the filepath of gps version*/
    public static final String GPS_CSR_CG_CONFIG_FILE = "/data/cg/supl/supl.xml";
    public static final String GPS_CSR_GNSS_CONFIG_FILE = "/data/gnss/supl/supl.xml";
    public static final String GPS_CSR_CONFIG_FIL_FOR_GE2 = "/data/gnss/config/config.xml";
    public static final String TP_VERSION = "/sys/touchscreen/firmware_version";

    /* Gps chip type is ge or ge2 , default type is ge */
    private static final String DEFAULT_GPS_CHIP_TYPE = "ge";
    public static final String GPS_CHIP_TYPE = SystemProperties.get("ro.wcn.gpschip", DEFAULT_GPS_CHIP_TYPE);
    /*END BUG559596 zhijie.yang 2016/05/05 modify the filepath of gps version*/

    private static final String KEY_HARDWARE_VERSION = "hardware_version";
    private static final String KEY_AP_VERSION = "ap_version";
    private static final String KEY_MODEM_VERSION = "modem_version";
    private static final String KEY_PS_VERSION = "ps_version";
    private static final String KEY_DSP_VERSION = "dsp_version";
    private static final String KEY_CP2_VERSION = "cp2_version";
    private static final String KEY_GPS_VERSION = "gps_version";
    private static final String KEY_TP_VERSION = "tp_version";
    private static final String KEY_CMD_TYPE = "cmd_type";
    private static final String UNSUPPORT = "UNSOPPORT IN THIS VERSION";

    private static final int GET_MODEM_VERSION = 0;
    private static final int GET_PS_VERSION = 1;
    private static final int GET_DSP_VERSION = 2;
    private static final int GET_CP2_VERSION = 3;
    private static final int GET_TP_VERSION = 4;

    private TextView mVersionText;
    private TextView mTpVersionText;
    private String mCmdType;
    private String mVersion;
    private VersionHandler mVersionHandler;
    private Handler mUiThread = new Handler();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.version);
        mVersionText = (TextView) findViewById(R.id.version_id);
        mVersionText.setText("");
        mTpVersionText = (TextView) findViewById(R.id.tp_version_id);
        mTpVersionText.setText("");

        Bundle extras = getIntent().getExtras();
        //Modify 360277 by sprd
        if (extras == null) {
            Log.d(TAG,"no requirment version");
            return;
        }
        
        mCmdType = extras.getString(KEY_CMD_TYPE, null);
        Log.d(TAG,"require version "+mCmdType);
        if (mCmdType == null) {
            return;
        } 

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mVersionHandler = new VersionHandler(ht.getLooper());
    }

    @Override
    protected void onStart() {
        /* SPRD: fix bug395601 Engnieer mode crash @{*/
        if (mCmdType != null) {
            if (mCmdType.equals(KEY_MODEM_VERSION)) {
                Message modemVersion = mVersionHandler.obtainMessage(GET_MODEM_VERSION);
                mVersionHandler.sendMessage(modemVersion);
            } else if (mCmdType.equals(KEY_PS_VERSION)) {
                Message psVersion = mVersionHandler.obtainMessage(GET_PS_VERSION);
                mVersionHandler.sendMessage(psVersion);
            } else if (mCmdType.equals(KEY_DSP_VERSION)) {
                Message dspVersion = mVersionHandler.obtainMessage(GET_DSP_VERSION);
                mVersionHandler.sendMessage(dspVersion);
            } else if (mCmdType.equals(KEY_CP2_VERSION)) {
                Message cp2Version = mVersionHandler.obtainMessage(GET_CP2_VERSION);
                mVersionHandler.sendMessage(cp2Version);
            } else if (mCmdType.equals(KEY_TP_VERSION)){
                Message tPVersion = mVersionHandler.obtainMessage(GET_TP_VERSION);
                mVersionHandler.sendMessage(tPVersion);
            }else if(mCmdType.equals(KEY_GPS_VERSION)) {
                /*BEGIN BUG559596 zhijie.yang 2016/05/05 modify the filepath of gps version*/
                mVersionText.setText(getGPSVersionInfo());
                /*END BUG559596 zhijie.yang 2016/05/05 modify the filepath of gps version*/
            }
        }
        /* @}*/
        super.onStart();
    }

    @Override
    protected void onDestroy() {
        if (mVersionHandler != null) {
            mVersionHandler.getLooper().quit();
        }
        super.onDestroy();
    }    

    /*BEGIN BUG559596 zhijie.yang 2016/05/05 modify the filepath of gps version*/
    /**
     * get GPS version info from configuration file
     * <p/>
     * ge: file is /data/cg/supl/supl.xml , key is VERSION; ge2: file is
     * /data/gnss/config/config.xml, key is GE2-VERSION
     */
    private String getGPSVersionInfo() {
        String gpsVersion = "UNKNOWN";
        if ("ge".equals(GPS_CHIP_TYPE)) {
            gpsVersion = getGPSVersion(GPS_CSR_CG_CONFIG_FILE, "PROPERTY", "VERSION");
        } else if ("ge2".equals(GPS_CHIP_TYPE)) {
            gpsVersion = getGPSVersion(GPS_CSR_CONFIG_FIL_FOR_GE2, "PROPERTY", "GE2-VERSION");
        } else {
            Log.d(TAG, "No match for version " + GPS_CHIP_TYPE);
        }
        return gpsVersion;
    }
    /*END BUG559596 zhijie.yang 2016/05/05 modify the filepath of gps version*/

    private String getGPSVersion(String path, String elementName,String key) {
        String gpsVersion = "UNKNOWN";
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
        Log.d(TAG,"GPS Version is "+gpsVersion);
        return gpsVersion;
    }

    private String recombinResult (int cmdType, String res){
        String result = null;
        String str[] = res.split("\n");
        for(int i = 0; i < str.length - 1; i++) {
            if (cmdType == GET_MODEM_VERSION && str[i].contains("Platform Version")) {
                String str1[] = str[i].split("\\:");
                result = "Modem Version:"+"\n"+str1[1]+"\n"+"Release Date:"+"\n"+str[str.length - 2]+"\n";
            }
            // there are two space between BASE and Version
            else if (cmdType == GET_PS_VERSION && str[i].contains("BASE  Version")) {
                String str1[] = str[i].split("\\:");
                result = "PS Version:"+"\n"+str1[1]+"\n"+"Release Date:"+"\n"+str[str.length - 2]+"\n";
            }
        }
        if (cmdType == GET_CP2_VERSION) {
            if (res.startsWith("marlin")) {
                String debug = "Debug";
                if (res.contains("Release")) {
                    debug = "Release";
                }
                String str1[] = res.split("\\:");
                String cp2Version = str1[2].trim();
                int dateIndex = cp2Version.indexOf(debug) + debug.length();
                String version = cp2Version.substring(0, dateIndex);
                String time = cp2Version.substring(dateIndex) + ":"
                        + str1[3].trim() + ":" + str1[4].trim();
                result = "CP2 Version:" + "\n" + version + "\n"
                        + "Release Date:" + "\n" + time;
            } else {
                res = res.replaceAll("\\s+", "");
                if (res.startsWith("Platform")) {
                    final String PROC_VERSION_REGEX =
                            "PlatformVersion:(\\S+)" + "ProjectVersion:(\\S+)" + "HWVersion:(\\S+)";
                    Matcher m = Pattern.compile(PROC_VERSION_REGEX).matcher(res);
                    if (!m.matches()) {
                        Log.e(TAG, "Regex did not match on cp2 version: ");
                    } else {
                        String dateTime = m.group(3);
                        String modem = "modem";
                        int endIndex = dateTime.indexOf(modem) + modem.length();
                        String subString1 = dateTime.substring(0, endIndex);
                        String subString2 = dateTime.substring(endIndex);
                        String time = subString2.substring(10);
                        String date = subString2.substring(0, 10);
                        result = m.group(1) + "|" + m.group(2) + "|" + subString1 + "|" + date + " "
                                + time;
                    }
                }
            }
        }
        return result;
    }

    class VersionHandler extends Handler {

        public VersionHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String result = null;
            String dspVersionResult = null;
            switch (msg.what) {
                case GET_MODEM_VERSION:
                    result = IATUtils.sendATCmd(engconstents.ENG_AT_CGMR, "atchannel1");
                    Log.d(TAG,"Mode Version result is "+result);
                    if (result.contains(IATUtils.AT_OK)) {
                        mVersion = recombinResult(GET_MODEM_VERSION,result);               
                    } else {
                        Toast.makeText(VersionInfoActivity.this, "Get Modem Version Fail", Toast.LENGTH_SHORT).show(); 
                    }
                    break;
                case GET_PS_VERSION:
                    result = IATUtils.sendATCmd(engconstents.ENG_AT_CGMR, "atchannel1");
                    Log.d(TAG,"PS Version result is "+result);
                    if (result.contains(IATUtils.AT_OK)) {   
                        mVersion = recombinResult(GET_PS_VERSION,result);    
                    } else {
                        Toast.makeText(VersionInfoActivity.this, "Get PS Version Fail", Toast.LENGTH_SHORT).show(); 
                    }
                    break;
                case GET_DSP_VERSION:
                    String[] str;
                    String[] str1;
                    //GSM DSP Version
                    dspVersionResult = "GSM DSP Version Info:" + "\n"; 
                    result = IATUtils.sendATCmd(engconstents.ENG_AT_DSPVERSION+"0", "atchannel1");

                    str = result.split("\n");
                    if (result.contains("OK")) {
                        str1 = str[0].split("\\,");
                        mVersion = dspVersionResult + str1[1]+"\n\n";
                    } else {
                        mVersion = dspVersionResult + getString(R.string.feature_not_support) + "\n\n";
                    }
                    Log.d(TAG,"DSPVersion GSM is "+result);

                    //TD DSP Version
                    dspVersionResult = "TD DSP Version Info:" + "\n"; 
                    result = IATUtils.sendATCmd(engconstents.ENG_AT_DSPVERSION+"1", "atchannel1");
                    str = result.split("\n");
                    if (result.contains("OK")) {
                        str1 = str[0].split("\\,");
                        mVersion = mVersion + dspVersionResult + str1[1]+"\n\n";
                    } else {
                        mVersion = mVersion + dspVersionResult + getString(R.string.feature_not_support) +"\n\n";
                    }
                    Log.d(TAG,"DSPVersion TD is "+result);

                    //WCDMA DSP Version
                    dspVersionResult = "WCDMA DSP Version Info:" + "\n"; 
                    result = IATUtils.sendATCmd(engconstents.ENG_AT_DSPVERSION+"2", "atchannel1");
                    str = result.split("\n");
                    if (result.contains("OK")) {
                        str1 = str[0].split("\\,");
                        mVersion = mVersion + dspVersionResult + str1[1]+"\n\n";
                    } else {
                        mVersion = mVersion + dspVersionResult + getString(R.string.feature_not_support) +"\n\n";
                    }
                    Log.d(TAG,"DSPVersion WCDMA is "+result);

                    //LTE DSP Version
                    dspVersionResult = "LTE DSP Version Info:" + "\n"; 
                    result = IATUtils.sendATCmd(engconstents.ENG_AT_DSPVERSION+"3", "atchannel1");
                    str = result.split("\n");
                    if (result.contains("OK")) {
                        str1 = str[0].split("\\,");
                        mVersion = mVersion + dspVersionResult + str1[1]+"\n\n";
                    } else {
                        mVersion = mVersion + dspVersionResult + getString(R.string.feature_not_support) +"\n\n";
                    }
                    Log.d(TAG,"DSPVersion LTE is "+result);
                    break;
                case GET_CP2_VERSION:
                    String powerResult = SocketUtils.sendCmdAndRecResult("wcnd",
                            LocalSocketAddress.Namespace.ABSTRACT, "wcn poweron");
                    Log.d(TAG,"OPEN_CP2 Response is " + powerResult);
                    if (powerResult != null && powerResult.contains(SocketUtils.OK)) {
                        EngineerModeActivity.isCP2On = true;
                    }
                    result = SocketUtils.sendCmdAndRecResult("wcnd",
                            LocalSocketAddress.Namespace.ABSTRACT, "wcn at+spatgetcp2info");
                    Log.d(TAG,"CP2 Version is "+result);
                    if (result != null) {
                        mVersion = recombinResult(GET_CP2_VERSION,result);    
                    } else {
                        Toast.makeText(VersionInfoActivity.this, "Get CP2 Version Fail", Toast.LENGTH_SHORT).show(); 
                    }
                    break;
                case GET_TP_VERSION:
                    mVersion = EMFileUtils.read(TP_VERSION);
                    Log.d(TAG,"Tp Version is "+mVersion);
                    if(mVersion == null){
                        mVersion = UNSUPPORT;
                    }
                    break;
                default:
                    break;
            }
            Log.d(TAG,"mVersion is "+mVersion);
            mUiThread.post(new Runnable() {
                @Override
                public void run() {
                    mVersionText.setText(mVersion);
                }
            });
        }
    }
}
