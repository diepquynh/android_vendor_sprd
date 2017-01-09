
package com.spreadtrum.sgps;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.PendingIntent;
import android.app.ProgressDialog;
import android.app.TabActivity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.location.GpsSatellite;
import android.location.GpsStatus;
import android.location.GpsStatus.NmeaListener;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.StrictMode;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.widget.SlidingDrawer;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TabHost;
import android.widget.TabHost.OnTabChangeListener;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ScrollView;
import android.view.inputmethod.InputMethodManager;
import android.graphics.Color;
import android.util.Log;
import android.text.InputFilter;
import android.text.InputType;
import android.text.Selection;
import android.text.Spannable;
import android.text.TextUtils;
import android.text.method.DigitsKeyListener;
import android.text.method.MovementMethod;
import android.text.method.ScrollingMovementMethod;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.List;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

import com.spreadtrum.sgps.NmeaParser;
import com.spreadtrum.sgps.NmeaParser.LocalNmeaListener;

public class SgpsActivity extends TabActivity implements SatelliteDataProvider, LocalNmeaListener {

    private static final String TAG = "SGPS/Activity";
    private static final String TAG_BG = "EM/SGPS_BG";
    private static final String TAG_GPS = "GPS";

    private static final String SGPS_VRESION = "v1.0.0_2014.07.16";
    private static final String COMMAND_END = "*";
    private static final String COMMAND_START = "$";
    private static final int LOCATION_MAX_LENGTH = 12;
    private static final String INTENT_ACTION_SCREEN_OFF = "android.intent.action.SCREEN_OFF";
    private static final String SHARED_PREF_KEY_BG = "RunInBG";
    private static final String FIRST_TIME = "first.time";
    private static final String START_MODE = "start.mode";
    /* SPRD:add for GE2 @{ */
    private static final int GPS_ONLY = 0;
    private static final int GLONASS_ONLY = 1;
    private static final int BDS_ONLY = 2;
    private static final int GLONASS_GPS = 3;
    private static final int GPS_BDS = 4;
    private static final String SAVE_GPS = "save.gps";
    private static final String SAVE_GPS_LATITUDE = "save.gps.latitude";
    private static final String SAVE_GPS_LONGITUDE = "save.gps.longitude";
    /* @} */
    private static final String START_MODE_AUTO = "start.mode.auto";
    private static final String UART_LOG_SWITCH = "uart.log.switch";
    private static final int HOT_START = 0;
    private static final int COLD_START = 1;
    private static final int WARM_START = 2;
    private static final int FULL_START = 3;
    private static final String NMEA_LOG_SUFX = ".txt";
    private static final String NMEA_LOG_PREX = "Nmealog";
    private static final int MESSAGE_ARG_1 = 1;
    private static final int MESSAGE_ARG_0 = 0;
    private static final int INPUT_VALUE_MAX = 999;
    private static final int INPUT_VALUE_MIN = 0;
    private static final int INPUT_GPS_VALUE_MAX = 360;
    private static final int INPUT_GPS_VALUE_MIN = -360;
    private static final int ONE_SECOND = 1000;
    private static final int HANDLE_MSG_DELAY = 200;
    private static final int INTERVAL_MIN = 5;

    private static final boolean NMEALOG_SD = true;
    private static final String NMEALOG_PATH = "/data/misc/nmea_log";
    private static final int COUNT_PRECISION = 500;
    private static final int EXCEED_SECOND = 999;

    private static final String AUTOTEST_LOG_SUFX = ".txt";
    private static final String AUTOTEST_LOG_PREX = "Autotestlog";
    private static final boolean AUTOTESTLOG_SD = true;
    private static final String AUTOTESTLOG_PATH = "/data/misc/autotest_log";
    private static final int HANDLE_COUNTER = 1000;
    private static final int HANDLE_UPDATE_RESULT = 1001;
    private static final int HANDLE_CLEAR = 1002;
    private static final int HANDLE_CHECK_SATEREPORT = 1003;
    private static final int HANDLE_SET_CURRENT_TIMES = 1030;
    private static final int HANDLE_START_BUTTON_UPDATE = 1040;
    private static final int HANDLE_SET_COUNTDOWN = 1050;
    private static final int HANDLE_SET_MEANTTFF = 1070;
    private static final int HANDLE_EXCEED_PERIOD = 1080;
    private static final int HANDLE_TTFF_TIMEOUT = 1060;
    private static final int HANDLE_SET_PARAM_RECONNECT = 1090;
    private static final int HANDLE_COMMAND_JAMMINGSCAN = 1101;
    private static final int HANDLE_COMMAND_GETVERSION = 1102;
    private static final int HANDLE_COMMAND_OTHERS = 1103;
    private static final int HANDLE_COMMAND_OTHERS_UPDATE_RESULT_HINT = 1104;
    private static final int HANDLE_COMMAND_OTHERS_UPDATE_RESULT_LOG = 1105;
    private static final int HANDLE_COMMAND_OTHERS_UPDATE_RESULT_LOG_END = 1106;
    private static final int HANDLE_COMMAND_OTHERS_UPDATE_PROVIDER = 1107;
    private static final int HANDLE_SHOW_GEOFENCE_INFO = 1108;

    private static final int HANDLE_AUTO_TRANSFER_START_BUTTON_UPDATE = 1008;
    private static final int HANDLE_AUTO_TRANSFER_SET_PARAM_RECONNECT = 1009;
    private static final int HANDLE_AUTO_TRANSFER_UPDATE_CURRENT_MODE = 1010;
    private static final int HANDLE_AUTO_TRANSFER_START_BUTTON_UPDATE_02 = 1011;

    private static final int SOCKET_DATA_TYPE_GEOFENCE = 1;

    private static final DateFormat DATE_FORMAT = new SimpleDateFormat(
            "yyyyMMddhhmmss");
    private static final int DIALOG_WAITING_FOR_STOP = 0;
    private static final String GPS_EXTRA_POSITION = "position";
    private static final String GPS_EXTRA_EPHEMERIS = "ephemeris";
    private static final String GPS_EXTRA_TIME = "time";
    private static final String GPS_EXTRA_IONO = "iono";
    private static final String GPS_EXTRA_UTC = "utc";
    private static final String GPS_EXTRA_HEALTH = "health";
    private static final String GPS_EXTRA_ALL = "all";
    private static final String GPS_EXTRA_RTI = "rti";
    private static final String GPS_EXTRA_A1LMANAC = "almanac";
    /* SPRD: add for GE2 @{ */
    private static final String GPS_EXTRA_GPSONLY = "$PSPRD,00,3,1";
    private static final String GPS_EXTRA_GLONASS = "$PSPRD,00,3,4";
    private static final String GPS_EXTRA_BDSONLY = "$PSPRD,00,3,2";
    private static final String GPS_EXTRA_GLONASSGPS = "$PSPRD,00,3,5";
    private static final String GPS_EXTRA_GPSBDS = "$PSPRD,00,3,3";
    private static final String GPS_EXTRA_LTE_SWITCH_ON = "lteswitchon";
    private static final String GPS_EXTRA_LTE_SWITCH_OFF = "lteswitchoff";
    /* @} */
    private static final String GPS_EXTRA_LOG_SWITCH_ON = "svdir";
    private static final String GPS_EXTRA_LOG_SWITCH_OFF = "sadata";

    private static final int YEAR_START = 1900;
    private static final int GRAVITE_Y_OFFSET = 150;
    private static final int RESPONSE_ARRAY_LENGTH = 4;
    private static final int SATE_RATE_TIMEOUT = 3;

    private static Bundle Bundletemp = null;
    private static boolean ViewLogTab = false;
    private boolean mViewLogTab = false;
    private boolean mGpsAutoTest = false;

    private int[] mUsedInFixMask = new int[SatelliteDataProvider.SATELLITES_MASK_SIZE];
    private int mTtffValue = 0;
    private int mSatellites = 0;
    private int mTotalTimes = 0;
    private int mCurrentTimes = 0;
    private int mTestInterval = 0;
    private float mMeanTTFF = 0f;
    private int[] mPrns = new int[MAX_SATELLITES_NUMBER];
    private float[] mSnrs = new float[MAX_SATELLITES_NUMBER];
    private float[] mElevation = new float[MAX_SATELLITES_NUMBER];
    private float[] mAzimuth = new float[MAX_SATELLITES_NUMBER];

    private int[] mPrnsForCN0 = new int[5];
    private float[] mSrnsForCN0 = new float[5];
    private int[] mContForCN0 = new int[5];
    private int cont = 0;
    private float[] mTtff;
    private int mDistanceCont = 0;
    private float[] mDistance;
    private double[] mFirstFixLatitude;
    private double[] mFirstFixLongitude;
    private int mAutoTransferTotalTimes = 0;
    private int mAutoTransferTestInterval = 0;
    private int mAutosTransferInternal = 0;

    private boolean mShowLoc = false;
    private boolean mStartNmeaRecord = false;
    private boolean mFirstFix = false;
    private boolean mIsNeed3DFix = false;

    private static final Object mTestRunningLock = new Object();// thread lock
    private static final Object mAutoTransferTestRunningLock = new Object();// thread lock
    private boolean mIsTestRunning = false;
    private boolean mIsAutoTransferTestRunning = false;
    private boolean mIsExit = false;

    private boolean mIsRunInBg = true;// default can run in background mode
    private boolean mShowFirstFixLocate = true;
    private boolean mIsShowVersion = false;
    private int mSateReportTimeOut = 0;
    private boolean locationWhenFirstFix = false;

    private ClientSocket mSocketClient = null;

    private SatelliteSkyView mSatelliteView = null;
    private SatelliteSignalView mSignalView = null;
    private NmeaListenClass mNmeaListener = null;
    private LocationManager mLocationManager = null;
    private SgpsWakeLock mSgpsWakeLock = null;
    private Location mLastLocation = null;
    private Location mLastLocationRecord = null;
    private Location mLastLocationRefence = null;
    private GpsStatus mLastStatus = null;
    private GpsStatus mLastStatusRecord = null;
    private Button mBtnColdStart = null;
    private Button mBtnWarmStart = null;
    private Button mBtnHotStart = null;
    private Button mBtnFullStart = null;
    private Button mBtnReStart = null;
    private Button mBtnHotStill = null;
    private Button mBtnNmeaStart = null;
    private Button mBtnNMEAStop = null;
    private Button mBtnNMEADbgDbg = null;
    private Button mBtnNmeaDbgNmea = null;
    private Button mBtnNmeaDbgDbgFile = null;
    private Button mBtnNmeaDbgNmeaDdms = null;
    private Button mBtnNmeaClear = null;
    private Button mBtnNmeaSave = null;
    private Button mBtnGpsTestStart = null;
    private Button mBtnGpsTestStop = null;
    private Button mBtnGpsTestSave = null;

    private Button mBtnGpsTestAutoTransferStart = null;
    private Button mBtnGpsTestAutoTransferStop = null;
    private EditText mEtTestTimes_01 = null;
    private EditText mEtTestInterval_01 = null;
    private EditText mEtTestTransferInterval = null;
    private EditText mEtTTFFTimeout = null;

    private Button mBtnSuplLog = null;
    private EditText mEtTestTimes = null;
    private CheckBox mCbNeed3DFix = null;
    private CheckBox mTvUartLogSwitch = null;// tv_uart_log_switch
    private EditText mEtTestInterval = null;

    private ScrollView mScrollLog = null;
    private RadioButton mRadioBtnHot = null;
    private RadioButton mRadioBtnCold = null;
    private RadioButton mRadioBtnWarm = null;
    private RadioButton mRadioBtnFull = null;
    private Button mBtnStart = null;
    private Button mBtnAssert = null;
    /* SPRD:add for GE2 @{ */
    private RadioButton mRadioBtnGps = null;
    private RadioButton mRadioBtnGlonass = null;
    private RadioButton mRadioBtnBDS = null;
    private RadioButton mRadioBtnGlonassGps = null;
    private RadioButton mRadioBtnGpsBDS = null;
    private Switch mSpreadOrBitSwitch = null;
    private boolean IsOrBitSwitchOpen = false;
    /* @} */
    private Switch mRealEPHSwitch = null;
    private Switch mGNSSLogSwitch = null;
    private ScrollView mScrollResult = null;
    private TextView mTvRestartMode = null;
    private TextView mTvResultHint = null;
    private TextView mTvResultLog = null;
    private TextView mTvSGPSVersion = null;
    private EditText mEtTestLatitude = null;
    private EditText mEtTestLongitude = null;

    private AlertDialog alertDialog = null;
    private AlertDialog.Builder builder = null;
    private AlertDialog.Builder gpsBuilder = null;

    private double mTestLatitude = -1.0;
    private double mTestLongitude = -1.0;
    private int mLastTtffValue = 0;
    private float mLastDistance = 0;
    private float mTestMeanTTFF = 0f;
    private float mTestMeanDistance = 0f;

    private AutoTestThread mAutoTestThread = null;
    private AutoCircleTestThread mAutoCircleTestThread = null;
    private String mProvider = "";
    private String mStatus = "";
    private boolean mStopPressedHandling = false;
    private boolean mStartPressedHandling = false;

    private boolean mStopPressedAutoTransferHandling = false;
    private boolean mStartPressedAutoTransferHandling = false;

    private TextView mTvNmeaLog = null;
    private TextView mTvNMEAHint = null;
    private FileOutputStream mOutputNMEALog = null;
    private FileOutputStream mOutputAUTOTESTLog = null;
    // added for Jamming Scan
    private Button mBtnGpsHwTest = null;
    private Button mBtnGpsJamming = null;
    private EditText mEtGpsJammingTimes = null;
    // added end

    private CheckBox mChkAutoTestHot = null;
    private CheckBox mChkAutoTestCold = null;
    private CheckBox mChkAutoTestWarm = null;
    private CheckBox mChkAutoTestFull = null;

	private CheckBox mCircAutoTestHot = null;
	private CheckBox mCircAutoTestCold = null;
	private CheckBox mCircAutoTestWarm = null;
	private CheckBox mCircAutoTestFull = null;
    private RadioButton mRadioBtnAutoHot = null;
    private RadioButton mRadioBtnAutoCold = null;
    private RadioButton mRadioBtnAutoWarm = null;
    private RadioButton mRadioBtnAutoFull = null;

    // added to receive PowerKey pressed
    private IntentFilter mPowerKeyFilter = null;
    private BroadcastReceiver mPowerKeyReceiver = null;
    // added end
    private Toast mPrompt = null;
    private Toast mStatusPrompt = null;

    private int viewpages = 5;
    private boolean mAutoTestHot = false;
    private boolean mAutoTestWarm = false;
    private boolean mAutoTestCold = false;
    private boolean mAutoTestFull = false;
    private int mAutoTestCurrent = 0;
    private boolean isContinue = false;
    private int currCountTimes = 0;

    // add agps test
    private Switch msgpsSwitch = null;
/*    private Switch magpsNetworkSwitch = null;*/
    private RadioGroup magpsNetworkSwitch = null;
    private int[] magpsNetworkSwitchItemId;
    private RadioGroup mRGPlaneSwitch = null;
    private int[] mRGPlaneSwitchItemId;
    private RadioGroup mRGMolrPositionMethod = null;
    private int[] mRGMolrPositionMethodItemId;
    private RadioGroup mRGSimSelection = null;
    private int[] mRGSimSelectionItemId;

    private CheckBox mExternalAddressCheckBox = null;
    private CheckBox mMLCNumberCheckBox = null;
    private CheckBox mAutoResetCheckBox = null;

    private EditText mExternalAddressEditText = null;
    private EditText mMLCNumberEditText = null;

    private Button mExternalAddressSaveButton = null;
    private Button mMLCNumberSaveButton = null;
    private Switch mMOLASwitch = null;
    private boolean resetToDefault = false;

    // agps userplane settings
    private RadioGroup mAGPSModeRadioGroup = null;
    private int[] mAGPSModeRadioGroupItemId;
    private Button mSLPTemplateButton = null;
    private Button mSLPTestButton = null;
    private TextView mSLPTestResultTextView = null;
    private TextView mSLPAddressTextView = null;
    private Button mSLPAddressEditButton = null;
    private TextView mSLPPortTextView = null;
    private Button mSLPPortEditButton = null;
    private CheckBox mTLSEnableCheckBox = null;
    private RadioGroup mSetIDRadioGroup = null;
    private int[] mSetIDRadioGroupItemId;
    private RadioGroup mAccuracyUnitRadioGroup = null;
    private int[] mAccuracyUnitRadioGroupItemId;
    private TextView mHorizontalAccuracyTextView = null;
    private Button mHorizontalAccuracyEditButton = null;
    private TextView mVerticalAccuracyTextView = null;
    private Button mVerticalAccuracyEditButton = null;
    private TextView mLocationAgeTextView = null;
    private Button mLocationAgeEditButton = null;
    private TextView mDelayTextView = null;
    private Button mDelayEditButton = null;
    private CheckBox mCertificateVerificationCheckBox = null;
    private Button mCertificateVerificationEditButton = null;
    private CheckBox mEnableLabIOTTestCheckBox = null;
    private CheckBox mEnableeCIDCheckBox = null;

    // agps supl2.0
    private CheckBox mEnableSUPL2CheckBox = null;
    private LinearLayout mAGPSSUPL2Layout = null;
    private Button mSILRButton = null;
    private TextView mMSISDNTextView = null;
    private Button mMSISDNEditButton = null;
    private CheckBox mEnable3rdMSISDNCheckBox = null;
    private TextView m3rdMSISDNTextView = null;
    private Button m3rdMSISDNEditButton = null;
    private Button mTriggerStartButton = null;
    private Button mTriggerAbortButton = null;
    private RadioGroup mTriggerTypeRadioGroup = null;
    private int[] mTriggerTypeGroupItemId;
    private LinearLayout mPerodicTypeLayout = null;
    private LinearLayout mAreaTypeLayout = null;
    private TextView mPosMethodView = null;
    private Button mPosMethodSelectButton = null;
    private TextView mPerodicMinIntervalTextView = null;
    private Button mPerodicMinIntervalEditButton = null;
    private TextView mPerodicStartTimeTextView = null;
    private Button mPerodicStartTimeEditButton = null;
    private TextView mPerodicStopTimeTextView = null;
    private Button mPerodicStopTimeEditButton = null;
    private TextView mAreaTypeTextView = null;
    private Button mAreaTypeSelectButton = null;
    private TextView mAreaMinIntervalTextView = null;
    private Button mAreaMinIntervalEditButton = null;
    private TextView mMaxNumTextView = null;
    private Button mMaxNumEditButton = null;
    private TextView mAreaStartTimeTextView = null;
    private Button mAreaStartTimeEditButton = null;
    private TextView mAreaStopTimeTextView = null;
    private Button mAreaStopTimeEditButton = null;
    private Button mGeographicButton = null;
    private TextView mGeographicTextView = null;
    private TextView mGeoRadiusTextView = null;
    private Button mGeoRadiusEditButton = null;
    private TextView mLatitudeTextView = null;
    private Button mLatitudeEditButton = null;
    private TextView mLongitudeTextView = null;
    private Button mLongitudeEditButton = null;
    private RadioGroup mSignRadioGroup = null;
    private int[] mSignRadioGroupItemId;

    // agps common
    private CheckBox mAllowNetworkInitiatedRequestCheckBox = null;
    private CheckBox mAllowEMNotificationCheckBox = null;
    private CheckBox mAllowRoamingCheckBox = null;
    private CheckBox mLogSUPLToFileCheckBox = null;
    private Spinner mNotificationTimeoutSpinner = null;
    private Spinner mVerificationTimeoutSpinner = null;
    private Button mResetToDefaultButton = null;
    private Button mNITestSelectButton = null;

    private String[] mSLPArray;
    private ArrayList<String> mSLPNameList = new ArrayList<String>();
    private ArrayList<String> mSLPValueList = new ArrayList<String>();
    private String[] mPosMethodArray;
    private String[] mPosMethodArrayValues;
    private String[] mAreaTypeArray;
    private String[] mAreaTypeArrayValues;
    private String[] mNotificationTimeoutArray;
    private String[] mVerificationTimeoutArray;
    private String[] mNiDialogTestArray;
    private String[] mNiDialogTestArrayValues;

    private static final int DIALOG_SLP_TEMPLATE = 100;
    private static final int DIALOG_SLP_ADDRESS = 101;
    private static final int DIALOG_SLP_PORT = 102;
    private static final int DIALOG_HORIZONTAL_ACCURACY = 103;
    private static final int DIALOG_VERTICAL_ACCURACY = 104;
    private static final int DIALOG_LOCATIONAGE = 105;
    private static final int DIALOG_DELAY = 106;
    private static final int DIALOG_CERTIFICATEVERIFICATION = 107;
    private static final int DIALOG_MSISDN = 108;
    private static final int DIALOG_3RDMSISDN = 109;
    private static final int DIALOG_POSMETHOD_SELECT = 110;
    private static final int DIALOG_PERODIC_MININTERVAL = 111;
    private static final int DIALOG_PERODIC_STARTTIME = 112;
    private static final int DIALOG_PERODIC_STOPTIME = 113;
    private static final int DIALOG_AREA_TYPE_SELECT = 114;
    private static final int DIALOG_AREA_MININTERVAL = 115;
    private static final int DIALOG_MAXNUM = 116;
    private static final int DIALOG_AREA_STARTTIME = 117;
    private static final int DIALOG_AREA_STOPTIME = 118;
    private static final int DIALOG_GEORADIUS = 119;
    private static final int DIALOG_LATITUDE = 120;
    private static final int DIALOG_LONGITUDE = 121;
    private static final int DIALOG_NI_DIALOG_TEST = 122;
    private Switch mSingleSatelliteSwitch = null;

    private static final int SATELLITE_MIN_COUNT = 0;
    private static final int SATELLITE_MAX_COUNT = 100;

    private TextView mCurrentTimeHot = null;
    private TextView mCurrentTimeWarm = null;
    private TextView mCurrentTimeCold = null;
    private TextView mCurrentTimeFactory = null;

    private TextView mCountDownHot = null;
    private TextView mCountDownWarm = null;
    private TextView mCountDownCold = null;
    private TextView mCountDownFactory = null;
    private TextView mAverageCn0TextView = null;

    private TextView mLastTTFFHot = null;
    private TextView mLastTTFFWarm = null;
    private TextView mLastTTFFCold = null;
    private TextView mLastTTFFFactory = null;

    private TextView mAverageHot = null;
    private TextView mAverageWarm = null;
    private TextView mAverageCold = null;
    private TextView mAverageFactory = null;

    private TextView mHotSuccessRate = null;
    private TextView mColdSuccessRate = null;
    private TextView mWarmSuccessRate = null;
    private TextView mFullSuccessRate = null;

    private boolean mEnterCn0FirstFlag = true;
    private boolean mTtffTimeoutFlag = false;
    private long mSFST = 0;
    private long mStartSerchTime = 0;
    private long mSerchFirstSateTime = 0;
    private boolean mSerchFirstSateFlag = false;
    private boolean mFirstFixFlag = false;
    private int mSatelliteTestCont = 0;
    private int[] mTotalInused = null;
    private int[] mTotalView = null;
    private int[] mGpsInUsed = null;
    private int[] mGpsView = null;
    private int[] mGlonassInUsed = null;
    private int[] mGlonassView = null;
    private int[] mBeidouInUsed = null;
    private int[] mBeidouView = null;
    private int mTotalInUsedMin = SATELLITE_MAX_COUNT;
    private int mTotalInUsedMax = SATELLITE_MIN_COUNT;
    private int mTotalViewMin = SATELLITE_MAX_COUNT;
    private int mTotalViewMax = SATELLITE_MIN_COUNT;
    private int mGpsInUsedMin = SATELLITE_MAX_COUNT;
    private int mGpsInUsedMax = SATELLITE_MIN_COUNT;
    private int mGpsViewMin = SATELLITE_MAX_COUNT;
    private int mGpsViewMax = SATELLITE_MIN_COUNT;
    private int mGlonassInUsedMin = SATELLITE_MAX_COUNT;
    private int mGlonassInUsedMax = SATELLITE_MIN_COUNT;
    private int mGlonassViewMin = SATELLITE_MAX_COUNT;
    private int mGlonassViewMax = SATELLITE_MIN_COUNT;
    private int mBeidouInUsedMin = SATELLITE_MAX_COUNT;
    private int mBeidouInUsedMax = SATELLITE_MIN_COUNT;
    private int mBeidouViewMin = SATELLITE_MAX_COUNT;
    private int mBeidouViewMax = SATELLITE_MIN_COUNT;
    private int[] mSateTracking = null;
    private int mSateTrackingCont = 0;
    private int mTimeoutValue = 999;
    private int mTTFFTimeoutCont = 0;
    private int mCurrentMode = 0;
    private boolean mIsCompleted = false;
	private boolean mIsAutoTransferMode = false;
	private boolean mGpsTestStart = false;
	private String mGpsConfigFile = null;
    private String mGe2Cn_Sr = null;

    private AlertDialog mGeofenceDialog = null;

    //add new feature for gps download bin by ansel.li
    private RadioButton mRadiobtnGpsBeidou = null;
    private RadioButton mRadiobtnGpsGlonass = null;

    private NmeaParser nmeaParser = null;
    private static final boolean GPS_CHIP_TYPE_IS_GE2 = "ge2".equals(SgpsUtils.GPS_CHIP_TYPE);

    public void setSatelliteStatus(int svCount, int[] prns, float[] snrs,
            float[] elevations, float[] azimuths, int ephemerisMask,
            int almanacMask, int[] usedInFixMask) {
        Log.v(TAG, "Enter setSatelliteStatus function");
        synchronized (this) {
            emptyArray();
            mSatellites = svCount;
            System.arraycopy(prns, 0, mPrns, 0, mSatellites);
            System.arraycopy(snrs, 0, mSnrs, 0, mSatellites);
            System.arraycopy(elevations, 0, mElevation, 0, mSatellites);
            System.arraycopy(azimuths, 0, mAzimuth, 0, mSatellites);
            System.arraycopy(usedInFixMask, 0, mUsedInFixMask, 0,
                    SatelliteDataProvider.SATELLITES_MASK_SIZE);
        }
        mSatelliteView.postInvalidate();
        mSignalView.postInvalidate();
    }

    public void setSatelliteStatus(Iterable<GpsSatellite> list) {
        Log.v(TAG, "Enter setSatelliteStatus function");
        if (null == list) {
            emptyArray();
        } else {
            synchronized (this) {
                emptyArray();
                int index = 0;
                for (GpsSatellite sate : list) {
                    mPrns[index] = sate.getPrn();
                    mSnrs[index] = sate.getSnr();
                    mElevation[index] = sate.getElevation();
                    mAzimuth[index] = sate.getAzimuth();
                    if (sate.usedInFix()) {
                        int i = mPrns[index] - 1;
                        mUsedInFixMask[i
                                / SatelliteDataProvider.SATELLITES_MASK_BIT_WIDTH] |= (1 << (i % SatelliteDataProvider.SATELLITES_MASK_BIT_WIDTH));
                    }
                    index++;
                }
                mSatellites = index;
            }
        }
        Log.v(TAG,
                "Found " + mSatellites + " Satellites:"
                        + toString(mPrns, mSatellites) + ","
                        + toString(mSnrs, mSatellites));
        for (int i = 0; i < mUsedInFixMask.length; i++) {
            Log.v(TAG,
                    "Satellites Masks " + i + ": 0x"
                            + Integer.toHexString(mUsedInFixMask[i]));
        }
        mSatelliteView.postInvalidate();
        mSignalView.postInvalidate();
    }

    public int getSatelliteStatus(int[] prns, float[] snrs, float[] elevations,
            float[] azimuths, int ephemerisMask, int almanacMask,
            int[] usedInFixMask) {
        synchronized (this) {
            if (prns != null) {
                System.arraycopy(mPrns, 0, prns, 0, mSatellites);
            }
            if (snrs != null) {
                System.arraycopy(mSnrs, 0, snrs, 0, mSatellites);
            }
            if (azimuths != null) {
                System.arraycopy(mAzimuth, 0, azimuths, 0, mSatellites);
            }
            if (elevations != null) {
                System.arraycopy(mElevation, 0, elevations, 0, mSatellites);
            }
            if (usedInFixMask != null) {
                System.arraycopy(mUsedInFixMask, 0, usedInFixMask, 0,
                        SatelliteDataProvider.SATELLITES_MASK_SIZE);
            }
            return mSatellites;
        }
    }

    private boolean isUsedInFix(int prn) {
        int innerPrn = prn;
        boolean result = false;
        if (0 >= innerPrn) {
            for (int mask : mUsedInFixMask) {
                if (0 != mask) {
                    result = true;
                    break;
                }
            }
        } else {
            innerPrn = innerPrn - 1;
            int index = innerPrn / SatelliteDataProvider.SATELLITES_MASK_BIT_WIDTH;
            int bit = innerPrn % SatelliteDataProvider.SATELLITES_MASK_BIT_WIDTH;
            result = (0 != (mUsedInFixMask[index] & (1 << bit)));
        }
        return result;
    }

    @Override
    public void onPause() {
        super.onPause();
        Log.d(TAG, "Enter onPause function");
        /* SPRD: fix bug336487 Nmea view dump @{*/
        ViewLogTab = false;
        /* @}*/
    }

