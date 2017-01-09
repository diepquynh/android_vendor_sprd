/** Create by Spreadst */

package com.spreadst.lockscreen;

import android.app.ActionBar;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.GridView;

public class LockscreenActivity extends Activity implements
        View.OnClickListener {

    /** Called when the activity is first created. */
    private String TAG = "LockscreenActivity";
    private static final int ELS_REQUESTCODE = 101;
    private ElsInfoAdapter mElsInfoAdapter;

    Button mTestButton;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        mTestButton = (Button) findViewById(R.id.testbutton);
        GridView gridview = (GridView) findViewById(R.id.gridview);
        Resources res = getResources();
        ExpandLockscreenInfo[] elsinfos = ElsModel.getInstance().getElsInfos(
                res, false);
        int lockStyleId = Tools.getLockViewID(this);
        mElsInfoAdapter = new ElsInfoAdapter(this, elsinfos, lockStyleId);
        gridview.setAdapter(mElsInfoAdapter);
        mTestButton.setOnClickListener(this);
        final Intent intent = new Intent();
        gridview.setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View v,
                    int position, long id) {
                int elsId = ((ExpandLockscreenInfo) mElsInfoAdapter
                        .getItem(position)).getId();
                intent.putExtra("elsId", elsId);
                intent.putExtra("position", position);
                intent.setClass(LockscreenActivity.this,
                        ChooseElsActivity.class);
                startActivityForResult(intent, ELS_REQUESTCODE);
            }
        });

        Drawable backgroundDrawable = res.getDrawable(res
                .getIdentifier(
                        Constants.CURRENT_PACKAGE_NAME
                                + ":drawable/ab_solid_custom_blue_inverse_holo_sprd",
                        null, null));
        ActionBar actionBar = getActionBar();
        /*
         * SPRD:
         *     Bug248152
         *     remove the logo on actionbar
         * @{
         */
        actionBar.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM | ActionBar.DISPLAY_SHOW_TITLE);
        /*
         * @}
         */
        actionBar.setBackgroundDrawable(backgroundDrawable);
    }

    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
    }

    @Override
    public void onClick(View v) {
        Log.d(TAG, "v.getId()=" + v.getId());
        switch (v.getId()) {

            case R.id.testbutton:
                Settings.System.putInt(getContentResolver(),
                        Constants.SYSTEM_SETTINGS_LOCKSTYLE, -1);
                mTestButton.setText("defualt style");
                mElsInfoAdapter.setSelected(-1);
                mElsInfoAdapter.notifyDataSetChanged();
                break;
            default:
                Settings.System.putInt(getContentResolver(),
                        Constants.SYSTEM_SETTINGS_LOCKSTYLE, 0);
                mTestButton.setText("defualt style");
                break;

        }

    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onResume() {
        super.onResume();
        int lockStyleId = Tools.getLockViewID(this);
        if (lockStyleId > -1) {
            mTestButton.setText("cutom style");
        } else {
            mTestButton.setText("defualt style");
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == ELS_REQUESTCODE && resultCode == RESULT_OK) {
            int elsId = data.getIntExtra("elsId", -1);
            Log.d(TAG, "," + elsId + ",");
            mElsInfoAdapter.setSelected(elsId);
            mElsInfoAdapter.notifyDataSetChanged();
        }
    }
}
