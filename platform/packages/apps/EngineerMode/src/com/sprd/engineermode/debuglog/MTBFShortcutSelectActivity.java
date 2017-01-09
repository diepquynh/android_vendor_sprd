
package com.sprd.engineermode.debuglog;

import java.io.File;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import android.util.Log;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;
import android.preference.TwoStatePreference;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.CheckBox;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.engineermode.R;
import com.sprd.engineermode.R.xml;

public class MTBFShortcutSelectActivity extends PreferenceActivity implements OnClickListener,
        OnPreferenceChangeListener {
    private static final String TAG = "MTBFShortcutSelectActivity";
    private List<ResolveInfo> mAppList = new ArrayList<ResolveInfo>();
    private CheckBox mSelectAllCb = null;
    private TextView mAllSelectText = null;
    private int mSelCount = 0;
    PreferenceGroup mPreGroup = null;

    private void initPreferenceAPPList() {
        PackageManager packageManager = getPackageManager();
        List<ResolveInfo> apps = null;
        Set<String> uidSet = new HashSet<String>();

        // App filter
        Intent mainIntent = new Intent(Intent.ACTION_MAIN, null);
        mainIntent.addCategory(Intent.CATEGORY_LAUNCHER);
        apps = packageManager.queryIntentActivities(mainIntent, 0);

        // The apps which shoud be checked
        Intent intent = getIntent();
        Bundle bundle = intent.getExtras();
        if(bundle == null){
            return;
        }
        ArrayList<String> appChecked = bundle.getStringArrayList(MTBFActivity.NAMELIST);

        for (ResolveInfo resolveInfo : apps) {
            if (uidSet.add((String) packageManager
                    .getApplicationLabel(resolveInfo.activityInfo.applicationInfo))) {
                CheckBoxPreference appItem = new CheckBoxPreference(this);

                mAppList.add(resolveInfo);

                appItem.setTitle(packageManager
                        .getApplicationLabel(resolveInfo.activityInfo.applicationInfo));
                appItem.setSummary(resolveInfo.activityInfo.packageName);
                appItem.setOnPreferenceChangeListener(this);
                for (String s : appChecked) {
                    if (0 == s.compareTo(resolveInfo.activityInfo.packageName))
                    {
                        appItem.setChecked(true);
                        mSelCount++;
                    }
                }
                mPreGroup.addPreference(appItem);
            }
        }

        if (mSelCount == mPreGroup.getPreferenceCount()) {
            mSelectAllCb.setChecked(true);
            mAllSelectText.setText(R.string.select_all);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
        setContentView(R.layout.mtbf_shortcut_select);
        mPreGroup = getPreferenceScreen();
        mSelectAllCb = (CheckBox) findViewById(R.id.select_all_cb);
        mAllSelectText = (TextView) findViewById(R.id.allselect_text);
        mSelectAllCb.setOnClickListener(this);

        initPreferenceAPPList();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.shortcut_menu_ok: {
                saveAndSetResult();
                finish();
                return true;
            }
            default:
                return false;
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.multi_select_menu, menu);
        return true;
    }

    @Override
    public void onClick(View v) {
        CheckBox allSelectCb = (CheckBox) v;
        CheckBoxPreference checkBox = null;
        for (int index = 0; index < mPreGroup.getPreferenceCount(); index++) {
            checkBox = (CheckBoxPreference) mPreGroup.getPreference(index);
            checkBox.setChecked(allSelectCb.isChecked());
        }

        if (allSelectCb.isChecked()) {
            mSelCount = mPreGroup.getPreferenceCount();
            mAllSelectText.setText(R.string.unselect_all);
        } else {
            mSelCount = 0;
            mAllSelectText.setText(R.string.select_all);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference pre, Object newValue) {
        TwoStatePreference checkBoxPre = (TwoStatePreference) pre;

        if (checkBoxPre.isChecked()) {
            if (mSelCount == mPreGroup.getPreferenceCount()) {
                mSelectAllCb.setChecked(false);
                mAllSelectText.setText(R.string.select_all);
            }
            mSelCount--;
        } else {
            mSelCount++;
            if (mSelCount == mPreGroup.getPreferenceCount()) {
                mSelectAllCb.setChecked(true);
                mAllSelectText.setText(R.string.unselect_all);
            }
        }
        return true;
    }

    @Override
    public void onBackPressed() {
        saveAndSetResult();
        finish();
    }

    private void saveAndSetResult(){
        Intent intent = getIntent();
        Bundle bundle = new Bundle();
        CheckBoxPreference checkBox = null;
        ArrayList<String> selectPackagesName = new ArrayList<String>();

        for (int index = 0; index < mPreGroup.getPreferenceCount(); index++) {
            checkBox = (CheckBoxPreference) mPreGroup.getPreference(index);
            if (checkBox.isChecked()) {
                selectPackagesName.add(mAppList.get(index).activityInfo.packageName);
            }
        }
        bundle.putStringArrayList(MTBFActivity.NAMELIST, selectPackagesName);
        intent.putExtras(bundle);
        MTBFShortcutSelectActivity.this.setResult(0, intent);
    }
}
