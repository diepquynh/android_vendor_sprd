
package com.spreadtrum.dm;

import com.spreadtrum.dm.transaction.DMTransaction;
import com.spreadtrum.dm.vdmc.MyTreeIoHandler;
import com.spreadtrum.dm.vdmc.Vdmc;

import android.accounts.NetworkErrorException;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.util.Log;

public class DmNativeInterface {
    private String TAG = DmReceiver.DM_TAG + "DmNativeInterface: ";
    public static final int STOP_DM_REASON_DEFAULT = 0;
    private static Context mContext;
    private  DMTransaction mDmTransaction;
    //private static MyTreeIoHandler mMyTreeIoHandler;

    public DmNativeInterface(Context context, Handler handler) {
        mContext = context;
        mDmTransaction = new DMTransaction(mContext, handler);
	//mMyTreeIoHandler = new	MyTreeIoHandler(mContext);
    }

    static {
        System.loadLibrary("sprddm");
    }

    //native method define begin ++
    public native boolean spdm_jni_start(int type, byte[] msg_body, int msg_size);

    public native boolean spdm_jni_isDmRunning();

    public native boolean spdm_jni_transportDataToLib(byte[] data, int datalen);

    public native void spdm_jni_dialogConfirm(boolean ret);

    public native void spdm_jni_stopDm(int reason);
    //native method define end --

    //java method define for lib begin ++
    public void spdm_sendDataCb(byte[] data, int datalen, int finishflag, byte[] _uri) {
        Log.i(TAG, "spdm_sendDataCb be referenced.");
        Log.d(TAG, "spdm_sendDataCb len = " + datalen);
    /*    boolean connected = DmNetwork.getInstance().isConnected();
        if(!connected){
            DmNetwork.getInstance().init();
            connected = DmNetwork.getInstance().beginConnectivity(null) != -1;
        }
*/
        String uri = null;
        if(_uri != null && _uri.length > 0)
            uri = new String(_uri);
	DmNetwork.getInstance().init();		

        boolean connected = DmNetwork.getInstance().beginConnectivity(null) != -1;
        if(connected){
            Log.d(TAG, "connected successed send data");
            mDmTransaction.sendSyncMLData(uri, data, datalen);
        }
        else {
            Log.d(TAG, "connected failed stopdm");
            Vdmc.getInstance().stopVDM();
        }

        if(finishflag == 1){
            
        }
    }

    public void spdm_openDialogCb(int type, byte[] message, byte[] title, int timeout) {
        Log.i(TAG, "spdm_openDialogCb be referenced.");
        String strMessage = null;
        if (message != null) {
            strMessage = new String(message);
        }
        Log.d(TAG, "displayDialog id: " + type + " message: "+ strMessage);
        Intent intent = new Intent(mContext, DmAlertDialog.class);
        int intentFlags = Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP;

        intent.setFlags(intentFlags);
        intent.putExtra("dialogId", type);
        intent.putExtra("message", strMessage);
        intent.putExtra("timeout", timeout);

        mContext.startActivity(intent);
    }

    public void spdm_writeNullCb(int handletype) {
        Log.i(TAG, "spdm_writeNullCb be referenced.");
        MyTreeIoHandler.getInstance(mContext).writenull(handletype);
    }

    public void spdm_writeCb(int handletype, byte[] data, int offset, int size) {
        Log.i(TAG, "spdm_writeCb be referenced.");
        MyTreeIoHandler.getInstance(mContext).write(handletype, offset, data, size);
    }

    //?????????????????? void --> int
    public void spdm_readCb(int handletype, byte[] buf, int offset, int bufsize) {
        Log.i(TAG, "spdm_readCb be referenced.");
        MyTreeIoHandler.getInstance(mContext).read(handletype, offset, buf);
    }

    public void spdm_exitNotifyCb(int reason) {
        Log.i(TAG, "spdm_exitNotifyCb be referenced.");

        Vdmc.getInstance().stopVDM();
        DmNetwork.getInstance().endConnectivity();
        DmNetwork.getInstance().end();

    }
    //java method define for lib end --
}
