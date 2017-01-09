/** SPRD:Bug474632 bookmark setting Create */
package com.sprd.gallery3d.app;

import com.android.gallery3d.R;
import com.android.gallery3d.app.MovieActivity;
import com.android.gallery3d.util.GalleryUtils;

import android.app.ActionBar;
import android.app.Activity;
import android.app.LoaderManager;
import android.content.Context;
import android.content.CursorLoader;
import android.content.Intent;
import android.content.Loader;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.AdapterView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.CursorAdapter;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class MovieBookmarks extends Activity implements
LoaderManager.LoaderCallbacks<Cursor>,
OnItemClickListener{
    private static final int LOADER_BOOKAMRKS = 1;

    private static final String TAG = "MovieBookMarks";

    private static final String[] PROJECTION = new String[] {
            MovieViewContentProvider._ID, MovieViewContentProvider.BOOKMARK_TITLE,
            MovieViewContentProvider.BOOKMARK_URL
    };

    private static final int COLUMN_INDEX_ID = 0;

    private static final int COLUMN_INDEX_TITLE = 1;

    private static final int COLUMN_INDEX_URL = 2;

    private BookMarksAdapter mAdapter;

    private ListView mList;

    private View mEmpty;
    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        getActionBar().setDisplayOptions(ActionBar.DISPLAY_SHOW_TITLE, ActionBar.DISPLAY_SHOW_TITLE);
        setTitle(R.string.video_bookmarks);
        setContentView(R.layout.video_bookmarks);
        LoaderManager lm = getLoaderManager();
        lm.restartLoader(LOADER_BOOKAMRKS, null, this);
        mList = (ListView)findViewById(R.id.bookmarks);
        mEmpty = findViewById(R.id.empty);
        mAdapter = new BookMarksAdapter(this, null, 0);
        mList.setAdapter(mAdapter);
        mList.setOnCreateContextMenuListener(this);
        mList.setOnItemClickListener(this);
    }

    @Override
    public void onMultiWindowModeChanged(boolean isInMultiWindowMode) {
        if (isInMultiWindowMode){
            android.util.Log.d(TAG, "onMultiWindowModeChanged: " + isInMultiWindowMode);
            Toast.makeText(this,R.string.exit_multiwindow_video_tips, Toast.LENGTH_SHORT).show();
            finish();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        GalleryUtils.killActivityInMultiWindow(this, GalleryUtils.DONT_SUPPORT_VIEW_VIDEO);
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        if (view instanceof BookmarKItem) {
            String url = ((BookmarKItem) view).mUrl;
            openUrl(url);
        }
    }

    private void openUrl(String url) {
        Intent i = new Intent(this, MovieActivity.class);
        i.setData(Uri.parse(url));
        i.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        startActivity(i);
        finish();
    }
    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
        getMenuInflater().inflate(R.menu.bookmarkcontext, menu);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        View view = ((AdapterContextMenuInfo) item.getMenuInfo()).targetView;
        if (!(view instanceof BookmarKItem)) {
            return false;
        }
        BookmarKItem bookmarkItem = (BookmarKItem) view;
        switch (item.getItemId()) {
            case R.id.edit:
                editBookmark(bookmarkItem.mId, bookmarkItem.mTitle, bookmarkItem.mUrl);
                return true;
            case R.id.delete:
                deleteBookmark(bookmarkItem.mId);
                return true;
            default:
                return super.onContextItemSelected(item);
        }
    }
    private void deleteBookmark(long id) {
        getContentResolver().delete(MovieViewContentProvider.BOOKMARK_CONTENT_URI,
                MovieViewContentProvider._ID + "=" + id, null);
    }

    private void editBookmark(long id, String title, String url) {
        Intent intent = new Intent(this, EditBookmark.class);
        intent.putExtra(MovieViewContentProvider._ID, id);
        intent.putExtra(MovieViewContentProvider.BOOKMARK_TITLE, title);
        intent.putExtra(MovieViewContentProvider.BOOKMARK_URL, url);
        startActivity(intent);
    }
    @Override
    public Loader<Cursor> onCreateLoader(int id, Bundle args) {
        // TODO Auto-generated method stub
        if (id == LOADER_BOOKAMRKS) {
            CursorLoader loader = new CursorLoader(this,
                    MovieViewContentProvider.BOOKMARK_CONTENT_URI, PROJECTION, null, null, null);
            return loader;
        } else {
            Log.e(TAG, "onCreateLoader id error: " + id);
        }
        return null;
    }
    @Override
    public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
        // TODO Auto-generated method stub
        if (loader.getId() == LOADER_BOOKAMRKS) {
            mAdapter.changeCursor(data);
            if(mAdapter.isEmpty()){
                mEmpty.setVisibility(View.VISIBLE);
                mList.setVisibility(View.GONE);
            }else{
                mEmpty.setVisibility(View.GONE);
                mList.setVisibility(View.VISIBLE);
            }
        } else {
            Log.e(TAG, "onLoadFinished id error: " + loader.getId());
        }
    }
    @Override
    public void onLoaderReset(Loader<Cursor> loader) {
        // TODO Auto-generated method stub
        if (loader.getId() == LOADER_BOOKAMRKS) {
            mAdapter.changeCursor(null);
        }
    }
    class BookMarksAdapter extends CursorAdapter {

        LayoutInflater mInflater;

        Context mContext;

        BookMarksAdapter(Context context, Cursor cursor, int flags) {
            super(context, cursor, flags);
            mContext = context;
            mInflater = LayoutInflater.from(context);
        }

        @Override
        public View newView(Context context, Cursor cursor, ViewGroup parent) {
            // TODO Auto-generated method stub
            return new BookmarKItem(context);
        }

        @Override
        public void bindView(View view, Context context, Cursor cursor) {
            // TODO Auto-generated method stub
            if (view instanceof BookmarKItem) {
                BookmarKItem item = (BookmarKItem) view;
                item.setTitle(cursor.getString(COLUMN_INDEX_TITLE));
                item.setUrl(cursor.getString(COLUMN_INDEX_URL));
                item.mId = cursor.getLong(COLUMN_INDEX_ID);
            } else {
                Log.d(TAG, "bindView error: " + view.getClass().getName());
            }
        }
    }
    class BookmarKItem extends LinearLayout {
        TextView mTitleView;

        TextView mUrlView;

        long mId;

        String mTitle;

        String mUrl;

        BookmarKItem(Context context) {
            super(context);
            LayoutInflater factory = LayoutInflater.from(context);
            factory.inflate(R.layout.video_bookmark_item, this);
            mTitleView = (TextView) findViewById(R.id.title);
            mUrlView = (TextView) findViewById(R.id.url);
        }

        void setTitle(String title) {
            mTitle = title;
            mTitleView.setText(title);
        }

        void setUrl(String url) {
            mUrl = url;
            mUrlView.setText(url);
        }
    }

}

