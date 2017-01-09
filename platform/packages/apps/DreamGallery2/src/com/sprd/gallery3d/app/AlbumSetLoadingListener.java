/** Created by Spreadst */

package com.sprd.gallery3d.app;

import com.android.gallery3d.app.LoadingListener;

public interface AlbumSetLoadingListener extends LoadingListener {

    /**
     * Call when find some data need loading. Just for AlbumSetPage to update
     * CameraButton status.
     */
    public void onLoadingWill();
}
