/**SPRD: UPLMN Feature
 * Uplmn can be read, write and update.
 * Usim fileID is 28512 and sim fileID is 28464.
 *
 **/
package com.sprd.uplmnsettings;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.AsyncResult;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManagerEx;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.Toast;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppType;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.uicc.IccFileHandler;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.uicc.IccConstantsEx;


public class UplmnSettings extends Activity {

    private static final String LOG_TAG = "UplmnSettings";
    private static final boolean DBG = true;

    private ListView mListView = null;
    private ArrayAdapter mAdapter;
    private EditText mEditText01, mEditText02, mEditText03;
    private String[] mShowUPLMN = null;
    private String[] mOriginalUPLMN = null;
    private String[] mStrUorG = null;
    private int[] mOrder = null;
    private List<String> mData = new ArrayList<String>();
    private List<Integer> mOffset = new ArrayList<Integer>();

    private EventHandler mHandler;
    private Looper mLooper;

    private Phone mPhone;
    private int mSubId = 0;
    private int mPhoneId = 0;
    private boolean mIsUsim;

    private int LEN_UNIT = 0;
    private int mUplmnLen = 0;
    private int mUplmnNum = 0;
    private IccFileHandler mFh;
    private int mFileID;

    private static final int EVENT_READ_UPLMN_DONE = 1;
    private static final int EVENT_NEW_UPLMN_DONE = 2;
    private static final int EVENT_UPDATE_UPLMN_DONE = 3;
    private static final int EVENT_DELETE_UPLMN_DONE = 4;

    private static final int MENU_NEW_UPLMN =  0;
    private static final int MENU_DELETE_UPLMN = 1;

    private final static int USIM_LEN_UNIT = 5;
    private final static int SIM_LEN_UNIT = 3;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        setTheme(android.R.style.Theme_Material_Settings);
        super.onCreate(savedInstanceState);

        setContentView(R.layout.uplmn_list);

        mSubId = this.getIntent().getIntExtra("sub_id", 0);
        mPhoneId = SubscriptionManager.getPhoneId(mSubId);
        mIsUsim = TelephonyManagerEx.from(this).isUsimCard(mPhoneId);
        mPhone = PhoneFactory.getPhone(mPhoneId);
        mFh = mPhone.getIccCard() == null ? null : mPhone.getIccCard()
                .getIccFileHandler();
        if (mFh == null) {
            log("fileHandler is null, finish this activity");
            finish();
        }
        mLooper = Looper.myLooper();
        mHandler = new EventHandler(mLooper);
        log("mSubId = " + mSubId + ", mIsUsim = " + mIsUsim);

        LEN_UNIT = mIsUsim ? USIM_LEN_UNIT : SIM_LEN_UNIT;
        mFileID = mIsUsim? IccConstantsEx.EF_PLMN_ACT : IccConstantsEx.EF_PLMN_SEL;

        readUplmn();