    public final CompoundButton.OnCheckedChangeListener mSwitchCheckedChangeListener = new CompoundButton.OnCheckedChangeListener() {

        @Override
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
            int viewId = buttonView.getId();
            switch (viewId) {
                case R.id.spreadorbit_switch:
                    if (isChecked) {
                        sendCommand("$PSPRD,00,4,1");
                    } else {
                        sendCommand("$PSPRD,00,4,2");
                    }
                    break;
                case R.id.real_eph_switch:
                    setConfigration(SgpsUtils.GPS_CSR_CONFIG_FIL_FOR_GE2, "REALEPH-ENABLE",
                            isChecked ? "TRUE" : "FALSE");
                    break;
                case R.id.gnss_log_switch:
                    setConfigration(SgpsUtils.GPS_CSR_CONFIG_FIL_FOR_GE2, "LOG-ENABLE", isChecked ? "TRUE"
                            : "FALSE");
                    break;
                case R.id.single_satellite_switch:
                    setConfigration(mGpsConfigFile, "SINGLE-SATELLITE",
                            isChecked ? "TRUE" : "FALSE");
                    break;
                default:
                    Log.d(TAG, "viewId is not found!");
            }
        }
    };

    /* SPRD:add for GE2 @{ */
    public final OnClickListener mBtnGpsClickListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mStatusPrompt != null) {
                mStatusPrompt.cancel();
            }
            int mode = -1;
            if (v == (View) mRadioBtnGps) {
                mode = GPS_ONLY;
            } else if (v == (View) mRadioBtnGlonass) {
                mode = GLONASS_ONLY;
            } else if (v == (View) mRadioBtnBDS) {
                mode = BDS_ONLY;
            } else if (v == (View) mRadioBtnGlonassGps) {
                mode = GLONASS_GPS;
            } else if (v == (View) mRadioBtnGpsBDS) {
                mode = GPS_BDS;
            }
            setGNSSMode(mode);
            setCommandToProvider(mode);
        }
    };
    //add new feature for gps download bin by ansel.li
    public final OnClickListener mBtnGpsImgModeClickListener = new OnClickListener(){
        @Override
        public void onClick(View v) {
            if(v == (View) mRadiobtnGpsBeidou)
            {
                if (setConfigration(SgpsUtils.GPS_CSR_CONFIG_FIL_FOR_GE2, "GPS-IMG-MODE",
                        "GNSSBDMODEM")) {
                    mRadioBtnGlonass.setEnabled(false);
                    mRadioBtnGlonassGps.setEnabled(false);
                    mRadioBtnBDS.setEnabled(true);
                    mRadioBtnGpsBDS.setEnabled(true);
                    setConfigration(SgpsUtils.GPS_CSR_CONFIG_FIL_FOR_GE2, "CP-MODE", "011");
                    reStartGPS();
                } else{
                    Log.d(TAG, "Set GPS-IMG-MODE to GNSSBDMODEM failed.");
                }
            } else if(v == (View)mRadiobtnGpsGlonass){
                if (setConfigration(SgpsUtils.GPS_CSR_CONFIG_FIL_FOR_GE2, "GPS-IMG-MODE",
                        "GNSSMODEM")) {
                    mRadioBtnBDS.setEnabled(false);
                    mRadioBtnGpsBDS.setEnabled(false);
                    mRadioBtnGlonass.setEnabled(true);
                    mRadioBtnGlonassGps.setEnabled(true);
                    setConfigration(SgpsUtils.GPS_CSR_CONFIG_FIL_FOR_GE2, "CP-MODE", "101");
                    reStartGPS();
                } else{
                    Log.d(TAG, "Set GPS-IMG-MODE to GNSSMODEM failed.");
                }
            }
        }
    };
    /* @} */
    public final OnClickListener mBtnClickListener = new OnClickListener() {

        @Override
        public void onClick(View v) {
            if (mStatusPrompt != null) {
                mStatusPrompt.cancel();
            }
            Bundle extras = new Bundle();
			if (v == (View) mBtnGpsTestStart) {
				mBtnGpsTestStart.refreshDrawableState();
				mBtnGpsTestStart.setEnabled(false);
				Log.d(TAG, "GPSTest Start button is pressed");
				mHandler.sendEmptyMessageDelayed(HANDLE_CLEAR, HANDLE_MSG_DELAY);
				startGPSAutoTest();

				InputMethodManager im = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
				View viewOnFocus = getCurrentFocus();
				if (viewOnFocus != null) {
					im.hideSoftInputFromWindow(
							viewOnFocus.getApplicationWindowToken(),
							InputMethodManager.HIDE_NOT_ALWAYS);
				}
			} else if (v == (View) mBtnGpsTestStop) {
				if (!getmIsTestRunning()) {
					return;
				}
                mBtnGpsTestStop.setEnabled(false);
                mBtnGpsTestStop.refreshDrawableState();
                // purpose is to test weather mProgressDialog is already exist
                // or not, to resolve the multi ProgressDialog created problem
                // case that as soon as mProgressDialog is dismissed,
                // mBtnGPSTestStop can still be pressed, and this dialog will
                // pop up but will not be dismissed automatically
                // if(null == mProgressDialog)
                if (!mStopPressedHandling) {
                    mStopPressedHandling = true;
                    showDialog(DIALOG_WAITING_FOR_STOP);
                    setmIsTestRunning(false);
                } else {
                    Log.d(TAG, "stop has been clicked.");
                }
            } else if (v == (View) mBtnGpsTestSave){
                Log.d(TAG, "mBtnGPSTestSave is clicked");
                if (null != mEtTestLatitude && null != mEtTestLongitude) {
                    if (0 == mEtTestLatitude.getText().length()
                            || 0 == mEtTestLongitude.getText().length()) {
                        // do nothing

                    } else {
                        Double mTestLatitude_test = 0.0;
                        Double mTestLongitude_test = 0.0;
                        try {
                            mTestLatitude_test = Double.valueOf(mEtTestLatitude
                                    .getText().toString());
                            mTestLongitude_test = Double.valueOf(mEtTestLongitude
                                    .getText().toString());
                        } catch (Exception ex) {
                            Toast.makeText(SgpsActivity.this,
                                    R.string.toast_gps_input_range, Toast.LENGTH_LONG)
                                    .show();
                            return;
                        }

                        if ((mTestLatitude_test.doubleValue() < INPUT_GPS_VALUE_MIN || mTestLatitude_test
                                .doubleValue() > INPUT_GPS_VALUE_MAX)
                                || (mTestLongitude_test.doubleValue() < INPUT_GPS_VALUE_MIN || mTestLongitude_test
                                        .doubleValue() > INPUT_GPS_VALUE_MAX)) {
                            Toast.makeText(SgpsActivity.this,
                                    R.string.toast_gps_input_range, Toast.LENGTH_LONG)
                                    .show();
                            return;
                        }
                        final SharedPreferences preferences = SgpsActivity.this.getSharedPreferences(SAVE_GPS,
                                android.content.Context.MODE_PRIVATE);
                        preferences.edit().putString(SAVE_GPS_LATITUDE, mTestLatitude_test.toString()).commit();
                        preferences.edit().putString(SAVE_GPS_LONGITUDE, mTestLongitude_test.toString()).commit();
                        Toast.makeText(SgpsActivity.this, R.string.save_latitude_longitude, Toast.LENGTH_LONG)
                        .show();
                    }
                }
            }else if(v == (View) mBtnGpsTestAutoTransferStart) {
                mBtnGpsTestAutoTransferStart.setEnabled(false);
                mBtnGpsTestAutoTransferStart.refreshDrawableState();
                Log.d(TAG, "GPSTestAutoTransfer Start button is pressed");
                startGPSAutoTransferTest();
            }else if(v == (View) mBtnGpsTestAutoTransferStop) {
                if (!getmIsAutoTransferTestRunning()) {
                    return;
                }
                mBtnGpsTestAutoTransferStop.setEnabled(false);
                mBtnGpsTestAutoTransferStop.refreshDrawableState();
                Log.d(TAG, "GPSTestAutoTransfer Stop button is pressed");

                if (!mStopPressedAutoTransferHandling) {
                    mStopPressedAutoTransferHandling = true;
                    showDialog(DIALOG_WAITING_FOR_STOP);
                    //mStartPressedAutoTransferHandling = false;
                    setmIsAutoTransferTestRunning(false);
                } else {
                    Log.d(TAG, "GPSTestAutoTransfer stop has been clicked.");
                }
            } else if (v == (View) mBtnHotStart) {
                if (gpsTestRunning()) {
                    return;
                }
                mShowFirstFixLocate = true;
                enableBtns(false);
                // nothing should be put
                Log.v(TAG, "Hot Start button is pressed");
                extras.putBoolean(GPS_EXTRA_RTI, true);
                resetParam(extras, false, true);
                // enableBtns(true);
                mHandler.sendEmptyMessageDelayed(HANDLE_CLEAR, HANDLE_MSG_DELAY);
            } else if (v == (View) mBtnWarmStart) {
                if (gpsTestRunning()) {
                    return;
                }
                mShowFirstFixLocate = true;
                enableBtns(false);
                Log.v(TAG, "Warm Start button is pressed");
                extras.putBoolean(GPS_EXTRA_EPHEMERIS, true);
                resetParam(extras, false, true);
                // enableBtns(true);
                mHandler.sendEmptyMessageDelayed(HANDLE_CLEAR, HANDLE_MSG_DELAY);
            } else if (v == (View) mBtnColdStart) {
                if (gpsTestRunning()) {
                    return;
                }
                mShowFirstFixLocate = true;
                enableBtns(false);
                Log.v(TAG, "Cold Start button is pressed");
                extras.putBoolean(GPS_EXTRA_EPHEMERIS, true);
                extras.putBoolean(GPS_EXTRA_POSITION, true);
                extras.putBoolean(GPS_EXTRA_TIME, true);
                extras.putBoolean(GPS_EXTRA_IONO, true);
                extras.putBoolean(GPS_EXTRA_UTC, true);
                extras.putBoolean(GPS_EXTRA_HEALTH, true);
                resetParam(extras, false, true);
                // enableBtns(true);
                mHandler.sendEmptyMessageDelayed(HANDLE_CLEAR, HANDLE_MSG_DELAY);
            } else if (v == (View) mBtnFullStart) {
                if (gpsTestRunning()) {
                    return;
                }
                mShowFirstFixLocate = true;
                enableBtns(false);
                Log.v(TAG, "Full Start button is pressed");
                extras.putBoolean(GPS_EXTRA_ALL, true);
                resetParam(extras, false, true);
                // enableBtns(true);
                mHandler.sendEmptyMessageDelayed(HANDLE_CLEAR, HANDLE_MSG_DELAY);
                // add by Baochu, 2011/07/8
            } else if (v == (View) mBtnReStart) {
                if (gpsTestRunning()) {
                    return;
                }
                enableBtns(false);
                Log.v(TAG, "Restart button is pressed");
                extras.putBoolean(GPS_EXTRA_EPHEMERIS, true);
                extras.putBoolean(GPS_EXTRA_A1LMANAC, true);
                extras.putBoolean(GPS_EXTRA_POSITION, true);
                extras.putBoolean(GPS_EXTRA_TIME, true);
                extras.putBoolean(GPS_EXTRA_IONO, true);
                extras.putBoolean(GPS_EXTRA_UTC, true);
                resetParam(extras, false, true);
                // enableBtns(true);
                mHandler.sendEmptyMessageDelayed(HANDLE_CLEAR, HANDLE_MSG_DELAY);
            } else if (v == (View) mBtnNmeaStart) {
                Log.d(TAG, "NMEA Start button is pressed");
                if (!createFileForSavingNMEALog()) {
                    Log.i(TAG, "createFileForSavingNMEALog return false");
                    return;
                }
                mStartNmeaRecord = true;
                mBtnNmeaStart.setEnabled(false);
                mBtnNMEAStop.setEnabled(true);
            } else if (v == (View) mBtnNMEAStop) {
                Log.d(TAG, "NMEA Stop button is pressed");
                mStartNmeaRecord = false;
                finishSavingNMEALog();

            } else if (v == (View) mBtnNMEADbgDbg) {
                Log.v(TAG, "NMEA DbgDbg is pressed");
                String ss = GpsMnlSetting.getMnlProp(
                        GpsMnlSetting.KEY_DEBUG_DBG2SOCKET,
                        GpsMnlSetting.PROP_VALUE_0);
                if (ss.equals(GpsMnlSetting.PROP_VALUE_0)) {
                    mBtnNMEADbgDbg
                            .setText(R.string.btn_name_dbg2socket_disable);
                    GpsMnlSetting.setMnlProp(
                            GpsMnlSetting.KEY_DEBUG_DBG2SOCKET,
                            GpsMnlSetting.PROP_VALUE_1);
                } else {
                    mBtnNMEADbgDbg.setText(R.string.btn_name_dbg2socket_enable);
                    GpsMnlSetting.setMnlProp(
                            GpsMnlSetting.KEY_DEBUG_DBG2SOCKET,
                            GpsMnlSetting.PROP_VALUE_0);
                }

            } else if (v == (View) mBtnNmeaDbgNmea) {
                Log.v(TAG, "NMEA DbgNmea button is pressed");
                String ss = GpsMnlSetting.getMnlProp(
                        GpsMnlSetting.KEY_DEBUG_NMEA2SOCKET,
                        GpsMnlSetting.PROP_VALUE_0);
                if (ss.equals(GpsMnlSetting.PROP_VALUE_0)) {
                    mBtnNmeaDbgNmea
                            .setText(R.string.btn_name_nmea2socket_disable);
                    GpsMnlSetting.setMnlProp(
                            GpsMnlSetting.KEY_DEBUG_NMEA2SOCKET,
                            GpsMnlSetting.PROP_VALUE_1);
                } else {
                    mBtnNmeaDbgNmea
                            .setText(R.string.btn_name_nmea2socket_enable);
                    GpsMnlSetting.setMnlProp(
                            GpsMnlSetting.KEY_DEBUG_NMEA2SOCKET,
                            GpsMnlSetting.PROP_VALUE_0);
                }
            } else if (v == (View) mBtnNmeaDbgDbgFile) {
                Log.v(TAG, "NMEA DbgDbgFile is pressed");
                String ss = GpsMnlSetting.getMnlProp(
                        GpsMnlSetting.KEY_DEBUG_DBG2FILE,
                        GpsMnlSetting.PROP_VALUE_0);
                if (ss.equals(GpsMnlSetting.PROP_VALUE_0)) {
                    mBtnNmeaDbgDbgFile
                            .setText(R.string.btn_name_dbg2file_disable);
                    GpsMnlSetting.setMnlProp(GpsMnlSetting.KEY_DEBUG_DBG2FILE,
                            GpsMnlSetting.PROP_VALUE_1);
                } else {
                    mBtnNmeaDbgDbgFile
                            .setText(R.string.btn_name_dbg2file_enable);
                    GpsMnlSetting.setMnlProp(GpsMnlSetting.KEY_DEBUG_DBG2FILE,
                            GpsMnlSetting.PROP_VALUE_0);
                }

            } else if (v == (View) mBtnNmeaDbgNmeaDdms) {
                Log.v(TAG, "NMEA debug2ddms button is pressed");
                String ss = GpsMnlSetting.getMnlProp(
                        GpsMnlSetting.KEY_DEBUG_DEBUG_NMEA,
                        GpsMnlSetting.PROP_VALUE_1);
                if (ss.equals(GpsMnlSetting.PROP_VALUE_1)) { // default enabled
                    mBtnNmeaDbgNmeaDdms
                            .setText(R.string.btn_name_dbg2ddms_enable);
                    GpsMnlSetting.setMnlProp(
                            GpsMnlSetting.KEY_DEBUG_DEBUG_NMEA,
                            GpsMnlSetting.PROP_VALUE_0);
                } else {
                    mBtnNmeaDbgNmeaDdms
                            .setText(R.string.btn_name_dbg2ddms_disable);
                    GpsMnlSetting.setMnlProp(
                            GpsMnlSetting.KEY_DEBUG_DEBUG_NMEA,
                            GpsMnlSetting.PROP_VALUE_1);
                }
            } else if (v == (View) mBtnHotStill) {
                Log.v(TAG, "Hot still button is pressed");
                String ss = GpsMnlSetting.getMnlProp(
                        GpsMnlSetting.KEY_BEE_ENABLED,
                        GpsMnlSetting.PROP_VALUE_1);
                if (ss.equals(GpsMnlSetting.PROP_VALUE_1)) {
                    mBtnHotStill.setText(R.string.btn_name_hotstill_enable);
                    GpsMnlSetting.setMnlProp(GpsMnlSetting.KEY_BEE_ENABLED,
                            GpsMnlSetting.PROP_VALUE_0);
                } else {
                    mBtnHotStill.setText(R.string.btn_name_hotstill_disable);
                    GpsMnlSetting.setMnlProp(GpsMnlSetting.KEY_BEE_ENABLED,
                            GpsMnlSetting.PROP_VALUE_1);
                }
            } else if (v == (View) mBtnSuplLog) {
                Log.v(TAG, "supllog button is pressed");
                String ss = GpsMnlSetting.getMnlProp(
                        GpsMnlSetting.KEY_SUPLLOG_ENABLED,
                        GpsMnlSetting.PROP_VALUE_0);
                if (ss.equals(GpsMnlSetting.PROP_VALUE_1)) {
                    mBtnSuplLog.setText(R.string.btn_name_supllog_enable);
                    GpsMnlSetting.setMnlProp(GpsMnlSetting.KEY_SUPLLOG_ENABLED,
                            GpsMnlSetting.PROP_VALUE_0);
                } else {
                    mBtnSuplLog.setText(R.string.btn_name_supllog_disable);
                    GpsMnlSetting.setMnlProp(GpsMnlSetting.KEY_SUPLLOG_ENABLED,
                            GpsMnlSetting.PROP_VALUE_1);
                }
            } else if (v == (View) mBtnNmeaClear) {
                Log.v(TAG, "NMEA Clear button is pressed");
                mTvNmeaLog.setText(R.string.empty);
            } else if (v == (View) mBtnNmeaSave) {
                Log.v(TAG, "NMEA Save button is pressed");
                saveNMEALog();
            } else if (v == (View) mBtnGpsHwTest) {
                Log.v(TAG, "mBtnGPSHwTest Button is pressed");
                onGpsHwTestClicked();
            } else if (v == (View) mBtnGpsJamming) {
                Log.v(TAG, "mBtnGPSJamming Button is pressed");
                onGpsJammingScanClicked();
                // add Radio
            } else if (v == (View) mBtnStart) {
                if (gpsTestRunning()) {
                    return;
                }
                mShowFirstFixLocate = true;
                enableBtns(false);
                Bundletemp = extras;
                StartGpsMode();
                if (mAverageCn0TextView != null) {
                    mAverageCn0TextView.setText("N/A");
                }
                resetParam(Bundletemp, false, true);
                Bundletemp = null;
                mHandler.sendEmptyMessageDelayed(HANDLE_CLEAR, HANDLE_MSG_DELAY);

            } else if (v == (View) mRadioBtnHot) {
                final SharedPreferences preferences = SgpsActivity.this
                        .getSharedPreferences(START_MODE,
                                android.content.Context.MODE_PRIVATE);
                preferences.edit().putInt(START_MODE, HOT_START).commit();
                Log.v(TAG, "mRadioBtnHot Button is pressed");
//                mTvRestartMode.setText(GetStartMode(HOT_START));
            } else if (v == (View) mRadioBtnCold) {
                final SharedPreferences preferences = SgpsActivity.this
                        .getSharedPreferences(START_MODE,
                                android.content.Context.MODE_PRIVATE);
                preferences.edit().putInt(START_MODE, COLD_START).commit();
                Log.v(TAG, "mRadioBtnCold Button is pressed");
//                mTvRestartMode.setText(GetStartMode(COLD_START));
            } else if (v == (View) mRadioBtnWarm) {
                final SharedPreferences preferences = SgpsActivity.this
                        .getSharedPreferences(START_MODE,
                                android.content.Context.MODE_PRIVATE);
                preferences.edit().putInt(START_MODE, WARM_START).commit();
                Log.v(TAG, "mRadioBtnWarm Button is pressed");
//                mTvRestartMode.setText(GetStartMode(WARM_START));
            } else if (v == (View) mRadioBtnFull) {
                final SharedPreferences preferences = SgpsActivity.this
                        .getSharedPreferences(START_MODE,
                                android.content.Context.MODE_PRIVATE);
                preferences.edit().putInt(START_MODE, FULL_START).commit();
                Log.v(TAG, "mRadioBtnFull Button is pressed");
//                mTvRestartMode.setText(GetStartMode(FULL_START));
            /*SPRD: fix bug 360943 change checkbox to radiobutton @*/
            } else if (v == (View) mRadioBtnAutoHot) {
                final SharedPreferences preferences = SgpsActivity.this
                        .getSharedPreferences(START_MODE_AUTO,
                                android.content.Context.MODE_PRIVATE);
                preferences.edit().putInt(START_MODE_AUTO, HOT_START).commit();
                Log.v(TAG, "mRadioBtnAutoHot Button is pressed");
//                mTvRestartMode.setText(GetStartMode(HOT_START));
            } else if (v == (View) mRadioBtnAutoCold) {
                final SharedPreferences preferences = SgpsActivity.this
                        .getSharedPreferences(START_MODE_AUTO,
                                android.content.Context.MODE_PRIVATE);
                preferences.edit().putInt(START_MODE_AUTO, COLD_START).commit();
                Log.v(TAG, "mRadioBtnAutoCold Button is pressed");
//                mTvRestartMode.setText(GetStartMode(COLD_START));
            } else if (v == (View) mRadioBtnAutoWarm) {
                final SharedPreferences preferences = SgpsActivity.this
                        .getSharedPreferences(START_MODE_AUTO,
                                android.content.Context.MODE_PRIVATE);
                preferences.edit().putInt(START_MODE_AUTO, WARM_START).commit();
                Log.v(TAG, "mRadioBtnAutoWarm Button is pressed");
//                mTvRestartMode.setText(GetStartMode(WARM_START));
            } else if (v == (View) mRadioBtnAutoFull) {
                final SharedPreferences preferences = SgpsActivity.this
                        .getSharedPreferences(START_MODE_AUTO,
                                android.content.Context.MODE_PRIVATE);
                preferences.edit().putInt(START_MODE_AUTO, FULL_START).commit();
                Log.v(TAG, "mRadioBtnAutoFull Button is pressed");
//                mTvRestartMode.setText(GetStartMode(FULL_START));
            /* @}*/
            } else if (v == (CheckBox) mTvUartLogSwitch) {
                setUartLogCheckBox();
            } else if (v == (View) mBtnAssert) {
                Log.d(TAG, "mBtnAssert");
                if (!GPS_CHIP_TYPE_IS_GE2) {
                    mBtnAssert.setEnabled(false);
                    mBtnAssert.setText(R.string.feature_not_support);
                }
                sendCommand("$PSPRD,00,5,1");
            }else {
                return;
            }
        }
    };

    private String toString(float[] array, int count) {
        StringBuilder strBuilder = new StringBuilder();
        strBuilder.append("(");
        for (int idx = 0; idx < count; idx++) {
            strBuilder.append(Float.toString(array[idx]));
            strBuilder.append(",");
        }
        strBuilder.append(")");
        return strBuilder.toString();
    }

    private String toString(int[] array, int count) {
        StringBuilder strBuilder = new StringBuilder();
        strBuilder.append("(");
        for (int idx = 0; idx < count; idx++) {
            strBuilder.append(Integer.toString(array[idx]));
            strBuilder.append(",");
        }
        strBuilder.append(")");
        return strBuilder.toString();
    }

    private void emptyArray() {
        mSatellites = 0;
        for (int i = 0; i < MAX_SATELLITES_NUMBER; i++) {
            mPrns[i] = 0;
            mSnrs[i] = 0;
            mElevation[i] = 0;
            mAzimuth[i] = 0;
            if (i < SatelliteDataProvider.SATELLITES_MASK_SIZE) {
                mUsedInFixMask[i] = 0;
            }
        }
    }

    private void viewUartLogCheckBox() {
        SharedPreferences preferences = this.getSharedPreferences(
                UART_LOG_SWITCH, android.content.Context.MODE_PRIVATE);
        boolean uart_log_switch = preferences
                .getBoolean(UART_LOG_SWITCH, false);
        if (mTvUartLogSwitch != null)
            mTvUartLogSwitch.setChecked(uart_log_switch);
        setUartLogCheckBox();
    }

    private void setUartLogCheckBox() {
        SharedPreferences preferences = this.getSharedPreferences(
                UART_LOG_SWITCH, android.content.Context.MODE_PRIVATE);
        Bundle extras = new Bundle();
        if (mTvUartLogSwitch != null && mTvUartLogSwitch.isChecked()) {
            extras.putBoolean(GPS_EXTRA_LOG_SWITCH_ON, true);
            extras.putBoolean(GPS_EXTRA_TIME, true);
            preferences.edit().putBoolean(UART_LOG_SWITCH, true).commit();
        } else {
            extras.putBoolean(GPS_EXTRA_TIME, true);
            extras.putBoolean(GPS_EXTRA_LOG_SWITCH_OFF, true);
            preferences.edit().putBoolean(UART_LOG_SWITCH, false).commit();
        }
        if (mLocationManager != null)
            mLocationManager.sendExtraCommand(LocationManager.GPS_PROVIDER,
                    "delete_aiding_data", extras);
    }

    private String GetStartMode(int index) {
        switch (index) {
            case COLD_START:
                return getString(R.string.cold);
            case WARM_START:
                return getString(R.string.warm);
            case FULL_START:
                return getString(R.string.full);
            case HOT_START:
            default:
                return getString(R.string.hot);
        }
    }

    /**
     * Component initial
     */
    private void setLayout() {
        showGPSVersion();
        mSatelliteView = (SatelliteSkyView) findViewById(R.id.sky_view);
        mSatelliteView.setDataProvider(this);
        mSignalView = (SatelliteSignalView) findViewById(R.id.signal_view);
        mSignalView.setDataProvider(this);
        mBtnColdStart = (Button) findViewById(R.id.btn_cold);
        mBtnColdStart.setOnClickListener(mBtnClickListener);
        mBtnWarmStart = (Button) findViewById(R.id.btn_warm);
        mBtnWarmStart.setOnClickListener(mBtnClickListener);
        mBtnHotStart = (Button) findViewById(R.id.btn_hot);
        mBtnHotStart.setOnClickListener(mBtnClickListener);
        mBtnFullStart = (Button) findViewById(R.id.btn_full);
        mBtnFullStart.setOnClickListener(mBtnClickListener);
        mBtnReStart = (Button) findViewById(R.id.btn_restart);
        mBtnReStart.setOnClickListener(mBtnClickListener);
        mBtnHotStill = (Button) findViewById(R.id.btn_hotstill);
        mBtnHotStill.setOnClickListener(mBtnClickListener);
        mBtnSuplLog = (Button) findViewById(R.id.btn_supllog);
        mBtnSuplLog.setOnClickListener(mBtnClickListener);
        mTvNmeaLog = (TextView) findViewById(R.id.tv_nmea_log);
        mTvNMEAHint = (TextView) findViewById(R.id.tv_nmea_hint);
        mBtnNmeaStart = (Button) findViewById(R.id.btn_nmea_start);
        mBtnNmeaStart.setOnClickListener(mBtnClickListener);
        mBtnNMEAStop = (Button) findViewById(R.id.btn_nmea_stop);
        mBtnNMEAStop.setOnClickListener(mBtnClickListener);
        mBtnNMEAStop.setEnabled(false);
        mBtnNMEADbgDbg = (Button) findViewById(R.id.btn_nmea_dbg_dbg);
        mBtnNMEADbgDbg.setOnClickListener(mBtnClickListener);
        mBtnNmeaDbgNmea = (Button) findViewById(R.id.btn_nmea_dbg_nmea);
        mBtnNmeaDbgNmea.setOnClickListener(mBtnClickListener);
        mBtnNmeaDbgDbgFile = (Button) findViewById(R.id.btn_nmea_dbg_dbg_file);
        mBtnNmeaDbgDbgFile.setOnClickListener(mBtnClickListener);
        mBtnNmeaDbgNmeaDdms = (Button) findViewById(R.id.btn_nmea_dbg_nmea_file);
        mBtnNmeaDbgNmeaDdms.setOnClickListener(mBtnClickListener);
        mBtnNmeaClear = (Button) findViewById(R.id.btn_nmea_clear);
        mBtnNmeaClear.setOnClickListener(mBtnClickListener);
        mBtnNmeaSave = (Button) findViewById(R.id.btn_nmea_save);
        mBtnNmeaSave.setOnClickListener(mBtnClickListener);
        mBtnGpsTestStart = (Button) findViewById(R.id.btn_gps_test_start);
        mBtnGpsTestStart.setOnClickListener(mBtnClickListener);
        mBtnGpsTestStop = (Button) findViewById(R.id.btn_gps_test_stop);
        mBtnGpsTestStop.setOnClickListener(mBtnClickListener);
        mBtnGpsTestSave = (Button) findViewById(R.id.btn_gps_test_save);
        mBtnGpsTestSave.setOnClickListener(mBtnClickListener);

        mBtnGpsTestAutoTransferStart = (Button) findViewById(R.id.btn_gps_test_start_01);
        mBtnGpsTestAutoTransferStart.setOnClickListener(mBtnClickListener);
        mBtnGpsTestAutoTransferStop = (Button) findViewById(R.id.btn_gps_test_stop_01);
        mBtnGpsTestAutoTransferStop.setOnClickListener(mBtnClickListener);
        mBtnGpsTestAutoTransferStop.setEnabled(false);
        mEtTestTimes_01 = (EditText) findViewById(R.id.et_gps_test_times_01);
        mEtTestInterval_01 = (EditText) findViewById(R.id.et_gps_test_interval_01);
        mEtTestTransferInterval = (EditText) findViewById(R.id.et_gps_test_transfer_interval);
        mEtTTFFTimeout = (EditText) findViewById(R.id.gps_ttff_timeout);
        mCurrentTimeHot = (TextView) findViewById(R.id.tv_current_times_01);
        mCurrentTimeWarm = (TextView) findViewById(R.id.tv_current_times_02);
        mCurrentTimeCold = (TextView) findViewById(R.id.tv_current_times_03);
        mCurrentTimeFactory = (TextView) findViewById(R.id.tv_current_times_04);
        mCountDownHot = (TextView) findViewById(R.id.tv_reconnect_countdown_01);
        mCountDownWarm = (TextView) findViewById(R.id.tv_reconnect_countdown_02);
        mCountDownCold = (TextView) findViewById(R.id.tv_reconnect_countdown_03);
        mCountDownFactory = (TextView) findViewById(R.id.tv_reconnect_countdown_04);
        mLastTTFFHot = (TextView) findViewById(R.id.tv_last_ttff_01);
        mLastTTFFWarm = (TextView) findViewById(R.id.tv_last_ttff_02);
        mLastTTFFCold = (TextView) findViewById(R.id.tv_last_ttff_03);
        mLastTTFFFactory = (TextView) findViewById(R.id.tv_last_ttff_04);
        mAverageHot = (TextView) findViewById(R.id.tv_mean_ttff_01);
        mAverageWarm = (TextView) findViewById(R.id.tv_mean_ttff_02);
        mAverageCold = (TextView) findViewById(R.id.tv_mean_ttff_03);
        mAverageFactory = (TextView) findViewById(R.id.tv_mean_ttff_04);

        mBtnGpsTestStop.setEnabled(false);
        mEtTestTimes = (EditText) findViewById(R.id.et_gps_test_times);
        mCbNeed3DFix = (CheckBox) findViewById(R.id.cb_need_3d_fix);
        mTvUartLogSwitch = (CheckBox) findViewById(R.id.tv_uart_log_switch);
        mTvUartLogSwitch.setOnClickListener(mBtnClickListener);
        mEtTestInterval = (EditText) findViewById(R.id.et_gps_test_interval);
        mBtnGpsHwTest = (Button) findViewById(R.id.btn_gps_hw_test);
        mBtnGpsHwTest.setOnClickListener(mBtnClickListener);
        mBtnGpsJamming = (Button) findViewById(R.id.btn_gps_jamming_scan);
        mBtnGpsJamming.setOnClickListener(mBtnClickListener);
        mEtGpsJammingTimes = (EditText) findViewById(R.id.et_gps_jamming_times);
        mEtGpsJammingTimes.setText(getString(R.string.jamming_scan_times));
        mEtGpsJammingTimes.setSelection(mEtGpsJammingTimes.getText().length());
        mChkAutoTestHot = (CheckBox) findViewById(R.id.check_hot);
        mChkAutoTestCold = (CheckBox) findViewById(R.id.check_cold);
        mChkAutoTestWarm = (CheckBox) findViewById(R.id.check_warm);
        mChkAutoTestFull = (CheckBox) findViewById(R.id.check_full);

        mCircAutoTestHot = (CheckBox) findViewById(R.id.check_auto_hot);
        mCircAutoTestWarm = (CheckBox) findViewById(R.id.check_auto_warm);
        mCircAutoTestCold = (CheckBox) findViewById(R.id.check_auto_cold);
        mCircAutoTestFull = (CheckBox) findViewById(R.id.check_auto_full);

        mHotSuccessRate = (TextView) findViewById(R.id.success_rate_hot);
        mWarmSuccessRate = (TextView) findViewById(R.id.success_rate_warm);
        mColdSuccessRate = (TextView) findViewById(R.id.success_rate_cold);
        mFullSuccessRate = (TextView) findViewById(R.id.success_rate_full);

        mScrollLog = (ScrollView) findViewById(R.id.tv_scroll_log);

        mTvResultLog = (TextView) findViewById(R.id.tv_result_log);
        mTvResultHint = (TextView) findViewById(R.id.tv_result_hint);
        mEtTestLatitude = (EditText) findViewById(R.id.et_gps_test_latitude);
        mEtTestLongitude = (EditText) findViewById(R.id.et_gps_test_longitude);

        // sgps version
        mTvSGPSVersion = (TextView) findViewById(R.id.tv_sgps_version);
        // SPRD:add ScrollingBar
        mTvSGPSVersion.setMovementMethod(ScrollingMovementMethod.getInstance());
        mTvSGPSVersion.setText(SGPS_VRESION);

        mScrollResult = (ScrollView) findViewById(R.id.tv_scroll_result);
        mTvResultLog.setMovementMethod(ScrollingMovementMethod.getInstance());
        mTvResultLog.setScrollbarFadingEnabled(false);

        mBtnNMEADbgDbg.setVisibility(View.GONE);
        mBtnNmeaDbgNmea.setVisibility(View.GONE);
        mBtnNmeaSave.setVisibility(View.GONE);
        mBtnNmeaClear.setVisibility(View.GONE);

        // log view
        mBtnGpsJamming.setVisibility(View.GONE);
        mEtGpsJammingTimes.setVisibility(View.GONE);
        mBtnGpsHwTest.setVisibility(View.GONE);
        mBtnNmeaDbgDbgFile.setVisibility(View.GONE);
        mBtnNmeaDbgNmeaDdms.setVisibility(View.GONE);
        mCbNeed3DFix.setVisibility(View.GONE);

       // mTvRestartMode = (TextView) findViewById(R.id.tv_restart_mode);

        // change button to radiogroup
        mBtnHotStart.setVisibility(View.GONE);
        mBtnColdStart.setVisibility(View.GONE);
        mBtnWarmStart.setVisibility(View.GONE);
        mBtnFullStart.setVisibility(View.GONE);
        mBtnSuplLog.setVisibility(View.GONE);
        mBtnHotStill.setVisibility(View.GONE);
        mBtnReStart.setVisibility(View.GONE);

        mRadioBtnHot = (RadioButton) findViewById(R.id.radio_hot);
        mRadioBtnCold = (RadioButton) findViewById(R.id.radio_cold);
        mRadioBtnWarm = (RadioButton) findViewById(R.id.radio_warm);
        mRadioBtnFull = (RadioButton) findViewById(R.id.radio_full);

        mBtnStart = (Button) findViewById(R.id.btn_start);
        mBtnAssert = (Button) findViewById(R.id.btn_assert);

        SharedPreferences preferences = this.getSharedPreferences(START_MODE,
                android.content.Context.MODE_PRIVATE);

        preferences.edit().putInt(START_MODE, COLD_START).commit();// cold start
                                                                   // in
                                                                   // default.

        int start_mode = preferences.getInt(START_MODE, HOT_START);
        mRadioBtnHot.setChecked(start_mode == HOT_START ? true : false);
        mRadioBtnCold.setChecked(start_mode == COLD_START ? true : false);
        mRadioBtnWarm.setChecked(start_mode == WARM_START ? true : false);
        mRadioBtnFull.setChecked(start_mode == FULL_START ? true : false);
//        mTvRestartMode.setText(GetStartMode(start_mode));

        /*SPRD: fix bug 360943 change checkbox to radiobutton @*/
        mRadioBtnAutoHot = (RadioButton) findViewById(R.id.auto_radio_hot);
        mRadioBtnAutoWarm = (RadioButton) findViewById(R.id.auto_radio_warm);
        mRadioBtnAutoCold = (RadioButton) findViewById(R.id.auto_radio_cold);
        mRadioBtnAutoFull = (RadioButton) findViewById(R.id.auto_radio_full);

        SharedPreferences preferencesAuto = this.getSharedPreferences(START_MODE_AUTO,
                android.content.Context.MODE_PRIVATE);
        preferencesAuto.edit().putInt(START_MODE_AUTO, COLD_START).commit();

        int start_mode_auto = preferencesAuto.getInt(START_MODE_AUTO, HOT_START);
        mRadioBtnAutoHot.setChecked(start_mode_auto == HOT_START ? true : false);
        mRadioBtnAutoWarm.setChecked(start_mode_auto == WARM_START ? true : false);
        mRadioBtnAutoCold.setChecked(start_mode_auto == COLD_START ? true : false);
        mRadioBtnAutoFull.setChecked(start_mode_auto == FULL_START ? true : false);

        mRadioBtnAutoHot.setOnClickListener(mBtnClickListener);
        mRadioBtnAutoWarm.setOnClickListener(mBtnClickListener);
        mRadioBtnAutoCold.setOnClickListener(mBtnClickListener);
        mRadioBtnAutoFull.setOnClickListener(mBtnClickListener);

        mSpreadOrBitSwitch = (Switch) findViewById(R.id.spreadorbit_switch);

        mRealEPHSwitch = (Switch) findViewById(R.id.real_eph_switch);
        if ("TRUE".equals(getConfigration(SgpsUtils.GPS_CSR_CONFIG_FIL_FOR_GE2, "REALEPH-ENABLE"))) {
            mRealEPHSwitch.setChecked(true);
        } else {
            mRealEPHSwitch.setChecked(false);
        }
        mRealEPHSwitch.setOnCheckedChangeListener(mSwitchCheckedChangeListener);

        mGNSSLogSwitch = (Switch) findViewById(R.id.gnss_log_switch);
        if ("TRUE".equals(getConfigration(SgpsUtils.GPS_CSR_CONFIG_FIL_FOR_GE2, "LOG-ENABLE"))) {
            mGNSSLogSwitch.setChecked(true);
        } else {
            mGNSSLogSwitch.setChecked(false);
        }
        mGNSSLogSwitch.setOnCheckedChangeListener(mSwitchCheckedChangeListener);
        /* SPRD:add for GE2 @{ */
        mRadioBtnGps = (RadioButton) findViewById(R.id.radio_gps);
        mRadioBtnGlonass =  (RadioButton) findViewById(R.id.radio_glonass);
        mRadioBtnBDS =  (RadioButton) findViewById(R.id.radio_bsd);
        mRadioBtnGlonassGps =  (RadioButton) findViewById(R.id.radio_glogps);
        mRadioBtnGpsBDS =  (RadioButton) findViewById(R.id.radio_gpsbds);
        // add new feature for gps download bin by ansel.li
        mRadiobtnGpsBeidou = (RadioButton)findViewById(R.id.radio_gps_beidou);
        mRadiobtnGpsGlonass = (RadioButton)findViewById(R.id.radio_gps_glonass);
        if(!GPS_CHIP_TYPE_IS_GE2){
            mRadiobtnGpsBeidou.setVisibility(View.GONE);
            mRadiobtnGpsGlonass.setVisibility(View.GONE);
            mRadioBtnGlonass.setEnabled(false);
            mRadioBtnGlonassGps.setEnabled(false);
            mRadioBtnBDS.setEnabled(false);
            mRadioBtnGpsBDS.setEnabled(false);
        }
        Log.d(TAG, "GPS-IMG-MODE is " + getConfigration(SgpsUtils.GPS_CSR_CONFIG_FIL_FOR_GE2, "GPS-IMG-MODE"));
        if ("GNSSBDMODEM".equals(getConfigration(SgpsUtils.GPS_CSR_CONFIG_FIL_FOR_GE2, "GPS-IMG-MODE"))) {
            mRadiobtnGpsBeidou.setChecked(true);
            mRadioBtnGlonass.setEnabled(false);
            mRadioBtnGlonassGps.setEnabled(false);
        } else if("GNSSMODEM".equals(getConfigration(SgpsUtils.GPS_CSR_CONFIG_FIL_FOR_GE2, "GPS-IMG-MODE"))){
            mRadiobtnGpsGlonass.setChecked(true);
            mRadioBtnBDS.setEnabled(false);
            mRadioBtnGpsBDS.setEnabled(false);
        } else{
            if(SgpsUtils.getGPSVersionInfo().trim().contains("GPS_GLO")){
                Log.d(TAG,"Running for GPS_GLO that is not native");
                mRadiobtnGpsGlonass.setChecked(true);
                mRadioBtnBDS.setEnabled(false);
                mRadioBtnGpsBDS.setEnabled(false);
            }else if(SgpsUtils.getGPSVersionInfo().trim().contains("GPS_BDS")){
                Log.d(TAG,"Running for GPS_BDS that is not native");
                mRadiobtnGpsBeidou.setChecked(true);
                mRadioBtnGlonass.setEnabled(false);
                mRadioBtnGlonassGps.setEnabled(false);
            }
        }
        mRadiobtnGpsBeidou.setOnClickListener(mBtnGpsImgModeClickListener);
        mRadiobtnGpsGlonass.setOnClickListener(mBtnGpsImgModeClickListener);
        int gnssMode = getGNSSMode();
        Log.d(TAG, "gnssMode is " + gnssMode);
        mRadioBtnGps.setChecked(gnssMode == GPS_ONLY ? true : false);
        mRadioBtnGlonass.setChecked(gnssMode == GLONASS_ONLY ? true : false);
        mRadioBtnBDS.setChecked(gnssMode == BDS_ONLY ? true : false);
        mRadioBtnGlonassGps.setChecked(gnssMode == GLONASS_GPS ? true : false);
        mRadioBtnGpsBDS.setChecked(gnssMode == GPS_BDS ? true : false);
        mRadioBtnGps.setOnClickListener(mBtnGpsClickListener);
        mRadioBtnGlonass.setOnClickListener(mBtnGpsClickListener);
        mRadioBtnBDS.setOnClickListener(mBtnGpsClickListener);
        mRadioBtnGlonassGps.setOnClickListener(mBtnGpsClickListener);
        mRadioBtnGpsBDS.setOnClickListener(mBtnGpsClickListener);
       /* @} */
        mChkAutoTestHot.setVisibility(View.GONE);
        mChkAutoTestCold.setVisibility(View.GONE);
        mChkAutoTestWarm.setVisibility(View.GONE);
        mChkAutoTestFull.setVisibility(View.GONE);
        mBtnGpsTestStart.setVisibility(View.GONE);
        mBtnGpsTestStop.setVisibility(View.GONE);
        mEtTestTimes.setVisibility(View.GONE);
        mEtTestInterval.setVisibility(View.GONE);
        mRadioBtnAutoHot.setVisibility(View.GONE);
        mRadioBtnAutoWarm.setVisibility(View.GONE);
        mRadioBtnAutoCold.setVisibility(View.GONE);
        mRadioBtnAutoFull.setVisibility(View.GONE);
        /* @}*/

        mBtnStart.setOnClickListener(mBtnClickListener);
        mBtnAssert.setOnClickListener(mBtnClickListener);
        mRadioBtnHot.setOnClickListener(mBtnClickListener);
        mRadioBtnCold.setOnClickListener(mBtnClickListener);
        mRadioBtnWarm.setOnClickListener(mBtnClickListener);
        mRadioBtnFull.setOnClickListener(mBtnClickListener);

        mTvNmeaLog.setMovementMethod(new ScrollingMovementMethod());
        mTvNmeaLog.setScrollbarFadingEnabled(true);
        mCircAutoTestHot.setChecked(true);
        mCircAutoTestCold.setChecked(true);
        mCircAutoTestWarm.setChecked(true);
        mCircAutoTestFull.setChecked(true);
        // mTvNmeaLog.setMaxLines(250);//max 250 lines
        viewUartLogCheckBox();

        mAverageCn0TextView = (TextView) findViewById(R.id.average_cn0);
        mAverageCn0TextView.setText("N/A");
        ((TextView) findViewById(R.id.tv_distance)).setVisibility(View.GONE);
        ((TextView) findViewById(R.id.tv_distance_left))
                .setVisibility(View.GONE);

        mSingleSatelliteSwitch = (Switch) findViewById(R.id.single_satellite_switch);
        if ("TRUE".equals(getConfigration(mGpsConfigFile, "SINGLE-SATELLITE"))) {
            mSingleSatelliteSwitch.setChecked(true);
        } else {
            mSingleSatelliteSwitch.setChecked(false);
        }
        mSingleSatelliteSwitch.setOnCheckedChangeListener(mSwitchCheckedChangeListener);
        // Update buttons status
        String ss = GpsMnlSetting.getMnlProp(
                GpsMnlSetting.KEY_DEBUG_DBG2SOCKET, GpsMnlSetting.PROP_VALUE_0);
        if (ss.equals(GpsMnlSetting.PROP_VALUE_0)) {
            mBtnNMEADbgDbg.setText(R.string.btn_name_dbg2socket_enable);
        } else {
            mBtnNMEADbgDbg.setText(R.string.btn_name_dbg2socket_disable);
        }
        ss = GpsMnlSetting.getMnlProp(GpsMnlSetting.KEY_DEBUG_NMEA2SOCKET,
                GpsMnlSetting.PROP_VALUE_0);
        if (ss.equals(GpsMnlSetting.PROP_VALUE_0)) {
            mBtnNmeaDbgNmea.setText(R.string.btn_name_nmea2socket_enable);
        } else {
            mBtnNmeaDbgNmea.setText((R.string.btn_name_nmea2socket_disable));
        }
        ss = GpsMnlSetting.getMnlProp(GpsMnlSetting.KEY_DEBUG_DBG2FILE,
                GpsMnlSetting.PROP_VALUE_0);
        if (ss.equals(GpsMnlSetting.PROP_VALUE_0)) {
            mBtnNmeaDbgDbgFile.setText((R.string.btn_name_dbg2file_enable));
        } else {
            mBtnNmeaDbgDbgFile.setText((R.string.btn_name_dbg2file_disable));
        }
        ss = GpsMnlSetting.getMnlProp(GpsMnlSetting.KEY_DEBUG_DEBUG_NMEA,
                GpsMnlSetting.PROP_VALUE_1);
        if (ss.equals(GpsMnlSetting.PROP_VALUE_1)) {
            mBtnNmeaDbgNmeaDdms.setText((R.string.btn_name_dbg2ddms_disable));
        } else {
            mBtnNmeaDbgNmeaDdms.setText((R.string.btn_name_dbg2ddms_enable));
        }
        ss = GpsMnlSetting.getMnlProp(GpsMnlSetting.KEY_BEE_ENABLED,
                GpsMnlSetting.PROP_VALUE_1);
        if (ss.equals(GpsMnlSetting.PROP_VALUE_1)) {
            mBtnHotStill.setText((R.string.btn_name_hotstill_disable));
        } else {
            mBtnHotStill.setText((R.string.btn_name_hotstill_enable));
        }
        ss = GpsMnlSetting.getMnlProp(GpsMnlSetting.KEY_SUPLLOG_ENABLED,
                GpsMnlSetting.PROP_VALUE_0);
        if (ss.equals(GpsMnlSetting.PROP_VALUE_1)) {
            mBtnSuplLog.setText((R.string.btn_name_supllog_disable));
        } else {
            mBtnSuplLog.setText((R.string.btn_name_supllog_enable));
        }
        boolean bClearHwTest = false;
        preferences = this.getSharedPreferences(FIRST_TIME,
                android.content.Context.MODE_PRIVATE);
        ss = preferences.getString(FIRST_TIME, null);
        if (ss != null) {
            if (ss.equals(GpsMnlSetting.PROP_VALUE_1)) {
                preferences.edit()
                        .putString(FIRST_TIME, GpsMnlSetting.PROP_VALUE_2)
                        .commit();
            } else if (ss.equals(GpsMnlSetting.PROP_VALUE_2)) {
                bClearHwTest = true;
            }
        }
        ss = GpsMnlSetting.getMnlProp(GpsMnlSetting.KEY_TEST_MODE,
                GpsMnlSetting.PROP_VALUE_0);
        if (ss.equals(GpsMnlSetting.PROP_VALUE_0)) {
            mBtnGpsHwTest.setText((R.string.btn_name_dbg2gpsdoctor_enable));
        } else {
            if (bClearHwTest) {
                GpsMnlSetting.setMnlProp(GpsMnlSetting.KEY_TEST_MODE,
                        GpsMnlSetting.PROP_VALUE_0);
                mBtnGpsHwTest.setText((R.string.btn_name_dbg2gpsdoctor_enable));
            } else {
                mBtnGpsHwTest
                        .setText((R.string.btn_name_dbg2gpsdoctor_disable));
            }
        }
    }

    private Runnable mScrollToBottom = new Runnable() {
        @Override
        public void run() {
            int off = mTvNmeaLog.getMeasuredHeight()
                    - mScrollLog.getMeasuredHeight();
            if (off > viewpages * mScrollLog.getMeasuredHeight()) {
                int scroll_y = mScrollLog.getScrollY();
                int line_count_src = mTvNmeaLog.getLineCount();
                int line_count_des = 0;
                int line_height = mTvNmeaLog.getLineHeight();
                int offset_x = 0;
                int lens_pre_page = mScrollLog.getMeasuredHeight()
                        / line_height;

                int remove_line = line_count_src - viewpages * lens_pre_page;
                if (remove_line > 0) {
                    for (int i = 0; i < remove_line; i++) {
                        offset_x = mTvNmeaLog.getText().toString()
                                .indexOf("$", offset_x + 10);// find
                                                             // the
                                                             // second
                                                             // '$'
                    }

                    if (offset_x < 0)
                        offset_x = 10;// for exception
                    mTvNmeaLog.setText(mTvNmeaLog.getText().toString()
                            .substring(offset_x));// substring

                    line_count_des = mTvNmeaLog.getLineCount();

                    Log.d(TAG, "offset = " + offset_x + " line_count_src : "
                            + line_count_src + " line_count_des : "
                            + line_count_des);

                    scroll_y -= (line_count_src - line_count_des) * line_height;
                    if (scroll_y > 0) {
                        mScrollLog.scrollTo(0, scroll_y);// scroll to next line
                    }
                }
            }
            if (off > 0
                    && (mTvNmeaLog.getMeasuredHeight()
                            - mScrollLog.getScrollY() < 2 * mScrollLog
                            .getMeasuredHeight())) {
                mScrollLog.scrollTo(0, off);
            }
        }
    };

    /**
     * Clear location information
     */
    private void clearLayout() {
        // clear all information in layout
        ((TextView) findViewById(R.id.tv_date)).setText(R.string.empty);
        ((TextView) findViewById(R.id.tv_time)).setText(R.string.empty);
        ((TextView) findViewById(R.id.tv_latitude)).setText(R.string.empty);
        ((TextView) findViewById(R.id.tv_longitude)).setText(R.string.empty);
        ((TextView) findViewById(R.id.tv_altitude)).setText(R.string.empty);
        ((TextView) findViewById(R.id.tv_accuracy)).setText(R.string.empty);
        ((TextView) findViewById(R.id.tv_bearing)).setText(R.string.empty);
        ((TextView) findViewById(R.id.tv_speed)).setText(R.string.empty);
        ((TextView) findViewById(R.id.tv_distance)).setText(R.string.empty);
        // ((TextView) findViewById(R.id.tv_provider)).setText(R.string.empty);
        // ((TextView) findViewById(R.id.tv_status)).setText(R.string.empty);
        if (mShowFirstFixLocate) {
            ((TextView) findViewById(R.id.first_longtitude_text))
                    .setText(R.string.empty);
            ((TextView) findViewById(R.id.first_latitude_text))
                    .setText(R.string.empty);
        }
    }

    /**
     * Create file to record nmea log
     * 
     * @return True if create file success, or false
     */
    private boolean createFileForSavingNMEALog() {
        Log.d(TAG, "Enter startSavingNMEALog function");
        if (NMEALOG_SD) {
            if (!(Environment.getExternalStorageState()
                    .equals(Environment.MEDIA_MOUNTED))) {
                Log.v(TAG, "saveNMEALog function: No SD card");
                Toast.makeText(this, R.string.no_sdcard, Toast.LENGTH_LONG)
                        .show();
                return false;
            }
        }

        String strTime = DATE_FORMAT
                .format(new Date(System.currentTimeMillis()));
        File file = null;
        if (NMEALOG_SD) {
            file = new File(Environment.getExternalStorageDirectory(),
                    NMEA_LOG_PREX + strTime + NMEA_LOG_SUFX);
        } else {
            File nmeaPath = new File(NMEALOG_PATH);
            if (!nmeaPath.exists()) {
                nmeaPath.mkdirs();
            }
            if(getmIsAutoTransferTestRunning()){
                file = new File(nmeaPath, NMEA_LOG_PREX + getCurrentMode() + strTime + NMEA_LOG_SUFX);
            } else {
                file = new File(nmeaPath, NMEA_LOG_PREX + strTime + NMEA_LOG_SUFX);
            }
        }
        if (file != null) {
            try {
                file.createNewFile();
            } catch (IOException e) {
                Log.w(TAG, "create new file failed!");
                Toast.makeText(this, R.string.toast_create_file_failed,
                        Toast.LENGTH_LONG).show();
                return false;
            }
        }
        try {
            mOutputNMEALog = new FileOutputStream(file);
        } catch (FileNotFoundException e1) {
            Log.w(TAG,
                    "output stream FileNotFoundException: " + e1.getMessage());
            return false;
        }
        // set nmea hint
        if (!getmIsAutoTransferTestRunning()){
            mTvNMEAHint.setText((R.string.nmea_hint));
        }
        return true;
    }

    private void saveNMEALog(String nmea) {
        boolean bSaved = true;
        try {
            mOutputNMEALog.write(nmea.getBytes(), 0, nmea.getBytes().length);
            mOutputNMEALog.flush();
        } catch (IOException e) {
            bSaved = false;
            Log.d(TAG, "write NMEA log to file failed!");
        } finally {
            if (!bSaved) {
                finishSavingNMEALog();
                Toast.makeText(this, "Please check your SD card",
                        Toast.LENGTH_LONG).show();
            }
        }

    }

    /**
     * Finish record nmea log
     */
    private void finishSavingNMEALog() {
        try {
            mStartNmeaRecord = false;
            mBtnNMEAStop.setEnabled(false);
            mBtnNmeaStart.setEnabled(true);

            mTvNMEAHint.setText(R.string.empty);
            mTvNmeaLog.setText(R.string.empty);

            mOutputNMEALog.close();
            mOutputNMEALog = null;
            Toast.makeText(
                    this,
                    String.format(getString(R.string.toast_nmealog_save_at),
                            NMEALOG_SD ? Environment
                                    .getExternalStorageDirectory()
                                    .getAbsolutePath() : NMEALOG_PATH),
                    Toast.LENGTH_LONG).show();
        } catch (IOException e) {
            Log.w(TAG, "Close file failed!");
        }
    }

    /**
     * Save NMEA log to file
     */
    private void saveNMEALog() {
        if (NMEALOG_SD) {
            if (Environment.getExternalStorageState().equals(
                    Environment.MEDIA_MOUNTED)) {
                String strTime = DATE_FORMAT.format(new Date(System
                        .currentTimeMillis()));
                File file = new File(Environment.getExternalStorageDirectory(),
                        NMEA_LOG_PREX + strTime + NMEA_LOG_SUFX);
                FileOutputStream fileOutputStream = null;
                boolean flag = true;
                try {
                    if (!file.createNewFile()) {
                        Toast.makeText(this, R.string.toast_create_file_failed,
                                Toast.LENGTH_LONG).show();
                        return;
                    }
                    fileOutputStream = new FileOutputStream(file);
                    String nmea = ((TextView) findViewById(R.id.tv_nmea_log))
                            .getText().toString();
                    if (0 == nmea.getBytes().length) {
                        Toast.makeText(this, R.string.toast_no_log,
                                Toast.LENGTH_LONG).show();
                        return;
                    }
                    fileOutputStream.write(nmea.getBytes(), 0,
                            nmea.getBytes().length);
                    fileOutputStream.flush();
                    fileOutputStream.close();
                } catch (NotFoundException e) {
                    Log.d(TAG,
                            "Save nmealog NotFoundException: " + e.getMessage());
                    flag = false;
                } catch (IOException e) {
                    Log.d(TAG, "Save nmealog IOException: " + e.getMessage());
                    flag = false;
                } finally {
                    if (null != fileOutputStream) {
                        try {
                            fileOutputStream.close();
                        } catch (IOException e) {
                            Log.d(TAG, "Save nmealog exception in finally: "
                                    + e.getMessage());
                            flag = false;
                        }
                    }
                }
                if (flag) {
                    Log.d(TAG, "Save Nmealog to file Finished");
                    Toast.makeText(
                            this,
                            String.format(
                                    getString(R.string.toast_save_log_succeed_to),
                                    Environment.getExternalStorageDirectory()
                                            .getAbsolutePath()),
                            Toast.LENGTH_LONG).show();
                } else {
                    Log.w(TAG, "Save Nmealog Failed");
                    Toast.makeText(this, R.string.toast_save_log_failed,
                            Toast.LENGTH_LONG).show();
                }
            } else {
                Log.d(TAG, "saveNMEALog function: No SD card");
                Toast.makeText(this, (R.string.no_sdcard), Toast.LENGTH_LONG)
                        .show();
            }
        } else {
            String strTime = DATE_FORMAT.format(new Date(System
                    .currentTimeMillis()));
            File nmeaPath = new File(NMEALOG_PATH);
            if (!nmeaPath.exists()) {
                nmeaPath.mkdirs();
            }
            File file = new File(nmeaPath, NMEA_LOG_PREX + strTime
                    + NMEA_LOG_SUFX);
            if (file != null) {
                FileOutputStream outs = null;
                boolean flag = true;
                try {
                    file.createNewFile();
                    outs = new FileOutputStream(file);
                    String nmea = ((TextView) findViewById(R.id.tv_nmea_log))
                            .getText().toString();
                    if (0 == nmea.getBytes().length) {
                        Toast.makeText(this, R.string.toast_no_log,
                                Toast.LENGTH_LONG).show();
                        return;
                    }
                    outs.write(nmea.getBytes(), 0, nmea.getBytes().length);
                    outs.flush();
                } catch (IOException e) {
                    Log.d(TAG, "Save nmealog IOException: " + e.getMessage());
                    flag = false;
                } finally {
                    if (null != outs) {
                        try {
                            outs.close();
                        } catch (IOException e) {
                            Log.d(TAG, "Save nmealog exception in finally: "
                                    + e.getMessage());
                            flag = false;
                        }
                    }
                }
                if (flag) {
                    Log.d(TAG, "Save Nmealog to file Finished");
                    Toast.makeText(
                            this,
                            String.format(
                                    getString(R.string.toast_save_log_succeed_to),
                                    NMEALOG_PATH), Toast.LENGTH_LONG).show();
                } else {
                    Log.w(TAG, "Save NmeaLog failed!");
                    Toast.makeText(this, R.string.toast_save_log_failed,
                            Toast.LENGTH_LONG).show();
                }
            }
        }

    }

    private void enableSatrtModeBtn() {
        mRadioBtnHot.setEnabled(true);
        mRadioBtnCold.setEnabled(true);
        mRadioBtnWarm.setEnabled(true);
        mRadioBtnFull.setEnabled(true);
        mBtnStart.setEnabled(true);
        mBtnAssert.setEnabled(true);
        /* SPRD:add for GE2 @{ */
        mRadioBtnGps.setEnabled(true);
        mRadioBtnGlonass.setEnabled(true);
        mRadioBtnBDS.setEnabled(true);
        mRadioBtnGlonassGps.setEnabled(true);
        mRadioBtnGpsBDS.setEnabled(true);
        /* @} */
    }

    private void disableStartModeBtn() {
        mRadioBtnHot.setEnabled(false);
        mRadioBtnCold.setEnabled(false);
        mRadioBtnWarm.setEnabled(false);
        mRadioBtnFull.setEnabled(false);
        mBtnStart.setEnabled(false);
        mBtnAssert.setEnabled(false);
        /* SPRD:add for GE2 @{ */
        mRadioBtnGps.setEnabled(false);
        mRadioBtnGlonass.setEnabled(false);
        mRadioBtnBDS.setEnabled(false);
        mRadioBtnGlonassGps.setEnabled(false);
        mRadioBtnGpsBDS.setEnabled(false);
        /* @} */
    }

    // when start button is pressed, views excepts mBtnGPSTestStop must be
    // disabled

    private void setViewToAutoTransferStartState() {
        mBtnGpsTestAutoTransferStart.setFocusableInTouchMode(false);
        mBtnGpsTestAutoTransferStart.refreshDrawableState();
        mBtnGpsTestAutoTransferStart.setEnabled(false);
        mBtnGpsTestAutoTransferStop.setEnabled(true);
        mBtnGpsTestAutoTransferStop.refreshDrawableState();
        if (null != mEtTestTimes_01) {
            mEtTestTimes_01.setFocusable(false);
            mEtTestTimes_01.refreshDrawableState();
            mEtTestTimes_01.setEnabled(false);
        }
        if (null != mEtTestInterval_01) {
            mEtTestInterval_01.setFocusable(false);
            mEtTestInterval_01.refreshDrawableState();
            mEtTestInterval_01.setEnabled(false);
        }
        if (null != mEtTestTransferInterval) {
            mEtTestTransferInterval.setFocusable(false);
            mEtTestTransferInterval.refreshDrawableState();
            mEtTestTransferInterval.setEnabled(false);
        }
        if (null != mEtTTFFTimeout) {
            mEtTTFFTimeout.setFocusable(false);
            mEtTTFFTimeout.refreshDrawableState();
            mEtTTFFTimeout.setEnabled(false);
        }
        if (null != mCircAutoTestHot) {
            mCircAutoTestHot.setEnabled(false);
        }
        if (null != mCircAutoTestCold) {
            mCircAutoTestCold.setEnabled(false);
        }
        if (null != mCircAutoTestWarm) {
            mCircAutoTestWarm.setEnabled(false);
        }
        if (null != mCircAutoTestFull) {
            mCircAutoTestFull.setEnabled(false);
        }
        if (null != msgpsSwitch) {
            msgpsSwitch.setEnabled(false);
        }
        if (null != mGNSSLogSwitch) {
            mGNSSLogSwitch.setEnabled(false);
        }
    }

    private void setViewToAutoTransferStopState() {
        //mBtnGpsTestAutoTransferStart.setFocusableInTouchMode(true);
        mBtnGpsTestAutoTransferStart.setEnabled(true);
        mBtnGpsTestAutoTransferStart.refreshDrawableState();
        mBtnGpsTestAutoTransferStop.setEnabled(false);
        mBtnGpsTestAutoTransferStop.refreshDrawableState();
        if (null != mEtTestTimes_01) {
            mEtTestTimes_01.setEnabled(true);
            mEtTestTimes_01.setFocusableInTouchMode(true);
            mEtTestTimes_01.refreshDrawableState();
        }
        if (null != mEtTestInterval_01) {
            mEtTestInterval_01.setEnabled(true);
            mEtTestInterval_01.setFocusableInTouchMode(true);
            mEtTestInterval_01.refreshDrawableState();
        }
        if (null != mEtTestTransferInterval) {
            mEtTestTransferInterval.setEnabled(true);
            mEtTestTransferInterval.setFocusableInTouchMode(true);
            mEtTestTransferInterval.refreshDrawableState();
        }
        if (null != mEtTTFFTimeout) {
            mEtTTFFTimeout.setEnabled(true);
            mEtTTFFTimeout.setFocusableInTouchMode(true);
            mEtTTFFTimeout.refreshDrawableState();
        }
        if (null != mCircAutoTestHot) {
            mCircAutoTestHot.setEnabled(true);
        }
        if (null != mCircAutoTestCold) {
            mCircAutoTestCold.setEnabled(true);
        }
        if (null != mCircAutoTestWarm) {
            mCircAutoTestWarm.setEnabled(true);
        }
        if (null != mCircAutoTestFull) {
            mCircAutoTestFull.setEnabled(true);
        }
        if (null != mSpreadOrBitSwitch) {
            mSpreadOrBitSwitch.setEnabled(true);
        }
        if (null != mRealEPHSwitch) {
            mRealEPHSwitch.setEnabled(true);
        }
        if (null != msgpsSwitch) {
            msgpsSwitch.setEnabled(true);
        }
        if (null != mGNSSLogSwitch) {
            mGNSSLogSwitch.setEnabled(true);
        }
    }

    private void setViewToStartState() {
        mBtnGpsTestStart.setFocusableInTouchMode(false);
        mBtnGpsTestStart.refreshDrawableState();
        mBtnGpsTestStart.setEnabled(false);
        mBtnGpsTestSave.setEnabled(false);
        if (null != mEtTestTimes) {
            mEtTestTimes.setFocusable(false);
            mEtTestTimes.refreshDrawableState();
            mEtTestTimes.setEnabled(false);
        }
        if (null != mCbNeed3DFix) {
            mCbNeed3DFix.setFocusable(false);
            mCbNeed3DFix.refreshDrawableState();
            mCbNeed3DFix.setEnabled(false);
        }
        if (null != mEtTestInterval) {
            mEtTestInterval.setFocusable(false);
            mEtTestInterval.refreshDrawableState();
            mEtTestInterval.setEnabled(false);
        }
        if (null != mEtTestLatitude) {
            mEtTestLatitude.setFocusable(false);
            mEtTestLatitude.refreshDrawableState();
            mEtTestLatitude.setEnabled(false);
        }
        if (null != mEtTestLongitude) {
            mEtTestLongitude.setFocusable(false);
            mEtTestLongitude.refreshDrawableState();
            mEtTestLongitude.setEnabled(false);
        }
        mBtnGpsTestStop.setEnabled(true);
        clearLayout();
        disableStartModeBtn();
        if (null != mChkAutoTestHot) {
            mChkAutoTestHot.setEnabled(false);
        }
        if (null != mChkAutoTestCold) {
            mChkAutoTestCold.setEnabled(false);
        }
        if (null != mChkAutoTestWarm) {
            mChkAutoTestWarm.setEnabled(false);
        }
        if (null != mChkAutoTestFull) {
            mChkAutoTestFull.setEnabled(false);
        }
        /*SPRD: fix bug 360943 change checkbox to radiobutton @*/
        if (null != mRadioBtnAutoHot) {
            mRadioBtnAutoHot.setEnabled(false);
        }
        if (null != mRadioBtnAutoWarm) {
            mRadioBtnAutoWarm.setEnabled(false);
        }
        if (null != mRadioBtnAutoCold) {
            mRadioBtnAutoCold.setEnabled(false);
        }
        if (null != mRadioBtnAutoFull) {
            mRadioBtnAutoFull.setEnabled(false);
        }
        /* @} */
        if (null != mSpreadOrBitSwitch) {
            mSpreadOrBitSwitch.setEnabled(false);
        }
        if (null != mRealEPHSwitch) {
            mRealEPHSwitch.setEnabled(false);
        }
    }

    // when start button is pressed, views excepts mBtnGPSTestStop must be
    // disabled

    private void setViewToStopState() {

        mBtnGpsTestStop.setEnabled(false);
        if (null != mEtTestTimes) {
            mEtTestTimes.setEnabled(true);
            mEtTestTimes.setFocusableInTouchMode(true);
            mEtTestTimes.refreshDrawableState();
        }
        if (null != mCbNeed3DFix) {
            mCbNeed3DFix.setEnabled(true);
            mCbNeed3DFix.refreshDrawableState();
        }
        if (null != mEtTestInterval) {
            mEtTestInterval.setEnabled(true);
            mEtTestInterval.setFocusableInTouchMode(true);
            mEtTestInterval.refreshDrawableState();
        }
        if (null != mEtTestLatitude) {
            mEtTestLatitude.setEnabled(true);
            mEtTestLatitude.setFocusableInTouchMode(true);
            mEtTestLatitude.refreshDrawableState();
        }
        if (null != mEtTestLongitude) {
            mEtTestLongitude.setEnabled(true);
            mEtTestLongitude.setFocusableInTouchMode(true);
            mEtTestLongitude.refreshDrawableState();
        }

        mBtnGpsTestSave.setEnabled(true);
        mBtnGpsTestStart.setEnabled(true);
        mBtnGpsTestStart.setFocusableInTouchMode(false);
        mBtnGpsTestStart.refreshDrawableState();

        enableSatrtModeBtn();

        if (null != mChkAutoTestHot) {
            mChkAutoTestHot.setEnabled(true);
        }
        if (null != mChkAutoTestCold) {
            mChkAutoTestCold.setEnabled(true);
        }
        if (null != mChkAutoTestWarm) {
            mChkAutoTestWarm.setEnabled(true);
        }
        if (null != mChkAutoTestFull) {
            mChkAutoTestFull.setEnabled(true);
        }
        /*SPRD: fix bug 360943 change checkbox to radiobutton @*/
        if (null != mRadioBtnAutoHot) {
            mRadioBtnAutoHot.setEnabled(true);
        }
        if (null != mRadioBtnAutoWarm) {
            mRadioBtnAutoWarm.setEnabled(true);
        }
        if (null != mRadioBtnAutoCold) {
            mRadioBtnAutoCold.setEnabled(true);
        }
        if (null != mRadioBtnAutoFull) {
            mRadioBtnAutoFull.setEnabled(true);
        }
        /* @} */
    }

    private void startGPSAutoTransferTest() {

        if (mHotSuccessRate != null){
            mHotSuccessRate.setText(null);
        }
        if (mWarmSuccessRate != null){
            mWarmSuccessRate.setText(null);
        }
        if (mColdSuccessRate != null){
            mColdSuccessRate.setText(null);
        }
        if (mFullSuccessRate != null){
            mFullSuccessRate.setText(null);
        }
        // check Times
        if (null != mEtTestTimes_01) {
            if (0 == mEtTestTimes_01.getText().length()) {
                Toast.makeText(SgpsActivity.this, R.string.toast_input_times,
                        Toast.LENGTH_LONG).show();
                mBtnGpsTestAutoTransferStart.setEnabled(true);
                return;
            } else {
                Integer nTimes = Integer.valueOf(mEtTestTimes_01.getText()
                        .toString());
                if (nTimes.intValue() < INPUT_VALUE_MIN
                        || nTimes.intValue() > INPUT_VALUE_MAX) {
                    Toast.makeText(SgpsActivity.this,
                            R.string.toast_input_range, Toast.LENGTH_LONG)
                            .show();
                    mBtnGpsTestAutoTransferStart.setEnabled(true);
                    return;
                }
                mAutoTransferTotalTimes = nTimes.intValue();
                Log.d(TAG, "mAutoTransferTotalTimes->" + mAutoTransferTotalTimes);
            }
        }

        // check Interval
        if (null != mEtTestInterval_01) {
            if (0 == mEtTestInterval_01.getText().length()) {
                Toast.makeText(SgpsActivity.this,
                        R.string.toast_input_interval, Toast.LENGTH_LONG)
                        .show();
                mBtnGpsTestAutoTransferStart.setEnabled(true);
                return;
            } else {
                Integer nInterval = Integer.valueOf(mEtTestInterval_01.getText()
                        .toString());
                if (nInterval.intValue() < INPUT_VALUE_MIN
                        || nInterval.intValue() > INPUT_VALUE_MAX) {
                    Toast.makeText(SgpsActivity.this,
                            R.string.toast_input_range, Toast.LENGTH_LONG)
                            .show();
                    mBtnGpsTestAutoTransferStart.setEnabled(true);
                    return;
                }
                mAutoTransferTestInterval = nInterval.intValue();
                Log.d(TAG, "mAutoTransferTestInterval->" + mAutoTransferTestInterval);
            }
        }

        if (null != mEtTestTransferInterval) {
            if (0 == mEtTestTransferInterval.getText().length()) {
                Toast.makeText(SgpsActivity.this,
                        R.string.toast_input_interval, Toast.LENGTH_LONG)
                        .show();
                mBtnGpsTestAutoTransferStart.setEnabled(true);
                return;
            } else {
                Integer nInterval = Integer.valueOf(mEtTestTransferInterval.getText()
                        .toString());
                if (nInterval.intValue() < INPUT_VALUE_MIN
                        || nInterval.intValue() > INPUT_VALUE_MAX) {
                    Toast.makeText(SgpsActivity.this,
                            R.string.toast_input_range, Toast.LENGTH_LONG)
                            .show();
                    mBtnGpsTestAutoTransferStart.setEnabled(true);
                    return;
                }
                mAutosTransferInternal = nInterval.intValue();
                Log.d(TAG, "mAutosTransferInternal->" + mAutosTransferInternal);
            }
        }

        //check TTFF timeout
        if (null != mEtTTFFTimeout) {
            if (0 == mEtTTFFTimeout.getText().length()) {
                Toast.makeText(SgpsActivity.this,
                        R.string.toast_input_ttff_timeout_value, Toast.LENGTH_LONG)
                        .show();
                mBtnGpsTestAutoTransferStart.setEnabled(true);
                return;
            } else {
                Integer nInterval = Integer.valueOf(mEtTTFFTimeout.getText()
                        .toString());
                if (nInterval.intValue() < INPUT_VALUE_MIN
                        || nInterval.intValue() > INPUT_VALUE_MAX) {
                    Toast.makeText(SgpsActivity.this,
                            R.string.toast_input_range, Toast.LENGTH_LONG)
                            .show();
                    mBtnGpsTestAutoTransferStart.setEnabled(true);
                    return;
                }
                mTimeoutValue = nInterval.intValue();
                Log.d(TAG, "mTimeoutValue->" + mTimeoutValue);
            }
        }
        // check latitude and longitude
        if (null != mEtTestLatitude && null != mEtTestLongitude) {
            if (0 == mEtTestLatitude.getText().length()
                    || 0 == mEtTestLongitude.getText().length()) {
                // do nothing
                SharedPreferences preferences = this.getSharedPreferences(SAVE_GPS,
                        android.content.Context.MODE_PRIVATE);
                mTestLatitude = Double.valueOf(preferences.getString(SAVE_GPS_LATITUDE, "360"));
                mTestLongitude = Double.valueOf(preferences.getString(SAVE_GPS_LONGITUDE, "360"));
            } else {
                Double mTestLatitude_test = 0.0;
                Double mTestLongitude_test = 0.0;
                try {
                    mTestLatitude_test = Double.valueOf(mEtTestLatitude
                            .getText().toString());
                    mTestLongitude_test = Double.valueOf(mEtTestLongitude
                            .getText().toString());
                } catch (Exception ex) {
                    Toast.makeText(SgpsActivity.this,
                            R.string.toast_gps_input_range, Toast.LENGTH_LONG)
                            .show();
                    mBtnGpsTestStart.setEnabled(true);
                    mBtnGpsTestSave.setEnabled(true);
                    return;
                }

                if ((mTestLatitude_test.doubleValue() < INPUT_GPS_VALUE_MIN || mTestLatitude_test
                        .doubleValue() > INPUT_GPS_VALUE_MAX)
                        || (mTestLongitude_test.doubleValue() < INPUT_GPS_VALUE_MIN || mTestLongitude_test
                                .doubleValue() > INPUT_GPS_VALUE_MAX)) {
                    Toast.makeText(SgpsActivity.this,
                            R.string.toast_gps_input_range, Toast.LENGTH_LONG)
                            .show();
                    mBtnGpsTestStart.setEnabled(true);
                    mBtnGpsTestSave.setEnabled(true);
                    return;
                }
                mTestLatitude = mTestLatitude_test.doubleValue();
                mTestLongitude = mTestLongitude_test.doubleValue();
            }
            if (null == mLastLocationRefence)
                mLastLocationRefence = new Location("refence");
            mLastLocationRefence.setLatitude(mTestLatitude);
            mLastLocationRefence.setLongitude(mTestLongitude);
        }
       // resetTestView();

        // start test now,start been pressed more times
        if (!mStartPressedAutoTransferHandling) {
            mStartPressedAutoTransferHandling = true;
            mBtnGpsTestAutoTransferStop.setEnabled(true);
            mIsAutoTransferMode = true;
            clearLayout2();
            setViewToAutoTransferStartState();
            setViewToStartState();
            mAutoCircleTestThread = new AutoCircleTestThread();
            if (null != mAutoCircleTestThread) {
                setmIsAutoTransferTestRunning(true);
                mAutoCircleTestThread.start();
            } else {
                Log.w(TAG, "new AutoCircleTestThread failed");
            }
            Log.w(TAG, "new AutoCircleTestThread start!");

        } else {
            Log.w(TAG, "GpsAutoTransfer start button has been pushed.");
            mBtnGpsTestAutoTransferStart.refreshDrawableState();
            mBtnGpsTestAutoTransferStart.setEnabled(false);
        }

    }

    private void startGPSAutoTest() {
        // check Times
        if (null != mEtTestTimes) {
            if (0 == mEtTestTimes.getText().length()) {
                Toast.makeText(SgpsActivity.this, R.string.toast_input_times,
                        Toast.LENGTH_LONG).show();
                mBtnGpsTestStart.setEnabled(true);
                mBtnGpsTestSave.setEnabled(true);
                return;
            } else {
                Integer nTimes = Integer.valueOf(mEtTestTimes.getText()
                        .toString());
                if (nTimes.intValue() < INPUT_VALUE_MIN
                        || nTimes.intValue() > INPUT_VALUE_MAX) {
                    Toast.makeText(SgpsActivity.this,
                            R.string.toast_input_range, Toast.LENGTH_LONG)
                            .show();
                    mBtnGpsTestStart.setEnabled(true);
                    mBtnGpsTestSave.setEnabled(true);
                    return;
                }
                mTotalTimes = nTimes.intValue();
            }
        }

        // check Interval
        if (null != mEtTestInterval) {
            if (0 == mEtTestInterval.getText().length()) {
                Toast.makeText(SgpsActivity.this,
                        R.string.toast_input_interval, Toast.LENGTH_LONG)
                        .show();
                mBtnGpsTestStart.setEnabled(true);
                mBtnGpsTestSave.setEnabled(true);
                return;
            } else {
                Integer nInterval = Integer.valueOf(mEtTestInterval.getText()
                        .toString());
                if (nInterval.intValue() < INPUT_VALUE_MIN
                        || nInterval.intValue() > INPUT_VALUE_MAX) {
                    Toast.makeText(SgpsActivity.this,
                            R.string.toast_input_range, Toast.LENGTH_LONG)
                            .show();
                    mBtnGpsTestStart.setEnabled(true);
                    mBtnGpsTestSave.setEnabled(true);
                    return;
                }
                mTestInterval = nInterval.intValue();
                Log.d(TAG, "mTestInterval->" + mTestInterval);
            }
        }

        // check latitude and longitude
        if (null != mEtTestLatitude && null != mEtTestLongitude) {
            if (0 == mEtTestLatitude.getText().length()
                    || 0 == mEtTestLongitude.getText().length()) {
                // do nothing
                SharedPreferences preferences = this.getSharedPreferences(SAVE_GPS,
                        android.content.Context.MODE_PRIVATE);
                mTestLatitude = Double.valueOf(preferences.getString(SAVE_GPS_LATITUDE, "360"));
                mTestLongitude = Double.valueOf(preferences.getString(SAVE_GPS_LONGITUDE, "360"));
            } else {
                Double mTestLatitude_test = 0.0;
                Double mTestLongitude_test = 0.0;
                try {
                    mTestLatitude_test = Double.valueOf(mEtTestLatitude
                            .getText().toString());
                    mTestLongitude_test = Double.valueOf(mEtTestLongitude
                            .getText().toString());
                } catch (Exception ex) {
                    Toast.makeText(SgpsActivity.this,
                            R.string.toast_gps_input_range, Toast.LENGTH_LONG)
                            .show();
                    mBtnGpsTestStart.setEnabled(true);
                    mBtnGpsTestSave.setEnabled(true);
                    return;
                }

                if ((mTestLatitude_test.doubleValue() < INPUT_GPS_VALUE_MIN || mTestLatitude_test
                        .doubleValue() > INPUT_GPS_VALUE_MAX)
                        || (mTestLongitude_test.doubleValue() < INPUT_GPS_VALUE_MIN || mTestLongitude_test
                                .doubleValue() > INPUT_GPS_VALUE_MAX)) {
                    Toast.makeText(SgpsActivity.this,
                            R.string.toast_gps_input_range, Toast.LENGTH_LONG)
                            .show();
                    mBtnGpsTestStart.setEnabled(true);
                    mBtnGpsTestSave.setEnabled(true);
                    return;
                }
                mTestLatitude = mTestLatitude_test.doubleValue();
                mTestLongitude = mTestLongitude_test.doubleValue();
            }
            if (null == mLastLocationRefence)
                mLastLocationRefence = new Location("refence");
            mLastLocationRefence.setLatitude(mTestLatitude);
            mLastLocationRefence.setLongitude(mTestLongitude);
        }

        mAutoTestHot = mChkAutoTestHot.isChecked();
        mAutoTestCold = mChkAutoTestCold.isChecked();
        mAutoTestWarm = mChkAutoTestWarm.isChecked();
        mAutoTestFull = mChkAutoTestFull.isChecked();

        // need 3D fix? check it
        if (null != mCbNeed3DFix) {
            mIsNeed3DFix = mCbNeed3DFix.isChecked();
        }
        resetTestView();

        // start test now,start been pressed more times
        if (!mStartPressedHandling) {
            mStartPressedHandling = true;
            mIsAutoTransferMode = false;
            setViewToStartState();
            setViewToAutoTransferStartState();
            mAutoTestThread = new AutoTestThread();
            if (null != mAutoTestThread) {
                mAutoTestThread.start();
            } else {
                Log.w(TAG, "new myAutoTestThread failed");
            }
            Log.w(TAG, "new myAutoTestThread start!");

        } else {
            Log.w(TAG, "start button has been pushed.");
            mBtnGpsTestStart.refreshDrawableState();
            mBtnGpsTestStart.setEnabled(false);
            mBtnGpsTestSave.setEnabled(false);
        }

    }

    private void stopAutoTransferGPSAutoTest() {
        resetAutoTransferTestParam();
        setTestParam();
        Log.d(TAG, "resetParam stop autotest log");
        //mHandler.sendEmptyMessage(HANDLE_COMMAND_OTHERS_UPDATE_RESULT_LOG_END);
    }

    private void stopGPSAutoTest() {
        resetTestParam();
        // Bundle extras = new Bundle();
        // extras.putBoolean("ephemeris", true);
        // resetParam(extras, false); // do connect when stop test

        // disable reconnect
        setTestParam();
        mHandler.sendEmptyMessage(HANDLE_COMMAND_OTHERS_UPDATE_RESULT_LOG_END);
    }

    private void setmIsTestRunning(boolean isrunning) {
        synchronized (mTestRunningLock) {
            mIsTestRunning = isrunning;
        }
    }

    private boolean getmIsTestRunning() {
        synchronized (mTestRunningLock) {
            return mIsTestRunning;
        }
    }

    private void setmIsAutoTransferTestRunning(boolean isrunning) {
        synchronized (mAutoTransferTestRunningLock) {
            mIsAutoTransferTestRunning = isrunning;
        }
    }

    private boolean getmIsAutoTransferTestRunning() {
        synchronized (mAutoTransferTestRunningLock) {
            return mIsAutoTransferTestRunning;
        }
    }

    private void resetTestParam() {
        mIsNeed3DFix = false;
        mTotalTimes = 0;
        mCurrentTimes = 0;
        mTestInterval = 0;
        mMeanTTFF = 0f;
        setmIsTestRunning(false);
    }

    private void resetAutoTransferTestParam() {
        mAutoTransferTestInterval = 0;
        mAutoTransferTotalTimes = 0;
        mCurrentTimes = 0;
        setmIsAutoTransferTestRunning(false);
    }

    private void clearLayout2() {
        mCurrentTimeHot.setText(R.string.empty);
        mCurrentTimeWarm.setText(R.string.empty);
        mCurrentTimeCold.setText(R.string.empty);
        mCurrentTimeFactory.setText(R.string.empty);

        mCountDownHot.setText(R.string.empty);
        mCountDownWarm.setText(R.string.empty);
        mCountDownCold.setText(R.string.empty);
        mCountDownFactory.setText(R.string.empty);

        mLastTTFFHot.setText(R.string.empty);
        mLastTTFFWarm.setText(R.string.empty);
        mLastTTFFCold.setText(R.string.empty);
        mLastTTFFFactory.setText(R.string.empty);

        mAverageHot.setText(R.string.empty);
        mAverageWarm.setText(R.string.empty);
        mAverageCold.setText(R.string.empty);
        mAverageFactory.setText(R.string.empty);
    }

    private void updateLastTTFF(int ttffvalue){
        if(getmIsAutoTransferTestRunning()){
            switch(mCurrentMode){
                case 0:
                    mLastTTFFHot.setText(ttffvalue + getString(R.string.time_unit_ms));
                    break;
                case 1:
                    mLastTTFFCold.setText(ttffvalue + getString(R.string.time_unit_ms));
                    break;
                case 2:
                    mLastTTFFWarm.setText(ttffvalue + getString(R.string.time_unit_ms));
                    break;
                case 3:
                    mLastTTFFFactory.setText(ttffvalue + getString(R.string.time_unit_ms));
                    break;
            }
        }
    }

    private String getCurrentMode() {
        switch(mCurrentMode){
            case 0:
                return "Hot";
            case 1:
                return "Cold";
            case 2:
                return "Warm";
            case 3:
                return "Factory";
        }
        return "";
    }

    private void updateAverageTTFF(float meanttff) {
        if(getmIsAutoTransferTestRunning()){
            switch(mCurrentMode){
                case 0:
                    mAverageHot.setText(Float.valueOf(meanttff).toString() + getString(R.string.time_unit_ms));
                    break;
                case 1:
                    mAverageCold.setText(Float.valueOf(meanttff).toString() + getString(R.string.time_unit_ms));
                    break;
                case 2:
                    mAverageWarm.setText(Float.valueOf(meanttff).toString() + getString(R.string.time_unit_ms));
                    break;
                case 3:
                    mAverageFactory.setText(Float.valueOf(meanttff).toString() + getString(R.string.time_unit_ms));
                    break;
            }
        }
    }

    private void updateTransferCountDown(int nCountDown){
        //if(getmIsAutoTransferTestRunning()){
            switch(mCurrentMode){
                case 0:
                    mCountDownHot.setText(""+nCountDown);
                    break;
                case 1:
                    mCountDownCold.setText(""+nCountDown);
                    break;
                case 2:
                    mCountDownWarm.setText(""+nCountDown);
                    break;
                case 3:
                    mCountDownFactory.setText(""+nCountDown);
                    break;
            }
        //}
    }

    private void sendModeAndTimes(int nmode ,int ntime) {
        Message msg = mAutoTestHandler.obtainMessage(SgpsActivity.HANDLE_AUTO_TRANSFER_UPDATE_CURRENT_MODE);
        msg.arg1 = nmode;
        msg.arg2 = ntime;
        mAutoTestHandler.sendMessage(msg);
    }

    private void showToast(int nmode , int ntime){
        //runOnUiThread(new Runnable(){
            //public void run(){
                String tmpstr = "";
                switch(nmode){
                    case 0:
                        tmpstr += "mode = Hot runtime = " + ntime;
                        //Toast.makeText(SgpsActivity.this, "mode = Hot runtime = " + ntime ,Toast.LENGTH_LONG).show();
                        break;
                    case 1:
                        tmpstr += "mode = Cold runtime = " + ntime;
                        //Toast.makeText(SgpsActivity.this, "mode = Cold runtime = " + ntime ,Toast.LENGTH_LONG).show();
                        break;
                    case 2:
                        tmpstr += "mode = Warm runtime = " + ntime;
                        //Toast.makeText(SgpsActivity.this, "mode = Warm runtime = " + ntime ,Toast.LENGTH_LONG).show();
                        break;
                    case 3:
                        tmpstr += "mode = Factory runtime = " + ntime;
                        //Toast.makeText(SgpsActivity.this, "mode = Factory runtime = " + ntime ,Toast.LENGTH_LONG).show();
                        break;
                }
                Toast.makeText(SgpsActivity.this, tmpstr, Toast.LENGTH_LONG).show();

                switch(mCurrentMode){
                    case 0:
                        mCurrentTimeHot.setText(""+ntime);
                        break;
                    case 1:
                        mCurrentTimeCold.setText(""+ntime);
                        break;
                    case 2:
                        mCurrentTimeWarm.setText(""+ntime);
                        break;
                    case 3:
                        mCurrentTimeFactory.setText(""+ntime);
                        break;
                }
        //    }
        //});
    }

    private void resetTestView() {
        // ((TextView)SGPSActivity.this.findViewById(R.id.tv_CurrentTimes)).setText("");
        // ((TextView)SGPSActivity.this.findViewById(R.id.tv_Reconnect_Countdown)).setText("");
//        ((TextView) SgpsActivity.this.findViewById(R.id.tv_mean_ttff))
//                .setText("");
//        ((TextView) SgpsActivity.this.findViewById(R.id.tv_last_ttff))
//                .setText("");
    }

    private float meanTTFF(int n) {
        return (mMeanTTFF * (n - 1) + mTtffValue) / n;
    }

    private Handler mAutoTestHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case HANDLE_SET_CURRENT_TIMES:
