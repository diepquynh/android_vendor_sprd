
package com.android.plat.mms.plugin.Impl;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Collections;
import org.xmlpull.v1.XmlPullParser;

import dalvik.system.DexClassLoader;

import android.content.Context;
import android.util.Log;
import android.util.Xml;
import com.android.plat.mms.plugin.Interface.INotify;
import com.android.plat.mms.plugin.Impl.PluginItemComparator;

public class XmlParserConfig {

    public XmlParserConfig(String path, Context context) {
        mPath = path;
        mList = new ArrayList<INotify>();
        mPluginItemList = new ArrayList<PluginItem>();
        setContext(context);
    }

    public List<INotify> parse() {
        System.out.println(TAG + " enter parse()");
        File xmlFile = new File(getPath());
        if (!xmlFile.exists() || !xmlFile.isFile()) {
            System.out
                    .println("file is not exist or is not file, It's name is [" + getPath() + "]");
            return null;
        }
        try {
            FileInputStream inputStream = new FileInputStream(xmlFile);
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(inputStream, "UTF-8");
            int eventType = parser.getEventType();
            PluginItem pluginItem = null;
            while (eventType != XmlPullParser.END_DOCUMENT) {
                switch (eventType) {
                    case XmlPullParser.START_DOCUMENT:

                        break;
                    case XmlPullParser.START_TAG:
                        String name = parser.getName();
                        if (name.equalsIgnoreCase("PluginItem") && parser.getAttributeCount() == 3) {
                            String priority = parser.getAttributeValue(0);
                            String jarPath = parser.getAttributeValue(1);
                            String className = parser.getAttributeValue(2);
                            pluginItem = new PluginItem(priority, jarPath, className);

                            if (getPluginItemList() != null) {
                                getPluginItemList().add(pluginItem);
                            }
                        }
                        break;
                    case XmlPullParser.END_TAG:
                        break;
                    default:
                        break;
                }
                eventType = parser.next();
            }

            inputStream.close();

        } catch (Exception e) {
            System.out.println("return null because of the Exception : " + e);
            return null;
        }
        PluginItemComparator comparator = new PluginItemComparator();
        Collections.sort(getPluginItemList(), comparator);
        return ReflectProcess(getPluginItemList());
    }

    public List<INotify> ReflectProcess(List<PluginItem> itemList) {
        System.out.println(TAG + " enter ReflectProcess()");
        if (itemList == null) {
            System.out.println(TAG + " List<PluginItem> is null");
            return null;
        }
        Debug();
        System.out.println(TAG + " itemList size() = " + itemList.size());
        for (PluginItem item : itemList) {
            File dexOutputDir = getContext().getDir("dex", 0);
            Log.e(TAG, "dexOutputDir:" + dexOutputDir.getAbsolutePath());// data/data/com.cryto/app_dex
            DexClassLoader cl = new DexClassLoader(item.getJarPath(),
                    dexOutputDir.getAbsolutePath(), "", this.getClass().getClassLoader());
            Class<?> c;
            try {
                c = cl.loadClass(item.getClassName());
                INotify notifyObj = null;
                Object obj = c.newInstance();
                if (!(obj instanceof INotify)) {
                    Log.e(TAG, "obj is not instanceof INotify");
                    continue;
                }
                Log.e(TAG, "add INotify Object");
                notifyObj = (INotify) obj;
                getNotifyList().add(notifyObj);
            } catch (Exception e) {
                Log.e(TAG, "Exception : " + e);
                return null;
            }
        }
        return getNotifyList();
    }

    public void Debug() {
        for (PluginItem item : getPluginItemList()) {
            item.Debug();
        }
    }

    public String getPath() {
        return mPath;
    }

    public void setContext(Context context) {
        mContext = context;
    }

    public Context getContext() {
        return mContext;
    }

    private List<INotify> getNotifyList() {
        return mList;
    }

    private List<PluginItem> getPluginItemList() {
        return mPluginItemList;
    }

    private Context mContext = null;
    private String mPath = null;
    private List<INotify> mList = null;
    private List<PluginItem> mPluginItemList = null;
    private final String TAG = "XmlParserConfig";
}
