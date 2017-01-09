
package com.sprd.omacp.transaction;

import java.util.LinkedList;
import java.util.Queue;

import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Service;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.text.InputFilter;
import android.text.Spanned;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.Toast;
import com.sprd.xml.parser.prv.Define;
import com.sprd.omacp.R;
//add for bug 530106 begin
import com.sprd.xml.parser.prv.OmacpUtils;
//add for bug 530106 end
//import com.android.mms.transaction.MessagingNotification;

public class OtaOmaService extends Service {
    private static final String TAG = "OtaOmaService";
    private static final boolean DEBUG = true;

    private static final int EVENT_NEW_INTENT = 1;
    private static final int EVENT_OTA_REQUEST = 2;
    private static final int EVENT_PROCESS_NEXT_PENDING_OTA = 3;

    private ServiceHandler mHandler = null;
    // private Handler mHandler = new Handler();
    // private Looper mServiceLooper;
    private Queue<OtaOmaManager> mPending = new LinkedList<OtaOmaManager>();
    private OtaOmaManager mOtaManager = null;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        mHandler = new ServiceHandler();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        if (intent != null) {
            Log.i(TAG, "onStartCommand startId:" + startId);
            Message msg = mHandler.obtainMessage(EVENT_NEW_INTENT);
            msg.arg1 = startId;
            msg.obj = intent;
            mHandler.sendMessage(msg);
        }
        return Service.START_NOT_STICKY;// super.onStartCommand(intent, flags,
                                        // startId)
    }

    private final class ServiceHandler extends Handler {
        public void handleMessage(Message msg) {
            if (DEBUG) {
                Log.i(TAG, "Handling incoming message: " + msg + " = " + decodeMessage(msg));
            }

            switch (msg.what) {
                case EVENT_NEW_INTENT:
                    onNewIntent((Intent) msg.obj, msg.arg1);
                    break;
                case EVENT_OTA_REQUEST:
                    process();
                    break;
                case EVENT_PROCESS_NEXT_PENDING_OTA:
                    synchronized (mPending) {
                        mOtaManager = null;
                    }
                    mHandler.sendMessage(mHandler.obtainMessage(EVENT_OTA_REQUEST));
                    break;
            }
        }

        private String decodeMessage(Message msg) {
            if (msg.what == EVENT_OTA_REQUEST) {
                return "EVENT_OTA_REQUEST";
            } else if (msg.what == EVENT_PROCESS_NEXT_PENDING_OTA) {
                return "EVENT_PROCESS_NEXT_PENDING_OTA";
            } else if (msg.what == EVENT_NEW_INTENT) {
                return "EVENT_NEW_INTENT";
            }
            return "unknown message.what";
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    private void processNextPendingOta() {
        mHandler.sendMessage(mHandler.obtainMessage(EVENT_PROCESS_NEXT_PENDING_OTA));
    }

    private void onNewIntent(Intent intent, int serviceId) {
        synchronized (mPending) {
            mPending.offer(new OtaOmaManager(intent, serviceId));
            mHandler.sendMessage(mHandler.obtainMessage(EVENT_OTA_REQUEST));
        }
        if (DEBUG) {
            Log.d(TAG, "add the intent to pending queue.");
        }
    }

    private void process() {
        synchronized (mPending) {
            if (mOtaManager == null) {
                mOtaManager = mPending.poll();
                /* SPRD: add for bug353236 @{ */
            } else {
                return;
            }
            /* @} */
        }
        if (getOtaManager() == null) {
            Log.d(TAG, "mProcessingOta is null stopSelf()!");
            stopSelf();
            return;
        }

        if (getOtaManager().requiredInput()) {
            showDialog();
        } else {
            showConfirmDialog();
            return;
        }
    }

    private void otaParse(String inputPin) {
        if (getOtaManager() == null) {
            Log.d(TAG, "mProcessingOta is null stopSelf()!");
            stopSelf();
            return;
        }
        int ret = getOtaManager().parse(this, inputPin);
        getOtaManager().handleError(ret, this);
        if (ret == Define.ERROR_RETRY) {
            showDialog();
            return;
        }
        processNextPendingOta();
    }

    // private void stopSelfIfIdle(int startId) {
    // synchronized ( mPending ) {
    // if (mPending.isEmpty() && mProcessingOta != null) {
    // if ( DEBUG ) {
    // Log.d(TAG, "stopSelfIfIdle: STOP!");
    // stopSelf(startId);
    // }
    // }
    // }
    // }

    private OtaOmaManager getOtaManager() {
        return mOtaManager;
    }

    private AlertDialog showDialog() {
        Builder builder = createConfirmDialog();
        final AlertDialog dialog = builder.create();
        dialog.getWindow().getAttributes().softInputMode = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN;
        dialog.getWindow().setType((WindowManager.LayoutParams.TYPE_PHONE));

        mHandler.post(new Runnable() {
            @Override
            public void run() {
                dialog.show();
            }
        });
        return dialog;
    }

    private AlertDialog.Builder createConfirmDialog() {
        Builder dialog = new AlertDialog.Builder(this);
        LayoutInflater inflater = (LayoutInflater) this
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        LinearLayout layout = (LinearLayout) inflater.inflate(R.layout.ota_dialog, null);
        final EditText editor = (EditText) layout.findViewById(R.id.idEtOTAPin);
        // add for bug 583758 begin
        editor.setFilters(new InputFilter[] { new InputFilter.LengthFilter(100) });
        //add for bug 583758 end
        dialog.setView(layout);
        dialog.setTitle(R.string.OTAConfig_title).setIconAttribute(android.R.attr.alertDialogIcon)
                .setCancelable(true);
        dialog.setPositiveButton(R.string.yes, new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                String pin = editor.getText().toString();
                otaParse(pin);
                // add for bug 530106 begin
                OmacpUtils.clearMsgNf(OtaOmaService.this);
                // add for bug 530106 end
            }
        }).setNegativeButton(R.string.no, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                processNextPendingOta();
                // add for bug 530106 begin
                OmacpUtils.clearMsgNf(OtaOmaService.this);
                // add for bug 530106 end
            }
        }).setOnCancelListener(new OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                processNextPendingOta();
                // add for bug 530106 begin
                OmacpUtils.clearMsgNf(OtaOmaService.this);
                // add for bug 530106 end
            }
        });
        return dialog;
    }

    /* SPRD: Modify for Bug 304896. @{ */
    private AlertDialog showConfirmDialog() {
        Builder builder = createShowConfirmDialog();
        final AlertDialog dialog = builder.create();
        dialog.getWindow().setType((WindowManager.LayoutParams.TYPE_PHONE));

        mHandler.post(new Runnable() {
            @Override
            public void run() {
                dialog.show();
            }
        });
        return dialog;
    }

    /* @} */
    /* SPRD: Modify for Bug 304896.Alert a dialog before OTA settings. @{ */
    private AlertDialog.Builder createShowConfirmDialog() {
        Builder dialog = new AlertDialog.Builder(this);

        dialog.setTitle(R.string.confirm_ota_message).setCancelable(true);
        dialog.setPositiveButton(R.string.confirm_ota, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                // yes
                otaParse("");
                // add for bug 530106 begin
                OmacpUtils.clearMsgNf(OtaOmaService.this);
                // add for bug 530106 end
                // SPRD: Modify for Bug 383795 .
                // nonBlockingUpdateNewMessageIndicator();
            }
        }).setNegativeButton(R.string.cancel_ota, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                // cancel
                Log.i(TAG, "Cancel the OTA Settings.");
                Toast.makeText(OtaOmaService.this, R.string.ota_cancelled, Toast.LENGTH_SHORT)
                        .show();
                // SPRD: Modify for Bug 383795 .
                // nonBlockingUpdateNewMessageIndicator();
                // add for 529951 begin
                processNextPendingOta();
                // add for 529951 end
                // add for bug 530106 begin
                OmacpUtils.clearMsgNf(OtaOmaService.this);
                // add for bug 530106 end
                stopSelf();
            }
        // add for 529951 begin
        }).setOnCancelListener(new OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                Log.d(TAG, "createConfirmDialog Cancel");
                processNextPendingOta();
                // add for bug 530106 begin
                OmacpUtils.clearMsgNf(OtaOmaService.this);
                // add for bug 530106 end
            }
        });
        // add for bug 529951 end
        /* SPRD: Modify for Bug 409443 . @{ */
        dialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                // TODO Auto-generated method stub
                stopSelf();
            }
        });
        /* @} */
        return dialog;
    }
    /* @} */
    /* SPRD: Modify for Bug 383795 . @{ */
    /*
     * private void nonBlockingUpdateNewMessageIndicator() {
     * MessagingNotification.nonBlockingUpdateNewMessageIndicator( this,
     * Define.THREAD_ALL, false); }
     */
    /* @} */
}
