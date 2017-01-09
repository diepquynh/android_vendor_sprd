package com.android.insertdata.insertcontacts;

import java.util.ArrayList;
import java.util.Random;

import android.provider.ContactsContract;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.RawContacts;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.ContentProviderOperation;
import android.content.ContentProviderResult;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.OperationApplicationException;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Toast;
import com.android.insertdata.R;

public class InsertContactsActivity extends Activity {

	private static String DEBUG = "InsertContactsData";

	private char[] last;

	private char[] first;

	private String enName;

	private String lastName;

	private String firstName;

	private RadioGroup mRadioGroup;

	private RadioButton insertCN;

	private RadioButton insertEN;
	private RadioButton insertRandom;
	private static int count;

	private static final int INSERT_CN_TYPE = 0;
	private static final int INSERT_EN_TYPE = 1;
	private static final int INSERT_RANDOM_TYPE = 2;
	private static int INSERT_TYPE = INSERT_RANDOM_TYPE;

	private Handler myHandler;

	private static final int PROGRESS_DIALOG = 0;

	private static final int INCREASE = 0;

	private static final int FINISHDIALOG = 1; // insert cancle and close
												// progressdialog

	private static final int SHOWDIALOG = 2;

	private ProgressDialog pd;

	private Button mOkButton;

	private EditText inputName;

	private EditText inputNumber;

	private EditText addNumber;

	private volatile static boolean insertCancleFlag = false;

	private ToneGenerator mToneGenerator;
	private static final int TONE_LENGTH_MS = 200;

	/** The DTMF tone volume relative to other sounds in the stream */
	private static final int TONE_RELATIVE_VOLUME = 80;

	/**
	 * Stream type used to play the DTMF tones off call, and mapped to the
	 * volume control keys
	 */
	private static final int DIAL_TONE_STREAM_TYPE = AudioManager.STREAM_MUSIC;

	private ProgressDialog showMessage;
	private static long times;
	private long startting;
	private long end;

	// private static final String LOG_TAG = "insertContacts";
	// private PowerManager.WakeLock mWakeock;

	private ImportThread mImportThread;

	private class ImportThread extends Thread {
		private String name;
		private String addNumber;
		private Handler activityHandler;

		public ImportThread(String name, String phoneNumber, String addNumber,
				Handler activityHandler) {
			this.name = name;
			this.addNumber = addNumber;
			this.activityHandler = activityHandler;
		}

		@Override
		public void run() {
			startting = System.currentTimeMillis();
			insert();
			end = System.currentTimeMillis();
			times = end - startting;

			Log.e(DEBUG, "----------------run()    1 ------------------ ");

			myHandler.sendEmptyMessage(SHOWDIALOG);
		}