//                    ((TextView) SgpsActivity.this
//                            .findViewById(R.id.tv_current_times)).setText(Integer
//                            .valueOf(msg.arg1).toString());
                    break;
                case HANDLE_SET_COUNTDOWN:
//                    ((TextView) SgpsActivity.this
//                            .findViewById(R.id.tv_reconnect_countdown))
//              	.setText(Integer.valueOf(msg.arg1).toString());
                    updateTransferCountDown(msg.arg1);
                    break;
                case HANDLE_START_BUTTON_UPDATE:
                    mBtnGpsTestStart.setEnabled(MESSAGE_ARG_1 == msg.arg1);
                    mBtnGpsTestSave.setEnabled(MESSAGE_ARG_1 == msg.arg1);
                    mBtnGpsTestStop.setEnabled(MESSAGE_ARG_0 == msg.arg1);
                    if (msg.arg1 == MESSAGE_ARG_1) {
                        setViewToStopState();
                        setViewToAutoTransferStopState();
                    }
                    break;
                case HANDLE_AUTO_TRANSFER_START_BUTTON_UPDATE:
                    mBtnGpsTestAutoTransferStart.setEnabled(MESSAGE_ARG_1 == msg.arg1);
                    mBtnGpsTestAutoTransferStart.setEnabled(MESSAGE_ARG_0 == msg.arg1);
                    if (msg.arg1 == MESSAGE_ARG_1) {
                        setViewToAutoTransferStopState();
                        setViewToStopState();
                    }
                    break;
                case HANDLE_AUTO_TRANSFER_UPDATE_CURRENT_MODE:
                    showToast(msg.arg1, msg.arg2);
                    break;
                case HANDLE_EXCEED_PERIOD:
                    final int count = msg.arg1;
                    currCountTimes = count;
                    builder.setPositiveButton("Yes", new DialogInterface.OnClickListener() {

                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            isContinue = true;
                            if (getmIsTestRunning()) {
                                startGPSAutoTest();
                            } else {
                                startGPSAutoTransferTest();
                            }
                        }

                    });
                    builder.setNegativeButton("No", new DialogInterface.OnClickListener() {

                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            isContinue = false;
                        }

                    });
                    alertDialog = builder.create();
                    alertDialog.setTitle("Exceed period");
                    alertDialog.setMessage("Exceed period!Are you want to continue?");
                    //bug 331858 start
                    //When SgpsActivity has Destroyed,not show Dialog.
                    if (!isFinishing()) {
                        alertDialog.show();
                    } else {
                        Log.d(TAG,"SgpsActivity has Destroyed.");
                    }
                    //bug 331858 end
                    break;
                case HANDLE_SET_MEANTTFF:
