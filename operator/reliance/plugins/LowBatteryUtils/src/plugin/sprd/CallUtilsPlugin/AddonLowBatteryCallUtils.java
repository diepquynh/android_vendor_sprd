package plugin.sprd.CallUtilsPlugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.os.SystemProperties;
import android.telephony.LowBatteryCallUtils;
import android.os.BatteryManager;
import android.content.Intent;
import android.content.IntentFilter;
import android.app.Activity;

import android.telecom.TelecomManager;
import android.telecom.VideoProfile;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.WindowManager;
import android.telecom.Call;
import com.android.incallui.*;
import android.telecom.InCallService.VideoCall;
import com.android.dialer.util.DialerUtils;



public class AddonLowBatteryCallUtils extends LowBatteryCallUtils implements AddonManager.InitialCallback {

    public static final String TAG = "AddonLowBatteryCallUtils";
    private Context mAddonContext;
    public AddonLowBatteryCallUtils() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean isBatteryLow() {
        Log.d(TAG, "in isBatteryLow ");
        Intent batteryInfoIntent = mAddonContext.registerReceiver(null, new IntentFilter(
                Intent.ACTION_BATTERY_CHANGED));
        int current = batteryInfoIntent.getIntExtra("level", 0);
        int total = batteryInfoIntent.getIntExtra("scale", 0);
        if (current * 1.0 / total <= 0.15) {
            Log.d(TAG, "in isBattery Low 15%");
            return true;
        } else {
            Log.d(TAG, "in isBatteryLow high 15%");
            return false;
        }
    }

    public static class SessionModificationState {
        public static final int NO_REQUEST = 0;
        public static final int WAITING_FOR_RESPONSE = 1;
        public static final int REQUEST_FAILED = 2;
        public static final int RECEIVED_UPGRADE_TO_VIDEO_REQUEST = 3;
        public static final int UPGRADE_TO_VIDEO_REQUEST_TIMED_OUT = 4;
        public static final int REQUEST_REJECTED = 5;
    }
    @Override
    public void showLowBatteryDialDialog(final Context context, final Intent intent, final boolean isDialingByDialer) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mAddonContext);
        builder.setTitle(mAddonContext.getString(R.string.low_battery_warning_title));
        builder.setMessage(mAddonContext.getString(R.string.low_battery_warning_message));
        builder.setPositiveButton(mAddonContext.getString(R.string.low_battery_continue_video_call),
                new android.content.DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        if (isDialingByDialer) {
                            DialerUtils.startActivityWithErrorToast(context, intent,
                                    R.string.activity_not_available);
                        } else {
                            context.startActivity(intent);
                        }
                        if (dialog != null) {
                            dialog.dismiss();
                        }
                    }
                });
        builder.setNegativeButton(mAddonContext.getString(R.string.low_battery_convert_to_voice_call),
                new android.content.DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        intent.putExtra(TelecomManager.EXTRA_START_CALL_WITH_VIDEO_STATE,
                                VideoProfile.STATE_AUDIO_ONLY);
                        if (isDialingByDialer) {
                            DialerUtils.startActivityWithErrorToast(context, intent,
                                    R.string.activity_not_available);
                        } else {
                            context.startActivity(intent);
                        }
                        if (dialog != null) {
                            dialog.dismiss();
                        }
                    }
                });
        builder.setCancelable(false);
        AlertDialog dialog = builder.create();
        dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        dialog.show();
    }
    @Override
    public void showLowBatteryInCallDialog(final Context context , final android.telecom.Call telecomCall) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(mAddonContext.getString(R.string.low_battery_warning_title));
        builder.setMessage(mAddonContext.getString(R.string.low_battery_warning_message));
        builder.setPositiveButton(mAddonContext.getString(R.string.low_battery_continue_video_call),
                new android.content.DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        if (telecomCall != null) {
                            telecomCall.answer(VideoProfile.STATE_BIDIRECTIONAL);
                        }
                        if (dialog != null) {
                            dialog.dismiss();
                        }
                    }
                });
        builder.setNegativeButton(mAddonContext.getString(R.string.low_battery_convert_to_voice_call),
                new android.content.DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        if (telecomCall != null) {
                            telecomCall.answer(VideoProfile.STATE_AUDIO_ONLY);
                        }
                        if (dialog != null) {
                            dialog.dismiss();
                        }
                    }
                });
        builder.setCancelable(false);
        AlertDialog dialog = builder.create();
        dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        dialog.show();
    }
    @Override
    public void showLowBatteryChangeToVideoDialog(final android.telecom.Call telecomCall , final VideoProfile videoProfile) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mAddonContext);
        builder.setTitle(mAddonContext.getString(R.string.low_battery_warning_title));
        builder.setMessage(mAddonContext.getString(R.string.low_battery_warning_message));
        builder.setPositiveButton(mAddonContext.getString(R.string.low_battery_continue_video_call),
                new android.content.DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        if (telecomCall != null) {
                            CallList callList = InCallPresenter.getInstance().getCallList();
                            com.android.incallui.Call call = callList.getCallByTelecomCall(telecomCall);
                            if (call != null) {
                                VideoCall videoCall = call.getVideoCall();
                                videoCall.sendSessionModifyRequest(videoProfile);
                                call.setSessionModificationState(SessionModificationState.WAITING_FOR_RESPONSE);
                            }
                        }
                        if (dialog != null) {
                            dialog.dismiss();
                        }
                    }
                });
        builder.setNegativeButton(mAddonContext.getString(R.string.low_battery_convert_to_voice_call),
                new android.content.DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        if (dialog != null) {
                            dialog.dismiss();
                        }
                    }
                });
        builder.setCancelable(false);
        AlertDialog dialog = builder.create();
        dialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        dialog.show();
    }

}
