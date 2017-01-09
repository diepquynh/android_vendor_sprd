package com.sprd.fileexplorer.adapters;

import java.io.File;
import java.util.Arrays;
import java.util.List;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.DialogInterface;
import android.app.AlertDialog;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.net.Uri;


import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.activities.FileSearchResultActivity;
import com.sprd.fileexplorer.drmplugin.SearchListAdapterUtils;
import com.sprd.fileexplorer.loadimage.ImageCache;
import com.sprd.fileexplorer.util.FileType;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexplorer.util.IntentUtil;
import com.sprd.fileexplorer.util.NotifyManager;

public class SearchListAdapter extends FileAdapter {
    private FileSearchResultActivity mContext;
    private boolean inSearchUi = true;
    private File mTopFile;
    private File mCurrentFile;
    private ListView mListView;
    
    private FileExplorerScrollListener mScrollListener;
    
    private FileExplorerImageLoadCompleteListener mLoadCompleteListener;

    private Drawable mDefaultImageIcon;
    private Drawable mDefaultFolderIcon;
    private Drawable mDefaultApkIcon;

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = super.getView(position, convertView, parent);
        ViewHolder vHolder = (ViewHolder) view.getTag(R.id.action_paste_mode_cancel);
        File file = mFileList.get(position);
        view.setTag(file.getPath());
        String filePath = file.getPath();
        if (file.isDirectory()) {
            vHolder.fileIcon.setImageDrawable(mDefaultFolderIcon);
        } else {
            int fileType = FileType.getFileType(mContext).getFileType(file);
            view.setTag(R.id.viewtag_filetype, fileType);
            vHolder.fileIcon.setImageResource(fileType);
            Bitmap bitmap = null;
            //for drm file set icon
            if(SearchListAdapterUtils.getInstance().DRMFileSetIcon(mContext,file,vHolder)){
                return view;
            }
            if(fileType == FileType.FILE_TYPE_IMAGE){
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
            } else if (fileType == FileType.FILE_TYPE_PACKAGE) {
                if (mScrolling) {
                    bitmap = ImageCache.get(file.getPath());
                } else {
                    bitmap = ImageCache.get(file.getPath());
                    if (null == bitmap) {
                        ImageCache.loadImageBitmap(mContext, file.getPath(),
                                mLoadCompleteListener, mMainThreadHandler,
                                false, position);
                    }
                }
                if (null == bitmap) {
                    vHolder.fileIcon.setImageDrawable(mDefaultApkIcon);
                } else {
                    vHolder.fileIcon.setImageBitmap(bitmap);
                }
            }
        }
        return view;
    }

    public SearchListAdapter(FileSearchResultActivity context, ListView listView) {
        super(context);
        mContext = context;
        mDefaultImageIcon = mContext.getResources().getDrawable(R.drawable.file_item_image_ic);
        mDefaultFolderIcon = mContext.getResources().getDrawable(R.drawable.file_item_folder_ic);
        mDefaultApkIcon = mContext.getResources().getDrawable(R.drawable.file_item_apk_default_ic);

        mListView = listView;
        mLoadCompleteListener = new FileExplorerImageLoadCompleteListener(mListView);
        mScrollListener = new FileExplorerScrollListener(mListView);
        mListView.setOnScrollListener(mScrollListener);
    }

    public void execute(int position) {
        if(mFileList == null) {
            return;
        }
        File executeFile = mFileList.get(position);
        if (executeFile == null ) {
            return;
        }

        if (executeFile.isDirectory()) {
            try {
                refresh(executeFile);
            } catch (Exception e) {
                NotifyManager.getInstance().showToast(
                        mContext.getString(R.string.fragment_search_invalid_result));
                Log.d(TAG, "in execute have Excepion", e);
            }
        } else {
            int fileType = FileType.getFileType(mContext).getFileType(executeFile);
            final Intent intent = IntentUtil.getIntentByFileType(mContext, fileType, executeFile);
            final String filePath = executeFile.getPath();
            //SPRD:585915 file:// uri is not allowed in 7.0, use content uri
            final Uri uri = FileUtil.getFileUri(executeFile, FileType.getTypeByFile(executeFile), mContext);
            final File file = executeFile;
            
          //drm file click item action 
            if(SearchListAdapterUtils.getInstance().DRMFileSendIntent(mContext,file,uri)){
                return;
            }
            if(intent != null){
                mContext.startActivity(intent);
            }else{
                NotifyManager.getInstance().showToast(mContext.getResources().getString(R.string.msg_invalid_intent));
            }
        }
    }

    public void pushToFileList(File file) {
        if(file.exists()){
            mFileList.add(file);
        }
    }

    public void clearFileList() {
        mFileList.clear();
    }

    public void refresh(File filePath){
        final SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(mContext);
        boolean displayHide = settings.getBoolean("display_hide_file", false);
        File fileArray[] = filePath.listFiles();
        List<File> fileList = Arrays.asList(fileArray);
        mFileList.clear();
        for (File file : fileList) {
            if(displayHide){
                if(file.exists() && !mFileList.contains(file)){
                    mFileList.add(file);
                }
            }else{
                if(!file.isHidden() && file.exists() && !mFileList.contains(file)){
                    mFileList.add(file);
                }
            }

        }
        if(inSearchUi){
            mTopFile = filePath;
            inSearchUi = false;
        }
        mCurrentFile = filePath;
        startSort();
        notifyDataSetChanged();
    }

    /**
     * Go up level.
     * 
     * @return Return if finish activity.
     */
    public boolean popupFolder() {
        if(mTopFile != null && mCurrentFile != null){
            if (mTopFile.getPath().equals(mCurrentFile.getPath()) && !inSearchUi) {
                mContext.searchFile();
                inSearchUi = true;
                return false;
            }else if(!inSearchUi){
                File parentFile = mCurrentFile.getParentFile();
                refresh(parentFile);
                mCurrentFile = parentFile;
                return false;
            }else{
                return true;
            }
        }else{
            return true;
        }

    }

    public File getCurrentPath(){
        return mCurrentFile;
    }

    public boolean getInSearchUi(){
        return inSearchUi;
    }
    
}
