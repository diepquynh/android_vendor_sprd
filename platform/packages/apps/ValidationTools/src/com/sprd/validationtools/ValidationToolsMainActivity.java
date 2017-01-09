
package com.sprd.validationtools;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import android.telephony.TelephonyManager;
import android.util.Log;

import android.location.GpsSatellite;
import android.location.GpsStatus;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.UserHandle;
import android.os.Environment;
import android.os.EnvironmentEx;
import android.os.SystemProperties;
import android.provider.Settings;
import android.app.Activity;
import android.view.Menu;
import android.view.View;
import android.media.AudioManager;
import android.widget.ListView;
import android.app.ListActivity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.Intent;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.widget.Toast;

import com.sprd.validationtools.R;
import com.sprd.validationtools.engtools.BtTestUtil;
import com.sprd.validationtools.engtools.WifiTestUtil;
import com.sprd.validationtools.itemstest.BluetoothTestActivity;
import com.sprd.validationtools.itemstest.GpsTestActivity;
import com.sprd.validationtools.itemstest.ListItemTestActivity;
import com.sprd.validationtools.itemstest.SDCardTest;
import com.sprd.validationtools.itemstest.SIMCardTestActivity;
import com.sprd.validationtools.itemstest.TestResultActivity;
import com.sprd.validationtools.itemstest.BackgroundTestActivity;
import com.sprd.validationtools.itemstest.WifiTestActivity;
import com.sprd.validationtools.sqlite.EngSqlite;
import com.sprd.validationtools.testinfo.TestInfoMainActivity;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;

