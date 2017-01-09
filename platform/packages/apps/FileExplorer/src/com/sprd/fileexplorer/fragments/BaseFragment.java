package com.sprd.fileexplorer.fragments;

import android.support.v4.app.Fragment;

import com.sprd.fileexplorer.adapters.FileAdapter;

abstract public class BaseFragment extends Fragment {
    
    protected boolean mEnableExit;

    /**
     * Our fragment has onBackPressed now, we can use it to pop folder
     * 
     * @return return if allow finish the activity
     */
    abstract public boolean onBackPressed();

    public abstract int getIcon();
    
    abstract public FileAdapter getAdapter();
}
