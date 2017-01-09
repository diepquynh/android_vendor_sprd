//Create by Spreadst
package com.sprd.cellbroadcastreceiver.ui;

import java.util.ArrayList;

import android.database.sqlite.SQLiteDatabase;
import android.app.Activity;
import android.app.ActionBar;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.DialogInterface.OnClickListener;
import android.content.ContentValues;
import android.content.res.Configuration;
import android.database.Cursor;
import android.media.AudioAttributes;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.CheckBox;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.Button;
import android.widget.EditText;
import android.text.Selection;
import android.text.Spannable;
import android.text.method.DialerKeyListener;

//import com.android.cellbroadcastreceiver.CellBroadcastDatabaseHelper;
import com.sprd.cellbroadcastreceiver.R;
import com.sprd.cellbroadcastreceiver.data.ChannelItemData;
import com.sprd.cellbroadcastreceiver.data.ChannelMgr;
import com.sprd.cellbroadcastreceiver.data.itf.IModify;
import com.sprd.cellbroadcastreceiver.data.itf.ISoundSetting;
import com.sprd.cellbroadcastreceiver.data.itf.SoundSettingImpl;
import com.sprd.cellbroadcastreceiver.provider.ChannelTableDefine;
import com.sprd.cellbroadcastreceiver.util.Utils;

/**
 * Displays a list of the Cell Broadcast Sms.
 */
public class ChannelSettingEditActivity extends Activity {
    private static final String TAG = "ChannelSettingActivity";

    private static final int MENU_DELETE        = 0;
    private static final int REQUEST_RINGTONE   = 1;

    private EditText mNameField;
    private EditText mNumberField;
    private EditText mSetRingtone;
    private TextView mLableRingtone;
    private Button mButton;
    private String mName = null;
    private String mChannelID = null;

    private CheckBox mSaveCheckbox;
    private CheckBox mCheckbox;
    private CheckBox mNotificateCheckbox;
    private CheckBox mVibrateCheckbox;

    private int mSave                           = 1;
    private int mEnable                         = 1;
    private int mId                             = -1;
    private int mNotificate                     = 1;
    private int mVibrate                        = 1;

    private int mSubId;
    private int mChannelIdBeforeEdit            = -1;
    private int mIndexOfArray                   = -1;

    private int mOperation                      = 0; // Add|Edit|null
    private boolean DEBUG                       = true;

    private ISoundSetting mISoundSetting;

    private int getSaved() {
        return mSave;
    }

    private int getEnabled() {
        return mEnable;
    }

    private String getChannelID() {
        return mChannelID;
    }

    private int getId() {
        return mId = -1;
    }

    private int getSubId() {
        return mSubId;
    }

    private int getChannelBeforeEdit() {
        return mChannelIdBeforeEdit;
    }

    private int getIndexOfArray() {
        return mIndexOfArray;
    }

    private int getNotificate() {
        return mNotificate;
    }

    private int getVibrate() {
        return mVibrate;
    }

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        if (!resolveIntent()) {
            setResult(ChannelSettingActivity.REQUEST_FINISH_CHANNEL,
                    getIntent());
            finish();
            return;
        }
        setContentView(R.layout.cell_broadcast_setting_edit);
        /* SPRD:Bug#265252 @{ */
        getActionBar().setDisplayUseLogoEnabled(false);
        getActionBar().setDisplayShowHomeEnabled(false);
        getActionBar().setDisplayOptions(
                ActionBar.DISPLAY_SHOW_TITLE | ActionBar.DISPLAY_HOME_AS_UP);
        /* @} */