public class ValidationToolsMainActivity extends Activity implements
        AdapterView.OnItemClickListener {
    private final static String TAG = "ValidationToolsMainActivity";
    private final static int FULL_TEST = 0;
    private final static int UNIT_TEST = 1;
    private final static int TEST_INFO = 2;
    private final static int RESET = 3;
    private String[] mListItemString;
    private ListView mListView;
    private ArrayList<TestItem> mAutoTestArray = null;
    private int mAutoTestCur = 0;
    private int mUserId;

    private ArrayList<BackgroundTest> mBgTest = null;

    private boolean mSavedSoundEffect = false;
    private boolean mSavedLockSound = false;
    private  boolean mIsTested = false;
    public final static String IS_SYSTEM_TESTED = "is_system_tested";
    private SharedPreferences mPrefs;

    /*
     * Disable the Soundeffect for Phoneloopback,return the last status
     */
    private boolean setSoundEffect(boolean isOn) {
        AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
        if (isOn) {
            audioManager.loadSoundEffects();
        } else {
            audioManager.unloadSoundEffects();
        }
        boolean lastStatus = (Settings.System.getInt(getContentResolver(),
                Settings.System.SOUND_EFFECTS_ENABLED, 0) == 1);
        Settings.System.putInt(getContentResolver(),
                Settings.System.SOUND_EFFECTS_ENABLED, isOn ? 1 : 0);
        return lastStatus;
    }

    /*
     * Disable the LockSound for Phoneloopback,return the last status
     */
    private boolean setLockSound(boolean isOn) {
        boolean lastStatus = (Settings.System.getInt(getContentResolver(),
                Settings.System.LOCKSCREEN_SOUNDS_ENABLED, 0) == 1);
        Settings.System.putInt(getContentResolver(),
                Settings.System.LOCKSCREEN_SOUNDS_ENABLED, isOn ? 1 : 0);
        return lastStatus;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_validation_tools_main);
        mListItemString = new String[] {
                this.getString(R.string.full_test),
                this.getString(R.string.item_test),
                this.getString(R.string.test_info),
                this.getString(R.string.reset)
        };
        mListView = (ListView) findViewById(R.id.ValidationToolsList);
        ArrayAdapter<String> arrayAdapter = new ArrayAdapter<String>(this,
                R.layout.simple_list_item, mListItemString);

        mListView.setAdapter(arrayAdapter);
        mListView.setOnItemClickListener(this);
        mPrefs = PreferenceManager.getDefaultSharedPreferences(this);
        mIsTested = mPrefs.getBoolean(IS_SYSTEM_TESTED, false);
        mUserId = UserHandle.myUserId();

    }

    @Override
    public void onPause() {
    	if(mUserId==0){
    		saveTestInfo();
        }
    	
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        if (mUserId == 0) {
        	setSoundEffect(mSavedSoundEffect);
        	setLockSound(mSavedLockSound);
        }
        super.onDestroy();
    }

    public void onActivityResult(int requestCode, int resultCode, Intent intent) {
        if (resultCode == Const.TEST_ITEM_DONE) {
            autoTest();
        }
    }

    private void autoTest() {
        if (mAutoTestArray != null && mAutoTestCur < mAutoTestArray.size()) {
            Intent intent = new Intent(ValidationToolsMainActivity.this,
                    mAutoTestArray.get(mAutoTestCur).getTestClass());
            intent.putExtra(Const.INTENT_PARA_TEST_NAME,
                    mAutoTestArray.get(mAutoTestCur).getTestname());
            intent.putExtra(Const.INTENT_PARA_TEST_INDEX, mAutoTestCur);
            startActivityForResult(intent, 0);

            mAutoTestCur++;
        } else if (mBgTest != null && mAutoTestArray != null) {
            EngSqlite engSqlite = EngSqlite.getInstance(this);
            addFailedBgTestToTestlist();
            String result = "";
            result += getResources().getString(R.string.bg_test_notice)
                    + "\n\n";

            for (BackgroundTest bgTest : mBgTest) {
                bgTest.stopTest();
                engSqlite.updateDB(
                        Const.IS_SUPPORT_LED_TEST ? Const.ALL_TEST_ITEM_NAME[bgTest
                                .getTestItemIdx()] : Const.ALL_TEST_ITEM_NAME2[bgTest
                                .getTestItemIdx()],
                        bgTest.getResult() == BackgroundTest.RESULT_PASS ? Const.SUCCESS
                                : Const.FAIL);
                result += bgTest.getResultStr();
                result += "\n\n";
            }

            Intent intent = new Intent(ValidationToolsMainActivity.this,
                    BackgroundTestActivity.class);
            intent.putExtra(Const.INTENT_BACKGROUND_TEST_RESULT, result);
            startActivityForResult(intent, 0);
            mBgTest = null;
        } else {
            Intent intent = new Intent(ValidationToolsMainActivity.this,
                    TestResultActivity.class);
            startActivity(intent);
        }
    }

    private void addFailedBgTestToTestlist() {

        for (BackgroundTest bgTest : mBgTest) {
            if (bgTest.getResult() != BackgroundTest.RESULT_PASS) {
                TestItem item = new TestItem(bgTest.getTestItemIdx());
                mAutoTestArray.add(item);
            }
        }
    }

    private void startBackgroundTest() {
        mBgTest = new ArrayList<BackgroundTest>();
        mBgTest.add(new BackgroundBtTest(this));
        mBgTest.add(new BackgroundWifiTest(this));
        mBgTest.add(new BackgroundGpsTest(this));
        mBgTest.add(new BackgroundSimTest(this));
        mBgTest.add(new BackgroundSdTest());
        for (BackgroundTest bgTest : mBgTest) {
            bgTest.startTest();
        }
    }
    public void saveTestInfo() {
            SharedPreferences.Editor editor = mPrefs.edit();
            editor.putBoolean(IS_SYSTEM_TESTED, mIsTested);
            editor.apply();
    }

    public void onResume(){
        super.onResume();
        mClickCount = 0;
    }

    private Integer mClickCount = 0;
    @Override
    public void onItemClick(AdapterView l, View v, int position, long id) {
        /* SPRD: bug453083 ,Multi user mode, set button is not click. {@ */
        if (mUserId != 0) {
            Toast.makeText(getApplicationContext(), R.string.multi_user_hint, Toast.LENGTH_LONG).show();
            return;
        }
        synchronized (mClickCount) {
            Log.d(TAG, "mClickCount:"+mClickCount);
            if (mClickCount > 0) {
            return;
            }
            mClickCount++;
        }
        mSavedSoundEffect = setSoundEffect(false);
        mSavedLockSound = setLockSound(false);
        /* @} */
        switch (position) {
            case FULL_TEST:
                mAutoTestArray = Const.getSupportAutoTestList(this);
                mAutoTestCur = 0;
                mIsTested = true;
                startBackgroundTest();
                autoTest();
                break;
            case UNIT_TEST: {
                Intent intent = new Intent(this, ListItemTestActivity.class);
                startActivity(intent);
            }
                break;
            case TEST_INFO: {
                Intent intent = new Intent(this, TestInfoMainActivity.class);
                intent.putExtra(IS_SYSTEM_TESTED, mIsTested);
                startActivity(intent);
            }
                break;
            case RESET: {
                Intent intent = new Intent();
                intent.setAction("android.settings.BACKUP_AND_RESET_SETTINGS");

                startActivity(intent);
            }
                break;
        }
    }

    @Override
    public void onBackPressed() {
        if (mUserId != 0) {
            finish();
            return;
        }
    	
        if (!SystemProperties.get("ro.bootmode").contains("engtest")) {
            super.onBackPressed();
        }
    }
}

