
package com.sprd.fileexplorer.util;

// Porting from xingxing
// TODO: MERGE INTO FILEUTIL

import java.io.File;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;

import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.provider.MediaStore;
import android.util.Log;
import android.webkit.MimeTypeMap;

import com.sprd.fileexplorer.R;

/**
 * Using the to get file type and its icon.
 * 
 * @author Xingxing
 */
public class FileType {

    public static final int FILE_TYPE_IMAGE = R.drawable.file_item_image_ic;
    public static final int FILE_TYPE_VIDEO = R.drawable.file_item_video_ic;
    public static final int FILE_TYPE_AUDIO = R.drawable.file_item_audio_ic;
    public static final int FILE_TYPE_DOC = R.drawable.file_item_doc_ic;
    public static final int FILE_TYPE_PACKAGE = R.drawable.file_item_apk_default_ic;
    public static final int FILE_TYPE_VCARD = R.drawable.file_vcard_ic;
    public static final int FILE_TYPE_VCALENDER = R.drawable.file_vcalender_ic;
    public static final int FILE_TYPE_RECORD = 0;

    public static final int FILE_TYPE_WEBTEXT = R.drawable.file_item_web_ic;
    public static final int FILE_TYPE_TEXT = R.drawable.file_doc_txt_ic;
    public static final int FILE_TYPE_WORD = R.drawable.file_doc_word_ic;
    public static final int FILE_TYPE_EXCEL = R.drawable.file_doc_excel_ic;
    public static final int FILE_TYPE_PPT = R.drawable.file_doc_ppt_ic;
    public static final int FILE_TYPE_PDF = R.drawable.file_doc_pdf_ic;
    public static final int FILE_TYPE_VIDEO_MP4 = R.drawable.file_video_mp4_ic;
    public static final int FILE_TYPE_VIDEO_MKV = R.drawable.file_video_mkv_ic;
    public static final int FILE_TYPE_VIDEO_RMVB = R.drawable.file_video_rmvb_ic;
    public static final int FILE_TYPE_VIDEO_3GP = R.drawable.file_video_3gp_ic;
    public static final int FILE_TYPE_VIDEO_AVI = R.drawable.file_video_avi_ic;
    public static final int FILE_TYPE_VIDEO_MPEG = R.drawable.file_video_mpeg_ic;
    public static final int FILE_TYPE_VIDEO_FLV = R.drawable.file_video_flv_ic;
    public static final int FILE_TYPE_VIDEO_ASF = R.drawable.file_video_asf_ic;
    public static final int FILE_TYPE_VIDEO_DIVX = R.drawable.file_video_divx_ic;
    public static final int FILE_TYPE_VIDEO_MPE = R.drawable.file_video_mpe_ic;
    public static final int FILE_TYPE_VIDEO_MPG = R.drawable.file_video_mpg_ic;
    public static final int FILE_TYPE_VIDEO_RM = R.drawable.file_video_rm_ic;
    public static final int FILE_TYPE_VIDEO_VOB = R.drawable.file_video_vob_ic;
    public static final int FILE_TYPE_VIDEO_WMV = R.drawable.file_video_wmv_ic;
    public static final int FILE_TYPE_VIDEO_M4V = R.drawable.file_video_m4v_ic;
    public static final int FILE_TYPE_VIDEO_F4V = R.drawable.file_video_f4v_ic;
    public static final int FILE_TYPE_VIDEO_WEBM = R.drawable.file_video_webm_ic;
    // SPRD: Add for bug507035.
    public static final int FILE_TYPE_VIDEO_3G2 = R.drawable.file_video_3g2_ic;
    //SPRD : Add for 496451
    public static final int FILE_TYPE_VIDEO_TS = R.drawable.file_video_ts_ic;
    /* SPRD: Add for bug611635. @{ */
    public static final int FILE_TYPE_VIDEO_M2TS = R.drawable.file_video_m2ts_ic;
    public static final int FILE_TYPE_VIDEO_MOV = R.drawable.file_video_mov_ic;
    /* @} */
    // public static final int FILE_TYPE_VIDEO_3GPP =
    // R.drawable.file_video_3gpp_ic;
    // public static final int FILE_TYPE_VIDEO_3G2 =
    // R.drawable.file_video_3g2_ic;
    public static final int FILE_TYPE_AUDIO_MP3 = R.drawable.file_audio_mp3_ic;
    public static final int FILE_TYPE_AUDIO_OGG = R.drawable.file_audio_ogg_ic;
    public static final int FILE_TYPE_AUDIO_OGA = R.drawable.file_audio_oga_ic;
    public static final int FILE_TYPE_AUDIO_ACC = R.drawable.file_audio_acc_ic;
    public static final int FILE_TYPE_AUDIO_WAV = R.drawable.file_audio_wav_ic;
    public static final int FILE_TYPE_AUDIO_WMA = R.drawable.file_audio_wma_ic;
    public static final int FILE_TYPE_AUDIO_AMR = R.drawable.file_audio_amr_ic;
    public static final int FILE_TYPE_AUDIO_AIFF = R.drawable.file_audio_aiff_ic;
    public static final int FILE_TYPE_AUDIO_APE = R.drawable.file_audio_ape_ic;
    public static final int FILE_TYPE_AUDIO_AV = R.drawable.file_audio_av_ic;
    public static final int FILE_TYPE_AUDIO_CD = R.drawable.file_audio_cd_ic;
    public static final int FILE_TYPE_AUDIO_MIDI = R.drawable.file_audio_midi_ic;
    public static final int FILE_TYPE_AUDIO_VQF = R.drawable.file_audio_vqf_ic;
    public static final int FILE_TYPE_AUDIO_AAC = R.drawable.file_audio_aac_ic;
    public static final int FILE_TYPE_AUDIO_MID = R.drawable.file_audio_mid_ic;
    public static final int FILE_TYPE_AUDIO_M4A = R.drawable.file_audio_m4a_ic;
    public static final int FILE_TYPE_AUDIO_IMY = R.drawable.file_audio_imy_ic;
    /* SPRD 435235 @{ */
    public static final int FILE_TYPE_AUDIO_MP4 = R.drawable.file_audio_mp4_ic;
    public static final int FILE_TYPE_AUDIO_3GPP = R.drawable.file_audio_3gpp_ic;
    /* @} */
    // SPRD 435235
    public static final int FILE_TYPE_AUDIO_3GP = R.drawable.file_audio_3gp_ic;
    // SPRD: Add for bug507035.
    public static final int FILE_TYPE_AUDIO_3G2 = R.drawable.file_audio_3g2_ic;
    // SPRD 498509
    public static final int FILE_TYPE_AUDIO_OPUS = R.drawable.file_audio_opus_ic;
    /* SPRD 457501 @{ */
    public static final int FILE_TYPE_AUDIO_AWB = R.drawable.file_audio_awb_ic;
    public static final int FILE_TYPE_AUDIO_FLAC = R.drawable.file_audio_flac_ic;
    /* @} */
    // SPRD: Add for bug510953.
    public static final int FILE_TYPE_AUDIO_MKA = R.drawable.file_audio_mka_ic;
    /* SPRD: Add for bug511015. @{ */
    public static final int FILE_TYPE_AUDIO_M4B = R.drawable.file_audio_m4b_ic;
    public static final int FILE_TYPE_AUDIO_M4R = R.drawable.file_audio_m4r_ic;
    /* @} */
    public static final int FILE_TYPE_UNKNOE = R.drawable.file_item_default_ic;

