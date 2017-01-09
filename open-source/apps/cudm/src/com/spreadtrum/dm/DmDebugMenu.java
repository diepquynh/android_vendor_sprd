package com.spreadtrum.dm;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.ListView;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Toast;
import android.widget.AdapterView.OnItemClickListener;
import android.content.SharedPreferences;
import android.content.Context;
import android.util.Log;
import android.content.Intent;

import com.spreadtrum.dm.R;
import android.app.AlertDialog;
import android.content.DialogInterface;

public class DmDebugMenu extends Activity {
    private String TAG = DmReceiverCU.DM_TAG + "DmDebugMenu: ";

    private ListView mListView;

    private String[] mlistItem;

    private Toast mToast;

    private Context mContext;

    protected static final int ITEM_DEBUG_MODE = 0;

    protected static final int ITEM_CLEAN_SR_FLAG = 1;

    protected static final int ITEM_SEND_SR_MSG = 2;

    protected static final int ITEM_SWITCH_SERVER = 3;

    protected static final int ITEM_MANUFACTORY = 4;

    protected static final int ITEM_MODEL = 5;

    protected static final int ITEM_SW_VER = 6;

    protected static final int ITEM_IMEI = 7;

    protected static final int ITEM_IMEI2 = 8;

    protected static final int ITEM_SERVER_ADDR = 9;

    protected static final int ITEM_SMS_ADDR = 10;

    protected static final int ITEM_SMS_PORT = 11;

    // protected static final int ITEM_TRIGGER_NULL_SESSION = 12;
    protected static final int ITEM_START_SERVICE = 12;

    protected static final int ITEM_DM_STATE = 13;

    protected static final int ITEM_SELFREG_SWITCH = 14;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mListView = (ListView) findViewById(R.id.app_list);

