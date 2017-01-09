
package com.spreadtrum.dm.transaction;

import java.io.IOException;
import java.net.SocketException;
import java.net.InetAddress;
import java.net.UnknownHostException;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.NetworkUtils; //import android.net.NetworkConnectivityListener;
import android.util.Log;
import android.os.Handler;
import android.os.Message;
import android.content.Intent;

import com.android.internal.telephony.Phone; //import com.android.pim.*;
import com.spreadtrum.dm.DmNativeInterface;
//import com.spreadtrum.dm.DmJniInterface;
import com.spreadtrum.dm.DmNetwork;
import com.spreadtrum.dm.DmService;

import android.provider.ContactsContract;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.FileWriter;
import java.io.FileOutputStream;
//import com.android.internal.telephony.PhoneFactory;
import android.telephony.TelephonyManager;

public class DMTransaction implements Runnable {
    private final String TAG = "DMTransaction";

    public static final String PIM_PROGRESS_STATUS_ACTION = "com.android.pim.PROGRESS_STATUS";

    public static final int PIM_PROGRESS_START = -100;

    public static final int PIM_PROGRESS_ABORT = -101;

    public static final int PIM_PROGRESS_COMPLETE = -102;

    // zhl add
    public static final int PIM_DELETE_START = -103;

    // add end

    // private PIMTransactionSetting mTransactionSettings;
    // private DMPreferences mprefs;
    private  DMHttpUtils mPIMHttpUtils = null;

    // private DMGlobalInf mPIMGlobalInf;
    private Context mContext;

   private int mWorkSpaceId;

    private final boolean DEBUG = true;

    private Handler mManagerHandler;

    private int num = 1;

    private Thread mThread;

    private byte[] response = null;
	
    private byte[] senddata = null;

    private String mServerUri = null;
    private int mInet;
    public DMTransaction(Context context, Handler handler) {
        // mprefs = DMPreferences.getPreferences(context);
        // mPIMGlobalInf = PIMInf;
        mContext = context;
        mManagerHandler = handler;
    }

    public void process() {
        mThread = new Thread(this);
        mThread.start();
    }

    public void run() {
        //TODO: to be confirm
//        mWorkSpaceId = 0;//DMNativeMethod.JgetWorkSpaceId();
        byte[] data = senddata;
        //DMNativeMethod.JcopyDataForSending((short) mWorkSpaceId);
        // DMNativeMethod.JsetValueIsAlreadyReceive(false);
        if (mPIMHttpUtils == null)
       mPIMHttpUtils = new DMHttpUtils(/* mprefs, */mManagerHandler);
 
        if (DEBUG) {
            if (data != null) {
                writeXML(new String(data));
            }
        }

        if (data == null) {
            Log.d(TAG, "DMTransaction DATA null");
            return;
        }

        try {
            response = null;
            Log.d(TAG, "sendData: " + new String(data));
            response = sendData(data);
        }
        /*
         * catch (SocketException e2) { try { Thread.sleep(30*1000);
         * }catch(InterruptedException e1) { } try { response = null; Log.d(TAG,
         * "sendData: " +new String(data)); response = sendData(data); } catch
         * (IOException f) { if (DEBUG) { Log.v(TAG, "Unexpected IOException",
         * f); } } }
         */
        catch (IOException e) {
            if (DEBUG) {
                Log.v(TAG, "Unexpected IOException", e);
            }

        //TODO: to be confirm
//            if (DmService.getInstance().getDmInterface().getPppConnectStatus() != Phone.APN_ALREADY_ACTIVE) {
//                try {
//                    Thread.sleep(30 * 1000);
//                } catch (InterruptedException e1) {
//
//                }
//            }
            try {
		DmNetwork.getInstance().endConnectivity();
        	DmNetwork.getInstance().beginConnectivity(null);
                response = null;
                // Log.d(TAG, "sendData: " +new String(data));
                Log.d(TAG, "sendData: ");
                response = sendData(data);
            } catch (IOException f) {
                if (DEBUG) {
                    Log.v(TAG, "Unexpected IOException2", f);
                }
            }
 		mPIMHttpUtils = null;
        }

        if (response != null) {
            // Log.i(TAG, "sendData return response: " +new String(response));
            Log.i(TAG, "sendData return response: ");
            Log.d(TAG, "respone : " + new String(response));
            DmService.getInstance().getDmNativeInterface().spdm_jni_transportDataToLib(response, response.length);
            //TODO: to be confirm
//            mWorkSpaceId = DMNativeMethod.JgetWorkSpaceId();
//            DMNativeMethod.JcopySourceDataToBuffer((short) mWorkSpaceId, response,
//                    (long) response.length);
            if (DEBUG) {
                writeXML(new String(response));
            }
            stepNext();
        } else {
            // lihui for more actions notifyCommBroken
            Log.i(TAG, "NULL response, sendData fail ");
            //TODO: to be confirm
            DmService.getInstance().getDmNativeInterface().spdm_jni_stopDm(1);
//            DMNativeMethod.JnotifyCommBroken();
            
        }

	mPIMHttpUtils = null;
	//System.gc();
    }