    private static Set<String> mImageFileType = new HashSet<String>();

    private static Set<String> mAudioFileType = new HashSet<String>();

    private static Set<String> mVideoFileType = new HashSet<String>();

    private static Set<String> mDocFileType = new HashSet<String>();

    private static Set<String> mTextFileType = new HashSet<String>();

    private static Set<String> mWordFileType = new HashSet<String>();

    private static Set<String> mExcelFileType = new HashSet<String>();

    private static Set<String> mPPTFileType = new HashSet<String>();

    private static Set<String> mWebTextFileType = new HashSet<String>();

    private static Set<String> mPdfFileType = new HashSet<String>();

    private static Set<String> mPackageFileType = new HashSet<String>();

    private static Set<String> mVcardFileType = new HashSet<String>();

    private static Set<String> mVcalenderFileType = new HashSet<String>();

    private static FileType fileType = null;

    private static boolean inited = false;
    
    private static Context mContext;
    private Resources resource;
    private static String TAG = "FileType";

    private FileType(Context context) {
        super();
        mContext = context;
        resource = context.getResources();

        for (String s : resource.getStringArray(R.array.ImageFileType)) {
            mImageFileType.add(s);
        }
        for (String s : resource.getStringArray(R.array.AudioFileType)) {
            mAudioFileType.add(s);
        }
        for (String s : resource.getStringArray(R.array.VideoFileType)) {
            mVideoFileType.add(s);
        }
        for (String s : resource.getStringArray(R.array.TextFileType)) {
            mTextFileType.add(s);
        }
        for (String s : resource.getStringArray(R.array.WebTextFileType)) {
            mWebTextFileType.add(s);
        }
        for (String s : resource.getStringArray(R.array.WordFileType)) {
            mWordFileType.add(s);
        }
        for (String s : resource.getStringArray(R.array.ExcelFileType)) {
            mExcelFileType.add(s);
        }
        for (String s : resource.getStringArray(R.array.PackageType)) {
            mPackageFileType.add(s);
        }
        for (String s : resource.getStringArray(R.array.PdfFileType)) {
            mPdfFileType.add(s);
        }
        for (String s : resource.getStringArray(R.array.PPTFileType)) {
            mPPTFileType.add(s);
        }
        for (String s : resource.getStringArray(R.array.VcardType)) {
            mVcardFileType.add(s);
        }
        for (String s : resource.getStringArray(R.array.VcalenderType)) {
            mVcalenderFileType.add(s);
        }
        mDocFileType.addAll(mTextFileType);
        mDocFileType.addAll(mWordFileType);
        mDocFileType.addAll(mPdfFileType);
        mDocFileType.addAll(mPPTFileType);
        mDocFileType.addAll(mExcelFileType);
    }

