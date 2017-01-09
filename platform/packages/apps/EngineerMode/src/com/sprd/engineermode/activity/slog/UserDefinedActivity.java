
package com.sprd.engineermode.activity.slog;

import static com.sprd.engineermode.debuglog.slogui.SlogService.NOTIFICATION_SNAP;
import static com.sprd.engineermode.debuglog.slogui.SlogService.SERVICE_SNAP_KEY;

import org.apache.http.message.BasicNameValuePair;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.preference.TwoStatePreference;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.content.Intent;

import com.sprd.engineermode.EngineerModeActivity;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.activity.slog.SlogInfo.SceneStatus;
import com.sprd.engineermode.core.EngineerModeProtocol;
import com.sprd.engineermode.core.SlogCore;
import com.sprd.engineermode.debuglog.slogui.ISlogService;
import com.sprd.engineermode.debuglog.slogui.SlogAction;
import com.sprd.engineermode.debuglog.slogui.SlogService;
import com.sprd.engineermode.utils.IATUtils;
import android.os.SystemProperties;

//import android.telephony.TelephonyManagerSprd;

/**
 * Created by SPREADTRUM\zhengxu.zhang on 9/6/15.
 */
public class UserDefinedActivity extends Activity implements View.OnClickListener {

    // private ImageView ivBack;
    private Button btnCommit;

    private RelativeLayout rlGeneral1;
    private ImageView ivGeneral1;

    private RelativeLayout rlGeneral2;
    private ImageView ivGeneral2;

    private RelativeLayout rlAndroidLog;
    private ImageView ivAndroidLog;

    private RelativeLayout rlTcpipLog;
    private ImageView ivTcpipLog;

    private RelativeLayout rlBtLog;
    private ImageView ivBtLog;

    private RelativeLayout rlApMore;
    private ImageView ivApMore;
    private LinearLayout llMoreApLog;

    private RelativeLayout rlModemLog;
    private ImageView ivModemLog;

    private RelativeLayout rlCpTcpipLog;
    private ImageView ivCpTcpipLog;

    private RelativeLayout rlAutoDump;
    private ImageView ivAutoDump;

    private RelativeLayout rlCpMore;
    private ImageView ivCpMore;
    private LinearLayout llMoreCpLog;

    private RelativeLayout rlMainLog;
    private ImageView ivMainLog;

    private RelativeLayout rlSystemLog;
    private ImageView ivSystemLog;

    private RelativeLayout rlRadioLog;
    private ImageView ivRadioLog;

    private RelativeLayout rlEventLog;
    private ImageView ivEventLog;

    private RelativeLayout rlKernelLog;
    private ImageView ivKernelLog;

    private RelativeLayout rlCrashLog;
    private ImageView ivCrashLog;

    private RelativeLayout rlDspLog;
    private ImageView ivDspLog;

    private RelativeLayout rlDspMore;
    private ImageView ivDspMore;
    private LinearLayout llMoreDspOption;

    private RelativeLayout rlCpBtLog;
    private ImageView ivCpBtLog;

    private RelativeLayout rlGpsLog;
    private ImageView ivGpsLog;

    private RelativeLayout rlDumpWcn;
    private ImageView ivDumpWcn;

    private RelativeLayout rlArtDebugLog;
    private ImageView ivArtDebugLog;

    private RelativeLayout rlAG_DspLog;
    private ImageView ivAG_DspLog;

    private RelativeLayout rlCm4Log;
    private ImageView ivCm4Log;

    private RelativeLayout rlArmPcmLog;
    private ImageView ivArmPcmLog;

    private RelativeLayout rlDspPcmLog;
    private ImageView ivDspPcmLog;

    private RadioButton logLevel2;
    private RadioButton logLevel3;
    private RadioButton logLevel5;
    private RadioGroup logLevel;

    private RadioButton rbOutputUart;
    private RadioButton rbOutputModemLog;

    private TextView tvIQMode;
    private TextView tvMemoryLeak;
    private TextView tvCpLogLevel;

    private RelativeLayout rlAG_DspMorn;
    private ImageView ivAG_DspMore;
    private LinearLayout llMoreAG_DspOption;
    private RadioButton rbAG_DspOutputDisable;
    private RadioButton rbAG_DspOutputUart;
    private RadioButton rbAG_DspOutputUsb;
    private RelativeLayout rlAG_DspPcmDumpLog;
    private ImageView ivAG_DspPcmDumpLog;

    private boolean isSupport5mod = SystemProperties.get("persist.radio.ssda.mode").equals("csfb");

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        SlogInfo.x = UserDefinedActivity.this;
        setContentView(R.layout.layout_slog_user_defined);
        SlogInfo.self().loadCustomerOrder();
        // ivBack = (ImageView)findViewById(R.id.iv_back);
        // ivBack.setOnClickListener(this);
        btnCommit = (Button) findViewById(R.id.btn_commit);
        btnCommit.setOnClickListener(this);

        rlGeneral1 = (RelativeLayout) findViewById(R.id.rl_general_1);
        ivGeneral1 = (ImageView) findViewById(R.id.iv_general_1);
        initCheckBox(rlGeneral1, ivGeneral1, "SlogCore_AlwaysShowSlogInNotification");

