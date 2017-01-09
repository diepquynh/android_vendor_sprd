package com.sprd.audioprofile;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.List;
import java.util.Arrays;

import android.app.ActionBar;
import android.content.BroadcastReceiver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.database.sqlite.SQLiteException;
import android.media.AudioManager;
import android.media.RingtoneManager;
import android.media.RingtoneManagerEx;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.os.SystemProperties;
import android.os.UserManager;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.provider.MediaStore;
import android.provider.Settings;
import android.provider.SettingsEx;
import android.telephony.TelephonyManager;
import android.telephony.SubscriptionManager;
//import android.telephony.SubInfoRecord;
import android.telephony.SubscriptionInfo;
import android.text.TextUtils;
import android.util.Log;
import android.view.MenuItem;
import android.preference.PreferenceCategory;
import android.app.AlertDialog;
import android.app.Dialog;
import android.text.TextWatcher;
import android.widget.EditText;
import android.text.Editable;
import android.widget.Toast;
import android.content.DialogInterface;
import android.content.ContentUris;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.res.Configuration;

import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.PhoneConstants;

public class AudioProfileSoundSettings extends PreferenceActivity {
    private static final String TAG = "AudioProfileSoundSettings";

    private static final int TEXT_MAX = 32;

    private static final String KEY_VIBRATE = "vibrate";
    private static final String KEY_VIBRATE_MSG = "vibrate_msg";
    private static final String KEY_DTMF_TONE = "dtmf_tone";
    private static final String KEY_CHARGING_SOUNDS = "charging_sounds";
    private static final String KEY_RENAME_PROFILE = "rename_profile";
    // SPRD: Bug 608515
    private static final String VIBRATE_CATEGORY_FEEDBACK="vibrate_category_feedback";

    private static final String KEY_SOUND_EFFECTS = "sound_effects";
    private static final String KEY_HAPTIC_FEEDBACK = "haptic_feedback";
    private static final String KEY_LOCK_SOUNDS = "lock_sounds";

    public static final String KEY_SIM_RINGTONE = "ringtone";
    public static final String KEY_SIM_RINGTONE_CATEGORY = "category_calls_and_notification";

    public static final String KEY_SIM_MESSAGETONE = "messagetone";
    public static String [] mMessagetoneKey;
    private AudioProfileRingtonePreference [] mMessagetonePreference;
    private static final int MSG_UPDATE_MESSAGE_SUMMARY = 3;
    public static final int RINGTONETYPE_MESSAGE = 3;

    public static String[] ringtoneKey;

    private static final String KEY_NOTIFICATION = "notification_sound";
    private PreferenceCategory mRingtoneCategory;
    // SPRD: Bug 608515
    private PreferenceCategory mVibrateCategory;
    private AudioProfileRingtonePreference mNotificationPreference;
    private AudioProfileRingtonePreference[] mRingtonePreference;
    private Runnable mRingtoneLookupRunnable;
    private static final int MSG_UPDATE_RINGTONE_SUMMARY = 1;
    private static final int MSG_UPDATE_NOTIFICATION_SUMMARY = 2;
    private int mPhoneCount = 1;

    public static boolean UNIVERSEUI_SUPPORT = SystemProperties.getBoolean("universe_ui_support",
            false);
    // public static final boolean SUPPORT_MULTISIM_RINGTONE =
    // Settings.System.IS_MULTI_CARD_RINGTONE_SUPPORT;
    private CheckBoxPreference mVibrate;
    private CheckBoxPreference mVibrateMsg;

    private Preference mRenameProfile;
    private CheckBoxPreference mDtmfTone;
    // SPRD: Modified for bug 510013
    //private CheckBoxPreference mChargingSounds;
    private CheckBoxPreference mSoundEffects;
    private CheckBoxPreference mHapticFeedback;
    private CheckBoxPreference mLockSounds;
    private AudioProfileRingerVolumePreference mVolume;

    private AudioManager mAudioManager;

    private AudioProfile mAudioProfile = null;
    private int mAudioProfileId;
    private boolean mIsSelected = false;
    private boolean isFrist = true;
    private int mEditId;
    private static Uri mDefaultRingtoneUri, mDefaultNotificationUri;

