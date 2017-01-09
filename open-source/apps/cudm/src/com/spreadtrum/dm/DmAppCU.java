
package com.spreadtrum.dm;

import com.spreadtrum.dm.R;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.ListView;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Toast;
import android.widget.AdapterView.OnItemClickListener;

public class DmAppCU extends Activity {

    private ListView mListView;

    private String[] mlistItem;

    private Toast mToast;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mListView = (ListView) findViewById(R.id.app_list);

        // init list item content
        mlistItem = getResources().getStringArray(R.array.app_list);

        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, R.layout.text_view, mlistItem);
        mListView.setAdapter(adapter);
        mListView.setOnItemClickListener(mItemClickListenter);
    }

    private void ShowMessage(CharSequence msg) {
        if (mToast == null)
            mToast = Toast.makeText(this, msg, Toast.LENGTH_LONG);
        mToast.setText(msg);
        mToast.show();
    }

    private OnItemClickListener mItemClickListenter = new OnItemClickListener() {
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            DmServiceCU selfReg = DmServiceCU.getInstance();
            switch (position) {
                case 0:
                    // display current software version info
                    ShowMessage(selfReg.getSoftwareVersion());
                    break;

                case 1:
                    // search avalable software update package and download it
                    //do nothing here.
                    /*
                    if (selfReg.isSelfRegOk()) {
                    } else {
                        ShowMessage("Have not self registe!");
                    }
                    */
                    break;

                case 2:
                    // intall software update package immediatly
                    ShowMessage("No local update package!");
                    break;

                default:
                    break;
            }
        }
    };

}
