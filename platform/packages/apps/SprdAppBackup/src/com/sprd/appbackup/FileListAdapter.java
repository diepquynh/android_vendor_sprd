/**
 * File list adapter
 */

package com.sprd.appbackup;

import java.io.File;
import java.io.FilenameFilter;
import java.util.regex.Pattern;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.Comparator;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

// multi_delete
import android.widget.CheckBox;
import android.app.ListActivity;
import android.widget.CheckedTextView;
import android.util.Log;

public class FileListAdapter extends BaseAdapter {
    private Context mContext;
    private LayoutInflater mInflater;
    private ArrayList<String> mFileNameList;
    private File mCurrentDir;
    private final static String TAG = "FileListAdapter";
    private final static boolean DBG = true;

    private static final TheComparator mAcsOrder = new TheComparator();

    private static final TheFileTypeComparator mTypeOrder = new TheFileTypeComparator();
    private final TheFileSizeComparator mSizeOrder = new TheFileSizeComparator();
    private final TheFileTimeComparator mTimeOrder = new TheFileTimeComparator();

    public static final int SORT_ASCEND = 0;
    public static final int SORT_TIME = 1;
    public static final int SORT_SIZE = 2;
    public static final int SORT_TYPE = 3;

    public static class ViewHolder {
        ImageView fileicon;
        TextView filename;
        TextView filedate;
    }

    /**
     * This Comparator sorts strings into increasing order.
     */
    public static class TheComparator implements Comparator<String> {
        public int compare(String str1, String str2) {
            int len1 = str1.length();
            int len2 = str2.length();
            int len = len1 <= len2 ? len1 : len2;
            for (int i = 0; i < len; i++) {
                int value1 = str1.codePointAt(i);
                int value2 = str2.codePointAt(i);

                // 'A' -> 'a'
                if (value1 > 64 && value1 < 91)
                    value1 = value1 + 32;
                if (value2 > 64 && value2 < 91)
                    value2 = value2 + 32;

                if (value1 == value2)
                    continue;

                if (value1 > 256 && value2 > 256) {
                    return value1 > value2 ? 1 : -1;
                }

            }

            if (len1 == len2) {
                return 0;
            } else {
                return len1 > len2 ? 1 : -1;
            }
        }
    }

    /**
     * sorting by file type
     */
    public static class TheFileTypeComparator implements Comparator<String> {
        public int compare(String str1, String str2) {
            int index = str1.lastIndexOf(".");
            if (index != -1)
                str1 = str1.substring(index + 1);
            else
                str1 = null;

            index = str2.lastIndexOf(".");
            if (index != -1)
                str2 = str2.substring(index + 1);
            else
                str2 = null;

            if (str1 == null && str2 == null)
                return 0;
            else if (str1 == null)
                return -1;
            else if (str2 == null)
                return 1;

            str1 = str1.toLowerCase();
            str2 = str2.toLowerCase();

            int len1 = str1.length();
            int len2 = str2.length();
            int len = len1 <= len2 ? len1 : len2;
            for (int i = 0; i < len; i++) {
                int value1 = str1.codePointAt(i);
                int value2 = str2.codePointAt(i);
                if (value1 == value2)
                    continue;

                if (value1 > 256 && value2 > 256) {
                    return value1 > value2 ? 1 : -1;
                }
            }

            if (len1 == len2)
                return 0;
            else
                return len1 > len2 ? 1 : -1;
        }
        
        //bug 180197
        @Override
        public boolean equals(Object obj){
            return super.equals(obj);
        }
    }

    class TheFileSizeComparator implements Comparator<String> {
        public int compare(String str1, String str2) {
            File file1 = new File(mCurrentDir, str1);
            File file2 = new File(mCurrentDir, str2);

            long diff = file1.length() - file2.length();
            if (diff == 0)
                return 0;
            else
                return diff > 0 ? 1 : -1;
        }
    }

    class TheFileTimeComparator implements Comparator<String> {
        public int compare(String str1, String str2) {
            File file1 = new File(mCurrentDir, str1);
            File file2 = new File(mCurrentDir, str2);

            long diff = file1.lastModified() - file2.lastModified();
            if (diff == 0)
                return 0;
            else
                return diff > 0 ? 1 : -1;
        }
    }

    // ------------------------------------------------------------------

