package plugin.sprd.drmxposed;

import android.app.Activity;
import android.app.AddonManager;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.view.ContextMenu;
import android.provider.MediaStore;
import android.content.ContentResolver;
import android.net.Uri;
import android.util.Log;

import plugin.sprd.drmxposed.R;
import com.android.music.MusicUtils;
import com.android.music.TrackBrowserActivity;
import com.android.music.TrackBrowserActivity.NowPlayingCursor;
import com.android.music.TrackBrowserActivity.TrackListAdapter.ViewHolder;
import com.sprd.music.drm.*;

public class AddonMusicDRM extends MusicDRM implements AddonManager.InitialCallback{
    private static final String LOGTAG = "DRM";
    public static MusicDRMUtils mDrmUtils = null;
    private String mCurrentTrackData;
    public final static int PROTECT_MENU = 26;
    public final static int USE_AS_RINGTONE = 2;
    String mCurrentOptionItem = null;

    private Context mAddonContext;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void initDRM(Context context) {
        mDrmUtils = new MusicDRMUtils(context);
        Log.i(LOGTAG, "initDRM");
    }

    @Override
    public void destroyDRM() {
        if (mDrmUtils != null) {
            mDrmUtils.dismissDRMDialog();
            Log.i(LOGTAG, "destroyDRM");
        }
    }

    @Override
    public void onCreateDRMTrackBrowserContextMenu(ContextMenu menu,Cursor mTrackCursor ) {
        mCurrentTrackData = mTrackCursor.getString(mTrackCursor.getColumnIndexOrThrow(
                MediaStore.Audio.Media.DATA));
        Log.i(LOGTAG, "onCreateDRMTrackBrowserContextMenu mCurrentTrackData = " + mCurrentTrackData);
        Log.i(LOGTAG, "destroyDRMTrackBrowser " + mDrmUtils.isDRMFile(mCurrentTrackData));
        if (mDrmUtils.isDRMFile(mCurrentTrackData)) {
            menu.add(0, PROTECT_MENU, 0, mAddonContext.getString(R.string.protect_information)).setIcon(
                    mAddonContext.getResources().getDrawable(R.drawable.ic_menu_play_clip));
            menu.removeItem(USE_AS_RINGTONE);
        }
    }

    @Override
    public void onContextDRMTrackBrowserItemSelected(MenuItem item,Context context) {
        if (item.getItemId() == PROTECT_MENU) {
            mDrmUtils.showProtectInfo(mAddonContext,context, mCurrentTrackData);
        }
    }

    @Override
    public void onListItemClickDRM(Context context, Cursor cursor, int position) {
        if (isDRM(cursor, position)) {
        Log.i(LOGTAG, "onListItemClickDRM");
            if (mDrmUtils.isDrmValid(mCurrentTrackData)) {
                String drmFileType = mDrmUtils.getDrmFileType(mCurrentTrackData);
                Log.i(LOGTAG, "drmFileType = " +drmFileType);
                if (!mDrmUtils.FL_DRM_FILE.equals(drmFileType)) {
                    mDrmUtils.showConfirmDialog(context,mAddonContext, cursor, position,
                            cursor instanceof NowPlayingCursor);
                    return;
                }
                MusicUtils.playAll(context, cursor, position);
                // SPRD 530259 finish activity that start from search
                mDrmUtils.tryFinishActivityFromSearch(context);
                return;
            }
            Intent intent = mDrmUtils.getDownloadIntent(mCurrentTrackData);
            context.startActivity(intent);

            // SPRD 530259 finish activity that start from search
            mDrmUtils.tryFinishActivityFromSearch(context);
        }
    }

    @Override
    public boolean isDRM(Cursor cursor, int position) {
        if (cursor.moveToPosition(position)) {
            mCurrentTrackData = cursor.getString(cursor.getColumnIndexOrThrow(
                    MediaStore.Audio.Media.DATA));
        } else {
            mCurrentTrackData = null;
        }
        boolean isDRM = mDrmUtils.isDRMFile(mCurrentTrackData);
        Log.i(LOGTAG,"mCurrentTrackData = " +mCurrentTrackData);
        Log.i(LOGTAG, "isDRM = " + isDRM);
        return isDRM;
    }

    @Override
    public ViewHolder bindViewDrm(Cursor cursor,int mDataIdx,ViewHolder vh ) {
        String filePath = cursor.getString(mDataIdx);
        boolean isDRMFile = mDrmUtils.isDRMFile(filePath);
        if (isDRMFile) {
            Log.d("TrackBrowser", "this is drm file, path=" + filePath);
            boolean isDrmValid = mDrmUtils.isDrmValid(filePath);
            vh.drmIcon.setVisibility(View.VISIBLE);
            if (isDrmValid) {
                vh.drmIcon.setImageDrawable(mAddonContext.getResources().getDrawable(R.drawable.drm_icon_unlock));
            } else {
                vh.drmIcon.setImageDrawable(mAddonContext.getResources().getDrawable(R.drawable.drm_icon_lock));
            }
        }else {
            vh.drmIcon.setVisibility(View.GONE);
        }
        return vh;
   }

