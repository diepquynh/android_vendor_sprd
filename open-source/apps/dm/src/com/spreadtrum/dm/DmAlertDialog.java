
package com.spreadtrum.dm;

import android.app.Activity;
import android.net.Uri;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.AdapterView.OnItemClickListener;
import android.content.ContentResolver;
import android.content.SharedPreferences;
import android.content.Context;
import android.util.Log;
import android.content.Intent; //import com.android.dm.vdmc.VdmcFumoHandler;
//import com.android.dm.vdmc.MyConfirmation;
import android.app.AlertDialog;
import android.content.DialogInterface; //import com.redbend.vdm.*;
import android.content.pm.ActivityInfo;
import android.database.Cursor;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.os.Handler;

import java.io.File;
import java.lang.Runnable;
import android.view.WindowManager;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.provider.Browser;
import android.provider.MediaStore;
import android.app.KeyguardManager;
import android.app.KeyguardManager.KeyguardLock;

import com.spreadtrum.dm.R;
import com.spreadtrum.dm.DmNativeInterface;
import com.spreadtrum.dm.vdmc.Vdmc;

import android.text.format.DateFormat;
import android.media.AudioManager;
import android.widget.Toast;

public class DmAlertDialog extends Activity {
    private String TAG = DmReceiver.DM_TAG + "DmAlertDialog: ";
    private String TAGCONF = "MyConfirmation";

    private Context mContext = null;

    private static DmAlertDialog mInstance = null;

    private NotificationManager mNotificationMgr = null;

    private WakeLock mWakelock = null;

    private int mDialogId = Vdmc.DM_NULL_DIALOG;

    private KeyguardLock mKeyguardLock = null;

    private boolean isScreenLock = false;

    private boolean isTimerActive = false;

    private Toast mToast = null;
	
    private AlertDialog builder = null;
    private AlertDialog builderSub = null;
    private Handler handler = new Handler();

    private Runnable runnable = new Runnable() {


	
        public void run() {
            stopTimer(); // stop timer

            // close alert dialog
            Log.d(TAG, "run() : mDialogId = " + mDialogId);
            switch (mDialogId) {
                case Vdmc.DM_NIA_INFO_DIALOG:
                    // DmJniInterface.startPppConnect();//start dial up
                    // DMNativeMethod.JVDM_notifyNIASessionProceed();

                   // notifyNIASessionProceed(); 2012-3-31@hong
                    builder.dismiss();
                    finish();
                    DestroyAlertDialog();
    	            DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(true);

                    break;

                case Vdmc.DM_NIA_CONFIRM_DIALOG:
                    // Vdmc.getInstance().stopVDM();
                    builder.dismiss();
                    finish();
                    DestroyAlertDialog();
                    DmService.getInstance().getDmNativeInterface().spdm_jni_stopDm(DmNativeInterface.STOP_DM_REASON_DEFAULT);
                    break;

                case Vdmc.DM_ALERT_CONFIRM_DIALOG:
                    handleTimeoutEvent();
                    // MyConfirmation.getInstance().handleTimeoutEvent();
                    // cancel dm session
                    break;
                case DmService.SEND_SELF_SMS:
                case DmService.DATA_CONNECT_CONFRIM:                    
                    if(builder != null )
                        builder.dismiss();
                    if(builderSub != null)
                        builderSub.dismiss();
                    mNotificationMgr.cancel(R.drawable.icon);
                    finish();
                    DestroyAlertDialog();
                    break;
                                                       
                default:
                    Log.d(TAG, "run() : mDialogId is invalid ");
                    break;
            }
        }
    };    
    
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // setContentView(R.layout.main);
        // getWindow().setBackgroundDrawable(null);
		// Bug 340714
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
		mContext = this;
        mInstance = this;
        mNotificationMgr = (NotificationManager) mContext
                .getSystemService(Context.NOTIFICATION_SERVICE);

