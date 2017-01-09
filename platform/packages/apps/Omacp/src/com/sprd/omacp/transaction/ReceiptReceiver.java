
package com.sprd.omacp.transaction;

import com.sprd.xml.parser.prv.Define;
import com.sprd.xml.parser.prv.PrintLogHelper;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class ReceiptReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "Receive the getResultCode = " + getResultCode());
        // SPRD: fix for bug 488045
        if (intent.getAction() != null) {
            if (intent.getAction().equals(Define.ACTION_APN)) {
                if (Define.RECEIVE_FLAG_APN == getResultCode()) {
                    Log.d(TAG, "APN Received and handled the broadcast");
                    PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                            "\nAPN Received and handled the broadcast");
                } else {
                    Log.d(TAG, "APN don't Received the broadcast");
                    PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                            "\n APN don't Received the broadcast");
                }
            } else if (intent.getAction().equals(Define.ACTION_EMAIL)) {
                if (Define.RECEIVE_FLAG_EMAIL == getResultCode()) {
                    Log.d(TAG, "EMAIL Received and handled the broadcast");
                    PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                            "\n EMAIL Received the broadcast");
                } else {
                    Log.d(TAG, "EMAIL don't Received the broadcast");
                    PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                            "\n EMAIL don't Received the broadcast");
                }
            } else if (intent.getAction().equals(Define.ACTION_BROWSER)) {
                if (Define.RECEIVE_FLAG_BROWSER == getResultCode()) {
                    Log.d(TAG, "BROWSER Received and handled the broadcast");
                    PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                            "\n BROWSER Received the broadcast");
                } else {
                    Log.d(TAG, "BROWSER don't Received the broadcast");
                    PrintLogHelper.getPrintWriter(PrintLogHelper.getFilePath()).println(
                            "\n BROWSER don't Received the broadcast");
                }
            }
        }
    }

    private final String TAG = "ReceiptReceiver";
}
