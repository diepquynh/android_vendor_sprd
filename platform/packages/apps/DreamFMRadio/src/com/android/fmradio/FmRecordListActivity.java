/**
 * Created By Spreadst
 */

package com.android.fmradio;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;


import android.Manifest;
import android.app.ActionBar;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.graphics.Color;
import android.media.AudioManager;
import android.media.AudioManager.OnAudioFocusChangeListener;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.provider.MediaStore;
import android.provider.MediaStore.Audio.AudioColumns;
import android.provider.MediaStore.Audio.Media;
import android.text.SpannableString;
import android.text.style.ForegroundColorSpan;
import android.util.Log;
import android.view.ActionMode;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.inputmethod.EditorInfo;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.CheckBox;
import android.widget.CursorAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.SearchView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

public class FmRecordListActivity extends Activity implements OnItemClickListener,
        SearchView.OnQueryTextListener {

    /**
     * search FMRecord
     */
    private static final int INDEX_ID = 0;
    private static final int INDEX_DATA = 1;
    private static final int INDEX_DISPLAY_NAME = 2;

    final String[] PROJECTION = new String[] {
            AudioColumns._ID,
            AudioColumns.DATA,
            AudioColumns.DISPLAY_NAME,
    };

    private static final String TAG = "FmRecordListActivity";
    private static final Uri sBaseUri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
    private ListView mListView;
    private LinearLayout mBatchOperationLay;
    private CheckBox mCheckBoxSelectAll;
    private FmRecordAdapter mAdapter = null;
    private Cursor mCursor;
    private String mSelection;
    private String[] mSelectionArgs;
    private Context mContext = null; // application context

    private MenuItem mSearchItem;
    private SearchView mSearchView;
    private ContentResolver mContentResolver;
    private ImageView mRecodeIcon;
    private TextView mTitle;
    private SeekBar mCurSeekBar;
    private FmMediaPalyer mPlayer;
    private Handler mProgressRefresher = new Handler();
    private AudioManager mAudioManager;
    private ActionMode mActionMode = null;
    private Uri mIntentRecordUri;
    private MenuItem deleteButton;
    //SPRD: Add for Bug638498
    private boolean noSearchResult = false;
    // manage checkboxs status
    private HashMap<Integer, Boolean> mCheckboxStatus = new HashMap<Integer, Boolean>();
    private List<RecordDeleteItem> mDeleteItemChecked = new ArrayList<RecordDeleteItem>();
    private List<Integer> mSelectItemDataBaseId = new ArrayList<Integer>();

    private OnAudioFocusChangeListener mAudioFocusListener = new OnAudioFocusChangeListener() {
        public void onAudioFocusChange(int focusChange) {
            if (mPlayer == null) {
                mAudioManager.abandonAudioFocus(this);
                return;
            }
            switch (focusChange) {
                case AudioManager.AUDIOFOCUS_LOSS:
                    mPlayer.pause();
                    break;
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                    if (mPlayer.isPlaying()) {
                        mPlayer.pause();
                    }
                    break;
                case AudioManager.AUDIOFOCUS_GAIN:
                    if (mPlayer.isPrepared()) {
                        start();
                    }
                    break;
            }
            updatePlayPause();
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.fm_record_list_layout);
        ActionBar actionBar = getActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        mListView = (ListView) findViewById(R.id.fm_record_list);
        mBatchOperationLay = (LinearLayout) findViewById(R.id.layout_batch_operation);
        mCheckBoxSelectAll = (CheckBox) findViewById(R.id.ck_selectAll);

        mContext = getApplicationContext();
        mContentResolver = getContentResolver();
        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        Intent recordIntent = getIntent();
        mIntentRecordUri = recordIntent.getData();
        mListView.setOnItemLongClickListener(new OnListLongClickListener());
        registerSdcardReceiver();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.fm_record_list_menu, menu);
        mSearchItem = menu.findItem(R.id.list_search);
        mSearchView = (SearchView) mSearchItem.getActionView();
        int id = mSearchView.getContext().getResources()
                .getIdentifier("android:id/search_src_text", null, null);
        TextView textView = (TextView) mSearchView.findViewById(id);
        textView.setTextColor(Color.WHITE);
        textView.setHintTextColor(getResources().getColor(R.color.text_hint));
        mSearchView.setQueryHint(getResources().getString(R.string.search_hint));
        mSearchView.setOnQueryTextListener(this);
        mSearchView.setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                onBackPressed();
                break;
            case R.id.list_search:
                onSearchRequested();
                break;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onBackPressed() {
        if (mPlayer != null) {
            stopPlayback();
        } else {
            super.onBackPressed();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (!FmUtils.hasStoragePermission(this)) { // No StoragePermission
            String[] permissionsToRequest = new String[1];
            permissionsToRequest[0] = Manifest.permission.WRITE_EXTERNAL_STORAGE;
            requestPermissions(permissionsToRequest, FmUtils.FM_PERMISSIONS_REQUEST_CODE);
            return;
        }
        if (mCursor == null) {
            initCursor();
        }
        if (mActionMode == null) {
            initCheckStatus();
            FmUtils.updateStatusBarColor(this, false);
        } else {
            int checkedCount = 0;
            if (mCursor.getCount() == 0) {
                mActionMode.finish();
                return;
            }
            mDeleteItemChecked.clear();
            mCheckboxStatus.clear();
            for (int i = 0; i < mCursor.getCount(); i++) {
                if (itemSelected(i)) {
                    mDeleteItemChecked.add(i, getDeleteItem(i));
                    mCheckboxStatus.put(i, true);
                    checkedCount++;
                } else {
                    mDeleteItemChecked.add(i, null);
                    mCheckboxStatus.put(i, false);
                }
            }
            mActionMode.setTitle(String.valueOf(checkedCount));
            updateCheckAllBox();
            updateDeleteButton();
            FmUtils.updateStatusBarColor(this, true);
        }
        if (mAdapter == null) {
            mAdapter = new FmRecordAdapter(this, mCursor, mIntentRecordUri);
        }
        mListView.setAdapter(mAdapter);
        mListView.setOnItemClickListener(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        stopPlayback();
    }

    private void initCursor() {
       mSelection = AudioColumns.ALBUM + "=?";
       mSelectionArgs =  new String[] {FmRecorder.RECORDING_FILE_SOURCE};
       mCursor = mContentResolver.query(sBaseUri, PROJECTION,
               mSelection,mSelectionArgs, AudioColumns._ID + " collate NOCASE DESC");
    }


    private void initCheckStatus() {
        mCheckboxStatus.clear();
        mDeleteItemChecked.clear();
        if (mCursor == null) {
            return;
        }
        for (int i = 0; i < mCursor.getCount(); i++) {
            mDeleteItemChecked.add(null);
        }
        /**
         * SPRD: Add for Bug638498
         * @{
         */
        if (mCursor.getCount() == 0) {
            noSearchResult = true;
        }
        /* SPRD: Add for Bug638498  @} */
    }

    private boolean itemSelected(int position) {
        boolean res = false;
        int curId = getItemDataBaseID(position);
        for (int i = 0; i < mSelectItemDataBaseId.size(); i++) {
            if (curId == mSelectItemDataBaseId.get(i)) {
                return true;
            }
        }
        return res;
    }

    private static class ViewHolder {
        TextView title;
        LinearLayout recordStart;
        SeekBar curSeekBar;
        CheckBox checkBox;
    }

    private class FmRecordAdapter extends CursorAdapter {

        private LayoutInflater mInflater;
        private int mCurPosition;
        private int mLastId = -1;
        private Uri playUri;

        public FmRecordAdapter(Context context, Cursor c, Uri uri) {
            super(context, c);
            mInflater = LayoutInflater.from(context);
            playUri = uri;
        }

        @Override
        public View newView(Context context, Cursor cursor, ViewGroup parent) {
            View view = mInflater.inflate(R.layout.fm_record_item, null, false);
            ViewHolder holder = new ViewHolder();
            holder.title = (TextView) view.findViewById(R.id.fm_record_name);
            holder.recordStart = (LinearLayout) view.findViewById(R.id.record_start);
            holder.curSeekBar = (SeekBar) view.findViewById(R.id.progress);
            holder.checkBox = (CheckBox) view.findViewById(R.id.delete_check);
            view.setTag(holder);
            return view;
        }

        public void bindView(View view, Context context, Cursor cursor, int position, Uri uri) {
            String displayName = cursor.getString(INDEX_DISPLAY_NAME);
            final ViewHolder holder = (ViewHolder) view.getTag();
            holder.title.setText(displayName);
            holder.checkBox.setVisibility(View.VISIBLE);
            if (mActionMode != null) {
                holder.checkBox.setVisibility(View.VISIBLE);
                holder.checkBox.setChecked(itemSelected(position));
                holder.recordStart.setEnabled(false);
            } else {
                holder.checkBox.setVisibility(View.GONE);
                holder.recordStart.setEnabled(true);
            }
            cursor.moveToPosition(position);
            final int curId = cursor.getInt(INDEX_ID);
            if (mPlayer != null) {
                ImageView recordIcon = (ImageView)((View)holder.recordStart.getParent()).findViewById(R.id.recode_icon);
                if (mLastId == curId) {
                    mTitle = holder.title;
                    mRecodeIcon = recordIcon;
                    mCurSeekBar = holder.curSeekBar;
                    mCurSeekBar.setMax(mPlayer.mDuration);
                    mCurSeekBar.setProgress(mPlayer.getCurrentPosition());
                    mPlayer.setUISeekBar(holder.curSeekBar);
                    holder.title.setVisibility(View.GONE);
                    holder.curSeekBar.setVisibility(View.VISIBLE);
                    if (mPlayer.isPlaying()) {
                        recordIcon.setBackgroundDrawable(getResources().getDrawable(R.drawable.btn_fm_record_halt));
                    } else {
                        recordIcon.setBackgroundDrawable(getResources().getDrawable(R.drawable.btn_fm_record_play));
                    }
                    /**
                     * SPRD: Add for Bug638498
                     * @{
                     */
                    if (mCursor.getCount() > 0 && noSearchResult) {
                        noSearchResult = false;
                        mPlayer.setProgressListener();
                    }
                    /* SPRD: Add for Bug638498  @} */
                } else {
                    holder.curSeekBar.setVisibility(View.GONE);
                    holder.title.setVisibility(View.VISIBLE);
                    recordIcon.setBackgroundDrawable(getResources().getDrawable(R.drawable.btn_fm_record_play));
                }
            }
            String mdisplayName = Uri.encode(displayName);
            if (uri != null && uri.toString().contains(mdisplayName)) {
                itemStartPlay(holder.recordStart, curId);
            }

            holder.recordStart.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    itemStartPlay(v, curId);
                }
            });
        }

        private void itemStartPlay(View view, int curId) {
            View itemContainer = (View) view.getParent();
            ImageView recodeIcon = (ImageView) itemContainer.findViewById(R.id.recode_icon);
            SeekBar curSeekBar = (SeekBar) itemContainer.findViewById(R.id.progress);
            TextView title = (TextView) itemContainer.findViewById(R.id.fm_record_name);
            if (curId != mLastId && mPlayer != null) {
                mLastId = curId;
                if (mProgressRefresher != null) {
                    mProgressRefresher.removeCallbacksAndMessages(null);
                }
                if (mPlayer != null) {
                    updatePlayStop();
                    mPlayer.release();
                    mPlayer = null;
                }
            }
            mRecodeIcon = recodeIcon;
            mCurSeekBar = curSeekBar;
            mTitle = title;
            if (mPlayer == null) {
                title.setVisibility(View.GONE);
                curSeekBar.setVisibility(View.VISIBLE);
                mPlayer = new FmMediaPalyer();
                mPlayer.setActivity(FmRecordListActivity.this);
                try {
                    Uri mUri = ContentUris.withAppendedId(
                            sBaseUri, curId);
                    mPlayer.setDataSourceAndPrepare(mUri);
                    mPlayer.setUISeekBar(curSeekBar);
                    mLastId = curId;
                } catch (Exception e) {
                    Toast.makeText(FmRecordListActivity.this, R.string.playback_failed,
                            Toast.LENGTH_SHORT).show();
                    return;
                }
            } else {
                if (mPlayer.isPlaying()) {
                    mPlayer.pause();
                } else if (mPlayer.isPrepared()){
                    start();
                }
                updatePlayPause();
            }
            if (mIntentRecordUri != null) {
                mIntentRecordUri = null;
            }
            clearPalyUri();
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            mCurPosition = position;
            return super.getView(position, convertView, parent);
        }

        @Override
        public void bindView(View view, Context context, Cursor cursor) {
            bindView(view, context, cursor, mCurPosition, playUri);
        }

        @Override
        public void notifyDataSetChanged() {
            super.notifyDataSetChanged();
            updateCheckAllBox();
            updateDeleteButton();
        }

        private void clearPalyUri() {
            if (playUri != null) {
                playUri = null;
            }
        }

    }

    public void start() {
        if (!requestAudioFocus()) {
            Toast.makeText(this.getApplicationContext(), R.string.no_allow_play_calling,
                    Toast.LENGTH_SHORT).show();
            return;
        }
        mPlayer.start();
        if (mProgressRefresher != null) {
            mProgressRefresher.removeCallbacksAndMessages(null);
            mProgressRefresher.postDelayed(new ProgressRefresher(), 200);
        }
    }

    class ProgressRefresher implements Runnable {
        @Override
        public void run() {
            if (mPlayer != null && !mPlayer.mSeeking && mPlayer.mDuration != 0) {
                int currentTime = mPlayer.getCurrentPosition();
                mPlayer.mSeekBar.setProgress(currentTime);
            }
            mProgressRefresher.removeCallbacksAndMessages(null);
            if (mPlayer != null) {
                mProgressRefresher.postDelayed(new ProgressRefresher(), 200);
            }
        }
    }

    private boolean requestAudioFocus() {
        int audioFocus = mAudioManager.requestAudioFocus(mAudioFocusListener,
                AudioManager.STREAM_MUSIC,
                AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
        if (audioFocus == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
            return true;
        }
        return false;
    }

    public void updatePlayPause() {
        ImageView playicon = mRecodeIcon;
        if (playicon != null && mPlayer != null) {
            if (mPlayer.isPlaying()) {
                playicon.setBackgroundDrawable(getResources().getDrawable(
                        R.drawable.btn_fm_record_halt));
            } else {
                playicon.setBackgroundDrawable(getResources().getDrawable(
                        R.drawable.btn_fm_record_play));
                mProgressRefresher.removeCallbacksAndMessages(null);
            }
        }
    }

    public void updatePlayStop() {
        mCurSeekBar.setProgress(0);
        mCurSeekBar.setVisibility(View.GONE);
        mTitle.setVisibility(View.VISIBLE);
        mRecodeIcon
                .setBackgroundDrawable(getResources().getDrawable(R.drawable.btn_fm_record_play));
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

    private class RecordDeleteItem {
        private int id;
        private String data;

        public RecordDeleteItem(int id, String data) {
            this.id = id;
            this.data = data;
        }
    }

    private RecordDeleteItem getDeleteItem(int position) {
        if (mCursor == null) {
            return null;
        }
        mCursor.moveToPosition(position);
        int curId = mCursor.getInt(INDEX_ID);
        String data = mCursor.getString(INDEX_DATA);
        RecordDeleteItem RecordDeleteItem = new RecordDeleteItem(curId, data);
        return RecordDeleteItem;

    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        if (mActionMode != null && mCursor != null) {
            CheckBox checkBox = (CheckBox) view.findViewById(R.id.delete_check);
            if (null == checkBox) {
                return;
            }
            if (checkBox.isChecked()) {
                checkBox.setChecked(false);
            } else {
                checkBox.setChecked(true);
            }
            mCheckboxStatus.put(position, checkBox.isChecked());
            mDeleteItemChecked.set(position, checkBox.isChecked() ? getDeleteItem(position) : null);
            mActionMode.setTitle(String.valueOf(getCheckSelectSize()));
            saveSelectItemDataBaseId();
            mAdapter.notifyDataSetChanged();
        } else {
            return;
        }
    }

    @Override
    public boolean onQueryTextSubmit(String query) {
        return true;
    }

    @Override
    public boolean onQueryTextChange(String newText) {
        if (FmUtils.hasStoragePermission(this)) {
            mSelection = AudioColumns.ALBUM + "=? AND " + AudioColumns.DISPLAY_NAME + " like ?";
            mSelectionArgs = new String[] {FmRecorder.RECORDING_FILE_SOURCE, "%" + newText + "%"};
            mCursor = mContentResolver.query(sBaseUri,
                    PROJECTION,mSelection,mSelectionArgs, AudioColumns._ID + " collate NOCASE DESC");
            if (mAdapter == null || mCursor == null) {
                return true;
            }
            initCheckStatus();
            mAdapter.changeCursor(mCursor);
            mAdapter.notifyDataSetChanged();
        }
        return true;
    }

    @Override
    protected void onDestroy() {
        if (mCursor != null) {
            mCursor.close();
            mCursor = null;
        }
        unregisterSdcardListener();
        super.onDestroy();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        switch (requestCode) {
            case FmUtils.FM_PERMISSIONS_REQUEST_CODE: {
                boolean resultsAllGranted = true;
                if (grantResults.length > 0) {
                    for (int result : grantResults) {
                        if (PackageManager.PERMISSION_GRANTED != result) {
                            resultsAllGranted = false;
                        }
                    }
                } else {
                    resultsAllGranted = false;
                }

                if (resultsAllGranted) {
                    Log.d(TAG, "Get Storage Permission");
                } else {
                    Log.d(TAG, "No Storage Permission, finish !");
                    Toast.makeText(this, R.string.no_storage_permission, Toast.LENGTH_LONG).show();
                    this.finish();
                }
            }
        }
    }

    /* Long click enter ActionMode to Delete item */
    private class OnListLongClickListener implements OnItemLongClickListener {

        @Override
        public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
            if (mActionMode == null && mAdapter != null) {
                startActionMode(new longClickCallback(position));
            }
            return true;
        }
    }

    private class longClickCallback implements ActionMode.Callback {
        private int position = -1;

        public longClickCallback(int position) {
            this.position = position;
        }

        @Override
        public boolean onCreateActionMode(ActionMode mode, Menu menu) {
            mActionMode = mode;
            MenuInflater menuInflater = mode.getMenuInflater();
            menuInflater.inflate(R.menu.delete_record_action_menu, menu);
            deleteButton = menu.findItem(R.id.item_delete_record);
            stopPlayback();
            if (mCheckboxStatus.size() == 0) {
                if (mCursor == null) {
                    initCursor();
                }
                initCheckStatus();
            }
            mCheckboxStatus.put(position, true);
            mDeleteItemChecked.set(position, getDeleteItem(position));
            mActionMode.setTitle(String.valueOf(getCheckSelectSize()));
            saveSelectItemDataBaseId();
            mAdapter.notifyDataSetChanged();
            return true;
        }

        @Override
        public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
            FmUtils.updateStatusBarColor(FmRecordListActivity.this, true);
            setBatchOperLayoutVisibility(View.VISIBLE);
            return true;
        }

        @Override
        public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
            switch (item.getItemId()) {
                case R.id.item_delete_record:
                    deleteFileAysnc();
                    break;
                default:
                    break;
            }
            return true;
        }

        @Override
        public void onDestroyActionMode(ActionMode mode) {
            mActionMode = null;
            setBatchOperLayoutVisibility(View.GONE);
            FmUtils.updateStatusBarColor(FmRecordListActivity.this, false);
            initCheckStatus();
        }
    }

    private int getCheckSelectSize() {
        int size = 0;
        for (int i = 0; i < mDeleteItemChecked.size(); i++) {
            if (mDeleteItemChecked.get(i) != null) {
                size++;
            }
        }
        return size;
    }

    private void selectAll() {
        for (int i = 0; i < mCursor.getCount(); i++) {
            mDeleteItemChecked.set(i, getDeleteItem(i));
            mCheckboxStatus.put(i, true);
        }
        saveSelectItemDataBaseId();
        mActionMode.setTitle(String.valueOf(getCheckSelectSize()));
        mAdapter.notifyDataSetChanged();
    }

    private void cancelSelectAll() {
        for (int i = 0; i < mCursor.getCount(); i++) {
            mDeleteItemChecked.set(i, null);
            mCheckboxStatus.put(i, false);
        }
        mSelectItemDataBaseId.clear();
        mActionMode.setTitle(String.valueOf(getCheckSelectSize()));
        mAdapter.notifyDataSetChanged();
    }

    private void updateCheckAllBox() {
        if (mCheckBoxSelectAll == null) {
            return;
        }
        if (FmRecordListActivity.this.mCursor.getCount() == getCheckSelectSize()) {
            mCheckBoxSelectAll.setChecked(true);
        } else {
            mCheckBoxSelectAll.setChecked(false);
        }
    }

    private void updateDeleteButton() {
        if (deleteButton == null) {
            return;
        }
        if (getCheckSelectSize() != 0) {
            deleteButton.setEnabled(true);
            deleteButton.setIcon(getResources().getDrawable(R.drawable.ic_menu_delete_on));
        } else {
            deleteButton.setEnabled(false);
            deleteButton.setIcon(getResources().getDrawable(R.drawable.ic_menu_delete_disable));
        }
    }

    private boolean isFinishOrDestroy() {
        if (FmRecordListActivity.this.isDestroyed() || FmRecordListActivity.this.isFinishing()) {
            return true;
        }
        return false;
    }

    private void deleteFile(RecordDeleteItem item) {
        mContentResolver.delete(
                ContentUris.withAppendedId(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, item.id),
                null, null);
        File del = new File(item.data);
        if (!del.exists() || !del.delete()) {
            return;
        }
    }

    private SpannableString setSpString(String string) {
        SpannableString title = new SpannableString(string);
        int titleColor = getResources().getColor(R.color.actionbar_overflow_title_color);
        title.setSpan(new ForegroundColorSpan(titleColor), 0, title.length(), 0);
        return title;
    }

    private void setBatchOperLayoutVisibility(int visibility) {
        if (mBatchOperationLay == null || mCheckBoxSelectAll == null) {
            return;
        }
        mBatchOperationLay.setVisibility(visibility);
        if (visibility == View.VISIBLE) {
            mBatchOperationLay.setOnClickListener(new OnClickListener() {

                @Override
                public void onClick(View v) {
                    if (getCheckSelectSize() != mCursor.getCount()) {
                        selectAll();
                        mCheckBoxSelectAll.setChecked(true);
                    } else {
                        cancelSelectAll();
                        mCheckBoxSelectAll.setChecked(false);
                    }
                }
            });
        }
    }

    private void deleteFileAysnc() {
        AsyncTask<Void, Long, Void> task = new AsyncTask<Void, Long, Void>() {
            ProgressDialog pd = null;

            @Override
            protected void onPreExecute() {
                pd = new ProgressDialog(FmRecordListActivity.this);
                pd.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
                pd.setTitle(setSpString(getResources().getString(R.string.progress_deleting)));
                pd.setCancelable(false);
                pd.setMax(getCheckSelectSize());
                pd.show();
            }

            @Override
            protected Void doInBackground(Void... params) {
                long index = 0;
                for (int i = 0; i < mDeleteItemChecked.size(); i++) {
                    RecordDeleteItem deleteItem = mDeleteItemChecked.get(i);
                    if (deleteItem != null) {
                        deleteFile(deleteItem);
                        publishProgress(++index);
                    }
                }
                Log.i(TAG, "delete finished.");
                return null;
            }

            @Override
            protected void onProgressUpdate(Long... values) {
                pd.setProgress(values[0].intValue());
            }

            @Override
            protected void onPostExecute(Void result) {
                if (!isFinishOrDestroy()) {
                    pd.cancel();
                }
                mAdapter.notifyDataSetChanged();
                mActionMode.finish();
            }
        };
        task.execute((Void[]) null);
    }
    /* SPRD Bug 596189 Handle the KEYCODE_SEARCH key event by the onKeyUp method in FmRecordListActivity @{ */
    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        Log.i(TAG, "receive KeyEvent:" + event.getKeyCode());
        if (event.getKeyCode() == KeyEvent.KEYCODE_SEARCH && mSearchView != null) {
            /* SPRD Bug 598903 can not show mSearchView when ActionMode exist @{ */
            if (mActionMode != null) {
                mActionMode.finish();
            }
            /* Bug 598903 end @} */
            mSearchView.requestFocus();
            mSearchView.setIconified(false);
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }
    /* Bug 596189 end @} */

    private BroadcastReceiver mSdcardListener = null;

    private class SdcardListener extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.i(TAG, "on receive:" + action);
            if (mActionMode != null) {
                mActionMode.finish();
            }
            // Modify for 657741,FM save recordings through different path,music will show two album about fm recordings.
            // SPRD Change for 661271, SecurityException happen when MTP connect without storage permission set.
            if(!FmUtils.hasStoragePermission(mContext)) {
                Log.d(TAG, "There is no Storage Permission,return.");
                return;
            }
            initCursor();
            if (mAdapter == null || mCursor == null) {
                return;
            }
            initCheckStatus();
            mAdapter.changeCursor(mCursor);
            mAdapter.notifyDataSetChanged();
        }
    }
    /**
     * Register sdcard listener for record
     */
    private void registerSdcardReceiver() {
        if (mSdcardListener == null) {
            mSdcardListener = new SdcardListener();
        }
        IntentFilter filter = new IntentFilter();
        filter.addDataScheme("file");
        filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
        filter.addAction(Intent.ACTION_MEDIA_EJECT);
        filter.addAction(Intent.ACTION_MEDIA_SCANNER_FINISHED);
        registerReceiver(mSdcardListener, filter);
    }

    private void unregisterSdcardListener() {
        if (null != mSdcardListener) {
            unregisterReceiver(mSdcardListener);
        }
    }

    private int getItemDataBaseID(int position){
        int curId = -1;
        if (mCursor == null || position >= mCursor.getCount()) {
            return curId;
        }
        mCursor.moveToPosition(position);
        curId = mCursor.getInt(INDEX_ID);
        return curId;
    }

    private void saveSelectItemDataBaseId(){
        //to save select items database ID
        mSelectItemDataBaseId.clear();
        Iterator iter = mCheckboxStatus.entrySet().iterator();
        while (iter.hasNext()) {
            Map.Entry entry = (Map.Entry) iter.next();
            Integer key = (Integer) entry.getKey();
            Boolean val = (Boolean) entry.getValue();
            int curId = getItemDataBaseID(key);
            if (curId != -1 && val) {
                mSelectItemDataBaseId.add(curId);
            }
        }
    }
}
