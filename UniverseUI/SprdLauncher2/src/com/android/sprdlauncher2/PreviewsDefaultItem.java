/** Created by Spreadst */
package com.android.sprdlauncher2;

import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PaintFlagsDrawFilter;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import com.android.sprdlauncher2.CellLayout;
import com.android.sprdlauncher2.Workspace;
import com.android.sprdlauncher2.R;

public class PreviewsDefaultItem extends PreviewsBaseItem {
    private ImageView mThumbnailImageView;
    private ImageView mHomeImageView;
    private ImageView mCloseImageView;
    Bitmap mThumbnail;
    private  AlertDialog mAlertDialog;

    public PreviewsDefaultItem(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

    }

    public PreviewsDefaultItem(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public PreviewsDefaultItem(Context context) {
        super(context);
    }

    /**
     * To initialize the one preview of workspace
     * @param index
     *            the index of preview
     * @return
     */
    protected void initPreviewsItem(int index) {
        mIndex = index;
        if (mId == mWorkspace.getCurrentScreen()) {
            setCurrentScreenColor();
            requestFocus();
        }
        initThumbnailImageView();
        initHomeImageView();
        initCloseImageView();
    }
    private boolean isEmptyPreview(){
        //users can delete any workspace.cell even if the workspace.cell contains shortcuts
        //or widgets
        //Workspace-->CellLayout-->ShortcutAndWidgetContainer
        ShortcutAndWidgetContainer cell = (ShortcutAndWidgetContainer)((CellLayout) mWorkspace.getChildAt(mId)).getChildAt(0);
        if (cell != null && cell.getChildCount() == 0) {
            return true;
        }
        return false;
//          return true;
    }
    private void initThumbnailImageView() {
        mThumbnailImageView = (ImageView) findViewById(R.id.preview_image);
        initThumbnail(mWorkspace);
        mThumbnailImageView.setImageBitmap(mThumbnail);
        mThumbnailImageView.setTag(mIndex);
        mThumbnailImageView.setOnClickListener(this);
        mThumbnailImageView.setOnLongClickListener(new OnLongClickListener() {
            public boolean onLongClick(View v) {
                return performLongClick();
            }
        });
    }

    private void initHomeImageView() {
        mHomeImageView = (ImageView) findViewById(R.id.home_image);
        mHomeImageView.setOnClickListener(this);
        if (mId ==this.mWorkspace.mDefaultPage) {
            setDefaultScreenColor();
        }
    }

    //change the method of delete Workspace.cell,disable the close_image
    private void initCloseImageView() {
      mCloseImageView = (ImageView) findViewById(R.id.close_image);
      mCloseImageView.setOnClickListener(this);
      //
      /* SPRD: Fix bug262687,only show CloseView while screen count big than Workspace.MIN_PAGE. @{ */
      if (mWorkspace.getChildCount() > SprdWorkspace.MIN_PAGE && isEmptyPreview()) {
      /* @} */
          mCloseImageView.setVisibility(View.VISIBLE);
      } else {
          mCloseImageView.setVisibility(View.GONE);
      }
  }
    protected void showCloseView(boolean visible) {
        /* SPRD: Fix bug262687,only show CloseView while screen count big than Workspace.MIN_PAGE. @{ */
        if (visible && isEmptyPreview() && mWorkspace.getChildCount() > SprdWorkspace.MIN_PAGE) {
        /* @} */
            mCloseImageView.setVisibility(View.VISIBLE);
        } else {
            mCloseImageView.setVisibility(View.GONE);
        }
    }

    protected void showHomeView(boolean visible) {
        mCloseImageView.setVisibility(View.VISIBLE);
    }

    /**
     * The cellLayout zoom into an image.
     * @param index
     *            the screen of workspace
     * @param workspace
     */
    private void initThumbnail(Workspace workspace) {
        CellLayout cell = ((CellLayout) workspace.getChildAt(mId));
        int sWidth = mWidth;
        int width = mPreviews.mWindowWidth;
        int height = workspace.getHeight();
        /* SPRD: fix bug324186, monkey test NullPointerException. @{ */
        if (width > 0 && height > 0 && cell != null) {
            float scale = sWidth / (float) width;
            mThumbnail = Bitmap.createBitmap((int) sWidth,
                    (int) (height * scale), Bitmap.Config.ARGB_8888);
            final Canvas c = new Canvas(mThumbnail);
            c.scale(scale, scale);
            c.setDrawFilter(new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG
                    | Paint.FILTER_BITMAP_FLAG));
            cell.dispatchDraw(c);
        } else {
            mThumbnail = null;
            Log.w("Launcher.PreviewsDefaultItem", "cell="+ cell + " workspace.ChildCount="
                    + workspace.getChildCount() + " mId=" + mId + " mIndex="+ mIndex);
        }
        /* @} */
    }

