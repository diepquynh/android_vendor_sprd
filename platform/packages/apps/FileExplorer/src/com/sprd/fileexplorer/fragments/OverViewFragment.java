package com.sprd.fileexplorer.fragments;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.TextView;

import com.sprd.fileexplorer.R;
import com.sprd.fileexplorer.activities.FileExploreActivity;
import com.sprd.fileexplorer.activities.OverViewActivity;
import com.sprd.fileexplorer.adapters.FileAdapter;
import com.sprd.fileexplorer.util.ActivityUtils;
import com.sprd.fileexplorer.util.FileType;

public class OverViewFragment extends BaseFragment implements
        OnClickListener {
    private static final String TAG = "OverViewFragment";
    private static OverViewFragment mFragment;
    
    private Context mContext;
    private TextView mImageText;
    private TextView mVideoText;
    private TextView mAudioText;
    private TextView mDocText;
    private TextView mApkText;

    public OverViewFragment() {
        mFragment = this;
    }

    public synchronized static OverViewFragment getInstance(FileExploreActivity context) {
        // SPRD: Modify for bug465956.
        if (mFragment == null || context != mFragment.getActivity()) {
            mFragment = new OverViewFragment();
            mFragment.init(context);
        }
        return mFragment;
    }

    public void init(FileExploreActivity context) {
        mContext = context;
    }

    public synchronized static OverViewFragment getInstance() {
        if (mFragment == null) {
            throw new IllegalStateException("Shoud use getInstance(FileExploreActivity context) first");
        }
        return mFragment;
    }
    
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        mContext = activity;
    }
    
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {

        View view = inflater.inflate(R.layout.fragment_over_view, container,
                false);
        mAudioText = (TextView) view.findViewById(R.id.quickscan_audio_text);
        mImageText = (TextView) view.findViewById(R.id.quickscan_image_text);
        mVideoText = (TextView) view.findViewById(R.id.quickscan_video_text);
        mDocText = (TextView) view.findViewById(R.id.quickscan_doc_text);
        mApkText = (TextView) view.findViewById(R.id.quickscan_apk_text);
        view.findViewById(R.id.quickscan_image_layout).setOnClickListener(
                (OnClickListener) this);
        view.findViewById(R.id.quickscan_apk_layout).setOnClickListener(
                (OnClickListener) this);
        view.findViewById(R.id.quickscan_audio_layout).setOnClickListener(
                (OnClickListener) this);
        view.findViewById(R.id.quickscan_doc_layout).setOnClickListener(
                (OnClickListener) this);
        view.findViewById(R.id.quickscan_video_layout).setOnClickListener(
                (OnClickListener) this);
        return view;
    }

    @Override
    public void onClick(View v) {
        Intent intent = new Intent(mContext,OverViewActivity.class);
        int fileType = -1;
        switch (v.getId()) {
        case R.id.quickscan_image_layout:
            fileType = FileType.FILE_TYPE_IMAGE;
            break;
        case R.id.quickscan_audio_layout:
            fileType = FileType.FILE_TYPE_AUDIO;
            break;
        case R.id.quickscan_video_layout:
            fileType = FileType.FILE_TYPE_VIDEO;
            break;
        case R.id.quickscan_doc_layout:
            fileType = FileType.FILE_TYPE_DOC;
            break;
        case R.id.quickscan_apk_layout:
            fileType = FileType.FILE_TYPE_PACKAGE;
            break;
        }
        intent.putExtra("fileType", fileType);
        // SPRD: Modify for bug465956.
        startActivityForResult(intent, ActivityUtils.COPY_PATH_RESULT);
    }
    
    public boolean onBackPressed(){
        return true;
    }
    
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        mAudioText.setText(R.string.quickscan_audio);
        mImageText.setText(R.string.quickscan_image);
        mVideoText.setText(R.string.quickscan_video);
        mDocText.setText(R.string.quickscan_doc);
        mApkText.setText(R.string.quickscan_apk);
    }

    @Override
    public FileAdapter getAdapter() {
        return null;
    }

    @Override
    public int getIcon() {
        return R.drawable.main_fast;
    }
}
