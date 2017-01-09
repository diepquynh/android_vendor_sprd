/**
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */
package com.ucamera.ucomm.downloadcenter;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLDecoder;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import javax.net.ssl.SSLException;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.apache.http.HttpStatus;
import org.xml.sax.SAXException;

import android.R.integer;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.LinearLayout.LayoutParams;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.graphics.drawable.Drawable;

import com.ucamera.ucomm.downloadcenter.ThumbnailBaseAdapter.ViewHolder;


/**
 * product download center for application, frames in background mode and Uphoto.
 *
 */
public class DownloadCenterActivity extends Activity implements OnClickListener {
    private static final String TAG = "DownloadCenterActivity";
    //select all items
    private TextView mBtnSelect;
    //uncheck the item
    private TextView mBtnCancel;
    //contains delete and download
    private TextView mBtnConfirm;
    //show Categories
    private GridView mGridViewTab;
    //show downloaded/pre-download items.
    private GridView mGridView;
    private String mLocalResPath;
    private String[] tabNames = null;
    private MyTabBaseAdapter myTabBaseAdapter = null;
    //download type is frame or decor
    private String mDownloadType;
    //frame is 1, and decor is 2, photoframe is 3
    private String mDownloadTypeStr;
   //screen density, 1.0 is mdpi, and more than 1.5 is hdpi
    private String mScreenDensity = "hdpi";
    //mdpi is 1, and hdpi is 2
    private String mScreenDensityStr = "2";
    //display all items
    private List<Object> mAllItems = null;
    //thumbnail adapter
    private ThumbnailBaseAdapter mThumbnailBaseAdapter = null;
    //item count
    private int mItemCount;
    //selected list
    private List<Object> mMultiSelected = null;
    //group list
    private ArrayList<GroupMode> mGroupModeList;
    //request url item, the item contains tab and thumbnail
    private String mRequestHttpUrl;
    //temp storage location
    private String mTempDownload = Environment.getExternalStorageDirectory().toString();
    //download path
    private String mDownloadPath = null;
    //loading dialog
    private ProgressDialog mLoadingDialog = null;
    //group id
    private String mGroupId;
    //selected count
    private int mSelectedCount;
    //download and delete file dialog
    private ProgressDialog mProgressDialog;
    private ProgressDialog mCancelDialog;
    private AlertDialog mDownloadDialog;
    private ProgressBar mProgressbar;
    private TextView mPercent;
    private TextView mAmount;
    private TextView mDownloaded;
    //downloading, press cancel key to stop downloading file.
    private boolean mIsStopDownload = false;
    //toast message
    private int mToastMessage;
    //get current language
    private String mLanguage;
    //downloaded list, as the non-update "downloaded" tab's files
    private ArrayList<String> mDownloadedList = null;
    private ArrayList<String> mAssertsList = null;
    private static final int ACTION_DOWNLOAD_SUCCESSED = 0;
    private static final int ACTION_DOWNLOAD_FAILED = 1;
    private static final int ACTION_DOWNLOAD_RESOURCE_SUCCESSED = 2;
    private static final int ACTION_DOWNLOAD_CANCEL = 3;
    private static final int ACTION_DOWNLOAD_LENGTH = 4;
    private static final int ACTION_DOWNLOAD_DOWNLOADING = 5;
    private static final int ACTION_DOWNLOAD_SINGLE_SUCCESSE = 6;