        /*
         * //forbid sleep int flag =
         * WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON |
         * WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON |
         * WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED |
         * WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD;
         * getWindow().setFlags(flag, flag);
         * getWindow().setContentView(R.layout.alert_dialog);
         */

        getUnlock();

        Intent intent = getIntent();
        int id = intent.getIntExtra("dialogId", Vdmc.DM_NULL_DIALOG);
        Log.d(TAG, "OnCreate: id = " + id);
        String msg = intent.getStringExtra("message");
        // Log.d(TAG, "OnCreate: msg = " + msg);
        int timeout = intent.getIntExtra("timeout", 60); // default 1min
        Log.d(TAG, "OnCreate: timeout = " + timeout);
        Log.d(TAG, "OnCreate: mContext = " + mContext);
        
        if(id == DmService.DATA_CONNECT_CONFRIM){
            createDataConnectConfrimDialog(msg,timeout,intent);
        }else{
            CreateAlertDialog(id, msg, timeout);            
        }
        
        mDialogId = id;
        
    }

    @Override
    public void onDestroy() {        
        super.onDestroy();
        stopTimer(); // if switch to horizontal display, the window will auto
                     // close by system, and create new window, need stop timer
        if (mNotificationMgr != null) {
            mNotificationMgr.cancel(100);
            mNotificationMgr = null;
        }
        
        DmService.isDialogShowed = false;
        
        Log.d(TAG, "DestroyAlertDialog  DmService.isDialogShowed = " + DmService.isDialogShowed);
        /*
         * mContext = null; mInstance = null; 
         * mNotificationMgr.cancel(100);
         * mNotificationMgr = null; 
         * stopTimer(); 
         * releaseUnlock();
         */
    }

    // do some reset operation
    // finish() is asynchronous function, onDestroy() sometimes comes too
    // late,will result error
    private void DestroyAlertDialog() {
        Log.d(TAG, "DestroyAlertDialog");
        mContext = null;
        mInstance = null;
        mNotificationMgr.cancel(100);
        mNotificationMgr = null;
        DmService.isDialogShowed = false;
        Log.d(TAG, "DestroyAlertDialog  DmService.isDialogShowed = " + DmService.isDialogShowed);

        stopTimer();
        releaseUnlock();
	//System.gc();
    }

    public static DmAlertDialog getInstance() {
        return mInstance;
    }

    public void finishDialog() {
        finish();
        DestroyAlertDialog();
    }

    private void getUnlock() {
        // acquire wake lock
        PowerManager pm = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
        mWakelock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK | PowerManager.ACQUIRE_CAUSES_WAKEUP
                | PowerManager.ON_AFTER_RELEASE, "SimpleTimer");
        mWakelock.acquire();
        Log.d(TAG, "getUnlock: acquire Wakelock");

        // unlock screen
        KeyguardManager km = (KeyguardManager) mContext.getSystemService(Context.KEYGUARD_SERVICE);
        mKeyguardLock = km.newKeyguardLock(TAG);
        if (km.inKeyguardRestrictedInputMode()) {
            Log.d(TAG, "getUnlock: disable keyguard");
            mKeyguardLock.disableKeyguard();
            isScreenLock = true;
        } else {
            isScreenLock = false;
        }
    }

    private void releaseUnlock() {
        // release screen
        if (isScreenLock) {
            Log.d(TAG, "releaseUnlock: reenable Keyguard");
            mKeyguardLock.reenableKeyguard();
            isScreenLock = false;
        }
        // release wake lock
        if (mWakelock.isHeld()) {
            Log.d(TAG, "releaseUnlock: release Wakelock");
            mWakelock.release();
        }
    }

    private void playAlertSound() {
        Log.d(TAG, "playAlertSound:");
        Notification n = new Notification();

        n.flags |= Notification.FLAG_ONLY_ALERT_ONCE;
        
        //Fix 204720 on 20130822:the alert sound should be same to sms sound start
        Uri smsSoundUri = getSMSSoundUri();        
        Log.i(TAG, "playAlertSound  Uri = " + smsSoundUri);
        if(null == smsSoundUri){
        	n.defaults = Notification.DEFAULT_SOUND;        	 
        }else{
        	n.sound = smsSoundUri;        
        }
        //Fix 204720 on 20130822:the alert sound should be same to sms sound end

        AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        boolean nowSilent = audioManager.getRingerMode() != AudioManager.RINGER_MODE_NORMAL;
        if (nowSilent) {
            n.defaults |= Notification.DEFAULT_VIBRATE;
        }

        mNotificationMgr.notify(100, n);
    }

    //Fix 204720 on 20130822:the alert sound should be same to sms sound start
    private static final String SMS_SOUND = "SMS_SOUND";    
    
	private Uri getSMSSoundUri() {
		
		SharedPreferences smsSoundSP;
		smsSoundSP = getSharedPreferences(SMS_SOUND, MODE_PRIVATE);

		String smsSoundString = smsSoundSP.getString("smssound_dm", "Default");
		Log.i(TAG, "getSMSSoundUri smsSoundString : " + smsSoundString);
		//Bug:324837 Fix crash when the sound is default@{
		if((null != smsSoundString)&&(! smsSoundString.contains("media"))){			
			return null;
		}
		//@}
		if("Default".equals(smsSoundString)){			
			return null;
		}else{			
			Uri uri = Uri.parse(smsSoundString);	
			String smsSoundPath = null;
			Cursor smsSoundCursor = null;
			String[] proj = { MediaStore.Files.FileColumns.DATA };
			
			try{
				smsSoundCursor = getContentResolver().query(uri, proj, null, null, null);
				
				if ((null != smsSoundCursor) && (smsSoundCursor.moveToFirst())) {
					int smsSoundColumnIndex = smsSoundCursor
							.getColumnIndexOrThrow(MediaStore.Files.FileColumns.DATA);
					smsSoundPath = smsSoundCursor.getString(smsSoundColumnIndex);
					Log.i(TAG, "smsSoundPath = " + smsSoundPath);
				}
				
	        }catch (IllegalStateException e) {
	            Log.e(TAG, "getSMSSoundUri", e);
	        } finally {
	        	if(null != smsSoundCursor){
					smsSoundCursor.close();
				}
	        }		
			
			if(null == smsSoundPath){
				Log.i(TAG, "smsSoundPath is null ");
				return null;
			}
			
			File file = new File(smsSoundPath);
			Uri fileUri = Uri.fromFile(file);			
			Log.i(TAG, "fileUri = " + fileUri);			
			
			return fileUri;
		}		
	}
	//Fix 204720 on 20130822:the alert sound should be same to sms sound end
    
    private void startTimer(int timeout) // timeout : s
    {
        Log.d(TAG, "startTimer: timeout = " + timeout);

        // convert timout to ms
        isTimerActive = handler.postDelayed(runnable, timeout * 1000);// after timeout's operate runnable
    }

    private void stopTimer() {
        Log.d(TAG, "stopTimer: isTimerActive = " + isTimerActive);
        if (isTimerActive) {
            handler.removeCallbacks(runnable);
            isTimerActive = false;
        }
    }

    private void CreateAlertDialog(int id, String message, int timeout) {
        Log.d(TAG, "CreateAlertDialog: id = " + id);
        switch (id) {
            case Vdmc.DM_NULL_DIALOG:
                Log.d(TAG, "CreateAlertDialog: DM_NULL_DIALOG");
                break;

            case Vdmc.DM_NIA_INFO_DIALOG:
                Log.d(TAG, "CreateAlertDialog: DM_NIA_INFO_DIALOG");
/*				
		       if (mToast == null)
		            mToast = Toast.makeText(mContext,message,6000);
		        mToast.setText(message);
			 mToast.setDuration(6000);
			 playAlertSound();
		        mToast.show();
		DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(true);				
	        DmAlertDialog.getInstance().finishDialog();
*/	        
// @removed by Hong 2012-3-21
                String notify_msg = Vdmc.getAppContext().getResources().getString(
                        R.string.notify_msg);
                createNiaInformDialog(notify_msg, timeout);
//                
                break;

            case Vdmc.DM_NIA_CONFIRM_DIALOG:
                String notify_confirm_msg = Vdmc.getAppContext().getResources().getString(
                        R.string.notify_confirm_msg);
                Log.d(TAG, "CreateAlertDialog: DM_NIA_CONFIRM_DIALOG");
                createNiaConfirmDialog(notify_confirm_msg, timeout);
                break;

            case Vdmc.DM_ALERT_CONFIRM_DIALOG:
                Log.d(TAG, "CreateAlertDialog: DM_ALERT_CONFIRM_DIALOG" + message);
                CharSequence strSou = "&amp;";
                CharSequence strDes = "&";
                message = message.replace(strSou, strDes);
                Log.d(TAG, "CreateAlertDialog: DM_ALERT_CONFIRM_DIALOG1" + message);
                createAlertConfirmDialog(message, timeout);
                break;

            case Vdmc.DM_CONFIRM_DOWNLOAD_DIALOG:
                Log.d(TAG, "CreateAlertDialog: DM_CONFIRM_DOWNLOAD_DIALOG");
                // Vdmc.getFumoHandler().createDialog(mContext,
                // VdmcFumoHandler.CONFIRM_DOWNLOAD).show();
                break;

            case Vdmc.DM_CONFIRM_UPDATE_DIALOG:
                Log.d(TAG, "CreateAlertDialog: DM_CONFIRM_UPDATE_DIALOG");
                // Vdmc.getFumoHandler().createDialog(mContext,
                // VdmcFumoHandler.CONFIRM_UPDATE).show();
                break;

            case Vdmc.DM_SIMULATE_UPDATE_DIALOG:
                Log.d(TAG, "CreateAlertDialog: DM_CONFIRM_UPDATE_DIALOG");
                // Vdmc.getFumoHandler().createDialog(mContext,
                // VdmcFumoHandler.SIMULATE_UPDATE).show();
                break;

            case Vdmc.DM_PROGRESS_DIALOG:
                Log.d(TAG, "CreateAlertDialog: DM_PROGRESS_DIALOG");
                break;
                
            case DmService.SEND_SELF_SMS:                
                createShowSelfRegisterDialog(message,timeout);
                break;

            default:
                break;
        }
    }
    //CheckBox checkBox;
    private void createDataConnectConfrimDialog(String message, final int timeout,final Intent intent) {
        final String softkey_yes = getResources().getString(R.string.menu_yes);
        final String softkey_no = getResources().getString(R.string.menu_no);
        LinearLayout wappushLayout = (LinearLayout)getLayoutInflater().inflate(R.layout.cmcc_wappush_dialog, null);
        final CheckBox checkBox = (CheckBox)wappushLayout.findViewById(R.id.cb);
        //final Intent niaIntent = intent;        
//        byte[] data = intent.getByteArrayExtra("msg_body");
//        for (int i = 0; i < data.length; i++) {            
//            Log.d(TAG, "createDataConnectConfrimDialog data[" + i + "] = " + Integer.toHexString(data[i]&0xff));
//        }
        
        playAlertSound();
        
        builder = new AlertDialog.Builder(mContext).setTitle("").setView(wappushLayout)
                .setPositiveButton(softkey_yes, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        Log.d(TAG, "DmAlertDailog createDataConnectConfrimDialog yes on click");
                        DmService.getInstance().startVDM4NIA(intent);
                        DmService.getInstance().saveStatus4DataConnect(true, checkBox.isChecked());
                        mNotificationMgr.cancel(R.drawable.icon);
                        finish();
                        DestroyAlertDialog();
                    }
                }).setNegativeButton(softkey_no, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        Log.d(TAG, "DmAlertDailog createDataConnectConfrimDialog no on click");                        
                       //create double check when user click no
                        stopTimer();
                        startTimer(timeout);
                        builderSub = new AlertDialog.Builder(mContext).setTitle("").setMessage(getResources().getString(R.string.network_connect_prompt_cancel))
                        .setPositiveButton(softkey_yes, new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {                                                                  
                                Log.d(TAG, "createDataConnectConfrimDialog setNegativeButton checkBox = " + checkBox.isChecked());
                                DmService.getInstance().saveStatus4DataConnect(false, checkBox.isChecked());//checkBox.isChecked()
                                mNotificationMgr.cancel(R.drawable.icon);
                                finish();
                                DestroyAlertDialog();
                            }
                        }).setNegativeButton(softkey_no, new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                stopTimer();
                                startTimer(timeout);
                                Log.d(TAG, "DmAlertDailog createDataConnectConfrimDialog cliick no session!");
                                builder.show();
                            }
                        })
                        .create();
                        builderSub.setCanceledOnTouchOutside(false);
                        builderSub.setCancelable(false);
                        builderSub.show();                        
                        showNotification();
                    }
                }).setOnCancelListener(new DialogInterface.OnCancelListener() {
                    public void onCancel(DialogInterface dialog) {                                                                                               
                    }
                })
                .create();
               //.show();
        builder.setCanceledOnTouchOutside(false);
        builder.setCancelable(false);
        builder.show();                
        showNotification();
        startTimer(timeout);
    }
    
    private void createShowSelfRegisterDialog(String message, final int timeout) {
        String softkey_yes = getResources().getString(R.string.menu_yes);
        String softkey_no = getResources().getString(R.string.menu_no);
        
        playAlertSound();
        
        builder = new AlertDialog.Builder(mContext).setTitle("").setMessage(getResources().getString(R.string.sms_prompt))
                .setPositiveButton(softkey_yes, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        Log.d(TAG, "DmAlertDailog createShowSelfRegisterDialog yes on click");
                        //Bug312561,Create sub thread to run the send self register sms action
                        new Thread(){
							@Override
							public void run() {
								// TODO Auto-generated method stub								
								DmService.getInstance().saveStatus4AllowRegiste();
								DmService.getInstance().NotifySendSelfSMS();
							}}.start();
                        
                        mNotificationMgr.cancel(R.drawable.icon);
                        finish();
                        DestroyAlertDialog();
                    }
                }).setNegativeButton(softkey_no, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        Log.d(TAG, "DmAlertDailog createShowSelfRegisterDialog no on click");
                        //stopTimer();
                        //startTimer(timeout);
                        //create double check when user click no
                        builderSub = new AlertDialog.Builder(mContext).setTitle("").setMessage(getResources().getString(R.string.sms_prompt_cancel))
                        .setPositiveButton(getResources().getString(R.string.menu_yes), new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                Log.d(TAG, "createShowSelfRegisterDialog setNegativeButton click");                                
                                DmService.getInstance().saveStatus4CancelAllowRegiste();
                                mNotificationMgr.cancel(R.drawable.icon);
                                finish();
                                DestroyAlertDialog();
                            }
                        }).setNegativeButton(getResources().getString(R.string.menu_no), new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                                Log.d(TAG, "DmAlertDailog createShowSelfRegisterDialog cliick no session!");
                                //stopTimer();
                                //startTimer(timeout);
                                builder.show();
                            }
                        })
                        .create();
                        builderSub.setCanceledOnTouchOutside(false);
                        builderSub.setCancelable(false);
                        builderSub.show();                        
                        showNotification();                                                                                   
                    }
                }).setOnCancelListener(new DialogInterface.OnCancelListener() {
                    public void onCancel(DialogInterface dialog) {

                    }
                })
                .create();
                //.show();
        builder.setCanceledOnTouchOutside(false);
        builder.setCancelable(false);
        builder.show();                
        showNotification();
        //startTimer(timeout);
    }
    
    private void createNiaInformDialog(String message, int timeout) {
        String softkey_ok = getResources().getString(R.string.menu_ok);

        playAlertSound();

        builder = new AlertDialog.Builder(mContext).setTitle("").setMessage(message)
                .setPositiveButton(softkey_ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        Log.d(TAG, "createNiaInformDialog: onClick OK, notifyNIASessionProceed");
                        finish();
                        DestroyAlertDialog();
                        // start dial up
                        // DmJniInterface.startPppConnect();
                        // DMNativeMethod.JVDM_notifyNIASessionProceed();
                        notifyNIASessionProceed();
                        DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(true);
                        
                    }
                })
                .setOnCancelListener(new DialogInterface.OnCancelListener() {
                    public void onCancel(DialogInterface dialog) {
                        Log.d(TAG, "createNiaInformDialog: onCancel, notifyNIASessionProceed");
                        finish();
                        DestroyAlertDialog();
                        // start dial up
                        // DmJniInterface.startPppConnect();
                        // DMNativeMethod.JVDM_notifyNIASessionProceed();
                        notifyNIASessionProceed();
                        DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(true);

                    }
                })
                .show();
           startTimer(5);  //accord to cmcc, 2-10s 
