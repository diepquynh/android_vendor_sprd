package com.sprd.contacts.activities;

import java.util.ArrayList;
import java.util.HashMap;
import com.android.contacts.R;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.AccountTypeManager.SimAas;
import android.app.ListActivity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.util.SparseBooleanArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.Toast;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnShowListener;
import android.widget.EditText;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.text.InputFilter;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.text.Editable;

public class AAsManagerActivity extends ListActivity implements View.OnClickListener{
    /** Called when the activity is first created. */
    private AasAdatpter mAdapter;
    private boolean mEditMode = true;
    private LinearLayout mLayoutEdit = null;
    private LinearLayout mLayoutDelete = null;
    private ArrayList<SimAas> mArrayList = null;
    private ProgressDialog  progressDialog = null;
    private static final int EDIT_OR_ADD_AAS  = 0;
    private static final int DEL_AAS  = 1;
    private boolean isEdit = false;
    AasHandler mHandler;

    private static final int EDIT_DIALOG = 0;
    public int mPostion = 0;
    private static final int INPUT_TYPE_CUSTOM = EditorInfo.TYPE_CLASS_TEXT
            | EditorInfo.TYPE_TEXT_FLAG_CAP_WORDS;

    class Cache {
        public TextView textview;
        public CheckBox checkBox;
    }

    public boolean isChecked = true;

    class AasAdatpter extends BaseAdapter{
        private Context listContext;
        private ArrayList<SimAas> mList;
        public AasAdatpter(Context context,ArrayList<SimAas> mArrayList) {
            listContext = context;
            mList = mArrayList;
        }
        @Override
        public int getCount() {
            // TODO Auto-generated method stub
            return mList.size();
        }

        @Override
        public Object getItem(int position) {
            // TODO Auto-generated method stub
            return mList.get(position);
        }

