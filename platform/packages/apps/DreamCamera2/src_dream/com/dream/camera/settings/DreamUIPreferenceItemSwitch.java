package com.dream.camera.settings;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.SwitchPreference;
import android.util.AttributeSet;
import android.util.Log;

public class DreamUIPreferenceItemSwitch extends SwitchPreference implements
		DreamUIPreferenceItemInterface {

	public DreamUIPreferenceItemSwitch(Context context) {
		super(context);
	}

	public DreamUIPreferenceItemSwitch(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	public DreamUIPreferenceItemSwitch(Context context, AttributeSet attrs,
			int defStyleAttr) {
		super(context, attrs, defStyleAttr);
	}

	public DreamUIPreferenceItemSwitch(Context context, AttributeSet attrs,
			int defStyleAttr, int defStyleRes) {
		super(context, attrs, defStyleAttr, defStyleRes);
	}

	public static final String TAG = "DreamUIPreferenceItemSwitch";
	private DreamUISettingPartBasic mSettingPart;

	@Override
	protected boolean getPersistedBoolean(boolean defaultReturnValue) {

		if (!shouldPersist()) {
			return defaultReturnValue;
		}

		if (mSettingPart != null) {
			return mSettingPart.getPersistedBoolean(getKey());
		}

		return defaultReturnValue;
	}

	@Override
	protected boolean persistBoolean(boolean value) {
		if (shouldPersist()) {
			if (value == getPersistedBoolean(!value)) {
				// It's already there, so the same as persisting
				return true;
			}

			if (mSettingPart != null) {
				return mSettingPart.persistBoolean(getKey(), value);
			}

		}
		return false;

	}

	@Override
	public void initializeData(DreamUISettingPartBasic settingPart) {
		mSettingPart = settingPart;
		update();
	}

	@Override
	public void update() {
		this.setChecked(mSettingPart.getPersistedBoolean(getKey()));
	}

	@Override
	protected void notifyChanged() {
		super.notifyChanged();
		if (mSettingPart != null) {
			// TODO
		}

	}

}