        rlGeneral2 = (RelativeLayout) findViewById(R.id.rl_general_2);
        ivGeneral2 = (ImageView) findViewById(R.id.iv_general_2);
        initCheckBox(rlGeneral2, ivGeneral2, "SlogCore_SnapDevice");

        rlAndroidLog = (RelativeLayout) findViewById(R.id.rl_android_log);
        rlAndroidLog.setOnClickListener(this);
        ivAndroidLog = (ImageView) findViewById(R.id.iv_android_log);
        if (SlogInfo.self().getCustomerDefined("SlogCore_MainLogController").equals("1") ||
                SlogInfo.self().getCustomerDefined("SlogCore_SystemLogController").equals("1") ||
                SlogInfo.self().getCustomerDefined("SlogCore_RadioLogController").equals("1") ||
                SlogInfo.self().getCustomerDefined("SlogCore_EventLogController").equals("1") ||
                SlogInfo.self().getCustomerDefined("SlogCore_KernelLogController").equals("1") ||
                /* SPRD: Bug 595534 LogManager/custom scence logic error @{ */
                SlogInfo.self().getCustomer("SlogCore_CrashLogController").equals("1")) {
                /* @} */
            SlogInfo.self().setCustomer("SlogCore_AndroidLogController", "1");
        } else {
            SlogInfo.self().setCustomer("SlogCore_AndroidLogController", "0");
        }
        if (SlogInfo.self().getCustomer("SlogCore_AndroidLogController").equals("1")) {
            ivAndroidLog.setImageResource(R.drawable.checkbox_on);
        } else {
            ivAndroidLog.setImageResource(R.drawable.checkbox_off);
        }

        rlTcpipLog = (RelativeLayout) findViewById(R.id.rl_tcpip_log);
        ivTcpipLog = (ImageView) findViewById(R.id.iv_tcpip_log);
        initCheckBox(rlTcpipLog, ivTcpipLog, "SlogCore_APCapLogController");

        rlBtLog = (RelativeLayout) findViewById(R.id.rl_bt_log);
        ivBtLog = (ImageView) findViewById(R.id.iv_bt_log);
        initCheckBox(rlBtLog, ivBtLog, "SlogCore_BtHciLogController");

        rlApMore = (RelativeLayout) findViewById(R.id.rl_ap_more);
        rlApMore.setOnClickListener(this);
        ivApMore = (ImageView) findViewById(R.id.iv_ap_more);
        llMoreApLog = (LinearLayout) findViewById(R.id.ll_more_ap_log);
        llMoreApLog.setVisibility(View.GONE);

        rlModemLog = (RelativeLayout) findViewById(R.id.rl_modem_log);
        ivModemLog = (ImageView) findViewById(R.id.iv_modem_log);
        initCheckBox(rlModemLog, ivModemLog, "SlogCore_ArmLogController");

        rlCpTcpipLog = (RelativeLayout) findViewById(R.id.rl_cp_tcpip_log);
        ivCpTcpipLog = (ImageView) findViewById(R.id.iv_cp_tcpip_log);
        initCheckBox(rlCpTcpipLog, ivCpTcpipLog, "SlogCore_CpCapLogController");

        rlAutoDump = (RelativeLayout) findViewById(R.id.rl_auto_dump);
        ivAutoDump = (ImageView) findViewById(R.id.iv_auto_dump);
        initCheckBox(rlAutoDump, ivAutoDump, "SlogCore_EnableAutoDumpModem");

        rlApMore = (RelativeLayout) findViewById(R.id.rl_ap_more);
        rlApMore.setOnClickListener(this);
        ivApMore = (ImageView) findViewById(R.id.iv_ap_more);
        llMoreApLog = (LinearLayout) findViewById(R.id.ll_more_ap_log);
        llMoreApLog.setVisibility(View.GONE);

        rlMainLog = (RelativeLayout) findViewById(R.id.rl_ap_main);
        ivMainLog = (ImageView) findViewById(R.id.iv_ap_main);
        initCheckBox(rlMainLog, ivMainLog, "SlogCore_MainLogController");

        rlSystemLog = (RelativeLayout) findViewById(R.id.rl_ap_system);
        ivSystemLog = (ImageView) findViewById(R.id.iv_ap_system);
        initCheckBox(rlSystemLog, ivSystemLog, "SlogCore_SystemLogController");

        rlRadioLog = (RelativeLayout) findViewById(R.id.rl_ap_radio);
        ivRadioLog = (ImageView) findViewById(R.id.iv_ap_radio);
        initCheckBox(rlRadioLog, ivRadioLog, "SlogCore_RadioLogController");

        rlEventLog = (RelativeLayout) findViewById(R.id.rl_ap_event);
        ivEventLog = (ImageView) findViewById(R.id.iv_ap_event);
        initCheckBox(rlEventLog, ivEventLog, "SlogCore_EventLogController");

        rlKernelLog = (RelativeLayout) findViewById(R.id.rl_ap_kernel);
        ivKernelLog = (ImageView) findViewById(R.id.iv_ap_kernel);
        initCheckBox(rlKernelLog, ivKernelLog, "SlogCore_KernelLogController");

        rlCrashLog = (RelativeLayout) findViewById(R.id.rl_ap_crash);
        ivCrashLog = (ImageView) findViewById(R.id.iv_ap_crash);
        initCheckBox(rlCrashLog, ivCrashLog, "SlogCore_CrashLogController");

