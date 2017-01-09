package plugin.sprd.otaupdate;

import android.app.Activity;
import android.app.AddonManager;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Environment;
import android.os.EnvironmentEx;
import android.os.RecoverySystem;
import android.os.UserHandle;
import android.support.v7.preference.DialogPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.util.Log;
import android.widget.Toast;
import android.view.WindowManager;

import com.android.internal.logging.MetricsLogger;
import com.android.settings.OtaUpdate;
import com.android.settings.R;

import java.io.File;
import java.io.IOException;

public class AddonOtaUpdate extends OtaUpdate implements AddonManager.InitialCallback {

    private Context mContext;
    private BroadcastReceiver mBatteryLevelRcvr;
    private IntentFilter mBatteryLevelFilter;
    private int mLevelPower;
    private static String KEY_RECOVERY_SYSTEM_UPDATE = "RecoverySystemUpdate";

    private static final int MINIMUM_LEVEL_POWER = 35;//Porting from Android 4.4

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public boolean isSupport(){
        return true;
    }

    /* SPRD BUG: 616438  */
    public void initRecoverySystemUpdatePreference(Context context, PreferenceScreen preferenceScreen,Context appContext) {
        //only show RecoverySystemUpdate in USER_OWNER
        if (UserHandle.myUserId() != UserHandle.USER_OWNER) {
            return;
        }
        /* SPRD: added for #600472
        If user enter DeviceInfoSettings again in this screen by tapping siding drawer,
        receiver may leak. To avoid this error, add this block of code.  @{*/
        if (mContext != null && mBatteryLevelRcvr != null) {
            try {
                mContext.unregisterReceiver(mBatteryLevelRcvr);
                mBatteryLevelRcvr = null;
            } catch (Exception e) {
            }
        }
        /* @} */
        mContext = context;
        monitorBatteryState();
        PreferenceScreen mParentScreen = preferenceScreen;
        Preference rsup = new Preference (mContext);
        rsup.setTitle(R.string.recovery_update_title);
        rsup.setKey(KEY_RECOVERY_SYSTEM_UPDATE);
        rsup.setOrder(0);
        rsup.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
            public boolean onPreferenceClick(Preference preference){

                /* SPRD BUG: 616438  */
                AlertDialog.Builder builder = new AlertDialog.Builder(appContext);
                builder.setMessage(mContext.getResources().getString(R.string.recovery_update_message));
                builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface arg0, int arg1) {
                        String storageState="";
                        String storageDirectory="";
                        //external sdcard
                        storageState = EnvironmentEx.getExternalStoragePathState();
                        storageDirectory = EnvironmentEx.getExternalStoragePath().getAbsolutePath();

                        if (storageState.equals(Environment.MEDIA_MOUNTED)) {
                            File file=new File(storageDirectory+"/update.zip");
                            if(file.exists()){
                                if (mLevelPower >= MINIMUM_LEVEL_POWER) {
                                    /*
                                     * SPRD:DELETE the function is changed to update the
                                     * system. RECOVERY_DIR.mkdirs(); // In case we need it
                                     * COMMAND_FILE.delete(); // In case it's not writable
                                     * try { FileWriter command = new
                                     * FileWriter(COMMAND_FILE);
                                     * command.write("--update_package=/sdcard/update.zip");
                                     * command.write("\n"); command.close(); } catch
                                     * (IOException e) { e.printStackTrace(); } PowerManager
                                     * pm = (PowerManager)
                                     * cont.getSystemService(Context.POWER_SERVICE);
                                     * pm.reboot("recovery");
                                     */

                                    // SPRD:ADD the new function to update the system.
                                    try {
                                        RecoverySystem.installPackage(mContext, file);
                                    } catch (IOException e) {
                                        e.printStackTrace();
                                    }
                                } else {
                                    Toast.makeText(mContext, R.string.recovery_update_level, Toast.LENGTH_LONG).show();
                                }
                            } else {
                                Toast.makeText(mContext, R.string.recovery_no_package, Toast.LENGTH_LONG).show();
                            }
                        }else {
                            Toast.makeText(mContext, R.string.recovery_sdacrd_status, Toast.LENGTH_LONG).show();
                        }
                    }
                });
                builder.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface arg0, int arg1) {

                    }
                });
                AlertDialog dia = builder.create();
                /* SPRD BUG: 616438  */
                dia.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
                dia.show();
                /*  */
                return true;
            }

        });
        mParentScreen.addPreference(rsup);
    }

    public void monitorBatteryState() {
        mBatteryLevelRcvr = new BroadcastReceiver() {
            public void onReceive(Context context, Intent intent) {
                int rawlevel = intent.getIntExtra("level", -1);
                int scale = intent.getIntExtra("scale", -1);
                int level = -1; // percentage, or -1 for unknown
                if (rawlevel >= 0 && scale > 0) {
                    level = (rawlevel * 100) / scale;
                }
                mLevelPower = level;
            }
        };
        mBatteryLevelFilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        mContext.registerReceiver(mBatteryLevelRcvr, mBatteryLevelFilter);
    }

    public void unregisterBatteryReceiver() {
        if (mContext != null && ((Activity)mContext).isFinishing() && mBatteryLevelRcvr != null) { //SPRD:modify for #600472
            try {
                mContext.unregisterReceiver(mBatteryLevelRcvr);
            } catch (IllegalArgumentException e) {

            }
            mBatteryLevelRcvr = null;
        }
    }
}
