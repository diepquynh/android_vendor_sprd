package com.sprd.soundrecorder;

import java.io.File;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.TreeMap;
import java.util.regex.MatchResult;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.android.soundrecorder.R;
import com.android.soundrecorder.Recorder;
import com.android.soundrecorder.SoundRecorder;

import android.app.ActionBar;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.graphics.Color;
import android.media.AudioManager;
import android.media.AudioManager.OnAudioFocusChangeListener;
import android.media.Image;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Build;
import android.os.Message;
import android.Manifest;
import android.provider.MediaStore;
import android.text.Editable;
import android.text.InputFilter;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.view.ActionMode;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.Button;
import android.content.res.Resources;
import android.view.Surface;
import android.annotation.TargetApi;
import android.webkit.MimeTypeMap;

import com.sprd.soundrecorder.StorageInfos;
import com.sprd.soundrecorder.Utils;
import com.sprd.soundrecorder.MarkSeekBar;
import com.sprd.soundrecorder.PreviewPlayer;
import com.sprd.soundrecorder.RecordDatabaseHelper;
import com.sprd.soundrecorder.RecordingFilePlay;

public class RecordingFileList extends ListActivity
        implements AdapterView.OnItemLongClickListener, AdapterView.OnItemClickListener , TextWatcher , Button.OnClickListener{

    private static final String TAG = "RecordingFileList";
    //fix bug 203115"checkbox state changes after changing language" on 20130820 begin
    private static final String BUNDLEKEY = "1";
    //fix bug 203115"checkbox state changes after changing language" on 20130820 end
    private static String DUPLICATE_NAME = "duplicate_name";
    private static int INPUT_MAX_LENGTH = 50;
    private ListView mListView;
    private CursorRecorderAdapter mAdapter;

    //modify by linying bug115502 begin
    private TextView remindTextView = null;
    //modify by linying bug115502 begin
    private HashMap<Integer,Boolean> checkboxes = new HashMap<Integer,Boolean>();
    private Map<Integer,RecorderItem> checkItem = new TreeMap<Integer,RecorderItem>();
    private List<RecorderItem> items = new ArrayList<RecorderItem>();
    private boolean mIsPause = false;
    private List<RecorderItem> mList = new ArrayList<RecorderItem>();
    TextView mTextViewSelectCount;
    private Dialog mAlertDialog;
    private static final int START_RECORDING_DIALOG_SHOW = 1;
    public final static String SAVE_RECORD_TYPE = "recordType";
    public final static String SAVE_RECORD_TYPE_ID = "recordTypeId";
    public final static String SOUNDREOCRD_TYPE_AND_DTA = "soundrecord.type.and.data";
    //SPRD: fix bug 513105
    public final static String SYSTEM_VERSION = "systemVersion";
    public String mType = SoundRecorder.AUDIO_AMR;
    int index = 0;
    private boolean isFirstTime = true;

    private ActionMode mActionMode = null;
    private Menu menu = null;
    private MenuItem mMenuItem_Share = null;
    private MenuItem mMeunItem_Delete = null;
    private MenuItem mMenuItem_More = null;
    private MenuItem mMeunSubItem_selectAll = null;
    private MenuItem mMeunSubItem_reName = null;
    private MenuItem mMeunSubItem_setRing = null;
    private MenuItem mMeunSubItem_viewDetails = null;

    private boolean mPausedByTransientLossOfFocus;
    private boolean mPlayFromFocusCanDuck = false;
    private AudioManager mAudioManager;
    private PreviewPlayer mPlayer = null;
    private Handler mProgressRefresher = new Handler();
    private LinearLayout mPlayLayout = null;
    private ImageView mCurPlayButton = null;
    private RelativeLayout mCurItemLayout = null;
    private TextView mCurRecordSize = null;
    private MarkSeekBar mCurSeekBar =null;
    private ImageView mCurTagIcon = null;
    private TextView mCurRecordDuration = null;
    private int mLastClickPos = -1;
    private RecordDatabaseHelper mDBHelper;
    public static HashMap<Integer,Integer> mTagHashMap;
    private static final int FADEDOWN = 2;
    private static final int FADEUP = 3;
    private List<Integer> mRecordIDList = new ArrayList<Integer>();
    private View mPlayingItem = null;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().requestFeature(Window.FEATURE_ACTION_BAR);
        setContentView(R.layout.recording_file_list);
        ActionBar actionBar = getActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        //modify by linying bug115502 begin
        remindTextView = (TextView)findViewById(R.id.emptylist);
        //modify by linying bug115502 begin
        registerReceiver();

        setVolumeControlStream (AudioManager.STREAM_MUSIC);
        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        mDBHelper = new RecordDatabaseHelper(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.e(TAG,"onResume");
        getCurrentTypeId();
        mIsPause = false;
        mListView = getListView();
        mListView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
        if (checkItem != null) {
            checkItem.clear();
        }
        /** SPRD:Bug 615710 reset application preferencesonce again into the tape recorderthe recording list is empty  ( @{ */
        checkAndBuildPermissions();
        /** @} */
        /** SPRD:Bug 610511 com.android.soundrecorder happens ANR @{ */
        queryFileAysnc();
        /** @} */
        mListView.setOnItemClickListener(this);
        mListView.setOnItemLongClickListener(this);
    }

    private BroadcastReceiver mExternalMountedReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (!mIsPause) {
                mList = query();
                mAdapter  = new CursorRecorderAdapter(mList , mActionMode != null ? false : true);
                setListAdapter(mAdapter);
                Log.i(TAG,"onReceive update mAdapter");
                detectionList();
            }
            clearContainer();
            buttonClickableChanges();
            if (mDelDlg != null && mDelDlg.isShowing()) {
                dismissDelDialog();
            }
        }
    };

    private BroadcastReceiver mHomeKeyEventReceive = new BroadcastReceiver() {
        String SYSTEM_REASON = "reason";
        String SYSTEM_HOME_KEY = "homekey";
        String SYSTEM_HOME_KEY_RECENT = "recentapps";
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)) {
                String reason = intent.getStringExtra(SYSTEM_REASON);
                if (TextUtils.equals(reason, SYSTEM_HOME_KEY)
                        || TextUtils.equals(reason, SYSTEM_HOME_KEY_RECENT)) {
                    stopPlayback();
                }
            } else if (action.equals(Intent.ACTION_SCREEN_OFF)) {
                stopPlayback();
            }
        }
    };

    @Override
    public void onPause(){
        super.onPause();
        mIsPause = true;
        // SPRD: bug597698 Recording haven't stop when answer a call.
        stopPlayback();
    }

    @Override
    public void onDestroy(){
        super.onDestroy();
        Log.d(TAG,"onDestroy");
        unregisterReceiver(mExternalMountedReceiver);
        unregisterReceiver(mHomeKeyEventReceive);
        if(mActionMode != null){
            mActionMode.finish();
        }
        dismissDelDialog();
        if(mAlertDialog != null){
            mAlertDialog.dismiss();
        }
    }
    private void registerReceiver(){
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Intent.ACTION_MEDIA_REMOVED);
        intentFilter.addAction(Intent.ACTION_MEDIA_BAD_REMOVAL);
        intentFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_SCANNER_FINISHED);
        intentFilter.addDataScheme("file");
        registerReceiver(mExternalMountedReceiver, intentFilter);
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        registerReceiver(mHomeKeyEventReceive, filter);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        Intent intent = null;
        switch (item.getItemId()) {
            case android.R.id.home:
                if (mPlayer != null) {
                    stopPlayback();
                    return true;
                }
                onBackPressed();
                break;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    public void saveRecordTypeAndSetting(){
        SharedPreferences recordSavePreferences = this.getSharedPreferences(SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
        SharedPreferences.Editor edit = recordSavePreferences.edit();
        edit.putString(RecordingFileList.SAVE_RECORD_TYPE, mType);
        edit.putInt(RecordingFileList.SAVE_RECORD_TYPE_ID, index);
        edit.commit();
        Log.i(TAG,"mType is saved:" + mType);
        Log.e(TAG,"mTypeId is saveds:" + index);
    }

    private Handler hand= new Handler(){
        @Override
        public void handleMessage(Message msg) {
            Log.i(TAG,"the Message is:" + msg.what);
            switch(msg.what){
                case START_RECORDING_DIALOG_SHOW:
                    mType=(String)msg.obj;
                    Log.i(TAG,"mType is:" + mType);
                    saveRecordTypeAndSetting();
                    mAlertDialog.dismiss();
                    break;
            }
        }
    };
    private int getCurrentTypeId(){
        SharedPreferences recordSavePreferences = this.getSharedPreferences(SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
        index = recordSavePreferences.getInt(RecordingFileList.SAVE_RECORD_TYPE_ID, index);
        return index;
    }
    @Override
    public boolean onItemLongClick(AdapterView<?> adapter, View v, int pos, long id) {
        if (mPlayer != null) {
            stopPlayback();
        }
        if (mActionMode == null && mAdapter != null) {
            checkboxes.put(pos, true);
            RecorderItem item = mAdapter.findItem(pos);
            checkItem.put(pos, item);
            startActionMode(new itemLongClickCallback(mAdapter.findItem(pos)));
            if (isFirstTime) {
                buttonClickableChanges();
                isFirstTime = false;
            }
        } else {
            return false;
        }
        return true;
    }

    private class itemLongClickCallback implements ActionMode.Callback {
        private RecorderItem items;

        public itemLongClickCallback(RecorderItem item) {
            this.items = item;
        }

        @Override
        public boolean onCreateActionMode(ActionMode mode, Menu menu) {
            mActionMode = mode;
            RecordingFileList.this.menu = menu;
            MenuInflater inflater = mode.getMenuInflater();
            inflater.inflate(R.menu.mutli_choice, menu);
            mActionMode.setTitle("0");
            mAdapter.setCheckboxHidden(false);
            mAdapter.notifyDataSetChanged();
            Utils.updateStatusBarColor(RecordingFileList.this, true);
            return true;
        }

        @Override
        public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
            mMenuItem_Share = menu.findItem(R.id.item_share);
            mMeunItem_Delete = menu.findItem(R.id.item_delete);
            //mMenuItem_More = menu.findItem(R.id.item_more);
            mMeunSubItem_selectAll = menu.findItem(R.id.sub_item_select_all);
            mMeunSubItem_reName = menu.findItem(R.id.sub_item_rename);
            mMeunSubItem_setRing = menu.findItem(R.id.sub_item_set_ring);
            mMeunSubItem_viewDetails = menu.findItem(R.id.sub_view_details);
            buttonClickableChanges();
            return true;
        }

        @Override
        public void onDestroyActionMode(ActionMode mode) {
            mActionMode = null;
            clearContainer();
            if (mAdapter != null) {
                mAdapter.setCheckboxHidden(true);
                mAdapter.notifyDataSetChanged();
            }
            isFirstTime = true;
            Utils.updateStatusBarColor(RecordingFileList.this, false);
        }

        @Override
        public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
            try{
                switch (item.getItemId()) {
                case R.id.item_share:
                    shareFiles();
                    break;
                case R.id.item_delete:
                    fileDelete(null);
                    break;
                case R.id.sub_item_select_all:
                    if (mMeunSubItem_selectAll.getTitle() == getResources().getString(R.string.menu_recording_list_deselect_all)) {
                        unSelectAll();
                    } else {
                        selectAll();
                    }
                    buttonClickableChanges();
                    break;
                case R.id.sub_item_rename:
                    fileRename(null);
                    break;
                case R.id.sub_item_set_ring:
                    if (checkItem.size() == 1) {
                        RecorderItem recorditem = null;
                        for (Map.Entry<Integer, RecorderItem> entry : checkItem.entrySet()) {
                            recorditem = entry.getValue();
                            //set as ring tone
                            Utils.doChoiceRingtone(RecordingFileList.this, recorditem.id);
                        }
                        mActionMode.finish();
                    }
                    break;
                case R.id.sub_view_details:
                    filePath(null);
                    break;
                } 
            } catch(Exception e){
                Toast.makeText(RecordingFileList.this, R.string.file_is_corrupt, Toast.LENGTH_SHORT).show();
            }
            return true;
        }
    };

    private void buttonClickableChanges(){
        int checkitemSize = checkItem.size();
        if(mActionMode != null){
            mActionMode.setTitle(String.valueOf(checkitemSize));
        }
        if(menu == null){
            return;
        }
        if(checkitemSize == 1){
            mMeunSubItem_reName.setVisible(true);
            mMeunSubItem_setRing.setVisible(true);
            mMeunSubItem_viewDetails.setVisible(true);
            if (mList.size() == 1) {
                mMeunSubItem_selectAll.setTitle(R.string.menu_recording_list_deselect_all);
            }else{
                mMeunSubItem_selectAll.setTitle(R.string.menu_recording_list_select_all);
            }
        }else{
            mMeunSubItem_reName.setVisible(false);
            mMeunSubItem_setRing.setVisible(false);
            mMeunSubItem_viewDetails.setVisible(false);
            if (checkitemSize == mList.size()) {
                mMeunSubItem_selectAll.setTitle(R.string.menu_recording_list_deselect_all);
            }else{
                mMeunSubItem_selectAll.setTitle(R.string.menu_recording_list_select_all);
            }
        }
        if(checkitemSize <= 0){
            mMenuItem_Share.setVisible(false);
            mMeunItem_Delete.setVisible(false);
        }else{
            mMenuItem_Share.setVisible(true);
            mMeunItem_Delete.setVisible(true);
        }
       Button buttonRename = (Button) findViewById(R.id.textview_file_rename);
       buttonRename.setOnClickListener(this);
       Button buttonFilepath = (Button) findViewById(R.id.textview_file_path);
       buttonFilepath.setOnClickListener(this);
       if (checkitemSize > 1 || checkitemSize <= 0) {
           buttonRename.setEnabled(false);
           buttonFilepath.setEnabled(false);
       } else {
            buttonRename.setEnabled(true);
            buttonFilepath.setEnabled(true);
        }
        Button buttonDelete = (Button) findViewById(R.id.textview_file_delete);
        buttonDelete.setOnClickListener(this);
        if (checkitemSize <= 0) {
            buttonDelete.setEnabled(false);
        } else {
            buttonDelete.setEnabled(true);
        }
    }
    /**
     * update the DB
     * @param title
     * @param id
     */
    private int updateRecordingFileDB(String newPath, String title, String displayName, RecorderItem item) {
        Uri uri = MediaStore.Audio.Media.INTERNAL_CONTENT_URI;

        if (StorageInfos.isExternalStorageMounted() || !StorageInfos.isInternalStorageSupported()) {
            uri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
        }
        ContentValues values = new ContentValues();
        values.put(MediaStore.Audio.Media.DISPLAY_NAME, displayName);
        values.put(MediaStore.Audio.Media.TITLE, title);
        values.put(MediaStore.Audio.Media.DATA, newPath);
        /* SPRD: fix bug 523045 @{ */
        int result = 0;
        try {
            result = getContentResolver().update(uri, values, "_ID=?", new String[] {
                    String.valueOf(item.getId())
                });
        } catch (Exception e) {
            e.printStackTrace();
        }
        /* @} */
        return result;
    }

    /**
     * refresh Recording file list
     */
    private void reNewRecordingFileList(String data, String displayName, RecorderItem item) {
        for(int i = 0 ; i < mList.size(); i++){
            RecorderItem recordItem = mList.get(i);
            if(recordItem.data.equals(item.data)){
                recordItem.setData(data);
                recordItem.setDisplayName(displayName);
                mList.set(i , recordItem);
                break;
            }
        }
        mAdapter.notifyDataSetChanged();
    }

    /**
     * rename the file
     */
    private String renameRecordingFile(String data, String extenison , String title) {
        File renameFile = new File(data);
        String newPath = renameFile.getParent() + File.separator + title + extenison;
        File file = new File(newPath);
        if (file.exists() && !file.isDirectory()) {
            return DUPLICATE_NAME;
        }
        boolean result = false;
        try{
            result = renameFile.renameTo(new File(newPath));
        }catch(Exception e){
            Log.w(TAG , "when rename recording file,renameTo() error");
        }
        mAdapter.notifyDataSetChanged();
        if(result == true){
            return newPath;
        }else{
            return null;
        }
    }

    private void renameDialog(final RecorderItem item) {
        final EditText editText = new EditText(this);
        final String displayName = item.getDisplayName();
        final String extension = displayName.substring(displayName.lastIndexOf("."), displayName.length());
        final String diaplayNameExceptExtension = displayName.substring(0 , displayName.lastIndexOf("."));
        editText.setText(diaplayNameExceptExtension);
        editText.setFilters(new InputFilter[]{new InputFilter.LengthFilter(INPUT_MAX_LENGTH)});
        editText.addTextChangedListener(this);
        editText.requestFocus();
        final AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(R.string.rename).setView(editText)
                .setPositiveButton(R.string.save, new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        String fileName = editText.getEditableText().toString().trim();
                        String specialChar = stringFilter(fileName);
                        String emojiChar = EmojiUtil.filterEmoji(fileName);//bug 618348 the file suffix format display is not complete
                        if(!specialChar.isEmpty()){
                            Toast.makeText(getApplicationContext(), String.format(getResources().
                                    getString(R.string.special_char_exist), specialChar),
                                    Toast.LENGTH_LONG).show();
                        }else if (fileName.equals(diaplayNameExceptExtension)) {
                            Toast.makeText(getApplicationContext(), R.string.filename_is_not_modified,
                                    Toast.LENGTH_SHORT).show();
                        } else if(TextUtils.isEmpty(fileName)){
                            Toast.makeText(getApplicationContext(), R.string.filename_empty_error,
                                    Toast.LENGTH_SHORT).show();
                        }else if(EmojiUtil.containsEmoji(fileName)){//bug 618348 the file suffix format display is not complete
                            Toast.makeText(getApplicationContext(), String.format(getResources().
                                    getString(R.string.special_char_exist), emojiChar),
                                    Toast.LENGTH_SHORT).show();
                        }else {
                            String newPath = renameRecordingFile(item.data, extension , fileName);
                            if (newPath == null) {
                                Toast.makeText(getApplicationContext() , R.string.rename_nosave,
                                        Toast.LENGTH_SHORT).show();
                                return;
                            }else if (newPath.equals(DUPLICATE_NAME)) {
                                Toast.makeText(getApplicationContext() , R.string.duplicate_name,
                                        Toast.LENGTH_SHORT).show();
                                return;
                            }
                            String newDisplayName = fileName + extension;
                            int result = updateRecordingFileDB(newPath , fileName, newDisplayName, item);
                            if (result > 0) {
                                reNewRecordingFileList(newPath, newDisplayName, item);
                                mDBHelper.update(diaplayNameExceptExtension,fileName);
                                mAdapter.renameById(item.id, fileName);
                                Toast.makeText(getApplicationContext(), R.string.rename_save,
                                        Toast.LENGTH_SHORT).show();
                            } else {
                                renameRecordingFile(newPath, extension , diaplayNameExceptExtension);
                                Toast.makeText(getApplicationContext(), R.string.rename_nosave,
                                        Toast.LENGTH_SHORT).show();
                            }
                        }
                        dialog.dismiss();
                    }

                }).setNegativeButton(R.string.button_cancel, new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                }).create().show();
    }
    private String stringFilter(String str){
        String filter = "[/\\\\<>:*?|\"\n\t]";
        Pattern pattern = Pattern.compile(filter);
        Matcher matcher = pattern.matcher(str);
        StringBuffer buffer= new StringBuffer();
        boolean result = matcher.find();
        while(result){
            buffer.append(matcher.group());
            result = matcher.find();
        }
        return buffer.toString();
    }
    private void checkboxOnclick(int pos){
        Boolean result = checkboxes.get(pos);
        if(result == null || result == false){
            checkboxes.put(pos, true);
            RecorderItem item = mAdapter.findItem(pos);
            checkItem.put(pos,item);
        }else{
            checkboxes.put(pos, false);
            checkItem.remove(pos);
        }
        buttonClickableChanges();
    }

    private void invalidateCheckbox(CheckBox box,int pos,LinearLayout item_lay){
        Boolean result = checkboxes.get(pos);
        if(result == null || result == false){
            box.setChecked(false);
            item_lay.setBackgroundColor(Color.parseColor("#ffffff"));
        }else{
            RecorderItem item = mAdapter.findItem(pos);
            checkItem.put(pos, item);
            box.setChecked(true);
            item_lay.setBackgroundColor(Color.parseColor("#E3F3F3"));
        }
    }

    private void unSelectAll() {
        clearContainer();
        mListView.invalidateViews();
    }

    private void selectAll(){
        if(items != null){
            items.clear();
        }
        if(checkItem != null){
            checkItem.clear();
        }
        Integer index = 0;
        for(RecorderItem item:mList){
            checkboxes.put(index,true);
            checkItem.put(index,item);
            index++;
        }
        mListView.invalidateViews();
    }

    private void clearContainer() {
        if (items != null) {
            items.clear();
        }
        if (checkboxes != null) {
            checkboxes.clear();
        }
        if (checkItem != null) {
            checkItem.clear();
        }
    }
    /** SPRD:Bug 610511 com.android.soundrecorder happens ANR @{ */
    private void queryFileAysnc() {
        AsyncTask<Void, Long, Void> task = new AsyncTask<Void, Long, Void>() {

            @Override
            protected Void doInBackground(Void... params) {
                  mList = query();
                  return null;
            }

            @Override
            protected void onProgressUpdate(Long... values) {

            }

            @Override
            protected void onPostExecute(Void result) {
            if (!mIsPause){
                Collections.reverse(mList);
                mAdapter = new CursorRecorderAdapter(mList, mActionMode != null ? false : true);
                setListAdapter(mAdapter);
                Log.i(TAG,"onResume update mAdapter.");
                detectionList();

            if (mActionMode != null) {
                int checkedCount = 0;
                if (mList.size() == 0) {
                    mActionMode.finish();
                    return;
                }
                for (int i = 0; i < mList.size(); i++) {
                    if (checkboxes.get(i) != null && checkboxes.get(i) == true) {
                        checkItem.put(i,mList.get(i));
                        checkedCount++;
                    }
                }
                mActionMode.setTitle(String.valueOf(checkedCount));
                buttonClickableChanges();
            }
            }
            }
        };
        task.execute((Void[])null);
    }
    /** @} */
    private void deleteFileAysnc() {
        /*SPRD fix bug 543825&386249,FATAL EXCEPTION occurs when delete files @{*/
        AsyncTask<Void, Long, Void> task = new AsyncTask<Void, Long, Void>() {
            ProgressDialog pd = null;
            private Map<Integer,RecorderItem> deleteItem = new TreeMap<Integer,RecorderItem>(checkItem);

            @Override
            protected void onPreExecute() {
                pd = new ProgressDialog(RecordingFileList.this);
                pd.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
                pd.setIcon(android.R.drawable.ic_delete);
                pd.setTitle(R.string.recording_file_delete_alert_title);
                pd.setCancelable(false);
                pd.setMax(deleteItem.size());
                pd.show();
            }

            @Override
            protected Void doInBackground(Void... params) {
                long i = 0;
                for (Map.Entry<Integer, RecorderItem> entry : deleteItem.entrySet()) {
                    RecorderItem item = entry.getValue();
                    if(item != null){
                        deleteFile(item);
                        publishProgress(++i, item.id);
                    }
                }
                Log.i(TAG,"delete finished.");
                return null;
            }

            @Override
            protected void onProgressUpdate(Long... values) {
                pd.setProgress(values[0].intValue());
                if(mAdapter != null){
                    mAdapter.deleteById(values[1]);
                }
            }

            @Override
            protected void onPostExecute(Void result) {
                if(!isFinishOrDestroy()){
                    pd.cancel();
                }
                deleteFinish();
                dismissDelDialog();
            }
        };
        task.execute((Void[])null);
    }

    private boolean isFinishOrDestroy(){
        if(RecordingFileList.this.isDestroyed() || RecordingFileList.this.isFinishing()){
            return true;
        }
        return false;
    }
    /*SPRD fix bug 543825&386249,FATAL EXCEPTION occurs when delete files @}*/

    private void deleteFinish(){
//      if(mActionMode != null){
//          mActionMode.finish();
//      }
        clearContainer();
        Log.i(TAG,"deleteFinish and notify mAdapter");
        mListView.setVisibility(View.GONE);
        mAdapter.notifyDataSetChanged();
        mListView.setVisibility(View.VISIBLE);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
        //modify by linying bug115502 begin
        detectionList();
        //modify by linying bug115502 end
        if(mActionMode != null){
            mActionMode.finish();
        }
        Toast.makeText(RecordingFileList.this,R.string.recording_file_delete_success, Toast.LENGTH_SHORT).show();
    }

    private String [] cloum = new String[]{"_id,_data"};

    private AlertDialog mDelDlg = null;

    private void showDelDialog() {
        if (mDelDlg == null) {
            mDelDlg = new AlertDialog.Builder(this)
                    .setTitle(getString(android.R.string.dialog_alert_title))
                    .setIconAttribute(android.R.attr.alertDialogIcon)
                    .setMessage(getString(R.string.confirm_del))
                    .setPositiveButton(android.R.string.ok,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    deleteFileAysnc();
                                }
                            })
                    .setNegativeButton(android.R.string.cancel,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    dismissDelDialog();
                                }
                            }).create();
        }
        mDelDlg.show();
    }

    private void dismissDelDialog() {
        if (mDelDlg != null) {
            mDelDlg.dismiss();
            mDelDlg.cancel();
            mDelDlg = null;
        }
    }

    private void deleteFile(RecorderItem item) {
        if (StorageInfos.isExternalStorageMounted() || !StorageInfos.isInternalStorageSupported()) {
            getContentResolver().delete(ContentUris.withAppendedId(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, item.id), null,null);
        } else {
            getContentResolver().delete(ContentUris.withAppendedId(MediaStore.Audio.Media.INTERNAL_CONTENT_URI, item.id), null,null);
        }
        try {
            mDBHelper.delete(item.title);
        } catch (Exception e) {
            Log.e(TAG, "delete error: " + e.getMessage());
        }
        File del = new File(item.data);
        if (!del.exists() || !del.delete()) {
            return;
        }
        // SPRD: bug594656 Select all size is wrong after delete some files.
        mList.remove(item);
    }

    @Override
    public void onItemClick(AdapterView<?> adapter, View v, int pos, long id) {
        if (mActionMode==null) {
            if (mPlayer != null) {
                stopPlayback();
            }
            RecorderItem item = mAdapter.findItem(pos);
            Intent intent = new Intent(RecordingFileList.this, RecordingFilePlay.class);
            intent.putExtra("title", item.title);
            intent.putExtra("id", item.id);
            intent.putExtra("duration", item.duration);
            startActivity(intent);
        }else{
            CheckBox checkBox=(CheckBox)v.findViewById(R.id.recode_checkbox);
            if (null==checkBox) {
                return;
            }
            LinearLayout item_lay = (LinearLayout)v.findViewById(R.id.record_item_container);
            if (item_lay == null) {
                return;
            }
            if (true==checkBox.isChecked()) {
                checkBox.setChecked(false);
                item_lay.setBackgroundColor(Color.parseColor("#ffffff"));
            }else{
                item_lay.setBackgroundColor(Color.parseColor("#E3F3F3"));
                checkBox.setChecked(true);
            }
            checkboxOnclick(pos);
        }
    }

    private class RecorderItemClick
                implements DialogInterface.OnClickListener {

        private static final int LONG_CLICK  = 1;
        private static final int SHORT_CLICK = 2;
        private final int event;
        private final RecorderItem item;

        private RecorderItemClick(int event, RecorderItem item) {
            this.item = item;
            this.event = event;
        }

        @Override
        public void onClick(DialogInterface dialog, int which) {
            if (SHORT_CLICK == -which) {
                dialog.dismiss();
                return;
            }

            int row = -1;
            StringBuffer buff = new StringBuffer();
            buff.append(MediaStore.Audio.Media._ID).append("=").append(item.id);
            int toast_msg = -1;
            try {
                // delete database row
                if (StorageInfos.isExternalStorageMounted() || !StorageInfos.isInternalStorageSupported()) {
                    row = RecordingFileList.this.getContentResolver().delete(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, buff.toString(), null);
                } else {
                    row = RecordingFileList.this.getContentResolver().delete(MediaStore.Audio.Media.INTERNAL_CONTENT_URI, buff.toString(), null);
                }
                // validate database process
                if (row == -1) {
                    toast_msg = R.string.recording_file_database_failed;
                    return;
                }
                // validate file process
                File del = new File(item.data);
                if (!del.exists() || !del.delete()) {
                    toast_msg = R.string.recording_file_delete_failed;
                    return;
                }

                toast_msg = R.string.recording_file_delete_success;
            } catch (Exception e) {
                if (row == -1) toast_msg = R.string.recording_file_database_failed;
                Log.d(TAG, "execute delete recorder item failed; E: " + (e != null ? e.getMessage() : "NULL"));
            } finally {
                mAdapter.deleteById((row != -1 ? item.id : 0));
                Log.d(TAG, "mAdapter deleteItem row:" + row);
                clearContainer();
                Toast.makeText(RecordingFileList.this, toast_msg, Toast.LENGTH_SHORT).show();
                //modify by linying bug115502 begin
                detectionList();
                //modify by linying bug115502 end
            }
        }

        void show() {
            if (event == 0 || item == null)
                throw new RuntimeException("RecorderItemClick failed; event == " + event + " --- item == " + item);

            new AlertDialog.Builder(RecordingFileList.this)
                .setTitle(getString(android.R.string.dialog_alert_title))
                .setIconAttribute(android.R.attr.alertDialogIcon)
                .setMessage(item.getAlertMessage())
                .setPositiveButton(R.string.button_delete, this)
                .setNegativeButton(R.string.button_cancel, this)
                .show();
        }
    }

    private class CursorRecorderAdapter extends BaseAdapter {
        private List<RecorderItem> mData = new ArrayList<RecorderItem>();
        private boolean hiddenFlag = true;
        CursorRecorderAdapter(List<RecorderItem> data , boolean hiddenFlag) {
            super();
            this.mData.clear();
            this.mData.addAll(data);
        }

        @Override
        public int getCount() {
            synchronized (mData) {
                return mData == null ? 0 : mData.size();
            }
        }

        @Override
        public Object getItem(int pos) {
            return mData.get(pos);
        }

        @Override
        public long getItemId(int pos) {
            long result = -1L;
            RecorderItem item = findItem(pos);
            if (item != null) result = item.id;
            return result;
        }
       //bug 613017 Play the file icon will appear on another file or will not appear play
       private Uri mPlayingUri;
       private void getItemViews(View v){
           mPlayingItem = v;
           mPlayLayout = (LinearLayout) mPlayingItem.findViewById(R.id.record_start);
           mCurItemLayout = (RelativeLayout) mPlayingItem.findViewById(R.id.relative_container);
           mCurPlayButton = (ImageView) mPlayingItem.findViewById(R.id.recode_icon);
           mCurSeekBar = (MarkSeekBar) mPlayingItem.findViewById(R.id.progress);
           mCurTagIcon = (ImageView) mPlayingItem.findViewById(R.id.tag_icon);
           mCurRecordDuration = (TextView) mPlayingItem.findViewById(R.id.record_duration);
       }
       //bug 613017 end
        @Override
        public View getView(int pos, View cvt, ViewGroup pat) {
            if (cvt == null) {
                LayoutInflater flater =
                    (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                cvt = flater.inflate(R.layout.recording_file_item, null);
                if (cvt == null)
                    throw new RuntimeException("inflater \"record_item.xml\" failed; pos == " + pos);
            }
            //bug 613017 Play the file icon will appear on another file or will not appear play
            updatePlayItemInvisible(cvt);
            RecorderItem item = findItem(pos);
            if (item == null) throw new RuntimeException("findItem() failed; pos == " + pos);
            Uri nowUri = ContentUris.withAppendedId(
                MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,item.id);
            if (mPlayer != null && nowUri.equals(mPlayingUri)) {
                Log.d(TAG,"Now playing is "+mPlayingUri);
                getItemViews(cvt);
                updatePlayItemVisible();
            }
            //bug 613017 end
            TextView tv = null;
            tv = (TextView) cvt.findViewById(R.id.record_displayname);
            tv.setText(item.display_name);
            tv = (TextView) cvt.findViewById(R.id.record_duration);
            tv.setText(Utils.makeTimeString4MillSec(RecordingFileList.this, item.duration));

            ImageView iv = (ImageView) cvt.findViewById(R.id.tag_icon);
            if (item.tagNumber > 0) {
                iv.setBackgroundResource(R.drawable.tag_list);
            } else {
                iv.setBackgroundResource(R.color.transparent);
            }
            /* SPRD: add new feature @{ */
            LinearLayout lay = (LinearLayout) cvt.findViewById(R.id.record_start);
            lay.setContentDescription(getResources().getString(R.string.start_play));
            lay.setTag(pos);
            lay.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    View parent = (View) v.getParent();
                    int pos = (Integer) v.getTag();
                    if (mActionMode != null) {
                        stopPlayback();
                        CheckBox checkBox=(CheckBox)parent.findViewById(R.id.recode_checkbox);
                        if (null==checkBox) {
                            return;
                        }
                        LinearLayout item_lay = (LinearLayout)parent.findViewById(R.id.record_item_container);
                        if (item_lay == null) {
                            return;
                        }
                        if (true==checkBox.isChecked()) {
                            checkBox.setChecked(false);
                            item_lay.setBackgroundColor(Color.parseColor("#ffffff"));
                        }else{
                            item_lay.setBackgroundColor(Color.parseColor("#E3F3F3"));
                            checkBox.setChecked(true);
                        }
                        checkboxOnclick(pos);
                    }else {
                        if (pos != mLastClickPos && mPlayer != null) {
                            mLastClickPos = pos;
                            updatePlayStop();
                            mPlayer.release();
                            mPlayer = null;
                        }
                        mLastClickPos = pos;
                        if (mPlayer == null) {
                            // SPRD: bug598175 Recod item show seekbar when play a record during a call.
                            if(!requestAudioFocus()){
                                Toast.makeText(RecordingFileList.this, R.string.no_allow_play_calling, Toast.LENGTH_SHORT).show();
                                return;
                            }
                            getItemViews(parent);//bug 613017 Play the file icon will appear on another file or will not appear play
                            mCurItemLayout.setVisibility(View.GONE);
                            mCurSeekBar.setVisibility(View.VISIBLE);
                            mCurTagIcon.setVisibility(View.GONE);
                            if (mPlayer == null) {
                                mPlayer = new PreviewPlayer();
                                mPlayer.setActivity(RecordingFileList.this);
                                try {
                                    RecorderItem item = mAdapter.findItem(pos);
                                    Uri mUri = ContentUris
                                            .withAppendedId(
                                                    MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
                                                    item.id);
                                    Cursor cursor = mDBHelper.queryTag(item.title);
                                    mTagHashMap = new HashMap<Integer,Integer>();
                                    while(cursor.moveToNext()) {
                                        int tag = cursor.getInt(0);
                                        int location = cursor.getInt(1);
                                        mTagHashMap.put(tag, location);
                                    }
                                    cursor.close();
                                    mPlayingUri = mUri;
                                    mPlayer.setDataSourceAndPrepare(mUri);
                                    mPlayer.setUISeekBar(mCurSeekBar);
                                } catch (Exception ex) {
                                    Log.d(TAG, "Failed to open file: " + ex);
                                    Toast.makeText(RecordingFileList.this,
                                            R.string.playback_failed,
                                            Toast.LENGTH_SHORT).show();
                                    return;
                                }
                            }
                        } else {
                            if (mPlayer.isPlaying()) {
                                mPlayer.pause();
                            } else {
                                start();
                            }
                            updatePlayPause();
                        }
                    }
                }
            });

            LinearLayout item_lay = (LinearLayout)cvt.findViewById(R.id.record_item_container);
            item_lay.setTag(pos);
            /* @} */
            CheckBox cb = (CheckBox)cvt.findViewById(R.id.recode_checkbox);
            cb.setTag(pos);
            invalidateCheckbox(cb,pos,item_lay);
            if(mActionMode != null){
                cb.setVisibility(View.GONE);
            }else{
                cb.setVisibility(View.GONE);
            }
            return cvt;
        }
        public void setCheckboxHidden(boolean flag){
            hiddenFlag = flag;
        }
        private RecorderItem findItem(int pos) {
            RecorderItem result = null;
            Object obj = getItem(pos);
            if (obj != null && obj instanceof RecorderItem) {
                result = (RecorderItem) obj;
            }
            return result;
        }

        /*SPRD fix bug 543825&386249,FATAL EXCEPTION occurs when delete files @{*/
        private void deleteById(long id){
            boolean result = false;
            List<RecorderItem> tmp = new ArrayList<RecorderItem>();
            tmp.addAll(mData);
            Iterator<RecorderItem> it = tmp.iterator();
            while (it.hasNext()) {
                RecorderItem del = it.next();
                if (id == del.id) {
                    it.remove();
                    result = true;
                    break;
                }
            }
            if (result) {
                synchronized (mData) {
                    mData.clear();
                    mData.addAll(tmp);
                    notifyDataSetChanged();
                }
            }
        }
        /*SPRD fix bug 543825&386249,FATAL EXCEPTION occurs when delete files @}*/

        private void renameById(long id, String title){
            Iterator<RecorderItem> it = mData.iterator();
            while (it.hasNext()) {
                RecorderItem item = it.next();
                if (id == item.id) {
                    item.title = title;
                    break;
                }
            }
        }
    }

    private ArrayList<RecorderItem> query() {
        final int INIT_SIZE = 10;
        ArrayList<RecorderItem> result =
            new ArrayList<RecorderItem>(INIT_SIZE);
        Cursor cur = null;
        try {
            StringBuilder where = new StringBuilder();
            /* SPRD: fix bug 513105 @{ */
            SharedPreferences systemVersionShare = this.getSharedPreferences(SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
            String systemVersion = systemVersionShare.getString(RecordingFileList.SYSTEM_VERSION, "0");
            Log.d(TAG, "query(): systemVersion="+systemVersion+", currentVersion="+android.os.Build.VERSION.RELEASE);
            if (systemVersion.equals("0")) {
                /* SPRD: fix bug 521597 @{ */
                File pathDir = null;
                String defaultExternalPath = "0";
                String defaultInternalPath = "0";
                if(StorageInfos.isExternalStorageMounted()) {
                    pathDir = StorageInfos.getExternalStorageDirectory();
                    if (pathDir != null)
                        defaultExternalPath = pathDir.getPath() + Recorder.DEFAULT_STORE_SUBDIR;
                }
                pathDir = StorageInfos.getInternalStorageDirectory();
                if (pathDir != null)
                    defaultInternalPath = pathDir.getPath() + Recorder.DEFAULT_STORE_SUBDIR;

                Log.d(TAG, "query(): defaultExternalPath="+defaultExternalPath+", defaultInternalPath="+defaultInternalPath);

                where.append("(")
                .append(MediaStore.Audio.Media.MIME_TYPE)
                .append("='")
                .append(SoundRecorder.AUDIO_AMR)
                .append("' or ")
                .append(MediaStore.Audio.Media.MIME_TYPE)
                .append("='")
                .append(SoundRecorder.AUDIO_3GPP)
                .append("' or ")
                .append(MediaStore.Audio.Media.MIME_TYPE)
                .append("='")
                .append(SoundRecorder.AUDIO_MP4)
                .append("') and (")
                .append(MediaStore.Audio.Media.DATA)
                .append(" like '")
                .append(defaultExternalPath)
                .append("%' or ")
                .append(MediaStore.Audio.Media.DATA)
                .append(" like '")
                .append(defaultInternalPath)
                .append("%')");
                /* @} */
            } else {
                where.append(MediaStore.Audio.Media.COMPOSER)
                .append("='")
                .append(SoundRecorder.COMPOSER)
                .append("'");
            }
            /* @} */

            cur = RecordingFileList.this.getContentResolver().query(
                    MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
                    new String[] {
                        RecorderItem._ID,
                        RecorderItem._DATA,
                        RecorderItem.SIZE,
                        RecorderItem.TITLE,
                        RecorderItem.DISPLAY_NAME,
                        RecorderItem.MOD_DATE,
                        RecorderItem.MIME_TYPE,
                        RecorderItem.DU_STRING,
                        RecorderItem.TAG_NUMBER},
                        where.toString(), null, null);

            // read cursor
            int index = -1;
            for (cur.moveToFirst(); !cur.isAfterLast(); cur.moveToNext()) {
                index = cur.getColumnIndex(RecorderItem._ID);
                // create recorder object
                long id = cur.getLong(index);
                RecorderItem item = new RecorderItem(id);
                // set "data" value
                index = cur.getColumnIndex(RecorderItem._DATA);
                item.data = cur.getString(index);
                // set "size" value
                index = cur.getColumnIndex(RecorderItem.SIZE);
                item.size = cur.getLong(index);
                // set "title" value
                index = cur.getColumnIndex(RecorderItem.TITLE);
                item.title = cur.getString(index);
                // SET "display name" value
                index = cur.getColumnIndex(RecorderItem.DISPLAY_NAME);
                item.display_name = cur.getString(index);
                // set "time" value
                index = cur.getColumnIndex(RecorderItem.MOD_DATE);
                item.time = cur.getLong(index);
                // set "mime-type" value
                index = cur.getColumnIndex(RecorderItem.MIME_TYPE);
                item.mimeType = cur.getString(index);
                // add to mData
                index = cur.getColumnIndex(RecorderItem.DU_STRING);
                item.duration = cur.getInt(index);
                Log.w("mytest", "duration:"+item.duration);
                index = cur.getColumnIndex(RecorderItem.TAG_NUMBER);
                item.tagNumber = cur.getInt(index);
                /* SPRD: fix bug 513105 @{ */
                Log.d(TAG, "query(): item is "+item.toString());
                if (!item.data.endsWith(".3gpp") && item.mimeType.equals(SoundRecorder.AUDIO_MP4))
                    continue;
                result.add(item);
                mRecordIDList.add(cur.getInt(0));
                /* @} */
            }
            if (systemVersion.equals("0") && mRecordIDList.size() > 0) {
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        ContentValues cv = new ContentValues();
                        cv.put(MediaStore.Audio.Media.COMPOSER, SoundRecorder.COMPOSER);
                        ContentResolver resolver = getContentResolver();
                        for (int i = 0 ; i < mRecordIDList.size(); i++) {
                            Uri uri = ContentUris.withAppendedId(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, mRecordIDList.get(i));
                            Log.d(TAG, "query(): update COMPOSER to MediaStore, id="+mRecordIDList.get(i));
                            resolver.update(uri, cv, null, null);
                        }
                    }
                }).start();
            }
            /* SPRD: fix bug 513105 @{ */
            if (!systemVersion.equals(android.os.Build.VERSION.RELEASE)) {
                SharedPreferences.Editor edit = systemVersionShare.edit();
                edit.putString(RecordingFileList.SYSTEM_VERSION, android.os.Build.VERSION.RELEASE);
                edit.commit();
            }
            /* @} */
        } catch (Exception e) {
            Log.v(TAG, "RecordingFileList.CursorRecorderAdapter failed; E: " + e);
        } finally {
            if (cur != null) cur.close();
        }
        return result;
    }
    @SuppressWarnings("unused")
    private class RecorderItem {
        private final long id;
        private String data;
        private String mimeType;
        private long size;
        private String title;
        private String display_name;
        private long time;
        private int duration;
        private int tagNumber;

        private static final String _ID         = MediaStore.Audio.Media._ID;
        private static final String SIZE        = MediaStore.Audio.Media.SIZE;
        private static final String _DATA       = MediaStore.Audio.Media.DATA;
        private static final String TITLE       = MediaStore.Audio.Media.TITLE;
        private static final String DISPLAY_NAME= MediaStore.Audio.Media.DISPLAY_NAME;
        private static final String MOD_DATE    = MediaStore.Audio.Media.DATE_MODIFIED;
        private static final String MIME_TYPE   = MediaStore.Audio.Media.MIME_TYPE;
        private static final String DU_STRING   = MediaStore.Audio.Media.DURATION;
        private static final String TAG_NUMBER   = MediaStore.Audio.Media.BOOKMARK;

        private static final String AUDIO_AMR   = "audio/amr";
        private static final String AUDIO_3GPP  = "audio/3gpp";
        private static final double NUMBER_KB   = 1024D;
        private static final double NUMBER_MB   = NUMBER_KB * NUMBER_KB;

        RecorderItem(long id) {
            this.id = id;
        }

        RecorderItem(long id, String data, String mimeType) {
            this(id);
            this.data = data;
            this.mimeType = mimeType;
        }

        RecorderItem(long id, String data, String mimeType, long size, String title) {
            this(id, data, mimeType);
            this.size = size;
            this.title = title;
        }

        public String getSize() {
            StringBuffer buff = new StringBuffer();
            if (size > 0) {
                String format = null;
                double calculate = -1D;
                if (size < NUMBER_KB) {
                    format = getResources().getString(R.string.list_recorder_item_size_format_b);
                    int calculate_b = (int) size;
                    buff.append(String.format(format, calculate_b));
                } else if (size < NUMBER_MB) {
                    format = getResources().getString(R.string.list_recorder_item_size_format_kb);
                    calculate = (size / NUMBER_KB);
                    DecimalFormat df = new DecimalFormat(".##");
                    String st = df.format(calculate);
                    buff.append(String.format(format, st));
                } else {
                    format = getResources().getString(R.string.list_recorder_item_size_format_mb);
                    calculate = (size / NUMBER_MB);
                    DecimalFormat df = new DecimalFormat(".##");
                    String st = df.format(calculate);
                    buff.append(String.format(format, st));
                }
            }
            return buff.toString();
        }

        /**
         * get the file's name
         * @return file's name
         */
        public String getDisplayName() {
            return display_name;
        }
        public void setDisplayName(String displayName) {
            this.display_name = displayName;
        }

        public void setData(String data) {
            this.data = data;
        }

        public String getData(){
            return data;
        }

        /**
         * get the file's id
         * @return file's id
         */
        public long getId() {
            return id;
        }
        /* @} */
        /**
         * get last file modify date
         * @return last file modify date
         */
        public String getDate() {
            if (time > 0) {
                java.util.Date d = new java.util.Date(time * 1000);
                java.text.DateFormat formatter_date =
                    java.text.DateFormat.getDateInstance();
                return formatter_date.format(d);
            }
            return null;
        }

        /**
         * get last file modify time
         * @return modify time
         */
        public String getTime() {
            if (time > 0) {
                java.util.Date d = new java.util.Date(time * 1000);
                java.text.DateFormat formatter_time =
                    java.text.DateFormat.getTimeInstance();
                return formatter_time.format(d);
            }
            return null;
        }

        public String getAlertMessage() {
            String msg =
                getResources().getString(R.string.recording_file_delete_alert_message);
            String result = String.format(msg, (display_name != null ? display_name : ""));
            return result;
        }

        @Override
        public String toString() {
            StringBuffer buff = new StringBuffer();
            buff.append("id == ").append(id)
                .append(" --- data == ").append(data)
                .append(" --- mimeType == ").append(mimeType)
                .append(" --- size == ").append(size)
                .append(" --- title == ").append(title)
                .append(" --- display_name == ").append(display_name)
                .append(" --- time == ").append(time)
                .append(" --- duration == ").append(duration);
            return buff.toString();
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
       if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (mPlayer != null) {
                stopPlayback();
                return true;
            }
            finish();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    /**
     * detection list items count, if the count less than 1, show the remind textview
     */
    private void detectionList() {
        if (mAdapter.getCount() < 1) {
            remindTextView.setVisibility(View.VISIBLE);
        } else {
            remindTextView.setVisibility(View.GONE);
        }
    }

    /* SPRD:check the length when rename@{ */
    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
    }
    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) {
        if(s.toString().length() >= INPUT_MAX_LENGTH){
            Toast.makeText(this, R.string.input_length_overstep, Toast.LENGTH_SHORT).show();
        }
    }
    @Override
    public void afterTextChanged(Editable s) {
    }
    /* @} */

    public void fileDelete(View v) {
        if (checkItem != null && checkItem.size() == 0) {
            (Toast.makeText(this, this.getResources().getString(R.string.choose_file),
                    Toast.LENGTH_SHORT)).show();
            return;
        }
        showDelDialog();
    }

    public void fileRename(View v) {
        if (checkItem.size() == 0 || checkItem.size() > 1) {
            return;
        }
        for (Map.Entry<Integer, RecorderItem> entry : checkItem.entrySet()) {
            renameDialog(entry.getValue());
        }
        mActionMode.finish();
    }

    public void filePath(View v) {
        if (checkItem.size() == 0 || checkItem.size() > 1) {
            return;
        }
        Resources res = getResources();
        for (Map.Entry<Integer, RecorderItem> entry : checkItem.entrySet()) {
            LayoutInflater flater =
                    (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            View filepathView = flater.inflate(R.layout.recording_file_details, null);
            TextView tvName = (TextView)filepathView.findViewById(R.id.file_name_value);
            String fileName = entry.getValue().getDisplayName();
            tvName.setText(fileName);
            TextView tvPath = (TextView)filepathView.findViewById(R.id.file_path_value);
            String absolutePath =entry.getValue().getData();
            String filePath = absolutePath.substring(0,absolutePath.lastIndexOf('/'));
            tvPath.setText(filePath);

            TextView tvSize = (TextView)filepathView.findViewById(R.id.file_size_value);
            String fileSize = entry.getValue().getSize();
            tvSize.setText(fileSize);
            TextView tvDate = (TextView)filepathView.findViewById(R.id.file_date_value);
            String fileDate = entry.getValue().getDate()+" "+entry.getValue().getTime();
            tvDate.setText(fileDate);
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setView(filepathView).create();
            builder.setNegativeButton(R.string.button_cancel,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        dismissDelDialog();
                    }
                });
            builder.show();
        }
        mActionMode.finish();
    }

    public void onClick(View button){
        if (!button.isEnabled()){
            return;
            }      
        switch (button.getId()) {
            case R.id.textview_file_rename:
                fileRename(null);
                break;
            case R.id.textview_file_delete:
                fileDelete(null);
                break;
            case R.id.textview_file_path:
                filePath(null);
                break;
            }        
    }
    /* SPRD: fix bug 516659 @{ */
    @Override
    public void onBackPressed() {
        if (isResumed()) {
            super.onBackPressed();
        }
    }
    /* @} */

    /* SPRD: add new feature @{ */
    private Handler mPlayerHandler = new Handler() {
        float mCurrentVolume = 1.0f;
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            Log.d(TAG, "handleMessage msg.what = "+msg.what);
            switch (msg.what) {
                case FADEDOWN:
                    mCurrentVolume -= .05f;
                    if (mCurrentVolume > .2f) {
                        mPlayerHandler.sendEmptyMessageDelayed(FADEDOWN, 10);
                    } else {
                        mCurrentVolume = .2f;
                    }
                    mPlayer.setVolume(mCurrentVolume);
                    break;
                case FADEUP:
                    mCurrentVolume += .01f;
                    if (mCurrentVolume < 1.0f) {
                        mPlayerHandler.sendEmptyMessageDelayed(FADEUP, 10);
                    } else {
                        mCurrentVolume = 1.0f;
                    }
                    mPlayer.setVolume(mCurrentVolume);
                    break;
            }
        }
    };

    private boolean requestAudioFocus() {
        int audioFocus = mAudioManager.requestAudioFocus(mAudioFocusListener, AudioManager.STREAM_MUSIC,
                             AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
        if(audioFocus == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
            return true;
        }
        return false;
    }

    private OnAudioFocusChangeListener mAudioFocusListener = new OnAudioFocusChangeListener() {
        public void onAudioFocusChange(int focusChange) {
            Log.d(TAG, "onAudioFocusChange focusChange = "+focusChange);
            if (mPlayer == null) {
                mAudioManager.abandonAudioFocus(this);
                return;
            }
            switch (focusChange) {
                case AudioManager.AUDIOFOCUS_LOSS:
                    mPausedByTransientLossOfFocus = false;
                    mPlayer.pause();
                    break;
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                    if (mPlayer.isPlaying()) {
                        mPausedByTransientLossOfFocus = true;
                        mPlayer.pause();
                    }
                    break;
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                    mPlayFromFocusCanDuck = true;
                    mPlayerHandler.removeMessages(FADEUP);
                    mPlayerHandler.sendEmptyMessage(FADEDOWN);
                    break;
                case AudioManager.AUDIOFOCUS_GAIN:
                    if (mPausedByTransientLossOfFocus) {
                        mPausedByTransientLossOfFocus = false;
                        start();
                    }
                    if (mPlayFromFocusCanDuck) {
                        mPlayFromFocusCanDuck = false;
                        mPlayerHandler.removeMessages(FADEDOWN);
                        mPlayerHandler.sendEmptyMessage(FADEUP);
                    }
                    break;
            }
            updatePlayPause();
        }
    };

    public void start() {
        if(!requestAudioFocus()){
            Toast.makeText(this.getApplicationContext(), R.string.no_allow_play_calling, Toast.LENGTH_SHORT).show();
            return;
        }
        mPlayer.start();
        if (mProgressRefresher != null) {
            mProgressRefresher.removeCallbacksAndMessages(null);
            mProgressRefresher.post(new ProgressRefresher());
        }
    }

    class ProgressRefresher implements Runnable {
        @Override
        public void run() {
            if (mPlayer != null && !mPlayer.mSeeking && mPlayer.mDuration != 0) {
                int currentTime = mPlayer.getCurrentPosition();
                mPlayer.mSeekBar.setDuration(mPlayer.mDuration);
                mPlayer.mSeekBar.setProgress(currentTime);
                updatePlayedDuration(currentTime);
            }
            mProgressRefresher.removeCallbacksAndMessages(null);
            if (mPlayer != null) {
                mProgressRefresher.postDelayed(new ProgressRefresher(), 200);
            }
        }
    }

    public void updatePlayedDuration(int currentTime){
        int second =  Math.round((float)currentTime/1000);
        if (second != 0) {
            mCurRecordDuration.setText(Utils.makeTimeString4MillSec(RecordingFileList.this, currentTime));
        } else {
            mCurRecordDuration.setText(String.format(getResources().getString(R.string.timer_format), 0, 0, 0));
        }
    }

    public void updatePlayPause() {
        ImageView b = mCurPlayButton;
        if (b != null && mPlayer != null) {
            if (mPlayer.isPlaying()) {
                b.setImageResource(R.drawable.audio_suspended_image);
                mPlayLayout.setContentDescription(getResources().getString(R.string.pause));
            } else {
                b.setImageResource(R.drawable.audio_play_image);
                mPlayLayout.setContentDescription(getResources().getString(R.string.resume_play));
                mProgressRefresher.removeCallbacksAndMessages(null);
            }
        }
    }

    public void updatePlayStop() {
        mCurItemLayout.setVisibility(View.VISIBLE);
        mCurSeekBar.setProgress(0);
        mCurSeekBar.setVisibility(View.GONE);
        mCurTagIcon.setVisibility(View.VISIBLE);
        mCurPlayButton.setImageResource(R.drawable.audio_play_image);
        mPlayLayout.setContentDescription(getResources().getString(R.string.start_play));
        if (mPlayer.isPrepared()) {
            mCurRecordDuration.setText(Utils.makeTimeString4MillSec(RecordingFileList.this, mPlayer.mDuration));
        }
        mProgressRefresher.removeCallbacksAndMessages(null);
    }

    public void playPauseClicked() {
        if (mPlayer == null) {
            return;
        }
        if (mPlayer.isPlaying()) {
            mPlayer.pause();
        } else {
            start();
        }
        updatePlayPause();
    }

    private void stopPlayback() {
        if (mProgressRefresher != null) {
            mProgressRefresher.removeCallbacksAndMessages(null);
        }
        if (mPlayer != null) {
            updatePlayStop();
            mPlayer.release();
            mPlayer = null;
            mAudioManager.abandonAudioFocus(mAudioFocusListener);
        }
    }

    public void onCompletion() {
        stopPlayback();
    }

    public void shareFiles(){
        if (checkItem.size() <= 0) {
            return;
        }
        Intent intent = null;
        RecorderItem Ritem = null;
        File file = null;
        if (checkItem.size() > 1) {
            intent = new Intent(Intent.ACTION_SEND_MULTIPLE);
            ArrayList<Uri> sharedUris = new ArrayList<Uri>();
            for (Map.Entry<Integer, RecorderItem> entry : checkItem.entrySet()) {
                Ritem = entry.getValue();
                file = new File(Ritem.data);
                Uri uri = Uri.parse("file://" + file.getPath());
                sharedUris.add(uri);
            }
            String type = getShareTypeByFile(file);
            intent.setType(type);
            intent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, sharedUris);
        } else if(checkItem.size() == 1){
            intent = new Intent(Intent.ACTION_SEND);
            for (Map.Entry<Integer, RecorderItem> entry : checkItem.entrySet()) {
                Ritem = entry.getValue();
            }
            file = new File(Ritem.data);
            String type = getShareTypeByFile(file);
            Uri uri = Uri.parse("file://" + file.getPath());
            intent.setType(type);
            intent.putExtra(Intent.EXTRA_STREAM, uri);
        }
        startActivity(Intent.createChooser(intent,RecordingFileList.this.getResources().getString(R.string.operate_share)));
        mActionMode.finish();
    }

    public String getShareTypeByFile(File file) {
        String suffix = getSuffix(file);
        if (suffix == null) {
            return "file/*";
        }
        if(suffix.equalsIgnoreCase(".3gpp") ){
            return "audio/3gpp";
        }else if(suffix.equalsIgnoreCase(".mp4")){
            return "audio/mp4";
        }else if(suffix.equalsIgnoreCase(".3gp") ){
            return "audio/3gp";
        }else if(suffix.equalsIgnoreCase(".3g2")){
            return "audio/mp4";
        }
        String type = MimeTypeMap.getSingleton().getMimeTypeFromExtension(suffix.substring(1));
        if (type == null || type.isEmpty()) {
            return "file/*";
        }
        return type;
    }

    public String getSuffix(File file) {
        if (file == null || !file.exists() || file.isDirectory()) {
            return null;
        }
        String fileName = file.getName();
        if (fileName.equals("") || fileName.endsWith(".")) {
            return null;
        }
        int index = fileName.lastIndexOf(".");
        if (index != -1) {
            return fileName.substring(index).toLowerCase(Locale.US);
        } else {
            return null;
        }
    }
    /* @} */
    //bug 613017 Play the file icon will appear on another file or will not appear play
    public void updatePlayItemVisible() {
        mCurItemLayout.setVisibility(View.GONE);
        mCurSeekBar.setVisibility(View.VISIBLE);
        mCurTagIcon.setVisibility(View.GONE);
        mPlayer.setScrolSeekBar(mCurSeekBar);
        if (mPlayer !=null && mPlayer.isPlaying()) {
            mCurPlayButton.setImageResource(R.drawable.audio_suspended_image);
        } else {
            mCurPlayButton.setImageResource(R.drawable.audio_play_image);
        }
        if (mProgressRefresher != null) {
            mProgressRefresher.removeCallbacksAndMessages(null);
            mProgressRefresher.post(new ProgressRefresher());
        }
    }
    public void updatePlayItemInvisible(View v) {
        RelativeLayout itemLayout = (RelativeLayout) v
            .findViewById(R.id.relative_container);
        MarkSeekBar seekbar = (MarkSeekBar) v
            .findViewById(R.id.progress);
        ImageView tagIcon = (ImageView) v
            .findViewById(R.id.tag_icon);
        ImageView playButton = (ImageView) v
            .findViewById(R.id.recode_icon);

        itemLayout.setVisibility(View.VISIBLE);
        seekbar.setVisibility(View.GONE);
        tagIcon.setVisibility(View.VISIBLE);
        playButton.setImageResource(R.drawable.audio_play_image);
    }
    //bug 613017 end
    /** SPRD:Bug 615710 reset application preferencesonce again into the tape recorderthe recording list is empty  ( @{ */
    private static final int RECORD_PERMISSIONS_REQUEST_CODE = 200;
        private boolean checkAndBuildPermissions() {
        int numPermissionsToRequest = 0;

        boolean requestMicrophonePermission = false;
        boolean requestStoragePermission = false;
        boolean requestPhoneStatePermission = false;
        if (checkSelfPermission(Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            requestMicrophonePermission = true;
            numPermissionsToRequest++;
        }

        if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestStoragePermission = true;
            numPermissionsToRequest++;
        }

        if (checkSelfPermission(Manifest.permission.READ_PHONE_STATE) != PackageManager.PERMISSION_GRANTED) {
            requestPhoneStatePermission = true;
            numPermissionsToRequest++;
        }

        if (!requestMicrophonePermission && !requestStoragePermission
                && !requestPhoneStatePermission) {
            return false;
        }
        String[] permissionsToRequest = new String[numPermissionsToRequest];
        int permissionsRequestIndex = 0;
        if (requestMicrophonePermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.RECORD_AUDIO;
            permissionsRequestIndex++;
        }
        if (requestStoragePermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.WRITE_EXTERNAL_STORAGE;
            permissionsRequestIndex++;
        }

        if (requestPhoneStatePermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.READ_PHONE_STATE;
        }
        requestPermissions(permissionsToRequest, RECORD_PERMISSIONS_REQUEST_CODE);
        return true;
    }
    /** @} */
}
