/** Created by Spreadst */
package com.android.sprdlauncher2;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Toast;
import com.android.sprdlauncher2.R;
import com.android.sprdlauncher2.Workspace;

public class PreviewsAddItem extends PreviewsBaseItem {
    //private ImageView mAddPreviewImageView;


    public PreviewsAddItem(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public PreviewsAddItem(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public PreviewsAddItem(Context context) {
        super(context);
    }

    protected void initPreviewsItem(int index) {
        mIndex = index;
        this.setOnClickListener(this);
        /*mAddPreviewImageView = (ImageView) findViewById(R.id.preview_add_image);
        mAddPreviewImageView.setOnClickListener(this);
        mAddPreviewImageView.setOnLongClickListener(new OnLongClickListener() {
            public boolean onLongClick(View v) {
                return performLongClick();
            }
        });*/

    }

    public void onClick(View v) {
        if(mPreviews.getChildCount() >= SprdWorkspace.MAX_PAGE + 1){
            Toast.makeText(mContext, R.string.already_max_page, Toast.LENGTH_SHORT).show();
            return;
        }
        mPreviews.addChild();
    }

    protected void recycleThumbnailBitmap() {

    }
}
