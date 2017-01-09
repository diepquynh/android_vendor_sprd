package com.sprd.soundrecorder;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import com.android.soundrecorder.R;
import com.android.soundrecorder.SoundRecorder;
import com.android.soundrecorder.RecordService;
import android.app.Activity;
import android.app.ListActivity;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.RadioButton;
import android.widget.TextView;


public class SoundPicker extends ListActivity
       implements AdapterView.OnItemClickListener,MediaPlayer.OnCompletionListener,View.OnClickListener{
    static final String TAG = "SoundPicker";
    private long mSelectedId = -1;
    private Uri mSelectedUri;
    private ListView mListView;
    private Cursor mCursor = null;
    private MediaPlayer mMediaPlayer;
    private long mPlayingId = -1;
    private Button btnCancel,btnEnsure;
    private TextView mEmptylist;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.sound_picker);
        initResourceRefs();
        RecordService.activityList.add(SoundPicker.this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mListView = getListView();
        mAdapter  = new CursorRecorderAdapter();
        isVisibility();
        setListAdapter(mAdapter);
        mListView.setOnItemClickListener(this);
    }

    @Override
    public void onPause(){
        super.onPause();
        stopMediaPlayer();
    }

    @Override public void onStop() {
        super.onStop();
        stopMediaPlayer();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            finish();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }
    protected void setSelected(Cursor c) {
        Uri uri = null;
        if (StorageInfos.isExternalStorageMounted() || !StorageInfos.isInternalStorageSupported()){
            uri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
        }else
            uri = MediaStore.Audio.Media.INTERNAL_CONTENT_URI;
        long newId = mCursor.getLong(mCursor.getColumnIndex(MediaStore.Audio.Media._ID));
        mSelectedUri = ContentUris.withAppendedId(uri, newId);
        mSelectedId = newId;
        if (newId != mPlayingId || mMediaPlayer == null) {
            stopMediaPlayer();
            mMediaPlayer = new MediaPlayer();
            try {
                mMediaPlayer.setDataSource(this, mSelectedUri);
                mMediaPlayer.setOnCompletionListener(this);
                mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
                mMediaPlayer.prepare();
                mMediaPlayer.start();
                mPlayingId = newId;
                getListView().invalidateViews();
            } catch (IOException e) {
                Log.w("MusicPicker", "Unable to play track", e);
                mSelectedId = -1;
            }
        } else if (mMediaPlayer != null) {
            stopMediaPlayer();
            getListView().invalidateViews();
        }
    }
    void stopMediaPlayer() {
        if (mMediaPlayer != null) {
            mMediaPlayer.stop();
            mMediaPlayer.release();
            mMediaPlayer = null;
            mPlayingId = -1;
        }
    }
    private class CursorRecorderAdapter extends BaseAdapter {

        private final static int INIT_SIZE = 10;
        private List<RecorderItem> mData = null;

        CursorRecorderAdapter() {
            super();
            mData = query();
        }

        @Override
        public int getCount() {
            return mData.size();
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

        @Override
        public View getView(int pos, View cvt, ViewGroup pat) {
            if (cvt == null) {
                LayoutInflater flater =
                    (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                cvt = flater.inflate(R.layout.soundpicker_file_item, null);
                if (cvt == null)
                    throw new RuntimeException("inflater \"record_item.xml\" failed; pos == " + pos);
            }

            RecorderItem item = findItem(pos);
            if (item == null) throw new RuntimeException("findItem() failed; pos == " + pos);

            TextView tv = null;
            tv = (TextView) cvt.findViewById(R.id.picker_displayname);
            tv.setText(item.display_name);
            RadioButton rb = (RadioButton)cvt.findViewById(R.id.picker_radio);
            rb.setTag(pos);
            rb.setChecked(item.id == mSelectedId);
            return cvt;
        }
        private ArrayList<RecorderItem> query() {
            ArrayList<RecorderItem> result =
                new ArrayList<RecorderItem>(INIT_SIZE);
            try {
                StringBuilder where = new StringBuilder();
                where.append(MediaStore.Audio.Media.COMPOSER)
                    .append("='")
                    .append(SoundRecorder.COMPOSER)
                    .append("'");
                mCursor = SoundPicker.this.getContentResolver().query(
                        MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
                        new String[] {
                            RecorderItem._ID,
                            RecorderItem.DISPLAY_NAME,},
                            where.toString(), null, null);
                int index = -1;
                for (mCursor.moveToFirst(); !mCursor.isAfterLast(); mCursor.moveToNext()) {
                    index = mCursor.getColumnIndex(RecorderItem._ID);
                    // create recorder object
                    long id = mCursor.getLong(index);
                    RecorderItem item = new RecorderItem(id);
                    // SET "display name" value
                    index = mCursor.getColumnIndex(RecorderItem.DISPLAY_NAME);
                    item.display_name = mCursor.getString(index);
                    // set "time" value
                    result.add(item);
                }
            } catch (Exception e) {
                Log.v(TAG, "RecordingFileList.CursorRecorderAdapter failed; E: " + e);
            }
            return result;
        }
        private RecorderItem findItem(int pos) {
            RecorderItem result = null;
            Object obj = getItem(pos);
            if (obj != null && obj instanceof RecorderItem) {
                result = (RecorderItem) obj;
            }
            return result;
        }
    }
    private static class RecorderItem {
        private final long id;
        private String display_name;

        private static final String _ID         = MediaStore.Audio.Media._ID;
        private static final String DISPLAY_NAME= MediaStore.Audio.Media.DISPLAY_NAME;

        RecorderItem(long id) {
            this.id = id;
        }

        @Override
        public String toString() {
            StringBuffer buff = new StringBuffer();
            buff.append("id == ").append(id)
                .append(" --- display_name == ").append(display_name);
            return buff.toString();
        }
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        mCursor.moveToPosition(position);
        setSelected(mCursor);
        btnEnsure.setEnabled(true);
    }

    @Override
    public void onCompletion(MediaPlayer mp) {
        if (mMediaPlayer == mp) {
            mp.stop();
            mp.release();
            mMediaPlayer = null;
            mPlayingId = -1;
            getListView().invalidateViews();
        }
    }

    @Override
    public void onClick(View v){
        switch(v.getId()){
            case R.id.btnCancel:
                finish();
                break;
            case R.id.btnEnsure:
                Intent intent = new Intent(this, SoundRecorder.class);
                intent.setData(mSelectedUri);
                setResult(Activity.RESULT_OK, intent);
                finish();
                break;
        }
    }
    private void isVisibility(){
        if(mAdapter.getCount() < 1){
            mEmptylist.setVisibility(View.VISIBLE);
        }else
            mEmptylist.setVisibility(View.GONE);
    }
    private void initResourceRefs() {
        btnCancel = (Button)findViewById(R.id.btnCancel);
        btnEnsure = (Button)findViewById(R.id.btnEnsure);
        mEmptylist = (TextView)findViewById(R.id.picker_emptylist);
        btnCancel.setOnClickListener(this);
        btnEnsure.setOnClickListener(this);
        btnEnsure.setEnabled(false);
    }
}
