package com.sprd.fileexplorer.activities;

import java.io.File;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.adapters.FileAdapter.BackStackListener;
import com.sprd.fileexplorer.adapters.FileAdapter.EmptyViewListener;
import com.sprd.fileexplorer.adapters.FileAdapter.LoadingFileListener;
import com.sprd.fileexplorer.adapters.FileListAdapter;
import com.sprd.fileexplorer.adapters.FileListAdapter.PathChangeListener;
import com.sprd.fileexplorer.util.ActivityUtils;
import com.sprd.fileexplorer.util.FileCopyTask;
import com.sprd.fileexplorer.util.FileUtil;
import com.sprd.fileexplorer.util.FileUtilTask.OnFinishCallBack;
import com.sprd.fileexplorer.util.IStorageUtil.StorageChangedListener;
import com.sprd.fileexplorer.util.StorageUtil;

import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;

public class FilePasteActivity extends Activity {

    private static final String TAG = "FilePasteActivity";

    public static final String PASTE_FLAG = "paste_flag";
    public static final String PASTE_SOURCE_PATH = "paste_src_path";
    public static final String PASTE_SOURCE_PATHS = "paste_src_paths";
    public static final String PASTE_DEST_PATH = "paste_path";
    public static final String PASTE_LARGE_PATH = "past_large_path";
    public static final int FLAG_TO_SDCARD = 1;
    public static final int FLAG_CUT_OP = 1 << 1;
    public static final int FLAG_MULT_OP = 1 << 2;
    // SPRD 454659
    public static final int FLAG_TO_USB = 1 << 3;

    private FileListAdapter mAdapter;
    private ListView mListView = null;
    private int mFlag;
    private List<File> mOpFiles = new ArrayList<File>();
    private boolean mIsCut;
    private FileCopyTask mFileCopyTask = null;
    // SPRD: Add for bug598539, obtain the source path.
    private String mSrcPath = null;
    // SPRD: Add for bug603830.
    private String[] mSrcPaths = null;
    private static final File USB_LIST_DIRECTORY = new File("USB List");

    StorageChangedListener mStorageChangedListener = new StorageChangedListener() {

        // SPRD: Modify for bug509242.
        @Override
        public void onStorageChanged(String path, boolean available, boolean sdcard) {
            Log.d(TAG, "StorageChanged: path = " + path + " available = " + available + "; sdcard = " + sdcard);
            if(available) {
                return;
            }

            // SPRD: Modify for bug612613, keep copying unless the source or destination path isn't available.
            if ((mAdapter != null && !StorageUtil.isStorageMounted(mAdapter.getCurrentPath()))
                    || (mSrcPath != null && !StorageUtil.isStorageMounted(new File(mSrcPath)))) {
                new Handler(getMainLooper()).postDelayed(new Runnable() {

                    @Override
                    public void run() {
                        finish();
                    }
                }, 1000);
                // SPRD: Add for bug603830.
                return;
            }
            /* SPRD: Add for bug603830, check the source paths whether are available. @{ */
            if (mSrcPaths != null && mSrcPaths.length > 1) {
                for (String srcPath : mSrcPaths) {
                    /* SPRD: Modify for bug612613. @{ */
                    File srcFile = new File(srcPath);
                    if (!StorageUtil.isStorageMounted(srcFile)) {
                    /* @} */
                        new Handler(getMainLooper()).postDelayed(new Runnable() {

                            @Override
                            public void run() {
                                finish();
                            }
                        }, 1000);

                        break;
                    }
                }
            }
            /* @} */
        }
    };

