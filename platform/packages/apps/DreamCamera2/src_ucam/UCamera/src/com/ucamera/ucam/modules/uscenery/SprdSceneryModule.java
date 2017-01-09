package com.ucamera.ucam.modules.uscenery;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.AsyncTask;
import android.util.Log;
import android.view.Display;
import android.view.OrientationEventListener;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.GridView;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.Toast;

import com.android.camera.CameraActivity;
import com.android.camera.Exif;
import com.android.camera.PhotoController;
import com.android.camera.app.AppController;
import com.android.camera.exif.ExifInterface;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.util.CameraUtil;
import com.android.camera.util.Size;
import com.android.camera2.R;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;
import com.android.ex.camera2.portability.CameraCapabilities;
import com.dream.camera.ButtonManagerDream;
import com.ucamera.ucam.jni.ImageProcessJni;
import com.ucamera.ucam.modules.BasicModule;
import com.ucamera.ucam.modules.BasicModule.NamedImages.NamedEntity;
import com.ucamera.ucam.modules.ui.BasicUI;
import com.ucamera.ucam.modules.ui.PreviewFrameLayout;
import com.ucamera.ucam.modules.ui.SprdSceneryUI;
import com.ucamera.ucam.modules.utils.Constants;
import com.ucamera.ucam.modules.utils.DownloadCenter;
import com.ucamera.ucam.modules.utils.LogUtils;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.ucamera.ucam.modules.utils.UiUtils;
import com.ucamera.ucam.modules.utils.Utils;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;

public class SprdSceneryModule extends BasicModule {

    public static final String SPRDSCENERY_MODULE_STRING_ID = "CAM_SprdSceneryModule";
    private static final String TAG = SPRDSCENERY_MODULE_STRING_ID;

    // CID 124835 : UuF: Unused field (FB.UUF_UNUSED_FIELD)
    // private ModeOptionsOverlay mModeOptionsOverLay;
    private ViewGroup mViewModuleRoot;
    private boolean mSkippedFirstOnResume;

    public SprdSceneryModule(AppController app) {
        super(app);
    }

    @Override
    public void init(CameraActivity activity, boolean isSecureCamera, boolean isCaptureIntent) {
        super.init(activity, isSecureCamera, isCaptureIntent);
        mActivity.setPreviewStatusListener(getUcamUI());
        mActivity.getCameraAppUI().setLayoutAspectRation(4f/3f);
//        BasicModule.isSceneryModule = true;
        //showToastFirstTime();//SPRD:fix bug605019
    }

    public void showToastFirstTime(){
        Toast.makeText(mActivity, R.string.scenery_module_warning,Toast.LENGTH_LONG).show();
    }

    /**
     * initialize module controls
     */
    protected void initializeModuleControls() {
        Log.d(TAG, "initializeModuleControls() S.");
        // CID 124834 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mAssetManager = mActivity.getAssets();

        ViewGroup viewRoot = mActivity.getCameraAppUI().getModuleView();
        mActivity.getLayoutInflater().inflate(R.layout.scenery_module, viewRoot);
        PreviewFrameLayout framePreview = (PreviewFrameLayout) viewRoot.findViewById(R.id.frame);
        framePreview.setAspectRatio(4D / 3D);

        mSceneryMenuLayout = viewRoot.findViewById(R.id.framelayout_scenery_menu_layout);
        mSceneryMenuScroller = (HorizontalScrollView) viewRoot.findViewById(R.id.scroller_scenery_menu);
        mSceneryGrid = (GridView) viewRoot.findViewById(R.id.grid_scenery_menu);
        mSceneryGrid.setOnItemClickListener(new SceneryItemListener());
        mReviewView = (ImageView) viewRoot.findViewById(R.id.scenery_review);

        mViewModuleRoot = mActivity.getCameraAppUI().getModuleRootView();
        if (mViewModuleRoot == null) {
            return; // ignore
        }

        // update options overlay
        View view = null;

        /* SPRD: Fix 474843 Add for Filter Feature @{ */
        if (UCamUtill.isUcamFilterEnable()) {
            // hide some buttons
            view = mViewModuleRoot.findViewById(R.id.btn_filter_button);
            if (view != null) {
                view.setVisibility(View.GONE);
            }
        }
        /* @} */
        Log.d(TAG, "initializeModuleControls() E.");
    }

