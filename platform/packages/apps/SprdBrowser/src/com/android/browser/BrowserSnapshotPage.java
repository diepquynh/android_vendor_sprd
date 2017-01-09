/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.browser;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.Fragment;
import android.app.LoaderManager.LoaderCallbacks;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.CursorLoader;
import android.content.Loader;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.MeasureSpec;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.ResourceCursorAdapter;
import android.widget.TextView;

import com.android.browser.provider.SnapshotProvider.Snapshots;

import java.text.DateFormat;
import java.util.Date;
import android.util.Log;
import android.database.sqlite.SQLiteException;
import android.database.ContentObserver;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;
import android.app.AlertDialog;
import android.content.DialogInterface;
import java.io.File;

public class BrowserSnapshotPage extends Fragment implements
        LoaderCallbacks<Cursor>, OnItemClickListener {

    public static final String EXTRA_ANIMATE_ID = "animate_id";
    private AlertDialog sAlertdialog = null;

    private static final int LOADER_SNAPSHOTS = 1;
    private static final String[] PROJECTION = new String[] {
        Snapshots._ID,
        Snapshots.TITLE,
        Snapshots.VIEWSTATE_SIZE,
        Snapshots.THUMBNAIL,
        Snapshots.FAVICON,
        Snapshots.URL,
        Snapshots.DATE_CREATED,
        /*
         * for save_webpage
         *@{
         */
        Snapshots.PATH,
        Snapshots.ROOT_PATH,
        Snapshots.WEBARCHIVE_PATH,
        /*@}*/
    };
    private static final int SNAPSHOT_ID = 0;
    private static final int SNAPSHOT_TITLE = 1;
    private static final int SNAPSHOT_VIEWSTATE_SIZE = 2;
    private static final int SNAPSHOT_THUMBNAIL = 3;
    private static final int SNAPSHOT_FAVICON = 4;
    private static final int SNAPSHOT_URL = 5;
    private static final int SNAPSHOT_DATE_CREATED = 6;
    /*
     * for save_webpage
     *@{
     */
    private static final int SNAPSHOT_PATH = 7;
    private static final int SNAPSHOT_ROOTPAT =8;
    /*@}*/

    GridView mGrid;
    View mEmpty;
    SnapshotAdapter mAdapter;
    CombinedBookmarksCallbacks mCallback;
    long mAnimateId;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        /*
         * for save_webpage
         *@{
         */
        setHasOptionsMenu(true);
        /*@}*/
        mCallback = (CombinedBookmarksCallbacks) getActivity();
        mAnimateId = getArguments().getLong(EXTRA_ANIMATE_ID);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.snapshots, container, false);
        mEmpty = view.findViewById(android.R.id.empty);
        mGrid = (GridView) view.findViewById(R.id.grid);
        setupGrid(inflater);
        getLoaderManager().initLoader(LOADER_SNAPSHOTS, null, this);
        return view;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        getLoaderManager().destroyLoader(LOADER_SNAPSHOTS);
        if (mAdapter != null) {
            mAdapter.changeCursor(null);
            mAdapter = null;
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (sAlertdialog != null){
            if(sAlertdialog.isShowing()){
                sAlertdialog.dismiss();
            }
            sAlertdialog = null;
        }
    }

    void setupGrid(LayoutInflater inflater) {
        View item = inflater.inflate(R.layout.snapshot_item, mGrid, false);
        int mspec = MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED);
        item.measure(mspec, mspec);
        int width = item.getMeasuredWidth();
        mGrid.setColumnWidth(width);
        mGrid.setOnItemClickListener(this);
        mGrid.setOnCreateContextMenuListener(this);
    }

    @Override
    public Loader<Cursor> onCreateLoader(int id, Bundle args) {
        if (id == LOADER_SNAPSHOTS) {
            return new SnapshotLoader(getActivity(),
                    Snapshots.CONTENT_URI, PROJECTION,
                    null, null, Snapshots.DATE_CREATED + " DESC");
        }
        return null;
    }

    @Override
    public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
        if (loader.getId() == LOADER_SNAPSHOTS) {
            if (mAdapter == null) {
                mAdapter = new SnapshotAdapter(getActivity(), data);
                mGrid.setAdapter(mAdapter);
            } else {
                mAdapter.changeCursor(data);
            }
            if (mAnimateId > 0) {
                mAdapter.animateIn(mAnimateId);
                mAnimateId = 0;
                getArguments().remove(EXTRA_ANIMATE_ID);
            }
            boolean empty = mAdapter.isEmpty();
            mGrid.setVisibility(empty ? View.GONE : View.VISIBLE);
            mEmpty.setVisibility(empty ? View.VISIBLE : View.GONE);
        }
    }

    @Override
    public void onLoaderReset(Loader<Cursor> loader) {
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v,
            ContextMenuInfo menuInfo) {
        MenuInflater inflater = getActivity().getMenuInflater();
        inflater.inflate(R.menu.snapshots_context, menu);
        // Create the header, re-use BookmarkItem (has the layout we want)
        BookmarkItem header = new BookmarkItem(getActivity());
        header.setEnableScrolling(true);
        AdapterContextMenuInfo info = (AdapterContextMenuInfo) menuInfo;
        populateBookmarkItem(mAdapter.getItem(info.position), header);
        menu.setHeaderView(header);
    }

    private void populateBookmarkItem(Cursor cursor, BookmarkItem item) {
        item.setName(cursor.getString(SNAPSHOT_TITLE));
        item.setUrl(cursor.getString(SNAPSHOT_URL));
        item.setFavicon(getBitmap(cursor, SNAPSHOT_FAVICON));
    }

    static Bitmap getBitmap(Cursor cursor, int columnIndex) {
        byte[] data = cursor.getBlob(columnIndex);
        if (data == null) {
            return null;
        }
        return BitmapFactory.decodeByteArray(data, 0, data.length);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        if (item.getItemId() == R.id.delete_context_menu_id) {
            /*
             * for save_webpage
             * original code:
            AdapterContextMenuInfo info = (AdapterContextMenuInfo) item.getMenuInfo();
            deleteSnapshot(info.id);
             *@{
             */
            if (!(item.getMenuInfo() instanceof AdapterContextMenuInfo)) {
                return false;
            }
            final AdapterContextMenuInfo info = (AdapterContextMenuInfo) item.getMenuInfo();
            Cursor cursor = mAdapter.getItem(info.position);
            if (cursor == null) {
                return false;
            }
            Context context = getActivity();
            String title = cursor.getString(SNAPSHOT_TITLE);
            String message = context.getString(R.string.delete_snapshot_warning,title);
            String webArchive = cursor.getString(cursor.getColumnIndex(Snapshots.WEBARCHIVE_PATH));
            final String webArchiveDir = webArchive.substring(0, webArchive.lastIndexOf('/'));
            sAlertdialog = new AlertDialog.Builder(context)
                    .setIcon(android.R.drawable.ic_dialog_alert)
                    .setMessage(message)
                    .setPositiveButton(R.string.ok,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int whichButton) {
                                    deleteSnapshot(info.id,webArchiveDir);
                                }
                            })
                    .setNegativeButton(R.string.cancel, null)
                    .show();
            /*@}*/
            return true;
        }
        return super.onContextItemSelected(item);
    }

    void deleteSnapshot(long id) {
        final Uri uri = ContentUris.withAppendedId(Snapshots.CONTENT_URI, id);
        final ContentResolver cr = getActivity().getContentResolver();
        new Thread() {
            @Override
            public void run() {
                try {
                    cr.delete(uri, null, null);
                } catch (SQLiteException e) {
                    Log.e("BrowserSnapshotPage", "deleteSnapshot: " + e);
                }
            }
        }.start();

    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position,
            long id) {
        /*
         * for save_webpage
         *@{
         */
       String status = StorageUtils.getExternalStorageState();
        if((null == status) || (!status.equals(Environment.MEDIA_MOUNTED))){
            Toast.makeText(getActivity().getApplicationContext(), R.string.click_saved_webpage_has_deleted, Toast.LENGTH_SHORT).show();
            return;
        }

        Uri uri = ContentUris.withAppendedId(Snapshots.CONTENT_URI, id);
        Cursor c = null;
        try {
            c = getActivity().getApplicationContext().getContentResolver().query(uri, PROJECTION, null, null, null);
        } catch (SQLiteException e) {
            Log.e("BrowserSnapshotPage", "onItemClick: " + e);
        }
        if ((c != null) && (c.moveToFirst())) {
            String webArchive = c.getString(c.getColumnIndex(Snapshots.WEBARCHIVE_PATH));
            File file = new File(webArchive);
            if(!file.exists()){
                c.close();
                Toast.makeText(getActivity().getApplicationContext(), R.string.click_saved_webpage_has_deleted, Toast.LENGTH_SHORT).show();
                return;
            }
        }
        if(c != null){
            c.close();
        }
        /*@}*/
        mCallback.openSnapshot(id);
    }

    private static class SnapshotAdapter extends ResourceCursorAdapter {
        private long mAnimateId;
        private AnimatorSet mAnimation;
        private View mAnimationTarget;

        public SnapshotAdapter(Context context, Cursor c) {
            super(context, R.layout.snapshot_item, c, 0);
            mAnimation = new AnimatorSet();
            mAnimation.playTogether(
                    ObjectAnimator.ofFloat(null, View.SCALE_X, 0f, 1f),
                    ObjectAnimator.ofFloat(null, View.SCALE_Y, 0f, 1f));
            mAnimation.setStartDelay(100);
            mAnimation.setDuration(400);
            mAnimation.addListener(new AnimatorListener() {

                @Override
                public void onAnimationStart(Animator animation) {
                }

                @Override
                public void onAnimationRepeat(Animator animation) {
                }

                @Override
                public void onAnimationEnd(Animator animation) {
                    mAnimateId = 0;
                    mAnimationTarget = null;
                }

                @Override
                public void onAnimationCancel(Animator animation) {
                }
            });
        }

        public void animateIn(long id) {
            mAnimateId = id;
        }

        @Override
        public void bindView(View view, Context context, Cursor cursor) {
            long id = cursor.getLong(SNAPSHOT_ID);
            if (id == mAnimateId) {
                if (mAnimationTarget != view) {
                    float scale = 0f;
                    if (mAnimationTarget != null) {
                        scale = mAnimationTarget.getScaleX();
                        mAnimationTarget.setScaleX(1f);
                        mAnimationTarget.setScaleY(1f);
                    }
                    view.setScaleX(scale);
                    view.setScaleY(scale);
                }
                mAnimation.setTarget(view);
                mAnimationTarget = view;
                if (!mAnimation.isRunning()) {
                    mAnimation.start();
                }

            }
            ImageView thumbnail = (ImageView) view.findViewById(R.id.thumb);
            byte[] thumbBlob = cursor.getBlob(SNAPSHOT_THUMBNAIL);
            if (thumbBlob == null) {
                thumbnail.setImageResource(R.drawable.browser_thumbnail);
            } else {
                Bitmap thumbBitmap = BitmapFactory.decodeByteArray(
                        thumbBlob, 0, thumbBlob.length);
                thumbnail.setImageBitmap(thumbBitmap);
            }
            TextView title = (TextView) view.findViewById(R.id.title);
            title.setText(cursor.getString(SNAPSHOT_TITLE));
            TextView size = (TextView) view.findViewById(R.id.size);
            if (size != null) {
                int stateLen = cursor.getInt(SNAPSHOT_VIEWSTATE_SIZE);
                size.setText(String.format("%.2fMB", stateLen / 1024f / 1024f));
            }
            long timestamp = cursor.getLong(SNAPSHOT_DATE_CREATED);
            TextView date = (TextView) view.findViewById(R.id.date);
            DateFormat dateFormat = DateFormat.getDateInstance(DateFormat.SHORT);
            date.setText(dateFormat.format(new Date(timestamp)));
        }

        @Override
        public Cursor getItem(int position) {
            return (Cursor) super.getItem(position);
        }
    }
    /*
     * for save_webpage
     *@{
     */
    void deleteSnapshot(long id,String path) {
        final Uri uri = ContentUris.withAppendedId(Snapshots.CONTENT_URI, id);
        final long id_delete = id;
        final Context context = getActivity();
        if (context == null) {
            Log.e("BrowserSnapshotPage", "context is null.");
            return;
        }

        final ContentResolver cr = context.getContentResolver();
        final File file = new File(path);
        deleteFile(file);
        try {
            cr.delete(uri, "_id = "+id_delete, null);
        } catch (SQLiteException e) {
            Log.e("BrowserSnapshotPage", "deleteSnapshot: " + e);
        }
        getLoaderManager().restartLoader(LOADER_SNAPSHOTS, null, this);
    }

    private void deleteFile(File file){
        if(file.exists()){
            if(file.isFile()){
                file.delete();
            } else if(file.isDirectory()){
                File[] files = file.listFiles();
                for(int i =0 ;i<files.length;i++){
                    File subFile = files[i];
                    deleteFile(subFile);
                    file.delete();
                }
            }
        }
    }
    /*@}*/

    static class SnapshotLoader extends CursorLoader {

        public SnapshotLoader(Context context, Uri uri, String[] projection, String selection,
                String[] selectionArgs, String sortOrder) {
            super(context,uri,projection,selection,selectionArgs,sortOrder);
        }

        @Override
        public Cursor loadInBackground() {
            Cursor cursor = null;
            try {
                cursor = super.loadInBackground();
            } catch (SQLiteException  e) {
                Log.e("BrowserSnapshotPage","loadInBackground :"+e);
                if (cursor != null) {
                    cursor.close();
                    cursor = null;
                }
            }

            return cursor;
        }
    }
}
