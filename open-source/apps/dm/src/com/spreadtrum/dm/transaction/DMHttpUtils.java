
package com.spreadtrum.dm.transaction;

import java.io.DataInputStream;
import java.io.IOException;
import java.net.SocketException;
import java.net.URI;
import java.net.URISyntaxException;

import org.apache.http.HttpEntity;
import org.apache.http.HttpHost;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.StatusLine;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.conn.params.ConnRouteParams;
import org.apache.http.params.HttpParams;
import android.net.http.AndroidHttpClient;
import android.content.Context;
import android.util.Log;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;

import org.apache.http.params.HttpProtocolParams;
import org.apache.http.Header;
import org.apache.http.auth.AUTH;
import org.apache.http.message.BufferedHeader;
import org.apache.http.util.CharArrayBuffer;
import org.apache.http.util.EncodingUtils;
import org.apache.commons.codec.binary.Base64;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.entity.ByteArrayEntity;
import org.apache.http.params.CoreConnectionPNames;

//TODO: to be confirm
//import com.spreadtrum.dm.DMNativeMethod;

//import com.android.pim.PIMGlobalInf;
//import com.android.pim.PimDefine;
//import com.android.pim.PimNativeMethod;
//import com.android.pim.task.PIMManager;

public class DMHttpUtils {
    private static final String TAG = "DMHttpUtils";

    public static final int HTTP_POST_METHOD = 1;

    public static final int HTTP_GET_METHOD = 2;

    public static final boolean DEBUG = true;

    // Definition for necessary HTTP headers

    private static final String HDR_KEY_HOST = "Host";

    private static final String HDR_KEY_CACHE_CONTROL = "Cache-Control";

    private static final String HDR_KEY_ACCEPT = "Accept";

    private static final String HDR_KEY_ACCEPT_LANGUAGE = "Accept-Language";

    private static final String HDR_VALUE_ACCEPT = "application/vnd.syncml+xml, application/vnd.syncml+wbxml";

    private static final String HTTP_WBXML_MIME_TYPES = "application/vnd.syncml+wbxml";

    private static final String HTTP_XML_MIME_TYPES = "application/vnd.syncml+xml";

    private static final String HDR_KEY_CACHE_CONTROL_VALUE = "private";

    // private static final String USER_AGENT = "CS7903/V1/2.0/2.0/1.1";
    // private static final String USER_AGENT = "HS-EG968/V1/2.0/2.0/1.1";//zhl
    // modify
    // private static final String USER_AGENT = "HS-T66";
    // private static final String USER_AGENT =
    // "HS-N56_TD/1.0 ThreadX/4.0 MOCOR/W09B Release/05.25.2010 Browser/NF3.5 Profile/MIDP-2.0 Configuration/CLDC-1.1";
    //private static final String USER_AGENT = "HS-T81_TD/1.0 Android/2.2 Release/05.10.2011 Browser/AppleWebKit533.1 Profile/MIDP-2.0 Configuration/CLDC-1.1";
    private static final String USER_AGENT = "Mozilla/5.0 (Linux; U; Android 4.4.2;  zh-cn; Coolpad 8019; Android/4.4.2; Release/04.21.2014)  AppleWebKit/534.30 (KHTML, like Gecko) Version/4.0 Mobile Safari/534.30";

    // private static final String USER_AGENT="Mozilla/5.0
    // (HS-EG968/CIC.6.01.01.00;U;Android/1.6;320*480;CTC/2.0)
    // pleWebKit/528.5";

    private static final boolean LOCAL_LOGV = false;

    // private PIMTransactionSetting mPIMTransactionsetting;
    // private PIMGlobalInf mPIMGlobalInf;
    // private DMPreferences mprefs;
    private Handler mCallBackHandler;

    private static final boolean PIM_TEST_VERSION = true;

    public DMHttpUtils(/* DMPreferences preferences, */Handler handler) {
        // mPIMTransactionsetting = setting;
        // mPIMGlobalInf = PIMInf;
        // mprefs = preferences;
        mCallBackHandler = handler;
    }