    @Override
    public void makeModuleUI(PhotoController controller, View parent) {
        Log.d(TAG, "makeModuleUI S.");
        
        // nj dream test 55
        initializeModuleControls();
        // initialize BaseUI object
        getUcamUI();
        Log.d(TAG, "makeModuleUI E.");
    }

    @Override
    public void onPreviewStartedAfter() {
        Log.d(TAG, "onPreviewStartedAfter S.");
        // hide loading picture resource
        //mActivity.getCameraAppUI().onSurfaceTextureUpdated();//SPRD:fix bug616629
        // SPRD: Fix bug 607255 that scenery frame displays slowly
        if (mSkippedFirstOnResume) {
            mSkippedFirstOnResume = false;
            showMenu();
            getUcamUI();
        }
        mUI.intializeAIDetection(mActivity.getSettingsManager());
        Log.d(TAG, "onPreviewStartedAfter E.");
    }

    @Override
    protected void updateParametersBurstCount() {
        Log.i(TAG, "updateParametersBurstCount");
        CameraCapabilities.Stringifier stringifier = mCameraCapabilities.getStringifier();
        // CID 124818 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        // SettingsManager settingsManager = mActivity.getSettingsManager();
        mButstNumber = stringifier.burstNumberFromString(mActivity.getString(R.string.pref_camera_burst_entry_defaultvalue));
        mCameraSettings.setBurstPicNum(mButstNumber);
    }

    /* SPRD: fix bug463754 limit the picture size for scenery mode @{*/
    @Override
    protected void updateParametersPictureSize() {
        if (mCameraDevice == null) {
            Log.i(TAG, "attempting to set picture size without caemra device");
            return;
        }

        List<Size> supported = Size.convert(mCameraCapabilities.getSupportedPhotoSizes());
        Size pictureSize = getOnly43PictureSize(supported);
        if (pictureSize != null) {
            mCameraSettings.setPhotoSize(pictureSize.toPortabilitySize());
        }

        // SPRD: add fix bug 555245 do not display thumbnail picture in MTP/PTP Mode at pc
        mCameraSettings.setExifThumbnailSize(CameraUtil.getAdaptedThumbnailSize(pictureSize,
                mAppController.getCameraProvider()).toPortabilitySize());

        List<Size> sizes = Size.convert(mCameraCapabilities.getSupportedPreviewSizes());
//        List<Size> sizes = mCameraCapabilities.getSupportedPreviewSizes();
        Size optimalSize = CameraUtil.getOptimalPreviewSize(sizes,
                (double) pictureSize.width() / pictureSize.height());
//        Size original = mCameraSettings.getCurrentPreviewSize();
        Size original = new Size(mCameraSettings.getCurrentPreviewSize());
        if (!optimalSize.equals(original)) {
            Log.i(TAG, "setting preview size. optimal: " + optimalSize + "original: " + original);
            mCameraSettings.setPreviewSize(optimalSize.toPortabilitySize());

            mCameraDevice.applySettings(mCameraSettings);
            mCameraSettings = mCameraDevice.getSettings();
        }
        if (optimalSize.width() != 0 && optimalSize.height() != 0) {
            Log.i(TAG, "updating aspect ratio");
            mUI.updatePreviewAspectRatio((float) optimalSize.width()
                    / (float) optimalSize.height());
        }
        Log.d(TAG, "Preview size is " + optimalSize);
    }

