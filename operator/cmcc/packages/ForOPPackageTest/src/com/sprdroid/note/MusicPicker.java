package com.sprdroid.note;

import java.io.IOException;

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
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.RadioButton;
import android.widget.SimpleCursorAdapter;
import android.widget.TextView;

public class MusicPicker extends ListActivity {

    private static String[] CURSOR_COLS = new String[] {
            MediaStore.Audio.Media._ID, MediaStore.Audio.Media.TITLE,
            MediaStore.Audio.Media.TITLE_KEY, MediaStore.Audio.Media.DATA,
            MediaStore.Audio.Media.ALBUM, MediaStore.Audio.Media.ARTIST,
            MediaStore.Audio.Media.ARTIST_ID, MediaStore.Audio.Media.DURATION,
            MediaStore.Audio.Media.TRACK };
    private final static String UNKOWN_ALBUM = "<Unkown Album>";
    private final static String UNKOWN_ARTIST = "<Unkown Artist>";
    Cursor mCursor;
    private Uri baseUri;
    private MediaPlayer mp;
    private AudioManager am;
    private long playingId;

    class MusicListAdapter extends BaseAdapter {
        private int idIdx;
        private int dataIdx;
        private int titleIdx;
        private int albumIdx;
        private int artistIdx;
        private int durationIdx;
        private Context context;
        private LayoutInflater inflater;

        class ViewHolder {
            RadioButton radio;
            TextView title;
            TextView album;
            TextView artist;
            TextView time;
        }

        public MusicListAdapter(Context context) {
            inflater = LayoutInflater.from(context);
            this.context = context;
            if (mCursor != null) {
                idIdx = mCursor.getColumnIndex(MediaStore.Audio.Media._ID);
                dataIdx = mCursor.getColumnIndex(MediaStore.Audio.Media.DATA);
                titleIdx = mCursor.getColumnIndex(MediaStore.Audio.Media.TITLE);
                albumIdx = mCursor.getColumnIndex(MediaStore.Audio.Media.ALBUM);
                artistIdx = mCursor
                        .getColumnIndex(MediaStore.Audio.Media.ARTIST);
                durationIdx = mCursor
                        .getColumnIndex(MediaStore.Audio.Media.DURATION);
            }
            mCursor.moveToFirst();
        }

        @Override
        public int getCount() {
            return mCursor.getCount();
        }

        @Override
        public Object getItem(int position) {
            return mCursor.moveToPosition(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View view, ViewGroup parent) {
            ViewHolder vh;
            if (view == null) {
                vh = new ViewHolder();
                view = inflater.inflate(R.layout.music_list_item, null);
                vh.radio = (RadioButton) view.findViewById(R.id.music_radio);
                vh.title = (TextView) view.findViewById(R.id.music_title);
                vh.album = (TextView) view.findViewById(R.id.music_album);
                vh.artist = (TextView) view.findViewById(R.id.music_artist);
                vh.time = (TextView) view.findViewById(R.id.music_time);
                view.setTag(vh);
            } else {
                vh = (ViewHolder) view.getTag();
            }
            String title = mCursor.getString(titleIdx);
            String album = mCursor.getString(albumIdx);
            String artist = mCursor.getString(artistIdx);
            String data = mCursor.getString(dataIdx);

            if (album == null || album.equals(""))
                album = UNKOWN_ALBUM;
            if (artist == null || artist.equals(""))
                artist = UNKOWN_ARTIST;

            long newId = mCursor.getLong(mCursor
                    .getColumnIndex(MediaStore.Audio.Media._ID));
            vh.radio.setChecked(newId == playingId);
            vh.title.setText(title);
            vh.album.setText(album);
            vh.artist.setText(artist);
            if (mCursor.getPosition() < getCount())
                mCursor.moveToPosition(mCursor.getPosition() + 1);
            return view;
        }

    }

    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        setContentView(R.layout.music_picker);

        am = (AudioManager) getSystemService(AUDIO_SERVICE);
        Intent intent = this.getIntent();
        baseUri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
        doQuery();
        ListView lv = (ListView) findViewById(android.R.id.list);
        MusicListAdapter adapter = new MusicListAdapter(this);
        // lv.setAdapter(adapter);
        setListAdapter(adapter);
        lv.setFooterDividersEnabled(true);
        lv.setFastScrollEnabled(true);
        /*
         * lv.setOnItemClickListener(new ListView.OnItemClickListener(){
         * 
         * @Override public void onItemClick(AdapterView<?> arg0, View v, int
         * position, long arg3) { MusicListAdapter.ViewHolder vh =
         * (MusicListAdapter.ViewHolder)v.getTag(); RadioButton radio =
         * (RadioButton)v.findViewById(R.id.music_radio);
         * radio.setChecked(true); mCursor.moveToPosition(position); long id =
         * mCursor.getLong(mCursor.getColumnIndex(MediaStore.Audio.Media._ID));
         * play(id); } });
         */

    }

    void doQuery() {
        StringBuffer where = new StringBuffer();
        where.append(MediaStore.Audio.Media.TITLE + "!=''");
        String[] keywords = null;
        mCursor = getContentResolver().query(baseUri, CURSOR_COLS,
                where.toString(), keywords, MediaStore.Audio.Media.TITLE_KEY);
    }

    public void play(long id) {
        try {
            if (id != playingId || mp == null) {
                stop();
                mp = new MediaPlayer();
                Uri uri = ContentUris.withAppendedId(
                        MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, id);
                mp.setDataSource(this, uri);
                if (am.getStreamVolume(AudioManager.STREAM_ALARM) != 0) {
                    mp.setAudioStreamType(AudioManager.STREAM_ALARM);
                    // mp.setLooping(true);
                    mp.prepare();
                    mp.start();
                }
            } else {
                stop();
                getListView().invalidateViews();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

    }

    public void stop() {
        if (mp != null) {
            mp.stop();
            mp.release();
            mp = null;
            playingId = -1;
        }
    }

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        RadioButton radio = (RadioButton) v.findViewById(R.id.music_radio);
        radio.setChecked(true);
        mCursor.moveToPosition(position);
        long newId = mCursor.getLong(mCursor
                .getColumnIndex(MediaStore.Audio.Media._ID));
        play(newId);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            stop();
        }
        return super.onKeyDown(keyCode, event);
    }

}
