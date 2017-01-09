
package com.spreadtrum.dm.vdmc;

import android.os.Handler;
import android.os.Message;
import android.widget.Toast;

import com.spreadtrum.dm.DmService;

public class VdmcMsgHandler extends Handler {

    /*
     * (non-Javadoc)
     * @see android.os.Handler#handleMessage(android.os.Message) The
     * VdmcMsgHandler enables us to deliver messages and to handle them by the
     * Handler's handleMessage (VdmcMsgHandler#handleMessage) function.
     * MySessionObserver is sending messages to VdmcMsgHandler which handles
     * them.
     */
    public void handleMessage(Message msg) {
        super.handleMessage(msg);

        String statusStr = new String();

        switch (msg.what) {
            /*
             * case(MSG_TYPE_DOWNLOADING): // msg#arg1 is the current number of
             * bytes of the update package // msg#arg2 is the size update
             * package (in bytes) if(msg.arg1 == msg.arg2) {
             * Vdmc.getInstance()._progressDialog.show();
             * Vdmc.getInstance()._progressDialog.dismiss(); } else {
             * Vdmc.getInstance()._progressDialog.display(msg.arg1, msg.arg2); }
             * break; case (MSG_TYPE_DL_STARTED): statusStr =
             * "DL Session Started"; break; case (MSG_TYPE_DL_ENDED)://on
             * Session State = 'Completed'
             * Vdmc.getFumoHandler().onDownloadComplete(); statusStr =
             * "DL Session Ended"; break; case (MSG_TYPE_DL_ABORTED): statusStr
             * = "DL Session Aborted"; break;
             */
            case (MSG_TYPE_DM_STARTED):
                statusStr = "DM Session Started";
                break;
            case (MSG_TYPE_DM_ENDED):
                statusStr = "DM Session Ended";
                // exit vDM task
                Vdmc.getInstance().stopVDM();
                break;
            case (MSG_TYPE_DM_ABORTED):
                statusStr = "DM Session Aborted";
                // exit vDM task
                Vdmc.getInstance().stopVDM();
                break;
        }

        if (msg.what != MSG_TYPE_DOWNLOADING) {
            // Vdmc.getInstance().displayMsg(statusStr);
            if (DmService.getInstance().isDebugMode()) {
                ShowMessage(statusStr);
            }
        }
    }

    private Toast mToast;

    private void ShowMessage(CharSequence msg) {
        if (mToast == null)
            mToast = Toast.makeText(Vdmc.getAppContext(), msg, Toast.LENGTH_LONG);
        mToast.setText(msg);
        mToast.show();
    }

    // //////////////////////////
    // Data members
    // /////////////////////////
    // status types
    // DL
    public static final int MSG_TYPE_DOWNLOADING = 0;

    public static final int MSG_TYPE_DL_STARTED = 1;

    public static final int MSG_TYPE_DL_ENDED = 2;

    public static final int MSG_TYPE_DL_ABORTED = 3;

    // DM
    public static final int MSG_TYPE_DM_STARTED = 4;

    public static final int MSG_TYPE_DM_ENDED = 5;

    public static final int MSG_TYPE_DM_ABORTED = 6;
}
