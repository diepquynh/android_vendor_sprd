package com.sprd.simlanguages;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Looper;
import android.telephony.SubscriptionManager;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.Toast;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.uicc.IccFileHandler;
import com.sprd.simlanguages.R;

import java.io.UnsupportedEncodingException;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;

public class SimLangListActivity extends Activity{
    private static final String LOG_TAG = "SimLangListActivity";
    private final boolean DBG = true;

    private int mSubId;
    private int mPhoneId;
    private Phone mPhone;
    private IccFileHandler mFh;

    private EventHandler mEventHandler;
    private Looper mLooper;

    private ArrayAdapter mAdapter = null;
    private List<String> mAllLangName = new ArrayList<String>();
    private List<Integer> mAllLangCode = new ArrayList<Integer>();
    private int mCode[] = null;
    private String mLangName[] = null;

    private int mLongClickPos = -1;
    private int mEditType = 0;
    private int mFlagEdited = 0;
    private int mPlDataLength = 0;
    private int mLanguageAmount = 0;
    private int mLanguageAmountLimits = 0;
    private boolean isEfLi = true;
    private boolean isDataAvaliable = true;

    private static final int TYPE_EDIT = 0;
    private static final int TYPE_ADD = 1;

    private static final int FLAG_EF_LI_EDITED = 1;

    private static final int EF_LI= 28421;
    private static final int EF_PL = 12037;

    private static final int EVENT_READ_EF_LI_DONE = 101;
    private static final int EVENT_READ_EF_PL_DONE = 102;
    private static final int EVENT_UPDATE_EF_LI_DONE = 103;
    private static final int EVENT_UPDATE_EF_PL_DONE = 104;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        mSubId = getIntent().getIntExtra("sub_id",0);
        log("onCreate mSubId = " + mSubId);
        mPhoneId = SubscriptionManager.getPhoneId(mSubId);
        mPhone = PhoneFactory.getPhone(mPhoneId);
        mFh = mPhone.getIccCard() == null ? null : mPhone.getIccCard()
                .getIccFileHandler();
        mLooper = Looper.myLooper();
        mEventHandler = new EventHandler(mLooper);

        if (mFh == null) {
            log("fileHandler is null, finish this activity");
            finish();
        }