        mListView = (ListView) findViewById(R.id.ListView01);
        mAdapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_list_item_1, mData);
        mListView.setAdapter(mAdapter);
        mListView.setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> arg0, View arg1,
                    int position, long id) {
                showEditDialog(position,false);
            }
        });
        mListView.setOnItemLongClickListener(new OnItemLongClickListener(){
            public boolean onItemLongClick(AdapterView<?> parent,View view,int position, long id){
                log("onItemLongClick, position = " + position);
                showConfirmDeleteDialog(position);
                return true;
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        menu.add(0, MENU_NEW_UPLMN, 0,getResources().getString(R.string.menu_new_uplmn))
        .setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case MENU_NEW_UPLMN:
            showEditDialog(mOffset.size(),true);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }
    /**
     * Read uplmn from sim
     */
    private void readUplmn() {
        log("readUplmn");
        mFh.loadEFTransparent(mFileID, mHandler.obtainMessage(EVENT_READ_UPLMN_DONE));
    }

    /**
     * Delete uplmn from sim
     */
    private void deleteUplmn(int position){
        int delPos = position;
        String delStr= "";
        for (int i = 0; i < mUplmnNum; i++) {
            if (i != mOffset.get(delPos)) {
                delStr = delStr + mOriginalUPLMN[i];
            }
        }
        for(int i = 0; i< LEN_UNIT * 2; i++){
            delStr = delStr + "F";
        }
        byte binDelplmn[] = IccUtils.hexStringToBytes(delStr);
        mFh.updateEFTransparent(mFileID, binDelplmn, mHandler.obtainMessage(EVENT_DELETE_UPLMN_DONE));
    }

    /**
     * Write new uplmn into sim
     */
    private void newUplmn(int position){
        String newPlmn = changeDataFromEdit(mEditText02.getText().toString());
        String newType = transferUTRANorGSM(mEditText03.getText().toString());
        String tempNewPlmn = newPlmn + newType;
        String newStr = "";
        int newPos = position;
        log("newUplmn : mUplmnNum = " + mUplmnNum + ", newPos = " + newPos);
        if (position < mUplmnNum) {
            mOffset.add(newPos);
            for (int i = 0; i < mUplmnNum; i++) {
                if (i == mOffset.get(newPos)) {
                    mOriginalUPLMN[i] = tempNewPlmn;
                    mShowUPLMN[i] = changeDataFromEdit(mOriginalUPLMN[i].substring(0, 6));
                    setType(i);
                    mData.add(mShowUPLMN[i] + ":" + mStrUorG[i]);
                    mAdapter.notifyDataSetChanged();
                    if(!mIsUsim) {
                        mOriginalUPLMN[i] = mOriginalUPLMN[i].substring(0, 6);
                    }
                }
                newStr = newStr + mOriginalUPLMN[i];
            }
            log("newUplmn: newPlmn=" + newPlmn + ", newType=" + newType + ", newStr=" + newStr);
            byte binNewplmn[] = IccUtils.hexStringToBytes(newStr);
            mFh.updateEFTransparent(mFileID, binNewplmn, mHandler.obtainMessage(EVENT_NEW_UPLMN_DONE));
        } else {
            DisplayToast(getString(R.string.uplmn_exceeds_capacity));
        }
    }

    /**
     * Update uplmn value which has already exist in sim.
     */
    private void updateUplmn(int position){
        String setPlmn = changeDataFromEdit(mEditText02.getText().toString());
        String setType = transferUTRANorGSM(mEditText03.getText().toString());
        String tmpSetPlmn = setPlmn + setType;
        String setStr = "";
        int updatePos = position;
        log("updateUplmn: updatePos=" + updatePos);
        for (int i = 0; i < mUplmnNum; i++) {
            if (i == mOffset.get(updatePos)) {
                mOriginalUPLMN[i] = mIsUsim ? tmpSetPlmn : tmpSetPlmn.substring(0, 6);
            }
            setStr = setStr + mOriginalUPLMN[i];
        }
        log("updateUplmn: setPlmn =" + setPlmn + ", setType = "+ setType + ", setStr = " + setStr);
        byte binSetplmn[] = IccUtils.hexStringToBytes(setStr);
        mFh.updateEFTransparent(mFileID, binSetplmn, mHandler.obtainMessage(EVENT_UPDATE_UPLMN_DONE));
    }

    private void showEditDialog(int pos,boolean isNew) {
        final int offset = pos;
        final boolean isNewPlmn = isNew;
        LayoutInflater factory = LayoutInflater.from(UplmnSettings.this);
        final View view = factory.inflate(R.layout.uplmn_edit, null);
        mEditText01 = (EditText) view.findViewById(R.id.index_value);
        mEditText02 = (EditText) view.findViewById(R.id.id_value);
        mEditText03 = (EditText) view.findViewById(R.id.type_value);
        mEditText01.setText(Integer.toString(pos));
        mEditText02.setText(isNewPlmn ? "" : mShowUPLMN[pos]);
        mEditText03.setText(isNewPlmn ? "" : "" + mOrder[pos]);
        AlertDialog dialog = new AlertDialog.Builder(UplmnSettings.this)
        .setTitle(R.string.uplmn_setting)
        .setView(view)
        .setPositiveButton(android.R.string.ok,
                new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog,
                    int which) {
                final String editIndex = mEditText01.getText()
                        .toString();
                final String editId = mEditText02.getText()
                        .toString();
                final String editTag = mEditText03.getText()
                        .toString();
                if (!checkInputParametersIsWrong(editIndex,
                        editId, editTag)) {
                    if(isNewPlmn){
                        newUplmn(offset);
                    }else{
                        updateUplmn(offset);

                    }
                }
            }
        }).setNegativeButton(android.R.string.cancel, null)
        .create();
        dialog.show();
    }

    private void showConfirmDeleteDialog(final int position) {
        AlertDialog.Builder builder = new AlertDialog.Builder(UplmnSettings.this);
        builder.setMessage(R.string.menu_delete_uplmn)
        .setCancelable(true)
        .setPositiveButton(R.string.delete,
                new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog,
                    int which) {
                deleteUplmn(position);
            }
        }).setNegativeButton(R.string.cancel, null).show();
    }

    private boolean checkInputParametersIsWrong(String editIndex, String editId,
            String editTag) {
        boolean IsWrong = false;
        Matcher m = Pattern.compile("[^0-9]").matcher(editTag);
        if (m.matches()) {
            IsWrong = true;
            DisplayToast(getString(R.string.type_is_wrong_uplmn));
            return IsWrong;
        }
        Matcher checkId = Pattern.compile("[0-9]+").matcher(editId);
        if (!checkId.matches()) {
            IsWrong = true;
            DisplayToast(getString(R.string.id_is_wrong_uplmn));
            return IsWrong;
        }
        if (editIndex.length() == 0) {
            IsWrong = true;
            DisplayToast(getString(R.string.index_error_uplmn));
            return IsWrong;
        }
        if (editTag.length() == 0) {
            IsWrong = true;
            DisplayToast(getString(R.string.type_is_emplty_error_uplmn));
            return IsWrong;
        }
        if (editId.length() < 5) {
            IsWrong = true;
            DisplayToast(getString(R.string.number_too_short_uplmn));
            return IsWrong;
        }
        if (Integer.parseInt(editTag) > 3 || Integer.parseInt(editTag) < 0) {
            IsWrong = true;
            DisplayToast(getString(R.string.type_is_wrong_uplmn));
            return IsWrong;
        }
        return IsWrong;

    }

    /**
     * Format the uplmn which user has input.
     * For example, if user input a uplmn value 46000, it will be first changed to 64F000, then changed to 460F00. And 460F00 will be write into sim.
     */
    private String changeDataFromEdit(String s) {
        String string = "";
        char[] c = new char[6];
        if (s.length() == 5) {
            c[0] = s.charAt(1);
            c[1] = s.charAt(0);
            c[2] = 'F';
            c[3] = s.charAt(2);
            c[4] = s.charAt(4);
            c[5] = s.charAt(3);
            string = String.valueOf(c);
        } else if (s.length() == 6) {
            c[0] = s.charAt(1);
            c[1] = s.charAt(0);
            c[2] = s.charAt(5);
            c[3] = s.charAt(2);
            c[4] = s.charAt(4);
            c[5] = s.charAt(3);
            string = String.valueOf(c);
        }
        return string;
    }

    /**
     * Changed the uplmn type into hexadecimal numbers.
     **/
    private String transferUTRANorGSM(String s) {
        String string = "";
        if (s.equals("0")) {
            string = "8000";
        } else if (s.equals("1")) {
            string = "0080";
        } else if (s.equals("2")) {
            string = "4000";
        } else if (s.equals("3")) {
            string = "C0C0";
        }
        return string;
    }

    private void setAllParameters(int len) {
        mOriginalUPLMN = new String[len];
        mShowUPLMN = new String[len];
        mStrUorG = new String[len];
        mOrder = new int[len];
    }

    private void setType(int position) {
        String tag1 = mOriginalUPLMN[position].substring(6, 7);
        String tag2 = mOriginalUPLMN[position].substring(8, 9);
        String actUtran = formatBinaryType(Integer.toBinaryString(Integer.parseInt(tag1, 16)));
        String actGsm = formatBinaryType(Integer.toBinaryString(Integer.parseInt(tag2, 16)));

        log("setType: actUtran = "+ actUtran + ", actGsm = " +actGsm);

        boolean isUtran = "1".equals(actUtran.substring(0, 1));
        boolean isEutran = "1".equals(actUtran.substring(1, 2));
        boolean isGsm = "1".equals(actGsm.substring(0, 1)) || "1".equals(actGsm.substring(1, 2));
        if(isUtran && isEutran) {
            mOrder[position] = 3;
            mStrUorG[position] = "U/E/G";
        } else if(isUtran) {
            mOrder[position] = 0;
            mStrUorG[position] = "U";
        } else if(isEutran) {
            mOrder[position] = 2;
            mStrUorG[position] = "E";
        } else if(isGsm) {
            mOrder[position] = 1;
            mStrUorG[position] = "G";
        }
    }

    private String formatBinaryType(String act) {
        int length = act.length();
        for(int i = 0; i < 4 - length; i++ ) {
            act = "0" + act;
        }
        return act;
    }

    private class EventHandler extends Handler {
        public EventHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            log("handleMessage msg = " + msg);

            AsyncResult ar;
            byte data[];

            switch(msg.what) {
            case EVENT_READ_UPLMN_DONE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception != null ) {
                    log("Exception get uplmn");
                    DisplayToast(getString(R.string.no_sim_card_prompt));
                    mListView.setVisibility(View.GONE);
                    finish();
                    return;
                }
                data = (byte[])ar.result;
                mUplmnLen = data.length;
                if(mIsUsim && mUplmnLen > 250) {
                    mUplmnLen = 100;
                }
                mUplmnNum = mUplmnLen / LEN_UNIT;
                setAllParameters(mUplmnNum);
                log("mUplmnLen = " + mUplmnLen + ", mUplmnNum = " + mUplmnNum);

                String resultStr = IccUtils.bytesToHexString(data);
                log("resultStr = " + resultStr);

                for (int i = 0; i < mUplmnNum; i++) {
                    mOriginalUPLMN[i] = resultStr.substring(i * LEN_UNIT * 2, (i + 1) * LEN_UNIT * 2);
                    if((data[i * LEN_UNIT] & 0xff) != 0xff) {
                        mShowUPLMN[i] = IccUtils.bcdToString(data, i * LEN_UNIT, 3);
                        log("mOriginalUPLMN[i] = " + mOriginalUPLMN[i]
                                + ", mShowUPLMN[i] = "
                                + mShowUPLMN[i]);
                        if (mIsUsim) {
                            setType(i);
                        } else {
                            mOrder[i] = 1;
                            mStrUorG[i] = "G";
                        }
                        mOffset.add(i);
                        mData.add(mShowUPLMN[i] + ":" + mStrUorG[i]);
                        mAdapter.notifyDataSetChanged();
                        log("mOffset=" + mOffset + ", mData=" + mData);
                    }
                }
                break;

            case EVENT_NEW_UPLMN_DONE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null ) {
                    finish();
                }
                mAdapter.notifyDataSetChanged();
                break;

            case EVENT_UPDATE_UPLMN_DONE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null ) {
                    finish();
                    return;
                }
                mAdapter.notifyDataSetChanged();
                DisplayToast(getString(R.string.set_uplmn_unsuccessful));
                break;

            case EVENT_DELETE_UPLMN_DONE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null ) {
                    finish();
                }
                mAdapter.notifyDataSetChanged();
                break;
            }
        }
    }

    private void DisplayToast(String str) {
        Toast mToast = Toast.makeText(this, str, Toast.LENGTH_SHORT);
        mToast.show();
    }

    private void log(String s) {
        if (DBG) {
            Log.d(LOG_TAG, "[UplmnSettings" + mPhoneId + "] " + s);
        }
    }
}