    protected byte[] sendData(byte[] data) throws IOException {
        Log.d(TAG, "sendData datalen = " + data.length);
        
        boolean isProxySet = true;

        senddata = data;
        if (DmService.getInstance().getAPN().equals(DmService.APN_CMWAP)) {
            isProxySet = true;
        } else {
            isProxySet = false;
        }
        Log.d(TAG, "isProxySet==" + isProxySet);

        try {
            ensureRouteToHost();
        } catch (java.io.IOException e) {
            Log.i(TAG, "IOException : " + e.getMessage());
          //  Message msg = mManagerHandler.obtainMessage(DMDefine.PIM_EVENT.PIM_EVENT_COMM_ERROR);
           /// mManagerHandler.sendMessage(msg);
            throw new IOException(e.getMessage());
        }
        //TODO: to be confirm
        //String replaceserver = new String(DMNativeMethod.JgetReplaceServerAddress());

        String replaceserver= mServerUri;
        if(replaceserver == null )
            DmService.getInstance().getServerAddr();
        Log.d(TAG, "sendData replaceserver == " + replaceserver);
	InetAddress myinetaddr = NetworkUtils.intToInetAddress(mInet);
	
          Uri serverUri = Uri.parse(mServerUri);
        Log.d(TAG, "sendData replaceserver host" + serverUri.getHost() + "new"+myinetaddr.toString());

	String newserver = mServerUri.replaceFirst(serverUri.getHost(), myinetaddr.toString().substring(1));
        Log.d(TAG, "sendData newserver == " + newserver);
	
        try {
            if (isProxySet == true) {
                return mPIMHttpUtils.httpConnection(mContext, 
                        /* mprefs.getServerAdress(), */
                        replaceserver,
                        /* Integer.valueOf(DmService.getInstance().getServerPort()) */
                        7001, // liuhongxing Use proxy port, need address port
                        data, 
                        mPIMHttpUtils.HTTP_POST_METHOD, 
                        isProxySet, 
                        DmService.getInstance().getProxy(mContext), 
                        Integer.valueOf(DmService.getInstance().getProxyPort(mContext)), 
                        "OMADM", 
                        "mvpdm");
            } else {
                return mPIMHttpUtils.httpConnection(mContext, 
                        /* mprefs.getServerAdress(), */
                        newserver, //replaceserver,
                        /*Integer.valueOf(DmService.getInstance().getServerPort())*/
                        7001, // liuhongxing Use proxy port, need address port
                        data, 
                        mPIMHttpUtils.HTTP_POST_METHOD, 
                        isProxySet, 
                        null, 
                        0, 
                        "OMADM",
                        "mvpdm");
            }
        } catch (java.io.IOException e) {
            throw new IOException(e.getMessage());
        }
    }
/**
     * Look up a host name and return the result as an int. Works if the argument
     * is an IP address in dot notation. Obviously, this can only be used for IPv4
     * addresses.
     * @param hostname the name of the host (or the IP address)
     * @return the IP address as an {@code int} in network byte order
     */
    // TODO: move this to android-common
    public int lookupHost(String hostname) {
        InetAddress inetAddress;
	 Log.i(TAG,"lookupHost"+hostname);
        try {
            inetAddress = InetAddress.getByName(hostname);
        } catch (UnknownHostException e) {
            return -1;
        }
        byte[] addrBytes;
        int addr;
	 Log.d(TAG, "lookupHost:"+inetAddress.getHostName());
        addrBytes = inetAddress.getAddress();
        addr = ((addrBytes[3] & 0xff) << 24)
                | ((addrBytes[2] & 0xff) << 16)
                | ((addrBytes[1] & 0xff) << 8)
                |  (addrBytes[0] & 0xff);
	 Log.i(TAG, "lookupHost addr:"+addr);
        return addr;
    }