    // Constructor
    public FileListAdapter(Context context, File file, String fileType) {
        mContext = context;
        mCurrentDir = file;
        mInflater = LayoutInflater.from(mContext);
        String[] mFileNameArray;
        if (file != null && file.exists()) {
            // String fileType = FileManager.getFileType();
            if (fileType == null) {
                mFileNameArray = file.list();
            } else if (fileType.equals("*")) {
                mFileNameArray = file.list();
            } else {
                // String patternStr = "^.+\\.(jpg|png|mp3)$";
                // String patternStr = "^.+\\.(" + FileManager.getFileType() +
                // ")$";
                String patternStr = "^.+\\.(" + fileType + ")$";
                final Pattern pattern = Pattern.compile(patternStr,
                        Pattern.CASE_INSENSITIVE);

                mFileNameArray = file.list(new FilenameFilter() {
                    public boolean accept(File dir, String name) {
                        if (new File(dir, name).isFile()) {
                            return pattern.matcher(name).matches();
                        }
                        return true;
                    }
                });
            }

            mFileNameList = getSortedFileNameArray(file, mFileNameArray);
        } else {
            // mFileNameArray = new String[]{};
            mFileNameList = new ArrayList<String>();
            mCurrentDir = null;
        }
    }

    // Constructor 2
    public FileListAdapter(Context context) {
        mContext = context;
        mInflater = LayoutInflater.from(mContext);
        mFileNameList = new ArrayList<String>();
        if(DBG) {Log.d(TAG, " FileListAdapter mContext = " + mContext + "  mInflater= " + mInflater);}
        mCurrentDir = null;
    }

    public void sortImpl(File file, String fileType) {
        mCurrentDir = file;

        String[] mFileNameArray;
        if(DBG) {Log.d(TAG, " File list adapter sortImpl fileType = " + fileType + "  mCurrentDir= " + mCurrentDir);}
        if (file != null && file.exists()) {
            if (fileType == null) {
                mFileNameArray = file.list();
            } else if (fileType.equals("*")) {
                mFileNameArray = file.list();
            } else {
                // String patternStr = "^.+\\.(jpg|png|mp3)$";
                // String patternStr = "^.+\\.(" + FileManager.getFileType() +
                // ")$";
                String patternStr = "^.+\\.(" + fileType + ")$";
                final Pattern pattern = Pattern.compile(patternStr,
                        Pattern.CASE_INSENSITIVE);

                mFileNameArray = file.list(new FilenameFilter() {
                    public boolean accept(File dir, String name) {
                        if (new File(dir, name).isFile()) {
                            return pattern.matcher(name).matches();
                        }
                        return true;
                    }
                });
            }

            mFileNameList = getSortedFileNameArray(file, mFileNameArray);
            if(DBG) {Log.d(TAG, " File list adapter sortImpl mFileNameArray = " + mFileNameArray + " mFileNameList = " + mFileNameList);}
        } else {
            mFileNameList = new ArrayList<String>();
            mCurrentDir = null;
        }

        //notifyDataSetChanged();
    }

    // Return the number of items in the adapter
    public int getCount() {
        return mFileNameList.size();
    }

    public Object getItem(int position) {
        return mFileNameList.get(position);
    }

    public long getItemId(int position) {
        return position;
    }