    protected void recycleThumbnailBitmap() {
        if (mThumbnail != null && !mThumbnail.isRecycled()) {

            /* SPRD: bug321325 2014-06-10 reset ImageView's inner bitmap to null. @{ */
            if (mThumbnailImageView != null) {
                mThumbnailImageView.setImageBitmap(null);
            }
            /* SPRD: bug321325 2014-06-10 reset ImageView's inner bitmap to null. @} */

            /* SPRD: bug321331 2014-06-10 don't hold memory after previews disappear. @{ */
            // here we use post to recycle the bitmap, because the bitmap currently may
            // still be used by native level such as openGL render cache and this cause
            // nothing happen after recycled() called. This is not a big problem if we don't
            // use post method here. Because GC will always recycle the bitmap, but if we
            // use post, the bitmap can recycled more efficiently.
            if (mPreviews != null) {
                mPreviews.post(new Runnable() {
                    @Override
                    public void run() {
                        mThumbnail.recycle();
                    }
                });
            }
            /* SPRD: bug321331 2014-06-10 don't hold memory after previews disappear. @} */
        }
    }

    public void onClick(View v) {
        if(!mPreviews.mClickable){
            return;
        }
        if (v == mThumbnailImageView) {
            mPreviews.dismissPreview(mIndex, this);
        } else if (v == mHomeImageView) {
            int count = mPreviews.getChildCount();
            for (int i = 0; i < count; i++) {
                PreviewsBaseItem child = (PreviewsBaseItem) (mPreviews
                        .getChildAt(i));
                if (child.mIsHomeScreen) {
                    ((PreviewsDefaultItem) child).cancelDefaultScreenColor();
                }
            }
            setDefaultScreenColor();
            mWorkspace.updateDefaultScreenNum(mIndex);
        }else if (v == mCloseImageView) {
            ShortcutAndWidgetContainer cell = (ShortcutAndWidgetContainer)((CellLayout) mWorkspace.getChildAt(mId)).getChildAt(0);
            if(cell.getChildCount() == 0){

                mPreviews.deleteChild(this);

            }else{
                AlertDialog.Builder builder = new Builder(mContext);
                builder.setMessage(mContext.getString(R.string.delete_preview_tip));
                builder.setTitle(mContext.getString(R.string.delete_preview_alter));
                builder.setPositiveButton(mContext.getString(R.string.delete_preview_action), new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        mPreviews.deleteChild(PreviewsDefaultItem.this);
                        dialog.dismiss();
                    }
                });
                builder.setNegativeButton(mContext.getString(R.string.cancel_action), new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                });
                builder.create().show();
            }

        }
    }

    protected void setDefaultScreenColor() {
        mIsHomeScreen = true;
//        mHomeImageView.setBackgroundDrawable(getResources().getDrawable(
//                R.drawable.preview_default_home));
        mHomeImageView.setBackgroundDrawable(getResources().getDrawable(
                R.drawable.preview_default_home));
        /* SPRD: Fix bug 271677 @{ */
        if (mIsCurrentScreen) {
            setBackgroundDrawable(getResources().getDrawable(
                    R.drawable.preview_focused_default_home_bg));
        } else {
            setBackgroundDrawable(getResources().getDrawable(
                    R.drawable.preview_default_home_bg));
        }
        /* @} */
    }

    protected void cancelDefaultScreenColor() {
        mIsHomeScreen = false;
        mHomeImageView.setBackgroundDrawable(getResources().getDrawable(
                R.drawable.preview_normal_home));
        if (mIsCurrentScreen) {
            this.setBackgroundDrawable(getResources().getDrawable(
                    R.drawable.preview_current_bg));
        } else {
            setBackgroundDrawable(getResources().getDrawable(
                    R.drawable.preview_normal_bg));
        }
    }

    protected void setCurrentScreenColor() {
        mIsCurrentScreen = true;
        this.setBackgroundDrawable(getResources().getDrawable(
                R.drawable.preview_current_bg));
    }

    public ImageView getThumbnailImageView() {
        return mThumbnailImageView;
    }

    public ImageView getHomeImageView() {
        return mHomeImageView;
    }
/*
    public ImageView getCloseImageView() {
        return mCloseImageView;
    }
*/
    protected boolean isAlertDialogShowing(){
        if(mAlertDialog != null && mAlertDialog.isShowing()){
            return true;
        }
        return false;
    }
    protected void closeAlertDialog(){
        if(mAlertDialog != null && mAlertDialog.isShowing()){
            mAlertDialog.cancel();
        }
    }
}