        setupView();
        setTitle(R.string.edit);
    }

    private boolean resolveIntent() {
        Intent intent = getIntent();
        mOperation = intent.getIntExtra(Utils.OPERATION, -1);
        if (Utils.OPERATION_EDIT == mOperation) {
            // add for bug 577872 begin
            if (ChannelMgr.getInstance().size() == 0) {
                return false;
            }
            // add for bug 577872 end
            if (intent.getIntExtra(Utils.INDEXOFARRAY, -1) == -1) {
                log("Unexpected index of the ChannelMgr");
                // add for bug 577872 begin
                return true;
                // add for bug 577872 end
            } else {
                mIndexOfArray = intent.getIntExtra(Utils.INDEXOFARRAY, -1);
            }
            log("the index data is:"+ mIndexOfArray+ " and the itemdata is:"
                            + ChannelMgr.getInstance().get(getIndexOfArray())
                                    .toString());
            mName = ChannelMgr.getInstance().get(getIndexOfArray())
                    .getChannelName();
            mChannelID = String.valueOf(ChannelMgr.getInstance()
                    .get(getIndexOfArray()).getChannelId());
            mEnable = ChannelMgr.getInstance().get(getIndexOfArray())
                    .getEnabled();
            mChannelIdBeforeEdit = Integer.parseInt(mChannelID);
            mSave = ChannelMgr.getInstance().get(getIndexOfArray()).getSave();
            mISoundSetting = ChannelMgr.getInstance().get(getIndexOfArray()).getSoundSetting();

            try {
                mId = ChannelMgr.getInstance().get(getIndexOfArray()).getID();
            } catch (Exception e) {
                e.printStackTrace();
            }
        } else {
            mISoundSetting = new SoundSettingImpl();
        }
        log("the mISoundSetting is:"+(mISoundSetting == null));
        mSubId = intent.getIntExtra(ChannelTableDefine.SUB_ID, -1);
        if (DEBUG) {
            log("resolveIntent:Operation is " + mOperation
                    + "! And the subId is:" + getSubId());
        }
        // add for bug 577872 begin
        return true;
        // add for bug 577872 begin
    }

    private View.OnClickListener mClicked = new View.OnClickListener() {
        public void onClick(View v) {
            if (v == mNameField) {
                mNameField.requestFocus();
            } else if (v == mNumberField) {
                mNumberField.requestFocus();
            }else if (v == mVibrateCheckbox) {
                if (mVibrateCheckbox.isChecked()) {
                    mISoundSetting.setVibrate(true);
                } else {
                    mISoundSetting.setVibrate(false);
                }
            } else if (v == mNotificateCheckbox) {
                if (mNotificateCheckbox.isChecked()) {
                    mSetRingtone.setEnabled(true);
                    mLableRingtone.setEnabled(true);
                    mVibrateCheckbox.setEnabled(true);
                    mISoundSetting.setNotification(true);
                } else {
                    mSetRingtone.setEnabled(false);
                    mLableRingtone.setEnabled(false);
                    mVibrateCheckbox.setEnabled(false);
                    mISoundSetting.setNotification(false);
                }
            } else if (v == mSetRingtone) {
                // Launch the ringtone picker
                Intent intent = new Intent(RingtoneManager.ACTION_RINGTONE_PICKER);
                onPrepareRingtonePickerIntent(intent);
                startActivityForResult(intent, REQUEST_RINGTONE);
            } else if (v == mButton) {
                // save this channel
                String regexNum = "^[0-9]*$";
                String channel_num = mNumberField.getText().toString().trim();

                mName = mNameField.getText().toString();
                mChannelID = mNumberField.getText().toString().trim();
                mEnable = mCheckbox.isChecked() ? 1 : 0;
                mSave = mSaveCheckbox.isChecked() ? 1 : 0;
                log("the channel_name is:"+ mName+" and channel_id is:"+mChannelID);
                if (checkNameNotEmpty(mName) && checkChannelId(channel_num, regexNum)) {
                    processData();
                }
            }
        }
    };

    private boolean checkNameNotEmpty(String name){
        if (name == null || name.length() <= 0) {
            Toast.makeText(getApplicationContext(),
                    getString(R.string.input_channel_name),
                    Toast.LENGTH_SHORT).show();
            mNameField.requestFocus();
            return false;
        }
        return true;
    }

    // check the channel_id is valid
    private boolean checkChannelId(String channel_num, String regexNum) {
        if (channel_num == null || channel_num.length() <= 0) {
            Toast.makeText(getApplicationContext(),
                    getString(R.string.info_input_channel_number),
                    Toast.LENGTH_SHORT).show();
            return false;
        } else if (!channel_num.matches(regexNum)) {
            Toast.makeText(getApplicationContext(),
                    getString(R.string.info_input_channel_number_num),
                    Toast.LENGTH_SHORT).show();
            return false;
        } else if (Integer.parseInt(channel_num) > ChannelSettingActivity.PADDING) {
            Toast.makeText(getApplicationContext(),
                    getString(R.string.toast_input_channel_number_exceed),
                    Toast.LENGTH_SHORT).show();
            return false;
        } else if (getChannelBeforeEdit() != Integer.parseInt(getChannelID())
                && isChannelMatched(getChannelID(), getEnabled(), getSubId())) {
            if (DEBUG) {
                log("Save channcel: the channcel is already existed!");
            }
            Toast.makeText(ChannelSettingEditActivity.this,
                    R.string.same_setting, Toast.LENGTH_LONG).show();
            return false;
        }
        return true;
    }

    private void processData(){
        // save channcel info into datatbase
        if (Utils.OPERATION_ADD == (mOperation)) {// save add
            log("--OnClick--ADD action. And the sub_id is:"
                    + getSubId());
            ChannelMgr.getInstance().addNewData(getSubId(),
                    Integer.parseInt(getChannelID()), mName,
                    getEnabled() == 1, getSaved() == 1, mISoundSetting);
        } else if (Utils.OPERATION_EDIT == (mOperation)) {// save edit
            if (Integer.parseInt(getChannelID()) != getChannelBeforeEdit()) {// channel_id changed
                // set the old channel_id Flag to be DELETE
                log("--OnClick--EDIT action and channel_id changed.");
                ChannelMgr.getInstance().deleteDataByIndex(
                        getIndexOfArray());
                // add a new item
                ChannelMgr.getInstance().addNewData(getSubId(),
                        Integer.parseInt(getChannelID()), mName,
                        getEnabled() == 1, getSaved() == 1, mISoundSetting);
            } else {// channel_id is not changed, only the status changes
                log("--OnClick--EDIT action and channel_id is not changed.");
                ChannelMgr.getInstance().updateDataByIndex(
                        getIndexOfArray(),
                        Integer.parseInt(getChannelID()), mName,
                        getEnabled() == 1, getSaved() == 1, mISoundSetting);
            }
        } else {
            log("Save channel: don't save unknow operation!");
        }
        setResult(ChannelSettingActivity.RESULT_CHANGE_CHANNEL,
                getIntent());
        finish();
    }

    View.OnFocusChangeListener mOnFocusChangeHandler = new View.OnFocusChangeListener() {
        public void onFocusChange(View v, boolean hasFocus) {
            if (hasFocus) {
                TextView textView = (TextView) v;
                Selection.selectAll((Spannable) textView.getText());
            }
        }
    };

    /**
     * We have multiple layouts, one to indicate that the user needs to open the
     * keyboard to enter information (if the keybord is hidden). So, we need to
     * make sure that the layout here matches that in the layout file.
     */
    private void setupView() {
        mNameField = (EditText) findViewById(R.id.channel_name_edit);
        if (mNameField != null) {
            mNameField.setOnFocusChangeListener(mOnFocusChangeHandler);
            mNameField.setOnClickListener(mClicked);
            mNameField.setText(mName);
        }
        mNumberField = (EditText) findViewById(R.id.channel_number_edit);
        if (mNumberField != null) {
            mNumberField.setKeyListener(DialerKeyListener.getInstance());
            mNumberField.setOnFocusChangeListener(mOnFocusChangeHandler);
            mNumberField.setOnClickListener(mClicked);
            mNumberField.setText(getChannelID());
        }
        mSetRingtone = (EditText) findViewById(R.id.set_ringtone);
        if (mSetRingtone != null) {
            mSetRingtone.setOnClickListener(mClicked);
            mSetRingtone.setText(getRingtoneTitle(mISoundSetting.getSoundURI()));
        }

        mButton = (Button) findViewById(R.id.button);
        if (mButton != null) {
            mButton.setOnClickListener(mClicked);
        }

        mCheckbox = (CheckBox) findViewById(R.id.enable_checkbox);
        mSaveCheckbox = (CheckBox) findViewById(R.id.save_checkbox);
        mNotificateCheckbox = (CheckBox) findViewById(R.id.notification_checkbox);
        mVibrateCheckbox = (CheckBox) findViewById(R.id.vibrate_checkbox);
        mLableRingtone = (TextView) findViewById(R.id.set_ringtone_label);

        if (mISoundSetting.isNotification()) {
            mSetRingtone.setEnabled(true);
            mLableRingtone.setEnabled(true);
            mVibrateCheckbox.setEnabled(true);
        } else {
            mSetRingtone.setEnabled(false);
            mLableRingtone.setEnabled(false);
            mVibrateCheckbox.setEnabled(false);
        }
        checkIfNeedVisible();

        mCheckbox.setOnClickListener(mClicked);
        mCheckbox.setChecked((getEnabled() == 1));
        mSaveCheckbox.setOnClickListener(mClicked);
        mSaveCheckbox.setChecked((getSaved() == 1));

        mNotificateCheckbox.setOnClickListener(mClicked);
        mNotificateCheckbox.setChecked(mISoundSetting.isNotification());
        mVibrateCheckbox.setOnClickListener(mClicked);
        mVibrateCheckbox.setChecked(mISoundSetting.isVibrate());
    }

    private void checkIfNeedVisible() {
        if (!Utils.DEPEND_ON_SLOT) {
            mLableRingtone.setVisibility(View.VISIBLE);
            mSetRingtone.setVisibility(View.VISIBLE);
            mNotificateCheckbox.setVisibility(View.VISIBLE);
            mVibrateCheckbox.setVisibility(View.GONE);
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig){
        super.onConfigurationChanged(newConfig);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        if (DEBUG)
            log("onCreateOptionsMenu");
        if (mId != -1) {
            menu.add(0, MENU_DELETE, Menu.NONE,
                    R.string.menu_delete_sigle_channel)
                    .setIcon(android.R.drawable.ic_menu_delete)
                    .setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (DEBUG)
            log("onOptionsItemSelectedSIM MenuItem");
        switch (item.getItemId()) {
        case MENU_DELETE: {
            if (DEBUG)
                log("onOptionsItemSelectedSIM MENU_DELETE");
            //modify for bug 531888
//            AlertDialog.Builder builder = new AlertDialog.Builder(this);
//            builder.setTitle(R.string.menu_delete_sigle_channel);
//            builder.setIconAttribute(android.R.attr.alertDialogIcon);
//            builder.setCancelable(true);
//            builder.setMessage(R.string.confirm_delete_message);
//            builder.setPositiveButton(android.R.string.ok, deleteListener);
//            builder.setNegativeButton(R.string.no, null);
//            builder.show();
            ChannelMgr.getInstance().deleteDataByIndex(getIndexOfArray());

            setResult(ChannelSettingActivity.RESULT_DELETE_CHANNEL, getIntent());
            ChannelSettingEditActivity.this.finish();
            return true;
        }
        /* SPRD:Bug#265252 @{ */
        case android.R.id.home:
            finish();
            break;
        /* @} */
        default:
            break;
        }
        return false;
    }

    private OnClickListener deleteListener = new OnClickListener() {
        public void onClick(DialogInterface dialog, int which) {
            // mIndexOfArray must not be -1 so that option_menu delete shows
            ChannelMgr.getInstance().deleteDataByIndex(getIndexOfArray());

            setResult(ChannelSettingActivity.RESULT_DELETE_CHANNEL, getIntent());
            ChannelSettingEditActivity.this.finish();
        }
    };

    /**
     * If there exists a channel setting of the same channelId we think the
     * channel matched.
     * 
     * check the ListAdapter
     * 
     * @param channelId
     * @param enabled
     * @param phoneId
     * @return true If there is a matched channel.
     */
    private boolean isChannelMatched(String channelId, int enabled, int subId) {
        boolean isMatched = false;

        ArrayList<ChannelItemData> list = ChannelMgr.getInstance()
                .filterDelRecord();

        int size = list.size();
        for (int i = 0; i < size; i++) {
            ChannelItemData itemData = list.get(i);
            if ((itemData.getSubId() == subId)
                    && (itemData.getChannelId() == Integer.parseInt(channelId))) {
                isMatched = true;
                return isMatched;
            }
        }
        return isMatched;
    }

    private void onPrepareRingtonePickerIntent(Intent ringtonePickerIntent){
        ringtonePickerIntent.putExtra(RingtoneManager.EXTRA_RINGTONE_EXISTING_URI,
                getRingtoneUri());
        ringtonePickerIntent.putExtra(RingtoneManager.EXTRA_RINGTONE_SHOW_DEFAULT, true);
        ringtonePickerIntent.putExtra(RingtoneManager.EXTRA_RINGTONE_DEFAULT_URI,
                RingtoneManager.getDefaultUri(RingtoneManager.TYPE_RINGTONE));

        ringtonePickerIntent.putExtra(RingtoneManager.EXTRA_RINGTONE_SHOW_SILENT, true);
        //RingtoneManager.TYPE_RINGTONE=1 sounds that are used for the phone ringer
        //RingtoneManager.TYPE_NOTIFICATION = 2  sounds that are used for notifications
        ringtonePickerIntent.putExtra(RingtoneManager.EXTRA_RINGTONE_TYPE, RingtoneManager.TYPE_ALL);
        ringtonePickerIntent.putExtra(RingtoneManager.EXTRA_RINGTONE_TITLE, getRingtoneTitle());
        ringtonePickerIntent.putExtra(RingtoneManager.EXTRA_RINGTONE_AUDIO_ATTRIBUTES_FLAGS,
                AudioAttributes.FLAG_BYPASS_INTERRUPTION_POLICY);
    }

    /*Modify By SPRD for Bug:586050 Start*/
    private static Uri OLD_URI = null;
    private Uri getRingtoneUri() {
        return OLD_URI;
    }
    /*Modify By SPRD for Bug:586050 End*/

    private String getRingtoneTitle(){
        String mRingtoneString = mSetRingtone.getText().toString().trim();
        if (mRingtoneString == null || mRingtoneString.equals("")) {
            return null;
        }
        return mRingtoneString.split(":")[0];
    }

    private String getRingtoneTitle(String ringtoneUri){
        //modify for bug 545650 begin
        if (ringtoneUri != null) {
            if (Settings.AUTHORITY.equals(Uri.parse(ringtoneUri).getAuthority())) {
                final Ringtone tone = RingtoneManager.getRingtone(getApplicationContext(),
                        Uri.parse(ringtoneUri));
                if (tone != null) {
                    return tone.getTitle(this);
                } else {
                    return getString(R.string.silence);
                }
            } else {
                //add for bug 605579 begin
                OLD_URI = Utils.getRingtoneUri(this, Uri.parse(ringtoneUri));
                mISoundSetting.setSoundURI(OLD_URI.toString());
                return Utils.getRingtoneTitle(this, OLD_URI);
                //add for bug 605579 end
            }
        } else {
            return getString(R.string.silence);
        }
        //modify for bug 545650 end
    }

    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        String ringtoneName = null;

        if (requestCode == REQUEST_RINGTONE) {
            
            if (data != null) {
                Uri uri = data.getParcelableExtra(RingtoneManager.EXTRA_RINGTONE_PICKED_URI);

                if (uri != null) {
                    OLD_URI = uri;//Add for bug:586050
                    mISoundSetting.setSoundURI(uri.toString());
                    final Ringtone tone = RingtoneManager.getRingtone(this, uri);

                    if (tone != null) {
                        ringtoneName = tone.getTitle(this);
                    }
                } else {
                    ringtoneName = getString(R.string.silence);
                    mISoundSetting.setSoundURI(null);
                }
                log("uri return is:"+uri+" and ringtone name is:"+ringtoneName);
                mSetRingtone.setText(ringtoneName);
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    private void log(String string){
        Log.d(TAG, string);
    }
}