    public View getView(int position, View convertView, ViewGroup parent) {
        if (mCurrentDir == null) {
            return null;
        }

        ViewHolder holder;
        if (convertView == null) {
            convertView = mInflater.inflate(R.layout.path_list_item, null);
            holder = new ViewHolder();

            holder.fileicon = (ImageView) convertView.findViewById(R.id.icon);
            holder.filename = (TextView) convertView
                    .findViewById(R.id.file_name);
            // SPRD: for Bug283597,Optimization, this TextView is useless, remove it.
            // holder.subfilesnum = (TextView) convertView
            // .findViewById(R.id.sub_file_num);
            holder.filedate = (TextView) convertView
                    .findViewById(R.id.file_date);
            // SPRD: for Bug283597,Optimization, this CheckedTextView is useless, remove it.
            // multi_delete
            // holder.check = (CheckBox)convertView.findViewById(R.id.check);
            // holder.check = (CheckedTextView) convertView
            // .findViewById(R.id.check);
            convertView.setTag(holder);
        } else {
            holder = (ViewHolder) convertView.getTag();
        }

        // 1. Bind file name
        if(DBG) {Log.d(TAG, " File list adapter getView position = " + position + " mFileNameList = " + mFileNameList);}
        if (position > mFileNameList.size()) {
            return convertView;
        }
        String filename = mFileNameList.get(position);
        holder.filename.setText(filename);

        File thefile = new File(mCurrentDir, filename);
        /*
        if (thefile.isFile()) {
            // 2. Bind file icon
            // --- no extend name ---
            String fileNameExt = getFileExtendName(filename);
            if (fileNameExt == null) {
                holder.fileicon.setImageResource(R.drawable.unknown);
            }
            // --- picture ---
            else if (fileNameExt.equalsIgnoreCase("png")
                    || fileNameExt.equalsIgnoreCase("bmp")
                    || fileNameExt.equalsIgnoreCase("jpg")
                    || fileNameExt.equalsIgnoreCase("jpeg")
                    || fileNameExt.equalsIgnoreCase("gif")
                    || fileNameExt.equalsIgnoreCase("wbmp")) {
                holder.fileicon.setImageResource(R.drawable.pic);
            }
            // --- audio ---
            else if (fileNameExt.equalsIgnoreCase("mp3")
                    || fileNameExt.equalsIgnoreCase("amr")
                    || fileNameExt.equalsIgnoreCase("mid")
                    || fileNameExt.equalsIgnoreCase("midi")
                    || fileNameExt.equalsIgnoreCase("wav")
                    || fileNameExt.equalsIgnoreCase("m4a")
                    || fileNameExt.equalsIgnoreCase("wma")
                    || fileNameExt.equalsIgnoreCase("ogg")
                    || fileNameExt.equalsIgnoreCase("aac")
                    || fileNameExt.equalsIgnoreCase("ape")
                    || fileNameExt.equalsIgnoreCase("flac")) {
                holder.fileicon.setImageResource(R.drawable.audio);
            }
            // --- video ---
            else if (fileNameExt.equalsIgnoreCase("mp4")
                    || fileNameExt.equalsIgnoreCase("3gp")
                    || fileNameExt.equalsIgnoreCase("flv")
                    || fileNameExt.equalsIgnoreCase("3g2")) {
                holder.fileicon.setImageResource(R.drawable.video);
            } else if (fileNameExt.equalsIgnoreCase("avi")
                    || fileNameExt.equalsIgnoreCase("wmv")
                    || fileNameExt.equalsIgnoreCase("asf")) {
                holder.fileicon.setImageResource(R.drawable.video);
            }
            // --- office doc ---
            else if (fileNameExt.equalsIgnoreCase("doc")) {
                holder.fileicon.setImageResource(R.drawable.doc);
            } else if (fileNameExt.equalsIgnoreCase("pdf")) {
                holder.fileicon.setImageResource(R.drawable.pdf);
            } else if (fileNameExt.equalsIgnoreCase("ppt")
                    || fileNameExt.equalsIgnoreCase("pps")) {
                holder.fileicon.setImageResource(R.drawable.ppt);
            } else if (fileNameExt.equalsIgnoreCase("xls")) {
                holder.fileicon.setImageResource(R.drawable.xls);
            } else if (fileNameExt.equalsIgnoreCase("txt")) {
                holder.fileicon.setImageResource(R.drawable.txt);
            }
            // --- vCalendar and VCard ---
            else if (fileNameExt.equalsIgnoreCase("vcf")) {
                holder.fileicon.setImageResource(R.drawable.vcf);
            } else if (fileNameExt.equalsIgnoreCase("vcs")) {
                holder.fileicon.setImageResource(R.drawable.vcs);
            }
            // ---apk, html, bin, zip, rar, jar ---
            else if (fileNameExt.equalsIgnoreCase("apk")) {
                holder.fileicon.setImageResource(R.drawable.apk);
            } else if (fileNameExt.equalsIgnoreCase("html")
                    || fileNameExt.equalsIgnoreCase("htm")) {
                holder.fileicon.setImageResource(R.drawable.html);
            } else if (fileNameExt.equalsIgnoreCase("bin")) {
                holder.fileicon.setImageResource(R.drawable.bin);
            } else if (fileNameExt.equalsIgnoreCase("zip")) {
                holder.fileicon.setImageResource(R.drawable.zip);
            } else if (fileNameExt.equalsIgnoreCase("rar")) {
                holder.fileicon.setImageResource(R.drawable.rar);
            } else if (fileNameExt.equalsIgnoreCase("jar")) {
                holder.fileicon.setImageResource(R.drawable.jar);
            }
            // --- unknown ---
            else {
                holder.fileicon.setImageResource(R.drawable.unknown);
            }
            // SPRD: for Bug283597,Optimization, this TextView is useless, remove it.
            // 3. Bind file size
            // holder.subfilesnum.setText(getFileSize(thefile));
        } else */
        {
            // 2. Bind file icon
            holder.fileicon.setImageResource(R.drawable.folder);
            // SPRD: for Bug283597,Optimization, do nothing here, remove it.
            // 3. Bind sub file number
            // if (thefile.list() != null) {
            // //
            // holder.subfilesnum.setText(String.valueOf(thefile.list().length)
            // // + mContext.getString(R.string.item_str));
            // } else {
            // // holder.subfilesnum.setText(String.valueOf(0) +
            // // mContext.getString(R.string.item_str));
            // }
        }

        // 4. File date
        holder.filedate.setText(getFileDate(thefile));
        // SPRD: for Bug283597,Optimization, this CheckedTextView is useless, remove it.
        // holder.check.setVisibility(View.GONE);

        return convertView;

    }