    @Override
    public void onListDRMQueryBrowserItemClick(Context context,long[] list,Cursor cursor) {
        String audioData = "";
        try {
            audioData = cursor.getString(cursor
                    .getColumnIndexOrThrow(MediaStore.Audio.Media.DATA));
        } catch (Exception e) {
            Log.d(LOGTAG, "get filepath fail from query cursor");
        }
        Log.i(LOGTAG,"onListDRMQueryBrowserItemClick = "+audioData);
        if (mDrmUtils.isDRMFile(audioData)) {
            if(mDrmUtils.isDrmValid(audioData)) {
                if(!mDrmUtils.FL_DRM_FILE.equals(mDrmUtils.getDrmFileType(audioData))){
                    mDrmUtils.showConfirmDialog(mAddonContext,context, list, 0);
                    return;
                }
                MusicUtils.playAll(context, list, 0);
                return;
            }
            Intent intent = mDrmUtils.getDownloadIntent(audioData);
            context.startActivity(intent);
        } 
    }

    @Override
    public void playDRMQueryBrowser(Context context, long[] list, Uri uri) {
        String filePath = "";
        Cursor cursor = context.getContentResolver().query(uri, new String[] {
                "_data"
        }, null, null, null);
        try {
            if (cursor != null && cursor.moveToFirst() && cursor.getCount() == 1) {
                filePath = cursor.getString(cursor
                        .getColumnIndex(MediaStore.MediaColumns.DATA));
            }
        } catch (Exception e) {

        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        boolean isDRM = mDrmUtils.isDRMFile(filePath);
        if (isDRM) {
            if (mDrmUtils.isDrmValid(filePath)) {
                String drmFileType = mDrmUtils.getDrmFileType(filePath);
                if (!mDrmUtils.FL_DRM_FILE.equals(drmFileType)) {
                    mDrmUtils.showConfirmFromQuery(mAddonContext,context, list);
                } else {
                    MusicUtils.playAll(context, list, 0);
                    //context.finish();
                }
                return;
            }
            Intent downloadIntent = mDrmUtils.getDownloadIntent(filePath);
            context.startActivity(downloadIntent);
        }
    }

    @Override
    public boolean isDRM(Uri uri,Context context) {
        String filePath = "";
        Cursor cursor = context.getContentResolver().query(uri, new String[] {
                "_data"
        }, null, null, null);
        try {
            if (cursor != null && cursor.moveToFirst() && cursor.getCount() == 1) {
                filePath = cursor.getString(cursor
                        .getColumnIndex(MediaStore.MediaColumns.DATA));
            }
        } catch (Exception e) {

        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        Log.i(LOGTAG,"filePath = "+filePath);
        boolean isDRM = mDrmUtils.isDRMFile(filePath); 
        Log.i(LOGTAG,"isDRM =" +isDRM);
        return isDRM;
    }

    @Override
    public void onPrepareDRMMediaplaybackOptionsMenu(Menu menu) {
        Log.i(LOGTAG, "mDrmUtils.getCurrentAudioIsDRM() =" + mDrmUtils.getCurrentAudioIsDRM());
        mCurrentOptionItem = mDrmUtils.getCurrentAudioData();
        menu.add(0, PROTECT_MENU, 0, mAddonContext.getString(R.string.protect_information))
                .setIcon(
                        mAddonContext.getResources().getDrawable(R.drawable.ic_menu_play_clip));
        menu.removeItem(USE_AS_RINGTONE);
    }

    @Override
    public void onDRMMediaplaybackOptionsMenuSelected(Context context,MenuItem item) {
        if (item.getItemId() == PROTECT_MENU) {
            mCurrentOptionItem = mDrmUtils.getCurrentAudioData();
            Log.i(LOGTAG,"onContextDRMMediaplaybackItemSelected mCurrentOptionItem = " +mCurrentOptionItem);
            mDrmUtils.showProtectInfo(mAddonContext,context, mCurrentOptionItem);
        }
    }

    @Override
    public boolean openIsInvaildDrm(Cursor mCursor) {
        if (mDrmUtils!=null && mCursor != null && !mCursor.isClosed()) {
            String curData = mCursor.getString(mCursor
                    .getColumnIndex(MediaStore.Audio.Media.DATA));
            Log.i(LOGTAG,"open curData =" + curData);
            if (1 == mCursor.getInt(mCursor.getColumnIndex(MusicDRMUtils.DRMCols))) {
                if (!mDrmUtils.isDrmValid(curData)) {
                    return true;
                }
                return false;
            }
            return false;
        }
        return false;
    }

    @Override
    public String getAudioData(Cursor mCursor) {
        synchronized (this) {
            try {
                if (mCursor == null || mCursor.isClosed() || mCursor.getCount() == 0) {
                    Log.w(LOGTAG, "getAudioData is returned null");
                    return null;
                }
                return mCursor.getString(mCursor
                        .getColumnIndexOrThrow(MediaStore.Audio.Media.DATA));
            } catch (android.database.StaleDataException e) {
                return null;
            }
        }
    }

    @Override
    public boolean getAudioIsDRM(Cursor mCursor) {
        synchronized (this) {
            try {
                if (mCursor == null || mCursor.isClosed() || mCursor.getCount() == 0) {
                    Log.w(LOGTAG, "getAudioIsDRM is returned false");
                    return false;
                }
                if (mCursor.getInt(mCursor
                        .getColumnIndexOrThrow(MusicDRMUtils.DRMCols)) == 1) {
                    return true;
                } else {
                    return false;
                }
            } catch (android.database.StaleDataException e) {
                return false;
            }
        }
    }

    @Override
    public boolean isDRM(Cursor mCursor) {
        synchronized (this) {
        if (mCursor != null && !mCursor.isClosed() && mCursor.moveToFirst()) {
            if (1 == mCursor.getInt(mCursor.getColumnIndex(MusicDRMUtils.DRMCols))) {
                return true;
            }
        }
        return false;
        }
    }

    @Override
    public boolean isDRM() {
        return mDrmUtils.getCurrentAudioIsDRM();
    }
}
