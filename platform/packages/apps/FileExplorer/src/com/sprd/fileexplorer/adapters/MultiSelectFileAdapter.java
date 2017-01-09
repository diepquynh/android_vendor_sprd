
package com.sprd.fileexplorer.adapters;

import java.io.File;
import java.util.HashSet;
import java.util.Set;
import java.util.Stack;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.drmplugin.MultiSelectFileAdapterUtils;
import com.sprd.fileexplorer.fragments.DetailedListFragment;
import com.sprd.fileexplorer.loadimage.ImageCache;
import com.sprd.fileexplorer.util.FileType;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexplorer.util.StorageUtil;

public class MultiSelectFileAdapter extends FileAdapter {

    private String mCurrentFolderPath;

    private String mRootFolderPath;

    private boolean showHiddenFile;

    private Stack<String> mFolderStack;

    private static MultiSelectFileAdapter adapter = null;

    private Set<String> mCheckedItems = new HashSet<String>();

    private ListView mListView;

    private FileExplorerScrollListener mScrollListener;

    private FileExplorerImageLoadCompleteListener mLoadCompleteListener;

    private Drawable mDefaultImageIcon;
    private Drawable mDefaultFolderIcon;
    private Drawable mDefaultApkIcon;

    public MultiSelectFileAdapter(Context context, ListView listView) {
        super(context);
        mFolderStack = new Stack<String>();
        showHiddenFile = false;
        mDefaultImageIcon = mContext.getResources().getDrawable(R.drawable.file_item_image_ic);
        mDefaultFolderIcon = mContext.getResources().getDrawable(R.drawable.file_item_folder_ic);
        mDefaultApkIcon = mContext.getResources().getDrawable(R.drawable.file_item_apk_default_ic);
        mListView = listView;
        mLoadCompleteListener = new FileExplorerImageLoadCompleteListener(mListView);
        mScrollListener = new FileExplorerScrollListener(mListView);
        mListView.setOnScrollListener(mScrollListener);
    }

    @Deprecated
    public static String getCurrentFolderPath() {
        return adapter.mCurrentFolderPath;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = super.getView(position, convertView, parent);
        ViewHolder vHolder = (ViewHolder) view.getTag(R.id.action_paste_mode_cancel);
        /* SPRD 430982 @{ */
        if(mFileList.size() == 0){
            return view;
        }
        //File file = mFileList.get(position);
        File file = mFileList.get((position > mFileList.size() - 1) ? mFileList.size() - 1 : position);
        /* @} */
        view.setTag(file.getPath());
        Resources res = parent.getResources();
        String timeStr = FileUtil.SIMPLE_DATE_FOTMAT.format(file.lastModified());
        String filePath = file.getPath();
        if (file.isDirectory()) {
            vHolder.fileIcon.setImageDrawable(mDefaultFolderIcon);
            vHolder.fileMessage.setText(res.getString(
                    R.string.file_list_flodermsg, timeStr));
        } else {
            int fileType = FileType.getFileType(mContext).getFileType(file);
            view.setTag(R.id.viewtag_filetype, fileType);
            vHolder.fileIcon.setImageResource(fileType);
            Bitmap bitmap = null;
          //for drm file set icon
            if(MultiSelectFileAdapterUtils.getInstance().DRMFileSetIcon(mContext,file,vHolder)){
                vHolder.selectCb.setVisibility(View.VISIBLE);
                vHolder.selectCb.setChecked(isChecked(position));
                vHolder.fileIcon.setClickable(false);
                return view;
            }
            if (fileType == FileType.FILE_TYPE_IMAGE) {
                if (mScrolling) {
                    bitmap = ImageCache.get(file.getPath());
                } else {
                    bitmap = ImageCache.get(file.getPath());
                    if (null == bitmap) {
                        ImageCache.loadImageBitmap(mContext, file.getPath(), mLoadCompleteListener,
                                mMainThreadHandler, true, position);
                    }
                }
                if (null == bitmap) {
                    vHolder.fileIcon.setImageDrawable(mDefaultImageIcon);
                } else {
                    vHolder.fileIcon.setImageBitmap(bitmap);
                }
            }else if (fileType == FileType.FILE_TYPE_PACKAGE) {
                if (mScrolling) {
                    bitmap = ImageCache.get(file.getPath());
                } else {
                    bitmap = ImageCache.get(file.getPath());
                    if (null == bitmap) {
                        ImageCache.loadImageBitmap(mContext, file.getPath(), mLoadCompleteListener,
                                mMainThreadHandler, false, position);
                    }
                }
                if (null == bitmap) {
                    vHolder.fileIcon.setImageDrawable(mDefaultApkIcon);
                } else {
                    vHolder.fileIcon.setImageBitmap(bitmap);
                }
            }
        }
        vHolder.selectCb.setVisibility(View.VISIBLE);
        vHolder.selectCb.setChecked(isChecked(position));
        vHolder.fileIcon.setClickable(false);
        return view;
    }