    private Size getOnly43PictureSize(List<Size> supportedSizes) {
        List<Size> only43List = new ArrayList<Size>();
        List<com.android.ex.camera2.portability.Size> only43ListNew = new ArrayList<com.android.ex.camera2.portability.Size>();
        for (Size size : supportedSizes) {
            if (Utils.getOptimalSize(size.toPortabilitySize(),4,3) != null) {
                only43List.add(size);
                only43ListNew.add(size.toPortabilitySize());
            }
        }

        Size pictureSize = null;
        if (only43List.size() > 0) {
            Size maxSize = new Size(1600, 1200);
            Size minSize = new Size(320, 240);
            List<Size> selectedList = Size.convert(Utils.filterSizeItem(only43ListNew, minSize.toPortabilitySize(), maxSize.toPortabilitySize()));
            if (selectedList.size() > 0) {
                Size firstSize = selectedList.get(0);
                Size lastSize = selectedList.get(selectedList.size() - 1);
                if (firstSize.width() * firstSize.height() <= lastSize.width() * lastSize.height()) {
                    pictureSize = lastSize;
                } else {
                    pictureSize = firstSize;
                }
            } else {
                Size firstSize = only43List.get(0);
                Size lastSize = only43List.get(selectedList.size() - 1);
                if (firstSize.width() * firstSize.height() <= lastSize.width() * lastSize.height()) {
                    pictureSize = lastSize;
                } else {
                    pictureSize = firstSize;
                }
            }
        }

        return pictureSize;
    }
    /* @}*/
    //////////////////////////////////// Choose Panel ////////////////////////////////////////
    // CID 124834 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    //private boolean mIsHorizontalFrame;

    private int mItemWidth;
    private String[] mFrameImage;
    private int mCurrentScenery = 0;
    private String mCurrentSceneryName;

    private GridView mSceneryGrid;
    // CID 124834 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private AssetManager mAssetManager;
    private SceneryAdapter mSceneryAdapter;

    private ImageView mReviewView;
    public View mSceneryMenuLayout;
    private HorizontalScrollView mSceneryMenuScroller;

    /*SPRD:fix bug624552 scenery picture not show on time @{ */
    private void loadAssetResource() {
        DownloadCenter.loadAssetsFrame(mActivity, new DownloadCenter.OnLoadCallback() {
            @Override
            public void onLoadComplete(String[] result) {
                Log.i(TAG, "onLoadComplete");
                if (result != null && result.length > 0) {
                    mFrameImage = result;
                    int totalCount = result.length;
                    if (mSceneryAdapter != null) {
                        mSceneryAdapter.updateItems(result);
                    } else {
                        mSceneryAdapter = new SceneryAdapter(mActivity, result);
                    }
                    mSceneryGrid.setAdapter(mSceneryAdapter);
                    setDisplayItemCountsInWindow(totalCount);

                    if (!mSkippedFirstOnResume) {
                        showMenu();
                    }
                }
            }
        });
    }
    /* @} */

    private void showMenu() {
        if (mFrameImage !=null && mFrameImage.length > 0) {
            /*
             * FIX BUG: 5286
             * BUG COMMENT: get current scenery index by last scenery name
             * DATE: 2013-11-20
             */
            mCurrentScenery = mSceneryAdapter.getIndexOf(mCurrentSceneryName);

            /*
             * FIX BUG: 5597
             * BUG COMMETN: The HorizontalScrollView will scroll to the Correct position
             * DATE: 2013-12-13
             */
            UiUtils.scrollToCurrentPosition(
                mSceneryMenuScroller, mItemWidth, mCurrentScenery);
//            mSceneryMenuScroller.scrollTo(mItemWidth * mCurrentScenery, mSceneryMenuScroller.getScrollY());
            if(mCurrentScenery == 0) {
                mCurrentSceneryName = (String) mSceneryAdapter.getItem(mCurrentScenery);
            }
            mSceneryAdapter.setHighlight(mCurrentScenery);
            mSceneryMenuLayout.setVisibility(View.VISIBLE);
            setSceneSelectFrameView(mCurrentScenery);
        }
    }

    private void setDisplayItemCountsInWindow(int totalCount) {
//        mItemWidth = UiUtils.effectItemWidth();
//        mSceneryGrid.setNumColumns(totalCount);
//        final int layout_width = mItemWidth * totalCount;
//        mSceneryGrid.setLayoutParams(new LinearLayout.LayoutParams(layout_width,
//                LayoutParams.WRAP_CONTENT));
        WindowManager wm = mActivity.getWindowManager();
        int scrnW = wm.getDefaultDisplay().getWidth()+wm.getDefaultDisplay().getWidth()/10;
        int numberPerSrcn = 5;
        int itemLayoutW = scrnW / numberPerSrcn;
        int itemLayoutH = itemLayoutW;
        float offset = 0;
        float itemW = scrnW / (numberPerSrcn + offset);
        int layoutW = (int) (itemW * totalCount);
        mSceneryGrid.setNumColumns(totalCount);
        mSceneryGrid.setLayoutParams(new LinearLayout.LayoutParams(layoutW,
                LinearLayout.LayoutParams.WRAP_CONTENT));
    }

