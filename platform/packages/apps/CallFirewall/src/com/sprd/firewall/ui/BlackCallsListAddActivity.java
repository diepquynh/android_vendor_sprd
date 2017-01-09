
package com.sprd.firewall.ui;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ActionBar;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.database.sqlite.SQLiteDiskIOException;
import android.database.sqlite.SQLiteFullException;
import android.os.Bundle;
import android.os.SystemProperties;
import android.provider.Contacts;
import android.provider.Contacts.Intents.UI;
import android.provider.ContactsContract.CommonDataKinds;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.provider.ContactsContract.Data;
import android.telephony.PhoneNumberUtils;
import android.text.TextUtils;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.firewall.PhoneUtils;
import com.sprd.firewall.R;
import com.sprd.firewall.model.BlackNumberEntity;
import com.sprd.firewall.ui.BlackCallsListAddActivity.BlackNumberStatus;
import com.sprd.firewall.ui.BlackCallsListAddActivity.PendingBlackListOption;
import com.sprd.firewall.ui.view.PhoneEditText;
import com.sprd.firewall.util.BlackCallsUtils;

import android.content.ComponentName;
import android.provider.Telephony.Mms;

public class BlackCallsListAddActivity extends Activity {
    private static final String TAG = "BlackCallsListAddActivity";

    private static final int REQUESET_CODE_SELECT_CONTACTS = 4;

    private static final String NUM_PROJECTION[] = {
            CommonDataKinds.Phone.NUMBER, Data.DISPLAY_NAME
    };

    private PhoneEditText add_call_number_edit;
    private PhoneEditText add_call_name_edit;

    private ImageButton selectContact;

    private ListView mListView;

    private TypeAdapter mTypeAdapter;

    private int InterruptType = 0;
    private int OldInterruptType = 0;

    private Map<Integer, Boolean> selectedMap;

    private CheckBox mCheckBox;

    private Button mSave;

    private Button mCancel;

    private String PhoneNumber = "";
    private String PhoneName = "";

    private static final String SAVE_INTERRUPT_TYPR = "interruptType";

    private final int MAX_LENGTH = 256;

    private List<String> listArray;