//                    ((TextView) SgpsActivity.this.findViewById(R.id.tv_mean_ttff))
//                            .setText(Float.valueOf(mMeanTTFF).toString());
                    break;
                case HANDLE_SET_PARAM_RECONNECT:
                    Bundle extras = new Bundle();
                    // extras.putBoolean(GPS_EXTRA_EPHEMERIS, true);
                    extras.putBoolean(GPS_EXTRA_RTI, true);// change to hot
                    resetParam(extras, false, true);
                    break;
                case HANDLE_TTFF_TIMEOUT:
                    mTTFFTimeoutCont++;
                    if (mLocationManager != null) {
                        mLocationManager.removeUpdates(mLocListener);
                    }
                    break;
                default:
                    break;
            }
        }
    };

    private class AutoTestThread extends Thread {
        @Override
        public void run() {
            super.run();
            Looper.prepare();
            // try {
            setStartButtonEnable(false);
            if (isContinue) {
                reconnectTest(currCountTimes);
            } else {
                reconnectTest(1);
            }
            setStartButtonEnable(true);
            interrupt();
            // } catch (Exception e) {
            // e.printStackTrace();
            // }
        }
    }

    /* Spreadst: cold, warm, hot and factory switch. @{ */
    private class AutoCircleTestThread extends Thread {
        @Override
        public void run() {
            super.run();
            Looper.prepare();
            int mModeCount = 4;
            CheckBox[] mCheckboxArray = {
                    mCircAutoTestHot, mCircAutoTestCold, mCircAutoTestWarm, mCircAutoTestFull
            };
            setStartButtonEnable(false);
            try {
                for (int i = 0; i < mModeCount && getmIsAutoTransferTestRunning(); i++) {
                    if (!mCheckboxArray[i].isChecked()) {
                        continue;
                    }
                    if (!mBtnNmeaStart.isEnabled()) {
                        if (!createFileForSavingNMEALog()) {
                            Log.i(TAG, "createFileForSavingNMEALog return false mode is "
                                    + getCurrentMode());
                        }
                    }
                    Log.d(TAG, "AutoCircleTestThread " + GetStartMode(i) + " mode is start !");
                    mTTFFTimeoutCont = 0;
                    mTtff = new float[999];
                    mDistance = new float[999];
                    mFirstFixLatitude = new double[999];
                    mFirstFixLongitude = new double[999];
                    mSatelliteTestCont = 0;
                    mTotalInused = new int[999];
                    mTotalView = new int[999];
                    mGpsInUsed = new int[999];
                    mGpsView = new int[999];
                    mGlonassInUsed = new int[999];
                    mGlonassView = new int[999];
                    mBeidouInUsed = new int[999];
                    mBeidouView = new int[999];
                    mTotalInUsedMin = SATELLITE_MAX_COUNT;
                    mTotalInUsedMax = SATELLITE_MIN_COUNT;
                    mTotalViewMin = SATELLITE_MAX_COUNT;
                    mTotalViewMax = SATELLITE_MIN_COUNT;
                    mGpsInUsedMin = SATELLITE_MAX_COUNT;
                    mGpsInUsedMax = SATELLITE_MIN_COUNT;
                    mGpsViewMin = SATELLITE_MAX_COUNT;
                    mGpsViewMax = SATELLITE_MIN_COUNT;
                    mGlonassInUsedMin = SATELLITE_MAX_COUNT;
                    mGlonassInUsedMax = SATELLITE_MIN_COUNT;
                    mGlonassViewMin = SATELLITE_MAX_COUNT;
                    mGlonassViewMax = SATELLITE_MIN_COUNT;
                    mBeidouInUsedMin = SATELLITE_MAX_COUNT;
                    mBeidouInUsedMax = SATELLITE_MIN_COUNT;
                    mBeidouViewMin = SATELLITE_MAX_COUNT;
                    mBeidouViewMax = SATELLITE_MIN_COUNT;
                    mSateTracking = new int[999];
                    mSateTrackingCont = 0;
                    cont = 0;
                    mDistanceCont = 0;
                    /* spreadst: switch mode. @{ */
                    mCurrentMode = i;
                    perpareGpsMode(i);
                    /* spreadst: only create file. @{ */
                    mIsCompleted = false; // not create file......
                    resetParam(Bundletemp, true, true);
                    mGpsTestStart = true;
                    mIsCompleted = true; // already create file......
                    /* @} */

                    for (int j = 0; j < mAutoTransferTotalTimes && getmIsAutoTransferTestRunning(); j++) {
                        mCurrentTimes = j + 1;
                        mTtffTimeoutFlag = false;
                        sendModeAndTimes(i, j + 1);
                        Log.d(TAG, "mode = " + i + " times = " + j + " seconds ="
                                + mAutoTransferTestInterval);
                        Long beginTime = Calendar.getInstance().getTime().getTime()
                                / ONE_SECOND;
                        Log.d(TAG, "beginTime[first]" + beginTime);
                        for (; getmIsAutoTransferTestRunning();) {
                            Long nowTime = Calendar.getInstance().getTime().getTime()
                                    / ONE_SECOND;
                            if (mFirstFix) {
                                break;
                            } else if (nowTime - beginTime > EXCEED_SECOND) {
                                showExceedPeriod(EXCEED_SECOND, i);
                                break;
                            } else if (nowTime - beginTime > mTimeoutValue) {
                                mTtffTimeoutFlag = true;
                                showTTFFTimeout();
                                break;
                            }

                            Thread.sleep(ONE_SECOND / 10);
                        }
                        mShowFirstFixLocate = true;
                        boolean hasNextTest = false;
                        if (mCurrentTimes > 0 && mCurrentTimes < mAutoTransferTotalTimes) {
                            hasNextTest = true;
                        } else {
                            hasNextTest = false;
                        }
                        resetParam(Bundletemp, true, hasNextTest);
                    }
                    mMeanTTFF = 0;
                    /* spreadst: send command to close file. one mode to save one file. @{ */
                    mHandler.sendEmptyMessage(HANDLE_COMMAND_OTHERS_UPDATE_RESULT_LOG_END);
                    /* @} */
                    Thread.sleep(1000 * mAutosTransferInternal);
                }

                // all complete!!!
                stopAutoTransferGPSAutoTest();
            } catch (InterruptedException e) {
            }

            setAutoTransferStartButtonEnable(true);
            mGpsTestStart = false;
            interrupt();
        }
    }

    /* @} */

    private void setAutoTestMode() {
        if (mAutoTestHot) {
            perpareGpsMode(HOT_START);
            mAutoTestHot = false;
        } else if (mAutoTestWarm) {
            perpareGpsMode(WARM_START);
            mAutoTestWarm = false;
        } else if (mAutoTestCold) {
            perpareGpsMode(COLD_START);
            mAutoTestCold = false;
        } else {
            perpareGpsMode(FULL_START);
            mAutoTestFull = false;
        }
    }

    private void reconnectTest(int startCount) {
        boolean bExceed = false;
        // extras.putBoolean(GPS_EXTRA_EPHEMERIS, true);
        /*SPRD: fix bug 360943 change checkbox to radiobutton @*/
        if (mGpsAutoTest) {
//            setAutoTestMode();
            startGpsModeAuto();
        /* @} */
        } else {
            StartGpsMode();
        }
        if (!isContinue) {
            setCurrentTimes(0);
        }
        resetParam(Bundletemp, true, true);
        setmIsTestRunning(true);
        mTTFFTimeoutCont = 0;
        try {
            for (int i = startCount; i <= mTotalTimes && getmIsTestRunning(); ++i) {
                mCurrentTimes = i;
                Log.d(TAG,
                        "reconnectTest function: "
                                + Integer.valueOf(mCurrentTimes).toString());
                setCurrentTimes(i);
                // if (mIsNeed3DFix) {
                Long beginTime = Calendar.getInstance().getTime().getTime()
                        / ONE_SECOND;
                Log.d(TAG, "beginTime[first]" + beginTime);
                for (; getmIsTestRunning();) {
                    Long nowTime = Calendar.getInstance().getTime().getTime()
                            / ONE_SECOND;
                    Log.d(TAG, "retryTimes->" + (nowTime - beginTime));
                    if (mFirstFix) {
                        break;
                    } else if (nowTime - beginTime > EXCEED_SECOND) {
                        bExceed = true;
                        showExceedPeriod(EXCEED_SECOND, i);
                        break;
                    }
                    Thread.sleep(ONE_SECOND / 10);
                }
                if (bExceed) {
                    break;
                }

                resetParam(Bundletemp, true, true);
                // } else {
                // Thread.sleep(2 * ONE_SECOND);
                // }
            }
            Thread.sleep(ONE_SECOND);
            stopGPSAutoTest();
        } catch (InterruptedException e) {
            Log.w(TAG, "GPS auto test thread interrupted: " + e.getMessage());
            Toast.makeText(SgpsActivity.this, R.string.toast_test_interrupted,
                    Toast.LENGTH_LONG).show();
        }
        Bundletemp = null;
    }

    private void setTestParam() {
        Message msg = mAutoTestHandler
                .obtainMessage(SgpsActivity.HANDLE_SET_PARAM_RECONNECT);
        mAutoTestHandler.sendMessage(msg);
    }

    private void setAutoTransferStartButtonEnable(boolean bEnable) {
        Message msg = mAutoTestHandler
                .obtainMessage(SgpsActivity.HANDLE_AUTO_TRANSFER_START_BUTTON_UPDATE);
        msg.arg1 = bEnable ? MESSAGE_ARG_1 : MESSAGE_ARG_0;
        mAutoTestHandler.sendMessage(msg);
    }

    private void setStartButtonEnable(boolean bEnable) {
        Message msg = mAutoTestHandler
                .obtainMessage(SgpsActivity.HANDLE_START_BUTTON_UPDATE);
        msg.arg1 = bEnable ? MESSAGE_ARG_1 : MESSAGE_ARG_0;
        mAutoTestHandler.sendMessage(msg);
    }

    private void setCurrentTimes(int nTimes) {
        Message msg = mAutoTestHandler
                .obtainMessage(SgpsActivity.HANDLE_SET_CURRENT_TIMES);
        msg.arg1 = nTimes;
        mAutoTestHandler.sendMessage(msg);
    }

    private void setCountDown(int num) {
        Message msg = mAutoTestHandler
                .obtainMessage(SgpsActivity.HANDLE_SET_COUNTDOWN);
        msg.arg1 = num;
        mAutoTestHandler.sendMessage(msg);
    }

    private void showExceedPeriod(int period, int mCount) {
        Message msg = mAutoTestHandler
                .obtainMessage(SgpsActivity.HANDLE_EXCEED_PERIOD);
        msg.arg1 = mCount;
        mAutoTestHandler.sendMessage(msg);
    }

    private void showSuccessRate(String text) {
        switch(mCurrentMode){
        case 0:
            mHotSuccessRate.setText(text);
            break;
        case 1:
            mColdSuccessRate.setText(text);
            break;
        case 2:
            mWarmSuccessRate.setText(text);
            break;
        case 3:
            mFullSuccessRate.setText(text);
        default:
            break;
        }
    }

    private void showTTFFTimeout() {
        Message msg = mAutoTestHandler
                .obtainMessage(SgpsActivity.HANDLE_TTFF_TIMEOUT);
        mAutoTestHandler.sendMessage(msg);
    }

    /* SPRD:add for GE2 @{ */
    private void setCommandToProvider(int mode){
        String type = null;
        switch(mode){
            case GPS_ONLY:
                  type = GPS_EXTRA_GPSONLY;
                  break;
            case GLONASS_ONLY:
                  type = GPS_EXTRA_GLONASS;
                  break;
            case BDS_ONLY:
                  type = GPS_EXTRA_BDSONLY;
                  break;
            case GLONASS_GPS:
                  type = GPS_EXTRA_GLONASSGPS;
                  break;
            case GPS_BDS:
                  type = GPS_EXTRA_GPSBDS;
                  break;
            default:
                  break;
        }
        if(type != null){
            Log.d(TAG, " setCommandToProvider type: " + type+"mSocketClient"+mSocketClient);
            sendCommand(type);
        }
    }
    /* @} */
    private long mLastTimestamp = -1;

    public class NmeaListenClass implements NmeaListener {

        @Override
        public void onNmeaReceived(long timestamp, String nmea) {
            if (GPS_CHIP_TYPE_IS_GE2) {
                if (nmeaParser == null) {
                    nmeaParser = new NmeaParser(SgpsActivity.this);
                    nmeaParser.setLocalNmeaListener(SgpsActivity.this);
                }
                if (nmeaParser != null) {
                    nmeaParser.parserNmeaStatement(nmea);
                }
            }
            if (!mIsShowVersion) {
                if (timestamp - mLastTimestamp > ONE_SECOND) {
                    // showGPSVersion();
                    mLastTimestamp = timestamp;
                }
            }
            if (mStartNmeaRecord) {
                saveNMEALog(nmea);
                // mTvNmeaLog.setText(nmea);
                // if(mTvNmeaLog.isVisible())
                if (ViewLogTab) {
                    mTvNmeaLog.append(nmea);
                    mHandler.post(mScrollToBottom);
                }
            }
        }
    }
    protected void setDialog() {
        builder = new AlertDialog.Builder(SgpsActivity.this);
        gpsBuilder = new AlertDialog.Builder(SgpsActivity.this);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "Enter onCreate  function of Main Activity");
        super.onCreate(savedInstanceState);

        StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder()
                // .detectDiskReads()
                // .detectDiskWrites()
                .detectNetwork() // or .detectAll() for all detectable problems
                // .penaltyLog()
                .build());
        // StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder()
        // .detectLeakedSqlLiteObjects()
        // .detectLeakedClosableObjects()
        // .penaltyLog()
        // .penaltyDeath()
        // .build());
        TabHost tabHost = getTabHost();
        LayoutInflater.from(this).inflate(R.layout.layout_tabs,
                tabHost.getTabContentView(), true);

        // config tab
        tabHost.addTab(tabHost.newTabSpec(this.getString(R.string.agps_title))
                .setIndicator(this.getString(R.string.agps_title))
                .setContent(R.id.layout_agps));

        // tab1
        tabHost.addTab(tabHost.newTabSpec(this.getString(R.string.satellites))
                .setIndicator(this.getString(R.string.satellites))
                .setContent(R.id.layout_satellites));

        // tab2
        tabHost.addTab(tabHost.newTabSpec(this.getString(R.string.information))
                .setIndicator(this.getString(R.string.information))
                .setContent(R.id.layout_info));

        // tab3
        tabHost.addTab(tabHost.newTabSpec(this.getString(R.string.nmea_log))
                .setIndicator(this.getString(R.string.nmea_log))
                .setContent(R.id.layout_nmea));

        /* Spresdst: add a tab. @{ */
        // tab4
        tabHost.addTab(tabHost.newTabSpec(this.getString(R.string.gps_circle_test))
                .setIndicator(this.getString(R.string.gps_circle_test))
                .setContent(R.id.layout_auto_circle_test));
        /* @} */

        // tab5
        tabHost.addTab(tabHost.newTabSpec(this.getString(R.string.gps_test))
                .setIndicator(this.getString(R.string.gps_test))
                .setContent(R.id.layout_auto_test));

        tabHost.setOnTabChangedListener(new OnTabChangeListener() {
            public void onTabChanged(String tabId) {
                Log.v(TAG, "Select: " + tabId + " "
                        + getString(R.string.nmea_log));
                /*SPRD: fix bug 360943 change checkbox to radiobutton @*/
                if (tabId.equals(getString(R.string.gps_test))) {
                    mGpsAutoTest = true;
                } else {
                    mGpsAutoTest = false;
                }
                /* @}*/

                if (mTvNmeaLog != null
                        && tabId.equals(getString(R.string.nmea_log))) {
                    ViewLogTab = true;
                    /* SPRD: fix bug336487 Nmea view dump @{*/
                    mViewLogTab = true;
                    /* @}*/
                } else {
                    ViewLogTab = false;
                    /* SPRD: fix bug336487 Nmea view dump @{*/
                    mViewLogTab = false;
                    /* @}*/
                }
            }
        });
        mSocketClient = new ClientSocket(this);
        initGpsConfigFile();
        setLayout();
        initGpsConfigLayout();
        setDialog();
        Intent it = new Intent(SgpsActivity.this,SgpsService.class);
        startService(it);
        Log.d(TAG, "START service");
        mSgpsWakeLock = new SgpsWakeLock();
        mSgpsWakeLock.acquireScreenWakeLock(this);
        mNmeaListener = new NmeaListenClass();
        try {
            if (mLocationManager == null) {
                mLocationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
            }
            if (mLocationManager != null) {
                /*
                 * mLocationManager.requestLocationUpdates(
                 * LocationManager.GPS_PROVIDER, 0, 0, mLocListener); Log.v(TAG,
                 * "request(mLocListener)");
                 */
                mLocationManager.addGpsStatusListener(mGpsListener);
                mLocationManager.addNmeaListener(mNmeaListener);
                if (mLocationManager
                        .isProviderEnabled(LocationManager.GPS_PROVIDER)) {
                    mProvider = String.format(getString(
                            R.string.provider_status_enabled,
                            LocationManager.GPS_PROVIDER));
                } else {
                    mProvider = String.format(getString(
                            R.string.provider_status_disabled,
                            LocationManager.GPS_PROVIDER));
                }
                mStatus = getString(R.string.gps_status_unknown);
                /* SPRD:add for GE2 @{  */
                /* init setting */
                int gnssMode = getGNSSMode();
                setCommandToProvider(gnssMode);
                /* @} */
            } else {
                Log.w(TAG, "new mLocationManager failed");
            }
        } catch (SecurityException e) {
            Toast.makeText(this, "security exception", Toast.LENGTH_LONG)
                    .show();
            Log.w(TAG, "Exception: " + e.getMessage());
        } catch (IllegalArgumentException e) {
            Log.w(TAG, "Exception: " + e.getMessage());
        }
        /* mHandler.sendEmptyMessage(HANDLE_COUNTER); */
        final SharedPreferences preferences = this.getSharedPreferences(
                SHARED_PREF_KEY_BG, android.content.Context.MODE_PRIVATE);
        if (preferences.getBoolean(SHARED_PREF_KEY_BG, true)) {
            mIsRunInBg = true;
        } else {
            mIsRunInBg = false;
        }
        mShowFirstFixLocate = true;
        mPowerKeyFilter = new IntentFilter(INTENT_ACTION_SCREEN_OFF);
        mPowerKeyReceiver = new BroadcastReceiver() {

            @Override
            public void onReceive(Context context, Intent intent) {
                Log.v(TAG, "onReceive, receive SCREEN_OFF event");
                // finish();
            }
        };
        registerReceiver(mPowerKeyReceiver, mPowerKeyFilter);
        Log.v(TAG, "registerReceiver powerKeyReceiver");
        //mSocketClient = new ClientSocket(this);

        TextView tvTtff = (TextView) findViewById(R.id.tv_ttff);
        tvTtff.setText(0
                + getString(R.string.time_unit_ms));

        // blink issue
        // mHandler.sendEmptyMessage(HANDLE_CHECK_SATEREPORT);
        if (GPS_CHIP_TYPE_IS_GE2) {
            nmeaParser = new NmeaParser(SgpsActivity.this);
            nmeaParser.setLocalNmeaListener(SgpsActivity.this);
        }
    }

    private void initGpsConfigFile() {
        File cgConfig = new File(SgpsUtils.GPS_CSR_CG_CONFIG_FILE);
        if (cgConfig.exists()) {
            mGpsConfigFile = SgpsUtils.GPS_CSR_CG_CONFIG_FILE;
        } else {
            File gnssConfig = new File(SgpsUtils.GPS_CSR_GNSS_CONFIG_FILE);
            if (gnssConfig.exists()) {
                mGpsConfigFile = SgpsUtils.GPS_CSR_GNSS_CONFIG_FILE;
            } else {
                mGpsConfigFile = null;
            }
        }
        Log.d(TAG, "initGpsConfigFile mGpsConfigFile:" + mGpsConfigFile);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        boolean supRetVal = super.onCreateOptionsMenu(menu);
        // menu.add(0, 0, 0, getString(R.string.menu_gps_cfg));
        menu.add(0, 0, 0, getString(R.string.menu_agps_log));
        // MenuItemCompat.setShowAsAction(menu.getItem(1),
        // MenuItemCompat.SHOW_AS_ACTION_ALWAYS);
        // MenuItemCompat.setShowAsAction(menu.getItem(2),
        // MenuItemCompat.SHOW_AS_ACTION_ALWAYS);
        return supRetVal;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        // case 0:
        // try {
        // Intent intent = new Intent();
        // /* SPRD: modify for 306833 @{ */
        // /*
        // * intent.setAction("android.intent.action.MAIN");
        // * intent.setClassName("com.android.settings",
        // * "com.sprd.settings.LocationGpsConfig");
        // * intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        // * startActivity(intent);
        // */
        // intent.setAction("android.settings.GPS_CONFIG");
        // startActivity(intent);
        // /* @} */
        // } catch (Exception ex) {
        // Log.e(TAG, ex.toString());
        // }
        // return true;
            case 0:
                if (UserHandle.myUserId() != UserHandle.USER_OWNER) {
                    Toast.makeText(SgpsActivity.this, R.string.not_support_visitor_or_user_mode,
                            Toast.LENGTH_SHORT).show();
                    return false;
                }
                try {
                    Intent intent = new Intent();
                    /* SPRD: modify for 306833 @{ */
                    /*
                     * intent.setAction("android.intent.action.MAIN");
                     * intent.setClassName("com.android.settings",
                     * "com.sprd.settings.LocationAgpsLogShow");
                     * intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK); startActivity(intent);
                     */
                    intent.setAction("android.settings.AGPS_LOG_SHOW");
                    startActivity(intent);
                    /* @} */
                } catch (Exception ex) {
                    Log.e(TAG, ex.toString());
                }
                return true;
            default:
                break;
        }
        return false;
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        ProgressDialog dialog = null;
        if (DIALOG_WAITING_FOR_STOP == id) {
            dialog = new ProgressDialog(this);
            dialog.setTitle(R.string.dialog_title_stop);
            dialog.setMessage(getString(R.string.dialog_message_stop));
            dialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
            dialog.setCancelable(false);
            dialog.setProgress(0);
        } else {
            dialog = new ProgressDialog(this);
            dialog.setTitle(R.string.dialog_title_error);
            dialog.setMessage(getString(R.string.dialog_message_error));
        }
        return dialog;
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "Enter onResume function");
        startGPS();
        TextView tvProvider = (TextView) findViewById(R.id.tv_provider);
        tvProvider.setText(mProvider);
        TextView tvStatus = (TextView) findViewById(R.id.tv_status);
        tvStatus.setText(mStatus);
        //add new feature for gps download bin by ansel.li
        int gnssMode = getGNSSMode();
        Log.d(TAG, "gnssMode is " + gnssMode);
        mRadioBtnGps.setChecked(gnssMode == GPS_ONLY ? true : false);
        mRadioBtnGlonass.setChecked(gnssMode == GLONASS_ONLY ? true : false);
        mRadioBtnBDS.setChecked(gnssMode == BDS_ONLY ? true : false);
        mRadioBtnGlonassGps.setChecked(gnssMode == GLONASS_GPS ? true : false);
        mRadioBtnGpsBDS.setChecked(gnssMode == GPS_BDS ? true : false);