    private void ensureRouteToHost() throws IOException {
	Log.v(TAG,"ensureRouteToHost, phoneid:"+DmService.getInstance().getCurrentPhoneID());
        ConnectivityManager connMgr = (ConnectivityManager) mContext
                .getSystemService(Context.CONNECTIVITY_SERVICE);
// 	.getSystemService(TelephonyManager.getServiceName(Context.CONNECTIVITY_SERVICE, DmService.getInstance().getCurrentPhoneID()));
		Log.v(TAG,"ensureRouteToHost, Connectivitymanager ok in phoneid :"+DmService.getInstance().getCurrentPhoneID());
        String serverUrl = DmService.getInstance().getServerAddr();
//        int inetAddr;
        if (DmService.getInstance().getProxy(mContext) == null
                || (DmService.getInstance().getProxy(mContext) != null && DmService.getInstance()
                        .getProxy(mContext).equals(""))) {
            Uri serverUri = Uri.parse(serverUrl);
            mInet = NetworkUtils.lookupHost(serverUri.getHost());
            Log.i(TAG, "ensureRouteToHost mInet: " + mInet);
			
            if (mInet == -1) {
                throw new IOException("Cannot establish route for " + serverUrl + ": Unknown host");
            } else {
                /*
                 * if (DmNetwork.getInstance().connectiontype ==
                 * ConnectivityManager.TYPE_MOBILE_WAP) { if
                 * (!connMgr.requestRouteToHost
                 * (ConnectivityManager.TYPE_MOBILE_WAP, inetAddr)) { throw new
                 * IOException("Cannot establish route to proxy " + inetAddr); }
                 * } else { // liuhongxing Net type if
                 * (!connMgr.requestRouteToHost
                 * (ConnectivityManager.TYPE_MOBILE_DM, inetAddr)) { throw new
                 * IOException("Cannot establish route to proxy " + inetAddr); }
                 * }
                 */
                if (!connMgr.requestRouteToHost(
                  ConnectivityManager.getNetworkTypeByPhoneId(DmService.getInstance().getCurrentPhoneID(), ConnectivityManager.TYPE_MOBILE_DM),
					mInet)) {
                    throw new IOException("Cannot establish route to proxy " + mInet);
                }
            }
        } else {
            String proxyUrl = DmService.getInstance().getProxy(mContext);
            Log.i(TAG, "ensureRouteToHost proxyUrl: " + proxyUrl);
            //Uri proxyUri = Uri.parse(proxyUrl);
            mInet = NetworkUtils.lookupHost(proxyUrl/*proxyUri.getHost()*/);
            Log.i(TAG, "ensureRouteToHost mInet: " + mInet);
            if (mInet == -1) {
                throw new IOException("Cannot establish route for " + proxyUrl + ": Unknown host");
            } else {
                /*
                 * if (DmNetwork.getInstance().connectiontype ==
                 * ConnectivityManager.TYPE_MOBILE_WAP) { if
                 * (!connMgr.requestRouteToHost
                 * (ConnectivityManager.TYPE_MOBILE_WAP, inetAddr)) { throw new
                 * IOException("Cannot establish route to proxy " + inetAddr); }
                 * } else { // liuhongxing Net type if
                 * (!connMgr.requestRouteToHost
                 * (ConnectivityManager.TYPE_MOBILE_DM, inetAddr)) { throw new
                 * IOException("Cannot establish route to proxy " + inetAddr); }
                 * }
                 */
                if (!connMgr.requestRouteToHost(
                  ConnectivityManager.getNetworkTypeByPhoneId(DmService.getInstance().getCurrentPhoneID(), ConnectivityManager.TYPE_MOBILE_DM),
					mInet)) {
                    throw new IOException("Cannot establish route to proxy " + mInet);
                }
            }
        }

    }

