
package com.sprd.engineermode;

import java.util.ArrayList;
import java.util.List;

import android.app.Fragment;
import android.app.FragmentManager;
import android.support.v13.app.FragmentPagerAdapter;
import android.content.Context;

public class TabFragmentPagerAdapter extends FragmentPagerAdapter {

    private ArrayList<Fragment> mFragments;
    private int[] mTitleList;
    private Context mContext;

    public TabFragmentPagerAdapter(FragmentManager fm) {
        super(fm);
    }

    public TabFragmentPagerAdapter(FragmentManager fm, ArrayList<Fragment> fragments,
            int[] tabTitle, Context context) {
        super(fm);
        mFragments = fragments;
        mTitleList = tabTitle;
        mContext = context;
    }

    @Override
    public Fragment getItem(int position) {
        return (mFragments == null || mFragments.size() == 0) ? null : mFragments.get(position);
    }

    @Override
    public CharSequence getPageTitle(int position) {
        return (mTitleList.length > position) ? mContext.getString(mTitleList[position]) : "";
    }

    @Override
    public int getCount() {
        return mFragments == null ? 0 : mFragments.size();
    }
}
