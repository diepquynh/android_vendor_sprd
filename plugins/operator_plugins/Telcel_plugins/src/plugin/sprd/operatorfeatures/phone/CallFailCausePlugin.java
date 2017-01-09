package plugin.sprd.callFailCausePlugin;

import android.app.AddonManager;
import android.content.Context;
import android.content.res.Resources;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import com.android.internal.util.XmlUtils;

import java.io.IOException;
import java.util.HashMap;

import android.util.Log;
import com.android.phone.R;
import com.sprd.phone.CallFailCauseHelper;


public class CallFailCausePlugin extends CallFailCauseHelper implements AddonManager.InitialCallback {

    private static final String TAG = "CallFailCausePlugin";
    private static final String FIRST_ELEMELT_NAME = "resources";
    private static final String CAUSE_NUMBER = "number";
    private static final String TEXT_REQUIRED = "text_required";
    public static HashMap<Integer, String> textRequiredHashmap = new HashMap<Integer, String>();

    private Context mContext;


    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    public void CallerAddressPlugin() {

    }

    @Override
    public boolean isSupportCallFailCause() {
        return true;
    }

    @Override
    public void parserAttributeValue(Context context) {
        if (!textRequiredHashmap.isEmpty()) {
            Log.w(TAG, "parserAttributeValue not empty");
            return;
        }

        Log.i(TAG, "start  parserAttributeValue");
        Resources r = context.getResources();
        XmlPullParser parser = r.getXml(R.xml.networkvalue_config);

        if (parser != null) {
            try {
                XmlUtils.beginDocument(parser, FIRST_ELEMELT_NAME);
                XmlUtils.nextElement(parser);
                while (parser.getEventType() != XmlPullParser.END_DOCUMENT) {
                    int mCauseNumber = Integer.parseInt(parser.getAttributeValue(null, CAUSE_NUMBER));
                    String textRequired = parser.getAttributeValue(null, TEXT_REQUIRED);
                    textRequiredHashmap.put(mCauseNumber, textRequired);
                    XmlUtils.nextElement(parser);
                }
            } catch (XmlPullParserException e) {
                Log.e(TAG, "XmlPullParserException : " + e);
            } catch (IOException e) {
                Log.e(TAG, "IOException: " + e);
            }
        }
    }

    @Override
    public String getAttributeValue(Integer causeNumber) {
        String strRequired = null;
        if (textRequiredHashmap.containsKey(causeNumber)) {
            strRequired = textRequiredHashmap.get(causeNumber);
        }
        return strRequired;
    }

}
