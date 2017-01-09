/** SPRD:Bug474632 bookmark setting  Create*/
package com.sprd.gallery3d.app;

import com.android.gallery3d.R;
import com.android.gallery3d.util.GalleryUtils;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.ContentValues;
import android.content.res.Resources;
import android.database.Cursor;
import android.os.AsyncTask;
import android.os.Bundle;
import android.text.InputFilter;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

public class EditBookmark extends Activity{
    private final String TAG = "EditBookmark";

    private EditText mTitleView;

    private EditText mAddressView;

    private String mTitle;

    private String mUrl;

    private long mId;
    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.edit_bookmark);
        Bundle bundle = getIntent().getExtras();
        if (bundle != null) {
            mTitle = bundle.getString(MovieViewContentProvider.BOOKMARK_TITLE);
            mUrl = bundle.getString(MovieViewContentProvider.BOOKMARK_URL);
            mId = bundle.getLong(MovieViewContentProvider._ID);
        } else {
            mTitle = "";
            mUrl = "";
            mId = -1;
        }

        mTitleView = (EditText) findViewById(R.id.title);
        mTitleView.setText(mTitle);
        mTitleView.setFilters(new InputFilter[] {
            new InputFilter.LengthFilter(4096)
        });
        mAddressView = (EditText) findViewById(R.id.address);
        mAddressView.setText(mUrl);
        mAddressView.setFilters(new InputFilter[] {
            new InputFilter.LengthFilter(4096)
        });

        Button save = (Button) findViewById(R.id.OK);
        save.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                saveBookmark();
            }
        });

        Button cancel = (Button) findViewById(R.id.cancel);
        cancel.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                finish();
            }
        });
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

    void save(String title, final String url) {
        ContentValues values = new ContentValues();
        values.put(MovieViewContentProvider.BOOKMARK_TITLE, title);
        values.put(MovieViewContentProvider.BOOKMARK_URL, url);
        getContentResolver().update(MovieViewContentProvider.BOOKMARK_CONTENT_URI, values,
                MovieViewContentProvider._ID + "=" + mId, null);
        finish();
    }

    private void saveBookmark() {
        String title = mTitleView.getText().toString().trim();
        String url = mAddressView.getText().toString().trim();
        boolean emptyTitle = title.length() == 0;
        boolean emptyUrl = url.length() == 0;
        Resources r = getResources();
        if (emptyTitle || emptyUrl) {
            if (emptyTitle) {
                mTitleView.requestFocus();
                mTitleView.setError(r.getText(R.string.bookmark_needs_title));
            }
            if (emptyUrl) {
                mAddressView.requestFocus();
                mAddressView.setError(r.getText(R.string.bookmark_needs_url));
            }
            return;
        }

        if (!url.startsWith("http://")
                && !url.startsWith("rtsp://")
                && !url.startsWith("https://")) {
            mAddressView.requestFocus();
            mAddressView.setError(r.getText(R.string.bookmark_invalid_url));
            return;
        }

        if (mTitle.equals(title) && mUrl.equals(url)) {
            Log.d(TAG, "bookmark do not change, just return");
            finish();
            return;
        }
        if (mId < 0) {
            Log.e(TAG, "invalid id, return");
            finish();
            return;
        }

        postsave(title, url);
    }

    private void postsave(final String title, final String url) {
        // TODO Auto-generated method stub
        CharSequence message = getApplicationContext().getText(R.string.processing_please_wait);
        final ProgressDialog dialog = ProgressDialog.show(EditBookmark.this, null, message, true,
                true, null);
        dialog.setCanceledOnTouchOutside(false);

        new AsyncTask<Void, Void, Cursor>() {
            protected Cursor doInBackground(Void... unused) {
                Cursor cursor = getContentResolver().query(
                        MovieViewContentProvider.BOOKMARK_CONTENT_URI, new String[] {
                            MovieViewContentProvider._ID
                        }, "url = ?", new String[] {
                            url
                        }, null);
                return cursor;
            }

            protected void onPostExecute(Cursor cursor) {
                if (dialog != null) {
                    if (dialog.isShowing()) {
                        dialog.dismiss();
                    } else {
                        return;
                    }
                }
                if (cursor == null) {
                    Log.d(TAG, "cursor null update");
                    save(title, url);
                    return;
                }
                int count = cursor.getCount();
                if (count == 0) {
                    Log.d(TAG, "cursor 0 update");
                    cursor.close();
                    save(title, url);
                    return;
                }

                long id = -1;
                if (cursor.moveToFirst()) {
                    id = cursor.getLong(0);
                }
                cursor.close();
                if (count == 1 && id == mId) {
                    Log.d(TAG, "save title changed:" + id);
                    save(title, url);
                    return;
                } else if (count > 0) {
                    Log.d(TAG, "save same exist:" + id);
                    mAddressView.requestFocus();
                    mAddressView.setError(getResources().getText(R.string.bookmark_exist));
                }
            }
        }.execute();
    }
}
