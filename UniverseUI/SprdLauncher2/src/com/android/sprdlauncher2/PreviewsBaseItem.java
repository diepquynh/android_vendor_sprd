/** Created by Spreadst */
package com.android.sprdlauncher2;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.FrameLayout;
import com.android.sprdlauncher2.Launcher;
import com.android.sprdlauncher2.Workspace;
import com.android.sprdlauncher2.R;
public abstract class PreviewsBaseItem extends FrameLayout implements
        OnClickListener {
    int mId;
    int mWidth;
    int mHeight;
    Launcher mLauncher;
    SprdWorkspace mWorkspace;
    Previews mPreviews;
    Context mContext;
    int mIndex;
    boolean mIsHomeScreen = false;
    boolean mIsCurrentScreen = false;

    public PreviewsBaseItem(Context context) {
        super(context);
        this.mContext = context;
        init();
    }

    public PreviewsBaseItem(Context context, AttributeSet attrs) {
        super(context, attrs);
        this.mContext = context;
        init();
    }

    public PreviewsBaseItem(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        this.mContext = context;
        init();
    }
    private void init(){
        mWidth = (int) getResources().getDimension(R.dimen.preview_thumbnail_width);
        mHeight = (int) getResources().getDimension(R.dimen.preview_view_height);
    }

    public void onClick(View v) {

    }

    protected void recycleThumbnailBitmap() {

    }

    protected void initPreviewsItem(int index) {
    }

    public void setLauncher(Launcher Launcher) {
        this.mLauncher = Launcher;
    }

    public void setPreviews(Previews Previews) {
        this.mPreviews = Previews;
    }

    public void setWorkspace(SprdWorkspace workspace) {
        this.mWorkspace = workspace;
    }
}
