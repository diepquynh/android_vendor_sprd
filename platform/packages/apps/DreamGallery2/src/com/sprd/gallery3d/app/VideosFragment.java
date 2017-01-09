
package com.sprd.gallery3d.app;

import java.io.File;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.TreeMap;

import com.android.gallery3d.R;
import com.android.gallery3d.app.MovieActivity;

import com.android.gallery3d.util.GalleryUtils;
import com.sprd.gallery3d.drm.SomePageUtils;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.database.ContentObserver;
import android.drm.DrmManagerClient;
import android.drm.DrmStore;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.provider.MediaStore;
import android.util.Log;
import android.view.ActionMode;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.bumptech.glide.Glide;
import com.bumptech.glide.DrawableRequestBuilder;

public class VideosFragment extends Fragment implements OnItemClickListener,
        OnItemLongClickListener {
    private ListView mListView;
    private Context mContext;
    private VideoAdapter mAdapter;
    private static final String TAG = "VideosFragment";
    private ActionMode mActionMode;
    private Menu mMenu;
    private HashMap<Integer, Boolean> checkboxes = new HashMap<Integer, Boolean>();
    private Map<Integer, VideoItems> checkItem = new TreeMap<Integer, VideoItems>();
    private ArrayList<VideoItems> mVideoList = new ArrayList<VideoItems>();
    private String mFragmentName;
    private static final String FLAG_GALLERY = "startByGallery";
    private static final int REQUEST_TRIM = 6;
    public static final String KEY_MEDIA_ITEM_PATH = "media-item-path";
    private ImageView mImageView;
    private TextView mTextView;
    private String mOtgDevicePath;
    private static final String STATE_SAVE_IS_HIDDEN = "STATE_SAVE_IS_HIDDEN";
    private Activity mActivity;
    private DateFormat mFormater;
    private boolean mIsDeletingFiles = false;
    private VideoItems mItem = null;
    private int mVideoId = 0;
    private String mTitle = null;
    private AlertDialog mDeleteDialog;
    private AlertDialog mDetailsDialog;
    /*
     *  SPRD : add for self test: we must have a empty constructor here for instantiated
     * Unable to instantiate fragment com.sprd.gallery3d.app.VideosFragment @{
     */
    public VideosFragment() {
        Log.d(TAG, "Nullary constructor for VideosFragment");
    }
    /*@}*/

    public VideosFragment(ArrayList<VideoItems> videoList, String fragmentName) {
        // TODO Auto-generated constructor stub
        mVideoList = videoList;
        mFragmentName = fragmentName;
    }

    public int getFragmentId() {
        return this.getId();
    }

    public String getOtgDevicePath() {
        return mOtgDevicePath;
    }

    public void notifyChange() {
        if(mAdapter == null || mListView ==null)return;
        mAdapter.refreshVideoList();
        mAdapter.notifyDataSetChanged();
        mListView.setAdapter(mAdapter);
    }

    public VideosFragment(ArrayList<VideoItems> videoList, String otgdevicepath, String fragmentName) {
        mVideoList = videoList;
        mOtgDevicePath = otgdevicepath;
        mFragmentName = fragmentName;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate");
        super.onCreate(savedInstanceState);
        if (savedInstanceState != null) {
            boolean isSupportHidden = savedInstanceState.getBoolean(STATE_SAVE_IS_HIDDEN);
            FragmentTransaction ft = getFragmentManager().beginTransaction();
            if (isSupportHidden) {
                ft.hide(this);
            } else {
                ft.show(this);
            }
            ft.commit();
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean(STATE_SAVE_IS_HIDDEN, isHidden());
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        Log.d(TAG, "onCreateView()");
        View view = inflater.inflate(R.layout.all_videos_list, container, false);
        mListView = (ListView) view.findViewById(R.id.listView);
        mImageView = (ImageView) view.findViewById(R.id.imageView);
        mTextView = (TextView) view.findViewById(R.id.textView);
        mAdapter = new VideoAdapter();
        mListView.setAdapter(mAdapter);
        mListView.setOnItemClickListener(this);
        mListView.setOnItemLongClickListener(this);
        mFormater = new SimpleDateFormat(getResources().getString(R.string.date_format));
        if (mVideoList.size() == 0) {
            mListView.setVisibility(View.GONE);
            mImageView.setVisibility(View.VISIBLE);
            mTextView.setVisibility(View.VISIBLE);
            mTextView.setText(getResources().getString(R.string.no_videos));
        }
        return view;
    }

    @Override
    public void onAttach(Context context) {
        Log.d(TAG, "onAttach()");
        mActivity = (Activity)context;
        mContext = context;
        mContext.getContentResolver().registerContentObserver(
                MediaStore.Video.Media.EXTERNAL_CONTENT_URI, true, observer);
        super.onAttach(context);
    }

    @Override
    public void onResume() {
        Log.d(TAG, "onResume()");
        super.onResume();
        if (GalleryUtils.checkStoragePermissions(mContext)) {
            mAdapter.refreshVideoList();
            mAdapter.notifyDataSetChanged();
            updateActionModeIfNeed();
        }
    }

    private ContentObserver observer = new ContentObserver(new Handler()) {

        @Override
        public boolean deliverSelfNotifications() {
            return super.deliverSelfNotifications();
        }

        @Override
        public void onChange(boolean selfChange) {
            super.onChange(selfChange);
            Log.d(TAG, "onChange");
            /* SPRD:Add for bug595344 When clicking the gotonormalplay button,it shows wrong activity @{ */
            if (mAdapter == null) {
                return;
            }
            /* Bug595344 end @} */
            mAdapter.refreshVideoList();
            if (mVideoList.size() == 0) {
                mListView.setVisibility(View.GONE);
                mImageView.setVisibility(View.VISIBLE);
                mTextView.setVisibility(View.VISIBLE);
                // SPRD:Add for bug588637 The Gallery will crash when you remove the OTG cable in video player view
                mTextView.setText(R.string.no_videos);
                updateActionModeIfNeed();
            } else {
                mListView.setVisibility(View.VISIBLE);
                mImageView.setVisibility(View.GONE);
                mTextView.setVisibility(View.GONE);
                if (!mIsDeletingFiles) {
                    mAdapter.notifyDataSetChanged();
                }
                updateActionModeIfNeed();
            }
        }
    };

    public void playVideo(Activity activity, Uri uri, String title) {
        try {
            Intent intent = new Intent(Intent.ACTION_VIEW)
                    .setDataAndType(uri, "video/*")
                    .putExtra(Intent.EXTRA_TITLE, title)
                    .putExtra(FLAG_GALLERY, true)
                    .putExtra("mFragmentName", mFragmentName)
                    .putExtra("mOtgDevicePath", mOtgDevicePath)
                    .putExtra(MovieActivity.KEY_TREAT_UP_AS_BACK, true);
            // .putExtra(PermissionsActivity.UI_START_BY,startFrom);
            activity.startActivity(intent);
        } catch (ActivityNotFoundException e) {
            Toast.makeText(activity, activity.getString(R.string.video_err),
                    Toast.LENGTH_SHORT).show();
        }
    }

    public Uri getContentUri(int id) {
        Uri baseUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
        return baseUri.buildUpon().appendPath(String.valueOf(id)).build();
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        if (mActionMode == null && mVideoList.size() != 0) {
            Log.d(TAG, "mActionMode = " + mActionMode);
            String path = mVideoList.get(position).getUrl();
            mTitle = mVideoList.get(position).getDisplayName();
            mVideoId = mVideoList.get(position).getId();
            /* SPRDL: Add for DRM feature @{ */
            AlertDialog.OnClickListener onClickListener = new AlertDialog.OnClickListener() {

                @Override
                public void onClick(DialogInterface dialog, int which) {
                    // TODO Auto-generated method stub
                    playVideo(getActivity(), getContentUri(mVideoId), mTitle);
                }
            };
            if (SomePageUtils.getInstance().newCheckPressedIsDrm(mActivity, path, onClickListener,
                    false)) {
                return;
            } else {
                playVideo(getActivity(), getContentUri(mVideoId), mTitle);
            }
            Log.d(TAG, "getContentUri(videoId) =" + getContentUri(mVideoId));
        } else {
            CheckBox checkBox = (CheckBox) view.findViewById(R.id.checkBox);
            Log.d(TAG, "checkBox" + checkBox);
            if (null == checkBox) {
                return;
            }
            if (true == checkBox.isChecked()) {
                checkBox.setChecked(false);
            } else {
                checkBox.setChecked(true);
            }
            checkboxOnclick(position);
            /* SPRD: Delete for bug606821 Wrong logic in sharemenu showing @{ */
            if (checkItem.size() != 0) {
                for (VideoItems v : checkItem.values()) {
                    mItem = v;
                    mMenu.findItem(R.id.action_delete).setVisible(true);
                    /* SPRD: Delete for bug603592 Wrong logic in sharemenu showing @{ */
                    if (isDrmNotSupportShare(mItem.getUrl())) {
                        mMenu.findItem(R.id.action_share).setVisible(false);
                        break;
                    } else {
                        mMenu.findItem(R.id.action_share).setVisible(true);
                    }
                    /* Bug603592 end @} */
                }
            } else {
                mMenu.findItem(R.id.action_share).setVisible(false);
                mMenu.findItem(R.id.action_delete).setVisible(false);
            }
            /* Bug606821 end @} */
            checkIsDrmDetails();
            /* DRM feature end @} */
        }
    }

    /* SPRD: Add for drm new feature @{ */
    public void checkIsDrmDetails(){
        if (!SomePageUtils.getInstance().checkIsDrmFile(mItem.getUrl()) || checkItem.size() > 1 || checkItem.size() == 0) {
            mMenu.findItem(R.id.protection_information).setVisible(false);
        } else {
            mMenu.findItem(R.id.protection_information).setVisible(true);
        }
    }
    /* Drm new feature end @} */

    private void checkboxOnclick(int pos) {
        Boolean result = checkboxes.get(pos);
        if (result == null || result == false) {
            checkboxes.put(pos, true);
            VideoItems item = mVideoList.get(pos);
            checkItem.put(pos, item);
            Log.d(TAG, "the size of the checkItem=" + checkItem.size());
            mActionMode.setTitle(selectedNum());
        } else {
            checkboxes.put(pos, false);
            checkItem.remove(pos);
            Log.d(TAG, "the size of the checkItem=" + checkItem.size());
            mActionMode.setTitle(selectedNum());
        }
    }

    private String selectedNum() {
        String selectedText = getString(R.string.current_select, checkItem.size());
        return selectedText;
    }

    @Override
    public boolean onItemLongClick(AdapterView<?> parent, View view, int position,
            long id) {
        // TODO Auto-generated method stub
        Log.d(TAG, "mActionMode = " + mActionMode);
        if (mActionMode == null && mAdapter != null) {
            VideoItems item = mVideoList.get(position);
            mItem = item;
            checkItem.put(position, item);
            getActivity().startActionMode(new MyActionModeCallback());
            CheckBox checkBox = (CheckBox) view.findViewById(R.id.checkBox);
            checkBox.setChecked(true);
            checkboxes.put(position, true);
            Log.d(TAG, "checkItem=" + checkItem + "    position=" + position + "   item"
                    + item.getDisplayName() + "    number=" + checkItem.size());
            mActionMode.setTitle(selectedNum());
        } else {
            return false;
        }
        return true;
    }

    public class MyActionModeCallback implements ActionMode.Callback {

        @Override
        public boolean onCreateActionMode(ActionMode mode, Menu menu) {
            // TODO Auto-generated method stub
            mActionMode = mode;
            mMenu = menu;
            MenuInflater inflater = mode.getMenuInflater();
            inflater.inflate(R.menu.actionmode_share_delete, mMenu);
            checkIsDrmDetails();// SPRD:Add for drm new feature
            /* SPRD: Add for bug599941 non-sd drm videos are not supported to share @{ */
            MenuItem shareMenu = menu.findItem(R.id.action_share);
            if (isDrmNotSupportShare(mItem.getUrl())) {
                shareMenu.setVisible(false);
            } else {
                shareMenu.setVisible(true);
            }
            /* Bug599941 end @}　*/
            mAdapter.setCheckboxHidden(false);
            mAdapter.notifyDataSetChanged();
            return true;
        }

        @Override
        public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
            // TODO Auto-generated method stub
            if (checkItem.size() != mVideoList.size()) {
                menu.findItem(R.id.select_all).setTitle(getResources().getString(R.string.select_all));
            } else {
                menu.findItem(R.id.select_all).setTitle(getResources().getString(R.string.deselect_all));
            }
            return true;
        }

        @Override
        public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
            // TODO Auto-generated method stub
            if (item.getItemId() == R.id.action_share) {
                /* SPRD:Add for bug597680 When click the sharemenu several times,there will be many share dialogs @{ */
                if (VideoUtil.isFastClick()) {
                    return true;
                }
                /* Bug597680 @} */
                /* SPRD: Modify for subject tests @{ */
                Intent shareIntent = new Intent();
                Map<Integer, VideoItems> deleteItem = new TreeMap<Integer, VideoItems>(
                        checkItem);
                ArrayList<Uri> uris = new ArrayList<Uri>();
                for (Map.Entry<Integer, VideoItems> entry : deleteItem.entrySet()) {
                    String filePath = entry.getValue().getUrl();
                    uris.add(Uri.fromFile(new File(filePath)));
                }
                if (checkItem.size() > 1) {
                    shareIntent.setAction(Intent.ACTION_SEND_MULTIPLE);
                    shareIntent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, uris);
                } else {
                    shareIntent.setAction(Intent.ACTION_SEND);
                    shareIntent.putExtra(Intent.EXTRA_STREAM, uris.get(0));
                }
                shareIntent.setType("video/*");
                shareIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                startActivity(Intent.createChooser(shareIntent, getString(R.string.share)));
                /* Subject tests modification @} */
            } else if (item.getItemId() == R.id.action_delete) {
                /* SPRD:Add for bug597680 When click the sharemenu several times,there will be many share dialogs @{ */
                if (VideoUtil.isFastClick()) {
                    return true;
                }
                /* Bug597680 @} */
                AlertDialog.Builder dialog = new Builder(mContext);
                dialog.setTitle(getResources().getString(R.string.confirm))
                        .setMessage(R.string.delete_confirm)
                        .setPositiveButton(R.string.ok, new OnClickListener() {

                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                // TODO Auto-generated method stub
                                deleteFileAsync();
                            }
                        }).setNegativeButton(R.string.cancel, new OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        // TODO Auto-generated method stub
                        dialog.dismiss();
                    }
                });
                // SPRD:Add for bug605694 The dialog still exists when the actionmode is destroyed
                mDeleteDialog = dialog.show();
            } else if (item.getItemId() == R.id.select_all) {
                if (item.getTitle().equals(getResources().getString(R.string.select_all))) {
                    for (int i = 0; i < mVideoList.size(); i++) {
                        checkboxes.put(i, true);
                        VideoItems videoItem = mVideoList.get(i);
                        checkItem.put(i, videoItem);
                    }
                    item.setTitle(getResources().getString(R.string.deselect_all));
                    // SPRD:Delete for bug606821 Wrong logic in sharemenu showing
                    mMenu.findItem(R.id.action_delete).setVisible(true);
                } else {
                    for (int i = 0; i < mVideoList.size(); i++) {
                        checkboxes.clear();
                        checkItem.clear();
                        item.setTitle(getResources().getString(R.string.select_all));
                    }
                    // SPRD:Delete for bug606821 Wrong logic in sharemenu showing
                    mMenu.findItem(R.id.action_delete).setVisible(false);
                    //SPRD:Bug609412 Select all then cancel select all and click share_button,the VideoPlayer is crash
                    mMenu.findItem(R.id.action_share).setVisible(false);
                }
                mActionMode.setTitle(selectedNum());
                mAdapter.notifyDataSetChanged();
                /* SPRD:Add for drm new feature @{ */
                checkIsDrmDetails();
                /* SPRD: Delete for bug606821 Wrong logic in sharemenu showing @{ */
                for (VideoItems v : checkItem.values()) {
                    mItem = v;
                    if (isDrmNotSupportShare(mItem.getUrl())) {
                        mMenu.findItem(R.id.action_share).setVisible(false);
                        break;
                    } else {
                        mMenu.findItem(R.id.action_share).setVisible(true);
                    }
                }
                /* Bug606821 end @} */
            } else if (item.getItemId() == R.id.protection_information) {
                boolean isDrmVideoRightsValidity = SomePageUtils.getInstance().checkIsDrmFileValid(mItem.getUrl());
                boolean isDrmSupportTransfer = SomePageUtils.getInstance().isDrmSupportTransfer(mItem.getUrl());
                DrmManagerClient client = SomePageUtils.getInstance().getDrmManagerClient();
                ContentValues value = client.getConstraints(mItem.getUrl(), DrmStore.Action.PLAY);
                byte[] clickTime = value.getAsByteArray(DrmStore.ConstraintsColumns.EXTENDED_METADATA);

                Object drmStartTime = null;
                Object drmEndTime = null;
                Long startTime = null;
                Long endTime = null;
                if (value != null) {
                    startTime = value.getAsLong(DrmStore.ConstraintsColumns.LICENSE_START_TIME);
                    endTime = value.getAsLong(DrmStore.ConstraintsColumns.LICENSE_EXPIRY_TIME);
                }
                drmStartTime = SomePageUtils.getInstance().newTransferDate(startTime, NewVideoActivity.getNewVideoActivity());
                drmEndTime = SomePageUtils.getInstance().newTransferDate(endTime, NewVideoActivity.getNewVideoActivity());
                Object drmExpirationTime = SomePageUtils.getInstance().newCompareDrmExpirationTime(value.get(DrmStore.ConstraintsColumns.LICENSE_AVAILABLE_TIME), clickTime, NewVideoActivity.getNewVideoActivity());
                Object drmRemain  = SomePageUtils.getInstance().newCompareDrmRemainRight(mItem.getUrl(), value.get(DrmStore.ConstraintsColumns.REMAINING_REPEAT_COUNT), NewVideoActivity.getNewVideoActivity());
                View view = LayoutInflater.from(mContext).inflate(R.layout.dialog_view, null);
                TextView fileName = (TextView) view.findViewById(R.id.fileName);
                TextView rightsValidity = (TextView) view.findViewById(R.id.rightsValidity);
                TextView isTransferAllowed = (TextView) view.findViewById(R.id.isTransferAllowed);
                TextView startTimeView = (TextView) view.findViewById(R.id.startTime);
                TextView endTimeView = (TextView) view.findViewById(R.id.endTime);
                TextView expirationTimeView = (TextView) view.findViewById(R.id.expirationTime);
                TextView remainTime = (TextView) view.findViewById(R.id.remainTime);

                fileName.setText(getDrmString(R.string.file_name)+mItem.getDisplayName());
                rightsValidity.setText(getDrmString(R.string.rights_validity) + (isDrmVideoRightsValidity?getDrmString(R.string.rights_validity_valid):getDrmString(R.string.rights_validity_invalid)));
                isTransferAllowed.setText(getDrmString(R.string.rights_status) + (isDrmSupportTransfer?getDrmString(R.string.rights_status_share):getDrmString(R.string.rights_status_not_share)));
                startTimeView.setText(getDrmString(R.string.start_time) + drmStartTime);
                endTimeView.setText(getDrmString(R.string.end_time) + drmEndTime);
                expirationTimeView.setText(getDrmString(R.string.expiration_time) + drmExpirationTime);
                remainTime.setText(getDrmString(R.string.remain_times) + drmRemain);

                AlertDialog.Builder dialog = new AlertDialog.Builder(mContext);
                dialog.setTitle(R.string.drm_info).setView(view).setNegativeButton(R.string.close, new OnClickListener() {

                    public void onClick(DialogInterface dialog, int which) {
                        // TODO Auto-generated method stub
                        dialog.dismiss();
                    }
                }).create();
                // SPRD:Add for bug605694 The dialog still exists when the actionmode is destroyed
                mDetailsDialog = dialog.show();
                /* Drm new feature end @} */
            }
            return true;
        }

        @Override
        public void onDestroyActionMode(ActionMode mode) {
            VideoUtil.updateStatusBarColor(NewVideoActivity.getNewVideoActivity(), true);
            if (mAdapter != null) {
                mAdapter.setCheckboxHidden(true);
                mAdapter.notifyDataSetChanged();
                clearContainer();
                mActionMode = null;
            }
        }
    }

    private void deleteFile(VideoItems item) {
        Log.d(TAG,
                "StorageInfos.isExternalStorageMounted()="
                        + StorageInfos.isExternalStorageMounted()
                        + "      !StorageInfos.isInternalStorageSupported()"
                        + !StorageInfos.isInternalStorageSupported());
        if (StorageInfos.isExternalStorageMounted() || !StorageInfos.isInternalStorageSupported()) {
            mContext.getContentResolver().delete(
                    ContentUris.withAppendedId(MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
                            item.getId()), null, null);
        } else {
            mContext.getContentResolver().delete(
                    ContentUris.withAppendedId(MediaStore.Video.Media.INTERNAL_CONTENT_URI,
                            item.getId()), null, null);
        }
        File del = new File(item.getUrl());
        Log.d(TAG, "!del.exists()=" + !del.exists() + "        !del.delete()="
                + !del.delete());
        deleteById(item.getId());
    }

    private void deleteById(int id) {
        // TODO Auto-generated method stub
        if (mVideoList != null) {
            Iterator<VideoItems> it = mVideoList.iterator();
            while (it.hasNext()) {
                VideoItems del = it.next();
                if (id == del.getId()) {
                    it.remove();
                    Log.d(TAG, "delete successfully");
                    break;
                }
            }
        }
    }

    private void clearContainer() {
        if (checkboxes != null) {
            checkboxes.clear();
        }
        if (checkItem != null) {
            checkItem.clear();
        }
    }

    private void invalidateCheckbox(CheckBox box, int pos) {
        Log.d(TAG, "checkboxes=" + checkboxes);
        Boolean result = checkboxes.get(pos);
        if (result == null || result == false) {
            box.setChecked(false);
        } else {
            VideoItems item = mVideoList.get(pos);
            checkItem.put(pos, item);
            box.setChecked(true);
        }
    }

    private void deleteFileAsync() {
        AsyncTask<Void, Integer, Void> task = new AsyncTask<Void, Integer, Void>() {
            ProgressDialog dialog;
            private Map<Integer, VideoItems> deleteItem = new TreeMap<Integer, VideoItems>(
                    checkItem);

            @Override
            protected Void doInBackground(Void... params) {
                mIsDeletingFiles = true;
                Integer i = 0;
                Log.d(TAG, "the size of the checkItem=" + checkItem.size());
                for (Map.Entry<Integer, VideoItems> entry : deleteItem.entrySet()) {
                    Log.d(TAG, "delete the video：" + entry.getValue().getUrl());
                    deleteFile(entry.getValue());
                    publishProgress(++i);
                }
                return null;
            }

            @Override
            protected void onPreExecute() {
                dialog = new ProgressDialog(mContext);
                dialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
                dialog.setTitle(R.string.wait_for_deleting);
                dialog.setCancelable(false);
                dialog.setMax(deleteItem.size());
                dialog.show();
                super.onPreExecute();
            }

            @Override
            protected void onPostExecute(Void result) {
                super.onPostExecute(result);
                dialog.cancel();
                deleteFinish();
            }

            @Override
            protected void onProgressUpdate(Integer... values) {
                super.onProgressUpdate(values);
                dialog.setProgress(values[0]);
            }

        };
        task.execute((Void[]) null);
    }

    private void deleteFinish() {
        if (mActionMode != null) {
            mActionMode.finish();
        }
        clearContainer();
        mListView.setVisibility(View.GONE);
        mAdapter.refreshVideoList();
        mAdapter.notifyDataSetChanged();
        mListView.setVisibility(View.VISIBLE);
        mIsDeletingFiles = false;
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy()");
        mContext.getContentResolver().unregisterContentObserver(observer);
        super.onDestroy();
    }

    @Override
    public void onStop() {
        Log.d(TAG, "onStop()");
        if(mActionMode != null){
            mAdapter.setCheckboxHidden(true);
            mAdapter.notifyDataSetChanged();
            clearContainer();
            mActionMode.finish();
            mActionMode = null;
            /* SPRD:Add for bug605694 The dialog still exists when the actionmode is destroyed @{ */
            if (mDeleteDialog != null) {
                mDeleteDialog.dismiss();
                mDeleteDialog = null;
            }
            if (mDetailsDialog != null) {
                mDetailsDialog.dismiss();
                mDetailsDialog = null;
            }
            /* Bug605694 end @} */
        }
        super.onStop();
    }

    class VideoAdapter extends BaseAdapter {
        private LayoutInflater inflater;
        private String imageUrl;
        private boolean isCheckboxHidden = true;
        private String mCurrentFragment;
        private static final String ALL_VIDEOS_FRAGMENT = "AllVideosFragment";
        private static final String LOCAL_VIDEOS_FRAGMENT = "LocalVideosFragment";
        private static final String FILMED_VIDEOS_FRAGMENT = "FilmedVideosFragment";
        private static final String OTG_VIDEOS_FRAGMENT = "OtgVideosFragment";

        public void setCheckboxHidden(boolean flag) {
            isCheckboxHidden = flag;
        }

        public String getStringTime(int duration) {
            return VideoUtil.calculatTime(duration);
        }

        public VideoAdapter() {
            inflater = LayoutInflater.from(mContext);
        }

        @Override
        public int getCount() {
            return mVideoList.size();
        }

        @Override
        public Object getItem(int position) {
            return mVideoList.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            ViewHolder holder;
            if (convertView == null) {
                convertView = inflater.inflate(R.layout.videos_list_item, parent,
                        false);
                holder = new ViewHolder();
                holder.mDisplayName = (TextView) convertView
                        .findViewById(R.id.title);
                holder.mDuration = (TextView) convertView
                        .findViewById(R.id.duration);
                holder.mVideoSize = (TextView) convertView.findViewById(R.id.size);
                holder.mTimeModified = (TextView) convertView.findViewById(R.id.date_modified);
                holder.mThumbnail = (ImageView) convertView
                        .findViewById(R.id.imageView);
                holder.mDrmLock = (ImageView) convertView.findViewById(R.id.drmlock);
                holder.mCheckBox = (CheckBox) convertView.findViewById(R.id.checkBox);
                convertView.setTag(holder);
            } else {
                holder = (ViewHolder) convertView.getTag();
            }

            String displayName = mVideoList.get(position).getDisplayName();
            String duration = mVideoList.get(position).getDuration();
            long videoSize = mVideoList.get(position).getSize();
            String dateModified = mVideoList.get(position).getDate_modified();
            holder.mDisplayName.setText(displayName);
            if (duration == null) {
                duration = getResources().getString(R.string.unknown);
            } else {
                duration = getStringTime(Integer.parseInt(duration));
            }
            holder.mDuration.setText(duration);
            holder.mTimeModified.setText(mFormater.format(new Date(Long.parseLong(dateModified) * 1000)));
            holder.mVideoSize.setText(VideoUtil.getStringVideoSize(mContext, videoSize));
            imageUrl = mVideoList.get(position).getUrl();
            Log.d(TAG, "imageUrl=" + imageUrl);
            DrawableRequestBuilder requestBuilder = Glide.with(mContext).load(imageUrl);
            requestBuilder.into(holder.mThumbnail);
            /* SPRD: Add for DRM feature @{ */
            if (SomePageUtils.getInstance().checkIsDrmFile(imageUrl)) {
                holder.mDrmLock.setVisibility(View.VISIBLE);
                if (SomePageUtils.getInstance().checkIsDrmFileValid(imageUrl)) {
                    holder.mDrmLock.setImageResource(R.drawable.ic_drm_unlock);
                } else {
                    holder.mDrmLock.setImageResource(R.drawable.ic_drm_lock);
                }
            } else {
                holder.mDrmLock.setVisibility(View.GONE);
            }
            /* DRM feature end @} */
            invalidateCheckbox(holder.mCheckBox, position);
            if (!isCheckboxHidden) {
                holder.mCheckBox.setVisibility(View.VISIBLE);
            } else {
                holder.mCheckBox.setChecked(false);
                holder.mCheckBox.setVisibility(View.GONE);
            }
            return convertView;
        }

        public void refreshVideoList() {
            /* SPRD: Add for bug593851 boolean java.lang.String.equals(java.lang.Object) @{ */
            if(mFragmentName == null){
                return;
            }
            /* Bug593851 end @} */
            if (mFragmentName.equals(ALL_VIDEOS_FRAGMENT)) {
                Log.d(TAG, "refreshVideoList----AllVideosFragment");
                mVideoList = VideoUtil.getVideoList(mContext);
            } else if (mFragmentName.equals(LOCAL_VIDEOS_FRAGMENT)) {
                Log.d(TAG, "refreshVideoList----LocalVideosFragment");
                mVideoList = VideoUtil.getLocalVideos(VideoUtil.getVideoList(mContext));
            } else if (mFragmentName.equals(FILMED_VIDEOS_FRAGMENT)) {
                Log.d(TAG, "refreshVideoList----FilmedVideosFragment");
                mVideoList = VideoUtil.getFilmedVideos(VideoUtil.getVideoList(mContext));
            } else if(mFragmentName.equals(OTG_VIDEOS_FRAGMENT)){
                mVideoList = VideoUtil.getOtgVideos(VideoUtil.getVideoList(mContext),mOtgDevicePath);
            } else if (mFragmentName.equals("HistoryVideosFragment")) {
            }
        }
    }

    private static class ViewHolder {
        TextView mDisplayName;
        TextView mDuration;
        TextView mVideoSize;
        TextView mTimeModified;
        ImageView mThumbnail;
        ImageView mDrmLock;
        CheckBox mCheckBox;
    }

    public void updateActionModeIfNeed(){
        if (mVideoList.size() == 0 && mActionMode != null && mAdapter != null) {
            clearContainer();
            mActionMode.finish();
            mActionMode = null;
        } else {
            if (mActionMode != null) {
                mActionMode.setTitle(selectedNum());
            }
        }
    }

    public int getIntFromDimens(int index) {
        int result = this.getResources().getDimensionPixelSize(index);
        return result;
    }

    /* SPRD: Add for drm new feature @{ */
    public String getDrmString(int id){
        return mContext.getResources().getString(id);
     }
    /* Drm new feature end @}*/

    /* SPRD: Delete for bug603592 Wrong logic in sharemenu showing @{ */
    public boolean isDrmNotSupportShare(String filePath) {
        return SomePageUtils.getInstance().checkIsDrmFile(filePath) && !SomePageUtils.getInstance().newIsSupportShare(filePath);
    }
    /* Bug603592 end @} */
}