        rlArtDebugLog = (RelativeLayout) findViewById(R.id.rl_ap_artdebug);
        ivArtDebugLog = (ImageView) findViewById(R.id.iv_ap_artdebug);
        initCheckBox(rlArtDebugLog, ivArtDebugLog, "SlogCore_ArtDebugLogController");

        rlCpMore = (RelativeLayout) findViewById(R.id.rl_cp_more);
        rlCpMore.setOnClickListener(this);
        ivCpMore = (ImageView) findViewById(R.id.iv_cp_more);
        llMoreCpLog = (LinearLayout) findViewById(R.id.ll_more_cp_log);
        llMoreCpLog.setVisibility(View.GONE);

        rlDspLog = (RelativeLayout) findViewById(R.id.rl_dsp_log);
        ivDspLog = (ImageView) findViewById(R.id.iv_dsp_log);
        initCheckBox(rlDspLog, ivDspLog, "SlogCore_DspLogController");

        rlDspMore = (RelativeLayout) findViewById(R.id.rl_dsp_more_option);
        rlDspMore.setOnClickListener(this);
        ivDspMore = (ImageView) findViewById(R.id.iv_dsp_more_option);
        llMoreDspOption = (LinearLayout) findViewById(R.id.ll_more_dsp_log);
        llMoreDspOption.setVisibility(View.GONE);

        rlCpBtLog = (RelativeLayout) findViewById(R.id.rl_cp_bt_log);
        ivCpBtLog = (ImageView) findViewById(R.id.iv_cp_bt_log);
        initCheckBox(rlCpBtLog, ivCpBtLog, "SlogCore_WcnLogController");

        rlGpsLog = (RelativeLayout) findViewById(R.id.rl_gps_log);
        ivGpsLog = (ImageView) findViewById(R.id.iv_gps_log);
        initCheckBox(rlGpsLog, ivGpsLog, "SlogCore_GpsLogController");

        rlDumpWcn = (RelativeLayout) findViewById(R.id.rl_dump_wcn);
        ivDumpWcn = (ImageView) findViewById(R.id.iv_dump_wcn);
        initCheckBox(rlDumpWcn, ivDumpWcn, "SlogCore_EnableDumpWcnMemInUser");

        rlAG_DspLog = (RelativeLayout) findViewById(R.id.rl_ag_dsp_log);
        ivAG_DspLog = (ImageView) findViewById(R.id.iv_ag_dsp_log);
        initCheckBox(rlAG_DspLog, ivAG_DspLog, "SlogCore_AGDspLogController");

        rlCm4Log = (RelativeLayout) findViewById(R.id.rl_cm4_log);
        ivCm4Log = (ImageView) findViewById(R.id.iv_cm4_log);
        initCheckBox(rlCm4Log, ivCm4Log, "SlogCore_Cm4LogController");

        rlArmPcmLog = (RelativeLayout) findViewById(R.id.rl_arm_pcm_log);
        ivArmPcmLog = (ImageView) findViewById(R.id.iv_arm_pcm_log);
        initCheckBox(rlArmPcmLog, ivArmPcmLog, "SlogCore_ArmPcmDataController");

        rlDspPcmLog = (RelativeLayout) findViewById(R.id.rl_dsp_pcm_log);
        ivDspPcmLog = (ImageView) findViewById(R.id.iv_dsp_pcm_log);
        initCheckBox(rlDspPcmLog, ivDspPcmLog, "SlogCore_DspPcmDataController");

        logLevel = (RadioGroup) findViewById(R.id.slog_log_level);
        logLevel2 = (RadioButton) findViewById(R.id.slog_log_level_2);
        logLevel2.setOnClickListener(this);
        logLevel2.setChecked(false);
        logLevel3 = (RadioButton) findViewById(R.id.slog_log_level_3);
        logLevel3.setOnClickListener(this);
        logLevel3.setChecked(false);
        logLevel5 = (RadioButton) findViewById(R.id.slog_log_level_5);
        logLevel5.setOnClickListener(this);
        logLevel5.setChecked(false);

        /* SPRD: Bug 560292 java.lang.NullPointerException @{ */
        if (logLevel.getVisibility() != View.GONE) {
            switch (SlogCore.SlogCore_GetAndroidLogLevel()) {
                case 2:
                    logLevel2.setChecked(true);
                    break;
                case 3:
                    logLevel3.setChecked(true);
                    break;
                case 5:
                    logLevel5.setChecked(true);
                    break;
                default:
                    break;
            }
        }
        /* @} */
        rbOutputUart = (RadioButton) findViewById(R.id.rb_output_uart);
        rbOutputUart.setOnClickListener(this);
        rbOutputUart.setChecked(getDspOutput() == 1 ? true : false);
        rbOutputModemLog = (RadioButton) findViewById(R.id.rb_output_modemlog);
        rbOutputModemLog.setOnClickListener(this);
        rbOutputModemLog.setChecked(getDspOutput() == 2 ? true : false);

