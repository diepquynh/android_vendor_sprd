package com.sprd.engineermode.telephony;

import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;

import android.app.AlertDialog.Builder;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.widget.Toast;
import android.os.Looper;
import android.content.SharedPreferences;
import android.os.HandlerThread;
import android.os.SystemProperties;
import android.preference.PreferenceManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.preference.TwoStatePreference;
import android.os.SystemProperties;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.net.LocalSocketAddress;
import android.text.TextUtils;
import java.io.IOException;
import java.io.InputStream;
import android.os.SystemClock;
import android.app.ProgressDialog;

import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.utils.SocketUtils;
public class Cp2AssertActivity extends PreferenceActivity implements
		Preference.OnPreferenceChangeListener {
	private static final String TAG = "Cp2AssertActivity";
	private static final String SOCKET_NAME = "wcnd";
	private static final String KEY_CP2_RESET = "cp2_reset";
	private static final String KEY_MANUAL_CP2_ASSERT = "manual_cp2_assert";
	private static final String MANUAL_CP2_ASSERT_CMD = "at+spatassert=1\r";
	private static final String AT_OPEN_CP2 = "poweron";
	private static final int BUF_SIZE = 255;

	private static final int POWER_ON = 0;
	private static final int MANUAL_CP2_ASSERT = 1;
	private static final int CP2_ALIVE = 2;
	private static final int LISTENING_WCN = 3;

	private TwoStatePreference mCp2AssertEnabled;
	private Preference mManualCp2Assert;

	private Handler mUiThread = new Handler();
	private Cp2AssertHandler mCp2AssertHandler;

	private final Object mObjectLock = new Object();

	private ProgressDialog mProgressDialog;
	private boolean isUser = SystemProperties.get("ro.build.type").equalsIgnoreCase("user");
    //bengin bug561938 modify by suyan.yang 20160606
    private boolean isMarlin = false;
    //end bug561938 modify by suyan.yang 20160606

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		HandlerThread ht = new HandlerThread(TAG);
		ht.start();
		mCp2AssertHandler = new Cp2AssertHandler(ht.getLooper());
		addPreferencesFromResource(R.xml.pref_cp2_assert);
		mCp2AssertEnabled = (TwoStatePreference) findPreference(KEY_CP2_RESET);
		mCp2AssertEnabled.setOnPreferenceChangeListener(this);
		mManualCp2Assert = findPreference(KEY_MANUAL_CP2_ASSERT);

        //bengin bug561938 add by suyan.yang 20160606
        String WcnProduct="";
        WcnProduct=SystemProperties.get("ro.wcn.hardware.product");
        if(!"".equals(WcnProduct) && WcnProduct.contains("marlin")){
            isMarlin=true;
        }
        //bengin bug561938 add by suyan.yang 20160606
	}

	@Override
	public void onStart() {
		super.onStart();
		if (mCp2AssertEnabled != null) {
		  //if wcn hardware product is not marlin, the function is not support
			if(!isMarlin){
				mCp2AssertEnabled.setSummary(R.string.feature_not_support);
                mCp2AssertEnabled.setEnabled(false);
			}
			boolean isCp2Reset = SystemProperties.getBoolean(
					"persist.sys.sprd.wcnreset", false);
			Log.d(TAG, "CP2 reset: " + isCp2Reset);
			if (isCp2Reset) {
				mCp2AssertEnabled.setChecked(true);
			} else {
				mCp2AssertEnabled.setChecked(false);
			}
		}
		if (mManualCp2Assert != null) {
			if(!isMarlin && isUser){
				mManualCp2Assert.setSummary(R.string.feature_not_support);
				mManualCp2Assert.setEnabled(false);
			}else{
				Message poweron = mCp2AssertHandler.obtainMessage(POWER_ON);
				mCp2AssertHandler.sendMessage(poweron);
			}
		}
	}

	@Override
	protected void onDestroy() {
		if (mCp2AssertHandler != null) {
			mCp2AssertHandler.getLooper().quit();
			Log.d(TAG, "HandlerThread has quit");
		}
		super.onDestroy();
	}

	@Override
	public boolean onPreferenceChange(Preference pref, Object newValue) {
		if (pref == mCp2AssertEnabled) {
			if (!mCp2AssertEnabled.isChecked()) {
				Log.d(TAG, "Open CP2 Reset Switch");
				SystemProperties.set("persist.sys.sprd.wcnreset", "1");
				mCp2AssertEnabled.setChecked(true);
			} else {
				Log.d(TAG, "Open CP2 Reset Switch");
				SystemProperties.set("persist.sys.sprd.wcnreset", "0");
				mCp2AssertEnabled.setChecked(false);
			}
			return true;
		}
		return false;
	}

	@Override
	public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
			Preference preference) {
		String key = preference.getKey();
		if (key.equals(KEY_MANUAL_CP2_ASSERT)) {
			mManualCp2Assert.setEnabled(false);
			Message manualCp2Assert = mCp2AssertHandler
					.obtainMessage(MANUAL_CP2_ASSERT);
			mCp2AssertHandler.sendMessage(manualCp2Assert);
		}
		return false;
	}

	private void connectToSocket(LocalSocket socket,
			LocalSocketAddress socketAddr) {
		try {
			socket.connect(socketAddr);
			Log.d(TAG, "wcnd socket connect is " + socket.isConnected());
		} catch (IOException ioe) {
			ioe.printStackTrace();
		}
	}

	private void runSocket(LocalSocket socket, LocalSocketAddress socketAddr) {
		final long endTime = SystemClock.elapsedRealtime() + 180000;
		String mes = null;
		byte[] buf = new byte[BUF_SIZE];
		connectToSocket(socket, socketAddr);

		Log.d(TAG, " -runSocket");
		synchronized (mObjectLock) {
			for (;;) {
				int cnt = 0;
				InputStream is = null;
				try {
					is = socket.getInputStream();
					cnt = is.read(buf, 0, BUF_SIZE);
					Log.d(TAG, "read " + cnt + " bytes from socket: \n");
				} catch (IOException e) {
					Log.d(TAG, "read exception\n");
				}
				if (cnt > 0) {
					String info = "";
					try {
						info = new String(buf, 0, cnt, "US-ASCII");
					} catch (UnsupportedEncodingException e) {
						e.printStackTrace();
					} catch (StringIndexOutOfBoundsException e) {
						e.printStackTrace();
					}
					Log.d(TAG, "read something: " + info);
					if (TextUtils.isEmpty(info)) {
						continue;
					}
					if (info.contains("WCN-CP2-ALIVE")) {
						try {
							if (is != null) {
								is.close();
								is = null;
							}
							if (socket != null && socket.isConnected()) {
								socket.close();
								socket = null;
							}
						} catch (Exception e) {
							Log.d(TAG, "catch exception is " + e);
						} finally {
							mes = "OK";
							Message cp2Alive = mCp2AssertHandler.obtainMessage(
									CP2_ALIVE, 0, 0, mes);
							mCp2AssertHandler.sendMessage(cp2Alive);
							break;
						}
					} else {
						Log.d(TAG, "do nothing with info :" + info);
					}
				} else if (cnt < 0) {
					try {
						is.close();
						socket.close();
					} catch (IOException e) {
						Log.d(TAG, "close exception\n");
					}
					socket = new LocalSocket();
					connectToSocket(socket, socketAddr);
				}
				if (SystemClock.elapsedRealtime() > endTime) {
					Log.d(TAG, "time out");
					try {
						if (is != null) {
							is.close();
							is = null;
						}
						if (socket != null && socket.isConnected()) {
							socket.close();
							socket = null;
						}
					} catch (Exception e) {
						Log.d(TAG, "catch exception is " + e);
					} finally {
						mes = "Fail";
						Message cp2AliveFail = mCp2AssertHandler.obtainMessage(
								CP2_ALIVE, 0, 0, mes);
						mCp2AssertHandler.sendMessage(cp2AliveFail);
						break;
					}
				}
			}
		}
	}

	private void showProgressDialog() {
		mUiThread.post(new Runnable() {
			@Override
			public void run() {
				mProgressDialog = ProgressDialog.show(Cp2AssertActivity.this,
						"Setting...", "Please wait...", true, false);
			}
		});
	}

	private void dismissProgressDialog() {
		mUiThread.post(new Runnable() {
			@Override
			public void run() {
				if (mProgressDialog != null) {
					mProgressDialog.dismiss();
				}
			}
		});
	}

	class Cp2AssertHandler extends Handler {
		public Cp2AssertHandler(Looper looper) {
			super(looper);
		}

		@Override
		public void handleMessage(Message msg) {
			String atResponse = null;
			switch (msg.what) {
			case POWER_ON:
				atResponse = SocketUtils.sendCmdAndRecResult(SOCKET_NAME,
						LocalSocketAddress.Namespace.ABSTRACT, "wcn "
								+ AT_OPEN_CP2);
				if (atResponse != null && atResponse.contains(SocketUtils.OK)) {
					Log.d(TAG, "Power on OK");
				} else {
					Log.d(TAG, "Power on OK");
				}
				break;
			case MANUAL_CP2_ASSERT:
				showProgressDialog();
				SocketUtils.sendCmdAndRecResult(SOCKET_NAME,
						LocalSocketAddress.Namespace.ABSTRACT, "wcn "
								+ MANUAL_CP2_ASSERT_CMD);
				boolean isCp2Reset = SystemProperties.getBoolean(
						"persist.sys.sprd.wcnreset", false);
				if (isCp2Reset) {
					Message listeningWcn = mCp2AssertHandler
							.obtainMessage(LISTENING_WCN);
					mCp2AssertHandler.sendMessage(listeningWcn);
				} else {
					dismissProgressDialog();
				}
				break;
			case LISTENING_WCN:
				try {
					LocalSocket socket = new LocalSocket();
					LocalSocketAddress socketAddr = new LocalSocketAddress(
							SOCKET_NAME, LocalSocketAddress.Namespace.ABSTRACT);
					runSocket(socket, socketAddr);
				} catch (Exception ex) {
					ex.printStackTrace();
				}
				break;
			case CP2_ALIVE:
				dismissProgressDialog();
				atResponse = (String) (String) msg.obj;
				Log.d(TAG, "CP2 Reset Alive: " + atResponse);
				if (atResponse != null && atResponse.contains("OK")) {
					mUiThread.post(new Runnable() {
						@Override
						public void run() {
							mManualCp2Assert.setEnabled(true);
							Toast.makeText(Cp2AssertActivity.this,
									"CP2 Reset Success", Toast.LENGTH_SHORT)
									.show();
						}
					});
				} else {
					mUiThread.post(new Runnable() {
						@Override
						public void run() {
							Toast.makeText(Cp2AssertActivity.this,
									"CP2 Reset Fail", Toast.LENGTH_SHORT)
									.show();
						}
					});
				}
				break;
			default:
				break;
			}
		}
	}
}
