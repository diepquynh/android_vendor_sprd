package com.sprd.fileexplorer.util;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Stack;

import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.NinePatchDrawable;
import android.media.MediaScanner;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Handler.Callback;
import android.os.Message;
import android.provider.MediaStore;
import android.support.v4.content.FileProvider;
import android.text.Editable;
import android.text.InputFilter;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.activities.FileSearchResultActivity;
import com.sprd.fileexplorer.adapters.FileListAdapter;
import com.sprd.fileexplorer.adapters.QuickScanCursorAdapter;
import com.sprd.fileexplorer.adapters.SearchListAdapter;
import com.sprd.fileexplorer.drmplugin.FileUtilUtils;

public class FileUtil {
    private static final String TAG = "FileUtil";
    
    /**
     * when completed file operation, the processBar don't dismiss right now, it should
     * wait some times
     */
    // SPRD: Add for bug616271.
    public static final int MIN_CLICK_DELAY_TIME = 500;
    public static final int FILT_OP_COMPLETE_WAIT_TIME = 500;
    
    public static final int FILE_ATTRIBUTE_MESSAGE = 100;
    public static final int REFREAH_FILELIST = 101;

    public static final String CALC_FILE_NUM_KEY = "calc_file_num";
    public static final String CALC_FOLDER_NUM_KEY = "calc_folder_num";
    public static final String CALC_SIZE_KEY = "calc_size_num";
    // SPRD: Add for bug615382.
    public static final String FILE_PROVIDER_URI_HEAD = "com.sprd.fileexplorer.fileProvider";

    private static final int MAX_FILENAME_LENGTH = 50;
    private static final int MAX_IMAGE_SIZE = 20 * 1024 * 1024;
    //private static final int MAX_IMAGE_RESOLUTION = 4096 * 4096;
    /* SPRD： Modify for bug505784. @{ */
    private static final int MAX_IMAGE_WIDTH = 4096;
    private static final int MAX_IMAGE_HEIGTH = 4096;
    public static final int MAX_NUM_COMPRESSION = 5;
    /* @} */

    public static final String RIGHTS_NO_lIMIT = "-1";

    static List<Character> sInvalidCharOfFilename;
    
    public final static SimpleDateFormat SIMPLE_DATE_FOTMAT;
    /** SPRD Fix BUG# 526464, Changed file name incorrect after reset APP preferences {@ **/
    private static AlertDialog mAlertDialog;
    /** @} **/
    