//        setCommandToProvider(gnssMode);
        /* SPRD: fix bug336487 Nmea view dump @{*/
        if (mStartNmeaRecord && mViewLogTab) {
            ViewLogTab = true;
        }
        /* @}*/
        if (mSpreadOrBitSwitch != null) {
            mSpreadOrBitSwitch.setOnCheckedChangeListener(null);
            String value = getConfigration(SgpsUtils.GPS_CSR_CONFIG_FIL_FOR_GE2, "SPREADORBIT-ENABLE");
            Log.d(TAG, "onResume set mSpreadOrBitSwitch state SPREADORBIT-ENABLE is " + value);
            if ("TRUE".equals(value)) {
                mSpreadOrBitSwitch.setChecked(true);
            } else {
                mSpreadOrBitSwitch.setChecked(false);
            }
            mSpreadOrBitSwitch.setOnCheckedChangeListener(mSwitchCheckedChangeListener);
        }
    }

    private void showGPSVersion() {
        Log.d(TAG, "Enter show GPS version");
        if (mIsExit) {
            return;
        }
        TextView tvGPSVersion = (TextView) findViewById(R.id.tv_gps_version);
        // SPRD:add ScrollingBar
        tvGPSVersion.setMovementMethod(ScrollingMovementMethod.getInstance());

        String gpsVersion = SgpsUtils.getGPSVersionInfo();
        Log.d(TAG, "GPS Version ->" + gpsVersion);
        tvGPSVersion.setText(gpsVersion);

        // tvChipVersion.setText(GpsMnlSetting
        // .getChipVersion(getString(R.string.gps_status_unknown)));

    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.d(TAG, "Enter onStop function");
        Log.d(TAG_BG, "mbRunInBG " + mIsRunInBg);
        if (!mIsRunInBg) {
            mLocationManager.removeUpdates(mLocListener);
            Log.v(TAG, "removeUpdates(mLocListener)");
            mLocationManager.removeGpsStatusListener(mGpsListener);
        }
        mSgpsWakeLock.release();
        if (mPrompt != null) {
            mPrompt.cancel();
        }
        if (mStatusPrompt != null) {
            mStatusPrompt.cancel();
            // Toast.makeText(this, "onStop", Toast.LENGTH_SHORT).show();
        }
    }

    private boolean isGpsOpen() {
        LocationManager locationManager = (LocationManager) this.getApplicationContext()
                .getSystemService(Context.LOCATION_SERVICE);
        // Weather gps is open or not
        boolean gpsStatus = locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER);
        return gpsStatus;
    }

    protected void startGPS() {
        Log.d(TAG_GPS, "GpsStatus->" + isGpsOpen());
        if (!isGpsOpen()) {
            gpsBuilder.setPositiveButton("Yes", new DialogInterface.OnClickListener() {

                @Override
                public void onClick(DialogInterface dialog, int which) {
                    Intent gpsIntent = new Intent();
                    gpsIntent.setClassName("com.android.settings",
                            "com.android.settings.Settings$LocationSettingsActivity");
                    startActivity(gpsIntent);
                    if (mGpsConfigFile == null){
                        finish();
                        Log.d(TAG, "need restart activity to init data from mGpsConfigFile !");
                    } else {
                        Log.d(TAG, "mGpsConfigFile " + mGpsConfigFile + " exist !");
                    }
                }

            });
            /*
             * gpsBuilder.setNegativeButton("No", new
             * DialogInterface.OnClickListener() {
             * @Override public void onClick(DialogInterface dialog, int which)
             * { } });
             */
            Resources res = getResources();
            alertDialog = gpsBuilder.create();
            alertDialog.setTitle(res.getString(R.string.start_gps_dialog_title));
            alertDialog.setMessage((mGpsConfigFile != null) ? res
                    .getString(R.string.start_gps_dialog_message) : res
                    .getString(R.string.start_gps_dialog_message_need_restart));
            alertDialog.setCancelable(false);
            alertDialog.show();
        } else {
            showGPSVersion();
        }
    }
