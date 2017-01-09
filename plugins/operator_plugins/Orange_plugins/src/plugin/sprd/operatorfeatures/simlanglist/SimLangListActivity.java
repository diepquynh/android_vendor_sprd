package plugin.sprd.orangefeatures;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;


import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.Toast;

import com.android.internal.telephony.uicc.IccConstants;
import com.android.internal.telephony.uicc.IccUtils;
import android.telephony.TelephonyManager;
import android.telephony.SubscriptionManager;

import java.util.Arrays;

public class SimLangListActivity extends Activity implements IccConstants{
    private static final String LOG_TAG = "SimLangListActivity";
    private final boolean DEBUG = true;

    private static int mSubId;
    private List<String> mAllLangName = new ArrayList<String>();
    private List<Integer> mAllLangCode = new ArrayList<Integer>();
    //private List<Integer> mPlCode = new ArrayList<Integer>();
    //private List<Integer> mLpCode = new ArrayList<Integer>();
    private int mCode[] = null;
    private String mLangName[] = null;

    private int mLongClickPos = -1;
    private int mEditType = 0;
    private int mFlagEdited = 0;
    private TelephonyManager mTelmanager = null;
    private ArrayAdapter mAdapter = null;

    private static final int EF_LANG_CNT = 2;
    private static final int EDIT_CAHNGE = 0;
    private static final int EDIT_ADD_NEW = 1;

    private static final int FLAG_EF_USIM_LI_EDITED = 1;
    private static final int FLAG_EF_PL_EDITED = 2;
    private static final int FLAG_EF_LP_EDITED = 4;
    //update native user interface
    private static final int DATA_UPDATE = 101;
    private static final int RESPONSE_DATA_FILE_SIZE_1 = 2;
    private static final int RESPONSE_DATA_FILE_SIZE_2 = 3;

    private static final int USIM_FILE_ID= 28421;
    private static final int PL_FILE_ID = 12037;
    private static final int COMMAND_GET_LENGTH = 192;
    private static final int COMMAND_UPDATE = 214;
    private static final int DATA_LENGTH = 15;

    private int mPlDataLength = 0;
    private int mLanguageAmount = 0;
    private int mLanguageAmountLimits;
    private boolean isEfLi = true;
    private boolean isDataAvaliable = true;

    private String getLangNameByCode(int code){
        int len = mCode.length;
        for(int i=0; i<len; i++){
            if(code == mCode[i]){
                return mLangName[i];
            }
        }
        return getResources().getString(R.string.unknown_lang);
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
        Log.d(LOG_TAG,"getUsimLiData...mAllLangCode "+IccUtils.bytesToHexString(data));
        mLanguageAmountLimits = data.length/2;
        if(!(data[0] == 0xff && data[1] == 0xff)){
            if(data!=null){
                int code = 0;
                int len = data.length;
                mAllLangCode.clear();
                for(int i=0; i<len; i+=2){
                    code = (data[i]<<8) |data[i+1];
                    Log.d(LOG_TAG," code " +code);
                    if(code != -1){// ?????
                        mAllLangCode.add(code);
                        mLanguageAmount ++;
                    }
                }
                for(int i=0; i<mAllLangCode.size(); i++){
                    mAllLangName.add(getLangNameByCode(mAllLangCode.get(i)));
                }
                if(DEBUG){
                    for(int j=0;j<mAllLangCode.size();j++){
                        Log.d(LOG_TAG,"getUsimLiData...mAllLangCode["+j+"]= "+mAllLangCode.get(j));
                        Log.d(LOG_TAG,"getUsimLiData...mAllLangName["+j+"]= "+mAllLangName.get(j));
                    }
                }
            }
        }
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
                Log.d(LOG_TAG,"load data failed");
            }
            mCode[i] = Integer.parseInt(IccUtils.bytesToHexString(codelangbyte),16);
            ++i;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        mSubId = getIntent().getIntExtra("sub_id",0);
        mTelmanager = (TelephonyManager)TelephonyManager.from(this);
        setContentView(R.layout.simlanglist);
        ListView lv = (ListView)findViewById(R.id.listview);
        Button btn = (Button)findViewById(R.id.add_new_button);

        mAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, mAllLangName);
        lv.setAdapter(mAdapter);
        lv.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener(){
            @Override
            public boolean onItemLongClick(AdapterView<?> arg0, View arg1,int arg2, long arg3) {
                mLongClickPos = arg2;
                showLangChooseDlgForEdit(EDIT_CAHNGE);
                return false;
            }
        });

        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                showLangChooseDlgForEdit(EDIT_ADD_NEW);
            }
        });
        getSimLangData();
        super.onCreate(savedInstanceState);
    }

    private void showLangChooseDlgForEdit(int editType){
        mEditType = editType;
        int titleres = R.string.langchoose_list_title_change;
        if(EDIT_ADD_NEW == mEditType){
            titleres = R.string.langchoose_list_title_add;
        }else if(EDIT_CAHNGE == mEditType){
            titleres = R.string.langchoose_list_title_change;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setItems(mLangName, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface arg0, int arg1) {
                boolean notifyDataSet = true;
                if(EDIT_ADD_NEW == mEditType){
                    if(mLanguageAmount < mLanguageAmountLimits){
                        addLangugeFromListPos(arg1);
                        mLanguageAmount ++;
                    }else{
                        Toast.makeText(SimLangListActivity.this, getString(R.string.lang_exceed), Toast.LENGTH_SHORT).show();
                        notifyDataSet = false;
                    }
                }else if(EDIT_CAHNGE == mEditType){
                    changeLangugeFromListPos(arg1);
                }
                setSimLangData();
                if(mAdapter!=null&&notifyDataSet){
                    mAdapter.notifyDataSetChanged();
                }
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
            mFlagEdited |= FLAG_EF_USIM_LI_EDITED;
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

    private void setSimLangData(){
        Log.d(LOG_TAG,"setSimLangData...mFlagEdited: "+mFlagEdited);
        byte data[] = getWideCodeBytesFromList(mAllLangCode);
        if (isEfLi) {
            if (mTelmanager != null) {
                Log.d(LOG_TAG,"getWideCodeBytesFromList Li"+IccUtils.bytesToHexString(data));
                mTelmanager.iccExchangeSimIOUpdate(USIM_FILE_ID, COMMAND_UPDATE, 0, 0, data.length, IccUtils.bytesToHexString(data),"3F007FFF",mSubId);
            }
        }
        if ( isDataAvaliable && !isEfLi) {
            if (mTelmanager != null) {
                Log.d(LOG_TAG,"getWideCodeBytesFromList PL "+IccUtils.bytesToHexString(data));
                mTelmanager.iccExchangeSimIOUpdate(PL_FILE_ID, COMMAND_UPDATE, 0, 0, data.length, IccUtils.bytesToHexString(data),"3F00",mSubId);
            }
        }
        mFlagEdited = 0;
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
            Log.d(LOG_TAG,"everytime.... intstr= "+intstr);
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
            Log.d(LOG_TAG,"everytime.... hexstr= "+hexstr);
        }
        for(int j = 0; j<remain;j++){
            hexstr+="ffff";
        }
        if(DEBUG){
            Log.d(LOG_TAG,"getWideCodeBytesFromList.... hexstr= "+hexstr);
        }
        return IccUtils.hexStringToBytes(hexstr);
    }

    private void getSimLangData(){
        new Thread(){
            @Override
            public void run() {
                // TODO Auto-generated method stub
                get3GPPLangCodeList();
                if(mTelmanager!=null){
                    Log.d(LOG_TAG,"getSimLangData.... begin...");
                    getUsimLiData(getDataFromSim(USIM_FILE_ID, COMMAND_GET_LENGTH, DATA_LENGTH, "3F007FFF", mSubId));
                    if(isEfLi == false){
                        getUsimLiData(getDataFromSim(PL_FILE_ID, COMMAND_GET_LENGTH,  DATA_LENGTH, "3F00", mSubId));
                    }
                    Message message = new Message();
                    message.what = DATA_UPDATE;
                    //mTelmanager.GetSimEfLang(false,IccConstants.EF_LP);
                    SimLangListActivity.this.myHandler.sendMessage(message);
                }
                super.run();
            }
        }.start();
    }

    private byte[] getDataFromSim(int fileID, int command,  int p3, String filePath,int subId){
        byte[] result = mTelmanager.iccExchangeSimIOwithSubId(fileID, command, 0, 0, p3, filePath, subId);
        //return from iccExchangeSimIOwithSubId include sw1,sw2 and payload of IccIoResult."2" means sw1 and sw2 that have to be cutoff
        if (result.length<=2) {
            return null;
        }
        result = Arrays.copyOf(result, result.length-2);
        int size = ((result[RESPONSE_DATA_FILE_SIZE_1] & 0xff) << 8) + (result[RESPONSE_DATA_FILE_SIZE_2] & 0xff);
        result = mTelmanager.iccExchangeSimIOwithSubId(fileID, 176, 0, 0, size, filePath, subId);
        if (result.length<=2) {
            return null;
        }
        return Arrays.copyOf(result, result.length-2);
    }

    private Handler myHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case DATA_UPDATE:
                    if(mAdapter!=null){
                        mAdapter.notifyDataSetChanged();
                    }
                    break;
                }
            super.handleMessage(msg);
        }
    };
}
