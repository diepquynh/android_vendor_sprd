package com.android.internal.telephony;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import android.telephony.Rlog;
import android.util.Log;
import android.util.Xml;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Environment;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import com.android.internal.util.XmlUtils;

public class VolteConfig {

    private static final String TAG = VolteConfig.class.getSimpleName();
    static final String VOLTE_ENABLE_PATH ="etc/volte-conf.xml";
    static final String PREFERENCE_PACKAGE = "com.android.phone";
    // SPRD: Use file to save volte list for bug 595645.
    private List<String> mUserPlmnList = new ArrayList<String>();
    private Map<String, Map<String, String>> mPrebuiltConfigMap = new HashMap<String, Map<String, String>>();
    private ArrayList<String> mPrebuiltPlmn = new ArrayList<String>();
    /* SPRD: Use file to save volte list for bug 595645. @{ */
    private static final String PLMN_ENABLE_FILE_PATH = "/data/data/com.android.callsettings";
    private static final String PLMN_ENABLE_FILE_NAME = "voltelock-usr.xml";
    /* @} */
    // add disallow plmns
    private ArrayList<String> mDisAllowPlmns = new ArrayList<String>();

    public static String KEY_NUMERIC = "numeric";
    public static String KEY_ENABLE = "enable";
    public static String KEY_DOMAIN = "domain";
    public static String KEY_XCAP = "xcap";
    public static String KEY_BSF = "bsf";
    public static String KEY_CONFURI = "confuri";
    public static String KEY_CAMERAR_ESOLUTION = "cameraresolution";

    private static VolteConfig instance = null;
    public static VolteConfig getInstance(){
        if(instance == null){
            synchronized(VolteConfig.class){
                if(instance == null){
                    instance = new VolteConfig();
                }
            }
        }
        return instance;
    }

    public void loadVolteConfig(Context context){
        loadUserConfig(context);
        loadPrebuiltConfig();
    }

    public boolean containsCarrier(String carrier) {
        boolean prebuiltVolteEnable = getPrebuiltVolteEnable(carrier);
        if (mUserPlmnList.contains(carrier) || prebuiltVolteEnable) {
            return true;
        }
        return false;
    }

    public boolean getVolteEnable(String carrier) {
        boolean prebuiltVolteEnable = getPrebuiltVolteEnable(carrier);
        // SPRD: Use file to save volte list for bug 595645.
        if (mUserPlmnList.contains(carrier) || prebuiltVolteEnable) {
            return true;
        }
        return false;
    }

    public Map<String, String> getVolteConfig(String carrier){
        if(mPrebuiltConfigMap.containsKey(carrier)){
            Map<String, String> data = mPrebuiltConfigMap.get(carrier);
            return data;
         }
        return null;
    }
    // add disallow plmns
	public ArrayList<String> getDisAllowPlmns() {
		Rlog.w(TAG, "mDisAllowPlmns.size = " + mDisAllowPlmns.size());
		return mDisAllowPlmns;
	}

	public boolean isPlmnDisAllowed(String carrier) {
		Rlog.w(TAG, "mDisAllowPlmns.contains(carrier) = " + mDisAllowPlmns.contains(carrier));
		return mDisAllowPlmns.contains(carrier);
	}

    public String getCameraResolution(String carrier){
        if(mPrebuiltConfigMap.containsKey(carrier)){
            Map<String, String> data = mPrebuiltConfigMap.get(carrier);
            return data.get(KEY_CAMERAR_ESOLUTION);
        }
        return null;
    }

    public boolean getPrebuiltVolteEnable(String carrier){
        if(mPrebuiltConfigMap.containsKey(carrier)){
            Map<String, String> data = mPrebuiltConfigMap.get(carrier);
            return "true".equals(data.get(KEY_ENABLE));
        }
        return false;
    }

    public synchronized ArrayList getPrebuiltConfig(){
        synchronized(this){
            if(mPrebuiltPlmn.isEmpty()){
                loadPrebuiltConfig();
            }
        }
        return mPrebuiltPlmn;
    }

