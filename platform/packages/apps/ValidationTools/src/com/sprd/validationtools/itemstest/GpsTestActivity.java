
package com.sprd.validationtools.itemstest;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.R;

import android.content.Context;
import android.content.Intent;
import android.location.GpsSatellite;
import android.location.GpsStatus;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.os.Handler;
import android.provider.Settings;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import android.util.Log;

public class GpsTestActivity extends BaseActivity {
    private static final String TAG = "GpsTestActivity";
    /** GPS satellite count info name */
    public static final String GPS_SATELLITE_COUNT_NAME = "gpsSatelliteCount";

    /** GPS test flag info name */
    public static final String GPS_TEST_FLAG_NAME = "gpsTestFlag";

    /** the GPS settings package name */
    private static final String GPSSET_PACKAGE_NAME = "com.android.settings";

    /** the GPS settings class name */
    private static final String GPSSET_CLASS_NAME = "com.android.settings.Settings$LocationSettingsActivity";

    /** GPS provider name */
    private static final String PROVIDER = LocationManager.GPS_PROVIDER;

    /** location update min time */
    private static final long UPDATE_MIN_TIME = 1000;

    /** satellite min count for OK */
    private static final int SATELLITE_COUNT_MIN = 4;

    /** time count max : 5 minutes */
    // private static final int TIME_COUNT_MAX = 300;
    /** time count length : 1 second */
    private static final int TIME_LENGTH = 1000;

    /** location manager object */
    private LocationManager manager = null;

    /** location listener object */
    private LocationListener locationListener = null;

    /** gps status listener object */
    private GpsStatus.Listener gpsStatusListener = null;

    /** the text view object for show gps not enabled message */
    private TextView txtGpsMsg = null;

    /** the button that show gps settings activity */
    private Button btnShow = null;

    /** timer object */
    private Timer timer = null;

    /** not time left count */
    private int timeCount = 0;

    /** max satellite count that have been searched */
    private int mSatelliteCount;

    /** extra gps test result **/
    public static final String EXTRA_SET_FACTORY_SET_GPS_RESULT = "EXTRA_SET_FACTORY_SET_GPS_RESULT";

    /** extra auto test result **/

    public static final String INTENT_EXTRA_KEY_GPS_SEARCH_STAR_COUNT = "intentExtraGpsSearchStarCount";

    /** the success result of the gps test */
    private static final byte RESULT_SUCCESS = 1;

    /** the f result of the gps test */
    private static final byte RESULT_FAILURE = 0;

    private TextView mSatelliteInfo;
    public Handler mHandler = new Handler();

    private Runnable mR = new Runnable() {
        public void run() {
            if (mSatelliteCount > 0) {
                Toast.makeText(GpsTestActivity.this, R.string.text_pass,
                        Toast.LENGTH_SHORT).show();
                storeRusult(true);
            } else {
                Toast.makeText(GpsTestActivity.this, R.string.text_fail,
                        Toast.LENGTH_SHORT).show();
                storeRusult(false);
            }
            finish();
        }
    };

