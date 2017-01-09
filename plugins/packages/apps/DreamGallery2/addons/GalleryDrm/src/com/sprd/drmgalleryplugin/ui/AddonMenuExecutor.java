package com.sprd.drmgalleryplugin.ui;

import java.util.ArrayList;
import java.util.Map.Entry;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;

import com.android.gallery3d.app.AlbumPage;
import com.android.gallery3d.app.PhotoPage;
import com.android.gallery3d.app.GalleryAppImpl;
import com.android.gallery3d.data.MediaDetails;
import com.android.gallery3d.data.MediaObject;
import com.android.gallery3d.ui.DetailsHelper;

import com.sprd.drmgalleryplugin.R;
import com.sprd.drmgalleryplugin.data.AddonLocalMediaItem;
import com.sprd.drmgalleryplugin.util.DrmUtil;
import com.sprd.gallery3d.drm.MenuExecutorUtils;

public class AddonMenuExecutor extends MenuExecutorUtils implements AddonManager.InitialCallback {
    private Context mAddonContext;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void createDrmMenuItem(Menu menu) {
        MenuItem menuItem = menu.add(Menu.NONE, R.string.action_drm_info, 0
                , mAddonContext.getString(R.string.drm_info));
        menuItem.setVisible(false);
        menuItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_NEVER);
    }

    @Override
    public void updateDrmMenuOperation(Menu menu, int supported) {
        boolean suppoertDrmInfo = ((supported & AddonLocalMediaItem.SUPPORT_DRM_RIGHTS_INFO) != 0);
        MenuItem item = menu.findItem(R.string.action_drm_info);
        if (item != null) item.setVisible(suppoertDrmInfo);
    }
    
    @Override
    public boolean showHideDrmDetails(AlbumPage page, int itemId) {
        if(R.string.action_drm_info == itemId){
            page.showDrmDetails();
            return true;
        }
        return false;
    }

    @Override
    public boolean showHideDrmDetails(PhotoPage page, int itemId, int index) {
        if (R.string.action_drm_info == itemId) {
            page.showDrmDetails(index);
            return true;
        }
        return false;
    }

    @Override
    public boolean setDrmDetails(Context context, MediaDetails details
            , ArrayList<String> items, boolean isDrmDetails){
        if(!isDrmDetails) return false;
        for (Entry<Integer, Object> detail : details) {
            String value;
            Object valueObj = detail.getValue();

            if (valueObj == null) {
                valueObj = " ";
            }

            value = valueObj.toString();
            int key = detail.getKey();

            if (key != MediaDetails.INDEX_FILENAME
                    && key != MediaDetails.INDEX_REMAIN_TIMES
                    && key != MediaDetails.INDEX_RIGHTS_STARTTIME
                    && key != MediaDetails.INDEX_RIGHTS_ENDTIME
                    && key != MediaDetails.INDEX_RIGHTS_VALIDITY
                    && key != MediaDetails.INDEX_RIGHTS_STATUS
                    && key != MediaDetails.INDEX_EXPIRATION_TIME) {
                continue;
             }

            if (details.hasUnit(key)
                    && !value.equalsIgnoreCase(" ")
                    && !value.equals(mAddonContext.getString(R.string.drm_rights_no_limit))
                    && !value.equals(mAddonContext.getString(R.string.drm_rights_unknown))) {
                value = String.format("%s : %s %s", DetailsHelper.getDetailsName(
                        mAddonContext, key), value, mAddonContext.getString(details.getUnit(key)));
            } else {
                value = String.format("%s : %s", DetailsHelper.getDetailsName(
                        mAddonContext, key), value);
            }
            items.add(value);
        }
        return true;
    }

    @Override
    public String getDetailsNameForDrm(Context context, int key){
        String detailsName = "Unknown key" + key;
        switch (key) {
            case MediaDetails.INDEX_FILENAME:
                return context.getString(R.string.file_name);
            case MediaDetails.INDEX_RIGHTS_VALIDITY:
                return context.getString(R.string.rights_validity);
            case MediaDetails.INDEX_RIGHTS_STATUS:
                return context.getString(R.string.rights_status);
            case MediaDetails.INDEX_RIGHTS_STARTTIME:
                return context.getString(R.string.start_time);
            case MediaDetails.INDEX_RIGHTS_ENDTIME:
                return context.getString(R.string.end_time);
            case MediaDetails.INDEX_EXPIRATION_TIME:
                return context.getString(R.string.expiration_time);
            case MediaDetails.INDEX_REMAIN_TIMES:
                return context.getString(R.string.remain_times);
            default:
                break;
        }
        return detailsName;
    }

    @Override
    public boolean keyMatchDrm(int key) {
        if (key == MediaDetails.INDEX_FILENAME
                || key == MediaDetails.INDEX_REMAIN_TIMES
                || key == MediaDetails.INDEX_RIGHTS_STARTTIME
                || key == MediaDetails.INDEX_RIGHTS_ENDTIME
                || key == MediaDetails.INDEX_RIGHTS_VALIDITY
                || key == MediaDetails.INDEX_RIGHTS_STATUS
                || key == MediaDetails.INDEX_EXPIRATION_TIME) {
            return true;
        }
        return false;
    }
}