        // init list item content
        mlistItem = getResources().getStringArray(R.array.debug_list);

        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, R.layout.text_view, mlistItem);
        mListView.setAdapter(adapter);
        mListView.setOnItemClickListener(mItemClickListenter);

        mContext = this;
          Intent selfRegService = new Intent("com.android.dm.NIA");
          mContext.startService(selfRegService);		    
		
    }

    @Override
    protected void onDestroy() {
          Intent selfRegService = new Intent("com.android.dm.NIA");
          mContext.stopService(selfRegService);		        
        super.onDestroy();
    }

    private void ShowMessage(CharSequence msg) {
        if (mToast == null)
            mToast = Toast.makeText(this, msg, Toast.LENGTH_LONG);
        mToast.setText(msg);
        mToast.show();
    }

    private OnItemClickListener mItemClickListenter = new OnItemClickListener() {
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            Intent intent;

            Log.d(TAG, "mItemClickListenter : position = " + position);
            switch (position) {
                case ITEM_DEBUG_MODE:
                    // Debug mode
                    if (DmServiceCU.getInstance().isDebugMode()) {
                        DmServiceCU.getInstance().setDebugMode(mContext, false);
                        ShowMessage("Debug mode is closed!");
                    } else {
                        DmServiceCU.getInstance().setDebugMode(mContext, true);
                        ShowMessage("Debug mode is opened!");
                    }

                    break;

                case ITEM_CLEAN_SR_FLAG:
                    // Clean selfregiste flag
                    DmServiceCU.getInstance().saveImsi(mContext, "");
                    DmServiceCU.getInstance().setRegState(mContext,false);
                    DmServiceCU.getInstance().saveImsi(mContext, "");
                    DmServiceCU.getInstance().setRegState(mContext,false);
                    ShowMessage("Clean finished!");
                    break;

                case ITEM_SEND_SR_MSG:
                    // Send selfregiste message
                    if (DmServiceCU.getInstance().isDebugMode()) {
                        DmServiceCU.getInstance().sendSelfRegMsgForDebug();
                        ShowMessage("Have send selfregiste message!");
                    } else {
                        ShowMessage("Open debug mode first!");
                    }
                    break;

                case ITEM_SWITCH_SERVER:
                    // Switch server
                    boolean isReal = DmServiceCU.getInstance().isRealNetParam();
                    if (isReal) {
                        DmServiceCU.getInstance().setRealNetParam(mContext, false, true);
                        ShowMessage("Switch to lab param!");
                    } else {
                        DmServiceCU.getInstance().setRealNetParam(mContext, true, true);
                        ShowMessage("Switch to real param!");
                    }
                    break;

                case ITEM_MANUFACTORY:
                    // Manufactory
                    intent = new Intent(mContext, DmEditItemCU.class);
                    intent.putExtra("EditType", mlistItem[ITEM_MANUFACTORY]);
                    intent.putExtra("EditContent", DmServiceCU.getInstance().getManufactory());
                    startActivity(intent);
                    break;

                case ITEM_MODEL:
                    // Model
                    intent = new Intent(mContext, DmEditItemCU.class);
                    intent.putExtra("EditType", mlistItem[ITEM_MODEL]);
                    intent.putExtra("EditContent", DmServiceCU.getInstance().getModel());
                    startActivity(intent);
                    break;

                case ITEM_SW_VER:
                    // Software version
                    intent = new Intent(mContext, DmEditItemCU.class);
                    intent.putExtra("EditType", mlistItem[ITEM_SW_VER]);
                    intent.putExtra("EditContent", DmServiceCU.getInstance().getSoftwareVersion());
                    startActivity(intent);
                    break;

                case ITEM_IMEI:
                    // IMEI
                    intent = new Intent(mContext, DmEditItemCU.class);
                    intent.putExtra("EditType", mlistItem[ITEM_IMEI]);
                    intent.putExtra("EditContent", DmServiceCU.getInstance().getImei(0));
                    startActivity(intent);
                    break;

                case ITEM_IMEI2:
                    // APN                    
                    intent = new Intent(mContext, DmEditItemCU.class);
                    intent.putExtra("EditType", mlistItem[ITEM_IMEI2]);
                    intent.putExtra("EditContent", DmServiceCU.getInstance().getImei(1));
                    startActivity(intent);
/*                    intent = new Intent(mContext, DmEditItem.class);
                    intent.putExtra("EditType", mlistItem[ITEM_APN]);
		    if (null != DmServiceCU.getInstance().getSavedAPN())
                    intent.putExtra("EditContent", DmServiceCU.getInstance().getSavedAPN());				
			else
                    intent.putExtra("EditContent", DmServiceCU.getInstance().getAPN());
                    startActivity(intent);
 */
 			break;

                case ITEM_SERVER_ADDR:
                    // Server address
                    intent = new Intent(mContext, DmEditItemCU.class);
                    intent.putExtra("EditType", mlistItem[ITEM_SERVER_ADDR]);
                    intent.putExtra("EditContent", DmServiceCU.getInstance().getServerAddr());
                    startActivity(intent);
                    break;

                case ITEM_SMS_ADDR:
                    // Sms gateway address
                    intent = new Intent(mContext, DmEditItemCU.class);
                    intent.putExtra("EditType", mlistItem[ITEM_SMS_ADDR]);
                    intent.putExtra("EditContent", DmServiceCU.getInstance().getSmsAddr());
                    startActivity(intent);
                    break;

                case ITEM_SMS_PORT:
                    // Sms gateway port
                    intent = new Intent(mContext, DmEditItemCU.class);
                    intent.putExtra("EditType", mlistItem[ITEM_SMS_PORT]);
                    intent.putExtra("EditContent", DmServiceCU.getInstance().getSmsPort());
                    startActivity(intent);
                    break;

                // case ITEM_TRIGGER_NULL_SESSION:
                // Trigger null session
                /*
                 * if (Vdmc.getInstance().isVDMRunning()) {
                 * ShowMessage("vDM engine is running, stop it first,please wait..."
                 * ); Vdmc.getInstance().stopVDM(); } else {
                 * ShowMessage("Starting vDM engine, please wait...");
                 * Vdmc.getInstance().startVDM(mContext,
                 * Vdmc.SessionType.DM_SESSION_USER, null, null); }
                 */
                // break;

                case ITEM_START_SERVICE:
                    Log.d(TAG, "start service with intent com.android.dm.SelfReg");
                    intent = new Intent("com.android.dm.SelfReg");
                    startService(intent);
                    ShowMessage("Dm service is started!");
                    break;

                case ITEM_DM_STATE:
                    showDmState();
                    break;

                case ITEM_SELFREG_SWITCH:
                    if (DmServiceCU.getInstance().getSelfRegSwitch()) {
                        DmServiceCU.getInstance().setSelfRegSwitch(mContext, false);
                        ShowMessage("SelfReg Switch is closed!");
                    } else {
                        DmServiceCU.getInstance().setSelfRegSwitch(mContext, true);
                        ShowMessage("SelfReg Switch is opened!");
                    }

                    break;

                default:
                    break;
            }
        }
    };

    private void showDmState() {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        String message = new String();
        message = "Debug Mode : " + DmServiceCU.getInstance().isDebugMode();
        message = message + "\nSend message0 : " + DmServiceCU.getInstance().isHaveSendSelfRegMsg(0);
//        message = message + "\nSend message1 : " + DmServiceCU.getInstance().isHaveSendSelfRegMsg(1);
        message = message + "\nSelfReg State : " + DmServiceCU.getInstance().getRegState();
//        message = message + "\nSelfReg State : " + DmServiceCU.getInstance().isSelfRegOk(1);
//        message = message + "\nSession is running : " + Vdmc.getInstance().isVDMRunning();
//        message = message + "\nLast Session State : " + Vdmc.getLastSessionState();
//        message = message + "\nLast Error : " + Vdmc.getLastError();
/*        int regPhoneId = DmServiceCU.getInstance().getSelfRegSimIndex();
        if (0xff != regPhoneId){
            message = message + "\nReg Phone Id : SIM" + (regPhoneId + 1);
        }
*/
        builder.setTitle("").setMessage(message).setPositiveButton("OK",
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        Log.d(TAG, "showDmState: onClick OK");
                        dialog.dismiss();
                    }
                })

        .setOnCancelListener(new DialogInterface.OnCancelListener() {
            public void onCancel(DialogInterface dialog) {
                Log.d(TAG, "showDmState: onCancel");
                dialog.dismiss();
            }
        })

        .show();
    }
}
