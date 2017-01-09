/*
 *   Copyright (C) 2010,2012 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.uphoto.preference;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Environment;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

import com.ucamera.uphoto.R;

import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.ListIterator;

public class MyPathPreference extends MyDialogPreference {
    private static final String TAG = "MyPathPreference";
    private Context mContext;
    private MyAlertDialog mAlertDialog;

    private Button mBtnBack = null;
    private Button mBtnConfirm = null;
    private TextView mCurrentDirTextView = null;
    private ListView mFolderListView = null;
    private FileListAdapter folderAdapter = null;
    private List<String> mFolderList = null;
    private String ROOT_DIR = null; //Environment.getExternalStorageDirectory().toString();
    private String mCurrentDir = null; //Environment.getExternalStorageDirectory().toString();
    private boolean mIsSingleSdcard = false;

    public MyPathPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        getRootDir();
        mCurrentDir = ROOT_DIR;
    }

    public MyPathPreference(Context context) {
        this(context, null);
    }

    /**
     * get root director according Environment.getExternalStorageDirectory()
     */
    public void getRootDir() {
        /*
         * FIX BUG: 479
         * BUG CAUSE: Some phones has a internal sdcard, so Environment.getExternalStorageDirectory()
         * returns the internal sdcard's path
         * FIX COMMENT: Relocate the root directory to sdcard'parent
         * DATE: 2012-02-08
         */
        String strTemp = Environment.getExternalStorageDirectory().toString();
        ROOT_DIR = strTemp.substring(0, strTemp.lastIndexOf("/"));
        if (ROOT_DIR == null || ROOT_DIR.length() == 0) {
            ROOT_DIR = "/";
        }
        List<String> folderList = getAllFolderItems(ROOT_DIR);
        if (folderList != null && folderList.size() == 1) {
            ROOT_DIR = strTemp;
            mIsSingleSdcard = true;
        }
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);

        mFolderListView = (ListView) view.findViewById(R.id.lv_folder);
        mCurrentDirTextView = (TextView) view.findViewById(R.id.tv_view_current_dir);
        mCurrentDirTextView.setText(mCurrentDir);
        mFolderListView.setOnItemClickListener(new FolderOnItemClickListener());
        folderAdapter = new FileListAdapter(mContext);
        mFolderListView.setAdapter(folderAdapter);
        updateAdapter(ROOT_DIR);

        mBtnBack = (Button) view.findViewById(R.id.btn_back_to_parent_dir);
        mBtnBack.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                onClickBackBtn();
            }
        });

        mBtnConfirm = ((Button) view.findViewById(R.id.btn_confirm));
        mBtnConfirm.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mAlertDialog.dismiss();
                setSummary(mCurrentDir);
                setValue(mCurrentDir);