    @Override
    public void onClick(View v){
        mHandler.removeCallbacks(mR);
        super.onClick(v);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.gps_test_main);
        setTitle(R.string.gps_test);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);

        txtGpsMsg = (TextView) findViewById(R.id.txt_gps_not_enabled);
        mSatelliteInfo = (TextView) findViewById(R.id.txt_gps_satellite_info);
        mSatelliteInfo.setText("\n\n");
        mHandler.postDelayed(mR, 60000);
    }

    @Override
    public void onResume() {
        super.onResume();
        startTimer();
        showGpsMsg();
        showSatelliteCount();
    }

    @Override
    public void onPause() {
        cancelTimer();
        super.onPause();
    }

    @Override
    public void onStart() {
        super.onStart();
        locationListener = new LocationListener() {
            public void onLocationChanged(Location location) {
            }

            public void onProviderDisabled(String provider) {
                showGpsMsg();
            }

            public void onProviderEnabled(String provider) {
                showGpsMsg();
            }

            public void onStatusChanged(String provider, int status,
                    Bundle extras) {
            }
        };
        gpsStatusListener = new GpsStatus.Listener() {
            public void onGpsStatusChanged(int event) {
                Log.d(TAG, " " + event);
                if (event == GpsStatus.GPS_EVENT_SATELLITE_STATUS) {
                    showSatelliteCount();
                }
            }
        };
        manager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
        setTitle(R.string.gps_title_text);
         Settings.Secure.setLocationProviderEnabled(getContentResolver(),
        LocationManager.GPS_PROVIDER, true);
        try {
            manager.requestLocationUpdates(LocationManager.GPS_PROVIDER,
                    UPDATE_MIN_TIME, 0, locationListener);
            manager.addGpsStatusListener(gpsStatusListener);
        } catch (Exception e) {
            e.printStackTrace();
            Toast.makeText(this, getString(R.string.gps_open_err),
                    Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    public void onStop() {
        Settings.Secure.setLocationProviderEnabled(getContentResolver(),
        LocationManager.GPS_PROVIDER, false);
        if (gpsStatusListener != null) {
            manager.removeGpsStatusListener(gpsStatusListener);
        }
        if (locationListener != null) {
            manager.removeUpdates(locationListener);
        }
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        mHandler.removeCallbacks(mR);
        super.onDestroy();
    }

    private boolean isGpsEnabled() {
        if (manager == null) {
            return false;
        }

        return manager.isProviderEnabled(PROVIDER);
    }

    private void showGpsMsg() {
        if (!isGpsEnabled()) {
            // gps not enabled
            txtGpsMsg.setText(getString(R.string.gps_not_enabled_msg));
            // btnShow.setEnabled(true);
        } else {
            txtGpsMsg.setText("");
            // btnShow.setEnabled(false);
        }
    }

    @Override
    public void onBackPressed() {
        return ;
    }

    private void showSatelliteCount() {
        int count = 0;
        boolean flag = false;

        if (manager != null) {
            GpsStatus status = manager.getGpsStatus(null);
            Iterator<GpsSatellite> iterator = status.getSatellites().iterator();

            // get satellite count
            mSatelliteInfo.setText("\n\n");
            while (iterator.hasNext()) {
                Log.d(TAG, "has next");
                count++;
                GpsSatellite gpsSatellite = iterator.next();
                float snr = gpsSatellite.getSnr();
                Log.d(TAG, "snr = "+snr);
                if (snr > 35.0)
                    flag = true;
                mSatelliteInfo.append("id: ");
                mSatelliteInfo.append(String.valueOf(gpsSatellite.getPrn()));
                mSatelliteInfo.append("\nsnr: ");
                mSatelliteInfo.append(String.valueOf(snr));
                mSatelliteInfo.append("\n\n");
            }

            // satellite count is ok
            if (count >= SATELLITE_COUNT_MIN && flag) {
                Toast.makeText(GpsTestActivity.this, R.string.text_pass,
                        Toast.LENGTH_SHORT).show();
                storeRusult(true);
                finish();
            }

            // save max satellite count that have been searched
            if (count > mSatelliteCount) {
                mSatelliteCount = count;
            }
        }

        // show count
        TextView txtCount = (TextView) findViewById(R.id.txt_gps_satellite_count);
        txtCount.setText(" " + mSatelliteCount);

/*        if (flag) {
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(GpsTestActivity.this, R.string.text_pass, Toast.LENGTH_SHORT)
                            .show();
                    storeRusult(true);
                    finish();
                }
            }, 1000);
        }*/
    }

    private class TimerCountTask extends TimerTask {
        @Override
        public void run() {
            runOnUiThread(new Runnable() {
                public void run() {
                    // show time count
                    TextView txtTime = (TextView) findViewById(R.id.txt_gps_time_count);
                    txtTime.setText(timeCount + " ");
                }
            });

            timeCount++;
        }
    }

    private void startTimer() {
        timer = new Timer();
        timer.scheduleAtFixedRate(new TimerCountTask(), 0, TIME_LENGTH);
    }

    private void cancelTimer() {
        if (timer != null) {
            timer.cancel();
            timer = null;
        }
    }

    private Map<String, Byte> getGpsResultInfo() {
        byte result = RESULT_FAILURE;
        if (mSatelliteCount >= SATELLITE_COUNT_MIN) {
            result = RESULT_SUCCESS;
        }

        Map<String, Byte> info = new HashMap<String, Byte>();
        info.put(EXTRA_SET_FACTORY_SET_GPS_RESULT, result);
        info.put(INTENT_EXTRA_KEY_GPS_SEARCH_STAR_COUNT, (byte) mSatelliteCount);
        return info;
    }

}
