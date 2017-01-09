package com.sprd.engineermode.telephony;

import android.os.Bundle;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceScreen;
import android.view.KeyEvent;
import android.app.Fragment;
import android.content.Context;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnKeyListener;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.widget.EditText;
import android.text.InputFilter;
import android.text.InputFilter.LengthFilter;
import android.text.method.NumberKeyListener;
import android.preference.PreferenceActivity;
import com.sprd.engineermode.R;

public class FDActivity extends PreferenceActivity implements
		OnPreferenceChangeListener {
	private Context mContext;
	private SharedPreferences preferences;
	private SharedPreferences.Editor editor;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.pref_fd);
		mContext = this;
		preferences = mContext
				.getSharedPreferences("fd", mContext.MODE_PRIVATE);
		editor = preferences.edit();
	}

	@Override
	public boolean onPreferenceChange(Preference preference, Object newValue) {
		return true;
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			editor.putString("key_back", "true");
			editor.commit();
			finish();
		}
		return true;
	}

	@Override
	public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
			Preference preference) {
		String key = preference.getKey();
		// select default value settings
		if (key.equals("default_value")) {
			editor.putString("default", "true");
			editor.commit();
			finish();
			return true;
		}
		// select self_definite_value settings
		else if (key.equals("self_defining_value")) {
			editor.putString("default", "false");
			final EditText inputServer = new EditText(mContext);
			inputServer.setKeyListener(new NumberKeyListener() {
				private char[] numberChars = { '0', '1', '2', '3', '4', '5',
						'6', '7', '8', '9' };

				// Used to limit input method
				@Override
				public int getInputType() {
					return android.text.InputType.TYPE_CLASS_PHONE;
				}

				// Used to control the characters to input
				@Override
				protected char[] getAcceptedChars() {
					return numberChars;
				}
			});

			String value = preferences.getString("summary", "");

			inputServer.setHint(value);
			LengthFilter lengthFilter = new LengthFilter(5);
			InputFilter[] inputFilter = { lengthFilter };
			// Limit the input length
			inputServer.setFilters(inputFilter);

			AlertDialog.Builder builder = new AlertDialog.Builder(this);
			builder.setTitle("Input time(s) for dormancy");
			builder.setView(inputServer);
			builder.setCancelable(false);
			builder.setNegativeButton("Cancel",
					new DialogInterface.OnClickListener() {

						public void onClick(DialogInterface dialog, int which) {

						}
					});
			builder.setPositiveButton("Ok",
					new DialogInterface.OnClickListener() {

						public void onClick(DialogInterface dialog, int which) {
							editor.putString("definite_value", inputServer
									.getText().toString());
							editor.commit();
							finish();
						}
					});
			builder.show();

			return true;
		}
		return super.onPreferenceTreeClick(preferenceScreen, preference);
	}
}