    public static void init(Context context) {
        fileType = new FileType(context);
        inited = true;
    }

    public static FileType getFileType(Context context) {
        if (!inited) {
            init(context);
        }
        return fileType;

    }

    public int getFileType2(File file) {
        return getTypeBySuffix(getSuffix(file));
    }
    
    public int getFileTypeByName(String fileName) {
        return getTypeBySuffix(getSuffixByName(fileName));
    }
    
    private int getTypeBySuffix (String suffix) {
        if (suffix == null) {
            return FILE_TYPE_UNKNOE;
        } else if (mImageFileType.contains(suffix)) {
            return FILE_TYPE_IMAGE;
        } else if (mVideoFileType.contains(suffix)) {
            return FILE_TYPE_VIDEO;
        } else if (mAudioFileType.contains(suffix)) {
            return FILE_TYPE_AUDIO;
        } else if (mTextFileType.contains(suffix)) {
            return FILE_TYPE_DOC;
        } else if (mWebTextFileType.contains(suffix)) {
            return FILE_TYPE_DOC;
        } else if (mWordFileType.contains(suffix)) {
            return FILE_TYPE_DOC;
        } else if (mExcelFileType.contains(suffix)) {
            return FILE_TYPE_DOC;
        } else if (mPPTFileType.contains(suffix)) {
            return FILE_TYPE_DOC;
        } else if (mPdfFileType.contains(suffix)) {
            return FILE_TYPE_DOC;
        } else if (mWebTextFileType.contains(suffix)) {
            return FILE_TYPE_DOC;
        } else if (mPackageFileType.contains(suffix)) {
            return FILE_TYPE_PACKAGE;
        } else {
            return FILE_TYPE_UNKNOE;
        }
    }

    public static String getTypeByFile(File file) {
        String suffix = getSuffix(file);
        String filePath = file.getAbsolutePath();
        if (suffix == null) {
            return "file/*";
        } else if (mImageFileType.contains(suffix)) {
            return "image/*";
        } else if (mVideoFileType.contains(suffix)) {
            if(suffix.equalsIgnoreCase(".3GPP") && recordingType(file)){
                return "audio/*";
            }
            /* SPRD 434319 @{ */
            if(suffix.equalsIgnoreCase(".mp4") && recordingType(file)){
                return "audio/*";
            }
            /* @} */
            /* SPRD 450611 @{ */
            if(suffix.equalsIgnoreCase(".3gp") && recordingType(file)){
                return "audio/*";
            }
            /* @} */
            /* SPRD: Add for bug507035. @{ */
            if(suffix.equalsIgnoreCase(".3g2") && recordingType(file)){
                return "audio/*";
            }
            /* @} */
            return "video/*";
        } else if (mAudioFileType.contains(suffix)) {
            return "audio/*";
        } else if (mTextFileType.contains(suffix)) {
            return "Text/*";
        } else if (mWebTextFileType.contains(suffix)) {
            return "Text/*";
        } else if (mWordFileType.contains(suffix)) {
            return "application/msword";
        } else if (mExcelFileType.contains(suffix)) {
            return "application/vnd.ms-excel";
        } else if (mPPTFileType.contains(suffix)) {
            return "application/vnd.ms-powerpoint";
        } else if (mPdfFileType.contains(suffix)) {
            return "application/pdf";
        } else if (mPackageFileType.contains(suffix)) {
            return "application/vnd.android.package-archive";
        } else if (suffix.equals(".vcf")) {
            return "text/x-vcard";
        } else if (suffix.equals(".vcs")) {
            return "text/x-vcalendar";
        } else {
            return "file/*";
        }
    }