        tvIQMode = (TextView) findViewById(R.id.tv_iq_mode);
        tvIQMode.setClickable(true);
        tvIQMode.setOnClickListener(this);
        tvMemoryLeak = (TextView) findViewById(R.id.tv_memory_leak);
        tvMemoryLeak.setClickable(true);
        tvMemoryLeak.setOnClickListener(this);
        tvCpLogLevel = (TextView) findViewById(R.id.tv_cp_log_level);
        tvCpLogLevel.setClickable(true);
        tvCpLogLevel.setOnClickListener(this);
        tvCpLogLevel.setText("Log Filter  \n"
                + SlogInfo.self().mySharedPreferences.getString("logFilter", "Debug"));
        if (!isSupport5mod) {
            tvCpLogLevel.setVisibility(View.GONE);
        }
        rlAG_DspMorn = (RelativeLayout) findViewById(R.id.rl_ag_dsp_more_option);
        rlAG_DspMorn.setOnClickListener(this);
        ivAG_DspMore = (ImageView) findViewById(R.id.iv_ag_dsp_more_option);
        llMoreAG_DspOption = (LinearLayout) findViewById(R.id.ll_ag_dsp_output_mode);
        llMoreAG_DspOption.setVisibility(View.GONE);
        rbAG_DspOutputDisable = (RadioButton) findViewById(R.id.rb_ag_dsp_output_disable);
        rbAG_DspOutputUart = (RadioButton) findViewById(R.id.rb_ag_dsp_output_uart);
        rbAG_DspOutputUsb = (RadioButton) findViewById(R.id.rb_ag_dsp_output_usb);
        rbAG_DspOutputDisable.setOnClickListener(this);
        rbAG_DspOutputUart.setOnClickListener(this);
        rbAG_DspOutputUsb.setOnClickListener(this);
        initAgDspOutputRadioBox("SlogCore_AGDspOutputController");
        rlAG_DspPcmDumpLog = (RelativeLayout) findViewById(R.id.rl_ag_dsp_dump_log);
        ivAG_DspPcmDumpLog = (ImageView) findViewById(R.id.iv_ag_dsp_dump_log);
        ivAG_DspPcmDumpLog.setOnClickListener(this);
        initCheckBox(rlAG_DspPcmDumpLog, ivAG_DspPcmDumpLog, "SlogCore_AGDspPcmDumpLogController");
    }

    private void initCheckBox(RelativeLayout rl, ImageView iv, String slogcoreName) {
        rl.setOnClickListener(this);
        iv.setImageResource(
                SlogInfo.self().getCustomerDefined(slogcoreName).equals("0") ?
                        R.drawable.checkbox_off : R.drawable.checkbox_on);
    }

    private void initAgDspOutputRadioBox(String slogcoreName) {
        String value = SlogInfo.self().getCustomerDefined(slogcoreName);
        if ("0".equals(value)) {
            rbAG_DspOutputDisable.setChecked(true);
            rbAG_DspOutputUart.setChecked(false);
            rbAG_DspOutputUsb.setChecked(false);
        } else if ("1".equals(value)) {
            rbAG_DspOutputDisable.setChecked(false);
            rbAG_DspOutputUart.setChecked(true);
            rbAG_DspOutputUsb.setChecked(false);
        } else {
            rbAG_DspOutputDisable.setChecked(false);
            rbAG_DspOutputUart.setChecked(false);
            rbAG_DspOutputUsb.setChecked(true);
        }
    }

    private void setAgDspOutputRadioBox() {
        String value = SlogInfo.self().getCustomer("SlogCore_AGDspOutputController");
        if ("0".equals(value)) {
            rbAG_DspOutputDisable.setChecked(true);
            rbAG_DspOutputUart.setChecked(false);
            rbAG_DspOutputUsb.setChecked(false);
        } else if ("1".equals(value)) {
            rbAG_DspOutputDisable.setChecked(false);
            rbAG_DspOutputUart.setChecked(true);
            rbAG_DspOutputUsb.setChecked(false);
        } else {
            rbAG_DspOutputDisable.setChecked(false);
            rbAG_DspOutputUart.setChecked(false);
            rbAG_DspOutputUsb.setChecked(true);
        }
    }

    private void onClickCheckBox(ImageView iv, String slogcoreName) {
        if (SlogInfo.self().getCustomer(slogcoreName).equals("1")) {
            SlogInfo.self().setCustomer(slogcoreName, "0");
            iv.setImageResource(R.drawable.checkbox_off);
        } else {
            SlogInfo.self().setCustomer(slogcoreName, "1");
            iv.setImageResource(R.drawable.checkbox_on);
        }
    }

    private void onClickCheckBox(String value, String slogcoreName) {
        SlogInfo.self().setCustomer(slogcoreName, value);
    }

    private void syncAndroidLog() {

        if (SlogInfo.self().getCustomer("SlogCore_MainLogController").equals("1") ||
                SlogInfo.self().getCustomer("SlogCore_SystemLogController").equals("1") ||
                SlogInfo.self().getCustomer("SlogCore_RadioLogController").equals("1") ||
                SlogInfo.self().getCustomer("SlogCore_EventLogController").equals("1") ||
                SlogInfo.self().getCustomer("SlogCore_KernelLogController").equals("1") ||
                SlogInfo.self().getCustomer("SlogCore_CrashLogController").equals("1")) {

            SlogInfo.self().setCustomer("SlogCore_AndroidLogController", "1");
        } else {
            SlogInfo.self().setCustomer("SlogCore_AndroidLogController", "0");
        }

        ivAndroidLog.setImageResource(
                SlogInfo.self().getCustomer("SlogCore_AndroidLogController").equals("0") ?
                        R.drawable.checkbox_off : R.drawable.checkbox_on);

    }

    private void syncModemLog() {
        if (SlogInfo.self().getCustomer("SlogCore_ArmLogController")
                .equals("1")
                || SlogInfo.self().getCustomer("SlogCore_CpCapLogController")
                        .equals("1")
                || SlogInfo.self().getCustomer("SlogCore_DspLogController")
                        .equals("1")) {
            SlogInfo.self().setCustomer("SlogCore_ModemLogController", "1");
            SlogInfo.self().setCustomer("SlogCore_ArmLogController", "1");
        } else {
            SlogInfo.self().setCustomer("SlogCore_ModemLogController", "0");
            SlogInfo.self().setCustomer("SlogCore_ArmLogController", "0");
        }
        ivModemLog.setImageResource(
                SlogInfo.self().getCustomer("SlogCore_ArmLogController").equals("0") ?
                        R.drawable.checkbox_off : R.drawable.checkbox_on);
    }

    private void syncAGDspLog(String slogcoreName) {
        if ("SlogCore_AGDspLogController".equals(slogcoreName)
                || "SlogCore_AGDspPcmDumpLogController".equals(slogcoreName)) {
            if (SlogInfo.self().getCustomer("SlogCore_AGDspLogController").equals("1") ||
                    SlogInfo.self().getCustomer("SlogCore_AGDspPcmDumpLogController").equals("1")) {
                SlogInfo.self().setCustomer("SlogCore_AGDspOutputController", "2");
            }
        } else if ("SlogCore_AGDspOutputController".equals(slogcoreName)) {
            if (SlogInfo.self().getCustomer("SlogCore_AGDspOutputController").equals("0")
                    || SlogInfo.self().getCustomer("SlogCore_AGDspOutputController").equals("1")) {
                SlogInfo.self().setCustomer("SlogCore_AGDspLogController", "0");
                SlogInfo.self().setCustomer("SlogCore_AGDspPcmDumpLogController", "0");

            }
        } else {
            return;
        }
        ivAG_DspPcmDumpLog.setImageResource(
                SlogInfo.self().getCustomer("SlogCore_AGDspPcmDumpLogController").equals("0") ?
                        R.drawable.checkbox_off : R.drawable.checkbox_on);
        ivAG_DspLog.setImageResource(
                SlogInfo.self().getCustomer("SlogCore_AGDspLogController").equals("0") ?
                        R.drawable.checkbox_off : R.drawable.checkbox_on);
        setAgDspOutputRadioBox();
    }

    @Override
    public void onClick(View v) {
        // TODO Auto-generated method stub
        switch (v.getId()) {

            case R.id.btn_commit:
                SlogInfo.self().setSceneStatus(SceneStatus.customer);
                SlogInfo.self().commitCustomerOrder();
                Toast.makeText(getApplicationContext(), "custom scene is open", Toast.LENGTH_LONG)
                        .show();
                break;

            case R.id.rl_android_log:
                onClickCheckBox(ivAndroidLog, "SlogCore_AndroidLogController");
                String arg = SlogInfo.self().getCustomer("SlogCore_AndroidLogController");

                SlogInfo.self().setCustomer("SlogCore_MainLogController", arg);
                ivMainLog.setImageResource(
                        SlogInfo.self().getCustomer("SlogCore_MainLogController").equals("0") ?
                                R.drawable.checkbox_off : R.drawable.checkbox_on);

                SlogInfo.self().setCustomer("SlogCore_SystemLogController", arg);
                ivSystemLog.setImageResource(
                        SlogInfo.self().getCustomer("SlogCore_SystemLogController").equals("0") ?
                                R.drawable.checkbox_off : R.drawable.checkbox_on);

                SlogInfo.self().setCustomer("SlogCore_RadioLogController", arg);
                ivRadioLog.setImageResource(
                        SlogInfo.self().getCustomer("SlogCore_RadioLogController").equals("0") ?
                                R.drawable.checkbox_off : R.drawable.checkbox_on);

                SlogInfo.self().setCustomer("SlogCore_EventLogController", arg);
                ivEventLog.setImageResource(
                        SlogInfo.self().getCustomer("SlogCore_EventLogController").equals("0") ?
                                R.drawable.checkbox_off : R.drawable.checkbox_on);

                SlogInfo.self().setCustomer("SlogCore_KernelLogController", arg);
                ivKernelLog.setImageResource(
                        SlogInfo.self().getCustomer("SlogCore_KernelLogController").equals("0") ?
                                R.drawable.checkbox_off : R.drawable.checkbox_on);

                SlogInfo.self().setCustomer("SlogCore_CrashLogController", arg);
                ivCrashLog.setImageResource(
                        SlogInfo.self().getCustomer("SlogCore_CrashLogController").equals("0") ?
                                R.drawable.checkbox_off : R.drawable.checkbox_on);

                break;
            case R.id.rl_tcpip_log:
                onClickCheckBox(ivTcpipLog, "SlogCore_APCapLogController");
                break;
            case R.id.rl_bt_log:
                onClickCheckBox(ivBtLog, "SlogCore_BtHciLogController");
                break;

            case R.id.rl_ap_more:
                if (llMoreApLog.getVisibility() == View.GONE) {
                    ivApMore.setImageResource(R.drawable.slogui_option_expanded);
                    llMoreApLog.setVisibility(View.VISIBLE);
                } else if (llMoreApLog.getVisibility() == View.VISIBLE) {
                    ivApMore.setImageResource(R.drawable.slogui_option_collceted);
                    llMoreApLog.setVisibility(View.GONE);
                }
                // SlogInfo.self().setCustomer("SlogCore_AndroidLogController", "0");
                // rlAndroidLog.setVisibility(View.GONE);
                break;
            case R.id.rl_ap_main:
                onClickCheckBox(ivMainLog, "SlogCore_MainLogController");
                syncAndroidLog();
                break;
            case R.id.rl_ap_system:
                onClickCheckBox(ivSystemLog, "SlogCore_SystemLogController");
                syncAndroidLog();
                break;
            case R.id.rl_ap_radio:
                onClickCheckBox(ivRadioLog, "SlogCore_RadioLogController");
                syncAndroidLog();
                break;
            case R.id.rl_ap_event:
                onClickCheckBox(ivEventLog, "SlogCore_EventLogController");
                syncAndroidLog();
                break;
            case R.id.rl_ap_kernel:
                onClickCheckBox(ivKernelLog, "SlogCore_KernelLogController");
                syncAndroidLog();
                break;
            case R.id.rl_ap_crash:
                onClickCheckBox(ivCrashLog, "SlogCore_CrashLogController");
                syncAndroidLog();
                break;
            case R.id.rl_ap_artdebug:
                onClickCheckBox(ivArtDebugLog, "SlogCore_ArtDebugLogController");
                break;

            case R.id.rl_modem_log:
                onClickCheckBox(ivModemLog, "SlogCore_ArmLogController");
                syncModemLog();
                break;
            case R.id.rl_cp_tcpip_log:
                onClickCheckBox(ivCpTcpipLog, "SlogCore_CpCapLogController");
                syncModemLog();
                break;
            case R.id.rl_auto_dump:
                onClickCheckBox(ivAutoDump, "SlogCore_EnableAutoDumpModem");
                break;

            case R.id.rl_cp_more:
                if (llMoreCpLog.getVisibility() == View.VISIBLE) {
                    ivCpMore.setImageResource(R.drawable.slogui_option_collceted);
                    llMoreCpLog.setVisibility(View.GONE);
                } else if (llMoreCpLog.getVisibility() == View.GONE) {
                    ivCpMore.setImageResource(R.drawable.slogui_option_expanded);
                    llMoreCpLog.setVisibility(View.VISIBLE);
                }
                break;

            case R.id.rl_dsp_log:
                onClickCheckBox(ivDspLog, "SlogCore_DspLogController");
                syncModemLog();
                break;

            /* SPRD: add AG-DSP log in EngineerMode @{ */
            case R.id.rl_ag_dsp_log:
                onClickCheckBox(ivAG_DspLog, "SlogCore_AGDspLogController");
                syncAGDspLog("SlogCore_AGDspLogController");
                break;
            /* @} */

            /* SPRD: Bug 568186 add CM4 log in EngineerMode @{ */
            case R.id.rl_cm4_log:
                onClickCheckBox(ivCm4Log, "SlogCore_Cm4LogController");
                break;
            /* @} */

            /* SPRD: add voice and wcn log in EngineerMode @{ */
            case R.id.rl_dsp_pcm_log:
                onClickCheckBox(ivDspPcmLog, "SlogCore_DspPcmDataController");
                break;

            case R.id.rl_arm_pcm_log:
                onClickCheckBox(ivArmPcmLog, "SlogCore_ArmPcmDataController");
                break;
            /* @} */

            case R.id.rl_dsp_more_option:
                if (llMoreDspOption.getVisibility() == View.GONE) {
                    ivDspMore.setImageResource(R.drawable.slogui_option_expanded);
                    llMoreDspOption.setVisibility(View.VISIBLE);
                } else if (llMoreDspOption.getVisibility() == View.VISIBLE) {
                    ivDspMore.setImageResource(R.drawable.slogui_option_collceted);
                    llMoreDspOption.setVisibility(View.GONE);
                }
                break;

            case R.id.rl_cp_bt_log:
                onClickCheckBox(ivCpBtLog, "SlogCore_WcnLogController");
                break;
            case R.id.rl_gps_log:
                if (!SlogCore.isSupportGPS()) {
                    Toast.makeText(getApplicationContext(), "Not supported GPS Log！",
                            Toast.LENGTH_SHORT).show();
                    return;
                }
                onClickCheckBox(ivGpsLog, "SlogCore_GpsLogController");
                break;
            case R.id.rl_dump_wcn:
                onClickCheckBox(ivDumpWcn, "SlogCore_EnableDumpWcnMemInUser");
                break;

            case R.id.slog_log_level_2:
                logLevel2.setChecked(true);
                logLevel3.setChecked(false);
                logLevel5.setChecked(false);
                //SlogAction.setLevel(SlogAction.MAINKEY, 2);
                // IATUtils.sendATCmd("AT+SPLOGLEVEL="+SlogCore.LOG_USER_CMD, "atchannel0");
                break;
            case R.id.slog_log_level_3:
                logLevel3.setChecked(true);
                logLevel5.setChecked(false);
                logLevel2.setChecked(false);
               // SlogAction.setLevel(SlogAction.MAINKEY, 3);
                // IATUtils.sendATCmd("AT+SPLOGLEVEL="+SlogCore.LOG_DEBUG_CMD, "atchannel0");
                break;
            case R.id.slog_log_level_5:
                logLevel5.setChecked(true);
                logLevel3.setChecked(false);
                logLevel2.setChecked(false);
                //SlogAction.setLevel(SlogAction.MAINKEY, 5);
                // IATUtils.sendATCmd("AT+SPLOGLEVEL="+SlogCore.LOG_FULL_CMD, "atchannel0");
                break;

            case R.id.rb_output_uart:

                setDspOutput(1);
                rbOutputModemLog.setChecked(SlogCore.dspOption == 2 ? true : false);
                rbOutputUart.setChecked(SlogCore.dspOption == 1 ? true : false);
                break;
            case R.id.rb_output_modemlog:
                setDspOutput(2);
                rbOutputModemLog.setChecked(SlogCore.dspOption == 2 ? true : false);
                rbOutputUart.setChecked(SlogCore.dspOption == 1 ? true : false);
                break;

            case R.id.tv_iq_mode:
                setIQMode();
                break;
            case R.id.tv_memory_leak:
                Intent im = new Intent(this,
                        com.sprd.engineermode.debuglog.slogui.SdSettingsActivity.class);
                startActivity(im);
                // setMemoryLeak();
                break;
            case R.id.tv_cp_log_level:
                setCpLogLevel();
                break;
            case R.id.rl_ag_dsp_more_option:
                if (llMoreAG_DspOption.getVisibility() == View.GONE) {
                    ivAG_DspMore.setImageResource(R.drawable.slogui_option_expanded);
                    llMoreAG_DspOption.setVisibility(View.VISIBLE);
                } else if (v.getVisibility() == View.VISIBLE) {
                    ivAG_DspMore.setImageResource(R.drawable.slogui_option_collceted);
                    llMoreAG_DspOption.setVisibility(View.GONE);
                }
                break;
            case R.id.iv_ag_dsp_dump_log:
                onClickCheckBox(ivAG_DspPcmDumpLog, "SlogCore_AGDspPcmDumpLogController");
                syncAGDspLog("SlogCore_AGDspPcmDumpLogController");
                break;
            case R.id.rb_ag_dsp_output_disable:
                onClickCheckBox("0", "SlogCore_AGDspOutputController");
                syncAGDspLog("SlogCore_AGDspOutputController");
                break;
            case R.id.rb_ag_dsp_output_uart:
                onClickCheckBox("1", "SlogCore_AGDspOutputController");
                syncAGDspLog("SlogCore_AGDspOutputController");
                break;
            case R.id.rb_ag_dsp_output_usb:
                onClickCheckBox("2", "SlogCore_AGDspOutputController");
                syncAGDspLog("SlogCore_AGDspOutputController");
                break;
        }

    }

    private void setIQMode() {
        new AlertDialog.Builder(this).setTitle("IQ Mode").setItems(
                new String[] {
                        "WCDMA IQ", "GSM IQ"
                }, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog,
                            int which) {
                        Intent iq = null;
                        switch (which) {
                            case 0:
                                iq = new Intent(getApplication(),
                                        com.sprd.engineermode.debuglog.WCDMAIQActivity.class);
                                break;
                            case 1:
                                iq = new Intent(getApplication(),
                                        com.sprd.engineermode.debuglog.GSMIQActivity.class);
                                break;
                        }
                        startActivity(iq);
                    }
                }
                ).setNegativeButton(this.getString(R.string.redirection_close), null).show();
    }

    // memory leak
    public void setMemoryLeak() {
        if (!EngineerModeActivity.mIsFirst) {
            AlertDialog.Builder alert = new AlertDialog.Builder(UserDefinedActivity.this);
            alert.setMessage(R.string.memory_leak_confirm);
            alert.setCancelable(false);
            alert.setPositiveButton(getString(R.string.alertdialog_ok),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int whichButton) {
                            SlogCore.setMemoryLeak(null);

                        }
                    });
            alert.setNegativeButton(getString(R.string.alertdialog_cancel),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int whichButton) {
                        }
                    });
            alert.create();
            alert.show();
        } else {
            Toast.makeText(UserDefinedActivity.this,
                    R.string.memory_leak_tip, Toast.LENGTH_SHORT)
                    .show();
        }
    }

    private int whichLevel(String x) {
        if (x.equals("Debug"))
            return 0;
        else if (x.equals("Full"))
            return 1;
        else if (x.equals("Special"))
            return 2;
        else if (x.equals("Custom"))
            return 3;
        return 0;
    }

    private void setCpLogLevel() {
        new AlertDialog.Builder(this)
                .setTitle("Log Filter")
                .
                setSingleChoiceItems(
                        new String[] {
                                "Debug", "Full", "Special", "Custom"
                        },
                        whichLevel(SlogInfo.self().mySharedPreferences.getString("logFilter",
                                "Debug")),
                        new DialogInterface.OnClickListener() {

                            @Override
                            public void onClick(DialogInterface dialog, int which) {

                                switch (which) {
                                    case 0:
                                        SlogInfo.self().mySharedPreferences
                                                .edit()
                                                .putString("logFilter", "Debug")
                                                .commit();
                                        Toast.makeText(getApplicationContext(), "set Debug",
                                                Toast.LENGTH_SHORT).show();
                                        IATUtils.sendATCmd("AT+SPLOGLEVEL="
                                                + SlogCore.LOG_DEBUG_CMD, "atchannel0");
                                        tvCpLogLevel.setText("Log Level Filter \n   Debug");
                                        break;
                                    case 1:
                                        SlogInfo.self().mySharedPreferences
                                                .edit()
                                                .putString("logFilter", "Full")
                                                .commit();
                                        Toast.makeText(getApplicationContext(), "set Full",
                                                Toast.LENGTH_SHORT).show();
                                        IATUtils.sendATCmd(
                                                "AT+SPLOGLEVEL=" + SlogCore.LOG_FULL_CMD,
                                                "atchannel0");
                                        tvCpLogLevel.setText("Log Level Filter \n   Full");
                                        break;
                                    case 2:
                                        SlogInfo.self().mySharedPreferences
                                                .edit()
                                                .putString("logFilter", "Special")
                                                .commit();
                                        Toast.makeText(getApplicationContext(), "set Special",
                                                Toast.LENGTH_SHORT).show();
                                        Intent intentSpecialSet = new Intent(
                                                "android.intent.action.SPECIAL");
                                        UserDefinedActivity.this.startActivity(intentSpecialSet);
                                        tvCpLogLevel.setText("Log Level Filter \n   Special");
                                        // IATUtils.sendATCmd("AT+SPLOGLEVEL="+SlogCore.LOG_USER_CMD,
                                        // "atchannel0");

                                        // Toast.makeText(getApplicationContext(), "333",
                                        // Toast.LENGTH_LONG).show();
                                        break;
                                    case 3:
                                        SlogInfo.self().mySharedPreferences
                                                .edit()
                                                .putString("logFilter", "Custom")
                                                .commit();
                                        Toast.makeText(getApplicationContext(), "set Custom",
                                                Toast.LENGTH_SHORT).show();
                                        Intent intentCustomSet = new Intent(
                                                "android.intent.action.CUSTOMSET");
                                        UserDefinedActivity.this.startActivity(intentCustomSet);
                                        tvCpLogLevel.setText("Log Level Filter \n   Custom");
                                        break;
                                }

                            }
                        }).setNegativeButton(this.getString(R.string.redirection_close), null)
                .show();
    }

    public void setDspOutput(int num) {
        SlogCore.dspOption = num;
    }

    public int getDspOutput() {

        String atResponse = EngineerModeProtocol.sendAt(engconstents.ENG_AT_GETDSPLOG1,
                "atchannel0");
        String responValue = EngineerModeProtocol.analysisResponse(atResponse,
                EngineerModeProtocol.GET_DSP_LOG);
        if (atResponse.contains(IATUtils.AT_OK)) {
            if (responValue.trim().equals("1")) {
                return 1;
            } else if (responValue.trim().equals("2")) {
                return 2;
            }
        }
        return 0;
    }

    private String analysisResponse(String response, int type) {
        Log.d("Slog", "analysisResponse response= " + response + "type = " + type);
        if (response == null)
            return IATUtils.AT_FAIL;
        if (!(response.contains("FAIL") && response.startsWith("Fail")))
            return IATUtils.AT_OK;
        return IATUtils.AT_FAIL;

    }

    private String SendAt(String cmd, String serverName) {
        String strTmp = IATUtils.sendATCmd(cmd, serverName);
        return strTmp;
    }
    /*
     * Log.d(TAG, "mListPreferenceScenarios value=" + re); if (re.startsWith("2") && isSupportW) {
     * Intent intentSpecial = new Intent("android.intent.action.SPECIAL");
     * ModemLogSettings.this.startActivity(intentSpecial); } else if(re.startsWith("3") ||
     * (re.startsWith("2") && (!isSupportW))) { Intent intentCustomSet = new
     * Intent("android.intent.action.CUSTOMSET");
     * ModemLogSettings.this.startActivity(intentCustomSet); } else { Message setScenariosStatus =
     * logSettingHandler.obtainMessage(SET_LOG_SCENARIOS_STATUS, re);
     * logSettingHandler.sendMessage(setScenariosStatus); return true; } //@Override void commit() {
     * ; // TODO Auto-generated method stub } //@Override void onSlogServiceConnected() {
     * runOnUiThread(new Runnable() {
     * @Override public void run() { if (mService != null) { try { // boolean general =
     * SlogAction.getState(SlogAction.GENERALKEY); // mShowSnap.setEnabled(general);
     * mService.setNotification(NOTIFICATION_SNAP, true); } catch (RemoteException e) { // TODO
     * Auto-generated catch block e.printStackTrace(); } } } }); } //@Override public void
     * onSlogServiceDisconnected() { // TODO Auto-generated method stub //super(); return ; }
     * //@Override public void syncState() { // TODO Auto-generated method stub }
     */

}
