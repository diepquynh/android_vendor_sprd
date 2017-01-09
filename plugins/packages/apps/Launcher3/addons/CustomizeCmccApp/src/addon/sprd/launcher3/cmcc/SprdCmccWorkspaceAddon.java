package addon.sprd.launcher3.cmcc;

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

import com.android.launcher3.SprdCmccWorkspaceAddonStub;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map.Entry;
import java.util.TreeMap;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

public class SprdCmccWorkspaceAddon extends SprdCmccWorkspaceAddonStub
    implements AddonManager.InitialCallback {
    private static final String TAG = "SprdCmccWorkspaceAddon";

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