    /*
     * Filter file folder [lost+found] used in the wap
     */
    private void filterFileFold(ArrayList<String> strList) {
        Iterator<String> it = strList.iterator();
        while (it.hasNext()) {
            String filefoldname = it.next();
            if (filefoldname.equals("lost+found")) {
                it.remove();
            } else if (filefoldname.equals(".android_secure")) {
                it.remove();
            }
        }
    }

    private ArrayList<String> getSortedFileNameArray(final File theFile,
            String[] filenames) {
        ArrayList<String> fileNameArray = new ArrayList<String>();
        ArrayList<String> fileFolderArray = new ArrayList<String>();

        if (filenames != null) {
            for (String s : filenames) {
                if (new File(theFile, s).isDirectory())
                    fileFolderArray.add(s);
                else
                    fileNameArray.add(s);
            }

            filterFileFold(fileFolderArray);

            switch (SORT_ASCEND) {
            case SORT_ASCEND:
                try {
                    Collections.sort(fileFolderArray, mAcsOrder);
                    Collections.sort(fileNameArray, mAcsOrder);
                } catch (Exception e) {
                    e.printStackTrace();
                }
                fileFolderArray.addAll(fileNameArray);
                break;
            case SORT_TIME:
                try {
                    Collections.sort(fileFolderArray, mTimeOrder);
                    Collections.sort(fileNameArray, mTimeOrder);
                } catch (Exception e) {
                    e.printStackTrace();
                }
                fileFolderArray.addAll(fileNameArray);
                break;
            case SORT_SIZE:
                try {
                    Collections.sort(fileFolderArray, mAcsOrder);
                    Collections.sort(fileNameArray, mSizeOrder);
                } catch (Exception e) {
                    e.printStackTrace();
                }
                fileFolderArray.addAll(fileNameArray);
                break;
            case SORT_TYPE:
                try {
                    Collections.sort(fileFolderArray, mAcsOrder);
                    Collections.sort(fileNameArray, mTypeOrder);
                } catch (Exception e) {
                    e.printStackTrace();
                }
                fileFolderArray.addAll(fileNameArray);
                break;
            }
        }
        return fileFolderArray;
    }

    public ArrayList<String> getSortedFileNames() {
        return mFileNameList;
    }

    public int getItemIndex(String filename) {
        return mFileNameList.indexOf(filename);
    }

    private static String getFileSize(final File file) {
        String ret;
        long size = file.length();
        float fSize;
        if (size < 1024 * 1024) // KB
        {
            fSize = (float) size / (1024);
            ret = String.format("%.2f KB", fSize);
        } else if (size < 1024 * 1024 * 1024) // MB
        {
            fSize = (float) size / (1024 * 1024);
            ret = String.format("%.2f MB", fSize);
        } else // GB
        {
            fSize = (float) size / (1024 * 1024 * 1024);
            ret = String.format("%.2f GB", fSize);
        }
        return ret;
    }

    private static String getFileDate(final File file) {
        Date date = new Date(file.lastModified());
        SimpleDateFormat formatter = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss");
        return formatter.format(date);
    }

    public static String getFileExtendName(String filename) {
        int index = filename.lastIndexOf('.');
        return index == -1 ? null : filename.substring(index + 1);
    }

}