    public String getShareTypeByFile(File file) {
        String suffix = getSuffix(file);
        if (suffix == null) {
            return "file/*";
        }
        /* SPRD 465205 @{ */
        if(suffix.equalsIgnoreCase(".3gpp") && recordingType(file)){
            return "audio/3gpp";
        }else if(suffix.equalsIgnoreCase(".mp4") && recordingType(file)){
            return "audio/mp4";
        }else if(suffix.equalsIgnoreCase(".3gp") && recordingType(file)){
            return "audio/3gp";
        /* SPRD: Add for bug507035 @{ */
        }else if(suffix.equalsIgnoreCase(".3g2") && recordingType(file)){
            return "audio/mp4";
        }
        /* @} */
        /* @} */
        String type = MimeTypeMap.getSingleton().getMimeTypeFromExtension(suffix.substring(1));

        if (type == null || type.isEmpty()) {
            return "file/*";
        }
        return type;
    }

    public int getFileType(File file) {
        String suffix = getSuffix(file);
        if (suffix == null) {
            return FILE_TYPE_UNKNOE;
        } else if (mImageFileType.contains(suffix)) {
            return FILE_TYPE_IMAGE;
        } else if (mVideoFileType.contains(suffix)) {
            if(suffix.equalsIgnoreCase(".3GPP") && recordingType(file)){
                return getAudioFileIcon(suffix);
            }
            /* SPRD 433748 @{ */
            if(suffix.equalsIgnoreCase(".mp4") && recordingType(file)){
                return getAudioFileIcon(suffix);
            }
            /* @} */
            /* SPRD 450611 @{ */
            if(suffix.equalsIgnoreCase(".3gp") && recordingType(file)){
                return getAudioFileIcon(suffix);
            }
            /* @} */
            /* SPRD: Add for bug507035. @{ */
            if(suffix.equalsIgnoreCase(".3g2") && recordingType(file)){
                return getAudioFileIcon(suffix);
            }
            /* @} */
            return getVideoFileIcon(suffix);
        } else if (mAudioFileType.contains(suffix)) {
            return getAudioFileIcon(suffix);
        } else if (mTextFileType.contains(suffix)) {
            return FILE_TYPE_TEXT;
        } else if (mWebTextFileType.contains(suffix)) {
            return FILE_TYPE_WEBTEXT;
        } else if (mWordFileType.contains(suffix)) {
            return FILE_TYPE_WORD;
        } else if (mExcelFileType.contains(suffix)) {
            return FILE_TYPE_EXCEL;
        } else if (mPPTFileType.contains(suffix)) {
            return FILE_TYPE_PPT;
        } else if (mPdfFileType.contains(suffix)) {
            return FILE_TYPE_PDF;
        } else if (mWebTextFileType.contains(suffix)) {
            return FILE_TYPE_WEBTEXT;
        } else if (mPackageFileType.contains(suffix)) {
            return FILE_TYPE_PACKAGE;
        } else if (mVcardFileType.contains(suffix)) {
            return FILE_TYPE_VCARD;
        } else if (mVcalenderFileType.contains(suffix)) {
            return FILE_TYPE_VCALENDER;
        } else{
            return FILE_TYPE_UNKNOE;
        }
    }

    /**
     * confirm the docuement type
     * 
     * @param file
     * @return
     */
    public int getDocFileType(File file) {
        String suffix = getSuffix(file);
        if (suffix == null || !mDocFileType.contains(suffix)) {
            return FILE_TYPE_UNKNOE;
        } else if (mTextFileType.contains(suffix)) {
            return FILE_TYPE_TEXT;
        } else if (mWordFileType.contains(suffix)) {
            return FILE_TYPE_WORD;
        } else if (mExcelFileType.contains(suffix)) {
            return FILE_TYPE_EXCEL;
        } else if (mPPTFileType.contains(suffix)) {
            return FILE_TYPE_PPT;
        } else if (mPdfFileType.contains(suffix)) {
            return FILE_TYPE_PDF;
        } else {
            return FILE_TYPE_UNKNOE;
        }
    }