    private TGScrollViewOnTouch mThumbTGScrollViewOnTouch = new TGScrollViewOnTouch();
    private TGScrollViewOnTouch mTabTGScrollViewOnTouch = new TGScrollViewOnTouch();
    private HorizontalScrollView mThumbHorizontalScrollView;
    private HorizontalScrollView mTabHorizontalScrollView;
    private int mThumbNumColumns;
    private int mTabNumColumns;
    private int mThumbColumnsNumPreScreen;
    private ImageView mThumbLeftArrowImageView;
    private ImageView mThumbRightArrowImageView;
    private ImageView mTabLeftArrowImageView;
    private ImageView mTabRightArrowImageView;
    private int mScreenWidth;
    private int mScreenHeight;
    private TextView mTitleText;
    private LinearLayout mBottomLayout;
    private String mTitle;
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.download_center);
        /*
         * FIX BUG : 4900
         * BUG COMMENT : Download  center interface icons did not update to the latest resources.
         * DATE : 2013-11-12
         */
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(this);
        if(DownLoadUtil.upgradeDownloadCenterThumbnail(pref) ) {
            File file = new File(Constants.THUMBNAILS_DIR);
            if (file.exists()) {
                try {
                    DownLoadUtil.deletefile(file.getAbsolutePath());
                } catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        }
        Bundle extras = getIntent().getExtras();
        mDownloadType = extras.getString(Constants.ACTION_DOWNLOAD_TYPE);
        mAssertsList = new ArrayList<String>();
        initAssetsList();
        if(Constants.EXTRA_FRAME_VALUE.equals(mDownloadType)) {
            mDownloadTypeStr = "1";
            mTitle = getString(R.string.download_text_item_name_frame);
        } else if(Constants.EXTRA_DECOR_VALUE.equals(mDownloadType)) {
            mDownloadTypeStr = "2";
            mTitle = getString(R.string.download_text_edit_decoration);
        } else if (Constants.EXTRA_PHOTO_FRAME_VALUE.equals(mDownloadType)) {
            mDownloadTypeStr = "3";
            mTitle = getString(R.string.download_text_edit_photoframe);
        } else if (Constants.EXTRA_TEXTURE_VALUE.equals(mDownloadType)) {
            mDownloadTypeStr = "4";
            mTitle = getString(R.string.download_text_edit_texture);
        } else if (Constants.EXTRA_FONT_VALUE.equals(mDownloadType)) {
            mDownloadTypeStr = "5";
            mTitle = getString(R.string.download_font);
        }else if (Constants.EXTRA_PUZZLE_VALUE.equals(mDownloadType)) {
            mDownloadTypeStr = "6";
            mTitle = getString(R.string.download_text_puzzle);
        }else if(Constants.EXTRA_MANGA_FRAME_VALUE.equals(mDownloadType)) {
            mDownloadTypeStr = "7";
            mTitle = getString(R.string.download_manga);
        }

        mScreenWidth = getWindowManager().getDefaultDisplay().getWidth();
        mScreenHeight = getWindowManager().getDefaultDisplay().getHeight();
        mLanguage = Locale.getDefault().getLanguage();
        showProgressDialog();
        mGroupId = "-1";
        mLocalResPath = mTempDownload + "/UCam/download/" + mDownloadType + "/" + mDownloadType + "_" + mScreenDensity;
        File createTempHideFolder = new File(mTempDownload + "/UCam/.thumbnails");
        File tempFile1 = new File(mLocalResPath);
        File tempFile2 = new File(mTempDownload + "/UCam/download/" + mDownloadType + "/" + mDownloadType);
        Log.d(TAG, "tempFile1" + tempFile1);
        Log.d(TAG, "tempFile2" + tempFile2);
        /* SPRD:  CID 109327 : RV: Bad use of return value (FB.RV_RETURN_VALUE_IGNORED_BAD_PRACTICE)@{ */
        boolean mkdirs = true;
        if(!createTempHideFolder.exists()) {
            mkdirs = (mkdirs && createTempHideFolder.mkdirs());
        }
        if(!tempFile1.exists()) {
            mkdirs = (mkdirs && tempFile1.mkdirs());
        }
        if(!tempFile2.exists()) {
            mkdirs = (mkdirs && tempFile2.mkdirs());
        }
        if(!mkdirs){
            finish();
            return;
        }
        /**
        if(!createTempHideFolder.exists()) {
            createTempHideFolder.mkdirs();
        }
        if(!tempFile1.exists()) {
            tempFile1.mkdirs();
        }
        if(!tempFile2.exists()) {
            tempFile2.mkdirs();
        }
        */
        /* @} */
        mRequestHttpUrl = organizateUrl("list-groups", mDownloadTypeStr, "-1", null);
        Log.d(TAG, "mRequestHttpUrl = " + mRequestHttpUrl);
        mDownloadPath = organizatePath("groups_-" + mDownloadTypeStr + ".xml");
        Log.d(TAG, "mDownloadPath = " + mDownloadPath);

        /**
         * FIX BUG: 1007
         * BUG CAUSE: Maybe the downloa xml thread is executing and the initialization of the component is not yet complete.
         * FIX COMMENT: At first initialize the components and then start the thread.
         * DATE: 2012-05-17
         */
        setupView();
        new DownloadXmlThread().start();

        mMultiSelected = new ArrayList<Object>();
        mDownloadedList = new ArrayList<String>();

        if ("5".equals(mDownloadTypeStr)) {
            mThumbnailBaseAdapter = new ThumbnailBaseAdapter(this, mGridView,R.layout.download_center_thumbnail_font);
            //mGridView.setNumColumns(2);
        } else {
            mThumbnailBaseAdapter = new ThumbnailBaseAdapter(this, mGridView);
        }
        mGridView.setAdapter(mThumbnailBaseAdapter);
        if(mAllItems != null) {
            mAllItems.clear();
        } else {
            mAllItems = new ArrayList<Object>();
        }
        mTitleText.setText(mTitle);

        updateThumbnails(0);
    }
    private void initAssetsList() {
        String resBaseDir = mDownloadType + "/" + mDownloadType;
        try{
            String[] resNames = this.getAssets().list(resBaseDir);
            for(int i = 0; i < resNames.length; i++) {
                if(resNames[i].contains(".ucam")) {
                    mAssertsList.add(resNames[i]);
                }
            }
        } catch (Exception e) {
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_MENU) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    /**
     * init view
     */
    private void setupView() {
        mBtnSelect = (TextView) findViewById(R.id.btn_multi_all_select);
        mBtnSelect.setOnClickListener(this);
        mBtnCancel = (TextView) findViewById(R.id.btn_multi_cancel);
        mBtnCancel.setOnClickListener(this);
        mBtnConfirm = (TextView) findViewById(R.id.btn_multi_confirm);
        mBtnConfirm.setOnClickListener(this);
        mGridView = (GridView) findViewById(R.id.grid_thumbnail);

        /* FIX BUG: 5450
         * BUG CAUSE:  Remove the pressed effect when selected resources
         * Date: 2013-11-27
         */
        mGridView.setSelector(android.R.color.transparent);
        mGridView.setOnItemClickListener(new MyThumbnailOnItemClickListener());
        mGridView.setOnTouchListener(mThumbTGScrollViewOnTouch);
        mGridViewTab = (GridView) findViewById(R.id.gridview_tab);
        mGridViewTab.setOnTouchListener(mTabTGScrollViewOnTouch);
        mThumbLeftArrowImageView = (ImageView) findViewById(R.id.arrow_left);
        mThumbRightArrowImageView = (ImageView) findViewById(R.id.arrow_right);
        mThumbHorizontalScrollView = (HorizontalScrollView)findViewById(R.id.thumbnail_scrollview);
        mTabHorizontalScrollView = (HorizontalScrollView) findViewById(R.id.download_center_tab_scroller);
        mTabLeftArrowImageView = (ImageView) findViewById(R.id.arrow_left_tab);
        mTabLeftArrowImageView.setBackgroundColor(Color.TRANSPARENT);
        mTabRightArrowImageView = (ImageView) findViewById(R.id.arrow_right_tab);
        mTabRightArrowImageView.setBackgroundColor(Color.TRANSPARENT);
        mTitleText = (TextView) findViewById(R.id.download_title_text);
        mBottomLayout = (LinearLayout)findViewById(R.id.footer_three_buttons);
        Drawable drawable =getResources().getDrawable(R.drawable.download_image_action_bar);
        int itemHeight = drawable.getIntrinsicHeight();
        RelativeLayout.LayoutParams layoutParams= (RelativeLayout.LayoutParams)mBottomLayout.getLayoutParams();
        layoutParams.height = itemHeight;
        mBottomLayout.setLayoutParams(layoutParams);
    }

    private ArrayList<GroupMode> filterChineseFont(ArrayList<GroupMode> list){
        ArrayList<GroupMode> groupList = list;
        for(int i=0;i < groupList.size();i++){
            if(groupList.get(i).getGroupName().equals("中文字体") || groupList.get(i).getGroupName().equals("Chinese-Font")){
                groupList.remove(i);
                i--;
            }
        }
        return groupList;
    }

    /**
     * parser xml
     * @param parserXml downloaded the xml
     * @param groupId group id, -1 is default(downloaded), and more than -1 is server tab
     */
    private void parserXml(String parserXml, String groupId) {
        int groupIdInt = Integer.valueOf(groupId).intValue();
        try{
            SAXParserFactory factory = SAXParserFactory.newInstance();
            SAXParser parser = factory.newSAXParser();
            InputStream inputStream = new FileInputStream(parserXml);
            if(groupIdInt == -1) {
                GroupDefaultHandler groupHandler = new GroupDefaultHandler();
                parser.parse(inputStream, groupHandler);
                GroupsMode groupsMode = groupHandler.getGroupsMode();
                /* FIX BUG: 1138
                 * BUG CAUSE: supported null pointer
                 * Date: 2012-08-01
                 */
                if(groupsMode != null){
                    mGroupModeList = groupsMode.getModeList();
                }
                // filter chinese font for SourceNext
//                if (groupsMode != null && mGroupModeList != null
//                        && groupsMode.getGroupType().equals("font")
//                        && Build.FONT_DOWNLOAD.isCustom()){
//                    if(Build.isSourceNext()) {
//                        mGroupModeList = filterChineseFont(mGroupModeList);
//                    }
//                }
                int size = 0;
                if(mGroupModeList != null) {
                    size = mGroupModeList.size();
                }
                tabNames = new String[size + 1];
                tabNames[0] = getString(R.string.download_text_tab_local_name);
                for(int i = 0; i < size; i++) {
                    String groupName = mGroupModeList.get(i).getGroupName();
                    tabNames[i + 1] = groupName;
                }

                setDisplayItemCountsInWindow(size + 1, 3);
                myTabBaseAdapter = new MyTabBaseAdapter(this, tabNames);
                mGridViewTab.setAdapter(myTabBaseAdapter);
                mGridViewTab.setOnItemClickListener(new MyTabOnItemClickListener());
                myTabBaseAdapter.setHighlight(0);
            } else if(groupIdInt > -1) {
                ThumbnailDefaultHandler thumbnailHandler = new ThumbnailDefaultHandler();
                parser.parse(inputStream, thumbnailHandler);
                ThumbnailsMode thumbnailsMode = thumbnailHandler.getThumbnailsMode();
                ArrayList<ThumbnailMode> thumbnailModeList = null;
                /* FIX BUG: 5720
                 * BUG CAUSE: supported null pointer
                 * Date: 2014-01-09
                 */
                if(thumbnailsMode != null) {
                    thumbnailModeList = thumbnailsMode.getThumbnailsModeList();
                }
                if(thumbnailModeList != null && thumbnailModeList.size() > 0) {
                    for(ThumbnailMode thumbnailMode: thumbnailModeList) {
                        String fileName = mLocalResPath + "/" + thumbnailMode.getThumbnailName() + ".ucam";
                        String s = thumbnailMode.getThumbnailName() + ".ucam";
                        if((mDownloadedList != null && mDownloadedList.contains(fileName)) ||
                                (mAssertsList != null && mAssertsList.contains(s))) {
                            continue;
                        }
                        mAllItems.add(thumbnailMode);
                    }
                } else {
                    Toast.makeText(DownloadCenterActivity.this, R.string.download_text_request_no_resources, Toast.LENGTH_LONG).show();
                }

                updateThumbnails(myTabBaseAdapter.getHighlightIndex());
            }
            inputStream.close();
        } catch(SAXException e){
            Log.e(TAG, "parserXml(): SAXException, " + e);
            exceptionShow(groupIdInt);
            return;
        } catch(ParserConfigurationException e){
            Log.e(TAG, "parserXml(): ParserConfigurationException, " + e);
            exceptionShow(groupIdInt);
            return;
        } catch(IOException e){
            Log.e(TAG, "parserXml(): IOException, " + e);
            exceptionShow(groupIdInt);
            return;
        }
    }

    /**
     * exception, show the relative screen
     * @param groupIdInt group id
     */
    private void exceptionShow(int groupIdInt) {
        if(groupIdInt == -1) {
            tabNames = new String[]{getString(R.string.download_text_tab_local_name)};
            setDisplayItemCountsInWindow(1, 3);
            myTabBaseAdapter = new MyTabBaseAdapter(this, tabNames);
            mGridViewTab.setAdapter(myTabBaseAdapter);
            mGridViewTab.setOnItemClickListener(new MyTabOnItemClickListener());
            myTabBaseAdapter.setHighlight(0);
        } else if(groupIdInt > -1) {
            if(mAllItems != null) {
                mItemCount = mAllItems.size();
                mThumbnailBaseAdapter.clear();
                mThumbnailBaseAdapter.addItems(mAllItems);
                mThumbnailBaseAdapter.notifyDataSetChanged();
            }
        }
    }

    /**
     * load the thumbnails by highlightIndex
     * @param highlightIndex selected tab
     */
    private void updateThumbnails(int highlightIndex) {
        mThumbnailBaseAdapter.clear();
        mMultiSelected.clear();
        if(highlightIndex == 0) {
            if(mAllItems.size() > 0) {
                mAllItems.clear();
            }
            mDownloadedList.clear();
            File file = new File(mLocalResPath);
            File[] files = file.listFiles(new FileAccept(".ucam"));
            if(files != null && files.length > 0) {
                int len = files.length;
                for(int i = 0; i < len; i++) {
                    String filePath = files[i].getAbsolutePath();
                    mAllItems.add(filePath);
                    mDownloadedList.add(filePath);
                }
            }
            if(mAllItems != null) {
                mItemCount = mAllItems.size();
                mThumbnailBaseAdapter.clear();
                mThumbnailBaseAdapter.addItems(mAllItems);
                mThumbnailBaseAdapter.notifyDataSetChanged();
            }
        } else if(highlightIndex > 0) {
            if(mAllItems != null) {
                mItemCount = mAllItems.size();
                mThumbnailBaseAdapter.clear();
                mThumbnailBaseAdapter.addItems(mAllItems);
                mThumbnailBaseAdapter.notifyDataSetChanged();
            }
        }

        setThumbGridViewSizeByScreen();
        mThumbHorizontalScrollView.scrollTo(0, 0);
        mThumbTGScrollViewOnTouch.setCurrentView(mGridView);
        mThumbTGScrollViewOnTouch.updateScrollPosition();
        DownLoadUtil.setViewStatus(this, mMultiSelected);
        setViewText(highlightIndex, mBtnSelect, mBtnConfirm);
    }

    private void setThumbGridViewSizeByScreen() {

        int gridviewHeight;
        if (mThumbHorizontalScrollView.getHeight() != 0) {
            gridviewHeight = mThumbHorizontalScrollView.getHeight();
        } else {
            gridviewHeight = mScreenHeight
                    - (int) getDimensValue(R.dimen.download_gridview_thumbnail_height);
        }

        int thumbnailHeight;
        if ("5".equals(mDownloadTypeStr)) {
            thumbnailHeight = (int) getDimensValue(R.dimen.dd_font_thumbnails_height);
        } else {
            thumbnailHeight = (int) getDimensValue(R.dimen.dd_item_thumbnails_height);
        }

        // get thunbnail row number
        int thumbNumRow = (int) (gridviewHeight / (thumbnailHeight + getDimensValue(R.dimen.download_center_gridview_Spacing)));

        // get thumbnail column number
        mThumbNumColumns = (int) Math.ceil(mThumbnailBaseAdapter.getCount()
                / (float) thumbNumRow);

        int thumbnailWidth;
        if ("5".equals(mDownloadTypeStr)) {
            thumbnailWidth = (int) (getDimensValue(R.dimen.dd_item_thumbnails_width) * 2);
        } else {
            thumbnailWidth = (int) getDimensValue(R.dimen.dd_item_thumbnails_width);
        }

        mThumbColumnsNumPreScreen = (int) (mScreenWidth / (thumbnailWidth + getDimensValue(R.dimen.download_center_gridview_Spacing)));

        if (mThumbNumColumns < mThumbColumnsNumPreScreen
                && mThumbnailBaseAdapter.getCount() >= mThumbColumnsNumPreScreen) {
            mThumbNumColumns = mThumbColumnsNumPreScreen;
        } else if (mThumbnailBaseAdapter.getCount() < mThumbColumnsNumPreScreen) {
            mThumbNumColumns = mThumbnailBaseAdapter.getCount();
        }

        mGridView.setLayoutParams(new LinearLayout.LayoutParams(
                (mScreenWidth / mThumbColumnsNumPreScreen) * mThumbNumColumns,
                LayoutParams.WRAP_CONTENT));
        mGridView.setNumColumns(mThumbNumColumns);
    }

    private float getDimensValue(int id) {
        return getResources().getDimension(id);
    }
    /**
     * update button text by status or item count
     * @param highlightIndex selected tab
     * @param btnSelect select all button
     * @param btnConfirm delete or download button
     */
    public void setViewText(int highlightIndex, TextView btnSelect, TextView btnConfirm) {
        Drawable drawable= getResources().getDrawable(R.drawable.download_btn_status);
        String text = getResources().getString(R.string.download_text_multi_select_download);
        if(highlightIndex == 0) {
            drawable= getResources().getDrawable(R.drawable.download_delete_status);
            text = getResources().getString(R.string.download_text_multi_select_delete);
        }
        drawable.setBounds(0, 0, drawable.getMinimumWidth(), drawable.getMinimumHeight());
        btnConfirm.setCompoundDrawables(null, drawable, null, null);
        btnConfirm.setText(text);
        btnConfirm.setEnabled(false);
        btnSelect.setEnabled(mItemCount > 0);
    }

    /**
     * show progress dialog
     */
    private void showProgressDialog() {
        if(mLoadingDialog != null && !mLoadingDialog.isShowing()) {
            mLoadingDialog.show();
        } else {
            mLoadingDialog = ProgressDialog.show(this, "", getString(R.string.download_text_download_progress_message));
        }
    }

    /**
     * show cancel dialog
     */
    private void showCancelDialog() {
        if(mCancelDialog != null && !mCancelDialog.isShowing()) {
            mCancelDialog.show();
        } else {
            mCancelDialog = ProgressDialog.show(this, "", "");
        }
    }

    /**
     * reorganizate the url by parameters
     * @param action request parameter, contains list-groups and list-resources
     * @param type request parameter, contains frame and decor
     * @param groupId request parameter, group id
     * @param density request parameter, contains hdpi(2) and mdpi(1)
     * @return request url
     */
    private String organizateUrl(String action, String type, String groupId, String density) {
        String urlStr = null;
        int groupIdInt = Integer.valueOf(groupId).intValue();
        if(groupIdInt == -1) {
            urlStr = Constants.DOWNLOAD_RES + "?action=" + action + "&type=" + type + "&language=" + mLanguage;
        } else if(groupIdInt > -1) {
            urlStr = Constants.DOWNLOAD_RES + "?action=" + action + "&type=" + type + "&group-id=" + groupId + "&density=" + density;
        }
        return urlStr;
    }

    /**
     * reorganizate xml
     * @param xmlName downloaded local xml
     * @return parser xml path
     */
    private String organizatePath(String xmlName) {
        return mTempDownload + "/UCam/download/" + xmlName;
    }

    /**
     * calculate gridview columns and width
     * @param columns gridview columns
     * @param countPerPage count per page
     */
    private void setDisplayItemCountsInWindow(int columns, int countPerPage) {
        mTabNumColumns = columns;
        if(columns < countPerPage) {
            countPerPage = columns;
        }
        mGridViewTab.setNumColumns(columns);
        final int itemWidth = mScreenWidth / countPerPage;
        final int layout_width = itemWidth * columns;
        mGridViewTab.setLayoutParams(new LinearLayout.LayoutParams(layout_width, LayoutParams.WRAP_CONTENT));
        mTabTGScrollViewOnTouch.setCurrentView(mGridViewTab);
        mTabTGScrollViewOnTouch.updateScrollPosition();
    }

    /**
     * click group tab to display items
     * @param highlightIndex selected tab index
     */
    private void clickGroupTab(int highlightIndex) {
        mThumbnailBaseAdapter.clear();
        mThumbnailBaseAdapter.addItems(mAllItems);
        mThumbnailBaseAdapter.notifyDataSetChanged();
        showProgressDialog();
        GroupMode groupMode = mGroupModeList.get(highlightIndex - 1);
        String groupId = groupMode.getGroupId();
        mGroupId = groupId;
        mRequestHttpUrl = organizateUrl("list-resources", mDownloadTypeStr, groupId, mScreenDensityStr);
        mDownloadPath = organizatePath("thumbnails_" + groupId + ".xml");
        new DownloadXmlThread().start();
    }
    private int mCurrentTabIndex = 0;
    private class MyTabOnItemClickListener implements OnItemClickListener {
        public void onItemClick(AdapterView<?> adapterView, View view, int position, long id) {
            if(mCurrentTabIndex == position) {
                return;
            }
            mCurrentTabIndex = position;
            /*
             *  java.lang.ClassCastException: java.lang.String cannot be cast to com.ucamera.ucomm.downloadcenter.ThumbnailMode
             */
            mAllItems.clear();
            mMultiSelected.clear();
            myTabBaseAdapter.setHighlight(position);
            if(position == 0) {
                updateThumbnails(position);
            } else if(position > 0) {
                clickGroupTab(position);
            }
        }
    }

    private class MyThumbnailOnItemClickListener implements OnItemClickListener {
        public void onItemClick(AdapterView<?> adapterView, View view, int position, long id) {
            /*
             * FIX BUG: 4966
             * FIX COMMENT: avoid IndexOutOfBoundsException
             * DATE: 2013-10-11
             */
            if(position >= mAllItems.size()) return;
            Object object = mAllItems.get(position);
            ViewHolder viewHolder = (ViewHolder) view.getTag();
            viewHolder.checkebox.toggle();
            mThumbnailBaseAdapter.mCheckedMap.put(position, viewHolder.checkebox.isChecked());
            if(mMultiSelected.contains(object)) {
                mMultiSelected.remove(object);
            } else {
                mMultiSelected.add(object);
            }

            DownLoadUtil.setViewStatus(DownloadCenterActivity.this, mMultiSelected);
            mBtnSelect.setEnabled(mMultiSelected.size() != mItemCount);
            /*
             * FIX BUG: 5579
             * FIX COMMENT: add clickedimage effect;
             * DATE: 2013-12-23
             */
            mThumbnailBaseAdapter.notifyDataSetChanged();
        }
    }

    private class DownloadXmlThread extends Thread {
        public void run() {
            try {
                URL url=new URL(mRequestHttpUrl);
                HttpURLConnection httpURLConnection = (HttpURLConnection) url.openConnection();
                httpURLConnection.setDoInput(true);
                httpURLConnection.setConnectTimeout(5 * 1000);
                int contectionResultCode = httpURLConnection.getResponseCode();
                if(contectionResultCode == HttpStatus.SC_OK) {
                    Log.d(TAG, "contectionResultCode"+contectionResultCode);
                    InputStream input = httpURLConnection.getInputStream();
                    if (input == null) {
                        Log.d(TAG, "input"+input);
                        mToastMessage = R.string.download_text_network_link_exception;
                        mHandler.sendEmptyMessage(ACTION_DOWNLOAD_FAILED);
                        return;
                    }
                    FileOutputStream output = null;
                    try {
                        File file = new File(mDownloadPath);
                        if(file.exists()) {
                            /* SPRD: CID 109201 : RV: Bad use of return value (FB.RV_RETURN_VALUE_IGNORED_BAD_PRACTICE) @{ */
                            if(!file.delete()){
                                mToastMessage = R.string.download_text_network_link_exception;
                                mHandler.sendEmptyMessage(ACTION_DOWNLOAD_FAILED);
                                return;
                            }
                            // file.delete();
                            /* @} */
                        }
                        if(file.createNewFile()) {
                            output = new FileOutputStream(file);
                            byte buf[] = new byte[256];
                            do {
                                int numRead = input.read(buf);
                                if (numRead <= 0) {
                                    break;
                                }
                                output.write(buf, 0, numRead);
                            } while(true);

                            httpURLConnection.disconnect();
                            mHandler.sendEmptyMessage(ACTION_DOWNLOAD_SUCCESSED);
                        } else {
                            mToastMessage = R.string.download_text_network_link_exception;
                            mHandler.sendEmptyMessage(ACTION_DOWNLOAD_FAILED);
                        }
                    } catch (MalformedURLException e) {
                        mToastMessage = R.string.download_text_network_link_exception;
                        mHandler.sendEmptyMessage(ACTION_DOWNLOAD_FAILED);
                        e.printStackTrace();
                    } catch(SSLException e) {
                        mToastMessage = R.string.download_text_network_link_exception;
                        mHandler.sendEmptyMessage(ACTION_DOWNLOAD_FAILED);
                        e.printStackTrace();
                    }
                    catch(IOException e) {
                        mToastMessage = R.string.download_text_network_link_exception;
                        mHandler.sendEmptyMessage(ACTION_DOWNLOAD_FAILED);
                    } finally {
                        if(output != null) {
                            output.close();
                            output = null;
                        }

                        if(input != null) {
                            input.close();
                            input = null;
                        }
                    }
                } else {
                    mToastMessage = R.string.download_text_network_link_exception;
                    mHandler.sendEmptyMessage(ACTION_DOWNLOAD_FAILED);
                }
            } catch (MalformedURLException e) {
                mToastMessage = R.string.download_text_network_link_exception;
                mHandler.sendEmptyMessage(ACTION_DOWNLOAD_FAILED);
                e.printStackTrace();
            } catch(Exception e) {
                mToastMessage = R.string.download_text_network_link_exception;
                mHandler.sendEmptyMessage(ACTION_DOWNLOAD_FAILED);
            }
        }
    }

    @SuppressWarnings("deprecation")
    private void showDeleteProgessDialog() {
        if(mProgressDialog == null) {
            mProgressDialog = new ProgressDialog(this);
            mProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
            mProgressDialog.setMax(mSelectedCount);
            mProgressDialog.setCancelable(false);
            mProgressDialog.setButton(getText(R.string.download_text_multi_select_cancel), new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    mIsStopDownload = true;
                    showCancelDialog();
                }
            });
            mProgressDialog.show();
        }
    }

    private class DeleteFileThread extends Thread {
        public void run() {
            for(int i = 0; i < mSelectedCount; i++) {
                String thumbFilePath = (String)mMultiSelected.get(i);
                String filePath = thumbFilePath.replace(mDownloadType + "_" + mScreenDensity, mDownloadType);
                File thumbFile = new File(thumbFilePath);
                File file = new File(filePath);
                thumbFile.delete();
                file.delete();
                mDownloadedList.remove(thumbFilePath);
                Message message = new Message();
                message.arg1 = i + 1;
                message.what = ACTION_DOWNLOAD_RESOURCE_SUCCESSED;
                mHandler.sendMessage(message);
            }
        }
    }

    private class DownloadFileThread extends Thread {
        public void run() {
            for(int i = 0; i < mSelectedCount && i < mMultiSelected.size(); i++) {
                ThumbnailMode thumbnailMode = (ThumbnailMode)mMultiSelected.get(i);
                String thumbFileUrl = thumbnailMode.getThumnailUrl();
                String fileUrl = thumbnailMode.getDownloadUrl();
                String fileName = thumbnailMode.getThumbnailName() + ".ucam";
                String downloadingFileName = fileName + ".temp";
                thumbFileUrl = URLDecoder.decode(thumbFileUrl);
                fileUrl = URLDecoder.decode(fileUrl);
                // for stat
                if(thumbFileUrl != null && (thumbFileUrl.startsWith("http://") || thumbFileUrl.startsWith("www."))
                        && fileUrl != null && (fileUrl.startsWith("http://") || fileUrl.startsWith("www."))) {
                    String saveThumbPath = mLocalResPath + "/" + downloadingFileName;
                    String savePath = saveThumbPath.replace(mDownloadType + "_" + mScreenDensity, mDownloadType);
                    if (downloadFile(thumbFileUrl, saveThumbPath, false) && downloadFile(fileUrl, savePath, true)){
                        //successful
                        Message message = new Message();
                        message.arg1 = i + 1;
                        new File(saveThumbPath).renameTo(new File(saveThumbPath.replace(downloadingFileName, fileName)));
                        new File(savePath).renameTo(new File(savePath.replace(downloadingFileName, fileName)));
                        saveThumbPath = saveThumbPath.replace(downloadingFileName, fileName);
                        mAllItems.remove(thumbnailMode);
                        mDownloadedList.add(saveThumbPath);
                        message.what = ACTION_DOWNLOAD_SINGLE_SUCCESSE;
                        mHandler.sendMessage(message);
                    } else {
                        //failure
                        File thumbFile = new File(saveThumbPath);
                        File origFile = new File(savePath);
                        if(thumbFile.exists()) {
                            thumbFile.delete();
                        }
                        if(origFile.exists()) {
                            origFile.delete();
                        }

                        /*
                         *  mIsStopDownload == false means network exception
                         *  mIsStopDownload == true means canceled by user
                         */
                        if (!mIsStopDownload) {
                            mIsStopDownload = true;
                            runOnUiThread(new Runnable(){
                                @Override
                                public void run() {
                                    Toast.makeText(DownloadCenterActivity.this,
                                            R.string.download_text_network_link_exception, Toast.LENGTH_SHORT).show();
                                }
                            });
                        }
                    }
                }

                if(mIsStopDownload) {
                    mIsStopDownload = false;
                    Message message = new Message();
                    message.what = ACTION_DOWNLOAD_CANCEL;
                    mHandler.sendMessage(message);
                    break;
                }
            }
        }

        /*
         * BUG FIX: 2231
         * FIX COMMENT: when downloading file, can be canceld
         * DATE: 2013-01-05
         */
        private boolean downloadFile(String fileUrl, String saveFile, boolean isFile) {
            boolean isDownloadSuccessful = false;
            int length = 0;
            try {
                URL url = new URL(fileUrl);
                HttpURLConnection conn = (HttpURLConnection) url.openConnection();
                conn.setConnectTimeout(5 * 1000);
                /*
                 * Fix Bug: 1984
                 * Fix comment:  when read is timeout, will trigger an SocketException
                 * date: 2012-11-21
                 */
                conn.setReadTimeout(5*1000);
                conn.setRequestMethod("GET");
                if (mIsStopDownload) return false;
                InputStream inStream = conn.getInputStream();
                if (mIsStopDownload) return false;
                if(isFile)
                {
                    Message msg = new Message();
                    length = conn.getContentLength();
                    msg.arg1 = length;
                    msg.what = ACTION_DOWNLOAD_LENGTH;
                    mHandler.sendMessage(msg);
                }
                if(conn.getResponseCode() == HttpURLConnection.HTTP_OK) {
                    /**
                     * FIX BUG: 443
                     * BUG CAUSE: the input stream decod into the bitmap caused OOM.
                     * FIX COMMENT: through the input/output steam save into file
                     * DATE: 2012-01-20
                     */
                    FileOutputStream output = null;
                    try {
                        File file = new File(saveFile);
                        if(file.createNewFile()) {
                            int downloaded_length=0;
                            output = new FileOutputStream(file);
                            byte buf[] = new byte[1024];
                            do {
                                if (mIsStopDownload) return false;
                                int numRead = inStream.read(buf);
                                if (numRead <= 0) {
                                    break;
                                }
                                output.write(buf, 0, numRead);
                                Message msg = new Message();
                                if(isFile)
                                {
                                    downloaded_length += numRead;
                                    msg = Message.obtain(mHandler, ACTION_DOWNLOAD_DOWNLOADING);
                                    msg.what = ACTION_DOWNLOAD_DOWNLOADING;
                                    msg.arg1 = downloaded_length;
                                    msg.arg2 = downloaded_length*100/length;
                                    mHandler.sendMessage(msg);
                                }
                            } while(true);
                            output.flush();
                            isDownloadSuccessful = true;
                        }
                    } catch(IOException ioe) {
                        Log.w(TAG, "Error while downloading", ioe);
                    } finally {
                        conn.disconnect();
                        DownLoadUtil.closeSilently(output);
                        DownLoadUtil.closeSilently(inStream);
                    }
                }
            } catch (SSLException e) {
                Log.w(TAG, "SSLException Error when connecting", e);
            } catch (Exception e) {
                Log.w(TAG, "Error when connecting", e);
            }
            return isDownloadSuccessful;
        }
    }

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            /*
             * FIX BUG: 2943
             * BUG CAUSE: java.lang.IllegalArgumentException: View not attached to window manager
             * FIX COMMENT: If current Activity is finishing, does not execute other logic;
             * DATE: 2013-03-01
             */
            if(DownloadCenterActivity.this.isFinishing()) {
                return;
            }
            int what = msg.what;
            int per = msg.arg1;
            switch (what) {
                case ACTION_DOWNLOAD_SUCCESSED:
                    parserXml(mDownloadPath, mGroupId);
                    break;
                case ACTION_DOWNLOAD_FAILED:
                    Toast.makeText(DownloadCenterActivity.this, mToastMessage, Toast.LENGTH_LONG).show();
                    parserXml(mDownloadPath, mGroupId);
                    /*
                     * FIX BUG: 249
                     * BUG CAUSE: IllegalArgumentException:View not attached to window manager
                     * FIX COMMENT: dismiss the dialog only when the parent Activity is still alive
                     * DATE: 2012-07-16
                     */
                    if(mLoadingDialog != null && mLoadingDialog.isShowing() && !DownloadCenterActivity.this.isFinishing()) {
                        mLoadingDialog.dismiss();
                        mLoadingDialog = null;
                    }
                    break;
                case ACTION_DOWNLOAD_RESOURCE_SUCCESSED:
                    if(per >= mSelectedCount) {
                        /**
                         * FIX BUG: 344
                         * BUG CAUSE: cancel the download, the dialog has been dismissed.
                         * FIX COMMENT: judge the dialog status
                         * DATE: 2012-01-20
                         */
                        if(mProgressDialog != null && !DownloadCenterActivity.this.isFinishing()) {
                            mProgressDialog.incrementProgressBy(1);
                            mProgressDialog.dismiss();
                            mProgressDialog = null;
                        }
                        updateThumbnails(myTabBaseAdapter.getHighlightIndex());
                        if(mCancelDialog != null && !DownloadCenterActivity.this.isFinishing()) {
                            mIsStopDownload = false;
                            mCancelDialog.dismiss();
                            mCancelDialog = null;
                        }
                    } else {
                        if(mProgressDialog != null) {
                            mProgressDialog.incrementProgressBy(1);
                        }
                    }
                    break;
                case ACTION_DOWNLOAD_SINGLE_SUCCESSE:
                    if(per >= mSelectedCount) {
                        if(mDownloadDialog != null && !DownloadCenterActivity.this.isFinishing()) {
                            if(mDownloaded != null) {
                                mDownloaded.setText(Integer.toString(per));
                            }
                            mDownloadDialog.dismiss();
                            mDownloadDialog = null;
                        }
                        updateThumbnails(myTabBaseAdapter.getHighlightIndex());
                        if(mCancelDialog != null && !DownloadCenterActivity.this.isFinishing()) {
                            mIsStopDownload = false;
                            mCancelDialog.dismiss();
                            mCancelDialog = null;
                        }
                    } else {
                        if(mDownloadDialog != null) {
                            mDownloaded.setText(Integer.toString(per));
                        }
                    }
                    break;
                case ACTION_DOWNLOAD_CANCEL:
                    /**
                     * FIX BUG: 398
                     * BUG CAUSE: cancel the download, reload does not complete, start to click the frame or decor icon
                     * FIX COMMENT: add to reload the dialog box
                     * DATE: 2012-02-02
                     */
                    if(mProgressDialog != null && !DownloadCenterActivity.this.isFinishing()) {
                        mProgressDialog.dismiss();
                        mProgressDialog = null;
                    }
                    if(mDownloadDialog!=null && !DownloadCenterActivity.this.isFinishing()) {
                        mDownloadDialog.dismiss();
                        mDownloadDialog = null;
                    }
                    updateThumbnails(myTabBaseAdapter.getHighlightIndex());
                    if(mCancelDialog != null && !DownloadCenterActivity.this.isFinishing()) {
                        mCancelDialog.dismiss();
                        mCancelDialog = null;
                    }
                    break;
                case ACTION_DOWNLOAD_LENGTH:
                    if(mProgressbar != null) {
                        mProgressbar.setMax(per);
                    }
                    break;
                case ACTION_DOWNLOAD_DOWNLOADING:
                    if(mProgressbar != null) {
                        mProgressbar.setProgress(per);
                    }
                    if(mPercent != null) {
                        mPercent.setText(Integer.toString(msg.arg2)+"%");
                    }
                    if(msg.arg2 == 100) {
                        if(mProgressbar != null) {
                            mProgressbar.setProgress(0);
                        }
                        if(mPercent != null) {
                            mPercent.setText("0%");
                        }
                    }
                    break;
            }
            if(mLoadingDialog != null && mLoadingDialog.isShowing() && !DownloadCenterActivity.this.isFinishing()) {
                mLoadingDialog.dismiss();
                mLoadingDialog = null;
            }
        }
    };

    protected void onDestroy() {
        super.onDestroy();
        String tempDel = mTempDownload + "/UCam/download/";
        File file = new File(tempDel);
        File[] files = file.listFiles(new FileAccept(".xml"));
        if(files != null && files.length > 0) {
            for(File xmlFile : files) {
                xmlFile.delete();
            }
        }

        if (mThumbnailBaseAdapter != null) {
            mThumbnailBaseAdapter.stopImageLoader();
        }
    }

    private class MyTabBaseAdapter extends BaseAdapter {
        private Context mContext;
        private LayoutInflater mInflater;
        private String[] mTabNames;
        private int mHighlightIndex = -1;

        public MyTabBaseAdapter(Context context, String[] tabNames) {
            mContext = context;
            mInflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            mTabNames = tabNames;
        }

        public int getCount() {
            if(mTabNames != null && mTabNames.length > 0) {
                return mTabNames.length;
            } else {
                return 0;
            }
        }

        public Object getItem(int position) {
            if(mTabNames == null || position >= mTabNames.length) {
                return null;
            } else {
                return mTabNames[position];
            }
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            View view = null;
            if(convertView == null) {
                view = mInflater.inflate(R.layout.download_center_tab_item, parent, false);
            } else {
                view = convertView;
            }
            TextView tv = ((TextView)view.findViewById(R.id.tv_download_center_tab_name));
            tv.setText(mTabNames[position]);
            if(mHighlightIndex != -1 && mHighlightIndex == position) {
                view.setBackgroundResource(R.drawable.download_center_tab_pressed);
                tv.setSelected(true);
            } else {
                view.setBackgroundResource(R.drawable.download_center_tab_default);
                tv.setSelected(false);
            }

            return view;
        }

        public void setHighlight(int highlightIndex) {
            mHighlightIndex = highlightIndex;
            notifyDataSetInvalidated();
        }

        public int getHighlightIndex() {
            return mHighlightIndex;
        }
    }

    public void onClick(View v) {
        int viewId = v.getId();
        switch(viewId) {
            case R.id.btn_multi_all_select:
                toggleCheckboxStatus(true);
                mBtnSelect.setEnabled(false);
                break;
            case R.id.btn_multi_cancel:
                toggleCheckboxStatus(false);
                mBtnSelect.setEnabled(true);
                break;
            case R.id.btn_multi_confirm:
                int selectedPosition = myTabBaseAdapter.getHighlightIndex();
                if(mMultiSelected != null && mMultiSelected.size() > 0) {
                    mSelectedCount = mMultiSelected.size();
                }
                //showDeleteProgessDialog();
                if(selectedPosition == 0) {
                    showDeleteProgessDialog();
                    new DeleteFileThread().start();
                } else {
                    showDownloadDialog();
                    new DownloadFileThread().start();
                }
                break;
        }
    }
    private void showDownloadDialog()
    {
        if(mDownloadDialog==null)
        {
            Builder builder=new Builder(this);
            View view=LayoutInflater.from(this).inflate(R.layout.download_progress, null);
            mProgressbar = (ProgressBar)view.findViewById(R.id.download_bar);
            mAmount = (TextView)view.findViewById(R.id.download_amount);
            mDownloaded = (TextView)view.findViewById(R.id.download_single);
            mPercent = (TextView)view.findViewById(R.id.download_percent);
            mAmount.setText(Integer.toString(mSelectedCount));
            mDownloaded.setText("0");
            mPercent.setText("0%");
            builder.setView(view);
            builder.setCancelable(false);
            builder.setNegativeButton(getText(R.string.download_text_multi_select_cancel), new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    mIsStopDownload = true;
                    showCancelDialog();
                }
            });
            mDownloadDialog=builder.create();
            mDownloadDialog.show();
        }
    }
    /**
     * judge checkbox status
     * @param isSelectAll is selection all items
     */
    private void toggleCheckboxStatus(boolean isSelectAll) {
        /*
         * FIX BUG: 5015
         * BUG COMMENT: Fix index out of bounds exception
         * DATE: 2013-11-12
         */
        for (int index = 0; index < mAllItems.size(); index++) {
            if (isSelectAll) {
                mThumbnailBaseAdapter.mCheckedMap.put(index, true);
            } else {
                mThumbnailBaseAdapter.mCheckedMap.put(index, false);
            }
            Object object = mAllItems.get(index);
            View view = mGridView.getChildAt(index);
            if(mThumbnailBaseAdapter.mCheckedMap.get(index)) {
                if(view != null) {
                    ViewHolder vHollder = (ViewHolder) view.getTag();
                    vHollder.checkebox.setChecked(true);
                }
                if(!mMultiSelected.contains(object)) {
                    mMultiSelected.add(object);
                }
            } else {
                if(view != null) {
                    ViewHolder vHollder = (ViewHolder) view.getTag();
                    vHollder.checkebox.setChecked(false);
                }
                if(mMultiSelected.contains(object)){
                    mMultiSelected.remove(object);
                }
            }
          }

        DownLoadUtil.setViewStatus(this, mMultiSelected);
    }

    class TGScrollViewOnTouch implements View.OnTouchListener{

        Handler mHandler = new Handler();
        View currentTouchView;
        private float mScrollPosition;
        public void updateScrollPosition() {
            mScrollPosition = getScrollPosition();
            mHandler.postDelayed(r, 100);
        }

        public void setCurrentView(View view){
            currentTouchView = view;
        }

        public float getScrollPosition(){
            if(currentTouchView == null) return 0;
            if(currentTouchView.getId() == R.id.grid_thumbnail && mThumbHorizontalScrollView != null){
                return mThumbHorizontalScrollView.getScrollX();
            }else if(currentTouchView.getId() == R.id.gridview_tab && mTabHorizontalScrollView != null){
                return mTabHorizontalScrollView.getScrollX();
            }else{
                Log.e(TAG, "unknow view id,return 0");
                return 0;
            }
        }

        Runnable r = new Runnable() {
            public void run() {
                float tempScrollX = getScrollPosition();
                if (tempScrollX != mScrollPosition) {
                    mScrollPosition = tempScrollX;
                    mHandler.postDelayed(r, 100);
                }else{
                    setArrowVisible();
                }
            }
        };

        @Override
        public boolean onTouch(View view, MotionEvent event) {
            int action = event.getAction();
            currentTouchView = view;
            switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_MOVE:
            case MotionEvent.ACTION_CANCEL:
                mScrollPosition = getScrollPosition();
                mHandler.postDelayed(r, 100);
                break;
            }
            return false;
        }

        private void setArrowVisible() {

            if (currentTouchView == null)
                return;

            if (currentTouchView.getId() == R.id.grid_thumbnail) {
                if (mThumbNumColumns <= mThumbColumnsNumPreScreen) {
                    mThumbLeftArrowImageView.setVisibility(View.INVISIBLE);
                    mThumbRightArrowImageView.setVisibility(View.INVISIBLE);
                    return;
                }
                if (mScrollPosition <= 0) {
                    mThumbLeftArrowImageView.setVisibility(View.INVISIBLE);
                } else {
                    mThumbLeftArrowImageView.setVisibility(View.VISIBLE);
                }

                if (mScrollPosition >= (mGridView.getWidth() - mScreenWidth)) {
                    mThumbRightArrowImageView.setVisibility(View.INVISIBLE);
                } else {
                    mThumbRightArrowImageView.setVisibility(View.VISIBLE);
                }
            } else if (currentTouchView.getId() == R.id.gridview_tab) {
                if (mTabNumColumns <= 3) {
                    mTabLeftArrowImageView.setVisibility(View.INVISIBLE);
                    mTabRightArrowImageView.setVisibility(View.INVISIBLE);
                    return;
                }
                if (mScrollPosition <= 0) {
                    mTabLeftArrowImageView.setVisibility(View.INVISIBLE);
                } else {
                    mTabLeftArrowImageView.setVisibility(View.VISIBLE);
                }

                if (mScrollPosition >= (mGridViewTab.getWidth() - mScreenWidth)) {
                    mTabRightArrowImageView.setVisibility(View.INVISIBLE);
                } else {
                    mTabRightArrowImageView.setVisibility(View.VISIBLE);
                }
            }
        }
    }
}
