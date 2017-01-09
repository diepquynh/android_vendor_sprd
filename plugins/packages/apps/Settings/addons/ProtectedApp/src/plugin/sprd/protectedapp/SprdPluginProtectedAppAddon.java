package plugin.sprd.protectedapp;

import android.app.AddonManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.content.res.XmlResourceParser;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Pair;
import android.util.Xml;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map.Entry;
import java.util.TreeMap;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import com.android.settings.fuelgauge.SprdPluginProtectedAppAddonStub;

public class SprdPluginProtectedAppAddon extends SprdPluginProtectedAppAddonStub
    implements AddonManager.InitialCallback {
    private static final String TAG = "SprdPluginProtectedAppAddon";

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        Log.d(TAG, "[onCreateAddon]:: ");
        return clazz;
    }

    @Override
    protected boolean isDefault() {
        Log.d(TAG, "[isDefault]:: ");
        return true;
    }
}