    boolean isEditPage = false;
    // SPRD: Add for bug550014
    private boolean mIsSaveInstanceState = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.black_calls_add_list_item_layout);

        findViews();

        getActionBar().setDisplayShowTitleEnabled(true);
        getActionBar().setDisplayShowHomeEnabled(true);
        getActionBar().setDisplayHomeAsUpEnabled(true);

        getActionBar().setTitle(R.string.add_contacts);
        getActionBar().setDisplayOptions(
                ActionBar.DISPLAY_HOME_AS_UP | ActionBar.DISPLAY_USE_LOGO
                        | ActionBar.DISPLAY_SHOW_TITLE);

        Bundle bundle = this.getIntent().getExtras();
        if (bundle != null) {
            PhoneNumber = PhoneUtils.filterNumbers(bundle.getString("Click_BlackCalls_Number"));
            add_call_number_edit.setText(PhoneNumber);
            if (!TextUtils.isEmpty(PhoneNumber)) {
                if (PhoneNumber.length() < MAX_LENGTH) {
                    add_call_number_edit.setSelection(PhoneNumber.length());
                } else {
                    add_call_number_edit.setSelection(MAX_LENGTH);
                }
            }

            PhoneName = bundle.getString("Click_BlackCalls_Name");
            Log.d(TAG, "Phonename" + PhoneName);
            /* SPRD: add for bug493583 @{ */
            if (TextUtils.isEmpty(PhoneName)) {
                add_call_name_edit.setText(PhoneNumber);
                if (PhoneNumber != null) {
                    if (PhoneNumber.length() < MAX_LENGTH) {
                        add_call_name_edit.setSelection(PhoneNumber.length());
                    } else {
                        add_call_name_edit.setSelection(MAX_LENGTH);
                    }
                }
            } else {
                add_call_name_edit.setText(PhoneName);
                if (PhoneName.length() < MAX_LENGTH) {
                    add_call_name_edit.setSelection(PhoneName.length());
                } else {
                    add_call_name_edit.setSelection(MAX_LENGTH);
                }
            }
            /* @} */
            InterruptType = bundle.getInt("Click_BlackCalls_Type", 0);
            OldInterruptType = InterruptType;
            Log.v(TAG, "InterruptType.getInt()=" + InterruptType);

            isEditPage = bundle.getBoolean("Click_BlackCalls_Edit", false);
            Log.d(TAG, "isEditPage = " + isEditPage);
            if (!isEditPage) {
                getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);
            }
        }
        if (savedInstanceState != null) {
            InterruptType = savedInstanceState.getInt(SAVE_INTERRUPT_TYPR);
        }
        listArray = new ArrayList<String>();
        String[] stringArray = getResources().getStringArray(R.array.BlackList_list_stringArray);
        // Multiple languages, appear crash for fix bug490647
        int length = (stringArray.length > 2 ? stringArray.length - 1 : stringArray.length);
        for (int i = 0; i < length; i++) {
            listArray.add(stringArray[i]);
        }

        mTypeAdapter = new TypeAdapter(this);
        mListView.setAdapter(mTypeAdapter);

        mSave.setOnClickListener(saveListener);
        mCancel.setOnClickListener(cancelListener);

        mListView.setOnItemClickListener(mOnItemClickListener);
        selectContact.setOnClickListener(selectContactListener);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        // SPRD: Add for bug550014
        mIsSaveInstanceState = true;
        super.onSaveInstanceState(outState);
        outState.putInt(SAVE_INTERRUPT_TYPR, InterruptType);
    }

    @Override
    public void onBackPressed() {
        /* SPRD: Add for bug550014 @{ */
        if (isResumed() && !mIsSaveInstanceState) {
            super.onBackPressed();
        }
        /* @} */
    }

    /* SPRD: Add for bug550014 @{ */
    @Override
    protected void onResume() {
        mIsSaveInstanceState = false;
        super.onResume();
    }
    /* @} */

    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home: {
                /* SPRD: add for bug496493 @{ */
                //onBackPressed();
                finish();
                /* @} */
                return true;
            }
        }
        return false;
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy");
        if (listArray != null) {
            listArray.clear();
        }
        super.onDestroy();
    }

    enum PendingBlackListOption {
        NONE,
        ADD,
        UPDATE,
        CONFLICT,
    }

    enum BlackNumberStatus {
        NONE,
        EXIST_NUMBER,
        EXIST_NUMBER_AND_TYPE,
    }

    private void findViews() {
        Log.d(TAG, "RequestType1=" + findViewById(R.id.black_calls_add_edit_label));
        Log.d(TAG, "RequestType2=" + findViewById(R.id.black_calls_add_edit_label_name));
        add_call_number_edit = (PhoneEditText) findViewById(R.id.black_calls_add_edit_label);
        add_call_name_edit = (PhoneEditText) findViewById(R.id.black_calls_add_edit_label_name);
        selectContact = (ImageButton) findViewById(R.id.select_contact);
        mListView = (ListView) findViewById(R.id.type_list);

        mSave = (Button) findViewById(R.id.balck_calls_add_Submit);
        mCancel = (Button) findViewById(R.id.balck_calls_add_Cancel);

    }

    private OnClickListener saveListener = new OnClickListener() {
        @Override
        public void onClick(View arg0) {
            String phonenumber = add_call_number_edit.getText().toString().trim();
            PhoneName = add_call_name_edit.getText().toString();
            int calltype = InterruptType;
            Log.v("TAG", "calltype=" + calltype);
            validateForm(phonenumber, calltype, PhoneName);
        }
    };

    private OnClickListener cancelListener = new OnClickListener() {
        @Override
        public void onClick(View arg0) {
            ReturnFrontPage(PhoneNumber, InterruptType);
        }
    };

    private OnClickListener selectContactListener = new OnClickListener() {
        @Override
        public void onClick(View arg0) {
            Log.d(TAG, "onClick selectContactListener!");
            Intent mContactListIntent = new Intent(Intent.ACTION_GET_CONTENT);
            mContactListIntent.setType(Phone.CONTENT_ITEM_TYPE);
            mContactListIntent.setComponent(new ComponentName("com.android.contacts",
                    "com.android.contacts.activities.ContactSelectionActivity"));
            /* SPRD: Add for bug 594938. @{ */
            try {
                BlackCallsListAddActivity.this.startActivityForResult(mContactListIntent,
                        REQUESET_CODE_SELECT_CONTACTS);
            } catch (ActivityNotFoundException e) {
                showToast(R.string.Contact_error);
                Log.e(TAG,"fail to select contacts due to=" + e);
            }
            /* @} */
        }
    };

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        Log.d(TAG, "requestCode=" + requestCode + ", resultCode=" + resultCode);
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK) {
            Log.d(TAG, "fail due to resultCode=" + resultCode);
            return;
        }

        Cursor cursor = null;
        try {
            cursor = getContentResolver().query(data.getData(), NUM_PROJECTION, null, null, null);
            if ((cursor == null) || (!cursor.moveToFirst())) {
                Log.d(TAG, "onActivityResult: bad contact data, no results found.");
                return;
            }

            switch (requestCode) {
                case 4:
                    String phoneNumber = cursor.getString(0);
                    String phoneName = cursor.getString(1);
                    Log.d(TAG, "select number is : " + phoneNumber);
                    Log.d(TAG, "select name is : " + phoneName);
                    if (add_call_number_edit != null) {
                        add_call_number_edit.setText(phoneNumber);
                    }
                    if (add_call_name_edit != null) {
                        add_call_name_edit.setText(phoneName);
                        if (phoneName != null) {
                            if (phoneName.length() < MAX_LENGTH) {
                                add_call_name_edit.setSelection(phoneName.length());
                            } else {
                                add_call_name_edit.setSelection(MAX_LENGTH);
                            }
                        }
                    }
                    break;
                default:
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
    }

    private final OnItemClickListener mOnItemClickListener = new OnItemClickListener() {

        @Override
        public void onItemClick(AdapterView<?> arg0, View arg1, int position, long id) {
            mCheckBox.setChecked(!selectedMap.get(position));
            selectedMap.put(position, mCheckBox.isChecked());

            if (true == selectedMap.get(position)) {
                InterruptType = InterruptType | 1 << position;
            } else {
                InterruptType = InterruptType & (~(1 << position));
            }
            mListView.setAdapter(mTypeAdapter);
            mTypeAdapter.notifyDataSetChanged();
            Log.v("TAG", "InterruptType=" + InterruptType);
        }

    };

    private void validateForm(String phone_num, int type, String phone_name) {
        if (!Mms.isPhoneNumber(phone_num)) {
            showToast(R.string.blackcalls_add_phone_tip_error_input);
            return;
        }
        if (!selectedMap.containsValue(true)) {
            showToast(R.string.blackcalls_add_type_tip_error);
            return;
        }
        BlackCallsUtils blackcallsUtils = new BlackCallsUtils(BlackCallsListAddActivity.this);
        try {
            switch (getPendingOption(phone_num, type, phone_name)) {
                case NONE:
                    PhoneNumber = phone_num;
                    ReturnFrontPage(phone_num, type);
                    return;
                case ADD:
                    Log.d(TAG, "PhoneName=" + PhoneName);
                    blackcallsUtils.AddBlackCalls(phone_num, type, PhoneName);
                    PhoneNumber = phone_num;
                    showToast(R.string.blackcalls_add_phone_tip_success);
                    ReturnFrontPage(phone_num, type);
                    return;
                case UPDATE:
                    Log.d(TAG, "phone_name=" + phone_name + "PhoneNumber=" + PhoneNumber
                            + "phone_num=" + phone_num);
                    if (blackcallsUtils.UpdateBlackNumber(PhoneNumber, phone_num, type, phone_name)) {
                        showToast(R.string.blackcalls_edit_phone_tip_success);
                    } else {
                        showToast(R.string.blackcalls_update_phone_tip_fail);
                    }

                    PhoneNumber = phone_num;
                    PhoneName = phone_name;
                    ReturnFrontPage(phone_num, type);
                    return;
                case CONFLICT:
                    showToast(R.string.blackcalls_add_phone_tip_error_existed_phone);
                    return;
            }
        } catch (SQLiteFullException e) {
            showToast(R.string.sqlite_full);
        } catch (SQLiteDiskIOException e) {
            showToast(R.string.sqlite_full);
        } catch (Exception e) {
            showToast(R.string.sqlite_full);
        }

    }

    private PendingBlackListOption getPendingOption(String phone_num, int type, String phone_name) {
        String oldNum = PhoneNumber;
        String newNum = phone_num;
        if (newNum == null || newNum.equals("")) {
            return PendingBlackListOption.NONE;
        }
        BlackCallsUtils blackcallsUtils = new BlackCallsUtils(BlackCallsListAddActivity.this);
        boolean isEdited = !PhoneNumberUtils.compareStrictly(oldNum.trim(), newNum.trim());
        BlackNumberStatus oldNumS = FindNum(blackcallsUtils, oldNum, type, phone_name, this);
        BlackNumberStatus newNumS = FindNum(blackcallsUtils, newNum, type, phone_name, this);
        Log.d(TAG, "isEdited=" + isEdited + " oldNumS=" + oldNumS + " newNumS=" + newNumS);
        boolean oldNumFind = (oldNumS != BlackNumberStatus.NONE);
        boolean newNumFind = (newNumS != BlackNumberStatus.NONE);
        PendingBlackListOption os = PendingBlackListOption.ADD;
        Log.d(TAG, "newNumFind=" + newNumFind + " oldNumFind=" + oldNumFind + " isEditPage="
                + isEditPage);
        if (isEdited) {
            if (newNumFind) {
                os = PendingBlackListOption.CONFLICT;
            } else if (oldNumFind) {
                os = PendingBlackListOption.UPDATE;
            } else {
                os = PendingBlackListOption.ADD;
            }
        } else {
            switch (newNumS) {
                case NONE:
                    os = PendingBlackListOption.ADD;
                    break;
                case EXIST_NUMBER:
                    if (isEditPage) {
                        os = PendingBlackListOption.UPDATE;
                    } else {
                        os = PendingBlackListOption.CONFLICT;
                    }
                    break;
                case EXIST_NUMBER_AND_TYPE:
                    if (isEditPage) {
                        if (!oldNum.equals(newNum) || OldInterruptType != InterruptType) {
                            os = PendingBlackListOption.UPDATE;
                        } else {
                            os = PendingBlackListOption.NONE;
                        }
                    } else {
                        os = PendingBlackListOption.CONFLICT;
                    }
                    break;
            }
        }
        return os;
    }

    public BlackNumberStatus FindNum(BlackCallsUtils blackcallsUtils, String phone_num, int type, String name,
            Activity act) {
        List<BlackNumberEntity> bl = blackcallsUtils.selectBlacklistByNumber(phone_num);
        Log.v(TAG, "bl=" + bl);
        String phoneNamefromDB;
        if (bl == null || bl.size() == 0) {
            return BlackNumberStatus.NONE;
        } else {
            for (BlackNumberEntity entity : bl) {
                if (entity.getName() == null) {
                    phoneNamefromDB = "";
                } else {
                    phoneNamefromDB = entity.getName();
                }
                Log.v(TAG, "name = " + name + " phoneNamefromDB = " + phoneNamefromDB);
                if (type == entity.getType() && name.equals(phoneNamefromDB)) {
                    return BlackNumberStatus.EXIST_NUMBER_AND_TYPE;
                }
            }
            return BlackNumberStatus.EXIST_NUMBER;
        }
    }

    private void ReturnFrontPage(String phonenumber, int type) {
        Log.v(TAG, "ReturnFrontPage");
        Intent intent = new Intent();
        Bundle bundle = new Bundle();
        bundle.putString("phonenumber", phonenumber);
        Log.d(TAG, "PhoneName=" + PhoneName);
        bundle.putString("name", PhoneName);
        bundle.putInt("type", type);
        intent.putExtras(bundle);
        setResult(RESULT_OK, intent);
        finish();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
            Log.v(TAG, "_onKeyDown_PhoneNumber=" + PhoneNumber + ",onKeyDown_type=" + InterruptType);
            ReturnFrontPage(PhoneNumber, InterruptType);
        }
        return super.onKeyDown(keyCode, event);
    }

    public void showToast(int stingId) {
        Toast toast = Toast.makeText(this, stingId, Toast.LENGTH_SHORT);
        toast.show();
    }

    public class TypeAdapter extends BaseAdapter {
        private LayoutInflater mInflater;

        public TypeAdapter(Context context) {
            mInflater = (LayoutInflater) context.getSystemService(
                    Context.LAYOUT_INFLATER_SERVICE);
            selectedMap = new HashMap<Integer, Boolean>();
            if (listArray != null) {
                int lenght = listArray.size();
                for (int i = 0; i < lenght; i++) {
                    if ((1 << i & InterruptType) == 1 << i) {
                        selectedMap.put(i, true);
                    } else {
                        selectedMap.put(i, false);
                    }
                    Log.v(TAG, "selectedMap.get(i)" + selectedMap.get(i));
                }
            }
        }

        @Override
        public int getCount() {
            return listArray.size();
        }

        @Override
        public Object getItem(int position) {
            return position;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (convertView == null) {
                if (mInflater != null) {
                    convertView = mInflater.inflate(R.layout.black_calls_add_type_list_item, null);
                }
            }

            if (convertView != null) {
                TextView mTextView = (TextView) convertView.findViewById(R.id.type_text);
                mCheckBox = (CheckBox) convertView.findViewById(R.id.type_checkbox);
                mTextView.setText(listArray.get(position).toString());
                mCheckBox.setChecked(selectedMap.get(position));
            }
            return convertView;
        }
    }

}
