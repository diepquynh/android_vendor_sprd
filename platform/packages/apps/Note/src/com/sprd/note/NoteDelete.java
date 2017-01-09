
package com.sprd.note;

import java.util.ArrayList;
import java.util.List;

import android.app.ActionBar;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.LoaderManager;
import android.app.LoaderManager.LoaderCallbacks;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.CursorLoader;
import android.content.DialogInterface;
import android.content.Loader;
import android.database.Cursor;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.CheckBox;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.note.data.NoteItem;
import com.sprd.note.data.NoteProvider;

public class NoteDelete extends BaseActivity implements OnItemClickListener {

    private static String TAG = "NoteDelete";
    private ListView mNoteListView;
    private NoteAdapter mAdapter;
    private Cursor mCursor;

    private CheckBox mCheckAll;
    private TextView mCheckAllText;
    // this flag indicate to display the notes in the root or one folder.
    public static final int FOLDER_NO = -1;
    private int mFolderID = FOLDER_NO;

    public static final int DIALOG_DELTE_SOME_NOTES = 4;
    public static final int DIALOG_DELTE_PROGRESS = 88;
    
    public static final int MSG_PROGRESS_DIALOG_SHOW = 1;
    public static final int MSG_PROGRESS_DIALOG_DISMISS = 2;
    public static final int MSG_TOAST_SUCCESSED = 3;
    
    public static final String CURRENT_FOLDER_ID = "com.android.sprdnote.CURRENT_FOLDER_ID";
    public static final String CURRENT_AB_TITLE = "com.android.sprdnote.CURRENT_AB_TITLE";