    public int getDocFileIcon(File file) {
        int ret = getDocFileType(file);
        if (ret == FILE_TYPE_UNKNOE) {
            ret = R.drawable.file_item_doc_ic;
        }
        return ret;
    }

    public int getVideoFileIcon(File file) {
        String suffix = getSuffix(file);
        return getVideoFileIcon(suffix);
    }

    public int getVideoFileIcon(String suffix) {
        if (suffix == null || !mVideoFileType.contains(suffix)) {
            return R.drawable.file_item_video_ic;
        } else if (suffix.equals(".mp4")) {
            return FILE_TYPE_VIDEO_MP4;
        } else if (suffix.equals(".3gp")) {
            return FILE_TYPE_VIDEO_3GP;
        /* SPRD: Add for bug507035. @{ */
        } else if (suffix.equals(".3g2")) {
            return FILE_TYPE_VIDEO_3G2;
        /* @} */
        /* SPRD: Add for bug611635. @{ */
        } else if (suffix.equals(".m2ts")) {
            return FILE_TYPE_VIDEO_M2TS;
        } else if (suffix.equals(".mov")) {
            return FILE_TYPE_VIDEO_MOV;
        /* @} */
        } else if (suffix.equals(".avi")) {
            return FILE_TYPE_VIDEO_AVI;
        } else if (suffix.equals(".flv")) {
            return FILE_TYPE_VIDEO_FLV;
        } else if (suffix.equals(".rmvb")) {
            return FILE_TYPE_VIDEO_RMVB;
        } else if (suffix.equals(".mkv")) {
            return FILE_TYPE_VIDEO_MKV;
        } else if (suffix.equals(".mpeg")) {
            return FILE_TYPE_VIDEO_MPEG;
        } else if (suffix.equals(".asf")) {
            return FILE_TYPE_VIDEO_ASF;
        } else if (suffix.equals(".divx")) {
            return FILE_TYPE_VIDEO_DIVX;
        } else if (suffix.equals(".mpe")) {
            return FILE_TYPE_VIDEO_MPE;
        } else if (suffix.equals(".mpg")) {
            return FILE_TYPE_VIDEO_MPG;
        /* SPRD: Modify for bug498813. @{
        } else if (suffix.equals(".rm")) {
            return FILE_TYPE_VIDEO_RM;
        @} */
        } else if (suffix.equals(".vob")) {
            return FILE_TYPE_VIDEO_VOB;
        } else if (suffix.equals(".wmv")) {
            return FILE_TYPE_VIDEO_WMV;
        //SPRD : Add for 496451
        } else if (suffix.equals(".ts")) {
            return FILE_TYPE_VIDEO_TS;
        /* SPRD 452695 @{ */
        } else if (suffix.equals(".m4v")) {
            return FILE_TYPE_VIDEO_M4V;
        } else if (suffix.equals(".f4v")) {
            return FILE_TYPE_VIDEO_F4V;
        } else if (suffix.equals(".webm")) {
            return FILE_TYPE_VIDEO_WEBM;
        }
        /*  @} */
        else
        {
            return R.drawable.file_item_video_ic;

        }
    }

    public int getAudioFileIcon(File file) {
        String suffix = getSuffix(file);
        return getAudioFileIcon(suffix);
    }

