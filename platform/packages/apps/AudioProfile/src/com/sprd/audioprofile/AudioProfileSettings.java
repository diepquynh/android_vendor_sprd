package com.sprd.audioprofile;

import android.app.ActionBar;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.database.ContentObserver;
import android.database.Cursor;
import android.media.AudioManager;
import android.media.RingtoneManager;
import android.media.RingtoneManagerEx;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceGroup;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.text.Editable;
import android.text.InputFilter;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.text.style.AbsoluteSizeSpan;
import android.util.Log;
import android.view.ContextMenu;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RadioButton;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Set;

import android.telephony.TelephonyManager;
import android.app.LoaderManager;
import android.content.CursorLoader;
import android.content.Loader;
import android.content.res.Resources;
import android.os.SystemClock;
import android.view.KeyEvent;

import android.preference.CheckBoxPreference;
import android.preference.Preference.OnPreferenceClickListener;
import android.os.SystemProperties;

public class AudioProfileSettings extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener, LoaderManager.LoaderCallbacks<Cursor>, OnPreferenceClickListener {
    static final String TAG = "AudioProfileSettings";
    private static AudioManager mAudioManager;
    private static ContentResolver mResolver;
    private static ContentObserver mObserver;
    private ListView mListView;

    private static HashMap<Integer, AudioProfilePreference> mAllProfilesMap;
    private static int mSelectedId;

    // private PreferenceGroup mAudioProfileList;
    private PreferenceCategory mPredefinedCategory;
    private PreferenceCategory mUserDefinedCategory;

    public static final int SET_PROFILE = 1;
    public static final int INSERT_PROFILE = 2;
    public static final int DELETE_PROFILE = 3;

    public static final int MENU_NEW = 1;
    public static final int MENU_RESET = 2;
    public static final int MENU_USE = 3;
    public static final int MENU_RENAME = 4;
    public static final int MENU_DELETE = 5;

    private static final int ADD_DIALOG = 1;
    private static final int RESET_DIALOG = 2;
    private static final int MENU_DIALOG = 3;
    private static final int DELETE_DIALOG = 4;
    private static final int DUPLICATE_DISPLAY_NAME_DIALOG = 5;
    private static final int EMTPY_NAME_DIALOG = 6;
    private static final int TEXT_MAX = 32;

    private int mAudioProfileId = -1;
    private String dialogName;
    private static Context mContext;
    private static boolean isInProfile = false;

    public static final String INTENT_ACTION_MODE_NAME_CHANGED = "com.sprd.audioprofile.mode_name.changed";
    public static final String EDIT_ID_KEY = "currentId";
    public static final String EDITED_NAME_KEY = "edited_name";
    private static int mPhoneCount = 1;
    //SPRD: 427745 The status was not changed when in outdoor mode with seekbar to adjust the volume
    private static int mMSG_Volume = 1;
    private static String Volume_Ring_Speaker="volume_ring_speaker";

    // SPRD: 459035 Add for mNameChangedReceiver to store whether registered
    private boolean mRegistered = false;

    /* SPRD: bug462372, add for magic sound @{ */
    private static final String KEY_SOUND_IMPROVEMENT_SELECT = "sound_improvement_select";
    // SPRD 465003
    private static final String KEY_SOUND_IMPROVEMENT_SELECT_CATEGARY = "sound_improvement";
    private static final String MAGIC_AUDIO = "magic_audio";
    private static final String MAGIC_AUDIO_OPEN = "1";
    private static final String MAGIC_AUDIO_CLOSED = "0";
    private CheckBoxPreference mSoundCheckBoxPreference;
    private static final String KEY_MAGIC_AUDIO_PROPERTY = "persist.sys.magicaudio";
    //private PreferenceCategory KEY_SOUND_IMPROVEMENT = "sound_improvement";
    private PreferenceCategory mMagicSoundCategory;
    /* @} */
    // SPRD 604132
    private static int mRingMode = 0;
    private static int mTempRingMode = 0;

    public static class ModeChangeReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            /* SPRD: Modify for bug462567. @{ */
            if (intent.getAction() == null) {
                return;
            }
            /* @} */

