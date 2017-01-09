package com.sprd.cellbroadcastreceiver.ui;

import java.util.ArrayList;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;

import com.sprd.cellbroadcastreceiver.R;
import com.sprd.cellbroadcastreceiver.data.LanguageMgr;
import com.sprd.cellbroadcastreceiver.ui.LanguageSettingActivity;
import com.sprd.cellbroadcastreceiver.util.LanguageIds;

public class LanguageChooseDialogActivity extends Activity {

    private String TAG = "LanguageChooseDialogActivity";

    private ListView mLangToChooseLV;
    private ArrayList<Integer> mLangIndexList;
    private String[] mLangString;

    private ArrayList<Integer> getLangBitList() {
        return mLangIndexList;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.language_choosing);

        mLangToChooseLV = (ListView) findViewById(R.id.language_list);
        mLangIndexList = new ArrayList<Integer>();
        getShowLangList();

        mLangToChooseLV.setOnItemClickListener(clickListner);
        mLangToChooseLV.setAdapter(new ArrayAdapter<String>(this,
                R.layout.language_choosing_item, mLangString));
    }

    private boolean getShowLangList() {
        mLangIndexList = LanguageMgr.getInstance().filterToLangChooseAdapter(
                getLangBitList());
        int size = mLangIndexList.size();
        mLangString = new String[size];
        for (int i = 0; i < size; i++) {
            int bit = LanguageMgr.getInstance().get(mLangIndexList.get(i)).getLanguageBit();
            mLangString[i] = getString(LanguageIds.LangMap[bit-1]);
        }
        return true;
    }

    private AdapterView.OnItemClickListener clickListner = new AdapterView.OnItemClickListener() {
        public void onItemClick(AdapterView<?> parent, View view, int position,
                long id) {
            if (view != null) {
                int index = mLangIndexList.get(position);
                if (LanguageMgr.getInstance().get(index).getEnabled() == 1) {
                    // this language had been added
                    final Toast toast = Toast
                            .makeText(getApplicationContext(),
                                    R.string.language_had_been_added,
                                    Toast.LENGTH_LONG);
                    toast.setGravity(
                            Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL, 0, 20);
                    toast.show();
                    //LanguageChooseDialogActivity.this.finish();
                } else {
                    LanguageMgr.getInstance().get(index).setEnabled(1);
                    //add for bug 532474
                    setResult(LanguageSettingActivity.RESULT_CHANGED,
                            getIntent());
                    LanguageChooseDialogActivity.this.finish();
                }
            }
        }
    };
}