    protected List<NoteItem> mItems = null;// list all items
    private static MenuItem mMenuDone;
    public static Boolean isDeleting = false;
    private WakeLock mWakeLock;
    private ArrayList<Integer> mCheckedList;
    //SPRD 597364
    private LoaderManager mLoaderManager;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate");
        setContentView(R.layout.note_delete_layout);
        if (null != getIntent() && null != getIntent().getExtras()) {
            mFolderID = getIntent().getExtras().getInt(CURRENT_FOLDER_ID);
        }
        /* SPRD 597364 @{ */
         mLoaderManager = getLoaderManager();
         if(isLoad()){
             Log.d(TAG, "mLoaderManager start first");
             mLoaderManager.initLoader(NoteActivity.LOADER_NOTES, null, mNoteLoaderListener);
         }else{
             Log.d(TAG, "mLoaderManager restart");
             mLoaderManager.getLoader(NoteActivity.LOADER_NOTES).commitContentChanged();
         }
         /* @} */
        initViews();
    }

    /* SPRD 597364 @{ */
    private boolean isLoad(){
        if(mLoaderManager == null || mLoaderManager.getLoader(NoteActivity.LOADER_NOTES) == null){
            Log.d(TAG, "mLoaderManager is null");
            return true;
        }
        if(mLoaderManager.getLoader(NoteActivity.LOADER_NOTES) == null){
            Log.d(TAG, "getLoader is null");
            return true;
        }
        if(mCursor == null){
            Log.d(TAG, "cursor is null");
            return false;
        }
        return false;
    }
    /* @} */
    private void initViews() {
        Log.d(TAG, "initViews");
        mNoteListView = (ListView) findViewById(R.id.page_list);
        View listHeaderView = getLayoutInflater().inflate(R.layout.note_delete_header, null);
        mNoteListView.addHeaderView(listHeaderView);
        mCheckAllText = (TextView) findViewById(R.id.select_text);
        mCheckAll = (CheckBox) listHeaderView.findViewById(R.id.select_check);// selece_all_notes
        mCheckAll.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (!mCheckAll.isChecked()) {
                    mAdapter.setAllItemCheckedAndNotify(false);
                    mCheckAllText.setText(R.string.select_all);
                    if (mMenuDone != null) {
                        mMenuDone.setEnabled(false);
                    }
                } else {
                    mAdapter.setAllItemCheckedAndNotify(true);
                    mCheckAllText.setText(R.string.unselect_all);
                    if (mMenuDone != null) {
                        mMenuDone.setEnabled(true);
                    }
                }
            }
        });

        mNoteListView.setVerticalScrollBarEnabled(true);// vertical scroll;
        mNoteListView.setOnItemClickListener(this);
    }

    /*
     * update listview items
     */
    protected void updateDisplay() {
        Log.i(TAG, "updateDisplay displayFoldID-->" + mFolderID);
        if (isDeleting == true) {
            return;
        }
        if (mFolderID == FOLDER_NO) {
            getActionBar().setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP | ActionBar.DISPLAY_SHOW_TITLE);
            getActionBar().setTitle(R.string.app_name);
        } else {
            getActionBar().setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP | ActionBar.DISPLAY_SHOW_HOME | ActionBar.DISPLAY_SHOW_TITLE);
            getActionBar().setIcon(R.drawable.ic_group_sprd);
            NoteItem noteItem = getDataManager(this).getNoteItem(mFolderID);
            if (noteItem != null) {
                getActionBar().setTitle(noteItem.getShowTitle());
            }
        }
        if (mItems == null || mItems.size() == 0) {
            if (mFolderID == FOLDER_NO) {
                mItems = getDataManager(this).getRootFoldersAndNotes();
            } else {
                mItems = getDataManager(this).getNotesFromFolder(mFolderID);
            }
        }
        if (mItems != null && mItems.size() != 0 && mCheckedList != null && mCheckedList.size() != 0) {
            if (mMenuDone != null) {
                mMenuDone.setEnabled(true);
            }
            if (mItems.size() == mCheckedList.size()) {
                mCheckAll.setChecked(true);
            }else {
                mCheckAll.setChecked(false);
            }
            for (int i = 0; i < mCheckedList.size(); i++) {
                mItems.get(mCheckedList.get(i)).isSelected = true;
            }
        }
        if (mAdapter == null) {
            mAdapter = new NoteAdapter(this, mItems);
            mNoteListView.setAdapter(mAdapter);
        } else {
            mAdapter.setListItems(mItems);
            mAdapter.notifyDataSetChanged();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.i(TAG, "onResume");
        updateDisplay();
        if (mAdapter != null && mAdapter.getShowType() != NoteAdapter.SHOW_TYPE_DELETE) {
            mAdapter.setShowType(NoteAdapter.SHOW_TYPE_DELETE);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_notes_delete, menu);
        mMenuDone = menu.findItem(R.id.menu_notes_delete_done);
        return true;
    }
    
    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        if (mAdapter != null && mAdapter.getSelectedCount() > 0) {
            mMenuDone.setEnabled(true);
        }else {
            mMenuDone.setEnabled(false);
        }
        return super.onPrepareOptionsMenu(menu);
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                break;
            case R.id.menu_notes_delete_cancel:
                finish();
                return true;

            case R.id.menu_notes_delete_done:
                mMenuDone.setEnabled(false);
                deleteSomeNotes();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        Log.d(TAG, "onItemClick");
        ListItemView listItemViews = (ListItemView) view.getTag();
        //SPRD 343886:Crashed in monkey test.
        if (listItemViews == null) {
            return;
        }
        listItemViews.mCheckView.toggle();
        mAdapter.toggleChecked(position - 1);
        switch (mAdapter.getCheckedState()) {
            case NoteAdapter.CHECKED_NONE:
                mCheckAll.setChecked(false);
                mCheckAllText.setText(R.string.select_all);
                if (mMenuDone != null) {
                    mMenuDone.setEnabled(false);
                }
                break;
            case NoteAdapter.CHECKED_SOME:
                mCheckAll.setChecked(false);
                mCheckAllText.setText(R.string.select_all);
                if (mMenuDone != null) {
                    mMenuDone.setEnabled(true);
                }
                break;
            case NoteAdapter.CHECKED_ALL:
                mCheckAll.setChecked(true);
                mCheckAllText.setText(R.string.unselect_all);
                if (mMenuDone != null) {
                    mMenuDone.setEnabled(true);
                }
                break;
            default:
                break;
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        Log.d(TAG, "onSaveInstanceState");
        super.onSaveInstanceState(outState);
        outState.putInt("folder_id", mFolderID);
        if (mAdapter == null) {
            return;
        }
        ArrayList<Integer> checkedList = new ArrayList<Integer>();
        int j = 0;
        for (int i = 0; i < mAdapter.getCount(); i++) {
            if (mAdapter.getItem(i).isSelected) {
                checkedList.add(j, Integer.valueOf(i));
                j++;
            }
        }
        // SPRD: Add for bug460962.
        mCheckedList = null;
        outState.putIntegerArrayList("checked_list", checkedList);
        outState.putBoolean("checked_all", mCheckAll.isChecked());
    }

    /*SPRD: modified for bug615805  @{*/
    @Override
    protected void onDestroy(){
        super.onDestroy();
        if (mAdapter != null ){
            mAdapter.setAllItemCheckedAndNotify(false);
        }
    }
    /*}@*/
    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        mFolderID = savedInstanceState.getInt("folder_id");
        mCheckedList = savedInstanceState.getIntegerArrayList("checked_list");
        Boolean isCheckAll = savedInstanceState.getBoolean("checked_all");
        mCheckAll.setChecked(isCheckAll);
        /* SPRD: modify 20140514 Spreadtrum of 312379 After you change the language, deselect all into the Select @{ */
        if (!mCheckAll.isChecked()) {
            mCheckAllText.setText(R.string.select_all);
        } else {
            mCheckAllText.setText(R.string.unselect_all);
        }
        /* @} */
    }
    
    private void deleteSomeNotes() {
        int mSelectedCount = 0;
        for (int i = 0; i < mAdapter.getCount(); i++) {
            if (mAdapter.getItem(i).isSelected) {
                mSelectedCount++;
            }
        }
        if (mSelectedCount == 0) {  
            Toast.makeText(getApplicationContext(), R.string.selected_is_empty, Toast.LENGTH_LONG).show();
            return;
        }
        Bundle bundle = new Bundle();
        bundle.putInt("tag", DIALOG_DELTE_SOME_NOTES);
        showDialog(bundle);
    }

    private final LoaderManager.LoaderCallbacks<Cursor> mNoteLoaderListener = new LoaderCallbacks<Cursor>() {
        public CursorLoader onCreateLoader(int id, Bundle args) {
            // use given URI to new a CursorLoader sample and return
            return new CursorLoader(NoteDelete.this, NoteProvider.NOTE_CONTENT_URI, null, null, null, null);
        }

        public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
            if (isDeleting) {
                return;
            }
            mCursor = data;
            bindCursor();
            updateDisplay();
        }

        public void onLoaderReset(Loader<Cursor> loader) {
        }
    };

    private void bindCursor() {
        if (mCursor == null) {
            Log.d("onLoadFinished", "mCursor == null");
            return;
        }
        getDataManager(this).setCursor(mCursor);
    }

    public static class AlertDialogFragment extends DialogFragment {
        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            final int fragmentTag = getArguments().getInt("tag");
            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            switch (fragmentTag) {
                case DIALOG_DELTE_SOME_NOTES:
                    builder.setNegativeButton(R.string.Cancel, null);
                    builder.setTitle(R.string.prompt);
                    builder.setMessage(R.string.delete_selected_items); 
                    builder.setPositiveButton(R.string.Ok,
                        new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            ((NoteDelete)getActivity()).doPositiveClick(which, dialog, getArguments());
                        }
                    });
                    break;
                case DIALOG_DELTE_PROGRESS:
                    ProgressDialog mProgressDialog = new ProgressDialog(getActivity());
                    mProgressDialog.setTitle(R.string.delete);
                    mProgressDialog.setMessage(getActivity().getResources().getString(R.string.Deleting));
                    mProgressDialog.setCanceledOnTouchOutside(false);
                    return mProgressDialog;
                default:
                    break;
            }
            return builder.create();
        }
        /* SPRD: modify 20131209 Spreadtrum of 248851 double click option "Done" and alertDialog display twice @{ */
        @Override
        public void onDestroyView() {
            Log.d(TAG, " onDestroyView");
            mMenuDone.setEnabled(true);
            super.onDestroyView();
        }
        /* @} */
    }
    
    public void doPositiveClick (int which, DialogInterface dialog, Bundle bundle) {
        
        switch (bundle.getInt("tag")) {
            case DIALOG_DELTE_SOME_NOTES:
                Log.d("doPositiveClick", "deleteTask.execute");
                DeleteTask deleteTask = new DeleteTask();
                deleteTask.execute(DIALOG_DELTE_SOME_NOTES);
                break;
            default:
                break;
        }
    }
    
    void showDialog (Bundle bundle) {
        AlertDialogFragment dialogFragment = new AlertDialogFragment();
        dialogFragment.setArguments(bundle);
        if (bundle.getInt("tag") == DIALOG_DELTE_PROGRESS) {
            dialogFragment.setCancelable(false);
        }else {
            dialogFragment.setCancelable(true);
        }
        dialogFragment.show(getFragmentManager(), "" + bundle.getInt("tag"));
    }

    public void acquireWakeLock() {
        PowerManager powerManager = (PowerManager) getApplicationContext().getSystemService(Context.POWER_SERVICE);
        if (mWakeLock == null) {
            mWakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "delete_notes");
            mWakeLock.acquire();
        }
    }

    public void releaseWakeLock() {
        if (mWakeLock != null) {
            mWakeLock.release();
            mWakeLock = null;
        }
    }

    private class DeleteTask extends AsyncTask<Integer, Integer, Boolean> {

        @Override
        protected void onPreExecute() {
            acquireWakeLock();
            Bundle bundle = new Bundle();
            bundle.putInt("tag", DIALOG_DELTE_PROGRESS);
            showDialog(bundle);
        }

        @Override
        protected Boolean doInBackground(Integer... params) {
            isDeleting = true;
            List<NoteItem> deleteItems = null;
            for (int i = 0; i < mAdapter.getCount(); i++) {
                if (mAdapter.getItem(i).isSelected) {
                    NoteItem item = mItems.get(i);
                    getDataManager(NoteDelete.this).deleteNoteItem(item);
                }
            }
            return true;
        }

        @Override
        protected void onPostExecute(Boolean result) {
            AlertDialogFragment dialogFragment = (AlertDialogFragment) getFragmentManager().findFragmentByTag("" + DIALOG_DELTE_PROGRESS);
            if (dialogFragment != null) {
                dialogFragment.dismissAllowingStateLoss();
            }
            releaseWakeLock();
            Toast.makeText(getApplicationContext(), R.string.delete_success, Toast.LENGTH_SHORT).show();
            isDeleting = false;
            finish();
        }

    }

}