    public void update() {
        updateFileList(new File(mCurrentFolderPath), false);
    }

    public void intoFolder(File folder) {
        updateFileList(folder, true);
    }

    private void updateFileList(File folder, boolean isRecord) {
        if (folder == null) {
            return;
        }
        if (!folder.exists() || !folder.isDirectory()) {
            return;
        }
        final SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(mContext);
        showHiddenFile = settings.getBoolean("display_hide_file", false);
        File[] allFiles = folder.listFiles();
        if (isRecord) {
            mFolderStack.push(mCurrentFolderPath);
        }
        mCurrentFolderPath = folder.getPath();
        synchronized (mFileList) {
            mFileList.clear();
            if (allFiles != null) {
                for (File f : allFiles) {
                    if (showHiddenFile || !f.isHidden()) {
                        if (f.exists()) {
                            mFileList.add(f);
                        }
                    }
                }
                startSort(new SortListener() {

                    @Override
                    public void onSortStarting() {

                    }

                    @Override
                    public void onSortFinished() {

                    }
                });
            }
        }
    }

    public void callBack() {
        if (mFolderStack.empty()) {
            return;
        }
        File preFile = new File(mFolderStack.pop());
        while (!preFile.exists() && !mFolderStack.isEmpty()) {
            preFile = new File(mFolderStack.pop());
        }
        updateFileList(preFile, false);
    }

    public void upBack() {
        if (mRootFolderPath.equals(mCurrentFolderPath)) {
            return;
        }
        File file = new File(mCurrentFolderPath);
        updateFileList(file.getParentFile(), true);
    }

    public void init(File folder) {
        mFolderStack.clear();
        mCurrentFolderPath = null;
        mRootFolderPath = folder.getPath();
        /*
         * SPRD: add 20131226 Spreadtrum of 232689 click "select more" may
         * display bad code and then normal @{
         */
        Log.d(TAG, "mRootFolderPath = " + mRootFolderPath);
        int type = DetailedListFragment.FRAGMENT_TYPE_INTERNAL;
        if (mRootFolderPath.startsWith(StorageUtil.getExternalStorage().getAbsolutePath())) {
            type = DetailedListFragment.FRAGMENT_TYPE_EXTERNAL;
        } else if (StorageUtil.inWhichUSBStorage(mRootFolderPath) != -1) {
            type = DetailedListFragment.FRAGMENT_TYPE_USB;
        }
        DetailedListFragment preFragment = DetailedListFragment.getInstance(type);
        if (preFragment != null && preFragment.getAdapter() != null) {
            /* SPRD: Modify for bug613249. @{ */
            //mFileList = preFragment.getAdapter().getFileList();
            synchronized (mFileList) {
                mFileList.clear();
                synchronized (preFragment.getAdapter().getFileList()) {
                    for (File f : preFragment.getAdapter().getFileList()) {
                        mFileList.add(f);
                    }
                }
            }
            /* @} */
        } else {
            updateFileList(folder, false);
        }
        /* @} */
    }

    /* SPRD: Add for Bug512310. @{ */
    public void refresh(){
        File file = new File(mRootFolderPath);
        updateFileList(file, false);
        notifyDataSetChanged();
    }
    /* @} */

    public boolean isEmptyStack() {
        return mFolderStack.isEmpty();
    }

    public boolean isShowHiddenFile() {
        return showHiddenFile;
    }

    public boolean isAtTop() {
        return mRootFolderPath.equals(mCurrentFolderPath);
    }

    public void setShowHiddenFile(boolean showHiddenFile) {
        this.showHiddenFile = showHiddenFile;
    }

    public boolean isChecked(int position) {
        File file = (File) getItem(position);
        if (file != null) {
            return mCheckedItems.contains(file.getAbsolutePath());
        } else {
            return false;
        }
    }

    public void setChecked(int position, boolean checked) {
        File file = (File) getItem(position);
        if (file != null) {
            if (checked) {
                mCheckedItems.add(file.getAbsolutePath());
            } else {
                mCheckedItems.remove(file.getAbsolutePath());
            }
        }
    }

    public void checkAll(boolean checked) {
        if (checked) {
            int count = getCount();
            for (int i = 0; i < count; i++) {
                File file = (File) getItem(i);
                if (file != null) {
                    mCheckedItems.add(file.getAbsolutePath());
                }
            }
        } else {
            mCheckedItems.clear();
        }
    }

    public boolean isAllChecked() {
        return (mCheckedItems.size() > 0 && mCheckedItems.size() == getCount()) ? true : false;
    }

}