        setContentView(R.layout.sim_language_list);
        Button btn = (Button)findViewById(R.id.btn_add_language);
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                showLangChooseDlgForEdit(TYPE_ADD);
            }
        });

        ListView lv = (ListView)findViewById(R.id.listview);
        mAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, mAllLangName);
        lv.setAdapter(mAdapter);
        lv.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener(){
            @Override
            public boolean onItemLongClick(AdapterView<?> arg0, View arg1,int arg2, long arg3) {
                mLongClickPos = arg2;
                showLangChooseDlgForEdit(TYPE_EDIT);
                return false;
            }
        });

        get3GPPLangCodeList();
        loadEfLi();

        super.onCreate(savedInstanceState);
    }

    private void get3GPPLangCodeList(){
        int i = 0;
        int cnt = 0;
        String itemList[] = getResources().getStringArray(R.array.code_to_lang);
        cnt = itemList.length;
        if(mCode == null){
            mCode = new int[cnt];
        }
        if(mLangName == null){
            mLangName = new String[cnt];
        }
        for (String item:itemList){
            String codelang[] = item.split("=");
            mLangName[i] = codelang[1];
            byte codelangbyte[] = null;
            try{
                codelangbyte = codelang[0].getBytes("ISO-8859-1");
            }catch(UnsupportedEncodingException e){
                log("load data failed");
            }
            mCode[i] = Integer.parseInt(IccUtils.bytesToHexString(codelangbyte),16);
            ++i;
        }
    }

    private void loadEfLi(){
        log("loadEfLi");
        mFh.loadEFTransparent(EF_LI,mEventHandler.obtainMessage(EVENT_READ_EF_LI_DONE));
    }

    private void loadEfPl(){
        log("loadEfPl");
        mFh.loadEFTransparent(EF_PL,mEventHandler.obtainMessage(EVENT_READ_EF_PL_DONE));
    }

    private void updateEfLiAndEfPl(){
        byte data[] = getWideCodeBytesFromList(mAllLangCode);
        if (isEfLi) {
            mFh.updateEFTransparent(EF_LI, data, mEventHandler.obtainMessage(EVENT_UPDATE_EF_LI_DONE));
        }
        if ( isDataAvaliable && !isEfLi) {
            mFh.updateEFTransparent(EF_PL, data, mEventHandler.obtainMessage(EVENT_UPDATE_EF_PL_DONE));
        }
        mFlagEdited = 0;
    }

    private void showLangChooseDlgForEdit(int editType){
        mEditType = editType;
        int titleres = R.string.langchoose_list_title_change;
        if(TYPE_ADD == mEditType){//add language
            titleres = R.string.langchoose_list_title_add;
        }else if(TYPE_EDIT == mEditType){//edit language
            titleres = R.string.langchoose_list_title_change;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setItems(mLangName, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface arg0, int arg1) {
                if(TYPE_ADD == mEditType){
                    if(mLanguageAmount < mLanguageAmountLimits){
                        addLangugeFromListPos(arg1);
                        mLanguageAmount ++;
                    }else{
                        Toast.makeText(SimLangListActivity.this, getString(R.string.lang_exceed), Toast.LENGTH_SHORT).show();
                    }
                }else if(TYPE_EDIT == mEditType){
                    changeLangugeFromListPos(arg1);
                }
                updateEfLiAndEfPl();
            }
        })
        .setTitle(titleres)
        .create()
        .show();
    }

    private void addLangugeFromListPos(int pos){
        if(!mAllLangCode.contains(mCode[pos])){
            mAllLangName.add(mLangName[pos]);
            mAllLangCode.add(mCode[pos]);
            mFlagEdited |= FLAG_EF_LI_EDITED;
        }
        if(mFlagEdited == 0){
            Toast.makeText(this, getString(R.string.lang_already_exist), Toast.LENGTH_SHORT).show();
            mLanguageAmount --;
        }
    }

    private void changeLangugeFromListPos(int pos){
        int code = mAllLangCode.get(mLongClickPos);
        for (int i = 0;i< mAllLangCode.size(); i++ ) {
            if(mAllLangCode.get(i) ==  mCode[pos] && i!=mLongClickPos){
                Toast.makeText(SimLangListActivity.this, getString(R.string.lang_already_exist), Toast.LENGTH_SHORT).show();
                return;
            }
        }
        mAllLangName.set(mLongClickPos, mLangName[pos]);
        mAllLangCode.set(mLongClickPos, mCode[pos]);
    }

    private byte[] getWideCodeBytesFromList(List<Integer> list){
        String hexstr = "";
        String intstr;
        int len = list.size();
        int intstrlen = 0;
        List<String> itemUsed = new ArrayList<String>();
        int remain = mLanguageAmountLimits;
        for(int i=0; i < len; i++){
            intstr = Integer.toHexString(list.get(i));
            log("everytime.... intstr= "+intstr);
            intstrlen = intstr.length();
            if((intstrlen & 0x01) == 0x01){// if intstrlen is not a even number
                intstr = "0"+intstr;
            }
            if(!itemUsed.contains(intstr)){
                if(list.get(i).intValue() <= 0xff){
                    hexstr = hexstr + "00" +  intstr;
                }else{
                    hexstr = hexstr + intstr;
                }
                itemUsed.add(intstr);
                remain--;
            }
            log("everytime.... hexstr= "+hexstr);
        }
        for(int j = 0; j<remain;j++){
            hexstr+="ffff";
        }
        if(DBG){
            log("getWideCodeBytesFromList.... hexstr= "+hexstr);
        }
        return IccUtils.hexStringToBytes(hexstr);
    }

    private void getUsimLiData(byte[] data){
        if (data == null) {
            if (isEfLi == false) {
                isDataAvaliable = false;
                return;
            }
            isEfLi = false;
            return;
        }
        log("getUsimLiData...mAllLangCode " + IccUtils.bytesToHexString(data));
        mLanguageAmountLimits = data.length/2;
        if(!(data[0] == 0xff && data[1] == 0xff)){
            if(data!=null){
                int code = 0;
                int len = data.length;
                mAllLangCode.clear();
                for(int i=0; i<len; i+=2){
                    code = (data[i]<<8) |data[i+1];
                    log(" code " +code);
                    if(code != -1){// ?????
                        mAllLangCode.add(code);
                        mLanguageAmount ++;
                    }
                }
                for(int i=0; i<mAllLangCode.size(); i++){
                    mAllLangName.add(getLangNameByCode(mAllLangCode.get(i)));
                }
                if(DBG){
                    for(int j=0;j<mAllLangCode.size();j++){
                        log("getUsimLiData...mAllLangCode["+j+"]= "+mAllLangCode.get(j));
                        log("getUsimLiData...mAllLangName["+j+"]= "+mAllLangName.get(j));
                    }
                }
            }
        }
    }
    private String getLangNameByCode(int code){
        int len = mCode.length;
        for(int i=0; i<len; i++){
            if(code == mCode[i]){
                return mLangName[i];
            }
        }
        return getResources().getString(R.string.unknown_lang);
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

            switch(msg.what){
            case EVENT_READ_EF_LI_DONE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception != null ) {
                    log("Exception read ef_li");
                    break;
                }
                data = (byte[])ar.result;
                log("data : " + IccUtils.bytesToHexString(data));
                getUsimLiData(data);
                if(isEfLi == false){
                    loadEfPl();
                }
                mAdapter.notifyDataSetChanged();
                break;
            case EVENT_READ_EF_PL_DONE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception != null ) {
                    log("Exception read ef_pl");
                    break;
                }
                data = (byte[])ar.result;
                log("data : " + IccUtils.bytesToHexString(data));
                getUsimLiData(data);
                mAdapter.notifyDataSetChanged();
                break;
            case EVENT_UPDATE_EF_LI_DONE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception != null ) {
                    log("Exception update ef_li");
                    break;
                }
                mAdapter.notifyDataSetChanged();
                break;
            case EVENT_UPDATE_EF_PL_DONE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception != null ) {
                    log("Exception update ef_pl");
                    break;
                }
                mAdapter.notifyDataSetChanged();
                break;
            }

        }
    }

    private void log(String s) {
        if (DBG) {
            Log.d(LOG_TAG, "[SimLangListActivity" + mPhoneId + "] " + s);
        }
    }

}