interface BackgroundTest {
    public static int RESULT_INVALID = -1;
    public static int RESULT_FAIL = 0;
    public static int RESULT_PASS = 1;

    public void startTest();

    public void stopTest();

    public int getResult();

    public String getResultStr();

    public int getTestItemIdx();
}

class BackgroundBtTest implements BackgroundTest {
    private BtTestUtil mBtTestUtil = null;
    private int testResult = RESULT_INVALID;
    private Context mContext = null;

    public BackgroundBtTest(Context context) {
        mContext = context;
    }

    @Override
    public void startTest() {
        mBtTestUtil = new BtTestUtil() {
            boolean mIsPass = false;

            public void btDeviceListAdd(BluetoothDevice device) {

                if (device != null) {
                    if (device.getBondState() != BluetoothDevice.BOND_BONDED) {
                        testResult = RESULT_PASS;
                        mIsPass = true;
                    }
                }
            }

            public void btDiscoveryFinished() {
                if (!mIsPass) {
                    testResult = RESULT_FAIL;
                }
            }
        };
        mBtTestUtil.startTest(mContext);
    }

    @Override
    public void stopTest() {
        mBtTestUtil.stopTest();
    }

    @Override
    public int getResult() {
        return testResult;
    }

    @Override
    public String getResultStr() {
        String btResult = "BlueTooth:";
        if (RESULT_PASS == testResult) {
            btResult += "PASS";
        } else {
            btResult += "FAIL";
        }

        return btResult;
    }

    @Override
    public int getTestItemIdx() {
        // SPRD: Modify for bug464743,Some phones don't support Led lights test.
        if(Const.IS_SUPPORT_LED_TEST){
            for (int i = 0; i < Const.ALL_TEST_ITEM.length; i++) {
                if (Const.ALL_TEST_ITEM[i] == BluetoothTestActivity.class) {
                    return i;
                }
            }
        }else{
            for (int i = 0; i < Const.ALL_TEST_ITEM2.length; i++) {
                if (Const.ALL_TEST_ITEM2[i] == BluetoothTestActivity.class) {
                    return i;
                }
            }
        }

        return -1;
    }
}

class BackgroundGpsTest implements BackgroundTest {
    private static final String TAG = "BackgroundGpsTest";
    private LocationListener locationListener = null;
    private LocationManager manager = null;
    private GpsStatus.Listener gpsStatusListener = null;
    private int testResult = RESULT_INVALID;
    private static final long UPDATE_MIN_TIME = 1000;
    private static final int SATELLITE_COUNT_MIN = 4;
    private Context mContext = null;

    public BackgroundGpsTest(Context context) {
        mContext = context;
    }

