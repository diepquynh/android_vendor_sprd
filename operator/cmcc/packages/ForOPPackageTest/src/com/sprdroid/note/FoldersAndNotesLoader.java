package com.sprdroid.note;

import android.content.Context;
import android.content.CursorLoader;
import android.database.Cursor;

public class FoldersAndNotesLoader extends CursorLoader{

	private Context mContext;
	private DBOperations mDBO;
	
	public FoldersAndNotesLoader(Context context) {
		super(context);
		mContext = context;
	}
	
	public FoldersAndNotesLoader(Context context, DBOperations dbo){
		this(context);
		mDBO = dbo;
	}
	
    @Override
    public Cursor loadInBackground() {
    		return mDBO.queryFoldersAndNotes(mContext);
    }
	
}