    private Context mContext;
    private ContentResolver mResolver;
    // SPRD: Add for bug 453949
    private boolean mSimInside[];
    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(AudioManager.RINGER_MODE_CHANGED_ACTION)) {
                Log.d(TAG, "receive action: RINGER_MODE_CHANGED_ACTION");
                if (isFrist) {
                    isFrist = false;
                } else {
                    Intent it = new Intent();
                    it.setAction("com.sprd.action.AUDIO_PROFILE");
                    startActivity(it);
                    if (mVolume != null && mVolume.mSeekBarVolumizer[0] != null) {
                        mVolume.mSeekBarVolumizer[0].mOriginalSystemVolume = mAudioManager
                                .getStreamVolume(AudioManager.STREAM_RING);
                    }
                }

            } else if (intent.getAction().equals(TelephonyIntents.ACTION_SIM_STATE_CHANGED)) {
                // SPRD: Add for bug 453949
                if (getResources().getBoolean(
                        com.android.internal.R.bool.config_hotswapCapable) == false) {
                    Log.d(TAG, "not support hot swap");
                    return;
                }
                String state = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
                int phoneId = intent.getIntExtra(PhoneConstants.PHONE_KEY,
                        SubscriptionManager.INVALID_SUBSCRIPTION_ID);
                if (IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(state)
                        || IccCardConstants.INTENT_VALUE_ICC_LOADED.equals(state)
                        || IccCardConstants.INTENT_VALUE_ICC_UNKNOWN.equals(state)
                        || IccCardConstants.INTENT_VALUE_ICC_LOCKED.equals(state)) {
                    boolean hasIccCard = mSimInside[phoneId];
                    mSimInside[phoneId] = !IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(state);
                    if (mSimInside[phoneId] && !hasIccCard || !mSimInside[phoneId] && hasIccCard) {
                        setRingTone(phoneId);
                    }
                }
            /* SPRD: Modified for bug 591802, set Music stream max volume when headset on @{ */
            } else if (Intent.ACTION_HEADSET_PLUG.equals(intent.getAction())) {
              if (mVolume.getDialog() != null) {
                  mVolume.getDialog().dismiss();
              }
          }
            /* @} */
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "AudioProfileSoundSettings----onCreate");
        // Adding ActionBar style make sure that, user can pop menu to uplevel.
        ActionBar actionBar = getActionBar();
        actionBar.setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP
                , ActionBar.DISPLAY_HOME_AS_UP);
        actionBar.setDisplayHomeAsUpEnabled(true);
        actionBar.setDisplayShowTitleEnabled(true);
        mContext = this;
        mResolver = getContentResolver();
        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);

        addPreferencesFromResource(R.xml.audio_profile_sound_settings);
        mRenameProfile = findPreference(KEY_RENAME_PROFILE);
        mEditId = getIntent().getIntExtra(AudioProfileSettings.EDIT_ID_KEY, -1);

        /* SPRD: bug 462545, intentfuzz-activity,Settings Crash @{ */
        if (getIntent().getAction() == null) {
            finish();
            return;
        }
        /* @} */

        if (mEditId < AudioProfile.PREDEFINED_PROFILE_COUNT) {
            getPreferenceScreen().removePreference(mRenameProfile);
        }
        mAudioProfileId = Settings.System.getInt(getContentResolver(), "currentAudioProfileId", -1);
        if (mEditId != -1) {
            mAudioProfile = AudioProfile.restoreProfileWithId(this, mEditId);
            if (mAudioProfile.mIsSelected == AudioProfile.IS_SELECTED) {
                mIsSelected = true;
            }
        } else {
            throw new RuntimeException("get audio profile id error");
        }
        Log.i(TAG, "currentAudioProfileId = " + mAudioProfileId + "mEditId = " + mEditId
                + "mIsSelected = " + mIsSelected);
        mVibrate = (CheckBoxPreference) findPreference(KEY_VIBRATE);
        if (mAudioProfile.mIsVibrate == AudioProfile.IS_VIBRATE) {
            mVibrate.setChecked(true);
        } else {
            mVibrate.setChecked(false);
        }
        /* SPRD: Bug 608515  @{ */
        mVibrateCategory = (PreferenceCategory) findPreference(VIBRATE_CATEGORY_FEEDBACK);
        mVibrateCategory.setOrderingAsAdded(false);
        mVibrateMsg = (CheckBoxPreference) findPreference(KEY_VIBRATE_MSG);
        if (isGMSVersion()) {
            if(mVibrateCategory != null && mVibrateMsg != null) {
                mVibrateCategory.removePreference(mVibrateMsg);
            }
        } else {
            if (mAudioProfile.mIsVibrateMsg == AudioProfile.IS_VIBRATE_MSG) {
                mVibrateMsg.setChecked(true);
            } else {
                mVibrateMsg.setChecked(false);
            }
        }
        /* @} */

        mVolume = (AudioProfileRingerVolumePreference) findPreference("ring_volume");
        mVolume.setEditId(mEditId);
        mDtmfTone = (CheckBoxPreference) findPreference(KEY_DTMF_TONE);
        mDtmfTone.setPersistent(false);
        mDtmfTone.setChecked(mAudioProfile.mDTMFTone == AudioProfile.IS_DTMF_TONE);

        mSoundEffects = (CheckBoxPreference) findPreference(KEY_SOUND_EFFECTS);
        mSoundEffects.setPersistent(false);
        mSoundEffects.setChecked(mAudioProfile.mSoundEffects == AudioProfile.IS_SOUND_EFFECTS);
        mHapticFeedback = (CheckBoxPreference) findPreference(KEY_HAPTIC_FEEDBACK);
        mHapticFeedback.setPersistent(false);

        mHapticFeedback
                .setChecked(mAudioProfile.mHapticFeedback == AudioProfile.IS_HAPTIC_FEEDBACK);

        mLockSounds = (CheckBoxPreference) findPreference(KEY_LOCK_SOUNDS);
        mLockSounds.setPersistent(false);
        mLockSounds.setChecked(mAudioProfile.mLockSounds == AudioProfile.IS_LOCK_SOUNDS);
        /* SPRD: Modified for bug 510013 @{ */
        /*mChargingSounds = (CheckBoxPreference) findPreference(KEY_CHARGING_SOUNDS);
        mChargingSounds.setPersistent(false);
        mChargingSounds.setChecked(mAudioProfile.mChargingSounds == AudioProfile.IS_CHARGING_SOUNDS);*/
        /* @} */

        // Add for ringtone summary
        mRingtoneCategory = (PreferenceCategory) findPreference(KEY_SIM_RINGTONE_CATEGORY);
        mRingtoneCategory.setOrderingAsAdded(false);
        final TelephonyManager telephonyManager = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
        mPhoneCount = telephonyManager.getPhoneCount();
        mRingtonePreference = new AudioProfileRingtonePreference[mPhoneCount];
        ringtoneKey = new String[mPhoneCount];

        mMessagetoneKey = new String[mPhoneCount];
        mMessagetonePreference = new AudioProfileRingtonePreference[mPhoneCount];

        SubscriptionManager subscriptionManager = new SubscriptionManager(mContext);
        List<SubscriptionInfo> subInfoRecords = subscriptionManager.getActiveSubscriptionInfoList();
        if (subInfoRecords == null) {
            subInfoRecords = new ArrayList<SubscriptionInfo>();
        }
        /* SPRD: Add for bug 453949 @{ */
        mSimInside = new boolean[mPhoneCount];
        Arrays.fill(mSimInside, false);
        for (int i = 0; i < subInfoRecords.size(); i++) {
            mSimInside[subInfoRecords.get(i).getSimSlotIndex()] = true;
        }
        /* @} */

        mRingtoneLookupRunnable = new Runnable() {
            public void run() {
                for (int i = 0; i < mPhoneCount; i++) {
                    //SPRD: Add for bug 550229
                    if (mRingtonePreference[i] != null /*&& (TelephonyManager
                            .from(mContext)).hasIccCard(i)*/) {
                        updateRingtoneName(RingtoneManager.TYPE_RINGTONE,
                                i, mRingtonePreference[i], MSG_UPDATE_RINGTONE_SUMMARY);
                    }
                    //SPRD: Add for bug 550229
                    if (mMessagetonePreference[i] != null /*&& (TelephonyManager
                            .from(mContext)).hasIccCard(i)*/) {
                        updateRingtoneName(RINGTONETYPE_MESSAGE,
                                i, mMessagetonePreference[i], MSG_UPDATE_MESSAGE_SUMMARY);
                    }

                }
                if (mNotificationPreference != null) {
                    updateRingtoneName(RingtoneManager.TYPE_NOTIFICATION,
                            0, mNotificationPreference, MSG_UPDATE_NOTIFICATION_SUMMARY);
                }
            }
        };

        /* SPRD: Add for bug 453949 @{ */
        for (int i = 0; i < mPhoneCount; i++) {
            mRingtonePreference[i] = new AudioProfileRingtonePreference(this, null);
            mMessagetonePreference[i] = new AudioProfileRingtonePreference(this, null);
            setRingTone(i);
        }
        getDefaultUri(this);
        /* @} */
        IntentFilter filter = new IntentFilter();
        filter.addAction(AudioManager.RINGER_MODE_CHANGED_ACTION);
        // SPRD: Add for bug 453949
        filter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        // SPRD: Add for bug 591802
        filter.addAction(Intent.ACTION_HEADSET_PLUG);
        registerReceiver(mReceiver, filter);
    }

    /* SPRD 602407: modify the title{@ */
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        // TODO Auto-generated method stub
        super.onConfigurationChanged(newConfig);
        ActionBar actionBar = getActionBar();
        if (actionBar != null && getResources() != null) {
            actionBar.setTitle(AudioProfileSettings.setActionText(getResources(), actionBar, newConfig));
        }
    }
    /* @} */

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            int index = msg.arg1;
            switch (msg.what) {
                case MSG_UPDATE_RINGTONE_SUMMARY:
                    if (mRingtonePreference[index] != null) {
                        mRingtonePreference[index].setSummary((CharSequence) msg.obj);
                    }
                    break;
                case MSG_UPDATE_NOTIFICATION_SUMMARY:
                    mNotificationPreference.setSummary((CharSequence) msg.obj);
                    break;
                case MSG_UPDATE_MESSAGE_SUMMARY:
                    if (mMessagetonePreference[index] != null) {
                        mMessagetonePreference[index].setSummary((CharSequence) msg.obj);
                    }
                    break;
            }
        }
    };

    /* SPRD 627906 set ringtone to SIM1 after change language{@ */
    private static int phoneID = 0;
    public static void setPhoneID(int mPhoneID) {
        phoneID = mPhoneID;
    }
    public static int getPhoneID() {
        return phoneID;
    }
    /* @} */

    /**
     * SPRD: Add for bug 453949. for refresh sim card ringtone state. @{
     *
     * @param phoneId
     */
    private void setRingTone(int phoneId) {
        SubscriptionManager subscriptionManager = new SubscriptionManager(mContext);
        List<SubscriptionInfo> subInfoRecords = subscriptionManager.getActiveSubscriptionInfoList();
        if (subInfoRecords == null) {
            subInfoRecords = new ArrayList<SubscriptionInfo>();
        }
        ringtoneKey[phoneId] = KEY_SIM_RINGTONE + phoneId;
        mRingtonePreference[phoneId].setKey(ringtoneKey[phoneId]);
        mRingtonePreference[phoneId].setPersistent(false);
        mRingtonePreference[phoneId]
                .setTitle(getString(R.string.sim_ringtone_title, (phoneId + 1), " "));
        mRingtonePreference[phoneId].setRingtoneType(RingtoneManager.TYPE_RINGTONE);
        mRingtonePreference[phoneId].setSummary(R.string.ringtone_summary);

        mRingtonePreference[phoneId].setEditId(mEditId);
        mRingtonePreference[phoneId].setOrder(phoneId);
        mRingtoneCategory.addPreference(mRingtonePreference[phoneId]);

        mMessagetoneKey[phoneId] = KEY_SIM_MESSAGETONE + phoneId;
        mMessagetonePreference[phoneId].setKey(mMessagetoneKey[phoneId]);
        mMessagetonePreference[phoneId].setPersistent(false);
        mMessagetonePreference[phoneId].setTitle(getString(R.string.sim_messagetone_title, (phoneId + 1), " "));
        mMessagetonePreference[phoneId].setRingtoneType(RINGTONETYPE_MESSAGE);
        mMessagetonePreference[phoneId].setSummary(R.string.messagetone_summary);
        mMessagetonePreference[phoneId].setEditId(mEditId);
        mMessagetonePreference[phoneId].setOrder(phoneId+2);
        /* SPRD: Bug 608515  @{ */
        if (!isGMSVersion()) {
            mRingtoneCategory.addPreference(mMessagetonePreference[phoneId]);
        }
        /* @} */

        if (TelephonyManager.from(mContext).hasIccCard(phoneId)) {
            mRingtonePreference[phoneId].setEnabled(true);
            mMessagetonePreference[phoneId].setEnabled(true);
            for (int i = 0; i < subInfoRecords.size(); i++) {
                if (subInfoRecords.get(i).getSimSlotIndex() == phoneId) {
                    mRingtonePreference[phoneId].setTitle(getString(R.string.sim_ringtone_title,
                            (subInfoRecords.get(i).getSimSlotIndex() + 1),
                            subInfoRecords.get(i).getDisplayName()));
                    mMessagetonePreference[phoneId].setTitle(getString(R.string.sim_messagetone_title,
                            (subInfoRecords.get(i).getSimSlotIndex() + 1),
                            subInfoRecords.get(i).getDisplayName()));
                }
            }
        } else {
            mRingtonePreference[phoneId].setEnabled(false);
            mMessagetonePreference[phoneId].setEnabled(false);
            mRingtonePreference[phoneId].setTitle(getString(R.string.sim_ringtone_title,
                    (phoneId + 1), " ", R.string.ringtone_title));
            mMessagetonePreference[phoneId].setTitle(getString(R.string.sim_messagetone_title,
                    (phoneId + 1), " ", R.string.messagetone_title));
        }
        mNotificationPreference = (AudioProfileRingtonePreference) findPreference(KEY_NOTIFICATION);
        mNotificationPreference.setEditId(mEditId);
        lookupRingtoneNames();
    }

    /** @} */
    private void lookupRingtoneNames() {
        new Thread(mRingtoneLookupRunnable).start();
    }

    public static void getDefaultUri(Context context) {
        String ringerUriString = Settings.System.getString(context.getContentResolver(),
                SettingsEx.SystemEx.DEFAULT_RINGTONE);
        mDefaultRingtoneUri = (ringerUriString != null ? Uri.parse(ringerUriString) : null);
        String notificationUriString = Settings.System.getString(context.getContentResolver(),
                SettingsEx.SystemEx.DEFAULT_NOTIFICATION);
        mDefaultNotificationUri = (notificationUriString != null ? Uri.parse(notificationUriString)
                : null);
        Log.d(TAG, "mDefaultRingtoneUri = " + mDefaultRingtoneUri + " mDefaultNotificationUri = "
                + mDefaultNotificationUri);
    }

    private void updateRingtoneName(int type, int index, Preference preference, int msgId) {
        if (preference == null)
            return;
        Context context = this;
        if (context == null)
            return;
        Uri ringtoneUri = null;
        if (mIsSelected) {
            if (type == RingtoneManager.TYPE_RINGTONE) {
                ringtoneUri = RingtoneManagerEx.getActualDefaultRingtoneUri(context, type, index);
                if (ringtoneUri != null) {
                    ContentValues values = new ContentValues();
                    if (AudioProfileProvider.ringtoneStr[index] != null) {
                        values.put(AudioProfileProvider.ringtoneStr[index], ringtoneUri.toString());
                        mAudioProfile.update(context, values);
                    }
                }
            } else if (type == RingtoneManager.TYPE_NOTIFICATION){
                ringtoneUri = RingtoneManager.getActualDefaultRingtoneUri(context, type);
                ContentValues values = new ContentValues();
                if (ringtoneUri != null) {
                    values.put(AudioProfileColumns.NOTIFICATION_URI,
                            ringtoneUri == null ? null : ringtoneUri.toString());
                    mAudioProfile.update(context, values);
                }
            } else {
                String messagetone = Settings.System.getString(context.getContentResolver(), KEY_SIM_MESSAGETONE + index);
                if(messagetone == null){
                    Uri defaultUri = RingtoneManager.getActualDefaultRingtoneUri(context, RingtoneManager.TYPE_NOTIFICATION);
                    if(defaultUri != null){
                        messagetone = defaultUri.toString();
                        Settings.System.putString(context.getContentResolver(), KEY_SIM_MESSAGETONE + index, messagetone);
                        ringtoneUri = defaultUri;
                    }else{
                        ringtoneUri = null;
                    }
                }else{
                    try {
                        ParcelFileDescriptor pfd = null;
                        pfd = context.getContentResolver().openFileDescriptor(
                             Uri.parse(messagetone), "r");
                        if (pfd != null) {
                            ringtoneUri = Uri.parse(messagetone);
                            pfd.close();
                        }
                    } catch (FileNotFoundException ex) {
                        /* SPRD: Modified for bug 564645,set default message tone @{ */
                        ringtoneUri = Uri.parse(Settings.System.getString(context.getContentResolver(),
                                SettingsEx.SystemEx.DEFAULT_NOTIFICATION));
                        Settings.System.putString(context.getContentResolver(),
                                KEY_SIM_MESSAGETONE + index, ringtoneUri.toString());
                        /* @} */
                    } catch (Exception sqle) {
                        Log.d(TAG, sqle.toString());
                    }
                }
                if (ringtoneUri != null) {
                    ContentValues values = new ContentValues();
                    if (AudioProfileProvider.mMessagetoneStr[index] != null) {
                        values.put(AudioProfileProvider.mMessagetoneStr[index], ringtoneUri.toString());
                        mAudioProfile.update(context, values);
                    }
                }
            }
        } else {
            String stringUri = AudioProfileColumns.RING_URI + 0;
            if (type == RingtoneManager.TYPE_RINGTONE) {
                if (AudioProfileProvider.ringtoneStr[index] != null) {
                    stringUri = AudioProfileProvider.ringtoneStr[index];
                }
            } else if(type == RingtoneManager.TYPE_NOTIFICATION){
                stringUri = AudioProfileColumns.NOTIFICATION_URI;
            } else {
                if (AudioProfileProvider.mMessagetoneStr[index] != null) {
                    stringUri = AudioProfileProvider.mMessagetoneStr[index];
                }
            }
            Cursor cursor = getContentResolver().query(AudioProfile.CONTENT_URI, new String[] {
                    stringUri
            }, AudioProfileColumns.ID + "=" + mAudioProfile.mId, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                String ringtoneString = cursor.getString(cursor.getColumnIndex(stringUri));
                if (ringtoneString != null) {
                    ringtoneUri = Uri.parse(ringtoneString);
                }
            }
            if (cursor != null) {
                cursor.close();
                cursor = null;
            }
        }
        CharSequence summary = context.getString(com.android.internal.R.string.ringtone_default);
        Cursor defCursor = null;
        try {
            if (type == RingtoneManager.TYPE_RINGTONE && mDefaultRingtoneUri != null) {
                defCursor = context.getContentResolver().query(mDefaultRingtoneUri,
                        new String[] {
                                MediaStore.Audio.Media.TITLE, MediaStore.Audio.Media.DATA
                        }, null, null, null);
            } else if (type == RingtoneManager.TYPE_NOTIFICATION && mDefaultNotificationUri != null) {
                defCursor = context.getContentResolver().query(mDefaultNotificationUri,
                        new String[] {
                                MediaStore.Audio.Media.TITLE, MediaStore.Audio.Media.DATA
                        }, null, null, null);
            } else if (type == RINGTONETYPE_MESSAGE && mDefaultNotificationUri != null){
                defCursor = context.getContentResolver().query(mDefaultNotificationUri,
                        new String[] {
                                MediaStore.Audio.Media.TITLE, MediaStore.Audio.Media.DATA
                        }, null, null, null);
            }
            if (defCursor != null && defCursor.getCount() > 0) {
                if (defCursor.moveToFirst()) {
                    summary = defCursor.getString(0);
                }
            }
        } catch (SQLiteException sqle) {
            Log.d(TAG, "sqle=" + sqle.toString());
        } finally {
            if (defCursor != null) {
                defCursor.close();
                defCursor = null;
            }
        }
        // Is it a silent ringtone?
        if (ringtoneUri != null) {
            Log.d(TAG, "ringtoneUri = " + ringtoneUri.toString());
        }
        if (ringtoneUri == null) {
            updateRingtoneUri(type, index, context);
        } else if (TextUtils.isEmpty(ringtoneUri.toString())) {
            updateRingtoneUri(type, index, context);
        } else {
            // Fetch the ringtone title from the media provider
            Cursor cursor = null;
            try {
                cursor = context.getContentResolver().query(ringtoneUri, new String[] {
                        MediaStore.Audio.Media.TITLE, MediaStore.Audio.Media.DATA
                }, null, null, null);
                if (cursor != null && cursor.getCount() > 0) {
                    if (cursor.moveToFirst()) {
                        File filePath = new File(cursor.getString(1));
                        if (filePath.exists() && !RingtoneManager.isDefault(ringtoneUri)) {
                            summary = cursor.getString(0);
                            Log.d(TAG, "filePath is exists");
                            if (summary == null) {
                                Log.d(TAG, "summary is null");
                                summary = context
                                        .getString(com.android.internal.R.string.ringtone_default);
                                updateRingtoneUri(type, index, context);
                            }
                        } else if (!filePath.exists()) { // exist in db but the
                            // file is deleted
                            Log.d(TAG, "filePath is not exists");
                            updateRingtoneUri(type, index, context);
                        }
                    }
                } else {
                    Log.d(TAG, "cursor is null");
                    updateRingtoneUri(type, index, context);
                }
            } catch (SQLiteException sqle) {
                Log.e(TAG, sqle.toString());
            } catch (IllegalArgumentException e) {
                Log.e(TAG, "IllegalArgumentException " + e);
            } finally {
                if (cursor != null) {
                    cursor.close();
                    cursor = null;
                }
            }
        }
        Log.d(TAG, "summary = " + summary.toString());

        Message msg = mHandler.obtainMessage();
        msg.arg1 = index;
        msg.what = msgId;
        msg.obj = summary;
        mHandler.sendMessage(msg);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Catch pop menu action
        if (item.getItemId() == android.R.id.home) {
            finish();
            return super.onOptionsItemSelected(item);
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onResume() {
        super.onResume();
        //SPRD:600156:update default uri in onResume
        getDefaultUri(this);
        //SPRD 601316: update the selected state in onResume
        updateSelectedState();
        lookupRingtoneNames();
        mVolume.mSoundEnable = true;
        /* SPRD: Add for bug 507924 @{ */
        UserManager mUserManager = (UserManager) this.getSystemService(Context.USER_SERVICE);
        if (!mUserManager.isSystemUser()) {
            mVibrate.setEnabled(false);
            /* SPRD: Bug 608515  @{ */
            if (!isGMSVersion()) {
                mVibrateMsg.setEnabled(false);
            }
            /* @} */
            mNotificationPreference.setEnabled(false);
            mLockSounds.setEnabled(false);
            for (int i = 0; i < mPhoneCount; i++) {
                mRingtonePreference[i].setEnabled(false);
                mMessagetonePreference[i].setEnabled(false);
            }
        }
        /* @} */
    }

    /*SPRD 601316: update the selected state in onResume @{ */
    private void updateSelectedState() {
        mIsSelected = false;
        mAudioProfileId = Settings.System.getInt(getContentResolver(), "currentAudioProfileId", -1);
        if (mEditId != -1) {
            mAudioProfile = AudioProfile.restoreProfileWithId(this, mEditId);
            if (mAudioProfile.mIsSelected == AudioProfile.IS_SELECTED) {
                mIsSelected = true;
            }
        }
    }
    /* @} */

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "onPause");
        if (mVolume != null) {
            mVolume.stopAllVolume();
            if (mVolume.getDialog() != null) {
                Log.d(TAG, "mVolume.getDialog() != null");
                mVolume.getDialog().dismiss();
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        /* SPRD: bug 462545, intentfuzz-activity,Settings Crash @{ */
        try {
            unregisterReceiver(mReceiver);
        } catch (Exception e) {
            Log.d(TAG, "unregisterReceiver error");
        }
        /* @} */
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        if (preference == mRenameProfile) {
            showRenameDialog(mAudioProfile.mDisplayName);
        }
        /* SPRD: Modified for bug 510013 @{ */
        /*else if (preference == mChargingSounds) {
            Log.i(TAG, "mChargingSounds " + "is checked " + mChargingSounds.isChecked());
            if (mIsSelected) {
                Settings.Global.putInt(getContentResolver(), Settings.Global.CHARGING_SOUNDS_ENABLED,
                        mChargingSounds.isChecked() ? 1 : 0);
            }
            // update db
            ContentValues values = new ContentValues();
            values.put(AudioProfileColumns.CHARGING_SOUNDS,
                    mChargingSounds.isChecked() ? AudioProfile.IS_CHARGING_SOUNDS : AudioProfile.NOT_CHARGING_SOUNDS);
            mAudioProfile.update(this, values);
        } */
        /* @} */
           else if (preference == mDtmfTone) {
            Log.i(TAG, "mDtmfTone " + "is checked " + mDtmfTone.isChecked());
            if (mIsSelected) {
                Settings.System.putInt(getContentResolver(),
                        Settings.System.DTMF_TONE_WHEN_DIALING, mDtmfTone.isChecked() ? 1 : 0);
            }
            // update db
            ContentValues values = new ContentValues();
            values.put(AudioProfileColumns.DTMF_TONE,
                    mDtmfTone.isChecked() ? AudioProfile.IS_DTMF_TONE : AudioProfile.NOT_DTMF_TONE);
            mAudioProfile.update(this, values);
        } else if (preference == mSoundEffects) {
            Log.i(TAG, "mSoundEffects " + "is checked " + mSoundEffects.isChecked());
            if (mIsSelected) {
                Settings.System.putInt(getContentResolver(),
                        Settings.System.SOUND_EFFECTS_ENABLED, mSoundEffects.isChecked() ? 1 : 0);
                if (mSoundEffects.isChecked()) {
                    mAudioManager.loadSoundEffects();
                } else {
                    mAudioManager.unloadSoundEffects();
                }
            }
            // update db
            ContentValues values = new ContentValues();
            values.put(AudioProfileColumns.SOUND_EFFECTS, mSoundEffects
                    .isChecked() ? AudioProfile.IS_SOUND_EFFECTS : AudioProfile.NOT_SOUND_EFFECTS);
            mAudioProfile.update(this, values);
        } else if (preference == mHapticFeedback) {
            Log.i(TAG, "mHapticFeedback " + "is checked " + mHapticFeedback.isChecked());
            if (mIsSelected) {
                Settings.System.putInt(getContentResolver(),
                        Settings.System.HAPTIC_FEEDBACK_ENABLED, mHapticFeedback.isChecked() ? 1
                                : 0);
            }
            // update db
            ContentValues values = new ContentValues();
            values.put(AudioProfileColumns.HAPTIC_FEEDBACK, mHapticFeedback
                    .isChecked() ? AudioProfile.IS_HAPTIC_FEEDBACK
                    : AudioProfile.NOT_HAPTIC_FEEDBACK);
            mAudioProfile.update(this, values);
        } else if (preference == mLockSounds) {
            Log.i(TAG, "mLockSounds " + "is checked " + mLockSounds.isChecked());
            if (mIsSelected) {
                Settings.System.putInt(getContentResolver(),
                        Settings.System.LOCKSCREEN_SOUNDS_ENABLED, mLockSounds.isChecked() ? 1 : 0);
            }
            // update db
            ContentValues values = new ContentValues();
            values.put(AudioProfileColumns.LOCK_SOUNDS, mLockSounds
                    .isChecked() ? AudioProfile.IS_LOCK_SOUNDS : AudioProfile.NOT_LOCK_SOUNDS);
            mAudioProfile.update(this, values);
        } else if (preference == mVibrate) {
            Log.i(TAG, "mVibrate " + "is checked " + mVibrate.isChecked());
            if (mIsSelected) {
                if (mVibrate.isChecked()) {
                    Settings.System.putInt(getContentResolver(),
                            Settings.System.VIBRATE_WHEN_RINGING, 1);
                    mAudioManager.setVibrateSetting(
                            AudioManager.VIBRATE_TYPE_RINGER, AudioManager.VIBRATE_SETTING_ON);
                    mAudioManager
                            .setVibrateSetting(
                                    AudioManager.VIBRATE_TYPE_NOTIFICATION,
                                    AudioManager.VIBRATE_SETTING_ON);
                } else {
                    Settings.System.putInt(getContentResolver(),
                            Settings.System.VIBRATE_WHEN_RINGING, 0);
                    mAudioManager.setVibrateSetting(
                            AudioManager.VIBRATE_TYPE_RINGER, AudioManager.VIBRATE_SETTING_OFF);
                    mAudioManager.setVibrateSetting(
                            AudioManager.VIBRATE_TYPE_NOTIFICATION,
                            AudioManager.VIBRATE_SETTING_OFF);
                }
            }
            // update db
            mAudioProfile.mIsVibrate = mVibrate.isChecked() ? AudioProfile.IS_VIBRATE
                    : AudioProfile.NOT_VIBRATE;
            ContentValues cv = new ContentValues();
            cv.put(AudioProfileColumns.IS_VIBRATE, mAudioProfile.mIsVibrate);
            mAudioProfile.update(this, cv);
            if (mAudioManager.getRingerMode() == AudioManager.RINGER_MODE_NORMAL
                    && Settings.System.getInt(mResolver, "currentAudioProfileId", -1) == AudioProfile.DEFAULT_GENERAL_PROFILE_ID) {
                Settings.System.putInt(mResolver, "generalAudioProfileVibrate",
                        mAudioProfile.mIsVibrate);
            }
        } else if (preference == mVibrateMsg) {
            Log.i(TAG, "mVibrateMsg " + "is checked " + mVibrateMsg.isChecked());
            if (mIsSelected) {
                if (mVibrateMsg.isChecked()) {
                    Settings.System.putInt(getContentResolver(), "vibrate_when_message", 1);
                } else {
                    Settings.System.putInt(getContentResolver(), "vibrate_when_message", 0);
                }
            }
            // update db
            mAudioProfile.mIsVibrateMsg = mVibrateMsg.isChecked() ? AudioProfile.IS_VIBRATE_MSG
                    : AudioProfile.NOT_VIBRATE_MSG;
            ContentValues cv = new ContentValues();
            cv.put(AudioProfileColumns.IS_VIBRATE_MSG, mAudioProfile.mIsVibrateMsg);
            mAudioProfile.update(this, cv);
        }
        return true;
    }

    private void updateRingtoneUri(int type, int index, Context mContext) {
        Log.d(TAG, "updateRingtoneUri", new Throwable());
        ContentValues values = new ContentValues();
        switch (type) {
            case RingtoneManager.TYPE_RINGTONE:
                if (AudioProfileProvider.ringtoneStr[index] != null) {
                    values.put(AudioProfileProvider.ringtoneStr[index],
                            mDefaultRingtoneUri == null ? null : mDefaultRingtoneUri.toString());
                    if (mIsSelected) {
                        Log.d(TAG, "updateRingtoneUri----Ringtone----setActualDefaultRingtoneUri");
                        Log.d(TAG, "updateRingtoneUri----Ringtone----mDefaultRingtoneUri = "
                                + mDefaultRingtoneUri);
                        RingtoneManagerEx.setActualDefaultRingtoneUri(mContext,
                                RingtoneManager.TYPE_RINGTONE, mDefaultRingtoneUri, index);
                    }
                }
                break;
            case RingtoneManager.TYPE_NOTIFICATION:
                values.put(AudioProfileColumns.NOTIFICATION_URI,
                        mDefaultNotificationUri == null ? null : mDefaultNotificationUri.toString());
                if (mIsSelected) {
                    Log.d(TAG, "updateRingtoneUri----Notification----setActualDefaultRingtoneUri");
                    Log.d(TAG, "updateRingtoneUri----Notification----mDefaultNotificationUri = "
                            + mDefaultNotificationUri);
                    RingtoneManager.setActualDefaultRingtoneUri(mContext,
                            RingtoneManager.TYPE_NOTIFICATION, mDefaultNotificationUri);
                }
                break;
            case RINGTONETYPE_MESSAGE:
                if (AudioProfileProvider.mMessagetoneStr[index] != null) {
                    values.put(AudioProfileProvider.mMessagetoneStr[index],
                            mDefaultNotificationUri == null ? null : mDefaultNotificationUri.toString());
                    if (mIsSelected) {
                        Settings.System.putString(getContentResolver(), "messagetone" + index
                                , mDefaultNotificationUri == null ? null : mDefaultNotificationUri.toString());
                    }
                }
                break;
        }
        mAudioProfile.update(mContext, values);
    }

    private void showRenameDialog(final String originName) {
        final Toast mtoast = Toast.makeText(AudioProfileSoundSettings.this,
                AudioProfileSoundSettings.this.getText(R.string.input_limit), Toast.LENGTH_SHORT);
        AlertDialog.Builder builder = null;
        final EditText renameEt = new EditText(this);
        // SPRD: Bug 426128 The dialog was obscured when the line too much
        renameEt.setMaxLines(5);

        renameEt.setText(originName);
        renameEt.setSelection(originName.length());

        renameEt.addTextChangedListener(new TextWatcher() {

            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            public void onTextChanged(CharSequence s, int start, int before, int count) {
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
        //SPRD: Added for bug 601568, adjust the UI
        String mMsg = "\n" + mContext.getText(R.string.rename_profile_msg);
        builder = new AlertDialog.Builder(this);
        builder.setTitle(R.string.rename_profile);
        //SPRD: Modified for bug 601568, adjust the UI
        builder.setMessage(mMsg);
        builder.setView(renameEt, 5, 0, 5, 0);
        builder.setPositiveButton(R.string.dlg_ok,
                new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        // SPRD :Bug 435081 Rename the audioprofilename with blank spaces.
                        String text = renameEt.getText().toString().trim();
                        if ("".equals(text) || text == null) {
                            showNameCannotBeNullDialog();
                            renameEt.setText("");
                            return;
                        }
                        else if (originName.equals(text)) {
                            // do nothing
                        }
                        else if (existDuplicateDisplayName(text)) {
                            showNameAlreadyExistDialog();
                            renameEt.setText("");
                            return;
                        } else {
                            mAudioProfile
                                    .updateDisplayName(mContext, renameEt.getText().toString());
                        }
                        renameEt.setText("");
                    }
                });
        builder.setNegativeButton(R.string.dlg_cancel,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                    }
                });
        builder.show();
    }

    private void showNameCannotBeNullDialog() {
        AlertDialog.Builder builder = null;
        builder = new AlertDialog.Builder(this);
        builder.setIcon(android.R.drawable.ic_dialog_alert);
        builder.setTitle(R.string.empty_name_dialog);
        builder.setPositiveButton(R.string.dlg_ok,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        showRenameDialog(mAudioProfile.mDisplayName);
                    }
                });
        builder.show();
    }

    private void showNameAlreadyExistDialog() {
        AlertDialog.Builder builder = null;
        builder = new AlertDialog.Builder(this);
        builder.setIcon(android.R.drawable.ic_dialog_alert);
        builder.setTitle(R.string.duplicate_display_name_dialog);
        builder.setPositiveButton(R.string.dlg_ok,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        showRenameDialog(mAudioProfile.mDisplayName);
                    }
                });
        builder.show();
    }

    private boolean existDuplicateDisplayName(String name) {
        boolean result = false;
        Cursor cursor = managedQuery(AudioProfile.CONTENT_URI, new String[] {
                AudioProfileColumns.ID, AudioProfileColumns.DISPLAY_NAME
        },
                AudioProfileColumns.DISPLAY_NAME + "=? AND " + AudioProfileColumns.ID + ">?",
                new String[] {
                        name, String.valueOf(AudioProfile.PREDEFINED_PROFILE_COUNT)
                }, null);
        if (cursor != null && cursor.getCount() > 0) {
            result = true;
        }
        return result;
    }

    /* SPRD: Bug 608515  @{ */
    public boolean isGMSVersion(){
        boolean ret = false;
        if (!SystemProperties.get("ro.com.google.gmsversion").isEmpty()) {
            ret = true;
        }
        return ret;
    }
    /* @} */

}