            //sprd: Bug 426173 Modify the status of Audioprofile
            if (intent.getAction().equals(AudioManager.INTERNAL_RINGER_MODE_CHANGED_ACTION)) {
                Log.d(TAG,"receive action: AudioManager.INTERNAL_RINGER_MODE_CHANGED_ACTION");
                /* SPRD: synchronized cts test information @{ */
                String ctsStub = intent.getStringExtra("CTSMODE");
                if (ctsStub != null && ctsStub.equalsIgnoreCase("cts")) {
                    return;
                }
                /* @} */
                if (mResolver == null) {
                    mResolver = context.getContentResolver();
                }
                if (mContext == null) {
                    mContext = context;
                }
                int originSelectId = Settings.System.getInt(mResolver, "currentAudioProfileId", -1);
                int ringerMode = -1;
                if (mAudioManager == null) {
                    mAudioManager = (AudioManager) context.getSystemService(AUDIO_SERVICE);
                }
                // sprd: Bug 426173 Modify the status of Audioprofile
                ringerMode = mAudioManager.getRingerModeInternal();
                Log.d(TAG,"ringerMode   = "+ringerMode+"    originSelectId = "+originSelectId+"    isInProfile = "+isInProfile);
                /* SPRD: 629298 ringer mode set to general after take video in user defined mode @{ */
                if (!isInProfile && originSelectId > AudioProfile.PREDEFINED_PROFILE_COUNT) {
                    mTempRingMode = originSelectId;
                } else if (!isInProfile && mTempRingMode > AudioProfile.PREDEFINED_PROFILE_COUNT &&
                        ringerMode != AudioManager.RINGER_MODE_VIBRATE) {
                    ringerMode = mTempRingMode;
                    mTempRingMode = 0;
                } else if (ringerMode == AudioManager.RINGER_MODE_VIBRATE) {
                    mTempRingMode = 0;
                }
                /* @} */
                if (changeToRingerMode(originSelectId) == ringerMode) {
                    return;
                } else {
                    if (originSelectId == -1 && ringerMode != -1) {
                        updateCurrentId(context, originSelectId);
                    } else if (originSelectId != -1 && ringerMode != -1) {
                        if ((originSelectId == AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID
                                || originSelectId == AudioProfile.DEFAULT_VIBRATE_PROFILE_ID
                                || originSelectId == AudioProfile.DEFAULT_SILENT_PROFILE_ID)
                                && ringerMode == AudioManager.RINGER_MODE_NORMAL) {
                            updateCurrentId(context, originSelectId);
                        /* SPRD: 629298 ringer mode set to general after take video in user defined mode @{ */
                        } else if (!isInProfile && ringerMode > AudioProfile.PREDEFINED_PROFILE_COUNT) {
                            setProfileIsSelected(ringerMode);
                            return;
                        /* @} */
                        }
                    } else {
                        setProfileIsSelected(AudioProfile.DEFAULT_GENERAL_PROFILE_ID);
                    }
                    if (isInProfile) {
                        mSelectedId = getSelectedId(true);
                        updateSelectedState();
                    } else {
                        updateCurrentId(context, originSelectId);
                    }
                }
            }
        }
    }

    private static int changeToRingerMode(int selectId) {
        int ringerMode = AudioManager.RINGER_MODE_NORMAL;
        switch (selectId) {
            case 1:
                ringerMode = AudioManager.RINGER_MODE_NORMAL;
                break;
            case 2:
                ringerMode = AudioManager.RINGER_MODE_SILENT;
                break;
            case 3:
                ringerMode = AudioManager.RINGER_MODE_VIBRATE;
                break;
            case 4:
                ringerMode = AudioManager.RINGER_MODE_OUTDOOR;
                break;
        }
        return ringerMode;
    }
    private BroadcastReceiver mNameChangedReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(INTENT_ACTION_MODE_NAME_CHANGED)) {
                int changeId = intent.getIntExtra(EDIT_ID_KEY, -1);
                String changeName = intent.getStringExtra(EDITED_NAME_KEY);
                if (changeId != -1 && changeName != null && !changeName.equals("")) {
                    mAllProfilesMap.get(changeId).setTitle(changeName);
                }
            }
        }
    };

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        Log.d(TAG, "AudioProfileSettings----onCreate");
        mContext = this;

        // Setting ActionBar Style to popup menu.
        Intent intent = getIntent();
        if (intent != null && false == intent.getBooleanExtra("systemui:ui_options", false)) {
            ActionBar actionBar = getActionBar();
            actionBar.setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP
                    , ActionBar.DISPLAY_HOME_AS_UP);
            Set<String> categories = intent.getCategories();
            /* SPRD:Bug409773 @{*/
            /*if (categories != null && categories.contains("com.android.settings.SHORTCUT")) {
                actionBar.setDisplayHomeAsUpEnabled(false);
            } else {
                actionBar.setDisplayHomeAsUpEnabled(true);
            }*/
            actionBar.setDisplayHomeAsUpEnabled(true);
            /* @} */
            actionBar.setDisplayShowTitleEnabled(true);
        }

        addPreferencesFromResource(R.xml.audio_profile_settings);

        mAudioManager = (AudioManager) getSystemService(AUDIO_SERVICE);
        mResolver = getContentResolver();
        /* SPRD: 427745 The status was not changed when in outdoor mode with seekbar to adjust the volume @{ */
        mObserver = new ContentObserver(new Handler()) {
            @Override
            public void onChange(boolean selfChange) {
                try {
                    mHandler.obtainMessage(mMSG_Volume).sendToTarget() ;
                }
                catch (Exception e) {
                    e.printStackTrace();
                }
            }
        };
        registerContentObservers();
        /* @} */
        /* SPRD: bug 449112 & 476713  Audioprofile crash & display was wrong @{ */
        //mAllProfilesMap = new HashMap<Integer, AudioProfilePreference>();

        // mAudioProfileList = (PreferenceGroup)
        // findPreference("audio_profile_list");

        mPredefinedCategory = (PreferenceCategory) findPreference("predefined_audio_profiles_list");
        mUserDefinedCategory = (PreferenceCategory) findPreference("user_defined_audio_profiles_list");
        AudioProfilePreference standard_pref;
        AudioProfilePreference silent_pref;
        AudioProfilePreference vibrate_pref;
        AudioProfilePreference outdoor_pref;
        if (mAllProfilesMap == null) {
            mAllProfilesMap = new HashMap<Integer, AudioProfilePreference>();
        }

        if (mAllProfilesMap.isEmpty()) {
            standard_pref = new AudioProfilePreference(this);
            standard_pref.setId(AudioProfile.DEFAULT_GENERAL_PROFILE_ID);
            standard_pref.setKey("standard_mode");
            standard_pref.setTitle(R.string.standard_string);
            standard_pref.setPersistent(false);
            standard_pref.setOnPreferenceChangeListener(this);

            silent_pref = new AudioProfilePreference(this);
            silent_pref.setKey("silent_mode");
            silent_pref.setTitle(R.string.silent_string);
            silent_pref.setId(AudioProfile.DEFAULT_SILENT_PROFILE_ID);
            silent_pref.setPersistent(false);
            silent_pref.setOnPreferenceChangeListener(this);

            vibrate_pref = new AudioProfilePreference(this);
            vibrate_pref.setKey("vibrate_mode");
            vibrate_pref.setTitle(R.string.vibrate_string);
            vibrate_pref.setId(AudioProfile.DEFAULT_VIBRATE_PROFILE_ID);
            vibrate_pref.setPersistent(false);
            vibrate_pref.setOnPreferenceChangeListener(this);

            outdoor_pref = new AudioProfilePreference(this);
            outdoor_pref.setKey("outdoor_mode");
            outdoor_pref.setTitle(R.string.outdoor_string);
            outdoor_pref.setSummary(R.string.outdoor_summary);
            outdoor_pref.setId(AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID);
            outdoor_pref.setPersistent(false);
            outdoor_pref.setOnPreferenceChangeListener(this);

            mAllProfilesMap.put(standard_pref.getId(), standard_pref);
            mAllProfilesMap.put(silent_pref.getId(), silent_pref);
            mAllProfilesMap.put(vibrate_pref.getId(), vibrate_pref);
            mAllProfilesMap.put(outdoor_pref.getId(), outdoor_pref);
        }else {
            standard_pref = mAllProfilesMap.get(AudioProfile.DEFAULT_GENERAL_PROFILE_ID);
            silent_pref = mAllProfilesMap.get(AudioProfile.DEFAULT_SILENT_PROFILE_ID);
            vibrate_pref = mAllProfilesMap.get(AudioProfile.DEFAULT_VIBRATE_PROFILE_ID);
            outdoor_pref = mAllProfilesMap.get(AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID);
        }

        mPredefinedCategory.addPreference(standard_pref);
        mPredefinedCategory.addPreference(silent_pref);
        mPredefinedCategory.addPreference(vibrate_pref);
        mPredefinedCategory.addPreference(outdoor_pref);

        /* SPRD: bug462372, add for magic sound @{ */
        /* SPRD 465003 @{ */
        // mSoundCheckBoxPreference = (CheckBoxPreference)findPreference(KEY_SOUND_IMPROVEMENT_SELECT);
        // mMagicSoundCategory = (PreferenceCategory)
        // findPreference("sound_improvement");
        if (findPreference(KEY_SOUND_IMPROVEMENT_SELECT_CATEGARY) == null) {
            mMagicSoundCategory = new PreferenceCategory(this);
            mMagicSoundCategory.setTitle(R.string.sound_improvement);
            mMagicSoundCategory.setKey(KEY_SOUND_IMPROVEMENT_SELECT_CATEGARY);
            getPreferenceScreen().addPreference(mMagicSoundCategory);

            mSoundCheckBoxPreference = new CheckBoxPreference(this);
            mSoundCheckBoxPreference.setTitle(R.string.sound_improvement_select);
            mSoundCheckBoxPreference.setSummary(R.string.sound_improvement_summary);
            mSoundCheckBoxPreference.setKey(KEY_SOUND_IMPROVEMENT_SELECT);

            mSoundCheckBoxPreference.setOnPreferenceClickListener(this);
            Log.d(TAG,"magic sound mAudioManager.getParameters(MAGIC_AUDIO) = "
                            + mAudioManager.getParameters(MAGIC_AUDIO));

            Log.d(TAG,"magi sound SystemProperties.get(KEY_MAGIC_AUDIO_PROPERTY) = "
                            + SystemProperties.get(KEY_MAGIC_AUDIO_PROPERTY, "2"));
            mMagicSoundCategory.addPreference(mSoundCheckBoxPreference);

            if (mAudioManager.getParameters(MAGIC_AUDIO).equals(MAGIC_AUDIO_OPEN)) {
                mSoundCheckBoxPreference.setChecked(true);
            } else {
                mSoundCheckBoxPreference.setChecked(false);
            }

            if (!mAudioManager.getParameters(MAGIC_AUDIO).equals(MAGIC_AUDIO_OPEN)
                    && !mAudioManager.getParameters(MAGIC_AUDIO).equals(MAGIC_AUDIO_CLOSED)) {
                Log.d(TAG, "magi sound remove item SystemProperties.get(KEY_MAGIC_AUDIO_PROPERTY) = "
                        + SystemProperties.get(KEY_MAGIC_AUDIO_PROPERTY, "2"));
                mMagicSoundCategory.removeAll();
                getPreferenceScreen().removePreference(findPreference("sound_improvement"));
            }
        }
        /* @} */
        /* @} */

        int selectId = Settings.System.getInt(mResolver, "currentAudioProfileId", -1);
        if (selectId != -1 && selectId <= AudioProfile.PREDEFINED_PROFILE_COUNT) {
            mSelectedId = getSelectedId(true);
            updateSelectedState();
        }
        mPhoneCount = TelephonyManager.getDefault().getPhoneCount();
        getLoaderManager().initLoader(0, null, this);
        /* @} */

    }

    public Loader<Cursor> onCreateLoader(int id, Bundle args) {
        return new CursorLoader(this, AudioProfile.CONTENT_URI,
                null, AudioProfileColumns.ID + ">" + AudioProfile.PREDEFINED_PROFILE_COUNT, null,
                null);
        /* AudioProfileColumns.ID + ">" + AudioProfile.PREDEFINED_PROFILE_COUNT */
    }

    public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
        fillList(data);
        IntentFilter nameChangeFilter = new IntentFilter(INTENT_ACTION_MODE_NAME_CHANGED);
        registerReceiver(mNameChangedReceiver, nameChangeFilter);
        // SPRD: 459035 Set mRegistered to true after register receiver
        mRegistered = true;
        mSelectedId = getSelectedId(true);
        updateSelectedState();
    }

    public void onLoaderReset(Loader<Cursor> loader) {

    }

    @Override
    protected void onResume() {
        super.onResume();
        //SPRD: bug 449112 & 476713  Audioprofile crash & display was wrong
        refresh();
        isInProfile = true;
        /* SPRD: 427745 The status was not changed when in outdoor mode with seekbar to adjust the volume @{ */
        int currentVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_SYSTEM);
        mSelectedId = getSelectedId(true);
        if (mSelectedId == AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID && currentVolume > 0
                && currentVolume < mAudioManager.getStreamMaxVolume(AudioManager.STREAM_RING)) {
            mSelectedId=AudioProfile.DEFAULT_GENERAL_PROFILE_ID;
            setProfileIsSelected(mSelectedId);
        }
        /* @} */
        updateSelectedState();
        syncSystemToProfile();
        syncRingtoneToProfile(mContext,mSelectedId);
    }
    @Override
    protected void onPause() {
        super.onPause();
        isInProfile = false;
    }

    public boolean onKeyDown(int keyCode, KeyEvent event) {
        boolean isdown = (event.getAction() == KeyEvent.ACTION_DOWN);
        int amount = 0;
        switch (keyCode) {
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                amount = -1;
                break;
            case KeyEvent.KEYCODE_VOLUME_UP:
                amount = 1;
                break;
            default:
                return super.onKeyDown(keyCode, event);
        }
        if (isdown && amount != 0) {
            /*SPRD: for bug432007 let it do as system when music.FM@{*/
            /*if (mAudioManager.isMusicActive()) {
                mAudioManager.adjustStreamVolume(AudioManager.STREAM_MUSIC, amount,
                        AudioManager.FLAG_VIBRATE | AudioManager.FLAG_SHOW_UI);
                return true;
            } else if (mAudioManager.isFmActive()) {
                mAudioManager.adjustStreamVolume(AudioManager.STREAM_FM, amount,
                        AudioManager.FLAG_VIBRATE | AudioManager.FLAG_SHOW_UI);
                return true;
            } else {
                mAudioManager.adjustStreamVolume(AudioManager.STREAM_RING, amount,
                        AudioManager.FLAG_VIBRATE | AudioManager.FLAG_SHOW_UI);
                mSelectedId = getSelectedId(false);
                syncSystemToProfile();
                updateSelectedState();
                return true;
            }*/
            if(!mAudioManager.isMusicActive()){
                /*SPRD: for bug 604132 let it do as system when normal@{*/
                int selectedId = getSelectedId(false);
                if ((mSelectedId == AudioProfile.DEFAULT_GENERAL_PROFILE_ID && selectedId == AudioProfile.DEFAULT_VIBRATE_PROFILE_ID)
                        || (selectedId == AudioProfile.DEFAULT_SILENT_PROFILE_ID && mSelectedId == AudioProfile.DEFAULT_VIBRATE_PROFILE_ID) ){
                    mRingMode = 1;
                }
                /* @} */
                mAudioManager.adjustStreamVolume(AudioManager.STREAM_RING, amount,
                        AudioManager.FLAG_VIBRATE | AudioManager.FLAG_SHOW_UI);
                mSelectedId = selectedId;
                syncSystemToProfile();
                updateSelectedState();
                return true;
            }
            /*@}*/
        }

        return super.onKeyDown(keyCode, event);
    }

    private void syncSystemToProfile (){
      //sprd: Bug 426173 Modify the status of Audioprofile
        int ringerMode = mAudioManager.getRingerModeInternal();
        final AudioProfile profile = AudioProfile.restoreProfileWithId(mContext,
                mSelectedId);
        Log.d(TAG,"mSelectedId = "+mSelectedId+" ringerMode = "+ringerMode);
        if (mAudioManager.RINGER_MODE_NORMAL == ringerMode && profile != null) {
            /*SPRD: for bug 604132 let it do as system when normal@{*/
            if (!(mRingMode == 1)){
                profile.updateVolume(mContext);
            }
            /* @} */
        }
    }

    @Override
    protected void onDestroy() {
        if (mAllProfilesMap != null) {
            mAllProfilesMap.clear();
            mAllProfilesMap = null;
        }
        /* SPRD: 459035 When mRegistered is set to true then need to unregisterReceiver @{ */
        if (mNameChangedReceiver != null && mRegistered) {
            unregisterReceiver(mNameChangedReceiver);
            mRegistered = false;
        }
        /* @} */
        /* SPRD: 427745 The status was not changed when in outdoor mode with seekbar to adjust the volume @{ */
        if (mObserver != null) {
            mResolver.unregisterContentObserver(mObserver);
        }
        /* @} */
        super.onDestroy();
    }
    /* SPRD 602407: modify the title{@ */
    @Override
    public void onConfigurationChanged(
            android.content.res.Configuration newConfig) {
        ActionBar actionBar = getActionBar();
        if (actionBar != null && getResources() != null) {
            actionBar.setTitle(setActionText(getResources(), actionBar, newConfig));
        }
        super.onConfigurationChanged(newConfig);
    }

    public static SpannableString setActionText(Resources res, ActionBar actionBar,
            android.content.res.Configuration newConfig) {
        final SpannableString text = new SpannableString(actionBar.getTitle());
        if (newConfig.toString().contains("land")) {
            text.setSpan(new AbsoluteSizeSpan(res.getDimensionPixelSize(
                    R.dimen.audioprofile_land_actionbar_title_size)), 0,
                    text.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        } else {
            text.setSpan(new AbsoluteSizeSpan(res.getDimensionPixelSize(
                    R.dimen.audioprofile_port_actionbar_title_size)), 0,
                    text.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        return text;
    }
    /* @} */

    private void startEditActivity(int currentId) {
        Intent intent = new Intent("com.sprd.action.AUDIO_PROFILE_SOUND_SETTINGS");
        intent.putExtra(EDIT_ID_KEY, currentId);
        startActivity(intent);
    }

    private void fillList(Cursor cursor) {

        if (cursor == null || !cursor.moveToFirst())
            return;

        ArrayList<Preference> userDefinedList = new ArrayList<Preference>();

        cursor.moveToFirst();
        while (!cursor.isAfterLast()) {
            int id = cursor.getInt(cursor
                    .getColumnIndex(AudioProfileColumns.ID));
            String displayName = cursor.getString(cursor
                    .getColumnIndex(AudioProfileColumns.DISPLAY_NAME));

            AudioProfilePreference pref = new AudioProfilePreference(this);
            pref.setKey(displayName);
            pref.setId(id);
            pref.setTitle(displayName);
            pref.setPersistent(false);
            pref.setOnPreferenceChangeListener(this);

            userDefinedList.add(pref);

            mAllProfilesMap.put(pref.getId(), pref);
            cursor.moveToNext();
        }

        for (Preference preference : userDefinedList) {
            mUserDefinedCategory.addPreference(preference);
            // mAudioProfileList.addPreference(mUserDefinedCategory);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        menu.add(0, MENU_NEW, 0,getResources().getString(R.string.add_profile))
                .setIcon(android.R.drawable.ic_menu_add);
        menu.add(0, MENU_RESET, 0,getResources().getString(R.string.reset_profile))
                .setIcon(android.R.drawable.ic_menu_revert);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        Log.i(TAG, "click option menu id = " + item.getItemId());
        switch (item.getItemId()) {
            case MENU_NEW:
                showDialog(ADD_DIALOG);
                return true;

            case MENU_RESET:
                showDialog(RESET_DIALOG);
                return true;

            case android.R.id.home:
                finish();
                return super.onOptionsItemSelected(item);
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        mAudioProfileId = ((AudioProfilePreference) preference).getId();
        Log.d(TAG, "mAudioProfileId = " + mAudioProfileId);
        if ((AudioProfile.DEFAULT_VIBRATE_PROFILE_ID == mAudioProfileId
                || AudioProfile.DEFAULT_SILENT_PROFILE_ID == mAudioProfileId
                || AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID == mAudioProfileId)
                && mSelectedId == mAudioProfileId) {
            Log.d(TAG, "mSelectedId == audioProfileId");
            return false;
        } else {
            removeDialog(MENU_DIALOG);
            Log.i(TAG, "show menu dialog");
            showDialog(MENU_DIALOG);
        }
        return true;
    }

    public boolean onPreferenceChange(Preference preference, Object newValue) {
        // update to system
        if (newValue instanceof Integer) {
            int profileId = ((Integer) newValue).intValue();
            Log.d(TAG, "PreferenceChangedId = " + profileId);
            setProfileIsSelected(profileId);
            mSelectedId = getSelectedId(true);
            updateSelectedState();
        }
        //S: when AudioProfile Mode is changed, dialog with MENU_DIALOG dismiss
        removeDialog(MENU_DIALOG);
        return true;
    }

    private static void updateSelectedState() {
        if (mAllProfilesMap == null) {
            return;
        }
        for (HashMap.Entry<Integer, AudioProfilePreference> entry : mAllProfilesMap.entrySet()) {
            if (mSelectedId != entry.getKey()) {
                ((AudioProfilePreference) entry.getValue()).setChecked(false);
            } else {
                ((AudioProfilePreference) entry.getValue()).setChecked(true);
            }
        }
    }

    @Override
    protected Dialog onCreateDialog(int id, final Bundle bundle) {
        Log.i(TAG, "onCreateDialog id = " + id);
        Dialog dialog = null;
        AlertDialog.Builder builder = null;
        //SPRD: Added for bug 596405, adjust the UI
        String mInfo = "\n" + mContext.getText(R.string.add_profile_msg);
        switch (id) {
            case ADD_DIALOG:
                final EditText addEt = new EditText(this);
                // SPRD: Bug 426128 The dialog was obscured when the line too much
                addEt.setMaxLines(5);
                addEt.addTextChangedListener(new TextWatcher() {
                    final Toast mtoast = Toast.makeText(mContext,
                            mContext.getText(R.string.input_limit), Toast.LENGTH_SHORT);
                    public void beforeTextChanged(CharSequence s, int start,
                            int count, int after) {
                    }

                    public void onTextChanged(CharSequence s, int start,
                            int before, int count) {
                    }

                    public void afterTextChanged(Editable s) {
                        int length = s.toString().length();
                        if (length > TEXT_MAX) {
                            if (mtoast != null) {
                                mtoast.show();
                            }
                            s.delete(TEXT_MAX, length);
                        }
                    }
                });


                builder = new AlertDialog.Builder(this);
                builder.setTitle(R.string.add_profile);
                builder.setView(addEt, 5, 0, 5, 0);
                //SPRD: Modified for bug 596405, adjust the UI
                builder.setMessage(mInfo);
                builder.setPositiveButton(R.string.dlg_ok,
                        new DialogInterface.OnClickListener() {

                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                String text = addEt.getText().toString().trim();
                                if (TextUtils.isEmpty(text) || text == null) {
                                    showDialog(EMTPY_NAME_DIALOG);
                                    addEt.setText("");
                                    removeDialog(ADD_DIALOG);
                                    return;
                                } else if (existDuplicateDisplayName(text)) {
                                    showDialog(DUPLICATE_DISPLAY_NAME_DIALOG);
                                    addEt.setText("");
                                    removeDialog(ADD_DIALOG);
                                    return;
                                } else {
                                    addNewAudioProfile(text);
                                }
                                addEt.setText("");
                                removeDialog(ADD_DIALOG);
                            }
                        });
                builder.setNegativeButton(R.string.dlg_cancel,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                removeDialog(ADD_DIALOG);
                            }
                        });
                dialog = builder.show();
                break;
            case RESET_DIALOG:
                builder = new AlertDialog.Builder(this);
                builder.setTitle(R.string.reset_profile);
                builder.setMessage(R.string.reset_profile_msg);
                builder.setPositiveButton(R.string.dlg_ok,
                        new DialogInterface.OnClickListener() {

                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                resetAudioProfile();
                                removeDialog(RESET_DIALOG);
                            }
                        });
                /* SPRD:BUG458036 add to remove RESET_DIALOG when exit RESET_DIALOG @{*/
                builder.setNegativeButton(R.string.dlg_cancel,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int which) {
                                removeDialog(RESET_DIALOG);
                            }
                        });
                builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                        public void onDismiss(DialogInterface dialog) {
                                removeDialog(RESET_DIALOG);
                            }
                        });
                /* @} */
                dialog = builder.show();
                break;
            case MENU_DIALOG:
                builder = new AlertDialog.Builder(this);

                String[] menuItem;
                String[] allMenu = new String[] {
                        getResources().getString(R.string.use_profile),
                        getResources().getString(R.string.edit_profile),
                        getResources().getString(R.string.delete_profile)
                };
                String[] useEditMenu = new String[] {
                        getResources().getString(R.string.use_profile),
                        getResources().getString(R.string.edit_profile)
                };
                String[] editDeleMenu = new String[] {
                        getResources().getString(R.string.edit_profile),
                        getResources().getString(R.string.delete_profile)
                };
                String[] useMenu = new String[] {
                        getResources().getString(R.string.use_profile)
                };
                String[] editMenu = new String[] {
                        getResources().getString(R.string.edit_profile)
                };

                final int USE_EDIT = 0,DEL_EDIT = 1,DEL = 2;
                AudioProfile profile;
                if (mAudioProfileId == -1) {
                    return null;
                } else {
                    profile = AudioProfile.restoreProfileWithId(mContext, mAudioProfileId);
                }

                if (mAudioProfileId <= AudioProfile.PREDEFINED_PROFILE_COUNT) {
                    dialogName = getResources().getStringArray(R.array.profile_display_name)[mAudioProfileId - 1];
                } else {
                    if (profile != null) {
                        dialogName = profile.mDisplayName;
                    } else {
                        return null;
                    }
                }
                if (profile != null && (AudioProfile.DEFAULT_VIBRATE_PROFILE_ID == profile.mId
                        || AudioProfile.DEFAULT_SILENT_PROFILE_ID == profile.mId
                        || AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID == profile.mId)) {
                    menuItem = useMenu;
                } else if (profile != null && profile.mId > AudioProfile.PREDEFINED_PROFILE_COUNT) {
                    if (mSelectedId == mAudioProfileId) {
                        menuItem = editDeleMenu;
                    } else {
                        menuItem = allMenu;
                    }
                } else {
                    if (mSelectedId == mAudioProfileId) {
                        menuItem = editMenu;
                    } else {
                        menuItem = useEditMenu;
                    }
                }
                builder.setTitle(dialogName);
                builder.setItems(menuItem, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        switch (which) {
                            case USE_EDIT:
                                if (mSelectedId == mAudioProfileId) {
                                    startEditActivity(mAudioProfileId);
                                } else {
                                    setProfileIsSelected(mAudioProfileId);
                                    mSelectedId = getSelectedId(true);
                                    updateSelectedState();
                                }
                                mAudioProfileId = -1;
                                break;
                            case DEL_EDIT:
                                if (mSelectedId == mAudioProfileId) {
                                    showDialog(DELETE_DIALOG);
                                } else {
                                    startEditActivity(mAudioProfileId);
                                    mAudioProfileId = -1;
                                }
                                break;
                            case DEL:
                                showDialog(DELETE_DIALOG);
                                break;
                        }
                        removeDialog(MENU_DIALOG);
                    }
                });
                dialog = builder.show();
                break;
            case DELETE_DIALOG:
                /* SPRD:397187 when change language,dialogName show null @{*/
                if(dialogName != null) {
                    builder = new AlertDialog.Builder(this);
                    builder.setTitle(R.string.delete_profile);
                    builder.setMessage(getString(R.string.delete_profile_msg, dialogName));
                    builder.setPositiveButton(R.string.dlg_ok,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    deleteAudioProfile(mAudioProfileId);
                                    removeDialog(DELETE_DIALOG);
                                    mAudioProfileId = -1;
                                }
                            });
                    builder.setNegativeButton(R.string.dlg_cancel,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    removeDialog(DELETE_DIALOG);
                                    mAudioProfileId = -1;
                                }
                            });
                    /* SPRD: add for bug 454225 @{ */
                    builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                        @Override
                        public void onDismiss(DialogInterface dialog) {
                            // TODO Auto-generated method stub
                            removeDialog(DELETE_DIALOG);
                            mAudioProfileId = -1;
                        }
                    });
                    /* @} */
                    dialog = builder.show();
                }
                /* @}*/
                break;
            case DUPLICATE_DISPLAY_NAME_DIALOG:
                builder = new AlertDialog.Builder(this);
                builder.setIcon(android.R.drawable.ic_dialog_alert);
                builder.setTitle(R.string.duplicate_display_name_dialog);
                builder.setPositiveButton(R.string.dlg_ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int which) {
                                removeDialog(DUPLICATE_DISPLAY_NAME_DIALOG);
                                showDialog(ADD_DIALOG);
                            }
                        });
                dialog = builder.show();
                break;
            case EMTPY_NAME_DIALOG:
                builder = new AlertDialog.Builder(this);
                builder.setIcon(android.R.drawable.ic_dialog_alert);
                builder.setTitle(R.string.empty_name_dialog);
                builder.setPositiveButton(R.string.dlg_ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int which) {
                                removeDialog(EMTPY_NAME_DIALOG);
                                showDialog(ADD_DIALOG);
                            }
                        });
                dialog = builder.show();
                break;
            default:
                break;
        }
        return dialog;
    }

    private static void saveProfileId(ContentResolver resolver, final int id) {
        if (resolver != null) {
            Settings.System.putInt(resolver, "currentAudioProfileId", id);
        }
        new Thread(new Runnable() {

            @Override
            public void run() {
                // TODO Auto-generated method stub

                AudioProfile profile = AudioProfile.restoreProfileWithId(mContext, id);
                if (profile != null && mContext != null) {
                    profile.setSelected(mContext);
                }
            }

        }).start();
        Log.i(TAG, "AudioProfileSettings save currentAudioProfileId = " + id);
    }

    private static void syncRingtoneToProfile(final Context context, final int id) {
        new Thread(new Runnable() {

            @Override
            public void run() {
                // TODO Auto-generated method stub

                final AudioProfile profile = AudioProfile.restoreProfileWithId(context, id);
                /* SPRD modified for bug 530430,
                 * update two SIM ringtones when two SIM cards are inserted @{
                 */
                int mPhoneCount = TelephonyManager.getDefault().getPhoneCount();
                for (int i = 0; i < mPhoneCount; i++) {
                    Uri ringtoneUri = RingtoneManagerEx.getActualDefaultRingtoneUri(context,
                            RingtoneManager.TYPE_RINGTONE,i);
                    String ringtoneString = ringtoneUri!= null ? ringtoneUri.toString() : null;
                    if ((id >= AudioProfile.PREDEFINED_PROFILE_COUNT || AudioProfile.DEFAULT_GENERAL_PROFILE_ID == id)
                            && profile != null
                            && ringtoneString != null
                            && ((profile.mRingtoneUri[i] != null && !profile.mRingtoneUri[i]
                                    .equals(ringtoneString)) || profile.mRingtoneUri[i] == null)) {
                        profile.updateRingtone(context);
                    }
                }
                /* @} */
            }

        }).start();

    }

    private static void updateCurrentId(Context context, int id) {
        AudioManager audioManager;
        ContentResolver resolver;
        if (context != null) {
            resolver = context.getContentResolver();
            audioManager = (AudioManager) context.getSystemService(AUDIO_SERVICE);
        } else {
            Log.i(TAG, "context == null");
            return;
        }
        int currentId = -1;
      // sprd: Bug 426173 Modify the status of Audioprofile
        if (audioManager.getRingerModeInternal() == AudioManager.RINGER_MODE_SILENT) {
            currentId = AudioProfile.DEFAULT_SILENT_PROFILE_ID;
            Settings.System.putInt(mResolver, Settings.System.HAPTIC_FEEDBACK_ENABLED, 0);
            syncRingtoneToProfile(context, id);
            /* SPRD: 615052 save volume to profile when change mode from general to silent @{ */
            if(id == AudioProfile.DEFAULT_GENERAL_PROFILE_ID){
                AudioProfile.updateVolumeOfNormal(context, id);
            }
            /* @} */
          //sprd: Bug 426173 Modify the status of Audioprofile
        } else if (audioManager.getRingerModeInternal() == AudioManager.RINGER_MODE_VIBRATE) {
            currentId = AudioProfile.DEFAULT_VIBRATE_PROFILE_ID;
            Settings.System.putInt(mResolver, Settings.System.HAPTIC_FEEDBACK_ENABLED, 1);
            syncRingtoneToProfile(context, id);
        } else {
            /* SPRD: 620160 general mode's volume become max after take video in outdoor mode @{ */
            if (audioManager.getRingerModeInternal() == AudioManager.RINGER_MODE_NORMAL) {
                currentId = AudioProfile.DEFAULT_GENERAL_PROFILE_ID;
            } else if (audioManager.getRingerModeInternal() == AudioManager.RINGER_MODE_OUTDOOR) {
                currentId = AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID;
            }
            /* @} */
            AudioProfile.updateVolumeOfNormal(context, currentId);
            AudioProfile profile = AudioProfile.restoreProfileWithId(context, currentId);
            if (profile == null) {
                return;
            }
            if (profile.mIsVibrate == 1) {
                if (resolver != null) {
                    Settings.System.putInt(resolver, Settings.System.VIBRATE_WHEN_RINGING, 1);
                }
            } else {
                if (resolver != null) {
                    Settings.System.putInt(resolver, Settings.System.VIBRATE_WHEN_RINGING, 0);
                }
            }
            if (profile.mIsVibrateMsg == 1) {
                if (resolver != null) {
                    Settings.System.putInt(resolver, "vibrate_when_message", 1);
                }
            } else {
                if (resolver != null) {
                    Settings.System.putInt(resolver, "vibrate_when_message", 0);
                }
            }
            if (context != null) {
                for (int i = 0; i < mPhoneCount; i++) {
                    if (profile.mRingtoneUri[i] != null
                            && !TextUtils.isEmpty(profile.mRingtoneUri[i])) {
                        RingtoneManagerEx.setActualDefaultRingtoneUri(mContext, RingtoneManager.TYPE_RINGTONE,
                                profile.mRingtoneUri[i] != null ? Uri.parse(profile.mRingtoneUri[i]) : null, i);
                    }
                    /* SPRD: Modified for bug 567390, General message tone changes with customized profile @{ */
                    if (profile.mMessagetoneUri[i] != null
                            && !TextUtils.isEmpty(profile.mMessagetoneUri[i])) {
                        Settings.System.putString(mResolver, "messagetone" + i, profile.mMessagetoneUri[i]);
                    }
                    /* @} */
                }
                if (profile.mNotificationUri != null && !TextUtils.isEmpty(profile.mNotificationUri)) {
                    RingtoneManager.setActualDefaultRingtoneUri(context,
                            RingtoneManager.TYPE_NOTIFICATION, Uri.parse(profile.mNotificationUri));
                }
            }
            if (audioManager != null) {
                /* SPRD: Modified for bug 573433, Ringer volume is not correct after change mode @{ */
                /* SPRD: Modified for bug 575284, Mode cannot be changed by click status bar.@{ */
                if (profile.mRingtoneVolumeIndex != 0){
                    audioManager.setStreamVolume(AudioManager.STREAM_NOTIFICATION,
                            profile.mNotificationVolumeIndex, 0);
                    /* SPRD 604132@{ */
                    //SharedPreferences share = PreferenceManager.getDefaultSharedPreferences(mContext.getApplicationContext());
                    //mRingMode = share.getInt("mRingMode", 1);
                    if (mRingMode == 0){
                        audioManager.setStreamVolume(AudioManager.STREAM_RING,
                                profile.mRingtoneVolumeIndex, 0);
                    }else {
                        audioManager.setStreamVolume(AudioManager.STREAM_RING,audioManager.getStreamVolume(AudioManager.STREAM_RING), 0);
                    }
                    /* @} */
                } else {
                    audioManager.setStreamVolume(AudioManager.STREAM_NOTIFICATION,
                            audioManager.getStreamVolume(AudioManager.STREAM_NOTIFICATION), 0);
                    audioManager.setStreamVolume(AudioManager.STREAM_RING,
                            audioManager.getStreamVolume(AudioManager.STREAM_RING), 0);
                }
                /* @} */
                /* @} */
                audioManager.setStreamVolume(AudioManager.STREAM_ALARM, profile.mAlarmVolumeIndex, 0);
            }
            Log.d(TAG,"updateCurrentId profile.mMediaVolumeIndex=="+profile.mMediaVolumeIndex+"-STREAM_MUSIC--"+audioManager.getStreamVolume(AudioManager.STREAM_MUSIC));
            audioManager.setStreamVolume(AudioManager.STREAM_MUSIC, profile.mMediaVolumeIndex, 0);
            if (audioManager != null) {
                if (profile.mSoundEffects == AudioProfile.IS_SOUND_EFFECTS) {
                    audioManager.loadSoundEffects();
                } else {
                    audioManager.unloadSoundEffects();
                }
            }
            if (resolver != null) {
                Settings.System.putInt(resolver, Settings.System.NOTIFICATIONS_USE_RING_VOLUME,
                        profile.mNotificationsUseRingVolume);
                Settings.System.putInt(resolver, Settings.System.DTMF_TONE_WHEN_DIALING,
                        profile.mDTMFTone == AudioProfile.IS_DTMF_TONE ? 1 : 0);
                Settings.System.putInt(resolver, Settings.System.SOUND_EFFECTS_ENABLED,
                        profile.mSoundEffects == AudioProfile.IS_SOUND_EFFECTS ? 1 : 0);
                Settings.System.putInt(resolver, Settings.System.LOCKSCREEN_SOUNDS_ENABLED,
                        profile.mLockSounds == AudioProfile.IS_LOCK_SOUNDS ? 1 : 0);
            }
            if (resolver != null) {
                Settings.System.putInt(resolver, Settings.System.HAPTIC_FEEDBACK_ENABLED,
                        profile.mHapticFeedback == AudioProfile.IS_HAPTIC_FEEDBACK ? 1 : 0);
            }
        }
        saveProfileId(resolver, currentId);
        mSelectedId = currentId;
    }

    private static void setProfileIsSelected(int id) {
        Settings.System.putInt(mResolver, "currentAudioProfileId", id);
        final AudioProfile profile = AudioProfile.restoreProfileWithId(mContext, id);
        new Thread(new Runnable() {
            @Override
            public void run() {
                // TODO Auto-generated method stub
                if (profile != null && mContext != null) {
                    profile.setSelected(mContext);
                }
            }

        }).start();
        if (id == AudioProfile.DEFAULT_SILENT_PROFILE_ID) {
            // silent
          //sprd: Bug 426173 Modify the status of Audioprofile
            mAudioManager.setRingerModeInternal(AudioManager.RINGER_MODE_SILENT);
            Settings.System.putInt(mResolver, Settings.System.HAPTIC_FEEDBACK_ENABLED, 0);
        } else if (id == AudioProfile.DEFAULT_VIBRATE_PROFILE_ID) {
            // vibrate
          //sprd: Bug 426173 Modify the status of Audioprofile
            mAudioManager.setRingerModeInternal(AudioManager.RINGER_MODE_VIBRATE);
            mAudioManager.setVibrateSetting(AudioManager.VIBRATE_TYPE_RINGER,
                    AudioManager.VIBRATE_SETTING_ON);
            mAudioManager.setVibrateSetting(AudioManager.VIBRATE_TYPE_NOTIFICATION,
                    AudioManager.VIBRATE_SETTING_ON);
            Settings.System.putInt(mResolver, Settings.System.HAPTIC_FEEDBACK_ENABLED, 1);
        } else {
            if (id == AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID) {
              /* sprd: Bug 426173 Modify the status of Audioprofile @{ */
                mAudioManager.setRingerModeInternal(AudioManager.RINGER_MODE_OUTDOOR);
            } else {
                mAudioManager.setRingerModeInternal(AudioManager.RINGER_MODE_NORMAL);
            }
            /* @} */
            if (profile.mIsVibrate == 1) {
                Settings.System.putInt(mResolver, Settings.System.VIBRATE_WHEN_RINGING,1);
                mAudioManager.setVibrateSetting(AudioManager.VIBRATE_TYPE_RINGER, AudioManager.VIBRATE_SETTING_ON);
                mAudioManager.setVibrateSetting(AudioManager.VIBRATE_TYPE_NOTIFICATION, AudioManager.VIBRATE_SETTING_ON);
            } else {
                Settings.System.putInt(mResolver, Settings.System.VIBRATE_WHEN_RINGING,0);
                mAudioManager.setVibrateSetting(AudioManager.VIBRATE_TYPE_RINGER, AudioManager.VIBRATE_SETTING_OFF);
                mAudioManager.setVibrateSetting(AudioManager.VIBRATE_TYPE_NOTIFICATION, AudioManager.VIBRATE_SETTING_OFF);
            }
            if (profile.mIsVibrateMsg == 1) {
                Settings.System.putInt(mResolver, "vibrate_when_message", 1);
            } else {
                Settings.System.putInt(mResolver, "vibrate_when_message", 0);
            }
            for (int i = 0; i < mPhoneCount; i++) {
                if (profile.mRingtoneUri[i] != null && !TextUtils.isEmpty(profile.mRingtoneUri[i])) {
                    RingtoneManagerEx.setActualDefaultRingtoneUri(mContext,
                            RingtoneManager.TYPE_RINGTONE,
                            profile.mRingtoneUri[i] != null ? Uri.parse(profile.mRingtoneUri[i])
                                    : null, i);
                } else {
                    Uri uri = AudioProfile.getDefaultUri(mContext,RingtoneManager.TYPE_RINGTONE);
                    if(uri != null){
                        RingtoneManagerEx.setActualDefaultRingtoneUri(mContext,
                                RingtoneManager.TYPE_RINGTONE, uri, i);
                        ContentValues values = new ContentValues();
                        values.put((AudioProfileProvider.ringtoneStr[i]), uri.toString());
                        profile.update(mContext, values);
                    }

                }
                if (profile.mMessagetoneUri[i] != null && !TextUtils.isEmpty(profile.mMessagetoneUri[i])) {
                    Settings.System.putString(mResolver, "messagetone" + i, profile.mMessagetoneUri[i]);
                } else {
                    Uri uri = AudioProfile.getDefaultUri(mContext,RingtoneManager.TYPE_NOTIFICATION);
                    if(uri != null){
                        Settings.System.putString(mResolver, "messagetone" + i, uri.toString());
                        ContentValues values = new ContentValues();
                        values.put((AudioProfileProvider.mMessagetoneStr[i]), uri.toString());
                        profile.update(mContext, values);
                    }
                }
            }
            // TODO message uri
            if (profile.mNotificationUri != null && !TextUtils.isEmpty(profile.mNotificationUri)) {
                RingtoneManager.setActualDefaultRingtoneUri(mContext,
                        RingtoneManager.TYPE_NOTIFICATION, Uri.parse(profile.mNotificationUri));
            } else {
                Uri uri = AudioProfile.getDefaultUri(mContext, RingtoneManager.TYPE_NOTIFICATION);
                if(uri != null){
                    RingtoneManager.setActualDefaultRingtoneUri(mContext, RingtoneManager.TYPE_NOTIFICATION, uri);
                    ContentValues values = new ContentValues();
                    values.put(AudioProfileColumns.NOTIFICATION_URI, uri.toString());
                    profile.update(mContext, values);
                }
            }

            Log.d(TAG, "profile.mRingtoneVolumeIndex = " + profile + "." + profile.mRingtoneVolumeIndex);
            if (profile.mRingtoneVolumeIndex <= 0 || profile.mNotificationVolumeIndex <= 0) {
                int defaultRingVolume = 1;
                profile.mRingtoneVolumeIndex = defaultRingVolume;
                profile.mNotificationVolumeIndex = defaultRingVolume;
                ContentValues values = new ContentValues();
                values.put(AudioProfileColumns.RING_VOLUME_INDEX, defaultRingVolume);
                values.put(AudioProfileColumns.NOTIFICATION_VOLUME_INDEX, defaultRingVolume);
                values.put(AudioProfileColumns.IS_SILENT, 0);
                profile.update(mContext, values);
            }
            mAudioManager.setStreamVolume(AudioManager.STREAM_NOTIFICATION,
                    profile.mNotificationVolumeIndex, 0);
            /* SPRD: bug497313, can not change outdoor volume in any mode @{ */
            if (id == AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID) {
                mAudioManager.setStreamVolume(AudioManager.STREAM_RING, mAudioManager.getStreamMaxVolume(AudioManager.STREAM_RING), 0);
            } else {
                mAudioManager.setStreamVolume(AudioManager.STREAM_RING, profile.mRingtoneVolumeIndex, 0);
            }
            /* @} */
            mAudioManager.setStreamVolume(AudioManager.STREAM_ALARM, profile.mAlarmVolumeIndex, 0);

            /* SPRD:bug 498097,Set the profile to outdoor,the music voice suddenly become bigger @{ */
            if (id != AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID) {
                Log.d(TAG, "setProfileIsSelected profile.mMediaVolumeIndex==" + profile.mMediaVolumeIndex
                        + "-STREAM_MUSIC-" + mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC));
                mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, profile.mMediaVolumeIndex, 0);
            }
            /* @} */

            Settings.System.putInt(mResolver, Settings.System.NOTIFICATIONS_USE_RING_VOLUME,
                    profile.mNotificationsUseRingVolume);
            Settings.System.putInt(mResolver, Settings.System.DTMF_TONE_WHEN_DIALING,
                    profile.mDTMFTone == AudioProfile.IS_DTMF_TONE ? 1 : 0);
            Settings.System.putInt(mResolver, Settings.System.SOUND_EFFECTS_ENABLED,
                    profile.mSoundEffects == AudioProfile.IS_SOUND_EFFECTS ? 1 : 0);
            Settings.System.putInt(mResolver, Settings.System.LOCKSCREEN_SOUNDS_ENABLED,
                    profile.mLockSounds == AudioProfile.IS_LOCK_SOUNDS ? 1 : 0);
            if (profile.mSoundEffects == AudioProfile.IS_SOUND_EFFECTS) {
                mAudioManager.loadSoundEffects();
            } else {
                mAudioManager.unloadSoundEffects();
            }
            Settings.System.putInt(mResolver, Settings.System.HAPTIC_FEEDBACK_ENABLED,
                    profile.mHapticFeedback == AudioProfile.IS_HAPTIC_FEEDBACK ? 1 : 0);
        }

    }

    // End
    private void addNewAudioProfile(String name) {
        // update db
        ContentValues values = new ContentValues();
        values.put(AudioProfileColumns.DISPLAY_NAME, name);
        Uri defRingtoneUri = AudioProfile.getDefaultUri(mContext, RingtoneManager.TYPE_RINGTONE);
        Uri defNotUri = AudioProfile.getDefaultUri(mContext,
                RingtoneManager.TYPE_NOTIFICATION);
        String defRingUri = "";
        if (defRingtoneUri != null) {
            defRingUri = defRingtoneUri.toString();
        }
        String defNotificationUri = "";
        if (defNotUri != null)
            defNotificationUri = defNotUri.toString();
        for (int i = 0; i < mPhoneCount; i++) {
            values.put(AudioProfileProvider.ringtoneStr[i], defRingUri);
        }
        values.put(AudioProfileColumns.NOTIFICATION_URI, defNotificationUri);
        Uri uri = mResolver.insert(AudioProfile.CONTENT_URI, values);
        int id = (int) ContentUris.parseId(uri);

        // update preference
        String displayName = name;
        AudioProfilePreference pref = new AudioProfilePreference(this);
        pref.setKey(displayName);
        pref.setId(id);
        pref.setTitle(displayName);
        pref.setPersistent(false);
        pref.setChecked(false);
        pref.setOnPreferenceChangeListener(this);
        mUserDefinedCategory.addPreference(pref);

        // update map
        mAllProfilesMap.put(id, pref);

    }

    private void deleteAudioProfile(int id) {
        if (id < 0) {
            return;
        }
        // update db
        Uri uri = ContentUris.withAppendedId(AudioProfile.CONTENT_URI, id);
        mResolver.delete(uri, null, null);

        // update preference
        mUserDefinedCategory.removePreference(mAllProfilesMap.get(id));

        // update map
        mAllProfilesMap.remove(id);

        // update selected
        if (mSelectedId == id) {
            setProfileIsSelected(AudioProfile.DEFAULT_GENERAL_PROFILE_ID);
            mSelectedId = getSelectedId(true);
            updateSelectedState();
        }
    }

    private void updateAudioProfile(int id, String displayName) {
        // udpate db
        ContentValues values = new ContentValues();
        values.put(AudioProfileColumns.DISPLAY_NAME, displayName);
        mResolver.update(
                ContentUris.withAppendedId(AudioProfile.CONTENT_URI, id),
                values, null, null);
        // update preference
        mAllProfilesMap.get(id).setTitle(displayName);
    }

    private void resetAudioProfile() {
        // reset
        getContentResolver().delete(AudioProfile.CONTENT_URI, null, null);
        // rest map
        HashMap<Integer, AudioProfilePreference> tempMap = new HashMap<Integer, AudioProfilePreference>();
        tempMap.put(AudioProfile.DEFAULT_GENERAL_PROFILE_ID,
                mAllProfilesMap.get(AudioProfile.DEFAULT_GENERAL_PROFILE_ID));
        tempMap.put(AudioProfile.DEFAULT_SILENT_PROFILE_ID,
                mAllProfilesMap.get(AudioProfile.DEFAULT_SILENT_PROFILE_ID));
        tempMap.put(AudioProfile.DEFAULT_VIBRATE_PROFILE_ID,
                mAllProfilesMap.get(AudioProfile.DEFAULT_VIBRATE_PROFILE_ID));
        tempMap.put(AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID,
                mAllProfilesMap.get(AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID));
        mUserDefinedCategory.removeAll();
        mAllProfilesMap.clear();
        mAllProfilesMap = null;
        mAllProfilesMap = tempMap;
        setProfileIsSelected(AudioProfile.DEFAULT_GENERAL_PROFILE_ID);
        mSelectedId = getSelectedId(true);
        updateSelectedState();
    }

    private static void syncNormalProfile(){
        AudioProfile profile = AudioProfile.restoreProfileWithId(mContext,AudioProfile.DEFAULT_GENERAL_PROFILE_ID);
        profile.updateVolume(mContext);
    }

    private static int getSelectedId(boolean isUpdateProfile) {

        int currentId = Settings.System.getInt(mResolver, "currentAudioProfileId", -1);
        int id = -1;
        int systemId = -1;
        //sprd: Bug 426173 Modify the status of Audioprofile
        switch (mAudioManager.getRingerModeInternal()) {
            case AudioManager.RINGER_MODE_SILENT:
                systemId = AudioProfile.DEFAULT_SILENT_PROFILE_ID;
                break;
            case AudioManager.RINGER_MODE_VIBRATE:
                systemId = AudioProfile.DEFAULT_VIBRATE_PROFILE_ID;
                break;
            case AudioManager.RINGER_MODE_OUTDOOR:
                systemId = AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID;
                break;
            case AudioManager.RINGER_MODE_NORMAL:
                systemId = AudioProfile.DEFAULT_GENERAL_PROFILE_ID;
                break;
        }
        Log.d(TAG,"systemId = "+systemId+"  currentId = "+currentId);
        if (currentId != -1) {
            id = currentId;
        } else {
            // if not find selected id (delete), set general profile is
            // selected.
            if (systemId != -1) {
                id = systemId;
            } else {
                id = AudioProfile.DEFAULT_GENERAL_PROFILE_ID;
            }
            if (isUpdateProfile) {
                Log.d(TAG, "getSelectedId enter isUpdateProfile id==" + id);
                if (id == AudioProfile.DEFAULT_GENERAL_PROFILE_ID) {
                    syncNormalProfile();
                }
                setProfileIsSelected(id);
            }
        }

        // sometimes,need refresh id from systemId.For example,use id
        // =normal,change volume to 0,systemId changed.
        if ((systemId != -1) && (id != -1) && (systemId != id)) {
            if (!(id > AudioProfile.PREDEFINED_PROFILE_COUNT && systemId == AudioProfile.DEFAULT_GENERAL_PROFILE_ID)) {
                id = systemId;
                if (isUpdateProfile) {
                   setProfileIsSelected(id);
                }
            }

            Cursor cursor = mResolver.query(AudioProfile.CONTENT_URI,
                    new String[] {AudioProfileColumns.ID}, AudioProfileColumns.ID + "=?",
                    new String[] {String.valueOf(id)}, null);
            if (cursor != null && cursor.moveToFirst()) {

            } else {
                id = systemId;
                if (isUpdateProfile) {
                    setProfileIsSelected(id);
                 }
            }
            if (cursor != null) {
                cursor.close();
                cursor = null;
            }

        }
        return id;
    }

    private boolean existDuplicateDisplayName(String name) {
        boolean result = false;
        Cursor cursor = managedQuery(AudioProfile.CONTENT_URI, new String[] {
                AudioProfileColumns.ID, AudioProfileColumns.DISPLAY_NAME
        },
                AudioProfileColumns.DISPLAY_NAME + "=? AND " + AudioProfileColumns.ID + ">?",
                new String[] {
                        name, String.valueOf(AudioProfile.PREDEFINED_PROFILE_COUNT)
                },
                null);
        if (cursor != null && cursor.getCount() > 0) {
            result = true;
        }
        // cursor.close();
        return result;
    }

    /* SPRD: 427745 The status was not changed when in outdoor mode with seekbar to adjust the volume @{ */
    private static void registerContentObservers(){
        Uri volumnUri=Settings.System.getUriFor(Volume_Ring_Speaker);
        if (volumnUri != null) {
            mResolver.registerContentObserver(volumnUri, true, mObserver);
        }
    }

    /* SPRD 491048 Automatically exit the editor interface @{ */
    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (msg.what == 1) {
                int currentVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_SYSTEM);
                if (mSelectedId == AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID
                        && currentVolume < mAudioManager.getStreamMaxVolume(AudioManager.STREAM_RING)
                        && currentVolume > 0 && isInProfile) {
                    mSelectedId = AudioProfile.DEFAULT_GENERAL_PROFILE_ID;
                    setProfileIsSelected(mSelectedId);
                    updateSelectedState();
                    syncSystemToProfile();
                    syncRingtoneToProfile(mContext, mSelectedId);
                }
            }
        }
    };
    /* @} */
    /* @} */

    /* SPRD: bug 449112 & 476713  Audioprofile crash & display was wrong @{ */
    private void refresh() {
        if (mUserDefinedCategory != null) {
            mUserDefinedCategory.removeAll();
        }
        getLoaderManager().restartLoader(0, null, this);
    }
    /* @} */

    /* SPRD: bug462372, add for magic sound @{ */
    @Override
    public boolean onPreferenceClick(Preference preference) {
        // TODO Auto-generated method stub
        Log.d(TAG,"mgaic sound mAudioManager.getParameters(MAGIC_AUDIO) = "+mAudioManager.getParameters(MAGIC_AUDIO));
        if (mAudioManager.getParameters(MAGIC_AUDIO).equals(MAGIC_AUDIO_CLOSED) && (preference == mSoundCheckBoxPreference) ) {
        //if (SystemProperties.get(KEY_MAGIC_AUDIO_PROPERTY, "2").equals(MAGIC_AUDIO_CLOSED)) {
            mAudioManager.setParameter(MAGIC_AUDIO, MAGIC_AUDIO_OPEN);
            mSoundCheckBoxPreference.setChecked(true);
            SystemProperties.set(KEY_MAGIC_AUDIO_PROPERTY, "1");
            Log.d(TAG,"mgaic sound SystemProperties.set(KEY_MAGIC_AUDIO_PROPERTY) = 1 ");
        } else if (preference == mSoundCheckBoxPreference) {
            mAudioManager.setParameter(MAGIC_AUDIO, MAGIC_AUDIO_CLOSED);
            mSoundCheckBoxPreference.setChecked(false);
            SystemProperties.set(KEY_MAGIC_AUDIO_PROPERTY, "0");
            Log.d(TAG,"mgaic sound  SystemProperties.set(KEY_MAGIC_AUDIO_PROPERTY) = 0 ");
        }
        return true;
    }
    /* @} */
}
