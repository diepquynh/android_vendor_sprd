/*
 * SPRD: FEATURE_VOLTE_LOCK
 *
 */
package com.android.callsettings;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import android.util.AtomicFile;
import android.util.Log;
import android.util.Xml;
import com.android.internal.util.XmlUtils;
import com.android.internal.util.FastXmlSerializer;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import com.android.internal.telephony.VolteConfig;

public class VoLTEConfigSettings extends Activity{

    private String LOG_TAG = "VoLTEConfigSettings";
    private final int MENU_ADD = Menu.FIRST;
    private final int INVALID_PLMN_POSITION = -1;
    private VolteConfig mVolteConfig;
    private List<String> mAllList = new ArrayList<String>();
    private List<String> mPrebuiltPlmnList = new ArrayList<String>();
    private List<String> mUserPlmnList = new ArrayList<String>();
    // add disable plmns
//    private List<String> mDisAllowPlmnList = new ArrayList<String>();
    private VoListAdapter mPlmnListAdapter;
    private ListView mListView = null;
    /* SPRD: Use file to save volte list for bug 595645. @{ */
    private static final String PLMN_ENABLE_FILE_PATH = "/data/data/com.android.callsettings";
    private static final String PLMN_ENABLE_FILE_NAME = "voltelock-usr.xml";
    private File mVolteUserFile;
    /* @} */

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.volte_config_list);
        mListView =  (ListView) findViewById(R.id.plmn_config_list);
        mVolteConfig = VolteConfig.getInstance();
        mPrebuiltPlmnList = mVolteConfig.getPrebuiltConfig();
        /* SPRD: Use file to save volte list for bug 595645. @{ */
        mVolteUserFile = new File(PLMN_ENABLE_FILE_PATH, PLMN_ENABLE_FILE_NAME);
        loadUserPlmnList(mVolteUserFile);
        /* @} */
        getAllVoList();
        mPlmnListAdapter = new VoListAdapter(this,mAllList);
        mListView.setAdapter(mPlmnListAdapter);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        menu.add(0, MENU_ADD, 0, R.string.menu_new_plmn)
                .setIcon(android.R.drawable.ic_menu_add);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case MENU_ADD:
                showDialog("add", "", INVALID_PLMN_POSITION);
                return true;
            default :
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    private boolean isUserPlmnExisted(String plmn) {
        for (int i = 0; i < mAllList.size(); i++) {
            if (mAllList.get(i).equals(plmn)) {
                return true;
            }
        }
        return false;
    }

    private boolean isPrebuiltPlmnExisted(String plmn) {
        for (int i = 0; i < mPrebuiltPlmnList.size(); i++) {
            if (mPrebuiltPlmnList.get(i).equals(plmn)) {
                return true;
            }
        }
        return false;
    }

    private void showDialog(final String action, final String oldPlmn,
            final int position) {
        LayoutInflater factory = LayoutInflater.from(VoLTEConfigSettings.this);
        final View view = factory.inflate(R.layout.volist_add, null);
        final EditText editText = (EditText) view.findViewById(R.id.plmn_value);
        final String editAction = VoLTEConfigSettings.this.getString(R.string.plmn_edit_tip);
        final String newAction = VoLTEConfigSettings.this.getString(R.string.plmn_new_tip);
        final boolean isEditPlmn = action.equals(editAction);
        if (isEditPlmn) {
            editText.setText(oldPlmn);
        } else {
            editText.setHint(R.string.enter_plmn_hint);
        }
        AlertDialog dialog = new AlertDialog.Builder(VoLTEConfigSettings.this)
                .setTitle(isEditPlmn
                        ? editAction : newAction)
                .setView(view)
                .setPositiveButton(android.R.string.ok,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                final String addValue = editText.getText()
                                        .toString();
                                if (!checkInputValidation(addValue)) {
                                    if (isEditPlmn) {
                                        editPlmn(oldPlmn, addValue, position);
                                    } else {
                                        addPlmn(addValue);
                                    }
                                    // SPRD: Use file to save volte list for bug 595645.
                                    writeUserPlmnList();
                                    mPlmnListAdapter.notifyDataSetChanged();
                                    Log.d(LOG_TAG,"action : " + action
                                            + "; add : " + addValue + ";old : " + oldPlmn);
                                }
                            }
                        }).setNegativeButton(android.R.string.cancel, null)
                .create();
        dialog.show();
    }

    private void getAllVoList() {
        mAllList.addAll(mPrebuiltPlmnList);
        mAllList.addAll(mUserPlmnList);
        Log.d(LOG_TAG, "mUserPlmnList : " + mUserPlmnList + "\n mPrebuiltPlmnList : " + mPrebuiltPlmnList);
    }

    /* SPRD: Use file to save volte list for bug 595645. @{ */
    private void loadUserPlmnList(File volteFile) {
        FileReader volteReader;
        try {
            volteReader = new FileReader(volteFile);
        } catch (FileNotFoundException e) {
            Log.e(LOG_TAG, "Can not open " + volteFile.getAbsolutePath());
            return;
        }
        try {
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(volteReader);
            XmlUtils.beginDocument(parser, "volteEnables");
            while (true) {
                XmlUtils.nextElement(parser);
                String name = parser.getName();
                if (!"volteEnable".equals(name)) {
                    break;
                }
                String numeric = parser.getAttributeValue(null, "numeric");
                Log.d(LOG_TAG, "numeric : " + numeric + "; existPlmn : "
                        + mUserPlmnList.contains(numeric));
                if (!mUserPlmnList.contains(numeric)) {
                    mUserPlmnList.add(numeric);
                }
            }
            volteReader.close();
        } catch (XmlPullParserException e) {
            Log.e(LOG_TAG, "Exception in volte-conf parser " + e);
        } catch (IOException e) {
            Log.e(LOG_TAG, "Exception in volte-conf parser " + e);
        }
    }

    private void writeUserPlmnList() {
        FileOutputStream os;
        try {
            File dir = new File(PLMN_ENABLE_FILE_PATH);
            if (!existFileDirectory(dir)) {
                dir.mkdirs();
            }
            AtomicFile mAtomicFile = new AtomicFile(
                    new File(dir, PLMN_ENABLE_FILE_NAME));
            os = mAtomicFile.startWrite();
            boolean success = false;
            try {
                XmlSerializer serializer = new FastXmlSerializer();
                serializer.setOutput(new BufferedOutputStream(os), "utf-8");
                serializer.startTag(null, "volteEnables");
                for (int i = 0; i < mUserPlmnList.size(); i++) {
                    serializer.startTag(null, "volteEnable");
                    serializer.attribute(null, "numeric", mUserPlmnList.get(i));
                    serializer.attribute(null, "enable", "true");
                    serializer.endTag(null, "volteEnable");
                }
                serializer.endTag(null, "volteEnables");
                serializer.endDocument();
                serializer.flush();
                success = true;
            } finally {
                if (success) {
                    Log.d(LOG_TAG, "Write success");
                    mAtomicFile.finishWrite(os);
                    os.close();
                } else {
                    Log.d(LOG_TAG, "Write fail");
                    mAtomicFile.failWrite(os);
                    os.close();
                }
            }
        } catch (Exception e) {
            Log.e(LOG_TAG, "Exception in volte-conf writer " + e);
        }
    }

    private boolean existFileDirectory(File file) {
        if (!file.exists() && !file.isDirectory()) {
            return false;
        }
        return true;
    }

    private void addPlmn(String key){
        mAllList.add(key);
        mUserPlmnList.add(key);
    }

    private void deletePlmn(String key, int position){
        mAllList.remove(position);
        mUserPlmnList.remove(key);
    }

    private void editPlmn(String oldKey, String newKey, int position) {
        mAllList.remove(position);
        mAllList.add(newKey);
        mUserPlmnList.remove(oldKey);
        mUserPlmnList.add(newKey);
    }
    /* @} */

    private boolean checkInputValidation(String plmn) {
        boolean isWrong = false;
        // add disable plmns
        if (mVolteConfig.isPlmnDisAllowed(plmn)) {
        	Log.d(LOG_TAG, "is in disallowplmns");
        	Toast.makeText(VoLTEConfigSettings.this, getString(R.string.plmn_disallow_plmn_toast),
                    Toast.LENGTH_SHORT).show();
        	return true;
        }
        if (plmn == null || plmn.length() == 0 || plmn.length() < 5) {
            isWrong = true;
            Toast.makeText(VoLTEConfigSettings.this, getString(R.string.plmn_enter_error),
                    Toast.LENGTH_SHORT).show();
            return isWrong;
        } else if(isUserPlmnExisted(plmn)){
            isWrong = true;
            Toast.makeText(VoLTEConfigSettings.this, getString(R.string.plmn_exist),
                    Toast.LENGTH_SHORT).show();
            return isWrong;
        }
        return isWrong;
    }

    public class VoListAdapter extends BaseAdapter {

        private Context context;
        private List<String> voList;

        private VoListAdapter(Context context, List<String> list) {
            this.context = context;
            if (list != null) {
                this.voList = list;
            } else {
                list = new ArrayList<String>();
            }
        }

        @Override
        public int getCount() {
            return voList.size();
        }

        @Override
        public Object getItem(int position) {
            return voList.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(final int position, View convertView, ViewGroup vewGroup) {
            if (convertView == null) {
                convertView = LayoutInflater.from(context).inflate(R.layout.volist_item_view,
                        null);
            }
            final TextView textView = (TextView) convertView.findViewById(R.id.tv_plmn);
            ImageButton imaBtnDelete = (ImageButton) convertView.findViewById(R.id.imaBtn_delete);
            ImageButton imaBtnEdit = (ImageButton) convertView.findViewById(R.id.imaBtn_edit);
            textView.setText(voList.get(position));
            if (isPrebuiltPlmnExisted(voList.get(position))) {
                textView.setEnabled(false);
                imaBtnDelete.setVisibility(View.GONE);
                imaBtnEdit.setVisibility(View.GONE);
            } else {
                textView.setEnabled(true);
                imaBtnDelete.setVisibility(View.VISIBLE);
                imaBtnEdit.setVisibility(View.VISIBLE);
            }
            imaBtnDelete.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View view) {
                    Log.d(LOG_TAG, "delete plmn : " + textView.getText()
                            + ";position : " + position);
                    deletePlmn(textView.getText().toString(), position);
                    // SPRD: Use file to save volte list for bug 595645.
                    writeUserPlmnList();
                    mPlmnListAdapter.notifyDataSetChanged();
                }
            });
            imaBtnEdit.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View view) {
                    showDialog(context.getString(R.string.plmn_edit_tip),
                            textView.getText().toString(), position);
                }
            });
            return convertView;
        }
    }

}