//                To do
//                ImageManager.updateBucketInfo(mContext);
                clear();
            }
        });
        if (!mIsSingleSdcard) {
            mBtnConfirm.setEnabled(false);
        }
        ((Button) view.findViewById(R.id.btn_cancel)).setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mAlertDialog.dismiss();
                clear();
            }
        });
    }

    @Override
    protected View onCreateDialogView() {
        super.onCreateDialogView();

        final LayoutInflater layoutInflater =
            (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mAlertDialog = getAlertDialog();
        View view = mAlertDialog.getContentView();
        final ViewGroup dialogContentFrame = (ViewGroup)view.findViewById(R.id.custom_dialog_content);

        layoutInflater.inflate(R.layout.folder_dialog, dialogContentFrame);

        return view;
    }

    private class FolderOnItemClickListener implements OnItemClickListener {
        public void onItemClick(AdapterView<?> adapterView, View view, int position, long id) {
            String filePath = mFolderList.get(position);
            Log.d(TAG, "onItemClick(): position = " + position + ", filePath = " + filePath);
            mCurrentDir = filePath;
            /*
             * FIX BUG: 1467
             * BUG CAUSE: May be the component initialization failed.
             * DATE: 2012-08-16
             */
            if(mCurrentDirTextView == null) {
                return;
            }
            mCurrentDirTextView.setText(mCurrentDir);

            if (!mIsSingleSdcard) {
                if(mBtnConfirm == null) {
                    return;
                }
                mBtnConfirm.setEnabled(true);
            }
            updateAdapter(filePath);
        }
    }

    private void updateAdapter(String path) {
        if(mFolderList != null) {
            mFolderList.clear();
        } else {
            mFolderList = new ArrayList<String>();
        }
        mFolderList = getAllFolderItems(path);
        folderAdapter.clear();
        folderAdapter.addItems(mFolderList);
        folderAdapter.notifyDataSetChanged();
    }

    private List<String> getAllFolderItems(String path) {
        List<String> folderList = null;
        try {
            folderList = getAllFolders(path);
        } catch (Exception e) {
            new AlertDialog.Builder(mContext).setTitle(R.string.text_folder_dialog_error_title)
                    .setMessage(R.string.text_folder_dialog_error_message)
                    .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                        }
                    }).show();
            e.printStackTrace();
            return null;
        }
        return folderList;
    }

    private List<String> getAllFolders(String path) throws Exception {
        File parent = new File(path);
        String[] folderNames = parent.list(new FilenameFilter(){
            public boolean accept(File parent, String filename) {
                if (filename.startsWith(".")) {
                    return false;
                } else {
                    return true;
                }
            }
        });
        if (folderNames == null) {
            if (ROOT_DIR.equals(path)) {
                return new ArrayList<String>();
            } else {
                return getAllFolders(ROOT_DIR);
            }
        }
        List<String> folders = new ArrayList<String>();
        String filePath = null;
        for (String fileName : folderNames) {
            filePath = path + File.separator + fileName;
            //Log.d(TAG, "getAllFiles(): filePath = " + filePath);
            File tempFile = new File(filePath);
            if(tempFile.exists() && tempFile.isDirectory() && tempFile.canWrite()) {
                folders.add(filePath);
            }
        }

        ComparatorByFileName sortComparator = new ComparatorByFileName();
        try {
            sort(folders, sortComparator);
        } catch (Exception e) {
            e.printStackTrace();
            Log.e("FileListAdapter", " compare error : " + e.getMessage());
        }
        return folders;
    }

    public static <T> void sort(List<T> list, Comparator<? super T> comparator) {
        T[] array = list.toArray((T[]) new Object[list.size()]);
        Arrays.sort(array, comparator);
        int i = 0;
        ListIterator<T> it = list.listIterator();
        while (it.hasNext()) {
            it.next();
            it.set(array[i++]);
        }
    }

    private class ComparatorByFileName implements Comparator {
        public int compare(Object obj1, Object obj2) {
            File f1 = new File((String) obj1);
            File f2 = new File((String) obj2);
            boolean f1IsDirecotry = f1.isDirectory();
            boolean f2IsDirecotry = f2.isDirectory();
            int result;
            if (f1IsDirecotry ^ f2IsDirecotry) {
                result = f1IsDirecotry ?-1:1;
            } else {
                String f1FileName = f1.getName().toLowerCase();
                String f2FileName = f2.getName().toLowerCase();
                result = f1FileName.compareTo(f2FileName);
            }
            return result;
        }
    }

    private void onClickBackBtn() {
        if (ROOT_DIR.equals(mCurrentDir)) {
            mAlertDialog.dismiss();
            clear();
        } else {
            mCurrentDir = mCurrentDir.substring(0, mCurrentDir.lastIndexOf(File.separator));
            updateAdapter(mCurrentDir);
            mFolderListView.setSelection(0);

            if (ROOT_DIR.equals(mCurrentDir) && !mIsSingleSdcard) {
                mBtnConfirm.setEnabled(false);
            }
            mCurrentDirTextView.setText(mCurrentDir);
        }
    }

    private void clear() {
        mCurrentDir = ROOT_DIR;
        if(mFolderList != null) {
            mFolderList.clear();
            mFolderList = null;
        }
        if(folderAdapter != null) {
            folderAdapter.clear();
            folderAdapter = null;
        }
    }
}
