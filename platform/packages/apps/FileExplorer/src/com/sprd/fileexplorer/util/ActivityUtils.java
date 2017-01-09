package com.sprd.fileexplorer.util;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.widget.BaseAdapter;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.activities.FilePasteActivity;
import com.sprd.fileexplorer.activities.FileSearchResultActivity;
import com.sprd.fileexplorer.adapters.CopyDestListAdapter;

public class ActivityUtils {

    private static String TAG = "ActivityUtils";
    /* SPRD 454659  @{ */
    public static int INTERNAL_STORAGE_AVAILABLE = 1;
    public static int EXTERNAL_STORAGE_AVAILABLE = 1 << 1;
    public static int USB_STORAGE_AVAILABLE = 1 << 2;
    /* @} */
    // SPRD: Add for bug601415, save the selection dialog.
    public static AlertDialog mSelectDialog;
    public static final int COPY_PATH_RESULT = 1;
    public static void showDestSelectDialog(final Context context,
            DialogInterface.OnClickListener listener) {
        DialogInterface.OnCancelListener clistener = (DialogInterface.OnCancelListener)listener;
        showDestSelectDialog(context,listener,clistener);
    }
    private static void showDestSelectDialog(final Context context,
            DialogInterface.OnClickListener listener,DialogInterface.OnCancelListener clistener) {
        if (context != null) {
            /* SPRD 454659  @{ */
            //BaseAdapter adapter = new CopyDestListAdapter();
            BaseAdapter adapter = new CopyDestListAdapter(getAvailableStatus());
            /* @} */
            // SPRD: Add for bug601415, save the selection dialog.
            mSelectDialog = new AlertDialog.Builder(context)
            .setTitle(R.string.select_destination)
            .setAdapter(adapter, listener)
            .setOnCancelListener(clistener)
            .show();
        }
    }
    /* SPRD 454659  @{ */
    public static int getAvailableStatus(){
        int i = 0;
        if(StorageUtil.getInternalStorageState()){
            i |= INTERNAL_STORAGE_AVAILABLE;
        }

        if(StorageUtil.getExternalStorageState()){
            i |= EXTERNAL_STORAGE_AVAILABLE;
        }
        if(StorageUtil.getUSBStorageState()){
            i |= USB_STORAGE_AVAILABLE;
        }

        Log.i(TAG, "getAvailableStatus i:"+i);
        return i;
    }
    /* @} */

    public static void startCopyActivity(Context context, boolean isCut,
            /* SPRD 454659  @{ */
            //boolean toSdcard, String copyFilePath) {
            int which, String copyFilePath) {
            /* @} */
        Intent intent = new Intent(context, FilePasteActivity.class);
        int flag = 0;
        if(isCut) {
            flag |= FilePasteActivity.FLAG_CUT_OP;
        }
        /* SPRD 454659  @{ */
        if(which == 1){
            /* SPRD: Modify for bug495412. @{ */
            int storageFlag = getAvailableStatus();
            if((storageFlag & ActivityUtils.EXTERNAL_STORAGE_AVAILABLE) != 0){
                flag |= FilePasteActivity.FLAG_TO_SDCARD;
            }else if((storageFlag & ActivityUtils.USB_STORAGE_AVAILABLE) != 0){
                flag |= FilePasteActivity.FLAG_TO_USB;
            }
            /* @} */
        }
        if(which == 2) {
            flag |= FilePasteActivity.FLAG_TO_USB;
        }
        /* @} */
        intent.putExtra(FilePasteActivity.PASTE_FLAG, flag);
        intent.putExtra(FilePasteActivity.PASTE_SOURCE_PATH, copyFilePath);
        // SPRD: Modify for bug465956.
        ((Activity) context).startActivityForResult(intent,ActivityUtils.COPY_PATH_RESULT);
    }

    public static void startCopyActivity(Context context, boolean isCut,
            /* SPRD 454659  @{ */
            //boolean toSdcard, String[] copyFilePaths) {
            int which, String[] copyFilePaths) {
            /* @} */
        if(copyFilePaths.length == 0) {
            Log.w(TAG, "call startCopyActivity(), but copyFilePaths is empty");
            return;
        } else if(copyFilePaths.length == 1) {
        	// SPRD 454659
            startCopyActivity(context, isCut, which, copyFilePaths[0]);
            return;
        }
        Intent intent = new Intent(context, FilePasteActivity.class);
        int flag = FilePasteActivity.FLAG_MULT_OP;
        if(isCut) {
            flag |= FilePasteActivity.FLAG_CUT_OP;
        }
        /* SPRD 454659  @{ */
        if(which == 1){
            /* SPRD: Modify for bug495412. @{ */
            int storageFlag = getAvailableStatus();
            if((storageFlag & ActivityUtils.EXTERNAL_STORAGE_AVAILABLE) != 0){
                flag |= FilePasteActivity.FLAG_TO_SDCARD;
            }else if((storageFlag & ActivityUtils.USB_STORAGE_AVAILABLE) != 0){
                flag |= FilePasteActivity.FLAG_TO_USB;
            }
            /* @} */
        }
        if(which == 2) {
            flag |= FilePasteActivity.FLAG_TO_USB;
        }
        /* @} */
        /* SPRD: Add for bug603830, send the source paths. @{ */
        Bundle bundle = new Bundle();
        bundle.putStringArray(FilePasteActivity.PASTE_SOURCE_PATHS, copyFilePaths);
        intent.putExtras(bundle);
        /* @} */
        CopyFilePath.copyFilePathsTran = copyFilePaths;
        intent.putExtra(FilePasteActivity.PASTE_FLAG, flag);
        intent.putExtra(FilePasteActivity.PASTE_LARGE_PATH, "LARGE_PATH");
        // SPRD: Modify for bug465956.
        ((Activity) context).startActivityForResult(intent, ActivityUtils.COPY_PATH_RESULT);
    }

    public static void startSearchMode(Bundle bun, Context context) {
        if (context != null && bun != null) {
            ((Activity) context).startActivity(new Intent().setClass(
                    context.getApplicationContext(),
                    FileSearchResultActivity.class).putExtra(
                    FileSearch.SEARCH_ATTACH, bun));
        }
    }

    public static class CopyFileListener implements DialogInterface.OnClickListener,DialogInterface.OnCancelListener {

        private Context mStartContext;

        private boolean mIsCut;

        private List<String> mOpFilesPath = new ArrayList<String>();

        public CopyFileListener(Context startContext) {
            super();
            this.mStartContext = startContext;
        }

        public void setCut(boolean isCut) {
            mIsCut = isCut;
        }

        public void addOpFiles(List<File> files) {
            for(File f: files) {
                mOpFilesPath.add(f.getAbsolutePath());
            }
        }

        public void addOpFile(File file) {
            mOpFilesPath.add(file.getAbsolutePath());
        }

        @Override
        public void onClick(DialogInterface dialog, int which) {
            /* SPRD 454659  @{ */
            //startCopyActivity(context, isCut, toSdcard, copyFilePaths[0]);
            startCopyActivity(mStartContext, mIsCut, which, mOpFilesPath.toArray(new String[0]));
            /* @} */
            clear();
        }
        @Override
        public void onCancel(DialogInterface arg0) {
            clear();
        }

        private void clear() {
            mIsCut = false;
            mOpFilesPath.clear();
        }
    }

    public static class CopyFilePath {
        public static String[] copyFilePathsTran;
    }
}