		private void insert() {
			int number = Integer.valueOf(addNumber);
			long phone = Long
					.parseLong(inputNumber.getText().toString().trim());
			Log.e(DEBUG, "number=" + number);
			for (int i = 0; i < number && !insertCancleFlag; i++) {
				phone++;
				count++;
				String f = Character.toString(first[getInt(first)]);
				String l = Character.toString(last[getInt(last)]);
				String la = Character.toString(last[getInt(last)]);

				switch (INSERT_TYPE) {
				case INSERT_CN_TYPE:
					name = f + l + la;
					break;
				case INSERT_EN_TYPE:
					name = getSubString(enName);
					break;
				case INSERT_RANDOM_TYPE:
                    // int type = (int) (Math.random() * 2) + 1;
                    int type = new Random().nextInt(2) + 1;

					if (type == 1) {
						name = f + l + la;
					} else {
						name = getSubString(enName);
					}
					break;
				}
				ArrayList<ContentProviderOperation> operationList = new ArrayList<ContentProviderOperation>();
				ContentProviderOperation.Builder builder = ContentProviderOperation
						.newInsert(RawContacts.CONTENT_URI);

                             builder.withValues(new ContentValues());
				builder.withYieldAllowed(true);
				/*
				 * builder.withValue(RawContacts.ACCOUNT_NAME, null);
				 * builder.withValue(RawContacts.ACCOUNT_TYPE, null);
				 */
			/*	builder.withValue(RawContacts.ACCOUNT_NAME,
						"com.thunderst.account_name.local");
				builder.withValue(RawContacts.ACCOUNT_TYPE,
						"com.thunderst.account_type.local");*/
				operationList.add(builder.build());
				builder = ContentProviderOperation.newInsert(Data.CONTENT_URI);
				builder.withValueBackReference(StructuredName.RAW_CONTACT_ID, 0);
				builder.withValue(Data.MIMETYPE,
						StructuredName.CONTENT_ITEM_TYPE);
				builder.withValue(StructuredName.PHONETIC_GIVEN_NAME, name);
				builder.withValue(StructuredName.DISPLAY_NAME, name);
				operationList.add(builder.build());
				builder = ContentProviderOperation.newInsert(Data.CONTENT_URI);
				builder.withValueBackReference(Phone.RAW_CONTACT_ID, 0);
				builder.withValue(Data.MIMETYPE, Phone.CONTENT_ITEM_TYPE);
				builder.withValue(Phone.TYPE, 1);
				builder.withValue(Phone.NUMBER, phone);
				operationList.add(builder.build());
				builder = ContentProviderOperation.newInsert(Data.CONTENT_URI);
				builder.withValueBackReference(Email.RAW_CONTACT_ID, 0);
				builder.withValue(Data.MIMETYPE, Email.CONTENT_ITEM_TYPE);
				builder.withValue(Email.TYPE, 1);
				builder.withValue(Email.DATA, name + (i + 1) + "@gmail.com");
				operationList.add(builder.build());
				try {
					ContentProviderResult[] results = getContentResolver()
							.applyBatch(ContactsContract.AUTHORITY,
									operationList);
				} catch (RemoteException e) {
					e.printStackTrace();
				} catch (OperationApplicationException e) {
					e.printStackTrace();
				}
				activityHandler.sendMessage(Message.obtain(activityHandler,
						INCREASE));
			}

			activityHandler.sendMessage(Message.obtain(activityHandler,
					FINISHDIALOG));
		}
	}

	private class mHandler extends Handler {
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case INCREASE:
				if (pd != null && pd.isShowing()) {
					pd.incrementProgressBy(1);
				}
				break;
			case FINISHDIALOG:
				if (pd != null && pd.isShowing()) {
					pd.cancel();
				}
				break;
			case SHOWDIALOG:

				showProgressDialog();
				break;
			}
			super.handleMessage(msg);

		};
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.insert_contacts);
		mOkButton = (Button) findViewById(R.id.okButton);
		inputName = (EditText) findViewById(R.id.inputName);
		inputNumber = (EditText) findViewById(R.id.inputNumber);
		addNumber = (EditText) findViewById(R.id.addNumber);
		mRadioGroup = (RadioGroup) findViewById(R.id.radio_group);
		insertCN = (RadioButton) findViewById(R.id.insert_cn_log);
		insertEN = (RadioButton) findViewById(R.id.insert_en_log);
		insertRandom = (RadioButton) findViewById(R.id.insert_random_log);
		// PowerManager pm = (PowerManager)
		// getSystemService(Context.POWER_SERVICE);
		// mWakeock = pm
		// .newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, LOG_TAG);
		// mWakeock.acquire();
		mRadioGroup
				.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {

					public void onCheckedChanged(RadioGroup group, int checkedId) {

						if (checkedId == insertCN.getId()) {
							INSERT_TYPE = INSERT_CN_TYPE;
						} else if (checkedId == insertEN.getId()) {
							INSERT_TYPE = INSERT_EN_TYPE;
						} else if (checkedId == insertRandom.getId()) {
							INSERT_TYPE = INSERT_RANDOM_TYPE;
						}
					}
				});

		firstName = this.getResources().getStringArray(R.array.xing)[0];
		lastName = this.getResources().getStringArray(R.array.name)[0];
		enName = this.getResources().getStringArray(R.array.en_name)[0];

		first = firstName.toCharArray();
		last = lastName.toCharArray();
		inputNumber.setHint(R.string.insert_contacts_hint);
		inputName.setText("HTD");
		addNumber.setHint("100");

		myHandler = new mHandler();

		mOkButton.setOnClickListener(new OnClickListener() {

			public void onClick(View v) {
				insertCancleFlag = false;
				if (TextUtils.isEmpty(inputNumber.getText())) {
					inputNumber.setText("13800138000");
				}
				if (TextUtils.isEmpty(addNumber.getText())) {
					addNumber.setText("100");
				}
				if (checkInput() == false) {
					Toast.makeText(getApplicationContext(),
							"some data field is null", Toast.LENGTH_SHORT)
							.show();
					return;
				}

				showDialog(PROGRESS_DIALOG);
				mImportThread = new ImportThread(inputName.getText().toString()
						.trim(), inputNumber.getText().toString().trim(),
						addNumber.getText().toString().trim(), myHandler);
				mImportThread.start();
			}
		});
		// if (mToneGenerator == null) {
		// try {
		// mToneGenerator = new ToneGenerator(DIAL_TONE_STREAM_TYPE,
		// TONE_RELATIVE_VOLUME);
		// setVolumeControlStream(DIAL_TONE_STREAM_TYPE);
		// } catch (RuntimeException e) {
		// mToneGenerator = null;
		// }
		// if (showMessage == null) {
		// showMessage = new ProgressDialog(InsertContactsActivity.this);
		// }
		// }
	}

	@SuppressWarnings("deprecation")
	@Override
	protected Dialog onCreateDialog(int id) {
		switch (id) {
		case PROGRESS_DIALOG:
			pd = new ProgressDialog(this);
			pd.setMax(Integer.valueOf(addNumber.getText().toString().trim()));
			pd.setTitle(R.string.title);
			pd.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
			pd.setButton(
					InsertContactsActivity.this.getResources().getString(
							R.string.exit),
					new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int i) {
							insertCancleFlag = true;
							dialog.cancel();

						}
					});

			pd.setOnDismissListener(new DialogInterface.OnDismissListener() {

				public void onDismiss(DialogInterface dialog) {
					playTone();

				}

				private void playTone() {
					AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
					int ringerMode = audioManager.getRingerMode();
					if ((ringerMode == AudioManager.RINGER_MODE_SILENT)
							|| (ringerMode == AudioManager.RINGER_MODE_VIBRATE)) {

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
							if (showMessage == null) {
								showMessage = new ProgressDialog(
										InsertContactsActivity.this);
							}

						}
						mToneGenerator.startTone(ToneGenerator.TONE_DTMF_3,
								TONE_LENGTH_MS);
					}
				}
			});

			pd.setCancelable(true);
			pd.setOnKeyListener(new DialogInterface.OnKeyListener() {

				public boolean onKey(DialogInterface dialog, int keyCode,
						KeyEvent event) {
					return true;
				}
			});
			break;
		}
		return pd;
	}

	@Override
	protected void onPrepareDialog(int id, Dialog dialog) {
		super.onPrepareDialog(id, dialog);
		pd.setMax(Integer.valueOf(addNumber.getText().toString().trim()));

	}

	private boolean checkInput() {

		return !(TextUtils.isEmpty(inputName.getText()) || TextUtils
				.isEmpty(addNumber.getText()));
	}

	// randomIndex
	private int getInt(char[] arr) {
		int index = (int) (Math.random() * (arr.length));
		return index;
	}

    public String getSubString(String enName) {
        StringBuffer sb = new StringBuffer();
        Random random = new Random();

        for (int i = 0; i < /* (int) (Math.random() * 20) */new Random()
                .nextInt(20) + 1; i++) {
            sb.append(enName.charAt(random.nextInt(enName.length())));
        }
        return sb.toString();
    }

	private void showProgressDialog() {
		StringBuffer msg = new StringBuffer();
		msg.append(InsertContactsActivity.this.getResources().getString(
				R.string.count));
		msg.append(count).append(
				InsertContactsActivity.this.getResources().getString(
						R.string.bar));
		msg.append("\n");
		msg.append(InsertContactsActivity.this.getResources().getString(
				R.string.times));
		msg.append(times);
		msg.append(InsertContactsActivity.this.getResources().getString(
				R.string.seconds));

		Dialog dialog = new AlertDialog.Builder(InsertContactsActivity.this)
				.setTitle(R.string.insert_contacts_title)
				.setMessage(msg.toString())
				.setPositiveButton(R.string.ok,
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog,
									int which) {
								mToneGenerator.stopTone();
								InsertContactsActivity.times = 0;
								InsertContactsActivity.count = 0;
								finish();
							}
						}).create();
		dialog.show();
	}

}