    static {
        sInvalidCharOfFilename = new ArrayList<Character>();
        sInvalidCharOfFilename.add('*');
        sInvalidCharOfFilename.add('\"');
        sInvalidCharOfFilename.add('/');
        sInvalidCharOfFilename.add('\\');
        sInvalidCharOfFilename.add('?');
        sInvalidCharOfFilename.add('|');
        sInvalidCharOfFilename.add('>');
        sInvalidCharOfFilename.add('<');
        sInvalidCharOfFilename.add(':');
        SIMPLE_DATE_FOTMAT = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault());
    }
    
    public static File execute(File currentFile, File executeFile,
            Context context) {
        if (executeFile == null || currentFile == null) {
            throw new NullPointerException();
        }
        Log.d(TAG, "currentFile=" + currentFile.getPath() + "executeFile" + executeFile.getPath());
        if (!executeFile.canRead()) {
            return currentFile;
        }
        if (executeFile.isDirectory()) {
            return executeFile;
        }
        return currentFile;

    }

    public static void rename(File renameFile, FileListAdapter mAdapter,
            Context context) {
        buildRenameDialog(renameFile, mAdapter, null, context);
    }

    public static void rename(File renameFile, SearchListAdapter mAdapter,
            Context context) {
        buildRenameDialog(renameFile, mAdapter, null, context);
    }

    public static void rename(File renameFile, Context context) {
        buildRenameDialog(renameFile, null, null, context);
    }

    public static void rename(File renameFile, Uri renameFileUri,
            QuickScanCursorAdapter adapter, Context context) {
        buildRenameDialog(renameFile, adapter, renameFileUri, context);
    }

    public static void viewDetails(File file, Context context) {
        buildDetailDialog(file, context);
    }

    public static void mkdir(final File filePath, FileListAdapter fileAdapter,
            Context context) {
        buildNewDirDialog(filePath, fileAdapter, context);
    }

    public static void newfile(final File filePath,
            FileListAdapter fileAdapter, Context context) {
        buildNewFileDialog(filePath, fileAdapter, context);
    }

    private static Bitmap getDefaultBitmap(Context context) {
        Drawable defaultImageIcon = context.getResources().getDrawable(R.drawable.file_item_image_ic);
        BitmapDrawable bitd = (BitmapDrawable) defaultImageIcon;
        return bitd.getBitmap();
    }

    public static Bitmap readBitMap(String filePath, boolean isImage,
            Context context) {
        if (isImage) {
            File tempFile = new File(filePath);
            if (tempFile.length() > MAX_IMAGE_SIZE){
                getDefaultBitmap(context);
            }
            BitmapFactory.Options opt = new BitmapFactory.Options();
            opt.inJustDecodeBounds = true;
            BitmapFactory.decodeFile(filePath, opt);
            long px = opt.outWidth * opt.outHeight;
            /* SPRD： Modify for bug505784. @{
            if (px > MAX_IMAGE_RESOLUTION) {
                return getDefaultBitmap(context);
            }
            @} */
            // SPRD： Modify for bug518232.
            //opt.inSampleSize = computeSampleSize(opt, -1, 128*128);
            opt.inJustDecodeBounds = false;
            opt.inPreferredConfig = Bitmap.Config.RGB_565;
            opt.inPurgeable = true;
            opt.inInputShareable = true;
            Bitmap bmp = null;

            /* SPRD： Modify for bug505784, give a default thumbnail icon for wbmp image with large resolution. @{ */
            /*try{
                bmp = BitmapFactory.decodeFile(filePath, opt);
            }catch(OutOfMemoryError e){
               Log.e(TAG,"Happen OOM");
            }*/
            boolean noBitmap = true;
            boolean wbmp = filePath.endsWith(".wbmp");
            int sampleSize = computeSampleSize(opt, -1, 128*128);
            int num_tries = 0;
            if (wbmp && (opt.outWidth > MAX_IMAGE_WIDTH || opt.outHeight > MAX_IMAGE_HEIGTH)) {
                return getDefaultBitmap(context);
            }
            while (noBitmap) {
                try {
                    Log.d(TAG, "readBitMap().try: num_tries = " + num_tries + "; filePath = " + filePath);
                    opt.inSampleSize = sampleSize;
                    bmp = BitmapFactory.decodeFile(filePath, opt);
                    noBitmap = false;
                } catch (OutOfMemoryError error) {
                    Log.e(TAG, "Happen OOM ", error);
                    if (++num_tries >= MAX_NUM_COMPRESSION) {
                        noBitmap = false;
                    }
                    if (bmp != null) {
                        bmp.recycle();
                        bmp = null;
                    }
                    System.gc();
                    sampleSize *= 2;
                    Log.d(TAG, "readBitMap().catch: num_tries = " + num_tries + "; filePath = " + filePath);
                }
            }
            /* @} */
            return bmp;
        } else {
            PackageManager pm = context.getPackageManager();
            PackageInfo info = pm.getPackageArchiveInfo(filePath,
                    PackageManager.GET_ACTIVITIES);
            if (info != null) {
                ApplicationInfo appInfo = info.applicationInfo;
                appInfo.sourceDir = filePath;
                appInfo.publicSourceDir = filePath;
                Drawable drawable = appInfo.loadIcon(pm);
                /* SPRD 443650 @{ */
                if (drawable instanceof NinePatchDrawable) {
                    Bitmap bitmap = Bitmap.createBitmap(
                            drawable.getIntrinsicWidth(),
                            drawable.getIntrinsicHeight(),
                            Bitmap.Config.RGB_565);
                    Canvas canvas = new Canvas(bitmap);
                    drawable.setBounds(0, 0, drawable.getIntrinsicWidth(),
                            drawable.getIntrinsicHeight());
                    drawable.draw(canvas);
                    return bitmap;
                } else if (drawable instanceof BitmapDrawable) {
                    BitmapDrawable bd = (BitmapDrawable) drawable;
                    return bd.getBitmap();
                } else {
                    return null;
                }
                /* @} */
            } else {
                return null;
            }

        }
    }

    public static String getPathName(String path) {
        int index = path.lastIndexOf('/');
        if (index == -1 || index == 0)
            return path;
        return path.substring(index + 1);
    }

    public static String pathNameAppend(String path, String apd) {
        String name = getPathName(path);
        int i = name.lastIndexOf(".");
        if (i == -1 || i == 0)
            return path + apd;
        int l = path.lastIndexOf("/") + 1;
        return path.substring(0, l + i) + apd + path.substring(l + i);
    }

    // SPRD: Modify for bug470877.
    public static boolean hasInvalidChar(boolean isFile, String fileName,
            boolean isExternalStorage, final Context context) {
        if (context == null) {
            Log.d(TAG, "hasInvalidChar context is null");
            return false;
        }
        Resources res = context.getResources();
        StringBuilder invalidCharOfFilename = new StringBuilder();
        boolean hasInvalidChar = false;
        for(char ch : fileName.toCharArray()) {
            if(sInvalidCharOfFilename.contains(ch)) {
                hasInvalidChar = true;
                invalidCharOfFilename.append(ch);
                invalidCharOfFilename.append(' ');
            }
        }
        if (hasInvalidChar) {
            int id = isExternalStorage ? R.string.title_section_external : R.string.title_section_internal;
            /* SPRD: Modify for bug470877. @{ */
            if (isFile) {
                showMsg(res.getString(
                        R.string.invalid_char_of_filename,
                        res.getString(id),
                        invalidCharOfFilename),
                        context);
            } else {
                showMsg(res.getString(
                        R.string.invalid_char_of_foldername,
                        res.getString(id),
                        invalidCharOfFilename),
                        context);
            }
            /* @} */
        }
        return hasInvalidChar;
    }
    
    private static void buildNewDirDialog(final File filePath,
            final FileListAdapter adapter, final Context context) {
        if (context == null) {
            return;
        }
        final boolean isExternalStorage = filePath.getAbsolutePath()
                .startsWith(StorageUtil.getExternalStorage().getAbsolutePath());
        View nameView = LayoutInflater.from(context).inflate(
                R.layout.edit_name, null);
        Builder builder = new AlertDialog.Builder(context);
        final EditText fileNameText = (EditText) nameView
                .findViewById(R.id.name_editor);
        String newFileName = context.getResources().getString(
                R.string.menu_mkdir);
        int copy = 1;
        while ((new File(filePath.toString() + "/" + newFileName)).exists()) {
            newFileName = pathNameAppend(
                    context.getResources().getString(R.string.menu_mkdir), "-"
                            + (copy++));
        }
        fileNameText.setText(newFileName);
        fileNameText.requestFocus();
        
        builder.setPositiveButton(android.R.string.ok,
                new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        Resources res = context.getResources();
                        String fileName = fileNameText.getText().toString()
                                .trim();
                        if (fileName == null || fileName.equals("")) {
                            /* SPRD: Modify for Bug405860. @{ */
                            //showMsg(R.string.msg_file_input_empty, context);
                            showMsg(R.string.msg_folder_input_empty, context);
                            /* @} */
                            keepDialog(dialog);
                            return;
                        } else if ("..".equals(fileName)
                                || ".".equals(fileName)) {
                            showMsg(R.string.badname_as_point, context);
                            keepDialog(dialog);
                            return;
                        } else if (fileName.endsWith(".")) {
                            showMsg(R.string.badname_endwith_point, context);
                            keepDialog(dialog);
                            return;
                        // SPRD: Modify for bug470877.
                        } else if(hasInvalidChar(false, fileName, isExternalStorage, context)) {
                            keepDialog(dialog);
                            return;
                        }
                        String folderPath = filePath.toString();
                        Log.d(TAG, "new folder name is : " + folderPath);
                        if (folderPath != null) {
                            File newFolder = new File(folderPath + "/"
                                    + fileName);
                            if (newFolder.exists()) {
                                showRepeatDialog(res.getString(
                                        R.string.msg_file_input_repeat,
                                        fileName), context);
                                keepDialog(dialog);
                            } else {
                                if (newFolder.mkdir()) {
                                    scanFile(context, newFolder);
                                    adapter.refresh();
                                    showMsg(res.getString(
                                            R.string.menu_mkdir_succeed,
                                            fileName), context);
                                    destroyDialog(dialog);
                                } else {
                                    // SPRD: Modify for bug470877.
                                    if (hasInvalidChar(false, fileName, isExternalStorage, context)) {
                                        keepDialog(dialog);
                                    } else {
                                        showMsg(res.getString(
                                                R.string.menu_mkdir_failed,
                                                fileName), context);

                                    }
                                }
                            }
                        }
                    }

                });
        builder.setNegativeButton(android.R.string.cancel,
                new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        destroyDialog(dialog);
                    }

                });
        final AlertDialog dialog = builder.create();
        dialog.setTitle(R.string.menu_mkdir);
        dialog.setView(nameView);
        dialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
            @Override
            public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                if ((keyCode == KeyEvent.KEYCODE_BACK) && (event.getAction() == KeyEvent.ACTION_UP)) {
                    destroyDialog(dialog);
                }
                return false;
            }
        });
        
        fileNameText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count,
                    int after) {
            }

            @Override
            public void onTextChanged(CharSequence newName, int start,
                    int before, int count) {
                // SPRD 430730
                String str = newName.toString().trim();
                final Button positiveButton = dialog
                        .getButton(AlertDialog.BUTTON_POSITIVE);
                if (positiveButton != null) {
                    /* SPRD 430730 @{ */
                    //if (newName.length() <= 0) {
                    if (str.length() <= 0) {
                    /* @} */
                        positiveButton.setEnabled(false);
                    } else {
                        positiveButton.setEnabled(true);
                    }
                }
            }

            @Override
            public void afterTextChanged(Editable s) {
                int length = s.toString().length();
                if (length >= MAX_FILENAME_LENGTH) {
                    showMsg(R.string.length_limited, context);
                }
            }
        });
        fileNameText.setFilters( new InputFilter[] { new InputFilter.LengthFilter(MAX_FILENAME_LENGTH) });
        dialog.getWindow().setSoftInputMode(
                WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
        dialog.show();
    }

    private static void buildNewFileDialog(final File filePath,
            final FileListAdapter adapter, final Context context) {
        if (context == null) {
            return;
        }
        final boolean isExternalStorage = filePath.getAbsolutePath()
                .startsWith(StorageUtil.getExternalStorage().getAbsolutePath());
        View nameView = LayoutInflater.from(context).inflate(
                R.layout.edit_name, null);
        Builder builder = new AlertDialog.Builder(context);
        final EditText fileNameText = (EditText) nameView
                .findViewById(R.id.name_editor);
        String newFileName = context.getResources().getString(
                R.string.menu_newfile);
        int copy = 1;
        while ((new File(filePath.toString() + "/" + newFileName + ".txt"))
                .exists()) {
            newFileName = pathNameAppend(
                    context.getResources().getString(R.string.menu_newfile),
                    "-" + (copy++));
        }
        fileNameText.setText(newFileName);
        fileNameText.requestFocus();
        builder.setPositiveButton(android.R.string.ok,
                new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {

                        String fileName = fileNameText.getText().toString()
                                .trim();
                        Resources res = context.getResources();
                        if (fileName == null || fileName.equals("")) {
                            showMsg(R.string.msg_file_input_empty, context);
                            keepDialog(dialog);
                            return;
                        } else if ("..".equals(fileName)
                                || ".".equals(fileName)) {
                            showMsg(R.string.badname_as_point, context);
                            keepDialog(dialog);
                            return;
                        // SPRD: Modify for bug470877.
                        } else if (hasInvalidChar(true, fileName, isExternalStorage,context)) {
                            keepDialog(dialog);
                            return;
                        }
                        String folderPath = filePath.toString();
                        if (folderPath != null) {
                            File newFile = new File(folderPath + "/" + fileName
                                    + ".txt");
                            if (newFile.exists()) {
                                showRepeatDialog(res.getString(
                                        R.string.msg_file_input_repeat,
                                        fileName), context);
                                keepDialog(dialog);
                            } else {
                                destroyDialog(dialog);
                                try {
                                    if (newFile.createNewFile()) {
                                        scanFile(context, newFile);
                                        adapter.refresh();
                                        showMsg(res.getString(
                                                R.string.menu_newfile_succeed,
                                                fileName), context);
                                    } else {
                                        showMsg(res.getString(
                                                R.string.menu_newfile_failed,
                                                fileName), context);
                                    }
                                } catch (NotFoundException e) {
                                    e.printStackTrace();
                                } catch (IOException e) {
                                    showMsg(res.getString(
                                            R.string.menu_newfile_failed,fileName), context);
                                    e.printStackTrace();
                                }
                            }
                        }
                    }

                });
        builder.setNegativeButton(android.R.string.cancel,
                new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        destroyDialog(dialog);
                    }

                });
        final AlertDialog dialog = builder.create();
        dialog.setTitle(R.string.menu_newfile);
        dialog.setView(nameView);
        dialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
            @Override
            public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                if ((keyCode == KeyEvent.KEYCODE_BACK) && (event.getAction() == KeyEvent.ACTION_UP)) {
                    destroyDialog(dialog);
                }
                return false;
            }
        });
        
        
        fileNameText.addTextChangedListener(new TextWatcher() {

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                
            }

            @Override
            public void onTextChanged(CharSequence newName, int start, int before, int count) {
                // SPRD 430730
                String str = newName.toString().trim();
                final Button positiveButton = dialog.getButton(AlertDialog.BUTTON_POSITIVE);
                if (positiveButton != null) {
                    /* SPRD 430730 @{ */
                    //if (newName.length() <= 0) {
                    if (str.length() <= 0) {
                    /* @} */
                        positiveButton.setEnabled(false);
                    } else {
                        positiveButton.setEnabled(true);
                    }
                }
            }

            @Override
            public void afterTextChanged(Editable s) {
                int length = s.toString().length();
                if (length >= MAX_FILENAME_LENGTH) {
                    showMsg(R.string.length_limited, context);
                }
            }

        });
        fileNameText.setFilters( new InputFilter[] { new InputFilter.LengthFilter(MAX_FILENAME_LENGTH) });
        dialog.getWindow().setSoftInputMode(
                WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
        dialog.show();
    }

    private static String extensionName;
    private static String subName;
    private static String newName;
    private static int mLastNameLength;

    private static void buildRenameDialog(final File file,
            final BaseAdapter fileAdapter, final Uri fileUri,
            final Context context) {
        View nameView = LayoutInflater.from(context).inflate(
                R.layout.edit_name, null);
        Builder builder = new AlertDialog.Builder(context);

        final EditText fileNameText = (EditText) nameView
                .findViewById(R.id.name_editor);
        final String fileName = file.getName();
        final int dotIndex = fileName.lastIndexOf(".");
        if (file.isFile()) {
            if (dotIndex > 0) {
                extensionName = getExtensionName(fileName, dotIndex);
                subName = fileName.substring(0, dotIndex);
                fileNameText.setText(subName);
            } else {
                fileNameText.setText(fileName);
            }
        } else if (file.isDirectory()) {
            fileNameText.setText(fileName);
        } else {
            Log.i(TAG, "Is neither a file nor directory!");
        }
        fileNameText.requestFocus();
        builder.setPositiveButton(android.R.string.ok,
                new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {

                        if (fileNameText.getText().toString().trim().equals("")) {
                            /* SPRD: Modify for Bug406560. @{ */
                            //showMsg(R.string.msg_file_input_empty, context);
                            if(file.isFile()){
                                showMsg(R.string.msg_file_input_empty, context);
                            } else if (file.isDirectory()){
                                showMsg(R.string.msg_folder_input_empty, context);
                            }
                            /* @} */
                            keepDialog(dialog);
                            return;
                        }
                        if (file.isFile()) {
                            if (dotIndex <= 0) {
                                newName = fileNameText.getText().toString().trim();
                            } else {
                                newName = fileNameText.getText().toString().trim()
                                        + extensionName;
                            }
                        } else if (file.isDirectory()) {
                            newName = fileNameText.getText().toString().trim();
                        } else {
                            Log.i(TAG, "Do nothing!");
                        }
                        if (newName == null) {
                            return;
                        }
                        Resources res = context.getResources();
                        Log.d(TAG, "new name is " + newName);
                        boolean isExternalStorageFile = file.getAbsolutePath()
                                .startsWith(StorageUtil.getExternalStorage().getAbsolutePath());
                        // SPRD: Modify for bug470877.
                        if (hasInvalidChar(file.isFile(), fileNameText.getText().toString(),
                                isExternalStorageFile, context)) {
                            keepDialog(dialog);
                            return;
                        }
                        if (newName.endsWith(".")) {
                            showMsg(R.string.badname_endwith_point, context);
                            keepDialog(dialog);
                            return;
                        }

                        final File newFile = new File(file.getParent() + "/" + newName);
                        boolean isExternalStorage = newFile.getAbsolutePath()
                                .startsWith(StorageUtil.getExternalStorage().getAbsolutePath());
                        final String type = FileType.getFileType(context).getTypeByFile(file);
                        if (newFile.exists()) {
                            if(fileName.equals(newName)){
                                destroyDialog(dialog);
                                return;
                            }else{
                                showMsg(res.getString(
                                        R.string.msg_file_input_repeat, newName),
                                        context);
                                 keepDialog(dialog);
                            }
                        } else {
                            if (file.renameTo(newFile)) {
                                destroyDialog(dialog);
                                if (newFile.isDirectory()) {
                                    scanDir(context, newFile.getParentFile());
                                }else {
                                    new Thread(new Runnable() {
                                        public void run() {
                                            //if branch resolve rename drm file not refresh in music or galley interface
                                            if(FileUtilUtils.getInstance().renameDRMFile(context,newFile,file)){

                                            }else{
                                                //SPRD:542522 & 637488 Copy hidden audio file,can not see after rename
                                                if (type.equals("audio/*") && !fileName.startsWith(".")
                                                        && !newName.startsWith(".")) {
                                                    ContentValues value = new ContentValues();
                                                    value.put(MediaStore.Audio.AudioColumns.DATA, newFile.getAbsolutePath());
                                                    value.put(MediaStore.Audio.AudioColumns.DISPLAY_NAME, newName);
                                                    context.getContentResolver().update(
                                                            MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
                                                            value,
                                                            MediaStore.Audio.AudioColumns.DATA + "=?",
                                                            new String[] {file.getAbsolutePath()});
                                                }else{
                                                    scanFile(context, newFile);
                                                    context.getContentResolver().delete(
                                                        MediaStore.Files.getContentUri("external"),
                                                        MediaStore.Files.FileColumns.DATA + "=?",
                                                        new String[] {file.getAbsolutePath()});
                                                }
                                            }
                                        }
                                    }).start();
                                }
                                if (fileAdapter != null
                                        && fileAdapter instanceof FileListAdapter) {
                                    ((FileListAdapter) fileAdapter).refresh();
                                } else if (fileAdapter != null
                                        && fileAdapter instanceof SearchListAdapter) {
                                    if (((SearchListAdapter) fileAdapter)
                                            .getInSearchUi()) {
                                        ((FileSearchResultActivity) context)
                                                /* SPRD 440831 */
                                                //.searchFile();
                                                .searchFile(true,newFile.getAbsolutePath());
                                                /* @} */
                                    } else {
                                        File file = ((SearchListAdapter) fileAdapter)
                                                .getCurrentPath();
                                        ((SearchListAdapter) fileAdapter)
                                                .refresh(file);
                                    }
                                }  else if (fileAdapter != null
                                        && fileAdapter instanceof QuickScanCursorAdapter) {
                                    //SPRD:433606 the file disappear when rename the file
                                    ((QuickScanCursorAdapter) fileAdapter)
                                            .refreshWaitForFile(newFile.getAbsolutePath());
                                }
                                showMsg(R.string.operate_rename_succeed,
                                        context);
                            } else {
                                // SPRD: Modify for bug470877.
                                if (hasInvalidChar(file.isFile(), newName, isExternalStorage, context)) {
                                    keepDialog(dialog);
                                } else {
                                    showMsg(res.getString(
                                            R.string.operate_rename_failed,
                                            file.getName()), context);
                                }
                            }

                        }

                    }

                });
        builder.setNegativeButton(android.R.string.cancel,
                new DialogInterface.OnClickListener() {

                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        destroyDialog(dialog);
                    }

                });

        final AlertDialog dialog = builder.create();
        dialog.setTitle(R.string.operate_rename);
        dialog.setView(nameView);
        dialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
            @Override
            public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                if ((keyCode == KeyEvent.KEYCODE_BACK) && (event.getAction() == KeyEvent.ACTION_UP)) {
                    destroyDialog(dialog);
                }
                return false;
            }
        });
        
        fileNameText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count,
                    int after) {
                mLastNameLength = s.toString().length();
            }
            
            @Override
            public void onTextChanged(CharSequence newName, int start,
                    int before, int count) {
                //SPRD 430730
                String str = newName.toString().trim();
                final Button positiveButton = dialog
                        .getButton(AlertDialog.BUTTON_POSITIVE);
                if (positiveButton != null) {
                    /* SPRD 430730 @{ */
                    //if (newName.length() <= 0) {
                    if (str.length() <= 0) {
                    /* @} 	*/
                        positiveButton.setEnabled(false);
                    } else {
                        positiveButton.setEnabled(true);
                    }
                }
            }

            @Override
            public void afterTextChanged(Editable s) {
                // TODO Auto-generated method stub
                int length = s.toString().length();
                if (length >= MAX_FILENAME_LENGTH && length >= mLastNameLength) {
                    showMsg(R.string.length_limited, context);
                }
            }

        });

        fileNameText.setFilters( new InputFilter[] { new InputFilter.LengthFilter(MAX_FILENAME_LENGTH) });
        dialog.getWindow().setSoftInputMode(
                WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
        dialog.show();
        /** SPRD Fix BUG# 526464, Changed file name incorrect after reset APP preferences {@ **/
        mAlertDialog = dialog;
        /** @} **/
    }

    /** SPRD Fix BUG# 526464, Changed file name incorrect after reset APP preferences {@ **/
    public static void dismissDialog(){
        if(mAlertDialog != null && mAlertDialog.isShowing()){
            mAlertDialog.dismiss();
        }
    }
    /** @} **/

    private static String getExtensionName(String fileName, int dotIndex) {
        if (fileName != null && (fileName.length() > 0)) {
            if ((dotIndex > -1) && (dotIndex < (fileName.length() - 1))) {
                return fileName.substring(dotIndex);
            }
        }
        return fileName;
    }

    private static void buildDetailDialog(final File file, Context context) {
        Resources res = context.getResources();
        String fileMessage = res
                .getString(file.isFile() ? R.string.operate_viewdetails_type_file
                        : R.string.operate_viewdetails_type_folder)
                + "\n"
                + res.getString(R.string.operate_viewdetails_path,
                        file.getParent())
                + "\n"
                + res.getString(R.string.operate_viewdetails_date,
                        SIMPLE_DATE_FOTMAT.format(file.lastModified()));
        AlertDialog.Builder deteilDialog = new AlertDialog.Builder(context);
        TextView detailView = new TextView(context);
        detailView.setTextColor(Color.BLACK);
        detailView.setPadding(20, 0, 10, 0);
        detailView.setTextSize(18);
        /* SPRD: modify 20131211 Spreadtrum of 250938 file properties content in horizontal screen is cut off with huge font@{ */
        detailView.setHorizontallyScrolling(false);
        ScrollView scrollView = new ScrollView(context);
        scrollView.addView(detailView);
        deteilDialog.setView(scrollView);
        /* @} */
        FitSizeExpression fitSize = new FitSizeExpression(file.length());
        if (file.isDirectory()) {
            detailView
                    .setText(fileMessage
                            + "\n"
                            + res.getString(R.string.caculating)
                            + "\n"
                            + res.getString(R.string.operate_viewdetails_size,
                                    fitSize.getExpression())
                            + "\n"
                            + (file.canRead() ? res
                                    .getString(R.string.operate_viewdetails_canread)
                                    : res.getString(R.string.operate_viewdetails_can_not_read))
                            + "\n"
                            + (file.canRead() ? res
                                    .getString(R.string.operate_viewdetails_can_write)
                                    : res.getString(R.string.operate_viewdetails_can_not_write))
                            + "\n"
                            + (file.isHidden() ? res
                                    .getString(R.string.operate_viewdetails_hide)
                                    : res.getString(R.string.operate_viewdetails_not_hide)));
            updateAttribute(deteilDialog, file, fileMessage, detailView, context);
        } else {
            detailView
                    .setText(fileMessage
                            + "\n"
                            + res.getString(R.string.operate_viewdetails_size,
                                    fitSize.getExpression())
                            + "\n"
                            + (file.canRead() ? res
                                    .getString(R.string.operate_viewdetails_canread)
                                    : res.getString(R.string.operate_viewdetails_can_not_read))
                            + "\n"
                            + (file.canWrite() ? res
                                    .getString(R.string.operate_viewdetails_can_write)
                                    : res.getString(R.string.operate_viewdetails_can_not_write))
                            + "\n"
                            + (file.isHidden() ? res
                                    .getString(R.string.operate_viewdetails_hide)
                                    : res.getString(R.string.operate_viewdetails_not_hide)));
        }
        deteilDialog
                .setTitle(file.getName())
                .setPositiveButton(R.string.dialog_back, null).show();
    }

    private static void updateAttribute(final AlertDialog.Builder dialog,
            final File file, final CharSequence headStr,
            final TextView textView, final Context context) {
        Handler handler = new Handler(context.getMainLooper(), new Callback() {

            @Override
            public boolean handleMessage(Message msg) {
                switch (msg.what) {
                case FILE_ATTRIBUTE_MESSAGE:
                    Resources res = context.getResources();
                    Bundle data = msg.getData();
                    long fileNum = data.getLong(CALC_FILE_NUM_KEY);
                    long folderNum = data.getLong(CALC_FOLDER_NUM_KEY);
                    String size = data.getString(CALC_SIZE_KEY);
                    textView.setText(headStr
                            + "\n"
                            + context.getResources().getString(
                                    R.string.operate_folder_attribute, fileNum,
                                    folderNum, size)
                            + "\n"
                            + (file.canRead() ? res
                                    .getString(R.string.operate_viewdetails_canread)
                                    : res.getString(R.string.operate_viewdetails_can_not_read))
                            + "\n"
                            + (file.canRead() ? res
                                    .getString(R.string.operate_viewdetails_can_write)
                                    : res.getString(R.string.operate_viewdetails_can_not_write))
                            + "\n"
                            + (file.isHidden() ? res
                                    .getString(R.string.operate_viewdetails_hide)
                                    : res.getString(R.string.operate_viewdetails_not_hide)));
                    break;
                }
                return false;
            }
        });
        CalcFileAttribute calc = new CalcFileAttribute(handler, file);
        calc.start();
    }

    public static class CalcFileAttribute extends Thread {

        private Handler mHandler;

        private File mFile;

        long[] calcRet = new long[3];
        
        private FitSizeExpression mFitSize = new FitSizeExpression(0);

        public CalcFileAttribute(Handler handler, File file) {
            super();
            this.mHandler = handler;
            mFile = file;
        }

        @Override
        public void run() {
            long startTime = System.currentTimeMillis();
            Stack<File> stack = new Stack<File>();
            if (mFile != null && mFile.exists()) {
                stack.push(mFile);
            }
            while (!stack.empty()) {
                File file = stack.pop();
                if (!file.exists()) {
                    break;
                }
                if (file.isFile()) {
                    calcRet[0]++;
                    calcRet[2] += file.length();
                    continue;
                } else {
                    calcRet[1]++;
                    calcRet[2] += file.length();
                    File[] list = file.listFiles();
                    if (list != null) {
                        for (File f : list) {
                            stack.add(f);
                        }
                    }
                }
                long tmp = System.currentTimeMillis();
                if (tmp - startTime > 1000) {
                    sendMessage();
                    startTime = tmp;
                }
            }
            if (mFile != null && mFile.isDirectory()) {
                calcRet[1]--;
            }
            sendMessage();
        }

        private void sendMessage() {
            Message msg = mHandler.obtainMessage();
            msg.what = FILE_ATTRIBUTE_MESSAGE;
            Bundle data = new Bundle();
            data.putLong(CALC_FILE_NUM_KEY, calcRet[0]);
            data.putLong(CALC_FOLDER_NUM_KEY, calcRet[1]);
            mFitSize.changeSize(calcRet[2]);
            data.putString(CALC_SIZE_KEY, mFitSize.getExpression());
            msg.setData(data);
            mHandler.sendMessage(msg);
        }

    }

    private static void showMsg(String msg, Context context) {
        Toast.makeText(context, msg, Toast.LENGTH_SHORT).show();
    }

    private static void showMsg(int resId, Context context) {
        Toast.makeText(context, resId, Toast.LENGTH_SHORT).show();
    }

    private static void showRepeatDialog(String msg, Context context){
        AlertDialog.Builder builder = new Builder(context);
        builder.setTitle(R.string.dialog_hint_title);
        builder.setMessage(msg);
        builder.setPositiveButton(R.string.dialog_back, new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which){
                dialog.dismiss();
            }
        });
        builder.create().show();
    }

    private static void keepDialog(DialogInterface dialog) {
        try {
            if (dialog != null) {
                Field field = dialog.getClass().getSuperclass()
                        .getDeclaredField("mShowing");
                if (field != null) {
                    field.setAccessible(true);
                    field.set(dialog, false);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static void destroyDialog(DialogInterface dialog) {
        try {
            if (dialog != null) {
                Field field = dialog.getClass().getSuperclass()
                        .getDeclaredField("mShowing");
                if (field != null) {
                    field.setAccessible(true);
                    field.set(dialog, true);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void scanFile(Context context, File file) {
        Log.i(TAG, "send broadcast to scan file");
        android.media.MediaScannerConnection.scanFile(context.getApplicationContext(),
                new String[]{ file.getAbsolutePath() }, null, null);
    }

    private static void scanDir(Context context, File dir) {
        Log.i(TAG,"send broadcast to scan dir = " + dir);
        /* SPRD: Add for bug599127. @{ */
        //context.sendBroadcast(new Intent("android.intent.action.MEDIA_SCANNER_SCAN_DIR"));
        String path = dir.getPath();
        Intent intent = new Intent("android.intent.action.MEDIA_SCANNER_SCAN_DIR");
        Bundle bundle = new Bundle();
        bundle.putString("scan_dir_path", path);
        intent.putExtras(bundle);
        context.sendBroadcast(intent);
        /* @} */
    }


    /**
     * in UI thread call this method is not a good idea unless you can sure the sourceFile don't
     * contain too many subfiles
     * @param sourceFile
     * @param containFolder
     * @return if sourceFile is a folder, return all files in the folder below, and contain sourceFile
     *   if sourceFile is a file, only return sourceFile
     */
    public static List<File> getAllFiles(File sourceFile, boolean containFolder) {
        Stack<File> dirStack = new Stack<File>();
        List<File> allFile = new LinkedList<File>();
        if(!sourceFile.exists()) {
            Log.e(TAG, "file: " + sourceFile + " does not exist");
            return allFile;
        }
        if (sourceFile.isDirectory()) {
            dirStack.add(sourceFile);
        } else {
            allFile.add(sourceFile);
        }
        while(!dirStack.empty()) {
            File file = dirStack.pop();
            if(containFolder) {
                allFile.add(file);
            }
            if (file != null && file.listFiles() != null) {
                for(File f : file.listFiles()) {
                    if(f.isDirectory()) {
                        dirStack.push(f);
                    } else {
                        allFile.add(f);
                    }
                }
            }

        }
        return allFile;
    }

    /* SPRD: Modify for bug615444. @{ */
    public static boolean isHidden(String path){
        if (path != null && path.indexOf("/.") >= 0) {
            return true;
        } else {
            return false;
        }
    }
    /* @} */

    public static boolean isParentOrEqualsFolder(String pfPath, String fPath) {
        boolean ret = fPath.startsWith(pfPath);
        if(!ret)
            return false;
        int index = pfPath.length();
        if(index == fPath.length())
            return true;
        return fPath.charAt(index) == '/';
    }

    public static boolean isParentFolder(String pfPath, String fPath) {
        boolean ret = fPath.startsWith(pfPath);
        if(!ret)
            return false;
        int index = pfPath.length();
        if(index == fPath.length())
            return false;
        return fPath.charAt(index) == '/';
    }

    public static int computeSampleSize(BitmapFactory.Options options,
            int minSideLength, int maxNumOfPixels) {
        int initialSize = computeInitialSampleSize(options, minSideLength,maxNumOfPixels);

        int roundedSize;
        if (initialSize <= 8 ) {
            roundedSize = 1;
            while (roundedSize < initialSize) {
                roundedSize <<= 1;
            }
        } else {
            roundedSize = (initialSize + 7) / 8 * 8;
        }

        return roundedSize;
    }

    private static int computeInitialSampleSize(BitmapFactory.Options options,int minSideLength, int maxNumOfPixels) {
        double w = options.outWidth;
        double h = options.outHeight;

        int lowerBound = (maxNumOfPixels == -1) ? 1 :
                (int) Math.ceil(Math.sqrt(w * h / maxNumOfPixels));
        int upperBound = (minSideLength == -1) ? 128 :
                (int) Math.min(Math.floor(w / minSideLength),
                Math.floor(h / minSideLength));

        if (upperBound < lowerBound) {
            // return the larger one when there is no overlapping zone.
            return lowerBound;
        }

        if ((maxNumOfPixels == -1) && (minSideLength == -1)) {
            return 1;
        } else if (minSideLength == -1) {
            return lowerBound;
        } else {
            return upperBound;
        }
    }

    /* SPRD: When share files, return corresponding uri according to the mimetype. @{ */
    public static Uri getSharedFileUri(File file, String mimetype, Context context) {
        Uri uri = null;
        /* SPRD 511715 change the uri format to content uri for audio and video files @{ */
        if (mimetype != null && (mimetype.startsWith("video/") || mimetype.startsWith("audio/"))) {
            Cursor cursor = null;
            try {
                if (mimetype.startsWith("video/")){
                    String[] mediaColums = new String[] { MediaStore.Video.Media._ID };
                    String whereClause = MediaStore.Video.Media.DATA
                            + " = '" + file.getAbsolutePath() + "'";
                    cursor = context.getContentResolver().query(
                            MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
                            mediaColums, whereClause, null, null);
                    if (cursor != null && cursor.getCount() != 0) {
                        cursor.moveToFirst();
                        uri = Uri.parse(MediaStore.Video.Media.EXTERNAL_CONTENT_URI
                                + "/"
                                + cursor.getInt(cursor.getColumnIndexOrThrow(MediaStore.Video.Media._ID)));
                        }
                } else if (mimetype.startsWith("audio/")){
                    String[] mediaColums = new String[] { MediaStore.Audio.Media._ID };
                    String whereClause = MediaStore.Audio.Media.DATA
                            + " = '" + file.getAbsolutePath() + "'";
                    cursor = context.getContentResolver().query(
                            MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
                            mediaColums, whereClause, null, null);
                    if (cursor != null && cursor.getCount() != 0) {
                        cursor.moveToFirst();
                        uri = Uri.parse(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI
                                + "/"
                                + cursor.getInt(cursor.getColumnIndexOrThrow(MediaStore.Audio.Media._ID)));
                    }
                }
            } catch (Exception ex) {
                String path = file.getPath().replaceAll("%","%25").replaceAll("#","%23");
                uri = Uri.parse("file://" + path);
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }
        }
        /* @} */
        /* SPRD 465205 @{ */
        if(uri == null){
            String path = file.getPath().replaceAll("%","%25").replaceAll("#","%23");
            uri = Uri.parse("file://" + path);
        }
        /* @} */
        Log.i(TAG, "getSharedFileUri(): mimetype:" + mimetype + "   uri:" + uri);
        return uri;
    }
    /* @} */

    /* SPRD 459367 just use "!" is OK, and need replace # to %23 @{ */
    /* SPRD 459367 @{ */
    //public static Uri getFileUri(File file){
    //SPRD 429148
    //	String path = file.getPath().replaceAll("%","%25").replaceAll("#","%23");
    //	return Uri.parse("file://" + path);
    //}
    /* SPRD: Add for bug615382, obtain the file uri by FileProvider. @{ */
    public static Uri getFileUri(File file,String mimetype,Context context){
        Uri uri = null;
        Cursor cursor = null;
        Log.d(TAG, "getFileUri(): file = " + file + "; mimetype = " + mimetype);
        /* SPRD: Modify for bug635681. @{ */
        try {
            if (mimetype.startsWith("image/")) {
                String[] mediaColums = new String[] { MediaStore.Images.Media._ID };
                String whereClause = MediaStore.Images.Media.DATA + " = \"" + file.getAbsolutePath() + "\"";
                cursor = context.getContentResolver().query(
                        MediaStore.Images.Media.EXTERNAL_CONTENT_URI,
                        mediaColums, whereClause, null, null);
                if (cursor != null && cursor.getCount() != 0) {
                    cursor.moveToFirst();
                    uri = Uri.parse(MediaStore.Images.Media.EXTERNAL_CONTENT_URI
                            + "/"
                            + cursor.getInt(cursor.getColumnIndexOrThrow(MediaStore.Images.Media._ID)));
                }
            } else if (mimetype.startsWith("video/")) {
                String[] mediaColums = new String[] { MediaStore.Video.Media._ID };
                String whereClause = MediaStore.Video.Media.DATA + " = \"" + file.getAbsolutePath() + "\"";
                cursor = context.getContentResolver().query(
                        MediaStore.Video.Media.EXTERNAL_CONTENT_URI,
                        mediaColums, whereClause, null, null);
                if (cursor != null && cursor.getCount() != 0) {
                    cursor.moveToFirst();
                    uri = Uri.parse(MediaStore.Video.Media.EXTERNAL_CONTENT_URI
                            + "/"
                            + cursor.getInt(cursor.getColumnIndexOrThrow(MediaStore.Video.Media._ID)));
                }
            } else if (mimetype.startsWith("audio/")) {
                String[] mediaColums = new String[] { MediaStore.Audio.Media._ID };
                String whereClause = MediaStore.Audio.Media.DATA + " = \"" + file.getAbsolutePath() + "\"";
                cursor = context.getContentResolver().query(
                        MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
                        mediaColums, whereClause, null, null);
                if (cursor != null && cursor.getCount() != 0) {
                    cursor.moveToFirst();
                    uri = Uri.parse(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI
                            + "/"
                            + cursor.getInt(cursor.getColumnIndexOrThrow(MediaStore.Audio.Media._ID)));
                }
            } else {
                String[] mediaColums = new String[]{MediaStore.Files.FileColumns._ID};
                String whereClause = MediaStore.Files.FileColumns.DATA + " = \"" + file.getAbsolutePath() + "\"";
                cursor = context.getContentResolver().query(
                        MediaStore.Files.getContentUri("external"),
                        mediaColums, whereClause, null, null);
                if (cursor != null && cursor.getCount() != 0) {
                    cursor.moveToFirst();
                    uri = Uri.parse(MediaStore.Files.getContentUri("external")
                            + "/"
                            + cursor.getInt(cursor.getColumnIndexOrThrow(MediaStore.Files.FileColumns._ID)));
                }
            }
        } catch (Exception e) {
            Log.i(TAG, "getFileUri error");
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        /* @} */

        if (uri == null) {
            Log.i(TAG, "getFileUri(): Fail to get file uri from MediaProvider database!");
            uri = FileProvider.getUriForFile(context, FILE_PROVIDER_URI_HEAD, file);
        }
        Log.i(TAG, "getFileUri(): file = " + file + "; mimetype = " + mimetype + "; uri = " + uri);
        return uri;
    }
    /* @} */

    /* SPRD: Add for bug505955. @{ */
    private static PastePathUtil sPastePathUtil = new PastePathUtil();
    private OnPastePathChangedListener mOnPastePathChangedListener;

    public static void addPastePathChangeListener(OnPastePathChangedListener onpathchengedlistener) {
        sPastePathUtil.addPastePathChangeListener(onpathchengedlistener);
    }

    public static void removePastePathChangeListener(OnPastePathChangedListener onpathchengedlistener) {
        sPastePathUtil.removePastePathChangeListener(onpathchengedlistener);
    }

    public static void notifyPathChanged(String path) {
        sPastePathUtil.notifyPathChanged(path);
    }
    /* @} */

    private static USBRefreshListener mUSBRefreshListener;
    public interface USBRefreshListener{
        public void onChanged();
    }

    public static void addUSBRefreshListener(USBRefreshListener listener){
        mUSBRefreshListener = listener;
    }

    public static void refreshUSBFragment(){
        if(mUSBRefreshListener != null){
            mUSBRefreshListener.onChanged();
        }
    }
}