    private class SceneryItemListener implements OnItemClickListener {
        @Override
        public void onItemClick(AdapterView<?> adapterView, View view, int position, long id) {
            LogUtils.debug(TAG, "select scenery " + position);
            LogUtils.debug(TAG, "mFrameImage " + mFrameImage.length);
            LogUtils.debug(TAG, "mCurrentScenery " + mCurrentScenery);
            if (mCurrentScenery == position) {
                return;
            }
            if (position == 0 && DownloadCenter.RESOURCE_DOWNLOAD_ON) {
                // go to download center
                //mCurrentScenery = 0;
                DownloadCenter.openResourceCenter(mActivity, Constants.EXTRA_FRAME_VALUE);
            } else {
                mSceneryAdapter.setHighlight(position);
                mCurrentScenery = position;
                setSceneSelectFrameView(mCurrentScenery);
                mCurrentSceneryName = (String) mSceneryAdapter.getItem(position);
            }
        }
    }

    // show the scene frame background when turn to the scene mode
    protected void setSceneSelectFrameView(int currentSceneMode) {
        if (mFrameImage == null)
            return;
        if (currentSceneMode < 0 || currentSceneMode > mFrameImage.length - 1) {
            LogUtils.error(TAG, "currentSceneMode(" + currentSceneMode + ") should between [0,"
                    + mFrameImage.length + ")");
            return;
        }

        String fileName = mFrameImage[currentSceneMode];
        Bitmap bitmap = showBitmap(fileName);
        /*
         * FIX BUG: 4800
         * BUG COMMENT: don't set bitmap to mFrameBitmap if it is null or has recycled
         * DATE: 2013-09-04
         */
        if(bitmap != mFrameBitmap && bitmap != null && !bitmap.isRecycled()) {
            Utils.recycleBitmap(mFrameBitmap);
            mFrameBitmap = bitmap;
        }

        if(mFrameBitmap != null) {
            setSceneryReviewParams();
            mReviewView.setImageBitmap(mFrameBitmap);
            mReviewView.setVisibility(View.VISIBLE);
        }
    }

