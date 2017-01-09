
package com.sprd.omacp.transaction;

import java.io.ByteArrayInputStream;
import java.io.IOException;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import com.sprd.omacp.parser.WbxmlParser;
import com.sprd.xml.parser.impl.DefaultHandler;
import com.sprd.xml.parser.prv.Define;

import android.content.Context;
import android.text.TextUtils;
import android.util.Config;
import android.util.Log;

public class OtaDefaultParser {
    public OtaDefaultParser() {
        mParser = new WbxmlParser();
    }

    public int parse(Context context) {

        if (getDataStream() == null) {
            Log.e(TAG, "mWapPushDataStream is not set!");
            return Define.STATE_ERROR;
        }
        try {
            // setTables before setInput
            if (getMimeType().equalsIgnoreCase(CONTENT_MIME_TYPE_B_OTA_OMA)) {
                Log.e("jordan", "enter parse and MimeType is OMA");
                // OMACP TYPE
                getWbxmlParser().setTagTable(0, Define.TAG_TABLE_OTA);
                getWbxmlParser().setTagTable(1, Define.TAG_TABLE_OTA1);
                getWbxmlParser().setAttrStartTable(0, Define.ATTR_START_TABLE_OTA);
                getWbxmlParser().setAttrStartTable(1, Define.ATTR_START_TABLE_OTA1);
                getWbxmlParser().setAttrValueTable(0, Define.ATTR_VALUE_TABLE_OTA);
                getWbxmlParser().setAttrValueTable(1, Define.ATTR_VALUE_TABLE_OTA1);
            } else if (getMimeType().equalsIgnoreCase(CONTENT_MIME_TYPE_B_OTA_NOKIA_SETTINGS)
                    || getMimeType().equalsIgnoreCase(CONTENT_MIME_TYPE_B_OTA_NOKIA_BOOKMARKS)) {
                // NOKIA TYPE
                Log.e("jordan", "enter parse and MimeType is Nokia");
                getWbxmlParser().setTagTable(0, Define.NOKIA_TAG_TOKENS);
                getWbxmlParser().setTagTable(1, Define.NOKIA_TAG_TOKENS);
                getWbxmlParser().setAttrStartTable(0, Define.NOKIA_ATTRIBUTE_START_TOKENS);
                getWbxmlParser().setAttrStartTable(1, Define.NOKIA_ATTRIBUTE_START_TOKENS);
                getWbxmlParser().setAttrValueTable(0, null);
                getWbxmlParser().setAttrValueTable(1, null);
            }

            getWbxmlParser().setInput(getDataStream(), null);
            if (LOCAL_LOGV) {
                Log.i(TAG, "Document charset : " + getWbxmlParser().getInputEncoding());
            }

            int eventType = getWbxmlParser().getEventType();
            Log.i(TAG, "eventType =  " + eventType);
            while (eventType != XmlPullParser.END_DOCUMENT) {
                switch (eventType) {
                    case XmlPullParser.START_DOCUMENT:
                        Log.i(TAG, "Start document");
                        DefaultHandler.getInstance().startDocument();
                        break;
                    case XmlPullParser.START_TAG:
                        Log.i(TAG, "Start tag = " + getWbxmlParser().getName());
                        DefaultHandler.getInstance().startTag(getWbxmlParser());
                        break;
                    case XmlPullParser.END_TAG:
                        Log.i(TAG, "End tag = " + getWbxmlParser().getName());
                        DefaultHandler.getInstance().endTag();
                        break;
                    case XmlPullParser.TEXT:
                        Log.i(TAG, "Text = " + getWbxmlParser().getText());
                        String text = getWbxmlParser().getText();
                        if (!TextUtils.isEmpty(text)) {
                            DefaultHandler.getInstance().startText(text);
                        }
                        break;
                    default:
                        Log.i(TAG, "unknown event type =  " + eventType);
                        break;
                }
                eventType = getWbxmlParser().next();
            }
            // End Document
            Log.i(TAG, "End document");
            DefaultHandler.getInstance().endDocument();
            System.out.println("root name = " + DefaultHandler.getNodeIns().getRoot().getName());
        } catch (IOException e) {
            // if (LOCAL_LOGV) {
            Log.e(TAG, "IOException : " + e.toString());
            // }
            return Define.STATE_ERROR;
        } catch (XmlPullParserException e) {
            // if (LOCAL_LOGV) {
            Log.e(TAG, "XmlPullParserException", new Throwable());
            // }
            return Define.STATE_ERROR;
        }
        // add for bug 537265 begin
        catch (Exception e) {
            Log.e(TAG, "Exception", e);
            return Define.STATE_ERROR;
        } finally {
            getWbxmlParser().clearData();
        }
        // add for bug 537265 end
        return Define.STATE_OK;
    }

    public void setData(byte[] pushDataStream) {
        mOTADataStream = new ByteArrayInputStream(pushDataStream);
    }

    public void setMimeType(String type) {
        mMimeType = type;
    }

    private String getMimeType() {
        return mMimeType;
    }

    public ByteArrayInputStream getDataStream() {
        return mOTADataStream;
    }

    private WbxmlParser getWbxmlParser() {
        return mParser;
    }

    private String mMimeType = null;
    private WbxmlParser mParser = null;
    private static final boolean DEBUG = true;
    private static final String TAG = "OTADefaultParser";
    private ByteArrayInputStream mOTADataStream = null;
    private static final boolean LOCAL_LOGV = DEBUG ? Config.LOGD : Config.LOGV;
    public static final String CONTENT_MIME_TYPE_B_OTA_OMA = "application/vnd.wap.connectivity-wbxml";
    public static final String CONTENT_MIME_TYPE_B_OTA_NOKIA_SETTINGS = "application/x-wap-prov.browser-settings";
    public static final String CONTENT_MIME_TYPE_B_OTA_NOKIA_BOOKMARKS = "application/x-wap-prov.browser-bookmarks";
}