    public void setWorkSpaceId(int id) {
        mWorkSpaceId = id;
    }

    private void writeXML(String data) {
        /*
         * PIMManager.writeLog(mContext, data); PIMManager.writeLog(mContext,
         * "\n"); FileOutputStream output = null; try { String name =
         * "/data/data/com.android.pim/" + Integer.toString(num) + ".xml";
         * num++; output = new FileOutputStream(name);
         * output.write(data.getBytes()); } catch(java.io.IOException e) {
         * Log.i(TAG, "sendSyncMLData num :" + num); } try { if (output != null)
         * { output.close(); } } catch(java.io.IOException e) {}
         */
    }

    public boolean sendSyncMLData(String url, byte[] data, int datalen) {
        senddata = data;
        mServerUri = url;
        process();
        return true;
    }

    public void sendMessageHelper(int what, Object obj, int arg1) {
        Log.d(TAG, "+sendMessageHelper1,what,:" + what + ",arg1:" + arg1);
        Message msg = mManagerHandler.obtainMessage(what);
        msg.obj = obj;
        msg.arg1 = arg1;
        mManagerHandler.sendMessage(msg);

    }

    public void sendMessageHelper(int what, int arg1) {

        Log.d(TAG, "+sendMessageHelper2,what,:" + what + ",arg1:" + arg1);

        if (-1 != arg1) {
            if (0 == what) {
                // if(arg1==2)//delete begin
                // broadcastProgressIfNeeded(PIM_DELETE_START, arg1);
                // else
                // broadcastProgressIfNeeded(PIM_PROGRESS_START, arg1);
                return;
            } else {
                if (arg1 == 2) {
                    broadcastProgressIfNeeded(what, arg1);
                    return;
                } else if ((arg1 == 0) || (arg1 == 1)) {
                    broadcastProgressIfNeeded(what, arg1);
                    return;
                } else
                    ;
            }

        }

        Message msg = mManagerHandler.obtainMessage(what);
        msg.arg1 = arg1;
        mManagerHandler.sendMessage(msg);

    }

    // add end
    public void sendMessageHelper(int what) {
        Log.d(TAG, "+sendMessageHelper3,what,:" + what);
        Message msg = mManagerHandler.obtainMessage(what);

        mManagerHandler.sendMessage(msg);

    }

    private void broadcastProgressIfNeeded(int progress, int type) {

        // Intent intent = new Intent(PIM_PROGRESS_STATUS_ACTION);
        // intent.putExtra("progress", progress);
        // intent.putExtra("type", type);//type = 0 ->send ,type = 1
        // ->receive,type=2->delete
        //
        // mContext.sendBroadcast(intent);

    }

    public void enableTransactionLock() {
        // for compile
        // mContext.getContentResolver().enableTransactionLock("contacts");
        // ContactsContract.enableTransactionLock(mContext.getContentResolver());
    }

    public void disableTransactionLock() {
        // for compile
        // mContext.getContentResolver().disableTransactionLock("contacts");
        // ContactsContract.disableTransactionLock(mContext.getContentResolver());
    }

    public void stepNext() {
        //TODO: to be confirm
//        DMNativeMethod.JstepDataReceive();
        // sendMessageHelper(PimDefine.PIM_EVENT.PIM_EVENT_NEXT_STEP);
    }

}
