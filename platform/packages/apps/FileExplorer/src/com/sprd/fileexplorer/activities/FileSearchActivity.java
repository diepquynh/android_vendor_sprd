
package com.sprd.fileexplorer.activities;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;
import android.widget.Toast;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.drmplugin.FileSearchActivityUtils;
import com.sprd.fileexplorer.util.ActivityUtils;
import com.sprd.fileexplorer.util.FileSearch;
import com.sprd.fileexplorer.util.FileType;
import com.sprd.fileexplorer.util.StorageUtil;
import com.sprd.fileexplorer.util.IStorageUtil.StorageChangedListener;
public class FileSearchActivity extends Activity implements
        View.OnClickListener {

    // SPRD: Modify for bug509242.
    private static final String TAG = "MultiSelectActivity";
    private String mSearchKey;
    private CheckBox mImageType;
    private CheckBox mVideoType;
    private CheckBox mAudioType;
    private CheckBox mApkType;
    private CheckBox mOthertype;
    private CheckBox mDocType;
    private int mSearchLocationType;
    public Set<Integer> mSearchType = new HashSet<Integer>();
    private RelativeLayout mSearchViewContainer;
    private EditText mSearchView;
    private ImageView mClearAll;

    private View mSearchLoactionLayout;
    private TextView mSearchLocationTextView;
    private List<Integer> mSearchLocationList = new ArrayList<Integer>();

    private static final String IMAGE_KEY = "image_type";
    private static final String VIDEO_KEY = "video_type";
    private static final String AUDIO_KEY = "audio_type";
    private static final String APK_KEY = "apk_type";
    private static final String OTHER_KEY = "other_type";
    private static final String DOC_KEY = "doc_type";
    private SharedPreferences prfs;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d("FileSearchActivity", "onCreate");
        setContentView(R.layout.fragment_search_view);
        updateSearchLocationList();
        mSearchViewContainer = (RelativeLayout) findViewById(R.id.search_view_container);
        if (mSearchViewContainer != null) {
            mSearchView = (EditText) findViewById(R.id.search_view);
            mSearchView.setOnEditorActionListener(new OnEditorActionListener() {
                @Override
                public boolean onEditorAction(TextView v, int actionId,
                        KeyEvent event) {
                    mSearchKey = v.getText().toString();
                    if (mSearchKey != null && !mSearchKey.isEmpty()) {
                        if (mSearchKey.contains("*") || mSearchKey.contains("\"")
                                || mSearchKey.contains("\\") ||
                                mSearchKey.contains("/") || mSearchKey.contains("?")
                                || mSearchKey.contains("|")
                                || mSearchKey.contains("|") || mSearchKey.contains("<")
                                || mSearchKey.contains(">") || mSearchKey.contains("\'")) {
                            String invalidchar = "*\":/?|<>";
                            Toast.makeText(FileSearchActivity.this, getResources().getString(
                                    R.string.invalid_char, invalidchar), Toast.LENGTH_SHORT).show();
                            return true;
                        }
                        /* SPRD : NewFeature bug390464 show a dialog if you didn't select file type in FileSearchActivity @{ */
                        if (mSearchType.size() == 0) {
                            Toast.makeText(FileSearchActivity.this, R.string.search_type_empty,
                                    Toast.LENGTH_SHORT).show();
                            return true;
                        }
                        /* @} */
                        Bundle bun = new Bundle();
                        bun.putString(FileSearch.SEARCH_KEY, mSearchKey);
                        bun.putInt(FileSearch.SEARCH_LOCATION,
                                mSearchLocationType);
                        bun.putIntegerArrayList(FileSearch.SEARCH_TYPE,
                                new ArrayList<Integer>(mSearchType));
                        ActivityUtils.startSearchMode(bun,
                                FileSearchActivity.this);
  //                      finish();
                    } else {
                        Toast.makeText(FileSearchActivity.this, R.string.search_empty,
                                Toast.LENGTH_SHORT).show();
                    }
                    return true;
                }
            });
            mSearchView.addTextChangedListener(mTextWatcher);
            mSearchView.clearFocus();
            mClearAll = (ImageView) findViewById(R.id.clear_all_img);
            mClearAll.setOnClickListener(this);
            ActionBar actionbar = getActionBar();
            actionbar.setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP | ActionBar.DISPLAY_SHOW_TITLE);
            /* SPRD : BugFix bug356908 when goto search in fileexplorer ,no deafault typewriting @{ */
            this.getWindow().setSoftInputMode(
                    WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
            /* @} */
        }

        mImageType = (CheckBox) findViewById(R.id.fragment_search_type_image);
        mVideoType = (CheckBox) findViewById(R.id.fragment_search_type_vedio);
        mAudioType = (CheckBox) findViewById(R.id.fragment_search_type_audio);
        mApkType = (CheckBox) findViewById(R.id.fragment_search_type_apks);
        mDocType = (CheckBox) findViewById(R.id.fragment_search_type_document);
        mOthertype = (CheckBox) findViewById(R.id.fragment_search_type_other);
        mSearchLoactionLayout = findViewById(R.id.fragment_search_location_parent);
        mSearchLocationTextView = (TextView) findViewById(R.id.fragment_search_location_type);
        mImageType.setOnClickListener(this);
        mVideoType.setOnClickListener(this);
        mAudioType.setOnClickListener(this);
        mApkType.setOnClickListener(this);
        mDocType.setOnClickListener(this);
        mOthertype.setOnClickListener(this);
        prfs = PreferenceManager.getDefaultSharedPreferences(FileSearchActivity.this);
        // SPRD: Modify for bug607772, search the files in OTG devices.
        if (StorageUtil.getInternalStorageState()
                && (StorageUtil.getExternalStorageState() || StorageUtil.getUSBStorageState())) {
            mSearchLoactionLayout.setClickable(true);
            mSearchLoactionLayout.setOnClickListener(this);
            mSearchLocationTextView.setText(R.string.title_section_all);
        } else {
            mSearchLoactionLayout.setClickable(false);
            mSearchLocationTextView.setText(R.string.title_section_internal);
        }
        StorageUtil.addStorageChangeListener(mStorageChangedListener);
        initCheckBoxStatus();
        setSearchLoaction(0);
    }

    protected void onDestroy() {
        super.onDestroy();
        Log.d("FileSearchActivity", "onDestroy");
        StorageUtil.removeStorageChangeListener(mStorageChangedListener);
        storeSearchTypeToPreference();
    };

    TextWatcher mTextWatcher = new TextWatcher() {

        @Override
        public void afterTextChanged(Editable s) {
        }

        @Override
        public void beforeTextChanged(CharSequence s, int start, int count,
                int after) {
        }

        @Override
        public void onTextChanged(CharSequence queryString, int start,
                int before, int count) {
            if (queryString.length() >= 84) {
                Toast.makeText(FileSearchActivity.this, R.string.length_limited, Toast.LENGTH_SHORT)
                        .show();
            }
            if (!TextUtils.isEmpty(queryString.toString())) {
                if (mClearAll != null) {
                    mClearAll.setVisibility(View.VISIBLE);
                }
            } else {
                if (mClearAll != null) {
                    mClearAll.setVisibility(View.GONE);
                }
            }
        }
    };

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.fragment_search_type_image:
            case R.id.fragment_search_type_vedio:
            case R.id.fragment_search_type_audio:
            case R.id.fragment_search_type_document:
            case R.id.fragment_search_type_apks:
            case R.id.fragment_search_type_other:
                updateSearchType(v);
                break;
            case R.id.clear_all_img:
                if (mSearchView != null && mClearAll != null) {
                    mSearchView.setText("");
                    mClearAll.setVisibility(View.GONE);
                }
                break;
            case R.id.fragment_search_location_parent:
                showSearchLocationDialog();
                break;
        }
    }

    private void updateSearchType(View v) {
        boolean isChecked = false;
        if (v instanceof CheckBox) {
            isChecked = ((CheckBox) v).isChecked();
        }
        switch (v.getId()) {
            case R.id.fragment_search_type_image:
                if (!isChecked) {
                    mSearchType.remove(FileType.FILE_TYPE_IMAGE);
                    //first param:0-image,1-video,2-audio;second param:0-remove,1-add
                    FileSearchActivityUtils.getInstance().addOrRemoveDRMFile(0,0,this);
                } else {
                    //first param:0-image,1-video,2-audio;second param:0-remove,1-add
                    FileSearchActivityUtils.getInstance().addOrRemoveDRMFile(0,1,this);
                    mSearchType.add(FileType.FILE_TYPE_IMAGE);
                }
                break;
            case R.id.fragment_search_type_vedio:
                if (!isChecked) {
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_MP4);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_MKV);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_RMVB);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_3GP);
                    //SPRD: Add for bug507035.
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_3G2);
                    /* SPRD: Add for bug611635. @{ */
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_M2TS);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_MOV);
                    /* @} */
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_AVI);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_MPEG);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_FLV);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_ASF);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_DIVX);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_MPE);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_MPG);
                    // SPRD: Modify for bug498813.
                    //mSearchType.remove(FileType.FILE_TYPE_VIDEO_RM);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_VOB);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_WMV);
                    /* SPRD 466571 @{*/
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_M4V);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_F4V);
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_WEBM);
                    /* @} */
                    // SPRD: Add for bug 591051 support search ts type video
                    mSearchType.remove(FileType.FILE_TYPE_VIDEO_TS);
                    //add drm type
                  //first param:0-image,1-video,2-audio;second param:0-remove,1-add
                    FileSearchActivityUtils.getInstance().addOrRemoveDRMFile(1,0,this);
                } else {
                  //add drm type
                  //first param:0-image,1-video,2-audio;second param:0-remove,1-add
                    FileSearchActivityUtils.getInstance().addOrRemoveDRMFile(1,1,this);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_MP4);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_MKV);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_RMVB);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_3GP);
                    //SPRD: Add for bug507035.
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_3G2);
                    /* SPRD: Add for bug611635. @{ */
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_M2TS);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_MOV);
                    /* @} */
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_AVI);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_MPEG);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_FLV);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_ASF);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_DIVX);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_MPE);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_MPG);
                    // SPRD: Modify for bug498813.
                    //mSearchType.add(FileType.FILE_TYPE_VIDEO_RM);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_VOB);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_WMV);
                    /* SPRD 466571 @{*/
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_M4V);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_F4V);
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_WEBM);
                    /* @} */
                    // SPRD: Add for bug 591051 support search ts type video
                    mSearchType.add(FileType.FILE_TYPE_VIDEO_TS);
                }
                break;
            case R.id.fragment_search_type_audio:
                if (!isChecked) {
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_MP3);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_OGG);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_OGA);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_ACC);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_WAV);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_WMA);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_AMR);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_AIFF);
                    // SPRD: Modify for bug505013.
                    //mSearchType.remove(FileType.FILE_TYPE_AUDIO_APE);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_AV);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_CD);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_MIDI);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_VQF);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_AAC);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_MID);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_M4A);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_IMY);
                    /* SPRD 462456 @{ */
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_3GPP);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_MP4);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_3GP);
                    // SPRD: Add for bug507035.
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_3G2);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_AWB);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_FLAC);
                    // SPRD: Add for bug498509
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_OPUS);
                    /* @} */
                    // SPRD: Add for bug510953.
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_MKA);
                    /* SPRD: Add for bug511015. @{ */
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_M4B);
                    mSearchType.remove(FileType.FILE_TYPE_AUDIO_M4R);
                    /* @} */
                  //add drm type
                  //first param:0-image,1-video,2-audio;second param:0-remove,1-add
                    FileSearchActivityUtils.getInstance().addOrRemoveDRMFile(2,0,this);
                    
                } else {
                  //add drm type
                  //first param:0-image,1-video,2-audio;second param:0-remove,1-add
                    FileSearchActivityUtils.getInstance().addOrRemoveDRMFile(2,1,this);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_MP3);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_OGG);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_OGA);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_ACC);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_WAV);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_WMA);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_AMR);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_AIFF);
                    // SPRD: Modify for bug505013.
                    //mSearchType.add(FileType.FILE_TYPE_AUDIO_APE);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_AV);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_CD);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_MIDI);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_VQF);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_AAC);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_MID);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_M4A);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_IMY);
                    /* SPRD 462456 @{ */
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_3GPP);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_MP4);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_3GP);
                    // SPRD: Add for bug507035.
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_3G2);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_AWB);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_FLAC);
                    // SPRD: Add for bug498509
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_OPUS);
                    /* @} */
                    // SPRD: Add for bug510953.
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_MKA);
                    /* SPRD: Add for bug511015. @{ */
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_M4B);
                    mSearchType.add(FileType.FILE_TYPE_AUDIO_M4R);
                    /* @} */
                }
                break;
            case R.id.fragment_search_type_document:
                if (!isChecked) {
                    mSearchType.remove(FileType.FILE_TYPE_TEXT);
                    mSearchType.remove(FileType.FILE_TYPE_WORD);
                    mSearchType.remove(FileType.FILE_TYPE_EXCEL);
                    mSearchType.remove(FileType.FILE_TYPE_PPT);
                    mSearchType.remove(FileType.FILE_TYPE_PDF);
                    // SPRD: Add for bug471306, add drm type.
                    //first param:0-image,1-video,2-audio,3-doc,4-apk;second param:0-remove,1-add
                    FileSearchActivityUtils.getInstance().addOrRemoveDRMFile(3,0,this);
                } else {
                    // SPRD: Add for bug471306, add drm type.
                    //first param:0-image,1-video,2-audio,3-doc,4-apk;second param:0-remove,1-add
                    FileSearchActivityUtils.getInstance().addOrRemoveDRMFile(3,1,this);
                    mSearchType.add(FileType.FILE_TYPE_TEXT);
                    mSearchType.add(FileType.FILE_TYPE_WORD);
                    mSearchType.add(FileType.FILE_TYPE_EXCEL);
                    mSearchType.add(FileType.FILE_TYPE_PPT);
                    mSearchType.add(FileType.FILE_TYPE_PDF);
                }
                break;
            case R.id.fragment_search_type_apks:
                if (!isChecked) {
                    mSearchType.remove(FileType.FILE_TYPE_PACKAGE);
                    // SPRD: Add for bug471306, add drm type.
                    //first param:0-image,1-video,2-audio,3-doc,4-apk;second param:0-remove,1-add
                    FileSearchActivityUtils.getInstance().addOrRemoveDRMFile(4,0,this);
                } else {
                    // SPRD: Add for bug471306, add drm type.
                    //first param:0-image,1-video,2-audio,3-doc,4-apk;second param:0-remove,1-add
                    FileSearchActivityUtils.getInstance().addOrRemoveDRMFile(4,1,this);
                    mSearchType.add(FileType.FILE_TYPE_PACKAGE);
                }
                break;
            case R.id.fragment_search_type_other:
                if (!isChecked) {
                    mSearchType.remove(FileType.FILE_TYPE_UNKNOE);
                    mSearchType.remove(FileType.FILE_TYPE_VCARD);
                    mSearchType.remove(FileType.FILE_TYPE_VCALENDER);
                    mSearchType.remove(FileType.FILE_TYPE_WEBTEXT);
                    // SPRD: Add for bug507423, add drm type.
                    //first param:0-image,1-video,2-audio,3-doc,4-apk,5-others;second param:0-remove,1-add
                    FileSearchActivityUtils.getInstance().addOrRemoveDRMFile(5,0,this);
                } else {
                    mSearchType.add(FileType.FILE_TYPE_UNKNOE);
                    mSearchType.add(FileType.FILE_TYPE_VCARD);
                    mSearchType.add(FileType.FILE_TYPE_VCALENDER);
                    mSearchType.add(FileType.FILE_TYPE_WEBTEXT);
                    // SPRD: Add for bug507423, add drm type.
                    //first param:0-image,1-video,2-audio,3-doc,4-apk,5-others;second param:0-remove,1-add
                    FileSearchActivityUtils.getInstance().addOrRemoveDRMFile(5,1,this);
                }
                break;
        }
    }

    private void showSearchLocationDialog() {
        List<String> searchLocationStrs = new ArrayList<String>();
        for(int id: mSearchLocationList) {
            searchLocationStrs.add(getString(id));
        }
        new AlertDialog.Builder(this)
        .setTitle(R.string.fragment_search_location)
        .setAdapter(new ArrayAdapter<String>(this, android.R.layout.simple_spinner_dropdown_item, 
                searchLocationStrs), new DialogInterface.OnClickListener() {
            
            @Override
            public void onClick(DialogInterface dialog, int which) {
                setSearchLoaction(which);
            }
        }).show();
    }

    private void updateSearchLocationList() {
        boolean internalReady = StorageUtil.getInternalStorageState();
        boolean externalReady = StorageUtil.getExternalStorageState();
        // SPRD: Add for bug607772, search the files in OTG devices.
        boolean usbdiskReady = StorageUtil.getUSBStorageState();

        mSearchLocationList.clear();
        mSearchLocationList.add(R.string.title_section_all);
        if (internalReady) {
            mSearchLocationList.add(R.string.title_section_internal);
        }
        if (externalReady) {
            mSearchLocationList.add(R.string.title_section_external);
        }
        /* SPRD: Add for bug607772, search the files in OTG devices. @{ */
        if (usbdiskReady) {
            mSearchLocationList.add(R.string.title_section_usbdisk);
        }
        /* @} */
        if (mSearchLocationList.size() < 2) {
            mSearchLocationList.remove(0);
        }
    }

    public void setSearchLoaction(int position) {
        try{
            int iTextViewId = mSearchLocationList.get(position);
            switch(iTextViewId) {
                case R.string.title_section_all:
                    mSearchLocationType = FileSearch.SEARCH_ALL;
                    break;
                case R.string.title_section_internal:
                    mSearchLocationType = FileSearch.SEARCH_INTERNAL;
                    break;
                case R.string.title_section_external:
                    mSearchLocationType = FileSearch.SEARCH_EXTERNAL;
                    break;
                /* SPRD: Add for bug607772, search the files in OTG devices. @{ */
                case R.string.title_section_usbdisk:
                    mSearchLocationType = FileSearch.SEARCH_USB;
                    break;
                /* @} */

            }
            mSearchLocationTextView.setText(iTextViewId);
        }catch(Exception e){
            Log.e("FileSearchActivity","not mount storage card");
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                onBackPressed();
                return true;
        }
        return false;
    }

    private void initCheckBoxStatus() {
        mImageType.setChecked(prfs.getBoolean(IMAGE_KEY, false));
        mVideoType.setChecked(prfs.getBoolean(VIDEO_KEY, false));
        mAudioType.setChecked(prfs.getBoolean(AUDIO_KEY, false));
        mApkType.setChecked(prfs.getBoolean(APK_KEY, false));
        mDocType.setChecked(prfs.getBoolean(DOC_KEY, false));
        mOthertype.setChecked(prfs.getBoolean(OTHER_KEY, false));
        updateSearchType(mImageType);
        updateSearchType(mVideoType);
        updateSearchType(mAudioType);
        updateSearchType(mApkType);
        updateSearchType(mDocType);
        updateSearchType(mOthertype);
     }

    private void storeSearchTypeToPreference() {
         SharedPreferences.Editor editor = prfs.edit();
         editor.putBoolean(IMAGE_KEY, mImageType.isChecked());
         editor.putBoolean(VIDEO_KEY, mVideoType.isChecked());
         editor.putBoolean(AUDIO_KEY, mAudioType.isChecked());
         editor.putBoolean(APK_KEY, mApkType.isChecked());
         editor.putBoolean(DOC_KEY, mDocType.isChecked());
         editor.putBoolean(OTHER_KEY, mOthertype.isChecked());
         editor.commit();
     }

    StorageChangedListener mStorageChangedListener = new StorageChangedListener() {

        // SPRD: Modify for bug509242.
        @Override
        public void onStorageChanged(String path, boolean available, boolean sdcard) {
            Log.d(TAG, "StorageChanged: path = " + path + " available = " + available + "; sdcard = " + sdcard);
            if(available) {
                return;
            }
            new Handler(getMainLooper()).postDelayed(new Runnable() {

                @Override
                public void run() {
                    finish();
                }
            }, 1000);
        }
    };
}