        @Override
        public long getItemId(int position) {
            // TODO Auto-generated method stub
            return 0;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            // TODO Auto-generated method stub
            convertView =  LayoutInflater.from(listContext).inflate(R.layout.item_view, null);
            if(convertView !=null){
                final Cache cache = new Cache();
                cache.textview  =  (TextView)convertView.findViewById(R.id.textView1);
                ImageView imview = (ImageView) convertView.findViewById(R.id.edit);
                final int cpositon  = position;
                imview.setOnClickListener(new OnClickListener(){
                    @Override
                    public void onClick(View v) {
                        mPostion = cpositon;
                        Log.d("AAsManagerActivity","old aas == " + cache.textview.getText().toString());
                        createDialog(cache.textview.getText().toString());
                    }
                });
                cache.checkBox = (CheckBox) convertView.findViewById(R.id.checkbox);
                cache.checkBox.setChecked(cbs.get(position));
                cache.checkBox.setOnCheckedChangeListener(new OnCheckedChangeListener(){
                    @Override
                    public void onCheckedChanged(CompoundButton buttonView,
                            boolean isChecked) {
                        // TODO Auto-generated method stub
                        cbs.put(cpositon, isChecked);
                    }
                });
                if (mEditMode) {
                    imview.setVisibility(View.VISIBLE);
                    cache.checkBox.setVisibility(View.GONE);
                } else {
                    imview.setVisibility(View.GONE);
                    cache.checkBox.setVisibility(View.VISIBLE);
                }
                cache.textview.setText(mList.get(position).name);
                convertView.setTag(cache);
            }
            return convertView;
        }
    }
    private SparseBooleanArray cbs = new SparseBooleanArray(10);
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.aas_manager);
        mArrayList  = AccountTypeManager.getInstance(this).getAasList();
        mLayoutEdit = (LinearLayout) findViewById(R.id.layoutedit);
        Button btnDelete = (Button) mLayoutEdit.findViewById(R.id.delete);
        btnDelete.setOnClickListener(this);
        Button btnClose = (Button) mLayoutEdit.findViewById(R.id.close);
        btnClose.setOnClickListener(this);
        Button btnInsert = (Button) mLayoutEdit.findViewById(R.id.insert);
        btnInsert.setOnClickListener(this);
        // delete mode
        mLayoutDelete = (LinearLayout) findViewById(R.id.layoutdelete);
        Button btnStartDelete = (Button) mLayoutDelete.findViewById(R.id.startdelete);
        btnStartDelete.setOnClickListener(this);
        Button btnSelectAll = (Button) mLayoutDelete.findViewById(R.id.selectAll);
        btnSelectAll.setOnClickListener(this);
        setEditMode(true);

        mHandler = new AasHandler();
        IntentFilter filter = new IntentFilter();
        filter.addAction("android.intent.action.AAS_SHOWDIALOG");
        filter.addAction("android.intent.action.AAS_DISMISSDIALOG");
        registerReceiver(receiver, filter);

        mAdapter = new AasAdatpter(this,mArrayList);
        setListAdapter(mAdapter);
    }
    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        cbs.clear();
        unregisterReceiver(receiver);
        super.onDestroy();
    }
    @Override
    public void onClick(View v) {
        // TODO Auto-generated method stub
        switch(v.getId()){
        case R.id.edit:
            break;
        case R.id.delete:
            mEditMode = false;
            mAdapter.notifyDataSetInvalidated();
            setEditMode(false);
            break;
        case R.id.close:
            finish();
            break;
        case R.id.startdelete:
            mHandler.sendEmptyMessage(DEL_AAS);
            break;
        case R.id.selectAll:
            for(int i = 0;i<mAdapter.getCount();i++){
                cbs.put(i, isChecked);
            }
            isChecked = !isChecked;
            mAdapter.notifyDataSetChanged();
            break;
        case R.id.insert:
            createDialog(null);
            break;
        }
    }

    BroadcastReceiver receiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if(intent.getAction().equals("android.intent.action.AAS_DISMISSDIALOG")){
                if(progressDialog != null && progressDialog.isShowing()){
                    Log.d("AAsManagerActivity","dismiss dialog");
                    progressDialog.dismiss();
                    progressDialog = null;
                }
            }
        }
    };

    class AasHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch(msg.what){
            case EDIT_OR_ADD_AAS:
                HashMap<String,String> map = (HashMap<String, String>) msg.obj;
                String oldAas = map.get("oldAas");
                String customText = map.get("customText");
                editOrAdd(oldAas, customText);
                break;
            case DEL_AAS:
                delAas();
                break;
            }
        }
    }

    private void editOrAdd(String oldAas, String customText){
        if (customText != null) {
            SimAas aas = new SimAas();
            aas.name = customText;
            if(oldAas != null){
                ArrayList<SimAas> aasList = AccountTypeManager.getInstance(AAsManagerActivity.this).getAasList();
                String aasIndex = "0";
                for(SimAas simAas : aasList){
                    if(simAas.name.equals(oldAas)){
                        aasIndex = simAas.index;
                        break;
                    }
                }
                AccountTypeManager.getInstance(AAsManagerActivity.this).updateAas(AAsManagerActivity.this, aasIndex, aas.name);
                mArrayList.set(mPostion, aas);
            } else {
                Uri uri = AccountTypeManager.getInstance(AAsManagerActivity.this).insertAas(AAsManagerActivity.this, aas.name);
                if (uri != null) {
                    Log.d("AAsManagerActivity","uri == " + uri);
                    if (uri.equals(Uri.parse("aas/over_aas_max_length"))) {
                        Toast.makeText(AAsManagerActivity.this, R.string.aas_maxlength, Toast.LENGTH_SHORT).show();
                        return;
                    } else if (uri.equals(Uri.parse("aas/aas_full"))) {
                        Toast.makeText(AAsManagerActivity.this, R.string.aasfull, Toast.LENGTH_SHORT).show();
                        return;
                    } else {
                        mArrayList.add(aas);
                    }
                }
            }
            AAsManagerActivity.this.sendBroadcast(new Intent("android.intent.action.LOAD_AAS"));
            mAdapter.notifyDataSetChanged();
        }
    }

    private void delAas(){
        int count = mAdapter.getCount();
        ArrayList<SimAas> deList = new ArrayList<SimAas>();
        for(int i = 0;i<count;i++){
            if(cbs.get(i)){
                SimAas aas = (SimAas)mAdapter.getItem(i);
                deList.add(aas);
             }
            Log.i("AAsManagerActivity","delist = "+deList);
        }
        ArrayList<SimAas> aasList = AccountTypeManager.getInstance(AAsManagerActivity.this).getAasList();
        ArrayList<SimAas> del = new ArrayList<SimAas>();
        for(SimAas simAas : aasList){
            for(SimAas delAas : deList){
                if(simAas.name.equals(delAas.name)){
                    boolean aasInContacts = AccountTypeManager.getInstance(AAsManagerActivity.this)
                            .findAasInContacts(this, simAas.index, simAas.name);
                    del.add(simAas);
                    if(!aasInContacts){
                        AccountTypeManager.getInstance(AAsManagerActivity.this)
                            .deleteAas(this, simAas.index ,simAas.name);
                    } else {
                        Toast.makeText(this, this.getString(R.string.aas_in_contacts, simAas.name), Toast.LENGTH_SHORT).show();
                        del.remove(simAas);
                    }
                }
            }
        }
        mArrayList.removeAll(del);
        mAdapter.notifyDataSetChanged();
        AAsManagerActivity.this.sendBroadcast(new Intent("android.intent.action.LOAD_AAS"));
        cbs.clear();
    }

    private void setEditMode(boolean edit){
        mEditMode = edit;
        if(edit){
            mLayoutEdit.setVisibility(View.VISIBLE);
            mLayoutDelete.setVisibility(View.GONE);
        }else{
            mLayoutEdit.setVisibility(View.GONE);
            mLayoutDelete.setVisibility(View.VISIBLE);
            isChecked = true;
            cbs.clear();
        }
    }

    private void createDialog(final String oldAas) {
        final AlertDialog.Builder builder = new AlertDialog.Builder(this);
        final LayoutInflater layoutInflater = LayoutInflater.from(this);
        builder.setTitle(R.string.customLabelPickerTitle);
        // in case user input "" for customLabel, and cancel the dialog...
        builder.setCancelable(false);

        final View view = layoutInflater.inflate(
                R.layout.contact_editor_label_name_dialog, null);
        final EditText editText = (EditText) view
                .findViewById(R.id.custom_dialog_content);
        editText.setInputType(INPUT_TYPE_CUSTOM);
        editText.setSaveEnabled(true);
        editText.setFilters(new InputFilter[] { new InputFilter.LengthFilter(
                140) });
        builder.setView(view);
        editText.requestFocus();
        if(oldAas != null) editText.setText(oldAas);
        builder.setPositiveButton(android.R.string.ok,
               new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        if (oldAas != null) {
                            isEdit = true;
                        }
                        dialog.dismiss();
                        final String customText = editText.getText().toString().trim();
                        HashMap<String,String> map = new HashMap<String,String>();
                        map.put("oldAas", oldAas);
                        map.put("customText", customText);
                        Message msg = new Message();
                        msg.what = EDIT_OR_ADD_AAS;
                        msg.obj = map;
                        mHandler.sendMessage(msg);
                    }
                });
        builder.setNegativeButton(android.R.string.cancel,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        isEdit = false;
                        dialog.dismiss();
                    }
                });

        final AlertDialog dialog = builder.create();
        dialog.setOnShowListener(new OnShowListener() {
            @Override
            public void onShow(DialogInterface dialogInterface) {
                if(oldAas == null){
                    editText.getText().clear();
                    updateCustomDialogOkButtonState(dialog, editText);
                }
            }
        });

        dialog.setOnDismissListener(new OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                if (isEdit) {
                    progressDialog = new ProgressDialog(AAsManagerActivity.this);
                    progressDialog.setMessage(getText(R.string.editing_aas));
                    progressDialog.show();
                }
            }
        });

        editText.addTextChangedListener(new TextWatcher() {
            @Override
            public void onTextChanged(CharSequence s, int start, int before,
                    int count) {
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count,
                    int after) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                updateCustomDialogOkButtonState(dialog, editText);
            }
        });
        dialog.getWindow().setSoftInputMode(
            WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
        dialog.show();
    }
        void updateCustomDialogOkButtonState(AlertDialog dialog, EditText editText) {
            final Button okButton = dialog.getButton(AlertDialog.BUTTON_POSITIVE);
            okButton.setEnabled(!TextUtils.isEmpty(editText.getText().toString().trim()));
        }
    @Override
    public void onBackPressed() {
        if(!mEditMode){
            mEditMode = true;
            mAdapter.notifyDataSetInvalidated();
            setEditMode(true);
        }else{
            super.onBackPressed();
        }
    }
}
