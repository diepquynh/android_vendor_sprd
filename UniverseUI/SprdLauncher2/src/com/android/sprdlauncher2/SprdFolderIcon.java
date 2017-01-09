package com.android.sprdlauncher2;

import java.util.ArrayList;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;
import android.widget.TextView;

/**
 * SPRD: code separation 289371 @{
 *
 */
public class SprdFolderIcon extends FolderIcon {

    /* SPRD: UUI : upgrade folder icon logic @{ */
    int row1Y;
    int row2Y;
    int colum1X;
    int colum2X;
    int iconSize;
    static final float iconScaleFactor = 0.3f;
    static final int iconMargin = 9;
    TextView v;
    Drawable d;
    /* @} */

    public SprdFolderIcon(Context context) {
        super(context);
    }

    public SprdFolderIcon(Context context, AttributeSet attrs){
        super(context, attrs);
    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
        super.dispatchDraw(canvas);
        /* SPRD: for UUI : folderIcon @{ */
        drawIconUI(canvas);
        /* @} */
    }

    /* SPRD: for UUI : folderIcon @{ */
    private void drawIconUI(Canvas canvas){
        Folder folder = getFolder();
        if (folder == null) return;
        if (folder.getItemCount() == 0 && !mAnimating) return;
        ArrayList<View> items = getFolder().getItemsInReadingOrder();
        computeInstrinctIconPosition(items);
        if(items.size()>=1){
                 drawPreviewItem(canvas,colum1X,row1Y,0,items);
        }
        if(items.size()>=2){
               drawPreviewItem(canvas,colum2X,row1Y,1,items);
        }
        if(items.size()>=3){
           drawPreviewItem(canvas,colum1X,row2Y,2,items);
        }
        if(items.size()>=4){
                drawPreviewItem(canvas,colum2X,row2Y,3,items);
        }
   }

    private void drawPreviewItem(Canvas canvas,float x,float y,int index,ArrayList<View> items){
        Drawable d;
        TextView v;
        canvas.save();
        canvas.scale(iconScaleFactor,iconScaleFactor);
        canvas.translate(x,y);
        v = (TextView) items.get(index);
        d = v.getCompoundDrawables()[1];
        if (d != null) {
            int right = mIntrinsicIconSize;
            int bottom = right;
            d.setBounds(0, 0, right, bottom);
            d.setFilterBitmap(true);
            d.setColorFilter(Color.argb(0, 0, 0, 0), PorterDuff.Mode.SRC_ATOP);
            d.draw(canvas);
            d.clearColorFilter();
            d.setFilterBitmap(false);
        }
        canvas.restore();
    }
    /* @}*/

    /* SPRD: UUI : upgrade folder icon logic @{ */
    private void computeInstrinctIconPosition(ArrayList<View> items) {
        v = (TextView) items.get(0);
        d = v.getCompoundDrawables()[1];
        computePreviewDrawingParams(d);
        int left = mPreviewBackground.getLeft();
        int top = mPreviewBackground.getTop();
        int width = mPreviewBackground.getWidth();
        int height = mPreviewBackground.getWidth();
        int iconSize = (int) (mIntrinsicIconSize * iconScaleFactor);
        row1Y = top + (height - iconSize * 2 - iconMargin) / 2;
        row2Y = row1Y + iconSize + iconMargin;
        colum1X = left + (width - iconSize * 2 - iconMargin) / 2;
        colum2X = colum1X + iconSize + iconMargin;
        row1Y = (int) (row1Y / iconScaleFactor);
        row2Y = (int) (row2Y / iconScaleFactor);
        colum1X = (int) (colum1X / iconScaleFactor);
        colum2X = (int) (colum2X / iconScaleFactor);
    }
    /* @} */
}
/** @} */