    protected byte[] httpConnection(Context context, String url, int port, byte[] data, int method,
            boolean isProxySet, String proxyHost, int proxyPort, String proxyUsr, String proxyPsw)
            throws IOException {

        if (DEBUG) {
            Log.v(TAG, "httpConnection: params list");
            Log.v(TAG, "\tmethod\t\t= "
                    + ((method == HTTP_POST_METHOD) ? "POST" : ((method == HTTP_GET_METHOD) ? "GET"
                            : "UNKNOWN")));
            Log.v(TAG, "\tisProxySet\t= " + isProxySet);
            Log.v(TAG, "\tproxyHost\t= " + proxyHost);
            Log.v(TAG, "\tproxyPort\t= " + proxyPort);
            Log.v(TAG, "\turl\t= " + url);
        }

        AndroidHttpClient client = null;

        try {
            // Make sure to use a proxy which supports CONNECT.

            HttpHost target = new HttpHost(Uri.parse(url).getHost(), port,
                    HttpHost.DEFAULT_SCHEME_NAME);
            Log.v(TAG, "\ttarget\t= " + target);
            client = createHttpClient(context);
            HttpRequest req = null;
            switch (method) {
                case HTTP_POST_METHOD:
                    // TODO:
                    // PIMProgressCallbackEntity entity = new
                    // PIMProgressCallbackEntity(context, data);
                    ByteArrayEntity entity = new ByteArrayEntity(data);
                    // Set request content type.
                    // if(mprefs.getEncodeType().equals("SML_WBXML")) {
                    // entity.setContentType(HTTP_WBXML_MIME_TYPES);
                    // } else if(mprefs.getEncodeType().equals("SML_XML")) {
                    entity.setContentType(HTTP_XML_MIME_TYPES);
                    // } else {
                    // Log.i(TAG, "unKnown encodeType.");
                    // return null;
                    // }
                    HttpPost post = new HttpPost(url);
                    // HttpPost post = new HttpPost(Uri.parse(url).getHost());
                    post.setEntity(entity);

                    req = post;
                    break;
                case HTTP_GET_METHOD:
                    req = new HttpGet(url);
                    // req = new HttpGet(Uri.parse(url).getHost());
                    break;
                default:
                    Log.e(TAG, "Unknown HTTP method: " + method + ". Must be one of POST["
                            + HTTP_POST_METHOD + "] or GET[" + HTTP_GET_METHOD + "].");
                    return null;
            }

            // Set route parameters for the request.
            HttpParams params = client.getParams();
            // HttpHost host = new HttpHost(proxyHost
            // /*Uri.parse(proxyHost).getHost()*/, proxyPort);
            // Log.v(TAG, "\thost\t= " + host);

            if (isProxySet) {
                ConnRouteParams.setDefaultProxy(params, new HttpHost(proxyHost /*
                                                                                * Uri.
                                                                                * parse
                                                                                * (
                                                                                * proxyHost
                                                                                * )
                                                                                * .
                                                                                * getHost
                                                                                * (
                                                                                * )
                                                                                */, proxyPort));
            }
            else
                ConnRouteParams.setDefaultProxy(params, 
                new HttpHost(Uri.parse(url).getHost(), port));
			
            req.setParams(params);

            // Set necessary HTTP headers for pim transmission.
            // req.addHeader(HDR_KEY_HOST, url + ":" + port);
            req.addHeader(HDR_KEY_HOST, url);
            req.addHeader(HDR_KEY_ACCEPT, HDR_VALUE_ACCEPT);

            req.addHeader(HDR_KEY_ACCEPT_LANGUAGE, "en");
            req.addHeader(authenticate(proxyUsr, proxyPsw));
            // req.addHeader(HDR_KEY_CACHE_CONTROL,
            // HDR_KEY_CACHE_CONTROL_VALUE);
            // req.addHeader("Connection", "keep-alive");
            req.addHeader("Connection", "keep-alive");

            HttpResponse response = client.execute(target, req);
            StatusLine status = response.getStatusLine();
            Log.i(TAG, "status : " + status);
            Log.d(TAG, "response is : " + response);
            // if(PIMManager.getCancelSyncFlag()){
            // return null;
            // }

            Log.d(TAG, "httpConnection status.getStatusCode() :" + status.getStatusCode());
            if (status.getStatusCode() != 200) { // HTTP 200 is success.
                Message msg;
                if (status.getStatusCode() == 302) {
                    Log.d(TAG, "you have a auth error ");
                    msg = mCallBackHandler.obtainMessage(DMDefine.PIM_EVENT.PIM_EVENT_AUTH_ERROR);
                } else if (status.getStatusCode() == 500) {
                    msg = mCallBackHandler
                            .obtainMessage(DMDefine.PIM_EVENT.PIM_EVENT_SERVER_HTTP500);
                } else {
                    msg = mCallBackHandler.obtainMessage(DMDefine.PIM_EVENT.PIM_EVENT_COMM_ERROR);
                }
                mCallBackHandler.sendMessage(msg);
                return null;
            }

            Header[] headers = response.getAllHeaders();
            // Log.d(TAG, "headers : " + new String(headers));
            String headbody = "";
            int len = 0;
            for (Header header : headers) {
                Log.d(TAG, "header.getName(): " + header.getName());
                Log.d(TAG, "header.getValue(): " + header.getValue());
                len += header.getName().length() + header.getValue().length() + 4;
                headbody = headbody + header.getName() + ": " + header.getValue() + "\r\n";
            }
            headbody = headbody + "\r\n\r\n";
            Log.d(TAG, "headbody" + headbody);
            //TODO: to be confirm
//            DMNativeMethod.Jparse_x_syncml_hmac(headbody.getBytes());
            HttpEntity entity = response.getEntity();
            byte[] body = null;
            if (entity != null) {
                try {
                    if (entity.getContentLength() > 0) {
                        body = new byte[(int) entity.getContentLength()];
                        DataInputStream dis = new DataInputStream(entity.getContent());
                        try {
                            dis.readFully(body);
                        } finally {
                            try {
                                dis.close();
                            } catch (IOException e) {
                                Log.e(TAG, "Error closing input stream: " + e.getMessage());
                            }
                        }
                    }
                } finally {
                    if (entity != null) {
                        entity.consumeContent();
                    }
                }
            }
            return body;
        } catch (IllegalStateException e) {
            e.printStackTrace();
            handleHttpConnectionException(e);
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
            handleHttpConnectionException(e);
        } catch (SocketException e) {
            e.printStackTrace();
            handleHttpConnectionException(e);
        } catch (Exception e) {
            e.printStackTrace();
            handleHttpConnectionException(e);
        } finally {
            if (client != null) {
                client.close();
            }
        }
        return null;

    }