    /* SPRD:fix bug465266 photoframe did not align with preivew @{*/
    private void setSceneryReviewParams() {
        RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) mReviewView.getLayoutParams();
        RectF rect = mActivity.getCameraAppUI().getPreviewArea();
        if (rect == null) {
            return;
        }
        WindowManager wm = mActivity.getWindowManager();
        Display display = wm.getDefaultDisplay();
        int width = display.getWidth();
        int height = display.getHeight();
        if (width > height) {
            height = width;
        }
        LogUtils.debug(TAG, " top = " + rect.top + " bottom = " + rect.bottom + " right = " + rect.right + " width = " + width + " height = " + height);
        params.setMargins(0, (int)rect.top, 0, (int)(height - rect.right * 4 / 3 - rect.top));
        mReviewView.setLayoutParams(params);
    }
    /* @}*/

    private Bitmap showBitmap(String fileName) {
        Bitmap bitmap = null;
        try {
            bitmap = DownloadCenter.assetsThumbNameToBitmap(mActivity, fileName, Constants.EXTRA_FRAME_ORIGINAL_VALUE);

            if (bitmap != null) {
                int bitmapWidth = bitmap.getWidth();
                int bitmapHeight = bitmap.getHeight();
                /* FIX BUG: 3096
                 * BUG CAUSE: the bitmap has been rotated with camera orientation change
                 * FIX COMMENT: not rotate bitmap when camera orientation has changed
                 * DATE: 2013-03-06
                 */
                if (bitmapWidth > bitmapHeight) {
                    // CID 124834 : UrF: Unread field (FB.URF_UNREAD_FIELD)
                    //mIsHorizontalFrame = true;
                    bitmap = Utils.rotate(bitmap, 90);
                } //else {
                    //mIsHorizontalFrame = false;
                //}
            }
        } catch (Exception e) {
            bitmap = null;
            e.printStackTrace();
        }

        /*
         * 
        if(com.ucamera.ucam.variant.Build.UCAM_FULL_SIZE.isOn()){
            String data_filename = null;
            if (fileName.startsWith(DownloadCenter.DOWNLOAD_DIRECTORY + "frame/frame_hdpi")) {
                String fn = fileName;
                int index = fileName.lastIndexOf("/") + 1;
                if (index >= fileName.length()) {
                    LogUtils.debug(TAG, "the "+fileName+" is incorrect!");
                } else {
                    fn = fileName.substring(index);
                }
                fileName = fileName.replace("frame_hdpi", Constants.EXTRA_FRAME_VALUE);
                data_filename = Constants.FULL_SIZE_FRAME_PATH + fn;
                copyFile(fileName, data_filename, false);
            } else {
                data_filename = Constants.FULL_SIZE_FRAME_PATH + fileName;
                fileName = "frame/frame/"+fileName;
                InputStream is = null;
                try {
                    is = mAssetManager.open(fileName);
                    copyFile(is, data_filename, false);
                } catch (Exception e) {
                    LogUtils.debug(TAG, "fail copy file:" + fileName + "=>" + data_filename);
                } finally {
                    Utils.closeSilently(is);
                }
            }
            DownloadCenter.setFileName(data_filename);
        }
                     */

        if (DownloadCenter.getFileName() != null) {
            String strParam = "TEXTURE=resourceTexture," + DownloadCenter.getFileName();
            mSceneryParams = ImageProcessJni.encrypt(strParam);
        }
        return bitmap;
    }

    /* CID 124833 : UPM: Private method is never called (FB.UPM_UNCALLED_PRIVATE_METHOD)
    private void copyFile(String src, String dist,  boolean override){
        InputStream is = null;
        try {
            is = new FileInputStream(src);
            copyFile(is, dist, override);
        } catch (Exception ex){
            LogUtils.debug(TAG,"Fail copy " + src + "=>" + dist, ex);
        } finally {
            Utils.closeSilently(is);
        }
    }

    private void copyFile(InputStream is, String dist, boolean override){
        File fd = new File(dist);

        if (fd.exists() && !override) {
            LogUtils.debug(TAG, dist + " exists and not override.");
            return;
        }

        if (!fd.getParentFile().exists()){
            fd.getParentFile().mkdir();
            FileUtils.setPermissions(
                    fd.getParentFile().getPath(),
                    FileUtils.S_IRWXU|FileUtils.S_IRWXG|FileUtils.S_IXOTH | FileUtils.S_IROTH,
                    -1, -1);
        }

        if (FileUtils.copyToFile(is,fd)){
            FileUtils.setPermissions(
                    fd.getPath(),
                    FileUtils.S_IRWXU|FileUtils.S_IRWXG|FileUtils.S_IROTH,
                    -1, -1);
        } else {
            LogUtils.error(TAG,"Fail copy to file " + dist);
        }
    }
    */