    @Override
    public void startTest() {
        locationListener = new LocationListener() {

            public void onLocationChanged(Location location) {
            }

            public void onProviderDisabled(String provider) {

            }

            public void onProviderEnabled(String provider) {
            }

            public void onStatusChanged(String provider, int status,
                    Bundle extras) {
            }
        };

        gpsStatusListener = new GpsStatus.Listener() {
            public void onGpsStatusChanged(int event) {
                Log.d(TAG, " " + event);
                if (event == GpsStatus.GPS_EVENT_SATELLITE_STATUS) {
                    GpsStatus status = manager.getGpsStatus(null);
                    Iterator<GpsSatellite> iterator = status.getSatellites()
                            .iterator();
                    int count = 0;
                    boolean flag = false;
                    while (iterator.hasNext()) {
                        Log.d(TAG, "has next");
                        count++;
                        GpsSatellite gpsSatellite = iterator.next();
                        float snr = gpsSatellite.getSnr();
                        Log.d(TAG, "snr = "+snr);
                        if (snr > 35.0)
                            flag = true;
                    }
                    if (count >= SATELLITE_COUNT_MIN && flag){
                        testResult = RESULT_PASS;
                    }
                }
            }
        };
        manager = (LocationManager) mContext
                .getSystemService(Context.LOCATION_SERVICE);
        Settings.Secure.setLocationProviderEnabled(
                mContext.getContentResolver(), LocationManager.GPS_PROVIDER,
                true);
        try {
            manager.requestLocationUpdates(LocationManager.GPS_PROVIDER,
                    UPDATE_MIN_TIME, 0, locationListener);
            manager.addGpsStatusListener(gpsStatusListener);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void stopTest() {
        if (gpsStatusListener != null) {
            manager.removeGpsStatusListener(gpsStatusListener);
        }
        if (locationListener != null) {
            manager.removeUpdates(locationListener);
        }
    }

    @Override
    public int getResult() {
        return testResult;
    }

    @Override
    public String getResultStr() {
        String btResult = "GPS:";
        if (RESULT_PASS == testResult) {
            btResult += "PASS";
        } else {
            btResult += "FAIL";
        }

        return btResult;
    }

    @Override
    public int getTestItemIdx() {
        if(Const.IS_SUPPORT_LED_TEST){
            for (int i = 0; i < Const.ALL_TEST_ITEM.length; i++) {
                if (Const.ALL_TEST_ITEM[i] == GpsTestActivity.class) {
                    return i;
                }
            }
        }else{
            for (int i = 0; i < Const.ALL_TEST_ITEM2.length; i++) {
                if (Const.ALL_TEST_ITEM2[i] == GpsTestActivity.class) {
                    return i;
                }
            }
        }
        return -1;
    }
}

class BackgroundWifiTest implements BackgroundTest {
    private WifiTestUtil wifiTestUtil = null;
    private int testResult = RESULT_INVALID;
    private Context mContext = null;

    public BackgroundWifiTest(Context context) {
        mContext = context;
    }

    @Override
    public void startTest() {
        wifiTestUtil = new WifiTestUtil(
                (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE)) {

            public void wifiDeviceListChange(List<ScanResult> wifiDeviceList) {
                if (wifiDeviceList != null) {
                    testResult = RESULT_PASS;
                    wifiTestUtil.stopTest();
                }
            }
        };
        wifiTestUtil.startTest(mContext);
    }

    @Override
    public void stopTest() {
        wifiTestUtil.stopTest();
    }

    @Override
    public int getResult() {
        return testResult;
    }

    @Override
    public String getResultStr() {
        String btResult = "Wifi:";
        if (RESULT_PASS == testResult) {
            btResult += "PASS";
        } else {
            btResult += "FAIL";
        }

        return btResult;
    }

    @Override
    public int getTestItemIdx() {
        if(Const.IS_SUPPORT_LED_TEST){
            for (int i = 0; i < Const.ALL_TEST_ITEM.length; i++) {
                if (Const.ALL_TEST_ITEM[i] == WifiTestActivity.class) {
                    return i;
                }
            }
        }else{
            for (int i = 0; i < Const.ALL_TEST_ITEM2.length; i++) {
                if (Const.ALL_TEST_ITEM2[i] == WifiTestActivity.class) {
                    return i;
                }
            }
        }

        return -1;
    }
}

class BackgroundSimTest implements BackgroundTest {
    private int testResult = RESULT_INVALID;
    private Context mContext = null;

    public BackgroundSimTest(Context context) {
        mContext = context;
    }

    @Override
    public void startTest() {
        new Thread() {
            public void run() {
                int phoneCount = TelephonyManager.from(mContext).getPhoneCount();
                TelephonyManager telMgr = null;
                int readyCount = 0;

                //modify 336688 by sprd
                //Single card and Multi card get TelephonyManager method is different
                if(phoneCount == 1){
                    telMgr = (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
                    if (telMgr.getSimState() == TelephonyManager.SIM_STATE_READY) {
                        readyCount++;
                    }
               /* SPRD:394857 system test wrong in sim background test @{*/
                }else{
                    telMgr = (TelephonyManager) mContext
                            .getSystemService(Context.TELEPHONY_SERVICE);
                    for (int i = 0; i < phoneCount; i++) {
                        if (telMgr.getSimState(i) == TelephonyManager.SIM_STATE_READY) {
                            readyCount++;
                        }
                    }
                }
                /* @}*/

                if (readyCount == phoneCount) {
                    testResult = RESULT_PASS;
                }
            }
        }.start();
    }

    @Override
    public void stopTest() {
    }

    @Override
    public int getResult() {
        return testResult;
    }

    @Override
    public String getResultStr() {
        String btResult = "Sim:";
        if (RESULT_PASS == testResult) {
            btResult += "PASS";
        } else {
            btResult += "FAIL";
        }

        return btResult;
    }

    @Override
    public int getTestItemIdx() {
        if(Const.IS_SUPPORT_LED_TEST){
            for (int i = 0; i < Const.ALL_TEST_ITEM.length; i++) {
                if (Const.ALL_TEST_ITEM[i] == SIMCardTestActivity.class) {
                    return i;
                }
            }
        }else{
            for (int i = 0; i < Const.ALL_TEST_ITEM2.length; i++) {
                if (Const.ALL_TEST_ITEM2[i] == SIMCardTestActivity.class) {
                    return i;
                }
            }
        }

        return -1;
    }
}

class BackgroundSdTest implements BackgroundTest {
    private int testResult = RESULT_INVALID;
    private static final String SPRD_SD_TESTFILE = "sprdtest.txt";
    private static final String PHONE_STORAGE_PATH = "/data/data/com.sprd.validationtools/";
    byte[] mounted = new byte[2];
    Thread thread = null;

    @Override
    public void startTest() {
        thread = new Thread() {
            public void run() {
                if (!EnvironmentEx.getExternalStoragePathState().equals(
                        Environment.MEDIA_MOUNTED)) {
                    mounted[0] = 1;
                } else {
                    mounted[0] = 0;
                }
                // if (!Environment.getInternalStoragePathState().equals(
                // Environment.MEDIA_MOUNTED)) {
                // mounted[1] = 1;
                // } else {
                mounted[1] = 0;
                // }

                if (mounted[0] == 1 || mounted[1] == 1) {
                    testResult = RESULT_FAIL;
                    return;
                }

                FileInputStream in = null;
                FileOutputStream out = null;
                try {
                    byte mSDCardTestFlag[] = new byte[1];
                    byte[] result = new byte[2];

                    if (mounted[0] == 0) {
                        File fp = new File(
                                EnvironmentEx.getExternalStoragePath(),
                                SPRD_SD_TESTFILE);
                        if (fp.exists())
                            fp.delete();
                        fp.createNewFile();
                        out = new FileOutputStream(fp);
                        mSDCardTestFlag[0] = '6';
                        out.write(mSDCardTestFlag, 0, 1);
                        out.close();
                        in = new FileInputStream(fp);
                        in.read(mSDCardTestFlag, 0, 1);
                        in.close();
                        if (mSDCardTestFlag[0] == '6') {
                            result[0] = 0;
                        } else {
                            result[0] = 1;
                        }
                    }
                    if (mounted[1] == 0) {
                        File fp = new File(PHONE_STORAGE_PATH, SPRD_SD_TESTFILE);
                        if (fp.exists())
                            fp.delete();
                        fp.createNewFile();
                        out = new FileOutputStream(fp);
                        mSDCardTestFlag[0] = 'd';
                        out.write(mSDCardTestFlag, 0, 1);
                        out.close();
                        in = new FileInputStream(fp);
                        in.read(mSDCardTestFlag, 0, 1);
                        in.close();
                        if (mSDCardTestFlag[0] == 'd') {
                            result[1] = 0;
                        } else {
                            result[1] = 1;
                        }
                    }

                    if (result[0] == 0 && result[1] == 0) {
                        testResult = RESULT_PASS;
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                    try {
                        if (out != null) {
                            out.close();
                            out = null;
                        }
                        if (in != null) {
                            in.close();
                            in = null;
                        }
                    } catch (IOException io) {
                        io.printStackTrace();
                    }
                } finally {
                    try {
                        if (out != null) {
                            out.close();
                            out = null;
                        }
                        if (in != null) {
                            in.close();
                            in = null;
                        }
                    } catch (IOException io) {
                        io.printStackTrace();
                    }
                }
            }
        };
        thread.start();
    }

    @Override
    public void stopTest() {
    }

    @Override
    public int getResult() {
        return testResult;
    }

    @Override
    public String getResultStr() {
        String btResult = "SD:";
        if (RESULT_PASS == testResult) {
            btResult += "PASS";
        } else {
            btResult += "FAIL";
        }

        return btResult;
    }

    @Override
    public int getTestItemIdx() {
        /* SPRD: Modify for bug464743,Some phones don't support Led lights test.{@ */
        if(Const.IS_SUPPORT_LED_TEST){
            for (int i = 0; i < Const.ALL_TEST_ITEM.length; i++) {
                if (Const.ALL_TEST_ITEM[i] == SDCardTest.class) {
                    return i;
                }
            }
        }else{
            for (int i = 0; i < Const.ALL_TEST_ITEM2.length; i++) {
                if (Const.ALL_TEST_ITEM2[i] == SDCardTest.class) {
                    return i;
                }
            }
        }
        /* @} */
        return -1;
    }
}