//        startTimer(timeout);
    }

    private void createNiaConfirmDialog(String message, int timeout) {
        String softkey_yes = getResources().getString(R.string.menu_yes);
        String softkey_no = getResources().getString(R.string.menu_no);

        playAlertSound();

        builder = new AlertDialog.Builder(mContext).setTitle("").setMessage(message)
                .setPositiveButton(softkey_yes, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        Log.d(TAG, "createNiaConfirmDialog: onClick Yes, notifyNIASessionProceed");
                        // start dial up
                        // DMNativeMethod.JVDM_notifyNIASessionProceed();
                        finish();
                        DestroyAlertDialog();
                        DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(true);
                        //notifyNIASessionProceed(); hong2012
                        // DmService.getInstance().startPppConnect();
                    }
                }).setNegativeButton(softkey_no, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        Log.d(TAG, "createNiaConfirmDialog: onClick No, cancel dm session!");
                        //DmService.getInstance().getDmNativeInterface().spdm_jni_stopDm(DmNativeInterface.STOP_DM_REASON_DEFAULT);
                        // Vdmc.getInstance().stopVDM();
                        finish();
                        DestroyAlertDialog();
                       DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(false);
                    }
                }).setOnCancelListener(new DialogInterface.OnCancelListener() {
                    public void onCancel(DialogInterface dialog) {
                        Log.d(TAG, "createNiaConfirmDialog: onCancel, default cancel dm session!");
                        // Vdmc.getInstance().stopVDM();
                        finish();
                        DestroyAlertDialog();
                        DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(false);
//                        DmService.getInstance().getDmNativeInterface().spdm_jni_stopDm(DmNativeInterface.STOP_DM_REASON_DEFAULT);
                    }
                })
                .show();

        startTimer(timeout);
    }

    private void createAlertConfirmDialog(String message, int timeout) {
        playAlertSound();
        startTimer(timeout);
        String title = getResources().getString(R.string.notify_confirm_msg);
        String softkey_yes = getResources().getString(R.string.menu_yes);
        String softkey_no = getResources().getString(R.string.menu_no);
        createAlertConfirmDialog(mContext, title, message, softkey_yes, softkey_no, timeout);
    }

    private void createAlertConfirmDialog(Context context, String title, String message,
            String leftSfk, String rightSfk, int timeout)// lihui NIA before
                                                         // ALERT
    {
        builder = new AlertDialog.Builder(context).setTitle(title).setMessage(message)

        .setPositiveButton(leftSfk, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                Log.d(TAGCONF,
                        "createAlertConfirmDialog: onClick Yes, notifyConfirmationResult true");
                // observer.notifyConfirmationResult(true);
                DmAlertDialog.getInstance().finishDialog();
                DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(true);
            }
        }).setNegativeButton(rightSfk, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                Log.d(TAGCONF,
                        "createAlertConfirmDialog: onClick Yes, notifyConfirmationResult false");
                DmAlertDialog.getInstance().finishDialog();
                DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(false);
                // observer.notifyConfirmationResult(false);//lihui
                // hs_dm_mmi_confirmationQuerycb
            }
        }).setOnCancelListener(new DialogInterface.OnCancelListener() {
            public void onCancel(DialogInterface dialog) {
                Log.d(TAGCONF,
                        "createAlertConfirmDialog: onClick Yes, notifyCancelEvent");
                // observer.notifyCancelEvent();
                DmAlertDialog.getInstance().finishDialog();
                DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(false);
            }
        })

        .show();
    }

    private void handleTimeoutEvent() {
        Log.d(TAGCONF, "handleTimeoutEvent: alert window timeout");
        // observer.notifyCancelEvent();
        // DmAlertDialog.getInstance().finishDialog();
        builder.dismiss();
        DmAlertDialog.getInstance().finishDialog();
        DmService.getInstance().getDmNativeInterface().spdm_jni_dialogConfirm(false);
    }

    private void notifyNIASessionProceed() {
/*		do nothing here.
        Thread t = new Thread() {
            public void run() {
                Log.d(TAG, "Thread notifyNIASessionProceed ");
                Log.d(TAG, " DIAL START TIME = "
                        + DateFormat.format("yyyy-MM-dd kk:mm:ss", System.currentTimeMillis()));
                //TODO: to be confirm
//                DMNativeMethod.JVDM_notifyNIASessionProceed();
            }
        };
        t.start();
*/        
    }
    
    private void showNotification(){                        
                //NotificationManager notificationManager = (NotificationManager)           
                 //   this.getSystemService(android.content.Context.NOTIFICATION_SERVICE);                                                                
                Notification notification =new Notification(R.drawable.icon,          
                        "手机终端消息提醒", System.currentTimeMillis());        
                //FLAG_AUTO_CANCEL         
                //FLAG_NO_CLEAR         
                //FLAG_ONGOING_EVENT 
                //FLAG_INSISTENT        
                notification.flags |= Notification.FLAG_ONGOING_EVENT; //        
                notification.flags |= Notification.FLAG_NO_CLEAR; //           
                //notification.flags |= Notification.FLAG_SHOW_LIGHTS;          
                //DEFAULT_ALL          
                //DEFAULT_LIGHTS          
                //DEFAULT_SOUNDS          
                //DEFAULT_VIBRATE use vibrate need add <uses-permission android:name="android.permission.VIBRATE" /> 
                notification.defaults = Notification.DEFAULT_LIGHTS;        
                //
                //notification.defaults=Notification.DEFAULT_LIGHTS|Notification.DEFAULT_SOUND;        
                //notification.ledARGB = Color.BLUE;          
                //notification.ledOnMS =5000;          
                //           
                CharSequence contentTitle ="增强售后服务"; // 
                CharSequence contentText ="增强售后服务"; //           
                Intent notificationIntent =new Intent(DmAlertDialog.this, DmAlertDialog.class); //          
                PendingIntent contentItent = PendingIntent.getActivity(this, 0, notificationIntent, 0);          
                notification.setLatestEventInfo(this, contentTitle, contentText, contentItent);                  
                //           
                mNotificationMgr.notify(R.drawable.icon, notification);
                //mNotificationMgr.notify(id, notification);
    }
}
