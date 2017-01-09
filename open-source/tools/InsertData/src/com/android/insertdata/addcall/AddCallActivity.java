package com.android.insertdata.addcall;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.widget.Toast;
import java.util.Random;
import com.android.insertdata.R;

public class AddCallActivity extends Activity {
	private Button mConfirmCall;
	private EditText mMobile;
	private EditText mCallLogCount;
	private RadioGroup mRadioGroup;
	private RadioButton mReceivedCall, mMissedCall, mDialedCall, mRandomCall;
	private ProgressDialog pdialog;
	private RadioGroup radioGroup;
	private RadioButton radioButton1;
	private RadioButton radioButton2;

	private static int status = 0;
	private static long startting;
	private static long end;
	private static long time;
	private ToneGenerator mToneGenerator;
	private static final int TONE_LENGTH_MS = 200; // Delay time

	private static final int TONE_RELATIVE_VOLUME = 80;

	private static final int DIAL_TONE_STREAM_TYPE = AudioManager.STREAM_MUSIC; // 3

	private static final int RECEIVED_CALL = 1;
	private static final int MISSED_CALL = 3;
	private static final int DIALED_CALL = 2;

	private boolean flag = true;

	private int callLogType;
	private static int count;

	private static final String NUMBER = "number";
	private static final String DATE = "date";
	private static final String DURATION = "duration";
	private static final String NEW = "new";

	private static final Uri CONTENT_URI = Uri
			.parse("content://call_log/calls");
	private static final String TYPE = "type";
	private static final int duration = 500000;
	// public static final String CACHED_NAME = "name";
	// public static final String CACHED_NUMBER_TYPE = "numbertype";
	// public static final String CACHED_NUMBER_LABEL = "numberlabel";
	private static final int PHONE_NUMBER_LENGTH = 3;
	private static final String PHONE_NUMBER = "13800138000";
	private static final int PHONE_COUNT = 500;
	private static Long phoneNum;

	private boolean isNumber = false;

	private boolean isRandom = true;