//////////////////////////////////////// Save Image //////////////////////////////////////
    private Bitmap mBitmap;
    private Bitmap mFrameBitmap;

    protected byte[] mJpegData;
    private boolean mIsScenerySaving;
    public static String mSceneryParams;

    @Override
     protected final void saveFinalPhoto(final byte[] jpegData, NamedEntity name, final ExifInterface exif,
            CameraProxy camera){
        mJpegData = jpegData;

        int orientation = Exif.getOrientation(exif);
        initBitmapAccordingImageData(mJpegData, orientation);

        if (mBitmap == null) {
            LogUtils.error(TAG, "Decode bitmap failed, buffer length: " + ((mJpegData == null) ? "(?)" : mJpegData.length));
            return;
        }

        postHandleSceneryMode(exif, camera, name);
        /* SPRD:ADD for bug 461734 @{ */
        if (!mIsImageCaptureIntent && sFreezeFrameControl != null &&
            sFreezeFrameControl.isFreezeFrame()) {
            sFreezeFrameControl.proxyAnimation(true,mDisplayOrientation);
            mAppController.getCameraAppUI().onShutterButtonClick();
        }
        /* @} */
    }

    private void initBitmapAccordingImageData(final byte[] jpegData, int orientation) {
        Bitmap oldBitmap = mBitmap;
        if (jpegData == null) {
            return;
        }

        BitmapFactory.Options options = Utils.getNativeAllocOptions();
        mBitmap = BitmapFactory.decodeByteArray(jpegData, 0, jpegData.length, options);
        Utils.recycleBitmap(oldBitmap);
    }

    protected Bitmap rotateToScreen(Bitmap bitmap, int exifOrientation) {
        int screenOrientation = mAppController.getOrientationManager().getDeviceOrientation().getDegrees();

        if (screenOrientation == OrientationEventListener.ORIENTATION_UNKNOWN) {
            screenOrientation = mCameraDisplayOrientation;
        }
        LogUtils.debug(TAG, "screenOrientation = " + screenOrientation + ", exifOrientation = " + exifOrientation);
        // SPRD: fix bug 553451 the scene is not consistent with the preview frame
        return rotateAndMirror(bitmap, screenOrientation, exifOrientation
                , screenOrientation - exifOrientation, isCameraFrontFacing());
    }

    private Bitmap rotateAndMirror(Bitmap b, int screenOrientation, int exifOrientation, int degrees, boolean mirror) {
        if (b == null) return null;

        Matrix m = new Matrix();
        if (mirror) {
            m.postRotate(screenOrientation,
                    (float) b.getWidth() / 2, (float) b.getHeight() / 2);
            if (!isCameraMirror()) {
                m.postScale(-1, 1);
            }
            if (exifOrientation != 0) {
                m.postRotate(-exifOrientation,
                        (float) b.getWidth() / 2, (float) b.getHeight() / 2);
            }
        } else {
            m.postRotate(degrees,
                    (float) b.getWidth() / 2, (float) b.getHeight() / 2);
        }

        Bitmap rotatedBitmap = null;
        try {
            rotatedBitmap = Bitmap.createBitmap(
                    b, 0, 0, b.getWidth(), b.getHeight(), m, true);
        } catch (OutOfMemoryError ex) {
            // We have no memory to rotate. Return the original bitmap.
        }
        return rotatedBitmap;
    }

    private void postHandleSceneryMode(
            final ExifInterface exif, final CameraProxy camera, final NamedEntity name) {

        new AsyncTask<Void, Void, byte[]>() {
            @Override
            protected void onPreExecute() {
                mIsScenerySaving = true;
            }

            @Override
            protected byte[] doInBackground(Void... arg0) {
                return pictureTakenSceneryProcess(exif);
            }

            @Override
            protected void onPostExecute(byte[] result) {
                /* SPRD:Fix for bug 461734 @{ */
                if (result != null) {
                    SprdSceneryModule.super.saveFinalPhoto(result,  name, exif, camera);
                } else {
                    SprdSceneryModule.super.saveFinalPhoto(mJpegData, name, exif, camera);
                }
                mJpegData = null;
                /* @} */
                mIsScenerySaving = false;
            }
        }.execute();

    }

    @Override
    public boolean isCameraIdle() {
        if (mIsScenerySaving) {
            return false;
        }
        return super.isCameraIdle();
    }

    private byte[] pictureTakenSceneryProcess(ExifInterface exif) {
        if (mFrameBitmap == null || mFrameBitmap.isRecycled()) {
            return null;
        }
        /*
         * AFTER calling: mSceneryBitmap may be same as mBitmap
         */
        Bitmap sceneryBitmap = createCompositeSceneryImage(exif);
        if (sceneryBitmap == null) {
            return null;
        }
        // to add the timestamp, we should prepare a vertical picture
        /**
         * SPRD: nj dream camera test debug 131 @{
         *
        int jpegOrientation = 0;

        if(mIsHorizontalFrame) {
            jpegOrientation = 270;
        }
         */
        int jpegOrientation = Exif.getOrientation(exif);
        /* @} */

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        sceneryBitmap.compress(Bitmap.CompressFormat.JPEG, 90, baos);
        sceneryBitmap.recycle();
        byte[] jpegData = baos.toByteArray();
        Utils.closeSilently(baos);

        jpegData = ImageProcessJni.SetJpegOrientation(jpegData, jpegOrientation);
        return jpegData;
    }

    /*
     * Operate on mBitmap and return the mBitmap.
     */
    private Bitmap createCompositeSceneryImage(ExifInterface exif) {
        /*
         * FIX BUG: 1112 BUG CAUSE: supported null pointer Date: 2012-06-18
         */
        if (mBitmap == null) {
            return null;
        }
        final int width = mBitmap.getWidth();
        final int height = mBitmap.getHeight();
        /*
         * Bugfix: 1030 1586 BUG CAUSE: mBitmap maybe immutable, and create a
         * Canvas with immutable bitmap is not allowed.try avoid OOM exception
         * FIX COMMENT: if mBitmap is immutable, we will create a mutable one
         * instead DATE: 2012-05-30 2012-09-14
         */
        Bitmap bitmap = mBitmap;
        if (!bitmap.isMutable()) {
            try {
                bitmap = mBitmap.copy(Config.ARGB_8888, true);
            } catch (OutOfMemoryError e) {
                e.printStackTrace();
                return null;
            }
        }

        Bitmap bmp = null;
        try {
            bmp = mFrameBitmap.copy(Config.ARGB_8888, true);
        } catch (IllegalStateException e) {
            e.printStackTrace();
            return null;
        }
        Bitmap rotatedFrameBitmap = rotateToScreen(bmp, Exif.getOrientation(exif));
        bmp.recycle();

        Canvas canvas = new Canvas(bitmap);
        if (rotatedFrameBitmap != null && !rotatedFrameBitmap.isRecycled()) {
            try {
                canvas.drawBitmap(rotatedFrameBitmap, null, new Rect(0, 0, width, height), new Paint());

                Bitmap thumbBitmap = (exif != null ? exif.getThumbnailBitmap() : null);
                if (thumbBitmap != null) {
                    if (!thumbBitmap.isMutable()) {
                        Bitmap thumbBitmapMutable = thumbBitmap;
                        thumbBitmap = thumbBitmapMutable.copy(Config.ARGB_8888, true);
                        thumbBitmapMutable.recycle();
                    }
                    Canvas thumbCanvas = new Canvas(thumbBitmap);
                    thumbCanvas.drawBitmap(rotatedFrameBitmap, null, new Rect(0, 0,
                            thumbBitmap.getWidth(), thumbBitmap.getHeight()), new Paint());
                    exif.setCompressedThumbnail(thumbBitmap);
                    thumbBitmap.recycle();
                }
            } catch (Throwable e) {
                Log.e(TAG, "Error occured while Scenery is playing!");
            } finally {
                rotatedFrameBitmap.recycle();
            }
        }
        return bitmap;
    }

    @Override
    public void resume() {
        super.resume();
        updateLayout();
        mSkippedFirstOnResume = true;
        loadAssetResource();
    }

    @Override
    public void pause() {
        super.pause();
        updateLayout();
    }

    public void updateLayout() {
        View view = mViewModuleRoot.findViewById(R.id.mode_options_overlay);
        if (view != null) {
            if (mPaused) {
                view.setPadding(0, 0, 0, 0);
            } else {
                int bottom = mActivity.getResources().getDimensionPixelSize(R.dimen.mode_option_overlay_scenery_height);
                view.setPadding(0, 0, 0, bottom);
            }
        }
    }