    private static AndroidHttpClient createHttpClient(Context context) {
        AndroidHttpClient client = AndroidHttpClient.newInstance(USER_AGENT, context);
        HttpParams params = client.getParams();

        HttpConnectionParams.setConnectionTimeout(params, 90000);
        HttpConnectionParams.setSoTimeout(params, 90 * 1000);

        HttpProtocolParams.setContentCharset(params, "UTF-8");
        return client;
    }

    private void handleHttpConnectionException(Exception exception) throws IOException {
        // Inner exception should be logged to make life easier.
        if (null != exception.getMessage()) {
            Log.e(TAG, exception.getMessage());
        }

        // PimNativeMethod.setPIMEvent(PimDefine.PIM_EVENT.PIM_EVENT_COMM_ERROR);

        // Message msg =
        // mCallBackHandler.obtainMessage(DMDefine.PIM_EVENT.PIM_EVENT_COMM_ERROR);
        // mCallBackHandler.sendMessage(msg);
        throw new IOException(exception.getMessage());
    }

    public static Header authenticate(String proxyUsr, String proxyPsw) {
        StringBuilder tmp = new StringBuilder();
        tmp.append(proxyUsr);
        tmp.append(":");
        tmp.append(proxyPsw);

        byte[] base64password = Base64
                .encodeBase64(EncodingUtils.getBytes(tmp.toString(), "UTF-8"));

        CharArrayBuffer buffer = new CharArrayBuffer(100);

        buffer.append(AUTH.PROXY_AUTH_RESP);
        // buffer.append(AUTH.WWW_AUTH_RESP);
        buffer.append(": Basic ");
        buffer.append(base64password, 0, base64password.length);
        Log.i(TAG, "authenticate : " + buffer.toString());
        return new BufferedHeader(buffer);
    }

}