// add new feature for download Gps bin by ansel.li
    protected void reStartGPS() {
        Log.d(TAG_GPS, "enter restart gps");
            gpsBuilder.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    Intent gpsIntent = new Intent();
                    gpsIntent.setClassName("com.android.settings",
                            "com.android.settings.Settings$LocationSettingsActivity");
                    startActivity(gpsIntent);
                }

            });
            Resources res = getResources();
            alertDialog = gpsBuilder.create();
            alertDialog.setTitle(res.getString(R.string.start_gps_dialog_title));
            alertDialog.setMessage(res.getString(R.string.restart_gps_dialog_message));
            alertDialog.setCancelable(false);
            alertDialog.show();
    }
    @Override
    protected void onRestart() {
        Log.d(TAG, "Enter onRestart function");
        Log.d(TAG_BG, "mbRunInBG " + mIsRunInBg);
        if (!mIsRunInBg) {
            mFirstFix = false;
            if (mLocationManager
                    .isProviderEnabled(LocationManager.GPS_PROVIDER)) {
                mProvider = String.format(getString(
                        R.string.provider_status_enabled,
                        LocationManager.GPS_PROVIDER));
            } else {
                mProvider = String.format(getString(
                        R.string.provider_status_disabled,
                        LocationManager.GPS_PROVIDER));
            }
            mStatus = getString(R.string.gps_status_unknown);
            mLocationManager.requestLocationUpdates(
                    LocationManager.GPS_PROVIDER, 0, 0, mLocListener);
            Log.v(TAG, "request(mLocListener)");
            mLocationManager.addGpsStatusListener(mGpsListener);
        }
        // TextView first_longitude = (TextView)
        // findViewById(R.id.first_longtitude_text);
        // if (first_longitude != null) {
        // first_longitude.setText("");
        // }
        // TextView first_latitude = (TextView)
        // findViewById(R.id.first_latitude_text);
        // if (first_latitude != null) {
        // first_latitude.setText("");
        // }
        // mSGPSWakeLock = new SGPSWakeLock();
        if (null != mSgpsWakeLock) {
            mSgpsWakeLock.acquireScreenWakeLock(this);
        } else {
            Log.d(TAG, "mSGPSWakeLock is null");
        }
        super.onRestart();
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "enter onDestroy function");
        if (mLocListener != null) {
            mLocationManager.removeUpdates(mLocListener);
            Log.v(TAG, "removeUpdates(mLocListener)");
        }
        mLocationManager.removeGpsStatusListener(mGpsListener);
        mLocationManager.removeNmeaListener(mNmeaListener);
        mHandler.removeMessages(HANDLE_UPDATE_RESULT);
        mHandler.removeMessages(HANDLE_COUNTER);
        mHandler.removeMessages(HANDLE_CHECK_SATEREPORT);
        mIsExit = true;
        if (mOutputNMEALog != null) {
            finishSavingNMEALog();
        }
        Intent it = new Intent(SgpsActivity.this,SgpsService.class);
        getBaseContext().stopService(it);
        Log.d(TAG, "STOP service");
        unregisterReceiver(mPowerKeyReceiver);
        Log.v(TAG, "unregisterReceiver powerKeyReceiver");
        mSocketClient.endClient();
        final SharedPreferences preferences = this.getSharedPreferences(
                FIRST_TIME, android.content.Context.MODE_PRIVATE);
        String ss = preferences.getString(FIRST_TIME, null);
        if (ss != null && ss.equals(GpsMnlSetting.PROP_VALUE_2)) {
            GpsMnlSetting.setMnlProp(GpsMnlSetting.KEY_TEST_MODE,
                    GpsMnlSetting.PROP_VALUE_0);
        }
        super.onDestroy();
    }

    public final LocationListener mLocListener = new LocationListener() {

        // @Override
        public void onLocationChanged(Location location) {
            Log.d(TAG, "Enter onLocationChanged function");
            if (!mFirstFix) {
                Log.w(TAG, "mFirstFix is false, onLocationChanged");
                if (null != mLastStatus) {// get prefix ttff
                    int ttff = mLastStatus.getTimeToFirstFix();
                    mHandler.removeMessages(HANDLE_COUNTER);
                    mTtffValue = ttff;
                    mFirstFix = true;
                    TextView tvTtff = (TextView) findViewById(R.id.tv_ttff);
                    tvTtff.setText(mTtffValue
                            + getString(R.string.time_unit_ms));
                    tvTtff.setTextColor(Color.GREEN);
                }
            } else {
                Log.d(TAG,
                        "Time stamp[onLocationChanged]->"
                                + new Date().getTime());
            }
            if (mShowLoc) {
                String str = null;
                String tmp = null;
                Date da = null;

                da = new Date(location.getTime());
                str = da.toString() + "\n";
                tmp = String.valueOf(location.getLatitude());
                if (tmp.length() > LOCATION_MAX_LENGTH) {
                    tmp = tmp.substring(0, LOCATION_MAX_LENGTH);
                }
                str += tmp + ",";
                tmp = String.valueOf(location.getLongitude());
                if (tmp.length() > LOCATION_MAX_LENGTH) {
                    tmp = tmp.substring(0, LOCATION_MAX_LENGTH);
                }
                str += tmp;
                if (mPrompt == null) {
                    mPrompt = Toast.makeText(SgpsActivity.this, str,
                            Toast.LENGTH_SHORT);
                    mPrompt.setGravity(Gravity.BOTTOM, 0, GRAVITE_Y_OFFSET);
                } else {
                    mPrompt.setText(str);
                }
                mPrompt.show();
                da = null;
            }
            Date d = new Date(location.getTime());
            String date = String.format("%s %+02d %04d/%02d/%02d", "GMT",
                    d.getTimezoneOffset(), d.getYear() + YEAR_START,
                    d.getMonth() + 1, d.getDate());
            String time = String.format("%02d:%02d:%02d", d.getHours(),
                    d.getMinutes(), d.getSeconds());

            TextView tvTime = (TextView) findViewById(R.id.tv_time);
            if (tvTime != null) {
                tvTime.setText(time);
            }

            TextView tvDate = (TextView) findViewById(R.id.tv_date);
            tvDate.setText(date);

            if (mShowFirstFixLocate) {
                mShowFirstFixLocate = false;
                TextView firstLon = (TextView) findViewById(R.id.first_longtitude_text);
                firstLon.setText(String.valueOf(location.getLongitude()));
                TextView firstLat = (TextView) findViewById(R.id.first_latitude_text);
                firstLat.setText(String.valueOf(location.getLatitude()));
            }

            if (getmIsAutoTransferTestRunning()) {
                if (locationWhenFirstFix) {
                    mFirstFixFlag = true;
                    locationWhenFirstFix = false;
                    float distance = 0;
                    distance = location.distanceTo(mLastLocationRefence);
                    Log.d(TAG, "location la is " + location.getLatitude()
                            + " , lo is " + location.getLongitude() + " , distance is " + distance
                            + ", mDistanceCont is " + mDistanceCont + "\n");
                    mDistance[mDistanceCont] = distance;
                    mFirstFixLatitude[mDistanceCont] = location.getLatitude();
                    mFirstFixLongitude[mDistanceCont] = location.getLongitude();
                    mDistanceCont++;
                }
            }
            TextView tvLat = (TextView) findViewById(R.id.tv_latitude);
            tvLat.setText(String.valueOf(location.getLatitude()));
            TextView tvLon = (TextView) findViewById(R.id.tv_longitude);
            tvLon.setText(String.valueOf(location.getLongitude()));
            TextView tvAlt = (TextView) findViewById(R.id.tv_altitude);
            tvAlt.setText(String.valueOf(location.getAltitude()));
            TextView tvAcc = (TextView) findViewById(R.id.tv_accuracy);
            tvAcc.setText(String.valueOf(location.getAccuracy()));
            TextView tvBear = (TextView) findViewById(R.id.tv_bearing);
            tvBear.setText(String.valueOf(location.getBearing()));
            TextView tvSpeed = (TextView) findViewById(R.id.tv_speed);
            tvSpeed.setText(String.valueOf(location.getSpeed()));
            String str_item ="";
            if (mLastStatus != null && mLastStatus.getTimeToFirstFix() >= 60000) {
                mAverageCn0TextView.setText("N/A");
            } else if ( mFirstFix && !mEnterCn0FirstFlag && !getmIsAutoTransferTestRunning()) {
                for (int i =0; i < mPrnsForCN0.length; i++) {
                    str_item += mPrnsForCN0[i] + ",";
                    if (mContForCN0[i] != 0) {
                        str_item += (int)mSrnsForCN0[i]/mContForCN0[i];
                    }
                    str_item += "  ";
                }
                mAverageCn0TextView.setText(str_item);
            }
            if (mLastLocation != null) {
                TextView tvDist = (TextView) findViewById(R.id.tv_distance);
                tvDist.setText(String.valueOf(location
                        .distanceTo(mLastLocation)));
            }

            TextView tvTtff = (TextView) findViewById(R.id.tv_ttff);
            tvTtff.setText(mTtffValue + getString(R.string.time_unit_ms));
            tvTtff.setTextColor(Color.GREEN);

            // TextView txt_test_ttff =
            // (TextView)findViewById(R.id.txt_test_ttff);
            // txt_test_ttff.setText(mTTFF+
            // getString(R.string.time_unit_ms+txt_padding);

            TextView tvProvider = (TextView) findViewById(R.id.tv_provider);
            tvProvider.setText(mProvider);
            TextView tvStatus = (TextView) findViewById(R.id.tv_status);
            tvStatus.setText(mStatus);
            d = null;
            mLastLocation = location;
        }

        // @Override
        public void onProviderDisabled(String provider) {
            Log.v(TAG, "Enter onProviderDisabled function");
            mProvider = String.format(getString(
                    R.string.provider_status_disabled,
                    LocationManager.GPS_PROVIDER));
            TextView tvProvider = (TextView) findViewById(R.id.tv_provider);
            tvProvider.setText(mProvider);
        }

        // @Override
        public void onProviderEnabled(String provider) {
            Log.v(TAG, "Enter onProviderEnabled function");
            mProvider = String.format(getString(
                    R.string.provider_status_enabled,
                    LocationManager.GPS_PROVIDER));
            TextView tvProvider = (TextView) findViewById(R.id.tv_provider);
            tvProvider.setText(mProvider);
            mTtffValue = 0;
        }

        // @Override
        public void onStatusChanged(String provider, int status, Bundle extras) {
            Log.v(TAG, "Enter onStatusChanged function");
        }
    };

    public final GpsStatus.Listener mGpsListener = new GpsStatus.Listener() {
        private void onFirstFix(int ttff) {
            Log.d(TAG, "Time stamp[onFirstFix]->" + new Date().getTime());
            Log.v(TAG, "Enter onFirstFix function: ttff = " + ttff);
            int currentTimes = mCurrentTimes;
            mHandler.removeMessages(HANDLE_COUNTER);
            mTtffValue = ttff;
            if (ttff != mTtffValue) {
                Log.w(TAG, "ttff != mTTFF");
                mTtffValue = ttff;
            }
            mFirstFix = true;
            Toast.makeText(
                    SgpsActivity.this,
                    String.format(getString(R.string.toast_first_fix), ttff,
                            getString(R.string.time_unit_ms)),
                    Toast.LENGTH_LONG).show();
            TextView tvTtff = (TextView) findViewById(R.id.tv_ttff);
            tvTtff.setText(mTtffValue + getString(R.string.time_unit_ms));
            tvTtff.setTextColor(Color.GREEN);

            if (getmIsTestRunning()) {
//                TextView tvLastTtff = (TextView) findViewById(R.id.tv_last_ttff);
//                tvLastTtff.setText(mTtffValue
//                        + getString(R.string.time_unit_ms));

                mMeanTTFF = meanTTFF(currentTimes);
//                ((TextView) findViewById(R.id.tv_mean_ttff)).setText(Float
//                        .valueOf(mMeanTTFF).toString()
//                        + getString(R.string.time_unit_ms));
                mLastTtffValue = mTtffValue;
            }

            if(getmIsAutoTransferTestRunning()){
                mLastTtffValue = mTtffValue;
            }
        }

        private void onPreFix(int ttff) {
            Log.d(TAG, "Enter onPreFix function: ttff = " + ttff);
            int currentTimes = mCurrentTimes;
            mHandler.removeMessages(HANDLE_COUNTER);
            mTtffValue = ttff;
            mFirstFix = true;
            TextView tvTtff = (TextView) findViewById(R.id.tv_ttff);
            tvTtff.setText(mTtffValue + getString(R.string.time_unit_ms));
            tvTtff.setTextColor(Color.GREEN);
        }

        private boolean isLocationFixed(Iterable<GpsSatellite> list) {
            boolean fixed = false;
            synchronized (this) {
                for (GpsSatellite sate : list) {
                    if (sate.usedInFix()) {
                        fixed = true;
                        break;
                    }
                }
            }
            return fixed;
        }

        private void setSatelliteInusedOrTracking(Iterable<GpsSatellite> list) {
            if (list != null) {
                Log.d(TAG, "setSatelliteInusedOrTracking");
                int totalInUsedNum = 0;
                int totalViewNum = 0;
                int gpsInUsedNum = 0;
                int gpsViewNum = 0;
                int glonassInUsedNum = 0;
                int glonassViewNum = 0;
                int beidouInUsedNum = 0;
                int beidouViewNum = 0;
                int trackingNum = 0;
                for (GpsSatellite sate : list) {
                    int prn = sate.getPrn();
                    totalViewNum++;
                    if (prn >= 1 && prn <= 32) {
                        gpsViewNum++;
                    } else if (prn >= 65 && prn <= 92) {
                        glonassViewNum++;
                    } else if (prn >= 151 && prn <= 187) {
                        beidouViewNum++;
                    } else {
                    }
                    if (!isUsedInFix(0)) {
                        // do nothing
                    } else if (isUsedInFix(prn)) {
                        totalInUsedNum++;
                        if (prn >= 1 && prn <= 32) {
                            gpsInUsedNum++;
                        } else if (prn >= 65 && prn <= 92) {
                            glonassInUsedNum++;
                        } else if (prn >= 151 && prn <= 187) {
                            beidouInUsedNum++;
                        } else {
                        }
                    } else {
                        trackingNum++;
                    }
                }
                Log.d(TAG, "setSatelliteInusedOrTracking totalInUsedNum is " + totalInUsedNum
                        + " , totalViewNum is " + totalViewNum + ", gpsInUsedNum is "
                        + gpsInUsedNum + " , gpsViewNum is " + gpsViewNum + " , glonassInUsedNum is "
                        + glonassInUsedNum + " , glonassViewNum is " + glonassViewNum + " , beidouInUsedNum is "
                        + beidouInUsedNum + " , beidouViewNum is " + beidouViewNum);
                getTotalInUsedMinMaxValue(totalInUsedNum);
                getTotalViewMinMaxValue(totalViewNum);
                getGpsInUsedMinMaxValue(gpsInUsedNum);
                getGpsViewMinMaxValue(gpsViewNum);
                getGlonassInUsedMinMaxValue(glonassInUsedNum);
                getGlonassViewMinMaxValue(glonassViewNum);
                getBeidouInUsedMinMaxValue(beidouInUsedNum);
                getBeidouViewMinMaxValue(beidouViewNum);
                mTotalInused[mSatelliteTestCont] = totalInUsedNum;
                mTotalView[mSatelliteTestCont] = totalViewNum;
                mGpsInUsed[mSatelliteTestCont] = gpsInUsedNum;
                mGpsView[mSatelliteTestCont] = gpsViewNum;
                mGlonassInUsed[mSatelliteTestCont] = glonassInUsedNum;
                mGlonassView[mSatelliteTestCont] = glonassViewNum;
                mBeidouInUsed[mSatelliteTestCont] = beidouInUsedNum;
                mBeidouView[mSatelliteTestCont] = beidouViewNum;
                mSateTracking[mSateTrackingCont] = trackingNum;
                if (mSatelliteTestCont < mCurrentTimes) {
                    mSatelliteTestCont++;
                    mSateTrackingCont++;
                }
                Log.d(TAG, "setSatelliteInusedOrTracking mSatelliteTestCont is "
                        + mSatelliteTestCont + " , mCurrentTimes is " + mCurrentTimes);
            }
            mFirstFixFlag = false;
        }

        private void getTotalInUsedMinMaxValue(int input) {
            if (input < mTotalInUsedMin) {
                mTotalInUsedMin = input;
            }
            if (input > mTotalInUsedMax) {
                mTotalInUsedMax = input;
            }
        }

        private void getTotalViewMinMaxValue(int input) {
            if (input < mTotalViewMin) {
                mTotalViewMin = input;
            }
            if (input > mTotalViewMax) {
                mTotalViewMax = input;
            }
        }

        private void getGpsInUsedMinMaxValue(int input) {
            if (input < mGpsInUsedMin) {
                mGpsInUsedMin = input;
            }
            if (input > mGpsInUsedMax) {
                mGpsInUsedMax = input;
            }
        }

        private void getGpsViewMinMaxValue(int input) {
            if (input < mGpsViewMin) {
                mGpsViewMin = input;
            }
            if (input > mGpsViewMax) {
                mGpsViewMax = input;
            }
        }

        private void getGlonassInUsedMinMaxValue(int input) {
            if (input < mGlonassInUsedMin) {
                mGlonassInUsedMin = input;
            }
            if (input > mGlonassInUsedMax) {
                mGlonassInUsedMax = input;
            }
        }

        private void getGlonassViewMinMaxValue(int input) {
            if (input < mGlonassViewMin) {
                mGlonassViewMin = input;
            }
            if (input > mGlonassViewMax) {
                mGlonassViewMax = input;
            }
        }

        private void getBeidouInUsedMinMaxValue(int input) {
            if (input < mBeidouInUsedMin) {
                mBeidouInUsedMin = input;
            }
            if (input > mBeidouInUsedMax) {
                mBeidouInUsedMax = input;
            }
        }

        private void getBeidouViewMinMaxValue(int input) {
            if (input < mBeidouViewMin) {
                mBeidouViewMin = input;
            }
            if (input > mBeidouViewMax) {
                mBeidouViewMax = input;
            }
        }

        private void satelliteStateCN0(Iterable<GpsSatellite> list) {

            ArrayList<GpsSatellite> satelliteList=new ArrayList<GpsSatellite>();
            for(GpsSatellite sate : list) {
                satelliteList.add(sate);
            }
            if (mEnterCn0FirstFlag) {
                mEnterCn0FirstFlag = false;
                for(int i = 0; i < satelliteList.size(); i++) {
                    for (int j = 0; j < satelliteList.size() - i - 1; j++) {
                        if (satelliteList.get(j).getSnr() < satelliteList.get(j+1).getSnr()) {
                            GpsSatellite temp = satelliteList.get(j);
                            satelliteList.set(j, satelliteList.get(j+1));
                            satelliteList.set(j+1, temp);
                        }
                    }
                }
                if (satelliteList.size() < 5) {
                    for(int i = 0; i < satelliteList.size(); i++) {
                        mPrnsForCN0[i] = satelliteList.get(i).getPrn();
                        mSrnsForCN0[i] = satelliteList.get(i).getSnr();
                        mContForCN0[i]++;
                    }
                } else {
                    for(int i = 0; i < 5; i++) {
                        mPrnsForCN0[i] = satelliteList.get(i).getPrn();
                        mSrnsForCN0[i] = satelliteList.get(i).getSnr();
                        mContForCN0[i]++;
                    }
                }
            } else {
                for(int i=0;i<satelliteList.size();i++) {
                    for(int j = 0; j < mPrnsForCN0.length; j++) {
                        if (mPrnsForCN0[j] == satelliteList.get(i).getPrn() && satelliteList.get(i).getSnr() != 0) {
                            mSrnsForCN0[j] += satelliteList.get(i).getSnr();
                            mContForCN0[j]++;
                        }
                    }
                }
            }
        }
        public void onGpsStatusChanged(int event) {
            Log.v(TAG, "Enter onGpsStatusChanged function");
            GpsStatus status = mLocationManager.getGpsStatus(null);
            switch (event) {
                case GpsStatus.GPS_EVENT_STARTED:
                    Log.d(TAG, "Time stamp[GPS_EVENT_STARTED]->" + new Date().getTime());
                    mStartSerchTime = new Date().getTime();
                    Log.d(TAG, "mStartSerchTime  is " + mStartSerchTime);
                    if (getmIsAutoTransferTestRunning()) {
                        mFirstFixFlag = false;
                        locationWhenFirstFix = true;
                        Log.d(TAG, "locationWhenFirstFix is " + locationWhenFirstFix);
                    }
                    mSerchFirstSateFlag = true;
                    mEnterCn0FirstFlag = true;
                    mStatus = getString(R.string.gps_status_started);
                    break;
                case GpsStatus.GPS_EVENT_STOPPED:
                    Log.d(TAG, "GPS_EVENT_STOPPED");
                    mStatus = getString(R.string.gps_status_stopped);
                    break;
                case GpsStatus.GPS_EVENT_FIRST_FIX:
                    onFirstFix(status.getTimeToFirstFix());
                    mStatus = getString(R.string.gps_status_first_fix);
                    break;
                case GpsStatus.GPS_EVENT_SATELLITE_STATUS:
                    Log.d(TAG, "GPS_EVENT_SATELLITE_STATUS is receive!!");
                    if (mSerchFirstSateFlag && getmIsAutoTransferTestRunning()) {
                        mSerchFirstSateFlag = false;
                        mSerchFirstSateTime = new Date().getTime();
                        Log.d(TAG, "mSerchFirstSateTime  is " + mSerchFirstSateTime);
                    }
                    long time = new Date().getTime();
                    if (mFirstFix && status.getTimeToFirstFix() <= 60000 && (time - mStartSerchTime) >= 61000 && (time - mStartSerchTime) <= 180000 && !getmIsAutoTransferTestRunning()) {
                        satelliteStateCN0(status.getSatellites());
                    }
                    mSateReportTimeOut = 0;
                    if (!GPS_CHIP_TYPE_IS_GE2) {
                        setSatelliteStatus(status.getSatellites());
                    }
                    if (mFirstFixFlag) {
                        setSatelliteInusedOrTracking(status.getSatellites());
                    }
                    if (!GPS_CHIP_TYPE_IS_GE2) {
                        if (!isLocationFixed(status.getSatellites())) {
                            clearLayout();
                            mStatus = getString(R.string.gps_status_unavailable);
                        } else {
                            mStatus = getString(R.string.gps_status_available);
                            if (!mFirstFix) {
                                // GPS has been fixed before entry SGPS.
                                // SGPS will not receive GPS_EVENT_FIRST_FIX.

                                // how to deal with we don't need this.
                                // onPreFix(status.getTimeToFirstFix());
                            }
                        }
                    }
                    if (!mIsShowVersion) {
                        // showGPSVersion();
                    }
                    mLastStatus = status;
                    break;
                default:
                    break;
            }
            TextView tvStatus = (TextView) findViewById(R.id.tv_status);
            tvStatus.setText(mStatus);
            Log.v(TAG, "onGpsStatusChanged:" + event + " Status:" + mStatus);
        }
    };

    // Change the interval
    private int modifyCountDown(int count) {
        int array[] = {
                -15, 15, -10, 10, -5, 5, 0
        };
        count += array[mCurrentTimes % 7];
        if (count < 0)
            count = 0;
        return count;
    }

    private void resetParam(Bundle extras, boolean bAutoConnectTest,boolean hasNextTest) {
        Log.d(TAG, "Enter resetParam function hasNextTest is " + hasNextTest);
        try {
            if (bAutoConnectTest && (mTestInterval != 0 || mAutoTransferTestInterval != 0)) {
                if(0 != mTestInterval) {
                    for (int i = modifyCountDown(mTestInterval); i >= 0
                            && getmIsTestRunning(); --i) {
                        setCountDown(i);
                        Thread.sleep(1 * ONE_SECOND);
                    }
                } else {
                    for (int i = modifyCountDown(mAutoTransferTestInterval); i >= 0
                            && getmIsAutoTransferTestRunning(); --i) {
                        setCountDown(i);
                        Thread.sleep(1 * ONE_SECOND);
                    }
                }

                Thread.sleep(1 * ONE_SECOND);

                /* spreadst: add mode modify logical. @{ */
               /*// if (!getmIsTestRunning()) {
               if(!getmIsAutoTransferTestRunning()) {
                    setCountDown(0);
                    // save start autotest log
                    if (mStopPressedHandling == false) {
                        mHandler.sendEmptyMessage(HANDLE_COMMAND_OTHERS_UPDATE_RESULT_HINT);
                    }
                } else {
                    // save one record
                    mLastLocationRecord = mLastLocation;
                    mLastStatusRecord = mLastStatus;
                    mHandler.sendEmptyMessage(HANDLE_COMMAND_OTHERS_UPDATE_RESULT_LOG);
                } */

                if((getmIsAutoTransferTestRunning()&& mIsCompleted == true) || getmIsTestRunning()) {
                    // save log record
                    mLastLocationRecord = mLastLocation;
                    mLastStatusRecord = mLastStatus;
                    Log.d(TAG, "resetParam save record log");
                    mHandler.sendEmptyMessage(HANDLE_COMMAND_OTHERS_UPDATE_RESULT_LOG);
                } else {
                    setCountDown(0);
                    // create autotest log
                    if (mStopPressedHandling == false || mStopPressedAutoTransferHandling == false) {
                        Log.d(TAG, "resetParam start autotest log");
                        mHandler.sendEmptyMessage(HANDLE_COMMAND_OTHERS_UPDATE_RESULT_HINT);
                    }
                }
                /* @} */
            }
        } catch (InterruptedException e) {
            Log.d(TAG, "resetParam InterruptedException: " + e.getMessage());
        }

        mLocationManager.removeUpdates(mLocListener);
        Log.d(TAG, "removeUpdates(mLocListener)");
        try {
            Thread.sleep(ONE_SECOND * 4);
        } catch (InterruptedException e) {
            Log.d(TAG, "resetParam InterruptedException: " + e.getMessage());
        }
        if (hasNextTest){
            mLocationManager.sendExtraCommand(LocationManager.GPS_PROVIDER,
                    "delete_aiding_data", extras);
        }

        mFirstFix = false;
        mTtffValue = 0;

        if (!mIsExit && hasNextTest) {
            mHandler.sendEmptyMessage(HANDLE_COMMAND_OTHERS_UPDATE_PROVIDER);
            Log.d(TAG, "request(mLocListener) in main thread");
        }
        // reset autotest start button
        if (!bAutoConnectTest && !mBtnGpsTestStart.isEnabled()) {
            clearLayout();
            setStartButtonEnable(true);
            removeDialog(DIALOG_WAITING_FOR_STOP);
            mStopPressedHandling = false;
            mStartPressedHandling = false;
        }

        if(!bAutoConnectTest && !mBtnGpsTestAutoTransferStart.isEnabled()) {
            Log.d(TAG, "resetParam end bAutoConnectTest = " + bAutoConnectTest);
            removeDialog(DIALOG_WAITING_FOR_STOP);
            mStartPressedAutoTransferHandling = false;
            mStopPressedAutoTransferHandling = false;
        }

        if (!mHandler.hasMessages(HANDLE_COUNTER)) {
            mHandler.sendEmptyMessage(HANDLE_COUNTER);
        }
    }

    private boolean gpsTestRunning() {
        if (getmIsTestRunning() || getmIsAutoTransferTestRunning()) {
            Toast.makeText(this, R.string.gps_test_running_warn,
                    Toast.LENGTH_LONG).show();
            return true;
        }
        return false;
    }

    private boolean createFileForSavingAutoTestLog() {
        Log.d(TAG, "Enter createFileForSavingAutoTestLog function");
        if (AUTOTESTLOG_SD) {
            if (!(Environment.getExternalStorageState()
                    .equals(Environment.MEDIA_MOUNTED))) {
                Log.d(TAG, "saveAUTOTESTLog function: No SD card");
                Toast.makeText(this, R.string.no_sdcard, Toast.LENGTH_LONG)
                        .show();
                return false;
            }
        }

        String strTime = DATE_FORMAT
                .format(new Date(System.currentTimeMillis()));
        File file = null;
        if (AUTOTESTLOG_SD) {
            if(getmIsAutoTransferTestRunning() || mIsAutoTransferMode) {
                file = new File(Environment.getExternalStorageDirectory(),
                        AUTOTEST_LOG_PREX + getCurrentMode() + strTime + AUTOTEST_LOG_SUFX);
            } else {
                file = new File(Environment.getExternalStorageDirectory(),
                        AUTOTEST_LOG_PREX + strTime + AUTOTEST_LOG_SUFX);
            }
        } else {
            File nmeaPath = new File(AUTOTESTLOG_PATH);
            if (!nmeaPath.exists()) {
                nmeaPath.mkdirs();
            }

            if(getmIsAutoTransferTestRunning() || mIsAutoTransferMode) {
                file = new File(nmeaPath, AUTOTEST_LOG_PREX + getCurrentMode() + strTime
                        + AUTOTEST_LOG_SUFX);
            } else {
                file = new File(nmeaPath, AUTOTEST_LOG_PREX + strTime
                        + AUTOTEST_LOG_SUFX);
            }
        }
        if (file != null) {
            try {
                file.createNewFile();
            } catch (IOException e) {
                Log.w(TAG, "create new file failed!");
                Toast.makeText(this, R.string.toast_create_file_failed,
                        Toast.LENGTH_LONG).show();
                return false;
            }
        }
        try {
            mOutputAUTOTESTLog = new FileOutputStream(file);
        } catch (FileNotFoundException e1) {
            Log.w(TAG,
                    "output stream FileNotFoundException: " + e1.getMessage());
            return false;
        }
        return true;
    }

    private void saveAUTOTESTLog(String autotest) {
        boolean bSaved = true;
        if (null == mOutputAUTOTESTLog)
            return;
        try {
            mOutputAUTOTESTLog.write(autotest.getBytes(), 0,
                    autotest.getBytes().length);
            mOutputAUTOTESTLog.flush();
        } catch (IOException e) {
            bSaved = false;
            Log.d(TAG, "write AUTOTEST log to file failed!");
        } finally {
            if (!bSaved) {
                finishSavingAUTOTESTLog();
                Toast.makeText(this, "Please check your SD card",
                        Toast.LENGTH_LONG).show();
            }
        }

    }

    private void finishSavingAUTOTESTLog() {
        if (null == mOutputAUTOTESTLog)
            return;
        try {
            mOutputAUTOTESTLog.close();
            mOutputAUTOTESTLog = null;
            Toast.makeText(
                    this,
                    String.format(
                            getString(R.string.toast_autotestlog_save_at),
                            AUTOTESTLOG_SD ? Environment
                                    .getExternalStorageDirectory()
                                    .getAbsolutePath() : AUTOTESTLOG_PATH),
                    Toast.LENGTH_LONG).show();
        } catch (IOException e) {
            Log.w(TAG, "Close file failed!");
        }
    }

    private String getSatelliteStatus(Iterable<GpsSatellite> list) {
        Log.d(TAG, "Enter private getSatelliteStatus function");
        int[] mPrns_temp = new int[MAX_SATELLITES_NUMBER];
        float[] mSnrs_temp = new float[MAX_SATELLITES_NUMBER];
        if (null == list) {
            return "0";
        } else {
            int index = 0;
            for (GpsSatellite sate : list) {
                mPrns_temp[index] = sate.getPrn();
                mSnrs_temp[index] = sate.getSnr();
                index++;
            }
            return toString(mPrns_temp, index) + ","
                    + toString(mSnrs_temp, index);
        }
    }

    private void perpareGpsMode(int start_mode) {
        Bundle extras = new Bundle();
        Bundletemp = extras;
        if (start_mode == HOT_START) {
            // nothing should be put
            Log.v(TAG, "Radio Hot Start is selected");
            Bundletemp.putBoolean(GPS_EXTRA_RTI, true);
        } else if (start_mode == COLD_START) {
            Log.v(TAG, "Radio Cold Start is selected");
            Bundletemp.putBoolean(GPS_EXTRA_EPHEMERIS, true);
            Bundletemp.putBoolean(GPS_EXTRA_POSITION, true);
            Bundletemp.putBoolean(GPS_EXTRA_TIME, true);
            Bundletemp.putBoolean(GPS_EXTRA_IONO, true);
            Bundletemp.putBoolean(GPS_EXTRA_UTC, true);
            Bundletemp.putBoolean(GPS_EXTRA_HEALTH, true);
        } else if (start_mode == WARM_START) {
            Log.v(TAG, "Radio Warm Start is selected");
            Bundletemp.putBoolean(GPS_EXTRA_EPHEMERIS, true);
        } else {
            Log.v(TAG, "Radio Full Start is selected");
            Bundletemp.putBoolean(GPS_EXTRA_ALL, true);
        }
        mAutoTestCurrent = start_mode;
    }

    private void StartGpsMode() {
        final SharedPreferences preferences = SgpsActivity.this
                .getSharedPreferences(START_MODE,
                        android.content.Context.MODE_PRIVATE);
        int start_mode = preferences.getInt(START_MODE, HOT_START);
        perpareGpsMode(start_mode);
    }

    /*SPRD: fix bug 360943 change checkbox to radiobutton @*/
    private void startGpsModeAuto() {
        final SharedPreferences preferences = SgpsActivity.this
                .getSharedPreferences(START_MODE_AUTO,
                        android.content.Context.MODE_PRIVATE);
        int start_mode = preferences.getInt(START_MODE_AUTO, HOT_START);
        perpareGpsMode(start_mode);
    }
    /* @}*/

    private void sendCommand(String command) {
        Log.d(TAG, "GPS Command is " + command);
        if (null == command || command.trim().length() == 0) {
            Toast.makeText(this, R.string.command_error, Toast.LENGTH_LONG)
                    .show();
            return;
        }
        String com = command;
        /*int index1 = command.indexOf(COMMAND_START);
        int index2 = command.indexOf(COMMAND_END);
        String com = command;
        if (index1 != -1 && index2 != -1) {
            if (index2 < index1) {
                Toast.makeText(this, R.string.command_error, Toast.LENGTH_LONG)
                        .show();
                return;
            }
            com = com.substring(index1 + 1, index2);
        } else if (index1 != -1) {
            com = com.substring(index1 + 1);
        } else if (index2 != -1) {
            com = com.substring(0, index2);
        }*/
        mSocketClient.sendCommand(com.trim());
    }

    public void onResponse(String res) {
        Log.d(TAG, "Enter getResponse: " + res);
        if (null == res || res.isEmpty()) {
            return;
        }
        if (res.startsWith("$PSPRD") && res.length() >= 12) {
            String[] data = parseResponseData(res);

            int type = -1;
            try {
                type = Integer.parseInt(data[3]);
            } catch (Exception e) {
                e.printStackTrace();
            }

            Message m = mHandler.obtainMessage(HANDLE_UPDATE_RESULT);
            switch (type) {
                case SOCKET_DATA_TYPE_GEOFENCE:
                    m.arg1 = HANDLE_SHOW_GEOFENCE_INFO;
                    m.obj = data;
                    break;
                default:
                    Log.d(TAG, "getResponse type " + type + " is invalid");
            }
            mHandler.sendMessage(m);
        }

        // Message m = mHandler.obtainMessage(HANDLE_UPDATE_RESULT);
        // if (res.startsWith("$PMTK705")) {
        // m.arg1 = HANDLE_COMMAND_GETVERSION;
        // } else if (res.contains("PMTK001")) {
        // m.arg1 = HANDLE_COMMAND_JAMMINGSCAN;
        // } else {
        // m.arg1 = HANDLE_COMMAND_OTHERS;
        // }
        // m.obj = res;
        // mHandler.sendMessage(m);
    }

    private String[] parseResponseData(String res) {
        String[] data = new String[5];
        data[0] = res.substring(0, 6);// TAG
        data[1] = res.substring(7, 8);// data flow
        data[2] = res.substring(8, 9);// data sequence
        data[3] = res.substring(10, 11);// type
        data[4] = res.substring(12, res.length());// info
        return data;
    }

    private void showGeofenceInfoDialog(String info) {
        Log.d(TAG, "showGeofenceInfoDialog");
        // dismiss dialog because it exist
        if (mGeofenceDialog != null) {
            mGeofenceDialog.dismiss();
            mGeofenceDialog = null;
        }

        Resources mRes = getResources();
        AlertDialog.Builder builder = new AlertDialog.Builder(SgpsActivity.this);
        builder.setTitle(mRes.getString(R.string.geofence_info_title));
        builder.setMessage(info);
        builder.setPositiveButton(com.android.internal.R.string.ok,
                new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if (mGeofenceDialog != null) {
                            mGeofenceDialog.dismiss();
                            mGeofenceDialog = null;
                        }

                    }
                });
        if (mGeofenceDialog == null) {
            mGeofenceDialog = builder.create();
        }
        if (mGeofenceDialog != null && !mGeofenceDialog.isShowing()) {
            mGeofenceDialog.setCancelable(false);
            Log.d(TAG, "mGeofenceDialog.show()");
            mGeofenceDialog.show();
        }
    }

    private void onGpsHwTestClicked() {
        String ss = GpsMnlSetting.getMnlProp(GpsMnlSetting.KEY_TEST_MODE,
                GpsMnlSetting.PROP_VALUE_0);
        if (ss.equals(GpsMnlSetting.PROP_VALUE_0)) {
            mBtnGpsHwTest.setText(R.string.btn_name_dbg2gpsdoctor_disable);
            GpsMnlSetting.setMnlProp(GpsMnlSetting.KEY_TEST_MODE,
                    GpsMnlSetting.PROP_VALUE_1);
            mBtnNmeaDbgDbgFile.setText(R.string.btn_name_dbg2file_disable);
            GpsMnlSetting.setMnlProp(GpsMnlSetting.KEY_DEBUG_DBG2FILE,
                    GpsMnlSetting.PROP_VALUE_1);
        } else {
            mBtnGpsHwTest.setText(R.string.btn_name_dbg2gpsdoctor_enable);
            GpsMnlSetting.setMnlProp(GpsMnlSetting.KEY_TEST_MODE,
                    GpsMnlSetting.PROP_VALUE_0);
        }
        final SharedPreferences preferences = this.getSharedPreferences(
                FIRST_TIME, android.content.Context.MODE_PRIVATE);
        preferences.edit().putString(FIRST_TIME, GpsMnlSetting.PROP_VALUE_1)
                .commit();
    }

    private void onGpsJammingScanClicked() {
        if (0 == mEtGpsJammingTimes.getText().length()) {
            Toast.makeText(SgpsActivity.this,
                    "Please input Jamming scan times", Toast.LENGTH_LONG)
                    .show();
            return;
        } else {
            Integer times = Integer.valueOf(mEtGpsJammingTimes.getText()
                    .toString());
            if (times <= INPUT_VALUE_MIN || times > INPUT_VALUE_MAX) {
                Toast.makeText(SgpsActivity.this, "Jamming scan times error",
                        Toast.LENGTH_LONG).show();
                return;
            }
        }
    }

    private void enableBtns(boolean bEnable) {
        mBtnHotStart.setClickable(bEnable);
        mBtnWarmStart.setClickable(bEnable);
        mBtnColdStart.setClickable(bEnable);
        mBtnFullStart.setClickable(bEnable);
        mBtnReStart.setClickable(bEnable);
        mBtnHotStill.setClickable(bEnable);

        mBtnStart.setClickable(bEnable);
        mBtnAssert.setClickable(bEnable);
    }

    private Long count_beginTime = 0l;
    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            Log.d(TAG, "handleMessage " + msg.what);
            switch (msg.what) {
                case HANDLE_COUNTER:
                    if (!mFirstFix) {
                        if (mTtffValue == 0) {
                            count_beginTime = Calendar.getInstance().getTime()
                                    .getTime();
                        }
                        mTtffValue += COUNT_PRECISION;
                        if (!getmIsTestRunning()) {
                            TextView tvTtff = (TextView) findViewById(R.id.tv_ttff);
                            // tvTtff.setText(mTtffValue % 500 == 0 ? mTtffValue
                            // + getString(R.string.time_unit_ms) : "");
                            tvTtff.setText(mTtffValue
                                    + getString(R.string.time_unit_ms));
                            // tvTtff.setText(getString(R.string.ttff_calculating));
                            tvTtff.setTextColor(Color.RED);
                        }
                        this.sendEmptyMessageDelayed(HANDLE_COUNTER,
                                COUNT_PRECISION);
                    }
                    break;
                case HANDLE_UPDATE_RESULT:
                    // String response = msg.obj.toString();
                    String[] response = (String[]) msg.obj;
                    switch (msg.arg1) {
                        case HANDLE_COMMAND_JAMMINGSCAN:
                            // if (response.contains("PMTK001,837")) {
                            // Toast.makeText(SgpsActivity.this,
                            // R.string.toast_jamming_succeed,
                            // Toast.LENGTH_LONG).show();
                            // }
                            break;
                        case HANDLE_COMMAND_GETVERSION:
                            /*
                             * if (response.startsWith("$PMTK705")) { String[] strA =
                             * response.split(","); if (strA.length >= RESPONSE_ARRAY_LENGTH) {
                             * TextView tMnlVersion = (TextView) findViewById(R.id.tv_mnl_version);
                             * if (null != tMnlVersion) { if (!tMnlVersion.getText().toString()
                             * .startsWith("MNL")) { tMnlVersion .setText(strA[RESPONSE_ARRAY_LENGTH
                             * - 1]); mIsShowVersion = true; } } else { Log.v(TAG,
                             * "txt_mnl_version is null"); } } }
                             */
                            break;
                        case HANDLE_COMMAND_OTHERS:
                            break;
                        case HANDLE_SHOW_GEOFENCE_INFO:
                            showGeofenceInfoDialog(response[4]);
                            break;
                        default:
                            break;
                    }
                    break;
                case HANDLE_CLEAR:
                    if (GPS_CHIP_TYPE_IS_GE2) {
                        updateSatelliteView(null);
                    } else {
                        setSatelliteStatus(null);
                    }
                    clearLayout();
                    enableBtns(true);
                    break;
                case HANDLE_CHECK_SATEREPORT:
                    mSateReportTimeOut++;
                    if (SATE_RATE_TIMEOUT < mSateReportTimeOut) {
                        mSateReportTimeOut = 0;
                        sendEmptyMessage(HANDLE_CLEAR);
                    }
                    sendEmptyMessageDelayed(HANDLE_CHECK_SATEREPORT, ONE_SECOND);
                    break;
                case HANDLE_COMMAND_OTHERS_UPDATE_RESULT_HINT:
                    if (null != mTvResultLog)
                        mTvResultLog.setText("");
                    if (!createFileForSavingAutoTestLog()) {
                        Log.d(TAG, "createFileForSavingAutoTestLog return false");
                        return;
                    }
                    SharedPreferences preferences = SgpsActivity.this
                            .getSharedPreferences(START_MODE,
                                    android.content.Context.MODE_PRIVATE);
                    // int start_mode = preferences.getInt(START_MODE,
                    // HOT_START);
                    String test_head = "SGPS Autotest      time : "
                            + DATE_FORMAT.format(new Date(System
                                    .currentTimeMillis())) + "  " + mTotalTimes
                            + " times " + "interval : " + mTestInterval
                            + " Mode : " + "\n";
                    if (mTestLatitude >= INPUT_GPS_VALUE_MAX
                            && mTestLongitude >= INPUT_GPS_VALUE_MAX) {
                        test_head += "Refence input Latitude&Longitude is not availiable! \n\n";
                    } else {
                        test_head += "Refence input Latitude : " + mTestLatitude
                                + " Longitude : " + mTestLongitude + "\n\n";
                    }
                    test_head += "No.      Latitude        Longitude   Distance(m)                   CN/O                        TTFF(s)         SFST(ms)        Mode\n";
                    saveAUTOTESTLog(test_head);
                    mTestMeanTTFF = 0;
                    mTestMeanDistance = 0;

                    if (null != mTvRestartMode)
                        mTvRestartMode.setText(getCurrentMode());
                    if (null != mTvResultHint)
                        mTvResultHint
                                .setText("No.      Latitude        Longitude   Distance(m)   TTFF(s)     SFST(ms)        Mode");
                    break;
                case HANDLE_COMMAND_OTHERS_UPDATE_RESULT_LOG:
                    String test_item = null;
                    if (mDistanceCont < 1) {
                        Log.d(TAG, "mDistanceCont is invalid !");
                        mTtffTimeoutFlag = true;
                        mTTFFTimeoutCont++;
                    }
                    if(!mTtffTimeoutFlag) {
                        if (null == mLastStatusRecord)
                            break;
                        mSFST = mSerchFirstSateTime - mStartSerchTime;
                        test_item = mCurrentTimes + " "
                                + mFirstFixLatitude[mDistanceCont - 1] + " "
                                + mFirstFixLongitude[mDistanceCont - 1];
                        // if input lat long is available
                        if (mTestLatitude >= INPUT_GPS_VALUE_MAX
                                || mTestLongitude >= INPUT_GPS_VALUE_MAX) {
                            test_item += " 0.0";
                        } else {
                            test_item += " " + mDistance[mDistanceCont-1];
                        }
                        Log.d(TAG, "test_item mDistanceCont[index] is "
                                + (mDistanceCont - 1) + ", info is " + test_item);
                        if (null != mLastStatusRecord)
                            if(GPS_CHIP_TYPE_IS_GE2){
                                test_item += " "
                                        + mGe2Cn_Sr;
                            } else{
                                test_item += " "
                                        + getSatelliteStatus(mLastStatusRecord
                                                .getSatellites());
                            }
                        test_item += " " + mLastTtffValue / 1000.0 + " " + mSFST + " "
                                + getCurrentMode() + "\n";
                    } else {
                        test_item = mCurrentTimes + "   N/A" + "\n";
                    }
                    // save data
                    saveAUTOTESTLog(test_item);
                    if (!mTtffTimeoutFlag && mTtff != null){
                        mTtff[cont]=mLastTtffValue;
                        cont++;
                    }
                    if (!mTtffTimeoutFlag && mCurrentTimes > 0) {
                        mTestMeanDistance = (mTestMeanDistance
                                * (mCurrentTimes - 1 - mTTFFTimeoutCont) + mDistance[mDistanceCont - 1])
                                / (mCurrentTimes - mTTFFTimeoutCont);
                        mTestMeanTTFF = (mTestMeanTTFF * (mCurrentTimes - 1 - mTTFFTimeoutCont) + mLastTtffValue)
                                / (mCurrentTimes - mTTFFTimeoutCont);
                    }
                    if(!mTtffTimeoutFlag && getmIsAutoTransferTestRunning()){
                        updateLastTTFF(mLastTtffValue);
                        updateAverageTTFF(mTestMeanTTFF);
                    }
                    // view data
                    String test_item_view = null;
                    if (!mTtffTimeoutFlag) {
                        test_item_view = mCurrentTimes + "     "
                                + ((float) mFirstFixLatitude[mDistanceCont - 1]) + "     "
                                + ((float) mFirstFixLongitude[mDistanceCont - 1]) + "      "
                                + mDistance[mDistanceCont - 1] + "      " + mLastTtffValue / 1000.0 + "     " + mSFST + "\n";
                    } else {
                        test_item_view = mCurrentTimes + "    N/A" + "\n";
                    }
                    if (null != mTvResultHint)
                        mTvResultLog.append(test_item_view);

                    break;
                case HANDLE_COMMAND_OTHERS_UPDATE_RESULT_LOG_END:
                    if (mAutoTransferTotalTimes != 0) {
                        String success_rate_text = "\nTTFF timeoutTimes     " + mTTFFTimeoutCont
                                + "\nTotle times           " + mAutoTransferTotalTimes
                                + "\nSuccess rate          "
                                + (float) (mAutoTransferTotalTimes - mTTFFTimeoutCont)
                                / mAutoTransferTotalTimes;
                        float disSum = sortAndSumFloat(mDistance);
                        String test_end = "Average distance(m) : " + mTestMeanDistance
                                + "\nAverage TTFF(s) : " + (float) mTestMeanTTFF / 1000.0;
                        float sum = sortAndSumFloat(mTtff);
                        float maxFirstDistance = 0;
                        float maxTtff = 0;
                        float m68FirstDistance = 0;
                        float m68Ttff = 0;
                        float m95FirstDistance = 0;
                        float m95Ttff = 0;
                        float totalInusedAve = 0;
                        float totalViewAve = 0;
                        float gpsInusedAve = 0;
                        float gpsViewAve = 0;
                        float gnssInusedAve = 0;
                        float gnssViewAve = 0;
                        float bdInusedAve = 0;
                        float bdViewAve = 0;
                        float sateTrackingAve = 0;
                        if (mSatelliteTestCont > 0) {
                            totalInusedAve = (float) intSum(mTotalInused) / mSatelliteTestCont;
                            totalViewAve = (float) intSum(mTotalView) / mSatelliteTestCont;
                            gpsInusedAve = (float) intSum(mGpsInUsed) / mSatelliteTestCont;
                            gpsViewAve = (float) intSum(mGpsView) / mSatelliteTestCont;
                            gnssInusedAve = (float) intSum(mGlonassInUsed) / mSatelliteTestCont;
                            gnssViewAve = (float) intSum(mGlonassView) / mSatelliteTestCont;
                            bdInusedAve = (float) intSum(mBeidouInUsed) / mSatelliteTestCont;
                            bdViewAve = (float) intSum(mBeidouView) / mSatelliteTestCont;
                        }
                        if (mSateTrackingCont != 0) {
                            sateTrackingAve = (float) intSum(mSateTracking) / mSateTrackingCont;
                        }
                        if (mDistanceCont >= 2) {
                            maxFirstDistance = mDistance[mDistanceCont - 1];
                            m68FirstDistance = mDistance[(int) (mDistanceCont * 0.68 - 1)];
                            m95FirstDistance = mDistance[(int) (mDistanceCont * 0.95 - 1)];
                        }
                        if (cont >= 2) {
                            maxTtff = mTtff[cont - 1];
                            m68Ttff = mTtff[(int) (cont * 0.68 - 1)];
                            m95Ttff = mTtff[(int) (cont * 0.95 - 1)];
                        }
                        String ttff_data = "\nAverage TTFF(s)       " + (float) mTestMeanTTFF
                                / 1000
                                + "\nm68Ttff is            " + m68Ttff / 1000
                                + "\nm95Ttff is            " + m95Ttff / 1000
                                + "\nmaxTtff is            " + maxTtff / 1000;
                        String distance_data = "\nAverage distance(m)   " + mTestMeanDistance
                                + "\nm68FirstDistance is   " + m68FirstDistance
                                + "\nm95FirstDistance is   " + m95FirstDistance
                                + "\nmaxFirstDistance is   " + maxFirstDistance;
                        String sate_status = "\nsateInusedAve is      " + totalInusedAve
                                + "\nsateTrackingAve is    " + sateTrackingAve;
                        String sate_status_total_min_max_ave_title = "\n\n                    Min"
                                + "       Max" + "        Ave";
                        String sate_status_total_min_max_ave = "\nUSE(TOT)       "
                                + mTotalInUsedMin + "        " + mTotalInUsedMax + "         "
                                + totalInusedAve + "\nVIEW(TOT)     " + mTotalViewMin + "        "
                                + mTotalViewMax + "         " + totalViewAve;
                        String sate_status_gps_min_max_ave = "\nUSE(GPS)       " + mGpsInUsedMin
                                + "        " + mGpsInUsedMax + "         " + gpsInusedAve
                                + "\nVIEW(GPS)     " + mGpsViewMin + "         " + mGpsViewMax
                                + "         " + gpsViewAve;
                        String sate_status_gnss_min_max_ave = "\nUSE(GLO)      " + mGlonassInUsedMin
                                + "        " + mGlonassInUsedMax + "         " + gnssInusedAve
                                + "\nVIEW(GLO)     " + mGlonassViewMin + "         " + mGlonassViewMax
                                + "         " + gnssViewAve;
                        String sate_status_bd_min_max_ave = "\nUSE(BDS)        " + mBeidouInUsedMin
                                + "         " + mBeidouInUsedMax + "         " + bdInusedAve
                                + "\nVIEW(BDS)     " + mBeidouViewMin + "         " + mBeidouViewMax
                                + "         " + bdViewAve;
                        String sate_status_min_max_ave = sate_status_total_min_max_ave_title
                                + sate_status_total_min_max_ave + sate_status_gps_min_max_ave
                                + sate_status_gnss_min_max_ave + sate_status_bd_min_max_ave;
                        showSuccessRate(success_rate_text + ttff_data + distance_data + sate_status
                                + sate_status_min_max_ave);
                        saveAUTOTESTLog(success_rate_text + ttff_data + distance_data + sate_status
                                + sate_status_min_max_ave);
                        finishSavingAUTOTESTLog();
                        if (null != mTvResultHint)
                            mTvResultLog.append(test_end);
                    }
                    break;
                case HANDLE_COMMAND_OTHERS_UPDATE_PROVIDER:
                    Log.d(TAG, "Time stamp[handleMessage]->" + new Date().getTime());
                    mLocationManager.requestLocationUpdates(
                            LocationManager.GPS_PROVIDER, 0, 0, mLocListener);
                    break;
                default:
                    break;
            }
            super.handleMessage(msg);
        }
    };

    public float sortAndSumFloat(float[] array) {
        float sum = 0;
        for (int i = 0; i < array.length; i++) {
                for (int j = 0; j < array.length - i - 1; j++) {
                    if (array[j] > array[j + 1] && array[j + 1] != 0) {
                    float temp = array[j];
                    array[j] = array[j + 1];
                    array[j + 1] = temp;
                }
            }
        }
        for (int i = 0; i < array.length; i++) {
            if(array[i] != 0)
            sum += array[i];
        }
        return sum;
    }
    public int intSum(int[] array) {
        int sum = 0;
        for (int i = 0; i < array.length; i++) {
            if (array[i] != 0)
            if (i < mCurrentTimes)
                sum += array[i];
        }
        return sum;
    }
    class SgpsWakeLock {
        private PowerManager.WakeLock mScreenWakeLock = null;
        private PowerManager.WakeLock mCpuWakeLock = null;

        /**
         * Acquire CPU wake lock
         * 
         * @param context Getting lock context
         */
        void acquireCpuWakeLock(Context context) {
            Log.d(TAG, "Acquiring cpu wake lock");
            if (mCpuWakeLock != null) {
                return;
            }

            PowerManager pm = (PowerManager) context
                    .getSystemService(Context.POWER_SERVICE);

            mCpuWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK
                    | PowerManager.ACQUIRE_CAUSES_WAKEUP, TAG);
            // | PowerManager.ON_AFTER_RELEASE, TAG);
            mCpuWakeLock.acquire();
        }

        void acquireScreenWakeLock(Context context) {
            Log.d(TAG, "Acquiring screen wake lock start");
            if (mScreenWakeLock != null) {
                return;
            }

            PowerManager pm = (PowerManager) context
                    .getSystemService(Context.POWER_SERVICE);

            mScreenWakeLock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK
                    | PowerManager.ACQUIRE_CAUSES_WAKEUP, TAG);
            // | PowerManager.ON_AFTER_RELEASE, TAG);
            mScreenWakeLock.acquire();
            Log.d(TAG, "Acquiring screen wake lock end");
        }

        void release() {
            Log.d(TAG, "Releasing wake lock");
            if (mCpuWakeLock != null) {
                mCpuWakeLock.release();
                mCpuWakeLock = null;
            }
            if (mScreenWakeLock != null) {
                mScreenWakeLock.release();
                mScreenWakeLock = null;
            }
        }
    }

    private void initGpsConfigLayout() {
        // GPS request Switch
        msgpsSwitch = (Switch) findViewById(R.id.sgps_switch);
        msgpsSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                boolean setGpsOpen = !isGpsOpen();
                Log.d(TAG, "onClick isGpsOpen " + !setGpsOpen + " , isChecked is" + isChecked);
                if (isChecked) {
                    mTtffValue = 0;
                    mHandler.removeMessages(HANDLE_COUNTER);
                    try {
                        if (mLocationManager == null) {
                            mLocationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
                        }
                        if (mLocationManager != null) {
                            mLocationManager.requestLocationUpdates(
                                    LocationManager.GPS_PROVIDER, 0, 0, mLocListener);
                        }
                    } catch (SecurityException e) {
                        Toast.makeText(SgpsActivity.this, "security exception", Toast.LENGTH_LONG)
                                .show();
                        Log.w(TAG, "Exception: " + e.getMessage());
                    }
                    mHandler.sendEmptyMessage(HANDLE_COUNTER);
                } else {
                    if (mLocListener != null) {
                        mLocationManager.removeUpdates(mLocListener);
                    }
                    mHandler.removeMessages(HANDLE_COUNTER);
                    TextView tvTtff = (TextView) findViewById(R.id.tv_ttff);
                    tvTtff.setText(0
                            + getString(R.string.time_unit_ms));
                }
            }
        });
        boolean bIs4G = SystemProperties.get("persist.service.agps.network",
                "2g").equals("4g");
        Log.d(TAG, "persist.service.agps.network value is bIs4G = " + bIs4G);

        magpsNetworkSwitch = (RadioGroup) findViewById(R.id.agps_network_switch);
        if (bIs4G) {
            magpsNetworkSwitch.check(R.id.agps_4g);
        } else {
            magpsNetworkSwitch.check(R.id.agps_2g);
        }
        int magpsNetworkSwitchSize = magpsNetworkSwitch.getChildCount();
        magpsNetworkSwitchItemId = new int[magpsNetworkSwitchSize];
        for (int i = 0; i < magpsNetworkSwitchSize; i++) {
            magpsNetworkSwitchItemId[i] = magpsNetworkSwitch.getChildAt(i)
                    .getId();
        }
        magpsNetworkSwitch
                .setOnCheckedChangeListener(new OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(RadioGroup group, int checkedId) {
                        if (checkedId == magpsNetworkSwitchItemId[0]) {
                            Log.d(TAG, "set persist.service.agps.network to 4g");
                            SystemProperties.set(
                                    "persist.service.agps.network", "4g");
                        } else if (checkedId == magpsNetworkSwitchItemId[1]) {
                            SystemProperties.set(
                                    "persist.service.agps.network", "2g");
                            Log.d(TAG, "set persist.service.agps.network to 2g");
                        }
                    }
                });
        // ControlPlane and User Plane switch
        mRGPlaneSwitch = (RadioGroup) findViewById(R.id.rg_plane_switch);
        final View mAgpsControlPlane = (View) findViewById(R.id.layout_agps_control_plane);
        final View mAgpsUserPlane = (View) findViewById(R.id.layout_agps_user_plane);
        String control_mode = getConfigration(mGpsConfigFile, "CONTROL-PLANE");
        int mRGPlaneSwitchSize = mRGPlaneSwitch.getChildCount();
        mRGPlaneSwitchItemId = new int[mRGPlaneSwitchSize];
        for (int i = 0; i < mRGPlaneSwitchSize; i++) {
            mRGPlaneSwitchItemId[i] = mRGPlaneSwitch.getChildAt(i).getId();
        }
        initAGpsControlPlane(mAgpsControlPlane);
        initAGpsUserPlane(mAgpsUserPlane);
        initAGpsCommon();
        initAGPSConfigLayoutStatus();
        if ("TRUE".equals(control_mode)) {
            mRGPlaneSwitch.check(mRGPlaneSwitchItemId[0]);
            mAgpsControlPlane.setVisibility(View.VISIBLE);
            mAgpsUserPlane.setVisibility(View.GONE);
        } else {
            mRGPlaneSwitch.check(mRGPlaneSwitchItemId[1]);
            mAgpsControlPlane.setVisibility(View.GONE);
            mAgpsUserPlane.setVisibility(View.VISIBLE);
        }
        mRGPlaneSwitch.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                if (checkedId == mRGPlaneSwitchItemId[0]) {
                    setConfigration(mGpsConfigFile, "CONTROL-PLANE", "TRUE");
                    mAgpsControlPlane.setVisibility(View.VISIBLE);
                    mAgpsUserPlane.setVisibility(View.GONE);
                } else if (checkedId == mRGPlaneSwitchItemId[1]) {
                    setConfigration(mGpsConfigFile, "CONTROL-PLANE", "FALSE");
                    mAgpsControlPlane.setVisibility(View.GONE);
                    mAgpsUserPlane.setVisibility(View.VISIBLE);
                }

            }
        });

    }

    private void initAGpsControlPlane(View view) {
        // MOLR position method
        mRGMolrPositionMethod = (RadioGroup) view.findViewById(R.id.rg_molr_position_method);
        String molr_position_method = getConfigration(mGpsConfigFile, "MPM");
        int mRGMolrPositionMethodSize = mRGMolrPositionMethod.getChildCount();
        mRGMolrPositionMethodItemId = new int[mRGMolrPositionMethodSize];
        for (int i = 0; i < mRGMolrPositionMethodSize; i++) {
            mRGMolrPositionMethodItemId[i] = mRGMolrPositionMethod.getChildAt(i).getId();
        }
        if ("LOCATION".equals(molr_position_method)) {
            mRGMolrPositionMethod.check(mRGMolrPositionMethodItemId[0]);
        } else {
            mRGMolrPositionMethod.check(mRGMolrPositionMethodItemId[1]);
        }

        mRGMolrPositionMethod.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                Log.d(TAG, "plane_switch_rg checkedId is " + checkedId);
                if (checkedId == mRGMolrPositionMethodItemId[0]) {
                    setConfigration(mGpsConfigFile, "MPM", "LOCATION");
                } else if (checkedId == mRGMolrPositionMethodItemId[1]) {
                    setConfigration(mGpsConfigFile, "MPM", "ASSISTANCE");
                }

            }
        });

        // External Address
        mExternalAddressCheckBox = (CheckBox) view.findViewById(R.id.cb_external_address);
        mExternalAddressEditText = (EditText) view.findViewById(R.id.et_external_address);
        mExternalAddressSaveButton = (Button) view.findViewById(R.id.bt_external_address_save);

        String mCheckBoxChecked = getConfigration(mGpsConfigFile, "ExtAddr Enable");
        if ("TRUE".equals(mCheckBoxChecked)) {
            mExternalAddressCheckBox.setChecked(true);
            mExternalAddressEditText.setEnabled(true);
            mExternalAddressSaveButton.setEnabled(true);
        } else {
            mExternalAddressCheckBox.setChecked(false);
            mExternalAddressEditText.setEnabled(false);
            mExternalAddressSaveButton.setEnabled(false);
        }

        mExternalAddressCheckBox
                .setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

                    @Override
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        mExternalAddressEditText.setEnabled(isChecked);
                        mExternalAddressSaveButton.setEnabled(isChecked);
                        setConfigration(mGpsConfigFile, "ExtAddr Enable",
                                isChecked ? "TRUE" : "FALSE");
                    }
                });
        mExternalAddressSaveButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                setConfigration(mGpsConfigFile, "EXTERNAL-ADDRESS",
                        mExternalAddressEditText.getText().toString().trim());
            }
        });
        mExternalAddressEditText.setText(getConfigration(mGpsConfigFile,
                "EXTERNAL-ADDRESS"));
        CharSequence eatext = mExternalAddressEditText.getText();
        if (eatext instanceof Spannable) {
            Spannable spanText = (Spannable) eatext;
            Selection.setSelection(spanText, eatext.length());
        }

        // MLC number
        mMLCNumberCheckBox = (CheckBox) view.findViewById(R.id.cb_mlc_number);
        mMLCNumberEditText = (EditText) view.findViewById(R.id.et_mlc_number);
        mMLCNumberSaveButton = (Button) view.findViewById(R.id.bt_mlc_number_save);

        mCheckBoxChecked = getConfigration(mGpsConfigFile, "MlcNum Enable");
        if ("TRUE".equals(mCheckBoxChecked)) {
            mMLCNumberCheckBox.setChecked(true);
            mMLCNumberEditText.setEnabled(true);
            mMLCNumberSaveButton.setEnabled(true);
        } else {
            mMLCNumberCheckBox.setChecked(false);
            mMLCNumberEditText.setEnabled(false);
            mMLCNumberSaveButton.setEnabled(false);
        }

        mMLCNumberCheckBox.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                mMLCNumberEditText.setEnabled(isChecked);
                mMLCNumberSaveButton.setEnabled(isChecked);
                setConfigration(mGpsConfigFile, "MlcNum Enable",
                        isChecked ? "TRUE" : "FALSE");

            }
        });
        mMLCNumberSaveButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                setConfigration(mGpsConfigFile, "MLC-NUMBER", mMLCNumberEditText.getText()
                        .toString().trim());
            }
        });
        mMLCNumberEditText.setText(getConfigration(mGpsConfigFile, "MLC-NUMBER"));
        CharSequence mntext = mMLCNumberEditText.getText();
        if (mntext instanceof Spannable) {
            Spannable spanText = (Spannable) mntext;
            Selection.setSelection(spanText, mntext.length());
        }

        // CP auto reset
        mAutoResetCheckBox = (CheckBox) view.findViewById(R.id.auto_reset);
        String IsAutoReset = getConfigration(mGpsConfigFile, "CP-AUTORESET");
        if ("TRUE".equals(IsAutoReset)) {
            mAutoResetCheckBox.setChecked(true);
        } else {
            mAutoResetCheckBox.setChecked(false);
        }
        mAutoResetCheckBox.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if (isChecked) {
                    setConfigration(mGpsConfigFile, "CP-AUTORESET", "TRUE");
                } else {
                    setConfigration(mGpsConfigFile, "CP-AUTORESET", "FALSE");
                }
            }
        });

        // SIM used
        mRGSimSelection = (RadioGroup) view.findViewById(R.id.rg_sim_selection);
        String sim = getConfigration(mGpsConfigFile, "SIM-PREFER");
        int mRGSimSelectionSize = mRGSimSelection.getChildCount();
        mRGSimSelectionItemId = new int[mRGSimSelectionSize];
        for (int i = 0; i < mRGSimSelectionSize; i++) {
            mRGSimSelectionItemId[i] = mRGSimSelection.getChildAt(i).getId();
        }
        if ("SIM1".equals(sim)) {
            mRGSimSelection.check(mRGSimSelectionItemId[0]);
        } else if ("SIM2".equals(sim)) {
            mRGSimSelection.check(mRGSimSelectionItemId[1]);
        }
        mRGSimSelection.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                Log.d(TAG, "plane_switch_rg checkedId is " + checkedId);
                if (checkedId == mRGSimSelectionItemId[0]) {
                    setConfigration(mGpsConfigFile, "SIM-PREFER", "SIM1");
                } else if (checkedId == mRGSimSelectionItemId[1]) {
                    setConfigration(mGpsConfigFile, "SIM-PREFER", "SIM2");
                }

            }
        });

        mMOLASwitch = (Switch) view.findViewById(R.id.mola_trigger);
        mMOLASwitch.setChecked(false);
        mMOLASwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                Log.d(TAG, "onCheckedChanged " + isChecked + ", resetToDefault " + resetToDefault);
                if (!resetToDefault) {
                    setMOLATrigger(isChecked);
                }
            }

        });
    }

    private int[] getRadioGroupItemId(RadioGroup item) {
        int mItemRadioGroupSize = item.getChildCount();
        int[] mRadioGroupItemId = new int[mItemRadioGroupSize];
        for (int i = 0; i < mItemRadioGroupSize; i++) {
            mRadioGroupItemId[i] = item.getChildAt(i).getId();
        }
        return mRadioGroupItemId;
    }

    private void initAGpsUserPlane(View view) {
        // A-GPS Mode
        mAGPSModeRadioGroup = (RadioGroup) findViewById(R.id.rg_agps_mode);
        mAGPSModeRadioGroupItemId = getRadioGroupItemId(mAGPSModeRadioGroup);
        mAGPSModeRadioGroup.setOnCheckedChangeListener(mAGPSRadioGroupCheckedChangeListener);

        mSLPArray = this.getResources().getStringArray(R.array.slp_array);
        for (int i = 0; i < mSLPArray.length; i++) {
            int index = mSLPArray[i].lastIndexOf(":");
            String name = mSLPArray[i].substring(index + 1);
            mSLPNameList.add(name);
            String values = mSLPArray[i].substring(0, index);
            mSLPValueList.add(values);
        }
        mPosMethodArray = this.getResources().getStringArray(R.array.posmethod_array);
        mPosMethodArrayValues = this.getResources().getStringArray(R.array.posmethod_array_values);
        mAreaTypeArray = this.getResources().getStringArray(R.array.area_type_array);
        mAreaTypeArrayValues = this.getResources().getStringArray(R.array.area_type_array_values);
        mNiDialogTestArrayValues = this.getResources().getStringArray(
                R.array.ni_dialog_test_array_values);

        mSLPTemplateButton = (Button) findViewById(R.id.bt_slp_template);
        mSLPTemplateButton.setOnClickListener(mAGPSBtnClickListener);

        mSLPTestButton = (Button) findViewById(R.id.bt_slp_test);
        mSLPTestButton.setOnClickListener(mAGPSBtnClickListener);
        mSLPTestResultTextView = (TextView) findViewById(R.id.slp_test_result);

        mSLPAddressTextView = (TextView) findViewById(R.id.slp_address_text);
        mSLPAddressEditButton = (Button) findViewById(R.id.slp_address_edit);
        mSLPAddressEditButton.setOnClickListener(mAGPSBtnClickListener);

        mSLPPortTextView = (TextView) findViewById(R.id.slp_port_text);
        mSLPPortEditButton = (Button) findViewById(R.id.slp_port_edit);
        mSLPPortEditButton.setOnClickListener(mAGPSBtnClickListener);

        mTLSEnableCheckBox = (CheckBox) view.findViewById(R.id.tls_enable_checkbox);
        setCheckBoxListener(mTLSEnableCheckBox, "TLS-ENABLE");

        mSetIDRadioGroup = (RadioGroup) findViewById(R.id.rg_set_id);
        mSetIDRadioGroupItemId = getRadioGroupItemId(mSetIDRadioGroup);
        mSetIDRadioGroup.setOnCheckedChangeListener(mAGPSRadioGroupCheckedChangeListener);

        mAccuracyUnitRadioGroup = (RadioGroup) findViewById(R.id.rg_accuracy_unit);
        mAccuracyUnitRadioGroupItemId = getRadioGroupItemId(mAccuracyUnitRadioGroup);
        mAccuracyUnitRadioGroup.setOnCheckedChangeListener(mAGPSRadioGroupCheckedChangeListener);

        mHorizontalAccuracyTextView = (TextView) findViewById(R.id.horizontal_accuracy_text);
        mHorizontalAccuracyEditButton = (Button) findViewById(R.id.horizontal_accuracy_edit);
        mHorizontalAccuracyEditButton.setOnClickListener(mAGPSBtnClickListener);

        mVerticalAccuracyTextView = (TextView) findViewById(R.id.vertical_accuracy_text);
        mVerticalAccuracyEditButton = (Button) findViewById(R.id.vertical_accuracy_edit);
        mVerticalAccuracyEditButton.setOnClickListener(mAGPSBtnClickListener);

        mLocationAgeTextView = (TextView) findViewById(R.id.location_age_text);
        mLocationAgeEditButton = (Button) findViewById(R.id.location_age_edit);
        mLocationAgeEditButton.setOnClickListener(mAGPSBtnClickListener);

        mDelayTextView = (TextView) findViewById(R.id.delay_text);
        mDelayEditButton = (Button) findViewById(R.id.delay_edit);
        mDelayEditButton.setOnClickListener(mAGPSBtnClickListener);

        mCertificateVerificationCheckBox = (CheckBox) view
                .findViewById(R.id.certificate_verification_checkbox);
        mCertificateVerificationCheckBox.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                setConfigration(mGpsConfigFile, "CER-VERIFY",
                        (isChecked ? "TRUE" : "FALSE"));
                mCertificateVerificationEditButton.setEnabled(isChecked);
            }
        });

        mCertificateVerificationEditButton = (Button) findViewById(R.id.certificate_verification_edit);
        mCertificateVerificationEditButton.setOnClickListener(mAGPSBtnClickListener);

        mEnableLabIOTTestCheckBox = (CheckBox) view.findViewById(R.id.enable_lab_iot_test_checkbox);
        setCheckBoxListener(mEnableLabIOTTestCheckBox, "LAB-IOT");

        mEnableeCIDCheckBox = (CheckBox) view.findViewById(R.id.enable_ecid_checkbox);
        setCheckBoxListener(mEnableeCIDCheckBox, "ECID-ENABLE");

        // agps supl2
        mAGPSSUPL2Layout = (LinearLayout) view.findViewById(R.id.layout_supl2);
        mEnableSUPL2CheckBox = (CheckBox) view.findViewById(R.id.supl2_enable_checkbox);
        mEnableSUPL2CheckBox
                .setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

                    @Override
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        if (isChecked) {
                            mAGPSSUPL2Layout.setVisibility(View.VISIBLE);
                        } else {
                            mAGPSSUPL2Layout.setVisibility(View.GONE);
                        }
                        setConfigration(mGpsConfigFile, "SUPL-VERSION",
                                isChecked ? "v2.0.0" : "v1.0.0");
                    }
                });

        if (showSUPL2View(getConfigration(mGpsConfigFile, "SUPL-VERSION"))) {
            mAGPSSUPL2Layout.setVisibility(View.VISIBLE);
            mEnableSUPL2CheckBox.setChecked(true);
        } else {
            mAGPSSUPL2Layout.setVisibility(View.GONE);
            mEnableSUPL2CheckBox.setChecked(false);
        }

        mSILRButton = (Button) findViewById(R.id.silr);
        mSILRButton.setOnClickListener(mAGPSBtnClickListener);
        mMSISDNTextView = (TextView) findViewById(R.id.msisdn_text);
        mMSISDNEditButton = (Button) findViewById(R.id.msisdn_edit);
        mMSISDNEditButton.setOnClickListener(mAGPSBtnClickListener);

        mEnable3rdMSISDNCheckBox = (CheckBox) view.findViewById(R.id.enable_3rd_msisdn_checkbox);
        m3rdMSISDNTextView = (TextView) findViewById(R.id.msisdn_3rd_text);
        m3rdMSISDNEditButton = (Button) findViewById(R.id.msisdn_3rd_edit);
        m3rdMSISDNEditButton.setOnClickListener(mAGPSBtnClickListener);

        mEnable3rdMSISDNCheckBox
                .setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

                    @Override
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        setConfigration(mGpsConfigFile, "",
                                (isChecked ? "TRUE" : "FALSE"));
                        m3rdMSISDNEditButton.setEnabled(isChecked);
                    }
                });

        mTriggerStartButton = (Button) findViewById(R.id.trigger_start);
        mTriggerStartButton.setOnClickListener(mAGPSBtnClickListener);
        mTriggerAbortButton = (Button) findViewById(R.id.trigger_abort);
        mTriggerAbortButton.setOnClickListener(mAGPSBtnClickListener);

        mAreaTypeLayout = (LinearLayout) findViewById(R.id.layout_area_type);
        mPerodicTypeLayout = (LinearLayout) findViewById(R.id.layout_perodic_type);

        mTriggerTypeRadioGroup = (RadioGroup) findViewById(R.id.rg_trigger_type);
        mTriggerTypeRadioGroup.setOnCheckedChangeListener(mAGPSRadioGroupCheckedChangeListener);
        mTriggerTypeGroupItemId = getRadioGroupItemId(mTriggerTypeRadioGroup);

        mPosMethodView = (TextView) findViewById(R.id.posmethod_text);
        mPosMethodSelectButton = (Button) findViewById(R.id.posmethod_edit);
        mPosMethodSelectButton.setOnClickListener(mAGPSBtnClickListener);

        mPerodicMinIntervalTextView = (TextView) findViewById(R.id.perodic_min_interval_text);
        mPerodicMinIntervalEditButton = (Button) findViewById(R.id.perodic_min_interval_edit);
        mPerodicMinIntervalEditButton.setOnClickListener(mAGPSBtnClickListener);

        mPerodicStartTimeTextView = (TextView) findViewById(R.id.perodic_start_time_text);
        mPerodicStartTimeEditButton = (Button) findViewById(R.id.perodic_start_time_edit);
        mPerodicStartTimeEditButton.setOnClickListener(mAGPSBtnClickListener);

        mPerodicStopTimeTextView = (TextView) findViewById(R.id.perodic_stop_time_text);
        mPerodicStopTimeEditButton = (Button) findViewById(R.id.perodic_stop_time_edit);
        mPerodicStopTimeEditButton.setOnClickListener(mAGPSBtnClickListener);

        mAreaTypeTextView = (TextView) findViewById(R.id.area_type_text);
        mAreaTypeSelectButton = (Button) findViewById(R.id.area_type_edit);
        mAreaTypeSelectButton.setOnClickListener(mAGPSBtnClickListener);

        mAreaMinIntervalTextView = (TextView) findViewById(R.id.min_interval_text);
        mAreaMinIntervalEditButton = (Button) findViewById(R.id.min_interval_edit);
        mAreaMinIntervalEditButton.setOnClickListener(mAGPSBtnClickListener);

        mMaxNumTextView = (TextView) findViewById(R.id.max_num_text);
        mMaxNumEditButton = (Button) findViewById(R.id.max_num_edit);
        mMaxNumEditButton.setOnClickListener(mAGPSBtnClickListener);

        mAreaStartTimeTextView = (TextView) findViewById(R.id.start_time_text);
        mAreaStartTimeEditButton = (Button) findViewById(R.id.start_time_edit);
        mAreaStartTimeEditButton.setOnClickListener(mAGPSBtnClickListener);

        mAreaStopTimeTextView = (TextView) findViewById(R.id.stop_time_text);
        mAreaStopTimeEditButton = (Button) findViewById(R.id.stop_time_edit);
        mAreaStopTimeEditButton.setOnClickListener(mAGPSBtnClickListener);

        mGeographicButton = (Button) findViewById(R.id.geographic_edit);
        mGeographicButton.setClickable(false);
        mGeographicTextView = (TextView) findViewById(R.id.geographic_text);

        mGeoRadiusTextView = (TextView) findViewById(R.id.georadius_text);
        mGeoRadiusEditButton = (Button) findViewById(R.id.georadius_edit);
        mGeoRadiusEditButton.setOnClickListener(mAGPSBtnClickListener);

        mLatitudeTextView = (TextView) findViewById(R.id.latitude_text);
        mLatitudeEditButton = (Button) findViewById(R.id.latitude_edit);
        mLatitudeEditButton.setOnClickListener(mAGPSBtnClickListener);

        mLongitudeTextView = (TextView) findViewById(R.id.longitude_text);
        mLongitudeEditButton = (Button) findViewById(R.id.longitude_edit);
        mLongitudeEditButton.setOnClickListener(mAGPSBtnClickListener);

        mSignRadioGroup = (RadioGroup) findViewById(R.id.rg_sign);
        mSignRadioGroup.setOnCheckedChangeListener(mAGPSRadioGroupCheckedChangeListener);
    }

    private void initAGpsCommon() {
        // agps common
        mAllowNetworkInitiatedRequestCheckBox = (CheckBox) findViewById(R.id.allow_network_initiated_request);
        setCheckBoxListener(mAllowNetworkInitiatedRequestCheckBox, "NI-ENABLE");

        mAllowEMNotificationCheckBox = (CheckBox) findViewById(R.id.allow_em_notification);
        setCheckBoxListener(mAllowEMNotificationCheckBox, "EM-NOTIFY");

        mAllowRoamingCheckBox = (CheckBox) findViewById(R.id.allow_roaming);
        setCheckBoxListener(mAllowRoamingCheckBox, "ROAMING-ENABLE");

        mLogSUPLToFileCheckBox = (CheckBox) findViewById(R.id.log_supl_to_file);
        setCheckBoxListener(mLogSUPLToFileCheckBox, "SUPLLOG-SAVE");

        mNotificationTimeoutArray = this.getResources().getStringArray(
                R.array.notification_timeout_array);
        mVerificationTimeoutArray = this.getResources().getStringArray(
                R.array.verification_timeout_array);
        mNiDialogTestArray = this.getResources().getStringArray(R.array.ni_dialog_test_array);

        mNotificationTimeoutSpinner = (Spinner) findViewById(R.id.notification_timeout);
        ArrayAdapter<String> mNotificationTimeoutSpinnerAdapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, mNotificationTimeoutArray);
        mNotificationTimeoutSpinner.setAdapter(mNotificationTimeoutSpinnerAdapter);
        mNotificationTimeoutSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {

            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                setConfigration(mGpsConfigFile, "NOTIFY-TIMEOUT",
                        String.valueOf(position + 1));
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
        mVerificationTimeoutSpinner = (Spinner) findViewById(R.id.verification_timeout);
        ArrayAdapter<String> mVerificationTimeoutSpinnerAdapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, mVerificationTimeoutArray);
        mVerificationTimeoutSpinner.setAdapter(mVerificationTimeoutSpinnerAdapter);
        mVerificationTimeoutSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {

            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                setConfigration(mGpsConfigFile, "VERIFY-TIMEOUT",
                        String.valueOf(position + 1));
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
        mResetToDefaultButton = (Button) findViewById(R.id.bt_reset_to_default);
        mResetToDefaultButton.setOnClickListener(mAGPSBtnClickListener);
        mNITestSelectButton = (Button) findViewById(R.id.bt_ni_dialog_test);
        mNITestSelectButton.setOnClickListener(mAGPSBtnClickListener);
    }

    private void initAGPSConfigLayoutStatus() {
        // user plane
        String agpsMode = getConfigration(mGpsConfigFile, "SUPL-MODE");
        if ("standalone".equals(agpsMode)) {
            mAGPSModeRadioGroup.check(mAGPSModeRadioGroupItemId[0]);
        } else if ("msb".equals(agpsMode)) {
            mAGPSModeRadioGroup.check(mAGPSModeRadioGroupItemId[1]);
        } else if ("msa".equals(agpsMode)) {
            mAGPSModeRadioGroup.check(mAGPSModeRadioGroupItemId[2]);
        }
        initAGPSTextViewItemStatus(mSLPAddressTextView, "SERVER-ADDRESS");
        initAGPSTextViewItemStatus(mSLPPortTextView, "SERVER-PORT");
        initAGPSCheckBoxItemStatus(mTLSEnableCheckBox, "TLS-ENABLE");
        String setId = getConfigration(mGpsConfigFile, "SETID");
        if ("IPV4".equals(setId)) {
            mSetIDRadioGroup.check(mSetIDRadioGroupItemId[0]);
        } else if ("IMSI".equals(setId)) {
            mSetIDRadioGroup.check(mSetIDRadioGroupItemId[1]);
        }
        String accuracyUnit = getConfigration(mGpsConfigFile, "ACCURACY-UNIT");
        if ("KVALUE".equals(accuracyUnit)) {
            mAccuracyUnitRadioGroup.check(mAccuracyUnitRadioGroupItemId[0]);
        } else if ("METER".equals(accuracyUnit)) {
            mAccuracyUnitRadioGroup.check(mAccuracyUnitRadioGroupItemId[1]);
        }
        initAGPSTextViewItemStatus(mHorizontalAccuracyTextView, "HORIZON-ACCURACY");
        initAGPSTextViewItemStatus(mVerticalAccuracyTextView, "VERTICAL-ACCURACY");
        initAGPSTextViewItemStatus(mLocationAgeTextView, "LOC-AGE");
        initAGPSTextViewItemStatus(mDelayTextView, "DELAY");

        String enable = getConfigration(mGpsConfigFile, "CER-VERIFY");
        if ("TRUE".equals(enable)) {
            mCertificateVerificationCheckBox.setChecked(true);
            mCertificateVerificationEditButton.setEnabled(true);
        } else {
            mCertificateVerificationCheckBox.setChecked(false);
            mCertificateVerificationEditButton.setEnabled(false);
        }

        initAGPSCheckBoxItemStatus(mEnableLabIOTTestCheckBox, "LAB-IOT");
        initAGPSCheckBoxItemStatus(mEnableeCIDCheckBox, "ECID-ENABLE");

        initAGPSTextViewItemStatus(mMSISDNTextView, "LOCAL-MSISDN");
        initAGPSTextViewItemStatus(m3rdMSISDNTextView, "3RD-MSISDN");

        if ("TRUE".equals(getConfigration(mGpsConfigFile, ""))) {
            mEnable3rdMSISDNCheckBox.setChecked(true);
            m3rdMSISDNEditButton.setEnabled(true);
        } else {
            mEnable3rdMSISDNCheckBox.setChecked(false);
            m3rdMSISDNEditButton.setEnabled(false);
        }

        initAGPSTextViewItemStatus(mPerodicMinIntervalTextView, "PERIODIC-MIN-INTERVAL");
        initAGPSTextViewItemStatus(mPerodicStartTimeTextView, "PERIODIC-START-TIME");
        initAGPSTextViewItemStatus(mPerodicStopTimeTextView, "PERIODIC-STOP-TIME");

        String TriggerType = getConfigration(mGpsConfigFile, "TRIGGER-TYPE");
        if ("PERIODIC".equals(TriggerType)) {
            mTriggerTypeRadioGroup.check(mTriggerTypeGroupItemId[0]);
            mAreaTypeLayout.setVisibility(View.GONE);
            mPerodicTypeLayout.setVisibility(View.VISIBLE);
        } else if ("AREA".equals(TriggerType)) {
            mTriggerTypeRadioGroup.check(mTriggerTypeGroupItemId[1]);
            mAreaTypeLayout.setVisibility(View.VISIBLE);
            mPerodicTypeLayout.setVisibility(View.GONE);
        }

        String posmethod = "";
        if ("PERIODIC".equals(TriggerType)) {
            posmethod = getConfigration(mGpsConfigFile, "PERIODIC-POSMETHOD");
        } else if ("AREA".equals(TriggerType)) {
            posmethod = getConfigration(mGpsConfigFile, "AREA-POSMETHOD");
        }

        List<String> mPosMethodArrayValuesList = Arrays.asList(mPosMethodArrayValues);
        if (!"".equals(posmethod) && posmethod != null) {
            mPosMethodView.setText(Arrays.asList(mPosMethodArray).get(
                    mPosMethodArrayValuesList.indexOf(posmethod)));
        }

        String areaType = getConfigration(mGpsConfigFile, "AREA-TYPE");
        List<String> mAreaTypeArrayValuesList = Arrays.asList(mAreaTypeArrayValues);
        if (areaType != null) {
            mAreaTypeTextView.setText(Arrays.asList(mAreaTypeArray).get(
                    mAreaTypeArrayValuesList.indexOf(areaType)));
        }

        initAGPSTextViewItemStatus(mAreaMinIntervalTextView, "AREA-MIN-INTERVAL");
        initAGPSTextViewItemStatus(mMaxNumTextView, "MAX-NUM");
        initAGPSTextViewItemStatus(mAreaStartTimeTextView, "AREA-START-TIME");
        initAGPSTextViewItemStatus(mAreaStopTimeTextView, "AREA-STOP-TIME");
        initAGPSTextViewItemStatus(mGeographicTextView, "GEOGRAPHIC");
        initAGPSTextViewItemStatus(mGeoRadiusTextView, "GEORADIUS");
        initAGPSTextViewItemStatus(mLatitudeTextView, "GEO-LAT");
        initAGPSTextViewItemStatus(mLongitudeTextView, "GEO-LON");
        mSignRadioGroupItemId = getRadioGroupItemId(mSignRadioGroup);
        String signType = getConfigration(mGpsConfigFile, "SIGN");
        if ("NORTH".equals(signType)) {
            mSignRadioGroup.check(mSignRadioGroupItemId[0]);
        } else if ("SOUTH".equals(signType)) {
            mSignRadioGroup.check(mSignRadioGroupItemId[1]);
        }
        // common
        initAGPSCheckBoxItemStatus(mAllowNetworkInitiatedRequestCheckBox, "NI-ENABLE");
        initAGPSCheckBoxItemStatus(mAllowEMNotificationCheckBox, "EM-NOTIFY");
        initAGPSCheckBoxItemStatus(mAllowRoamingCheckBox, "ROAMING-ENABLE");
        initAGPSCheckBoxItemStatus(mLogSUPLToFileCheckBox, "SUPLLOG-SAVE");

        int notifyIndex = Integer.parseInt(getConfigration(mGpsConfigFile,
                "NOTIFY-TIMEOUT") == null ? "8" : getConfigration(mGpsConfigFile,
                        "NOTIFY-TIMEOUT"));
        mNotificationTimeoutSpinner.setSelection(notifyIndex - 1);
        int verifyIndex = Integer.parseInt(getConfigration(mGpsConfigFile,
                "VERIFY-TIMEOUT") == null ? "8" : getConfigration(mGpsConfigFile,
                        "VERIFY-TIMEOUT"));
        mVerificationTimeoutSpinner.setSelection(verifyIndex - 1);
    }

    private OnCheckedChangeListener mAGPSRadioGroupCheckedChangeListener = new OnCheckedChangeListener() {

        @Override
        public void onCheckedChanged(RadioGroup group, int checkedId) {
            if (group == mAGPSModeRadioGroup) {
                if (checkedId == mAGPSModeRadioGroupItemId[0]) {
                    setConfigration(mGpsConfigFile, "SUPL-MODE", "standalone");
                } else if (checkedId == mAGPSModeRadioGroupItemId[1]) {
                    setConfigration(mGpsConfigFile, "SUPL-MODE", "msb");
                } else if (checkedId == mAGPSModeRadioGroupItemId[2]) {
                    setConfigration(mGpsConfigFile, "SUPL-MODE", "msa");
                }
            } else if (group == mSetIDRadioGroup) {
                if (checkedId == mSetIDRadioGroupItemId[0]) {
                    setConfigration(mGpsConfigFile, "SETID", "IPV4");
                } else if (checkedId == mSetIDRadioGroupItemId[1]) {
                    setConfigration(mGpsConfigFile, "SETID", "IMSI");
                }
            } else if (group == mAccuracyUnitRadioGroup) {
                if (checkedId == mAccuracyUnitRadioGroupItemId[0]) {
                    setConfigration(mGpsConfigFile, "ACCURACY-UNIT", "KVALUE");
                } else if (checkedId == mAccuracyUnitRadioGroupItemId[1]) {
                    setConfigration(mGpsConfigFile, "ACCURACY-UNIT", "METER");
                }
            } else if (group == mTriggerTypeRadioGroup) {
                String posmethod = "";
                if (checkedId == mTriggerTypeGroupItemId[0]) {
                    setConfigration(mGpsConfigFile, "TRIGGER-TYPE", "PERIODIC");
                    posmethod = getConfigration(mGpsConfigFile, "PERIODIC-POSMETHOD");
                    mAreaTypeLayout.setVisibility(View.GONE);
                    mPerodicTypeLayout.setVisibility(View.VISIBLE);
                } else if (checkedId == mTriggerTypeGroupItemId[1]) {
                    setConfigration(mGpsConfigFile, "TRIGGER-TYPE", "AREA");
                    posmethod = getConfigration(mGpsConfigFile, "AREA-POSMETHOD");
                    mAreaTypeLayout.setVisibility(View.VISIBLE);
                    mPerodicTypeLayout.setVisibility(View.GONE);
                }
                if(posmethod == null){
                    return;
                }
                List<String> mPosMethodArrayValuesList = Arrays.asList(mPosMethodArrayValues);
                mPosMethodView.setText(Arrays.asList(mPosMethodArray).get(
                        mPosMethodArrayValuesList.indexOf(posmethod)));
            } else if (group == mSignRadioGroup) {
                if (checkedId == mSignRadioGroupItemId[0]) {
                    setConfigration(mGpsConfigFile, "SIGN", "NORTH");
                } else if (checkedId == mSignRadioGroupItemId[1]) {
                    setConfigration(mGpsConfigFile, "SIGN", "SOUTH");
                }
            }

        }
    };

    protected Dialog onCreateDialog(int id, Bundle args) {
        Dialog dialog = null;
        AlertDialog.Builder builder = null;
        switch (id) {
            case DIALOG_SLP_TEMPLATE:
                builder = new AlertDialog.Builder(this);
                final ArrayList<String> SLPValueList = mSLPValueList;
                final CharSequence[] items = mSLPNameList.toArray(new CharSequence[mSLPNameList
                        .size()]);
                builder.setItems(items, new DialogInterface.OnClickListener() {

                    public void onClick(DialogInterface dialog, int which) {
                        String[] item = SLPValueList.get(which).split(":");
                        Log.d(TAG, "address = " + item[0] + ", port is " + item[1]);
                        setConfigration(mGpsConfigFile, "SERVER-ADDRESS", item[0]);
                        setConfigration(mGpsConfigFile, "SERVER-PORT", item[1]);
                        mSLPAddressTextView.setText(item[0]);
                        mSLPPortTextView.setText(item[1]);
                    }
                });
                dialog = builder.show();

                break;
            case DIALOG_SLP_ADDRESS:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mSLPAddressTextView,
                        InputType.TYPE_CLASS_TEXT, "SERVER-ADDRESS", null, -1, null);
                dialog = builder.show();
                break;
            case DIALOG_SLP_PORT:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mSLPPortTextView,
                        InputType.TYPE_CLASS_NUMBER, "SERVER-PORT", null, -1, null);
                dialog = builder.show();
                break;
            case DIALOG_HORIZONTAL_ACCURACY:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mHorizontalAccuracyTextView,
                        InputType.TYPE_CLASS_NUMBER, "HORIZON-ACCURACY", "0123456789", 3, "0～100");
                dialog = builder.show();
                break;
            case DIALOG_VERTICAL_ACCURACY:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mVerticalAccuracyTextView,
                        InputType.TYPE_CLASS_NUMBER, "VERTICAL-ACCURACY", "0123456789", 5,
                        "0～99999");
                dialog = builder.show();
                break;
            case DIALOG_LOCATIONAGE:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mLocationAgeTextView,
                        InputType.TYPE_CLASS_NUMBER, "LOC-AGE", "0123456789", 5,
                        "0～99999");
                dialog = builder.show();
                break;
            case DIALOG_DELAY:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mDelayTextView,
                        InputType.TYPE_CLASS_NUMBER, "DELAY", "0123456789", 5,
                        "0～99999");
                dialog = builder.show();
                break;
            case DIALOG_CERTIFICATEVERIFICATION:
                builder = new AlertDialog.Builder(this);
                final EditText et = new EditText(this);
                et.setInputType(InputType.TYPE_CLASS_TEXT);
                et.setText(getConfigration(mGpsConfigFile, "SUPL-CER"));
                builder.setTitle(R.string.edit);
                CharSequence text = et.getText();
                if (text instanceof Spannable) {
                    Spannable spanText = (Spannable) text;
                    Selection.setSelection(spanText, text.length());
                }
                builder.setView(et);
                builder.setPositiveButton(R.string.dlg_ok,
                        new DialogInterface.OnClickListener() {

                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                Log.d(TAG, et.getText().toString());
                                String value = et.getText().toString();
                                setConfigration(mGpsConfigFile, "SUPL-CER", value);
                            }
                        });
                builder.setNegativeButton(R.string.dlg_cancel, null);
                dialog = builder.show();
                break;
            case DIALOG_MSISDN:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mMSISDNTextView,
                        InputType.TYPE_CLASS_NUMBER, "LOCAL-MSISDN", "0123456789", 20, null);
                dialog = builder.show();
                break;
            case DIALOG_3RDMSISDN:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, m3rdMSISDNTextView,
                        InputType.TYPE_CLASS_NUMBER, "3RD-MSISDN", "0123456789", 20, null);
                dialog = builder.show();
                break;
            case DIALOG_POSMETHOD_SELECT:
                builder = new AlertDialog.Builder(this);
                final CharSequence[] posmethoditems = mPosMethodArray;
                builder.setItems(posmethoditems, new DialogInterface.OnClickListener() {

                    public void onClick(DialogInterface dialog, int which) {
                        String TriggerType = getConfigration(mGpsConfigFile,
                                "TRIGGER-TYPE");
                        if ("PERIODIC".equals(TriggerType)) {
                            setConfigration(mGpsConfigFile, "PERIODIC-POSMETHOD",
                                    mPosMethodArrayValues[which]);
                        } else {
                            setConfigration(mGpsConfigFile, "AREA-POSMETHOD",
                                    mPosMethodArrayValues[which]);
                        }
                        mPosMethodView.setText(posmethoditems[which]);
                    }
                });
                dialog = builder.show();
                break;
            case DIALOG_AREA_TYPE_SELECT:
                builder = new AlertDialog.Builder(this);
                final CharSequence[] areTypeitems = mAreaTypeArray;
                builder.setItems(areTypeitems, new DialogInterface.OnClickListener() {

                    public void onClick(DialogInterface dialog, int which) {
                        setConfigration(mGpsConfigFile, "AREA-TYPE",
                                mAreaTypeArrayValues[which]);
                        mAreaTypeTextView.setText(areTypeitems[which]);
                    }
                });
                dialog = builder.show();
                break;
            case DIALOG_PERODIC_MININTERVAL:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mPerodicMinIntervalTextView,
                        InputType.TYPE_CLASS_NUMBER, "PERIODIC-MIN-INTERVAL", null, -1, null);
                dialog = builder.show();
                break;
            case DIALOG_PERODIC_STARTTIME:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mPerodicStartTimeTextView,
                        InputType.TYPE_CLASS_NUMBER, "PERIODIC-START-TIME", null, -1, null);
                dialog = builder.show();
                break;
            case DIALOG_PERODIC_STOPTIME:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mPerodicStopTimeTextView,
                        InputType.TYPE_CLASS_NUMBER, "PERIODIC-STOP-TIME", null, -1, null);
                dialog = builder.show();
                break;
            case DIALOG_AREA_MININTERVAL:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mAreaMinIntervalTextView,
                        InputType.TYPE_CLASS_NUMBER, "AREA-MIN-INTERVAL", null, -1, null);
                dialog = builder.show();
                break;
            case DIALOG_MAXNUM:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mMaxNumTextView,
                        InputType.TYPE_CLASS_NUMBER, "MAX-NUM", null, -1, null);
                dialog = builder.show();
                break;
            case DIALOG_AREA_STARTTIME:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mAreaStartTimeTextView,
                        InputType.TYPE_CLASS_NUMBER, "AREA-START-TIME", null, -1, null);
                dialog = builder.show();
                break;
            case DIALOG_AREA_STOPTIME:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mAreaStopTimeTextView,
                        InputType.TYPE_CLASS_NUMBER, "AREA-STOP-TIME", null, -1, null);
                dialog = builder.show();
                break;
            case DIALOG_GEORADIUS:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mGeoRadiusTextView,
                        InputType.TYPE_CLASS_TEXT, "GEORADIUS", null, -1, null);
                dialog = builder.show();
                break;
            case DIALOG_LATITUDE:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mLatitudeTextView,
                        InputType.TYPE_CLASS_TEXT, "GEO-LAT", null, -1, null);
                dialog = builder.show();
                break;
            case DIALOG_LONGITUDE:
                builder = new AlertDialog.Builder(this);
                builder = setEditTextDialogBuilder(builder, mLongitudeTextView,
                        InputType.TYPE_CLASS_TEXT, "GEO-LON", null, -1, null);
                dialog = builder.show();
                break;
            case DIALOG_NI_DIALOG_TEST:
                builder = new AlertDialog.Builder(this);
                final CharSequence[] niDialogTestitems = mNiDialogTestArray;
                builder.setItems(niDialogTestitems, new DialogInterface.OnClickListener() {

                    public void onClick(DialogInterface dialog, int which) {
                        setConfigration(mGpsConfigFile, "NI-TEST",
                                mNiDialogTestArrayValues[which]);
                    }
                });
                dialog = builder.show();
                break;
        }
        return dialog;
    };

    private OnClickListener mAGPSBtnClickListener = new OnClickListener() {

        @Override
        public void onClick(View v) {
            if (v == (View) mSLPTemplateButton) {
                removeDialog(DIALOG_SLP_TEMPLATE);
                showDialog(DIALOG_SLP_TEMPLATE);
            } else if (v == (View) mSLPTestButton) {
                mSLPTestResultTextView.setText("");
            } else if (v == (View) mSLPAddressEditButton) {
                removeDialog(DIALOG_SLP_ADDRESS);
                showDialog(DIALOG_SLP_ADDRESS);
            } else if (v == (View) mSLPPortEditButton) {
                removeDialog(DIALOG_SLP_PORT);
                showDialog(DIALOG_SLP_PORT);
            } else if (v == (View) mHorizontalAccuracyEditButton) {
                removeDialog(DIALOG_HORIZONTAL_ACCURACY);
                showDialog(DIALOG_HORIZONTAL_ACCURACY);
            } else if (v == (View) mVerticalAccuracyEditButton) {
                removeDialog(DIALOG_VERTICAL_ACCURACY);
                showDialog(DIALOG_VERTICAL_ACCURACY);
            } else if (v == (View) mLocationAgeEditButton) {
                removeDialog(DIALOG_LOCATIONAGE);
                showDialog(DIALOG_LOCATIONAGE);
            } else if (v == (View) mDelayEditButton) {
                removeDialog(DIALOG_DELAY);
                showDialog(DIALOG_DELAY);
            } else if (v == (View) mCertificateVerificationEditButton) {
                removeDialog(DIALOG_CERTIFICATEVERIFICATION);
                showDialog(DIALOG_CERTIFICATEVERIFICATION);
            } else if (v == (View) mSILRButton) {
            } else if (v == (View) mMSISDNEditButton) {
                removeDialog(DIALOG_MSISDN);
                showDialog(DIALOG_MSISDN);
            } else if (v == (View) m3rdMSISDNEditButton) {
                removeDialog(DIALOG_3RDMSISDN);
                showDialog(DIALOG_3RDMSISDN);
            } else if (v == (View) mTriggerStartButton) {
                sendCommand("$PSPRD,00,7,1");

            } else if (v == (View) mTriggerAbortButton) {
                sendCommand("$PSPRD,00,7,2");
            } else if (v == (View) mPosMethodSelectButton) {
                removeDialog(DIALOG_POSMETHOD_SELECT);
                showDialog(DIALOG_POSMETHOD_SELECT);
            } else if (v == (View) mAreaTypeSelectButton) {
                removeDialog(DIALOG_AREA_TYPE_SELECT);
                showDialog(DIALOG_AREA_TYPE_SELECT);
            } else if (v == (View) mPerodicMinIntervalEditButton) {
                removeDialog(DIALOG_PERODIC_MININTERVAL);
                showDialog(DIALOG_PERODIC_MININTERVAL);
            } else if (v == (View) mPerodicStartTimeEditButton) {
                removeDialog(DIALOG_PERODIC_STARTTIME);
                showDialog(DIALOG_PERODIC_STARTTIME);
            } else if (v == (View) mPerodicStopTimeEditButton) {
                removeDialog(DIALOG_PERODIC_STOPTIME);
                showDialog(DIALOG_PERODIC_STOPTIME);
            } else if (v == (View) mAreaMinIntervalEditButton) {
                removeDialog(DIALOG_AREA_MININTERVAL);
                showDialog(DIALOG_AREA_MININTERVAL);
            } else if (v == (View) mMaxNumEditButton) {
                removeDialog(DIALOG_MAXNUM);
                showDialog(DIALOG_MAXNUM);
            } else if (v == (View) mAreaStartTimeEditButton) {
                removeDialog(DIALOG_AREA_STARTTIME);
                showDialog(DIALOG_AREA_STARTTIME);
            } else if (v == (View) mAreaStopTimeEditButton) {
                removeDialog(DIALOG_AREA_STOPTIME);
                showDialog(DIALOG_AREA_STOPTIME);
            } else if (v == (View) mGeoRadiusEditButton) {
                removeDialog(DIALOG_GEORADIUS);
                showDialog(DIALOG_GEORADIUS);
            } else if (v == (View) mLatitudeEditButton) {
                removeDialog(DIALOG_LATITUDE);
                showDialog(DIALOG_LATITUDE);
            } else if (v == (View) mLongitudeEditButton) {
                removeDialog(DIALOG_LONGITUDE);
                showDialog(DIALOG_LONGITUDE);
            } else if (v == (View) mResetToDefaultButton) {
                resetToDefault = true;
                resetToDefault();
                resetToDefault = false;
            } else if (v == (View) mNITestSelectButton) {
                removeDialog(DIALOG_NI_DIALOG_TEST);
                showDialog(DIALOG_NI_DIALOG_TEST);
            }
        }

    };

    private void resetToDefault() {
        // reset control plane
        String mDefault = getResources().getString(R.string.default_mpm);
        if ("LOCATION".equals(mDefault)) {
            mRGMolrPositionMethod.check(mRGMolrPositionMethodItemId[0]);
        } else {
            mRGMolrPositionMethod.check(mRGMolrPositionMethodItemId[1]);
        }
        setConfigration(mGpsConfigFile, "MPM", mDefault);

        boolean isTure = getResources().getBoolean(R.bool.default_external_address_checkbox_checked);
        setConfigration(mGpsConfigFile, "ExtAddr Enable", isTure ? "TRUE" : "FALSE");
        if (isTure) {
            mExternalAddressCheckBox.setChecked(true);
            mExternalAddressEditText.setEnabled(true);
            mExternalAddressSaveButton.setEnabled(true);
        } else {
            mExternalAddressCheckBox.setChecked(false);
            mExternalAddressEditText.setEnabled(false);
            mExternalAddressSaveButton.setEnabled(false);
        }
        resetAGPSTextViewItemStatus(mExternalAddressEditText, "EXTERNAL-ADDRESS", getResources()
                .getString(R.string.default_external_address));

        isTure = getResources().getBoolean(R.bool.default_mlc_number_checkbox_checked);
        setConfigration(mGpsConfigFile, "MlcNum Enable",
                isTure ? "TRUE" : "FALSE");
        if (isTure) {
            mMLCNumberCheckBox.setChecked(true);
            mMLCNumberEditText.setEnabled(true);
            mMLCNumberSaveButton.setEnabled(true);
        } else {
            mMLCNumberCheckBox.setChecked(false);
            mMLCNumberEditText.setEnabled(false);
            mMLCNumberSaveButton.setEnabled(false);
        }

        resetAGPSTextViewItemStatus(mMLCNumberEditText, "MLC-NUMBER", getResources()
                .getString(R.string.default_mlc_number));

        resetAGPSCheckBoxItemStatus(mAutoResetCheckBox, "CP-AUTORESET",
                getResources().getBoolean(R.bool.default_cp_autoreset));

        mDefault = getResources().getString(R.string.default_sim_prefer);
        if ("SIM1".equals(mDefault)) {
            mRGSimSelection.check(mRGSimSelectionItemId[0]);
        } else if ("SIM2".equals(mDefault)) {
            mRGSimSelection.check(mRGSimSelectionItemId[1]);
        }
        setConfigration(mGpsConfigFile, "SIM-PREFER", mDefault);

        mMOLASwitch.setChecked(false);

        // reset user plane
        mDefault = getResources().getString(R.string.default_supl_mode);
        if ("standalone".equals(mDefault)) {
            mAGPSModeRadioGroup.check(mAGPSModeRadioGroupItemId[0]);
        } else if ("msb".equals(mDefault)) {
            mAGPSModeRadioGroup.check(mAGPSModeRadioGroupItemId[1]);
        } else if ("msa".equals(mDefault)) {
            mAGPSModeRadioGroup.check(mAGPSModeRadioGroupItemId[2]);
        }
        setConfigration(mGpsConfigFile, "SUPL-MODE", mDefault);

        resetAGPSTextViewItemStatus(mSLPAddressTextView, "SERVER-ADDRESS", getResources()
                .getString(R.string.default_server_address));

        resetAGPSTextViewItemStatus(mSLPPortTextView, "SERVER-PORT", getResources()
                .getString(R.string.default_server_port));

        resetAGPSCheckBoxItemStatus(mAutoResetCheckBox, "TLS-ENABLE",
                getResources().getBoolean(R.bool.default_tls_enable));

        mDefault = getResources().getString(R.string.default_supl_version);
        setConfigration(mGpsConfigFile, "SUPL-VERSION", mDefault);
        if (showSUPL2View(mDefault)) {
            mAGPSSUPL2Layout.setVisibility(View.VISIBLE);
            mEnableSUPL2CheckBox.setChecked(true);
        } else {
            mAGPSSUPL2Layout.setVisibility(View.GONE);
            mEnableSUPL2CheckBox.setChecked(false);
        }

        mDefault = getResources().getString(R.string.default_set_id);
        if ("IPV4".equals(mDefault)) {
            mSetIDRadioGroup.check(mSetIDRadioGroupItemId[0]);
        } else if ("IMSI".equals(mDefault)) {
            mSetIDRadioGroup.check(mSetIDRadioGroupItemId[1]);
        }
        setConfigration(mGpsConfigFile, "SETID", mDefault);

        mDefault = getResources().getString(R.string.default_accuracy_unit);
        if ("KVALUE".equals(mDefault)) {
            mAccuracyUnitRadioGroup.check(mAccuracyUnitRadioGroupItemId[0]);
        } else if ("METER".equals(mDefault)) {
            mAccuracyUnitRadioGroup.check(mAccuracyUnitRadioGroupItemId[1]);
        }
        setConfigration(mGpsConfigFile, "ACCURACY-UNIT", mDefault);

        resetAGPSTextViewItemStatus(mHorizontalAccuracyTextView, "HORIZON-ACCURACY", getResources()
                .getString(R.string.default_horizon_accuracy));
        resetAGPSTextViewItemStatus(mVerticalAccuracyTextView, "VERTICAL-ACCURACY", getResources()
                .getString(R.string.default_vertical_accuracy));
        resetAGPSTextViewItemStatus(mLocationAgeTextView, "LOC-AGE", getResources()
                .getString(R.string.default_loc_age));
        resetAGPSTextViewItemStatus(mDelayTextView, "DELAY", getResources()
                .getString(R.string.default_delay));

        isTure = getResources().getBoolean(R.bool.default_cer_verify);
        resetAGPSCheckBoxItemStatus(mCertificateVerificationCheckBox, "CER-VERIFY", isTure);
        mCertificateVerificationEditButton.setEnabled(isTure);

        setConfigration(mGpsConfigFile, "SUPL-CER",
                getResources().getString(R.string.default_supl_cer));

        resetAGPSCheckBoxItemStatus(mEnableLabIOTTestCheckBox, "LAB-IOT",
                getResources().getBoolean(R.bool.default_lab_iot));
        resetAGPSCheckBoxItemStatus(mEnableeCIDCheckBox, "ECID-ENABLE",
                getResources().getBoolean(R.bool.default_enable_ecid));

        resetAGPSTextViewItemStatus(mMSISDNTextView, "LOCAL-MSISDN", getResources()
                .getString(R.string.default_local_msisdn));
        resetAGPSTextViewItemStatus(m3rdMSISDNTextView, "3RD-MSISDN", getResources()
                .getString(R.string.default_3rd_msisdn));

        mDefault = getResources().getString(R.string.default_trigger_type);
        if ("PERIODIC".equals(mDefault)) {
            mTriggerTypeRadioGroup.check(mTriggerTypeGroupItemId[0]);
            mAreaTypeLayout.setVisibility(View.GONE);
            mPerodicTypeLayout.setVisibility(View.VISIBLE);
        } else if ("AREA".equals(mDefault)) {
            mTriggerTypeRadioGroup.check(mTriggerTypeGroupItemId[1]);
            mAreaTypeLayout.setVisibility(View.VISIBLE);
            mPerodicTypeLayout.setVisibility(View.GONE);
        }
        setConfigration(mGpsConfigFile, "TRIGGER-TYPE", mDefault);

        String periodic_posmethod = getResources().getString(R.string.default_periodic_posmethod);
        setConfigration(mGpsConfigFile, "PERIODIC-POSMETHOD", periodic_posmethod);
        String area_posmethod = getResources().getString(R.string.default_area_posmethod);
        setConfigration(mGpsConfigFile, "AREA-POSMETHOD", area_posmethod);

        List<String> mPosMethodArrayValuesList = Arrays.asList(mPosMethodArrayValues);

        if ("PERIODIC".equals(mDefault)) {
            mPosMethodView.setText(Arrays.asList(mPosMethodArray).get(
                    mPosMethodArrayValuesList.indexOf(periodic_posmethod)));
        } else if ("AREA".equals(mDefault)) {
            mPosMethodView.setText(Arrays.asList(mPosMethodArray).get(
                    mPosMethodArrayValuesList.indexOf(area_posmethod)));
        }

        resetAGPSTextViewItemStatus(mAreaTypeTextView, "AREA-TYPE", getResources()
                .getString(R.string.default_area_type));

        resetAGPSTextViewItemStatus(mPerodicMinIntervalTextView, "PERIODIC-MIN-INTERVAL",
                getResources()
                        .getString(R.string.default_periodic_min_interval));
        resetAGPSTextViewItemStatus(mPerodicStartTimeTextView, "PERIODIC-START-TIME",
                getResources()
                        .getString(R.string.default_periodic_start_time));
        resetAGPSTextViewItemStatus(mPerodicStopTimeTextView, "PERIODIC-STOP-TIME", getResources()
                .getString(R.string.default_periodic_stop_time));

        resetAGPSTextViewItemStatus(mAreaMinIntervalTextView, "AREA-MIN-INTERVAL", getResources()
                .getString(R.string.default_area_min_interval));
        resetAGPSTextViewItemStatus(mMaxNumTextView, "MAX-NUM", getResources()
                .getString(R.string.default_max_num));
        resetAGPSTextViewItemStatus(mAreaStartTimeTextView, "AREA-START-TIME", getResources()
                .getString(R.string.default_area_start_time));
        resetAGPSTextViewItemStatus(mAreaStopTimeTextView, "AREA-STOP-TIME", getResources()
                .getString(R.string.default_area_stop_time));
        resetAGPSTextViewItemStatus(mGeographicTextView, "GEOGRAPHIC", getResources()
                .getString(R.string.default_geographic));
        resetAGPSTextViewItemStatus(mGeoRadiusTextView, "GEORADIUS", getResources()
                .getString(R.string.default_georadius));
        resetAGPSTextViewItemStatus(mLatitudeTextView, "GEO-LAT", getResources()
                .getString(R.string.default_geo_lat));
        resetAGPSTextViewItemStatus(mLongitudeTextView, "GEO-LON", getResources()
                .getString(R.string.default_geo_lon));

        mDefault = getResources().getString(R.string.default_sign);
        if ("NORTH".equals(mDefault)) {
            mSignRadioGroup.check(mSignRadioGroupItemId[0]);
        } else if ("SOUTH".equals(mDefault)) {
            mSignRadioGroup.check(mSignRadioGroupItemId[1]);
        }
        setConfigration(mGpsConfigFile, "SIGN", mDefault);

        // reset common
        resetAGPSCheckBoxItemStatus(mAllowNetworkInitiatedRequestCheckBox, "NI-ENABLE",
                getResources().getBoolean(R.bool.default_ni_enable));
        resetAGPSCheckBoxItemStatus(mAllowEMNotificationCheckBox, "EM-NOTIFY",
                getResources().getBoolean(R.bool.default_em_notify));
        resetAGPSCheckBoxItemStatus(mAllowRoamingCheckBox, "ROAMING-ENABLE",
                getResources().getBoolean(R.bool.default_roaming_enable));
        resetAGPSCheckBoxItemStatus(mLogSUPLToFileCheckBox, "SUPLLOG-SAVE",
                getResources().getBoolean(R.bool.default_supllog_save));

        mDefault = getResources().getString(R.string.default_notify_timeout);
        setConfigration(mGpsConfigFile, "NOTIFY-TIMEOUT", mDefault);
        mNotificationTimeoutSpinner.setSelection(Integer.parseInt(mDefault) - 1);

        mDefault = getResources().getString(R.string.default_verify_timeout);
        setConfigration(mGpsConfigFile, "VERIFY-TIMEOUT", mDefault);
        mVerificationTimeoutSpinner.setSelection(Integer.parseInt(mDefault) - 1);

        setConfigration(mGpsConfigFile, "NI-TEST",
                getResources().getString(R.string.default_ni_test));

    }

    private AlertDialog.Builder setEditTextDialogBuilder(AlertDialog.Builder builder,
            final TextView tv, int inputType,
            final String key, String limitdigits, int maxlength, String hint) {
        final EditText et = new EditText(this);
        et.setInputType(inputType);
        et.setText(tv.getText());
        if (maxlength > 0) {
            et.setFilters(new InputFilter[] {
                    new InputFilter.LengthFilter(maxlength)
            });
        }
        if (limitdigits != null) {
            et.setKeyListener(DigitsKeyListener.getInstance(limitdigits));
        }
        if (hint != null) {
            et.setHint(hint);
        }
        builder.setTitle(R.string.edit);
        CharSequence text = et.getText();
        if (text instanceof Spannable) {
            Spannable spanText = (Spannable) text;
            Selection.setSelection(spanText, text.length());
        }
        builder.setView(et);
        builder.setPositiveButton(R.string.dlg_ok,
                new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        String value = et.getText().toString();
                        setConfigration(mGpsConfigFile, key, value);
                        tv.setText(value);
                    }
                });
        builder.setNegativeButton(R.string.dlg_cancel, null);
        return builder;
    }

    private void resetAGPSTextViewItemStatus(TextView item, String key, String value) {
        setConfigration(mGpsConfigFile, key, value);
        item.setText(value);
    }

    private void resetAGPSCheckBoxItemStatus(CheckBox item, String key, boolean value) {
        setConfigration(mGpsConfigFile, key, value ? "TRUE" : "FALSE");
        item.setChecked(value);
    }

    private void initAGPSTextViewItemStatus(TextView item, String key) {
        String value = getConfigration(mGpsConfigFile, key);
        item.setText(value);
    }

    private void initAGPSCheckBoxItemStatus(CheckBox item, String key) {
        String enable = getConfigration(mGpsConfigFile, key);
        if ("TRUE".equals(enable)) {
            item.setChecked(true);
        } else {
            item.setChecked(false);
        }
    }

    private void setCheckBoxListener(CheckBox item, final String key) {
        item.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                setConfigration(mGpsConfigFile, key,
                        (isChecked ? "TRUE" : "FALSE"));
            }
        });
    }

    private String getConfigration(String path, String key) {
        String tmpConfig = null;
        tmpConfig = getAttrVal(path, "PROPERTY", key);
        return tmpConfig;
    }

    private boolean setConfigration(String path, String key, String newValue) {
        boolean setValues = setAttrVal(path, "PROPERTY", key, newValue);
        Log.e(TAG, "setValues  = " + setValues);
        if (!setValues) {
            Log.e(TAG, "config " + key + " error");
            return false;
        }
        return setValues;
    }

    /**
     * @param path the xml file entire path
     * @return the attribute value of the key.
     */
    private String getAttrVal(String path, String elementName, String key) {
        String val = null;
        try {
            DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
            DocumentBuilder db = dbf.newDocumentBuilder();
            File gpsconfig = new File(path);
            Document doc = db.parse(gpsconfig);

            NodeList list = doc.getElementsByTagName(elementName);
            for (int i = 0; i < list.getLength(); i++) {
                Element element = (Element) list.item(i);
                // SPRD: element may be null.
                if (element != null && element.getAttribute("NAME").equals(key)) {
                    val = element.getAttribute("VALUE");
                    break;
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Exception :", e);
        }
        return val;
    }

    /**
     * @param path the xml file entire path
     * @param newVal the new value will be set.
     * @return true if set attribute value successfully, false otherwise.
     */
    private boolean setAttrVal(String path, String elementName, String key, String newVal) {
        try {
            DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
            DocumentBuilder db = dbf.newDocumentBuilder();
            File gpsconfig = new File(path);
            Document doc = db.parse(gpsconfig);

            NodeList list = doc.getElementsByTagName(elementName);
            for (int i = 0; i < list.getLength(); i++) {
                Element element = (Element) list.item(i);
                if (element.getAttribute("NAME").equals(key)) {
                    element.setAttribute("VALUE", newVal);
                    break;
                }
            }

            TransformerFactory transformerFactory = TransformerFactory.newInstance();
            Transformer transformer = transformerFactory.newTransformer();
            DOMSource domSource = new DOMSource(doc);
            transformer.setOutputProperty(OutputKeys.ENCODING, "utf-8");
            StreamResult result = new StreamResult(new FileOutputStream(path));
            transformer.transform(domSource, result);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Exception :", e);
            return false;
        }
    }

    private void setTriggerCommand(String command) {
        if (mLocationManager == null) {
            mLocationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
        }
        if (mLocationManager != null) {
            Bundle extras = new Bundle();
            extras.putBoolean(command, true);
            mLocationManager
                    .sendExtraCommand(LocationManager.GPS_PROVIDER, "delete_aiding_data",
                            extras);
        }
    }

    private boolean showSUPL2View(String version) {
        boolean show = false;
        if ("v2.0.0".equals(version)) {
            show = true;
        }
        return show;
    }

    private void setMOLATrigger(boolean on) {
        Log.d(TAG, "SettingsObserver selfChange is " + on);
        if(on){
            if(mSocketClient != null){
                sendCommand("$PSPRD,00,6,1");
            }
        }else{
            if(mSocketClient != null){
                sendCommand("$PSPRD,00,6,2");
            }
        }
    }


    private int getGNSSMode() {
        // bit0--gps,bit1--beidou,bit2--glonass, value = 1 is turn on, 0 is turn off. Default is gps+glonass
        // example : "101" is gps+glonass

        int gps_mode = GPS_ONLY;;
        String gpsModeValue = getConfigration(SgpsUtils.GPS_CSR_CONFIG_FIL_FOR_GE2, "CP-MODE");
        if ("001".equals(gpsModeValue)) {
            gps_mode = GPS_ONLY;
        } else if ("100".equals(gpsModeValue)) {
            gps_mode = GLONASS_ONLY;
        } else if ("010".equals(gpsModeValue)) {
            gps_mode = BDS_ONLY;
        } else if ("101".equals(gpsModeValue)) {
            gps_mode = GLONASS_GPS;
        } else if ("011".equals(gpsModeValue)) {
            gps_mode = GPS_BDS;
        }
        return gps_mode;
    }

    private void setGNSSMode(int mode) {
        Log.d(TAG, "set GNSS mode mode is " + mode);
        String saveValue = null;
        switch (mode) {
            case GPS_ONLY:
                saveValue = "001";
                break;
            case GLONASS_ONLY:
                saveValue = "100";
                break;
            case BDS_ONLY:
                saveValue = "010";
                break;
            case GLONASS_GPS:
                saveValue = "101";
                break;
            case GPS_BDS:
                saveValue = "011";
                break;
        }
        if (!TextUtils.isEmpty(saveValue)) {
            setConfigration(SgpsUtils.GPS_CSR_CONFIG_FIL_FOR_GE2, "CP-MODE", saveValue);
        } else {
            Log.d(TAG, "set GNSS mode fail because value is invalid !");
        }
    }
    public void setSatelliteStatusForGe2(List<com.spreadtrum.sgps.GpsSatellite> list){
        mGe2Cn_Sr = getSatelliteStatusForGe2(list);
        Log.d(TAG, "mGe2Cn_Sr is  "+mGe2Cn_Sr);
    }
    private String getSatelliteStatusForGe2(List<com.spreadtrum.sgps.GpsSatellite> list) {
        Log.d(TAG, "Enter private getSatelliteStatusForGe2 function");
        int[] mPrns_temp = new int[MAX_SATELLITES_NUMBER];
        float[] mSnrs_temp = new float[MAX_SATELLITES_NUMBER];
        if (null == list) {
            return "0";
        } else {
            int index = 0;
            for (com.spreadtrum.sgps.GpsSatellite sate : list) {
                mPrns_temp[index] = sate.getPrn();
                mSnrs_temp[index] = sate.getSnr();
                index++;
            }
            return toString(mPrns_temp, index) + ","
                    + toString(mSnrs_temp, index);
        }
    }
    public void updateSatelliteView(List<com.spreadtrum.sgps.GpsSatellite> list) {
        Log.v(TAG, "updateSatelliteView");
        if (null == list) {
            emptyArray();
        } else {
            synchronized (this) {
                emptyArray();
                int index = 0;
                for (com.spreadtrum.sgps.GpsSatellite sate : list) {
                    mPrns[index] = sate.getPrn();
                    mSnrs[index] = sate.getSnr();
                    mElevation[index] = sate.getElevation();
                    mAzimuth[index] = sate.getAzimuth();
                    if (sate.usedInFix()) {
                        int i = mPrns[index] - 1;
                        mUsedInFixMask[i
                                / SatelliteDataProvider.SATELLITES_MASK_BIT_WIDTH] |= (1 << (i % SatelliteDataProvider.SATELLITES_MASK_BIT_WIDTH));
                    }
                    index++;
                }
                mSatellites = index;
            }
        }
        mSatelliteView.postInvalidate();
        mSignalView.postInvalidate();
    }

    @Override
    public void locationFixed(boolean available) {
        Log.d(TAG, "locationFixed available : " + available);
        if (available) {
            mStatus = getString(R.string.gps_status_available);
        } else {
            clearLayout();
            mStatus = getString(R.string.gps_status_unavailable);
        }
    }

}