    public int getAudioFileIcon(String suffix) {
        if (suffix == null || !mAudioFileType.contains(suffix)) {
            return R.drawable.file_item_audio_ic;
        } else if (suffix.equals(".mp3")) {
            return FILE_TYPE_AUDIO_MP3;
        } else if (suffix.equals(".ogg")) {
            return FILE_TYPE_AUDIO_OGG;
        } else if (suffix.equals(".oga")) {
            return FILE_TYPE_AUDIO_OGA;
        } else if (suffix.equals(".wma")) {
            return FILE_TYPE_AUDIO_WMA;
        } else if (suffix.equals(".wav")) {
            return FILE_TYPE_AUDIO_WAV;
        } else if (suffix.equals(".acc")) {
            return FILE_TYPE_AUDIO_ACC;
        } else if (suffix.equals(".amr")) {
            return FILE_TYPE_AUDIO_AMR;
        } else if (suffix.equals(".aiff")) {
            return FILE_TYPE_AUDIO_AIFF;
        /* SPRD: Modify for bug505013. @{
        } else if (suffix.equals(".ape")) {
            return FILE_TYPE_AUDIO_APE;
        @} */
        } else if (suffix.equals(".av")) {
            return FILE_TYPE_AUDIO_AV;
        } else if (suffix.equals(".cd")) {
            return FILE_TYPE_AUDIO_CD;
        } else if (suffix.equals(".midi")) {
            return FILE_TYPE_AUDIO_MIDI;
        } else if (suffix.equals(".aac")) {
            return FILE_TYPE_AUDIO_AAC;
        } else if (suffix.equals(".mid")) {
            return FILE_TYPE_AUDIO_MID;
        } else if (suffix.equals(".m4a")) {
            return FILE_TYPE_AUDIO_M4A;
        } else if (suffix.equals(".vqf")) {
            return FILE_TYPE_AUDIO_VQF;
        } else if (suffix.equals(".imy")) {
            return FILE_TYPE_AUDIO_IMY;
        /* SPRD 435235 @{ */
        } else if (suffix.equals(".mp4")) {
            return FILE_TYPE_AUDIO_MP4;
        } else if (suffix.equals(".3gpp")) {
            return FILE_TYPE_AUDIO_3GPP;
        /* @} */
        /* SPRD 451527 @{ */
        } else if (suffix.equals(".3gp")) {
            return FILE_TYPE_AUDIO_3GP;
        /* @} */
        /* SPRD: Add for bug507035. @{ */
        } else if (suffix.equals(".3g2")) {
            return FILE_TYPE_AUDIO_3G2;
        /* @} */
        /* SPRD 498509 @{ */
        }else if (suffix.equals(".opus")) {
            return FILE_TYPE_AUDIO_OPUS;
        /* @} */
        /* SPRD 457501 @{ */
        } else if (suffix.equals(".awb")) {
        return FILE_TYPE_AUDIO_AWB;
        } else if (suffix.equals(".flac")) {
        return FILE_TYPE_AUDIO_FLAC;
        /* @} */
        /* SPRD: Add for bug510953. @{ */
        } else if (suffix.equals(".mka")) {
            return FILE_TYPE_AUDIO_MKA;
        /* @} */
        /* SPRD: Add for bug511015. @{ */
        } else if (suffix.equals(".m4b")) {
            return FILE_TYPE_AUDIO_M4B;
        } else if (suffix.equals(".m4r")) {
            return FILE_TYPE_AUDIO_M4R;
        /* @} */
        } else {
            return R.drawable.file_item_audio_ic;
        }
    }

    public static String getSuffix(File file) {
        if (file == null || !file.exists() || file.isDirectory()) {
            return null;
        }
        String fileName = file.getName();
        if (fileName.equals("") || fileName.endsWith(".")) {
            return null;
        }
        int index = fileName.lastIndexOf(".");
        if (index != -1) {
            return fileName.substring(index).toLowerCase(Locale.US);
        } else {
            return null;
        }
    }

    private String getSuffixByName(String fileName) {
        if (fileName.equals("") || fileName.endsWith(".")) {
            return null;
        }
        int index = fileName.lastIndexOf(".");
        if (index != -1) {
            return fileName.substring(index).toLowerCase(Locale.US);
        } else {
            return null;
        }
    }
    
    public static boolean recordingType (File file) {
        /* SPRD: Add for Bug 502687. @{ */
        Log.d(TAG, "start recordingType()");
        Cursor cursor = null;
        try {
            cursor = mContext.getContentResolver().query(MediaStore.Files.getContentUri("external"),
                    new String[] { "mime_type" }, MediaStore.Files.FileColumns.DATA + "=?",
                    new String[] { file.getAbsolutePath() }, null);
        } catch (Exception e) {
            Log.d(TAG, "Query database error!");
            e.printStackTrace();
        }
        /* @} */
        if(cursor != null && cursor.moveToFirst() && cursor.getCount() == 1){
            /* SPRD 446593 @{ */
            //if(cursor.getString(0).startsWith("audio/")){
            if(cursor.getString(0) != null && cursor.getString(0).startsWith("audio/")){
            /* @} */
                cursor.close();
                return true;
            }
        }
        if (cursor != null) {
            cursor.close();
        }
        return false;
    }
    
    public Set<String> getDocFileType() {
        return mDocFileType;
    }

    public Set<String> getPackageFileType() {
        return mPackageFileType;
    }

    public Set<String> getVideoFileType() {
        return mVideoFileType;
    }
}