	private static int flg = 2;
	private static String phoneNumber;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.add_calls);
		initData(); // Call method
	}

	/*
	 * Initialization of variables
	 */
	private void initData() {

		mMobile = (EditText) findViewById(R.id.mobile);
		mCallLogCount = (EditText) findViewById(R.id.call_log_count);
		mRadioGroup = (RadioGroup) findViewById(R.id.radio_group);
		mReceivedCall = (RadioButton) findViewById(R.id.received_call_log_type);
		mMissedCall = (RadioButton) findViewById(R.id.missed_call_log_type);
		mDialedCall = (RadioButton) findViewById(R.id.dialed_call_log_type);
		mRandomCall = (RadioButton) findViewById(R.id.random_call_log_type);
		mConfirmCall = (Button) findViewById(R.id.confirm_call);

		radioGroup = (RadioGroup) findViewById(R.id.rediogroup);
		radioButton1 = (RadioButton) findViewById(R.id.rb1);
		radioButton2 = (RadioButton) findViewById(R.id.rb2);

		mMobile.addTextChangedListener(new TextWatcher() {

			@Override
			public void onTextChanged(CharSequence s, int start, int before,
					int count) {
			}

			@Override
			public void beforeTextChanged(CharSequence s, int start, int count,
					int after) {
			}

			@Override
			public void afterTextChanged(Editable s) {

				if (s.toString().trim().matches("[0-9]+")
						|| s.toString().equals("")) {
					radioButton2.setEnabled(true);
					radioButton2.setChecked(true);
				} else {
					radioButton1.setChecked(true);
					radioButton2.setEnabled(false);
				}
			}
		});

		radioButton2.setChecked(true);

		radioGroup.setOnCheckedChangeListener(new OnCheckedChangeListener() {

			@Override
			public void onCheckedChanged(RadioGroup group, int checkedId) {

				if (checkedId == radioButton1.getId()) {
					flg = 1;
				} else if (checkedId == radioButton2.getId()) {
					flg = 2;
				}
			}
		});

		mRadioGroup
				.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {

					@Override
					public void onCheckedChanged(RadioGroup group, int checkedId) {

						if (checkedId == mReceivedCall.getId()) {
							callLogType = RECEIVED_CALL;
							isRandom = false;
						} else if (checkedId == mMissedCall.getId()) {
							callLogType = MISSED_CALL;
							isRandom = false;
						} else if (checkedId == mDialedCall.getId()) {
							callLogType = DIALED_CALL;
							isRandom = false;
						} else if (checkedId == mRandomCall.getId()) {
							isRandom = true;
						}
					}
				});
		mMobile.setHint(R.string.call_hint);
		mCallLogCount.setHint(R.string.call_count);
		mConfirmCall.setOnClickListener(new ClickListener());

	}

	private class ClickListener implements View.OnClickListener {

		@Override
		public void onClick(View v) {
			flag = true;
			if ("".equals(mMobile.getText().toString())
					|| mMobile.getText().toString() == null) {
				phoneNumber = PHONE_NUMBER.trim();

			} else {
				phoneNumber = mMobile.getText().toString().trim();
			}

			String callLogTypeCount = mCallLogCount.getText().toString().trim();
			if (isPhoneNumberValid(phoneNumber) == true) {
				pdialog = new ProgressDialog(AddCallActivity.this);
				pdialog.setButton(AddCallActivity.this.getResources()
						.getString(R.string.exit),
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int i) {

								flag = false;
							}
						});

				pdialog.setOnKeyListener(new DialogInterface.OnKeyListener() {

					public boolean onKey(DialogInterface dialog, int keyCode,
							KeyEvent event) {
						return true;
					}
				});

				pdialog.setOnDismissListener(new DialogInterface.OnDismissListener() {

					public void onDismiss(DialogInterface dialog) {
						playTone();

					}

					private void playTone() {
						AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE); // audio
						int ringerMode = audioManager.getRingerMode();

						// 静音或震动时不发声 Silent or vibrate without sounding
						if ((ringerMode == AudioManager.RINGER_MODE_SILENT) // 0
								|| (ringerMode == AudioManager.RINGER_MODE_VIBRATE)) { // 1

							return;
						}
						synchronized (this) {
							if (mToneGenerator == null) {
								try {
									mToneGenerator = new ToneGenerator(
											DIAL_TONE_STREAM_TYPE,
											TONE_RELATIVE_VOLUME);
									setVolumeControlStream(DIAL_TONE_STREAM_TYPE);
								} catch (RuntimeException e) {
									mToneGenerator = null;
								}
							}
							mToneGenerator.startTone(ToneGenerator.TONE_DTMF_3,
									TONE_LENGTH_MS);
						}
					}
				});
				if (callLogTypeCount.equals("") || callLogTypeCount == null) {
					count = PHONE_COUNT;
				} else {
					count = Integer.parseInt(callLogTypeCount);
				}

				if (isNumber == true) {
					phoneNum = Long.parseLong(phoneNumber);
				}

				pdialog.setTitle(AddCallActivity.this.getResources().getString(
						R.string.title));
				pdialog.setCancelable(true);
				pdialog.setMax(count);
				pdialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
				pdialog.show();
				if (flag) {

					new AddCallLogTask().execute(count);
				}
			} else {
				mMobile.setText("");
				Toast.makeText(AddCallActivity.this, R.string.call_error,
						Toast.LENGTH_LONG).show();
			}

		}

	}

	protected boolean isPhoneNumberValid(String phoneNumber) {
		boolean isValid = false;

		String phonNumberString = phoneNumber.replaceAll("[,;+]+", "");

		if (phoneNumber.length() >= PHONE_NUMBER_LENGTH) {
			if (phoneNumber.contains("+")
					&& phonNumberString.matches(("[0-9]+"))) {
				if (phoneNumber.startsWith("+")
						&& phoneNumber.substring(1).contains("+") == false) {
					isValid = true;

				}

			} else if ((phoneNumber.contains(",") || phoneNumber.contains(";"))
					&& phonNumberString.matches(("[0-9]+"))) {
				isValid = true;

			} else if (phoneNumber.matches(("[0-9]+"))) {

				isValid = true;
				isNumber = true;
			}
		}
		return isValid;
	}

	private class AddCallLogTask extends AsyncTask<Integer, Integer, String> {

		@Override
		protected void onPreExecute() {
			super.onPreExecute();

		}

		@Override
		protected String doInBackground(Integer... params) {

			startting = System.currentTimeMillis();
			insert();
			end = System.currentTimeMillis();
			time = end - startting;

			return null;
		}

		@Override
		protected void onPostExecute(String result) {
			super.onPostExecute(result);
			showProgressDialog();
		}

		@Override
		protected void onProgressUpdate(Integer... values) {
			super.onProgressUpdate(values);

		}

		private void insert() {
			int[] callType = { RECEIVED_CALL, MISSED_CALL, DIALED_CALL };
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
			int i = 0;
			if (isNumber == true) {
				while (flag && i < count) {

					status++;

					pdialog.setMessage(String.valueOf(i) + "/" + count);
					pdialog.setProgress(i);
					if (isRandom) {
                        // int index = (int) (Math.random() * 3);
                        int index = new Random().nextInt(3);
						callLogType = callType[index];
					}
					addCall(AddCallActivity.this, phoneNum.toString(),
							callLogType, /* start, */duration);
					i++;

					if (flg == 1) {

					} else if (flg == 2) {
						phoneNum++;
					}

				}

			} else {
				while (flag && i < count) {

					System.out.println(phoneNum);

					status++;

					System.out.println("isNotNumber status=" + status);
					pdialog.setMessage(String.valueOf(i) + "/" + count);
					pdialog.setProgress(i);
					if (isRandom) {
                        // int index = (int) (Math.random() * 3);
                        int index = new Random().nextInt(3);
						callLogType = callType[index];
					}
					addCall(AddCallActivity.this, phoneNumber, callLogType,
					/* start, */duration);
					i++;

				}
			}

			if (pdialog != null) {
				pdialog.dismiss();
			}
		}

		public Uri addCall(Context context, String number, int callType,
		/* long start, */int duration) {
			final ContentResolver resolver = context.getContentResolver();
			ContentValues values = new ContentValues(5);
			values.put(NUMBER, number.toString());
			values.put(TYPE, callType);
			values.put(DATE, System.currentTimeMillis());
			values.put(DURATION, Long.valueOf(duration));
			values.put(NEW, Integer.valueOf(1));
			Uri result = resolver.insert(CONTENT_URI, values);
			return result;
		}

	}

	private void showProgressDialog() {
		StringBuffer msg = new StringBuffer();
		msg.append(AddCallActivity.this.getResources()
				.getString(R.string.count));
		msg.append(status).append(
				AddCallActivity.this.getResources().getString(R.string.bar));
		msg.append("\n");
		msg.append(AddCallActivity.this.getResources()
				.getString(R.string.times));
		msg.append(time);
		msg.append(AddCallActivity.this.getResources().getString(
				R.string.seconds));

		Dialog dialog = new AlertDialog.Builder(AddCallActivity.this)
				.setTitle(R.string.callcountshow)
				.setMessage(msg.toString())
				.setPositiveButton(R.string.ok,
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog,
									int which) {
								mToneGenerator.stopTone();
								AddCallActivity.status = 0;
								AddCallActivity.time = 0;
								finish();
							}
						}).create();
		dialog.show();
	}

}