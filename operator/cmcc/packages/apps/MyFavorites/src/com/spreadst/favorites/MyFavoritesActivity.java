
package com.spreadst.favorites;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

import android.app.ListActivity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.Toast;

public class MyFavoritesActivity extends ListActivity {
    private static final String TAG = "MyFavoritesActivity";
    private static final int MUSIC_ITEM = 0;
    private static final int VIDEO_ITEM = 1;
    private static final int GALLARY_ITEM = 2;
    private ArrayList<Map<String, ?>> mList;
    private static final String KEY_TITLE = "TITLE";
    private static final String KEY_ICON = "ICON";
    private static final String sKeys[] = new String[] {
            KEY_TITLE, KEY_ICON
    };
    private static final int sResourceIds[] = new int[] {
            R.id.title, R.id.icon
    };
    private int[] titleId = {
            R.string.app_music, R.string.app_vedio,
            R.string.app_gallary
    };
    private int[] iconId = {
            R.drawable.ic_audio, R.drawable.ic_video,
            R.drawable.ic_pictures
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mList = new ArrayList<Map<String, ?>>();
        fillAdapterList(mList);
        getListView().setAdapter(
                new SimpleAdapter(this, mList, R.layout.list_item, sKeys,
                        sResourceIds));
    }

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        if (position >= mList.size()) {
            return;
        }
        switch (position) {
            case MUSIC_ITEM:
                startMusicIntent();
                break;
            case VIDEO_ITEM:
                startVideoIntent();
                break;
            case GALLARY_ITEM:
                startGallaryIntent();
                break;
            default:
                break;
        }
    }

    private void startMusicIntent() {
        /* SPRD: Modify 20140922 Spreadst of bug349992 , Click the back button repeatedly to exit failure */
       /* Intent intent = new Intent("android.intent.action.PICK");
        intent.setDataAndType(Uri.EMPTY, "vnd.android.cursor.dir/track");
        intent.putExtra("withtabs", true);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);*/
        Intent intent = new Intent();
        intent.setClassName("com.android.music",
                "com.android.music.MusicBrowserActivity");
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        /* @} */
        try {
            startActivity(intent);
        } catch (Exception e) {
            Log.e(TAG, "can not to start Music :" + e);
        }
    }

    private void startVideoIntent() {
        Intent intent = new Intent();
        intent.setClassName("com.android.gallery3d",
                "com.android.gallery3d.app.VideoActivity");
        try {
            startActivity(intent);
        } catch (ActivityNotFoundException e) {
            Log.d(TAG,
                    "class not find com.android.gallery3d.app.VideoActivity");
            try {
                intent.setClassName("com.android.gallery3d",
                        "com.sprd.gallery3d.app.VideoActivity");
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                startActivity(intent);
            } catch (ActivityNotFoundException anf) {
                Log.e(TAG, "class not find com.sprd.gallery3d.app.VideoActivity. ");
            }
        }
    }

    private void startGallaryIntent() {
        Intent intent = new Intent();
        intent.setClassName("com.android.gallery3d",
                "com.android.gallery3d.app.Gallery");
	intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        try {
            startActivity(intent);
        } catch (Exception e) {
            Log.e(TAG, "can not to start Gallary :" + e);
        }
    }

    private void fillAdapterList(List<Map<String, ?>> list) {
        list.clear();
        for (int i = 0; i < titleId.length; i++) {
            Map<String, Object> map = new TreeMap<String, Object>();
            map.put(KEY_TITLE, getResources().getString(titleId[i]));
            map.put(KEY_ICON, iconId[i]);
            list.add(map);
        }
    }
}