    @Override
    public void onResume() {
        super.onResume();
        /* SPRD: 501785, judge destFile whether is available before refresh view @{ */
        final String toPath = mAdapter.getCurrentString();
        final File dstFile = new File(toPath);
        // SPRD: Modify for bug514449, without the runtime permission, cann't copy files.
        if ((checkSelfPermission(WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) && (dstFile.exists()
                && dstFile.isDirectory() || toPath.equals("/USB List"))) {
            mAdapter.ensureRefresh();
        } else {
            finish();
        }
        /* }@ */
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_file_paste);
        StorageUtil.addStorageChangeListener(mStorageChangedListener);
        mListView = (ListView) findViewById(R.id.list_paste);
        final View emptyView = findViewById(R.id.emptyView);
        final TextView pathBar = (TextView) findViewById(R.id.path_bar_file_explore_activity);
        final View standByView = findViewById(R.id.standby);
        Intent intent = getIntent();
        mFlag = intent.getIntExtra(PASTE_FLAG, 0);
        // SPRD: Add for bug598539, obtain the source path.
        mSrcPath = intent.getStringExtra(PASTE_SOURCE_PATH);
        /* SPRD: Add for bug603830, obtain the source paths. @{ */
        Bundle bundle = intent.getExtras();
        mSrcPaths = bundle.getStringArray(PASTE_SOURCE_PATHS);
        /* @} */
        mIsCut = (mFlag & FLAG_CUT_OP) != 0;
        Set<String> filePaths = new HashSet<String>();
        if((mFlag & FLAG_MULT_OP) != 0) {
            if(intent.getStringExtra(PASTE_LARGE_PATH).equals("LARGE_PATH")) {
                String[] array = ActivityUtils.CopyFilePath.copyFilePathsTran;
                if(array != null) {
                    for(String s: array) {
                        filePaths.add(s);
                    }
                }
            }
        } else {
            filePaths.add(intent.getStringExtra(PASTE_SOURCE_PATH));
        }
        filePaths.remove(null);
        for(String path: filePaths) {
            File f = new File(path);
            if(f.canRead()) {
                mOpFiles.add(f);
            } else {
                Log.w(TAG, "file: " + f + " can not read");
            }
        }
        if(mOpFiles.isEmpty()) {
            Log.w(TAG, "paste file is invalid, intent: " + intent + " mfalg: " + Integer.toBinaryString(mFlag));
            finish();
            return;
        }
        mAdapter = new FileListAdapter(this, mListView);
        /* SPRD 454659 @{ */
        File mFile = StorageUtil.getInternalStorage();
        if((mFlag & FLAG_TO_SDCARD) != 0){
            mFile = StorageUtil.getExternalStorage();
        }
        if((mFlag & FLAG_TO_USB) != 0){
            mFile = USB_LIST_DIRECTORY;
        }
        //mAdapter.initWithPath((mFlag & FLAG_TO_SDCARD) != 0 ? StorageUtil.getExternalStorage() : StorageUtil.getInternalStorage());
        mAdapter.initWithPath(mFile);
        /* @} */
        mAdapter.setPathChangeListener(new PathChangeListener() {
            @Override
            public void onPathChanged(String path) {
                pathBar.setText(path);
                /* SPRD: Modify for bug612613. @{ */
                if (mAdapter.getCurrentPath().equals(USB_LIST_DIRECTORY)) {
                    invalidateOptionsMenu();
                }
                /* @} */
            }
        });
        mAdapter.setLoadingFileListener(new LoadingFileListener() {
            @Override
            public void onLoadFileStart() {
                standByView.setVisibility(View.VISIBLE);
                mListView.setVisibility(View.GONE);
            }

            @Override
            public void onLoadFileFinished() {
                standByView.setVisibility(View.GONE);
            }
        });
        mAdapter.setEmptyViewListener(new EmptyViewListener() {
            @Override
            public void onEmptyStateChanged(boolean isEmpty) {
                if((mAdapter != null) && mAdapter.mIsLoading) {
                    return;
                }
                if (isEmpty) {
                    mListView.setVisibility(View.GONE);
                    emptyView.setVisibility(View.VISIBLE);
                } else {
                    mListView.setVisibility(View.VISIBLE);
                    emptyView.setVisibility(View.GONE);
                }
            }
        });
        mAdapter.setBackStackListener(new BackStackListener() {
            @Override
            public void onBackPrevious() {
                mListView.setSelection(mAdapter.restorePreviousPosition(mAdapter.getCurrentString()));
            }
        });
        pathBar.setText(mAdapter.getCurrentPath().toString());
        mListView.setAdapter(mAdapter);
        mListView.setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> arg0, View arg1, int arg2, long arg3) {
                mAdapter.execute(arg2, mListView.getFirstVisiblePosition());
                pathBar.setText(mAdapter.getCurrentPath().toString());
                invalidateOptionsMenu();
            }

        });
        ActionBar actionbar = getActionBar();
        actionbar.setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP | ActionBar.DISPLAY_SHOW_TITLE);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.paste_activity_menu, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        if (menu != null) {
            MenuItem pasteItem = menu.findItem(R.id.action_paste_mode_paste);
            if (mAdapter == null) {
                return false;
            }
            if (!mAdapter.getCurrentPath().equals(USB_LIST_DIRECTORY)) {
                if (pasteItem != null) {
                    pasteItem.setVisible(true);
                }
            } else {
                if (pasteItem != null) {
                    pasteItem.setVisible(false);
                }
            }
        }
        return super.onPrepareOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_paste_mode_paste:
                startPaste();
                break;
            case R.id.action_paste_mode_cancel:
                finish();
                break;
            case android.R.id.home:
                onBackPressed();
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    private void startPaste() {
        final String toPath = mAdapter.getCurrentString();
        final List<File> invalidPath = new ArrayList<File>();
        final List<File> cutFileExist = new ArrayList<File>();
        for(File f: mOpFiles) {
            String fap = f.getAbsolutePath();
            if(mIsCut && FileUtil.isParentOrEqualsFolder(fap, toPath + "/" + f.getName())){
                cutFileExist.add(f);
            }
            if(FileUtil.isParentOrEqualsFolder(fap, toPath)) {
                invalidPath.add(f);
            } else {
                File newFile = new File(toPath + "/" + f.getName());
                if(newFile.exists()) {
                    if(FileUtil.isParentFolder(newFile.getAbsolutePath(), fap)) {
                        invalidPath.add(f);
                    }
                }
            }
        }
        boolean pathOk = invalidPath.isEmpty();
        if(pathOk) {
            runPaste(invalidPath, cutFileExist, toPath);
        } else {
            File topInvalidFolder = invalidPath.get(0);
            for(File f: invalidPath) {
                if(topInvalidFolder.compareTo(f) < 0) {
                    topInvalidFolder = f;
                }
            }
            new AlertDialog.Builder(this)
            .setTitle(mIsCut? R.string.cannot_cutfolder_itself : R.string.cannot_copyfolder_itself )
            .setMessage(String.format(getResources().getString(R.string.cannot_paste_reason), topInvalidFolder))
            .setPositiveButton(R.string.common_text_ignore, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog,
                        int which) {
                    runPaste(invalidPath, cutFileExist, toPath);
                }
            })
            .setNegativeButton(android.R.string.cancel, null)
            .show();
        }
    }

    private void runPaste(List<File> invalidPath, List<File> cutFileExist, String dstPath) {
      Iterator<File> it = mOpFiles.iterator();
      while(it.hasNext()) {
          File f = it.next();
          if(invalidPath.contains(f) || cutFileExist.contains(f)) {
              it.remove();
          }
      }
      if(mOpFiles.isEmpty()) {
          Toast.makeText(this, R.string.delete_finish, Toast.LENGTH_SHORT).show();
          Intent intent = new Intent(this, FileExploreActivity.class);
          intent.putExtra(FilePasteActivity.PASTE_DEST_PATH, dstPath);
          startActivity(intent);
      } else {
          final File dstFile = new File(dstPath);
          mFileCopyTask = new FileCopyTask.Build(this, mIsCut)
          .addOperateFiles(mOpFiles).setDestFolder(dstFile)
          .setOnFinishCallBack(new OnFinishCallBack() {

              /* SPRD: Modify for bug465956. @{ */
              @Override
              public void onFinish(boolean cancelTask, String path) {
                  //if(cancelTask) {
                  if(!cancelTask) {
                      // SPRD: Add for bug505955.
                      Log.d(TAG, "paste file is finished, target path is " + path);
                      FileUtil.notifyPathChanged(path);
                      Intent intent = new Intent();
                      intent.putExtra(FilePasteActivity.PASTE_DEST_PATH, path);
                      setResult(ActivityUtils.COPY_PATH_RESULT,intent);
                      finish();
                  }
              }
              /* @} */
          })
          .creatTask();
          mFileCopyTask.start();
      }
    }

    @Override
    public void onBackPressed() {
        mAdapter.mCancelLoading = true;
        mAdapter.stopSort();
        if (mAdapter.popupFolder()) {
            super.onBackPressed();
        } else {
            mAdapter.setNeedBack();
        }
    }

    @Override
    protected void onDestroy() {
        if(mAdapter != null) {
            mAdapter.destroyThread();
            mAdapter = null;
        }
        StorageUtil.removeStorageChangeListener(mStorageChangedListener);
        /* sprd: fix fc issue progressdialog still exist as activity has been destroy @{ */
        if (mFileCopyTask != null){
            mFileCopyTask.stop();
            mFileCopyTask = null;
        }
        /* @} */
        super.onDestroy();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        ActionBar actionbar = getActionBar();
        actionbar.setTitle(R.string.app_name);
        this.invalidateOptionsMenu();

        if (mAdapter != null) {
            mAdapter.notifyDataSetChanged();
        }
        if (mFileCopyTask != null){
            mFileCopyTask.onConfigChanged();
        }
    }
}