    /* SPRD: Use file to save volte list for bug 595645. @{ */
    private void loadUserConfig(Context context){
       try{
            File volteUserFile = new File(PLMN_ENABLE_FILE_PATH, PLMN_ENABLE_FILE_NAME);
            loadFileUserList(volteUserFile);
            Rlog.w(TAG, "loadUserConfig in List mUserPlmnList = "+ mUserPlmnList);
        } catch(Exception e) {
            Rlog.w(TAG, "can not loadUserConfig in List mUserPlmnList");
            e.printStackTrace();
        }
    }

    private void loadFileUserList(File volteFile) {
        FileReader volteReader;
        try {
            volteReader = new FileReader(volteFile);
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Can not open " + volteFile.getAbsolutePath());
            return;
        }
        try {
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(volteReader);
            XmlUtils.beginDocument(parser, "volteEnables");
            while (true) {
                XmlUtils.nextElement(parser);
                String name = parser.getName();
                if (!"volteEnable".equals(name)) {
                    break;
                }
                String numeric = parser.getAttributeValue(null, "numeric");
                Log.d(TAG, "numeric : " + numeric + "; existPlmn : "
                        + mUserPlmnList.contains(numeric));
                if (!mUserPlmnList.contains(numeric)) {
                    mUserPlmnList.add(numeric);
                }
            }
            volteReader.close();
        } catch (XmlPullParserException e) {
            Log.e(TAG, "Exception in volte-conf parser " + e);
        } catch (IOException e) {
            Log.e(TAG, "Exception in volte-conf parser " + e);
        }
    }
    /* @} */

    private void loadPrebuiltConfig() {
        if(mPrebuiltConfigMap != null && mPrebuiltConfigMap.size() > 0){
            return;
        }

        FileReader volteReader;

        File volteFile = new File(Environment.getRootDirectory(),
                VOLTE_ENABLE_PATH);

        try {
            volteReader = new FileReader(volteFile);
        } catch (FileNotFoundException e) {
            Rlog.w(TAG, "Can not open " + volteFile.getAbsolutePath());
            return;
        }

        try {
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(volteReader);
            XmlUtils.beginDocument(parser, "allowPlmns");

            while (true) {
                XmlUtils.nextElement(parser);

                String name = parser.getName();

                String numeric = parser.getAttributeValue(null, KEY_NUMERIC);
                if ("allowPlmn".equals(name)){

		            Map<String, String> data = new HashMap<String, String>();
		            data.put(KEY_ENABLE, parser.getAttributeValue(null, KEY_ENABLE));
		            data.put(KEY_DOMAIN, parser.getAttributeValue(null, KEY_DOMAIN));
		            data.put(KEY_XCAP, parser.getAttributeValue(null, KEY_XCAP));
		            data.put(KEY_BSF, parser.getAttributeValue(null, KEY_BSF));
		            data.put(KEY_CONFURI, parser.getAttributeValue(null, KEY_CONFURI));
		            data.put(KEY_CAMERAR_ESOLUTION, parser.getAttributeValue(null, KEY_CAMERAR_ESOLUTION));

		            mPrebuiltConfigMap.put(numeric, data);
		            if("true".equals(data.get(KEY_ENABLE))){
		                mPrebuiltPlmn.add(numeric);
		            }
                } else if("disallowPlmn".equals(name)){
                	mDisAllowPlmns.add(numeric);
                	mPrebuiltConfigMap.remove(numeric);
                	mPrebuiltPlmn.remove(numeric);
                	Rlog.w(TAG, "disallow plmns: " + numeric);
                } else {
                	break;
                }
                
            }
        } catch (XmlPullParserException e) {
            Rlog.w(TAG, "Exception in volte-conf parser " + e);
        } catch (IOException e) {
            Rlog.w(TAG, "Exception in volte-conf parser " + e);
        } finally{
            try{
                if(volteReader != null){
                    volteReader.close();
                }
            }catch(IOException e){}
        }
    }
}
