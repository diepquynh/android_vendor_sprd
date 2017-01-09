/**
 * SPRD:bug471289 add account guide function {@
 */

package com.android.email.activity.setup;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import android.content.Context;
import android.content.res.TypedArray;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import com.android.email.R;
import com.android.email.activity.UiUtilities;
import com.android.email.activity.setup.AccountSetupBasicsFragment.Callback;
import com.android.emailcommon.mail.Address;

public class AccountSetupGuideFragment extends AccountSetupFragment {

    private static List<Map<String, Object>> mData;
    private static TypedArray mAccountTypedIconArray;
    private static String[] mAccountTypeNameArray;
    private ListView mGuideList;

    public interface Callback extends AccountSetupFragment.Callback {
    }

    public static AccountSetupGuideFragment newInstance() {
        return new AccountSetupGuideFragment();
    }

    public AccountSetupGuideFragment() {}

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        final View view = inflateTemplatedView(inflater, container,
                R.layout.account_setup_guide_fragment, -1);
        final Callback callback = (Callback) getActivity();

        mEmailType = null;
        mGuideList = (ListView) view.findViewById(R.id.guide_list);

        mAccountTypeNameArray =
                getResources().getStringArray(R.array.account_type_display_names);
        mAccountTypedIconArray =
                getResources().obtainTypedArray(R.array.account_type_display_icons);
        mData = getData();
        AccountTypeAdapter mAdapter = new AccountTypeAdapter(getActivity());
        mGuideList.setAdapter(mAdapter);
        mGuideList.setOnItemClickListener(new AdapterView.OnItemClickListener() {

            public void onItemClick(AdapterView<?> parent, View v, int position, long id) {
                showDetails(position);
                callback.onNextButton();
            }
        });
        setButtonView(View.GONE);

        return view;
    }

    public void showDetails(int index) {
        String[] mAccountTypeDomainArray;
        mAccountTypeDomainArray = getResources().getStringArray(
                R.array.account_type_domain_names);

        if (index < mAccountTypeNameArray.length) {
            mEmailType = mAccountTypeDomainArray[index];
        }

    }

    private List<Map<String, Object>> getData() {
        List<Map<String, Object>> mTypeList = new ArrayList<Map<String, Object>>();

        for (int i = 0; i < mAccountTypeNameArray.length; i++) {
            Map<String, Object> map = new HashMap<String, Object>();
            map.put("info", mAccountTypeNameArray[i]);
            map.put("img", mAccountTypedIconArray.getResourceId(i, -1));
            mTypeList.add(map);
        }
        // Add "others"item at the last
        Map<String, Object> map = new HashMap<String, Object>();
        map.put("info",getResources().getString(R.string.account_type_display_others));
        map.put("img", R.drawable.ic_mail_type_other);
        mTypeList.add(map);
        return mTypeList;
    }

    public class ViewHolder {
        public ImageView img;
        public TextView info;
    }

    public class AccountTypeAdapter extends BaseAdapter {

        private LayoutInflater mInflater;

        public AccountTypeAdapter(Context context) {
            this.mInflater = LayoutInflater.from(context);
        }

        @Override
        public int getCount() {
            return mData.size();
        }

        @Override
        public Object getItem(int arg0) {
            return mData.get(arg0);
        }

        @Override
        public long getItemId(int arg0) {
            return arg0;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {

            ViewHolder mHolder = null;
            // Only loading layout.xml when convertView is null, so as to avoid
            // unnecessary performance loss
            if (convertView == null) {

                mHolder = new ViewHolder();
                convertView = mInflater.inflate(
                        R.layout.account_setup_guide_item, null);
                mHolder.img = (ImageView) convertView
                        .findViewById(R.id.mail_type_icon);
                mHolder.info = (TextView) convertView
                        .findViewById(R.id.mail_type_name);
                convertView.setTag(mHolder);

            } else {
                mHolder = (ViewHolder) convertView.getTag();
            }
            mHolder.img.setBackgroundResource((Integer) mData.get(position).get("img"));
            mHolder.info.setText((String) mData.get(position).get("info"));

            return convertView;
        }
    }
}
/**
 * @}
 */