//    @Override
//    public String getModuleStringIdentifier() {
//        return SPRDSCENERY_MODULE_STRING_ID;
//    }

    @Override
    public void onPreviewVisibilityChanged(int visibility) {
        if (mSceneryMenuLayout == null || mReviewView == null) {
            return;
        }
        if (mActivity.isFilmstripVisible()) {
            mSceneryMenuLayout.setVisibility(View.GONE);
            mReviewView.setVisibility(View.GONE);
        } else {
            mSceneryMenuLayout.setVisibility(View.VISIBLE);
            mReviewView.setVisibility(View.VISIBLE);
        }
    }

    public boolean isCameraMirror() {
        return mActivity.getSettingsManager().getBoolean(
                SettingsManager.SCOPE_GLOBAL, Keys.KEY_FRONT_CAMERA_MIRROR);
    }
    /* @} */

    @Override
    public BasicUI getUcamUI(){
        if (mUI == null)
            mUI = new SprdSceneryUI(mCameraId, this, mActivity, mActivity.getModuleLayoutRoot());

        return mUI;
    }

    @Override
    public void updateBatteryLevel(int level) {
        super.updateBatteryLevel(level, Keys.KEY_FLASH_MODE, ButtonManagerDream.BUTTON_FLASH_DREAM);
    }
}
