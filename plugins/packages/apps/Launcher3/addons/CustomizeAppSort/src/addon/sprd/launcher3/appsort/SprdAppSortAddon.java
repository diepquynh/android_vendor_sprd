package addon.sprd.launcher3.appsort;

import android.app.AddonManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.res.TypedArray;
import android.content.res.XmlResourceParser;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Pair;
import android.util.Xml;

import com.android.launcher3.SprdAppSortAddonStub;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map.Entry;
import java.util.TreeMap;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

//SPRD: bug375748 2014-12-01 Feature customize app icon sort.
public class SprdAppSortAddon extends SprdAppSortAddonStub
    implements AddonManager.InitialCallback {

    private static final String TAG = "SprdAppSortAddon";

    private static final String ROOT_TAG = "apps";
    private static final String APP_TAG = "app";

    private static HashMap<Integer, Pair<String, String>> sCustomizePositions;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        if (sCustomizePositions == null) {
            sCustomizePositions = loadCustomizeAppPositions(context);
            // SPRD: bug375995 2014-12-01 Feature customize app icon sort refactor.
            mHasCustomizeData = (sCustomizePositions.size() > 0);
        }
        return clazz;
    }

    // SPRD: bug375995 2014-12-01 Feature customize app icon sort refactor.
    @Override
    protected void onSortApps(ArrayList<ComponentName> componentNames) {
        Log.d(TAG, "onSortApps customize implementation.");
        TreeMap<Integer, ComponentName> sortedMaps = new TreeMap<Integer, ComponentName>();

        // find the customize component in componentNames
        Pair<String, String> pair = null;
        Entry<Integer, Pair<String, String>> entry = null;

        for (ComponentName cn : componentNames) {
            Iterator<Entry<Integer, Pair<String, String>>> it = sCustomizePositions.entrySet().iterator();
            while(it.hasNext()) {
                entry = it.next();
                pair = entry.getValue();

                if (pair.first.equals(cn.getPackageName())) {
                    if (pair.second == null || pair.second.equals(cn.getClassName())) {
                        sortedMaps.put(entry.getKey(), cn);
                        break;
                    }
                }
            }
        }

        // remove the found component
        Iterator<Entry<Integer, ComponentName>> it = sortedMaps.entrySet().iterator();
        while(it.hasNext()) {
            componentNames.remove(it.next().getValue());
        }

        // insert at the customize position
        Entry<Integer, ComponentName> ent = null;
        it = sortedMaps.entrySet().iterator();
        while(it.hasNext()) {
            ent = it.next();
            if (ent.getKey() > componentNames.size()) {
                // append to last position
                componentNames.add(ent.getValue());
            } else {
                // insert at specific position
                componentNames.add(ent.getKey(), ent.getValue());
            }
        }
    }

    /**
     * Get customize app's position. The result is a map, the key indicate the
     * customize position, and the value is a pair of package name and class name.
     *
     * @param context
     * @return
     */
    private HashMap<Integer, Pair<String, String>> loadCustomizeAppPositions(Context context) {
        HashMap<Integer, Pair<String, String>> customizePositions = new HashMap<Integer, Pair<String, String>>();
        try {
            XmlResourceParser parser = context.getResources().getXml(R.xml.app_positions);
            AttributeSet attrs = Xml.asAttributeSet(parser);

            int eventType = parser.getEventType();
            while (eventType != XmlPullParser.END_DOCUMENT) {
                if(eventType == XmlPullParser.START_TAG) {
                    String tagName = parser.getName();
                    if (APP_TAG.equals(tagName)) {
                        final TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.SprdAppSort);
                        String pkgName = a.getString(R.styleable.SprdAppSort_pkgName);
                        String clsName = a.getString(R.styleable.SprdAppSort_clsName);
                        int position = a.getInteger(R.styleable.SprdAppSort_position, 0);

                        // package name must not be null or empty
                        if (pkgName != null && pkgName.length() != 0) {
                            customizePositions.put(position, new Pair<String, String>(pkgName, clsName));
                        }
                        a.recycle();
                    }
                }
                eventType = parser.next();
            }
        } catch (XmlPullParserException e) {
            Log.w(TAG, "parse xml failed", e);
        } catch (IOException e) {
            Log.w(TAG, "parse xml failed", e);
        } catch (RuntimeException e) {
            Log.w(TAG, "parse xml failed", e);
        }
        return customizePositions;
    }
}
