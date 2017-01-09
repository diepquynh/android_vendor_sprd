/** Create by Spreadst */

package com.spreadst.lockscreen;

import android.app.ActionBar;
import android.app.Activity;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.provider.Settings;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.TextView;

public class ChooseElsActivity extends Activity implements OnViewChangeListener {

    private static final String TAG = "ChooseElsActivity";
    private int mWhichScreen;
    private ExpandLockscreenInfo[] mElsInfos;

    private final int DEFAULT_GROUP = 0;
    private final int DEFAULT_ORDER = 0;
    private final int ACTION_CANCEL = 0;
    private final int ACTION_OK = 1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.preview);
        Resources res = getResources();
        Intent intent = getIntent();
        mWhichScreen = intent.getIntExtra("position", 0);
        mElsInfos = ElsModel.getInstance().getElsInfos(getResources(), false);
        PreScrollLayout mScrollLayout = (PreScrollLayout) findViewById(R.id.ScrollLayout);
        mScrollLayout.init(this, mWhichScreen);
        mScrollLayout.SetOnViewChangeListener(this);
        if (mElsInfos != null) {
            final LayoutInflater inflater = LayoutInflater.from(this);
            for (ExpandLockscreenInfo elsInfo : mElsInfos) {
                View fl = inflater.inflate(R.layout.enlargement, null);
                ImageView thumbnailView = (ImageView) fl.findViewById(R.id.thumbnailView);
                mScrollLayout.addView(fl);
                String preview_id = elsInfo.getPreview_id();
                thumbnailView.setImageResource(res.getIdentifier(
                        Constants.CURRENT_PACKAGE_NAME + ":drawable/"
                                + preview_id + "_look_sprd", null, null));

                thumbnailView.setScaleType(ScaleType.FIT_XY);
            }
        }

        Drawable backgroundDrawable = res.getDrawable(res
                .getIdentifier(
                        Constants.CURRENT_PACKAGE_NAME
                                + ":drawable/ab_solid_custom_blue_inverse_holo_sprd",
                        null, null));
        ActionBar actionBar = getActionBar();
        /*
         * SPRD:
         *     Bug248152
         *     remove the logo on actionbar.
         *
         * @orig
         * actionBar.setDisplayHomeAsUpEnabled(true);;
         *
         * @{
         */
        actionBar.setDisplayOptions(ActionBar.DISPLAY_SHOW_TITLE
                | ActionBar.DISPLAY_HOME_AS_UP);
        /*
         * @}
         */
        actionBar.setBackgroundDrawable(backgroundDrawable);
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);

        menu.clear();
        menu.add(DEFAULT_GROUP, ACTION_CANCEL, DEFAULT_ORDER, null)
                .setTitle(R.string.lockscreen_cancel)
                .setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS);
        menu.add(DEFAULT_GROUP, ACTION_OK, DEFAULT_ORDER, null).
                setTitle(R.string.lockscreen_ok)
                .setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS);

        return true;
    }

    @Override
    public boolean onMenuItemSelected(int featureId, MenuItem item) {

        switch (item.getItemId()) {
            case android.R.id.home:
            case ACTION_CANCEL:
                finish();
                break;

            case ACTION_OK: {
                int elsId = mElsInfos[mWhichScreen].getId();

                Settings.System.putInt(getContentResolver(),
                        Constants.SYSTEM_SETTINGS_LOCKSTYLE, elsId);
                Intent intent = new Intent();
                intent.putExtra("elsId", elsId);
                ChooseElsActivity.this.setResult(RESULT_OK, intent);
                ChooseElsActivity.this.finish();
            }
                break;
        }

        return super.onMenuItemSelected(featureId, item);
    }

    @Override
    public void OnViewChange(int view) {
        mWhichScreen = view;
    }

}
