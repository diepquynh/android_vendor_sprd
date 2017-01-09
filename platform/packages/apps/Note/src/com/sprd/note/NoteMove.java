
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

import com.sprd.note.data.NoteDataManager;
import com.sprd.note.data.NoteItem;
import com.sprd.note.data.NoteProvider;
public class NoteMove extends BaseActivity implements OnItemClickListener {

    private static String TAG = "NoteDelete";
    private ListView mNoteListView;
    private NoteAdapter mAdapter;
    private Cursor mCursor;

    private CheckBox mCheckAll;
    private TextView mCheckAllText;
    // this flag indicate to display the notes in the root or one folder.
    public static final int FOLDER_NO = -1;
    private int mFolderID = FOLDER_NO;

    public static final int DIALOG_MOVE_FOLDER = 5;
    public static final int DIALOG_MOVE_PROGRESS = 99;
    public static final String DIALOG_KEY_MOVE_TO = "moveToFolder";

    protected List<NoteItem> mItems = null;// list all items
    private MenuItem mMenuDone;
    public static Boolean isMoving = false;
    private WakeLock mWakeLock;
    private ArrayList<Integer> mCheckedList;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (null != getIntent() && null != getIntent().getExtras()) {
            mFolderID = getIntent().getExtras().getInt(NoteDelete.CURRENT_FOLDER_ID);
        }
        setContentView(R.layout.note_delete_layout);
        getLoaderManager().initLoader(NoteActivity.LOADER_NOTES, null, mNoteLoaderListener);
        initViews();
    }

    private void initViews() {
        Log.d(TAG, "initViews");
        mNoteListView = (ListView) findViewById(R.id.page_list);
        View listHeaderView = getLayoutInflater().inflate(R.layout.note_delete_header,
                null);
        mNoteListView.addHeaderView(listHeaderView);
        mCheckAllText = (TextView) findViewById(R.id.select_text);
        mCheckAll = (CheckBox) listHeaderView.findViewById(R.id.select_check);
        mCheckAll.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (!mCheckAll.isChecked()) {
                    mAdapter.setAllItemCheckedAndNotify(false);
                    mCheckAllText.setText(R.string.select_all);
                    mMenuDone.setEnabled(false);
                } else {
                    mAdapter.setAllItemCheckedAndNotify(true);
                    mCheckAllText.setText(R.string.unselect_all);
                    mMenuDone.setEnabled(true);
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
        if (isMoving == true) {
            return;
        }
        if (mItems == null || mItems.size() == 0) {
            if (mFolderID == FOLDER_NO) {
                getActionBar().setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP | ActionBar.DISPLAY_SHOW_TITLE);
                getActionBar().setTitle(R.string.app_name);
                mItems = getDataManager(this).getRootNotes();
            } else {
                getActionBar().setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP |ActionBar.DISPLAY_SHOW_HOME | ActionBar.DISPLAY_SHOW_TITLE);
                mItems = getDataManager(this).getNotesFromFolder(mFolderID);
                NoteItem noteItem = getDataManager(this).getNoteItem(mFolderID);
                getActionBar().setIcon(R.drawable.ic_group_sprd);
                if (noteItem != null) {
                    getActionBar().setTitle(noteItem.getShowTitle());
                }
            }
        }

        if (mItems != null && mItems.size() != 0 && mCheckedList != null && mCheckedList.size() != 0) {
            if (mMenuDone != null) mMenuDone.setEnabled(true);
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
        if (mAdapter != null && mAdapter.getShowType() != NoteAdapter.SHOW_TYPE_MOVETOFOLDER) {
            mAdapter.setShowType(NoteAdapter.SHOW_TYPE_MOVETOFOLDER);
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.menu_notes_move, menu);
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        mMenuDone = menu.findItem(R.id.menu_notes_delete_done);
        if (mAdapter != null && mAdapter.getSelectedCount() > 0) {
            mMenuDone.setEnabled(true);
        } else {
            mMenuDone.setEnabled(false);
        }
        return super.onPrepareOptionsMenu(menu);
    }
    /*SPRD: modified for bug615805  @{*/
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mAdapter != null ){
            mAdapter.setAllItemCheckedAndNotify(false);
        }
    }
    /*}@*/
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                break;
            case R.id.menu_notes_delete_cancel:
                finish();
                return true;
            case R.id.menu_notes_delete_done:
                moveSomeNotes();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        Log.d(TAG, "onItemClick");
        ListItemView listItems = (ListItemView) view.getTag();

        /* SPRD: 448899 avoid NPE @{ */
        if (listItems == null) {
            return;
        }
        /* @} */

        listItems.mCheckView.toggle();
        mAdapter.toggleChecked(position - 1);
        switch (mAdapter.getCheckedState()) {
            case NoteAdapter.CHECKED_NONE:
                mCheckAll.setChecked(false);
                mCheckAllText.setText(R.string.select_all);
                mMenuDone.setEnabled(false);
                break;
            case NoteAdapter.CHECKED_SOME:
                mCheckAll.setChecked(false);
                mCheckAllText.setText(R.string.select_all);
                mMenuDone.setEnabled(true);
                break;
            case NoteAdapter.CHECKED_ALL:
                mCheckAll.setChecked(true);
                mCheckAllText.setText(R.string.unselect_all);
                mMenuDone.setEnabled(true);
                break;
            default:
                break;
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putInt("mFolderId", mFolderID);
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
        // SPRD: Add for 464355
        mCheckedList = null;
        outState.putIntegerArrayList("checked_list", checkedList);
        outState.putBoolean("checked_all", mCheckAll.isChecked());
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        mFolderID = savedInstanceState.getInt("mFolderId");
        mCheckedList = savedInstanceState.getIntegerArrayList("checked_list");
        Boolean isCheckAll = savedInstanceState.getBoolean("checked_all");
        mCheckAll.setChecked(isCheckAll);
        //SPRD 346211: the selection of all is canceled, after change language
        if (!mCheckAll.isChecked()) {
            mCheckAllText.setText(R.string.select_all);
        } else {
            mCheckAllText.setText(R.string.unselect_all);
        }
        /* @} */
    }

    private void moveToFolder() {
        List<NoteItem> folders = getDataManager(this).getFolders();
        if (folders.size() == 0) {
            Toast.makeText(getApplicationContext(), R.string.no_folder_found, Toast.LENGTH_SHORT).show();
            return;
        }
        String[] folderNameStr = new String[folders.size()];
        for (int i = 0; i < folders.size(); i++) {
            folderNameStr[i] = folders.get(i).content;
        }

        Bundle bundle = new Bundle();
        bundle.putStringArray(DIALOG_KEY_MOVE_TO, folderNameStr);
        bundle.putInt("tag", DIALOG_MOVE_FOLDER);
        showDialog(bundle);
    }

    private void moveOutOfFolder() {
        if (mAdapter.getSelectedCount() == 0) { // select is empty.
            Toast.makeText(getApplicationContext(), R.string.pleasechoosenote, Toast.LENGTH_LONG).show();
        }
        MoveTask moveTask = new MoveTask();
        moveTask.execute(1000, 1000);
    }

    public void moveSomeNotes() {
        if (mAdapter.getSelectedCount() == 0) { // select is empty.
            Toast.makeText(getApplicationContext(), R.string.pleasechoosenote, Toast.LENGTH_LONG).show();
            return;
        }
        if (mFolderID == FOLDER_NO) {
            moveToFolder();
        } else {
            moveOutOfFolder();
        }
    }

    private final LoaderManager.LoaderCallbacks<Cursor> mNoteLoaderListener = new LoaderCallbacks<Cursor>() {
        public CursorLoader onCreateLoader(int id, Bundle args) {
            return new CursorLoader(NoteMove.this, NoteProvider.NOTE_CONTENT_URI, null, null,
                    null, null);
        }

        public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
            if (isMoving) {
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
            return;
        }
        //SPRD: bug 600351
        if (mCursor.isClosed()){
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
                case DIALOG_MOVE_FOLDER:
                    builder.setTitle(R.string.movetoFolder);
                    String[] folderTitle = getArguments().getStringArray(NoteMove.DIALOG_KEY_MOVE_TO);
                    builder.setItems(folderTitle,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    ((NoteMove)getActivity()).doPositiveClick(which, dialog, fragmentTag);
                                }
                            });
                    break;
                case DIALOG_MOVE_PROGRESS:
                    ProgressDialog mProgressDialog = new ProgressDialog(getActivity());
                    mProgressDialog.setTitle(R.string.move);
                    mProgressDialog.setMessage(getActivity().getResources().getString(R.string.moving));
                    mProgressDialog.setCanceledOnTouchOutside(false);
                    return mProgressDialog;
                default:
                    break;
            }
            return builder.create();
        }
    }

    public void doPositiveClick (int which, DialogInterface dialog, int fragmentTag) {
        switch (fragmentTag) {
            case DIALOG_MOVE_FOLDER:
                MoveTask moveTask = new MoveTask();
                moveTask.execute(DIALOG_MOVE_FOLDER, which);
                break;
            default:
                break;
        }
    }

    void showDialog (Bundle bundle) {
        AlertDialogFragment dialogFragment = new AlertDialogFragment();
        dialogFragment.setArguments(bundle);
        if (bundle.getInt("tag") == DIALOG_MOVE_PROGRESS) {
            dialogFragment.setCancelable(false);
        }else {
            dialogFragment.setCancelable(true);
        }
        dialogFragment.show(getFragmentManager(), "" + bundle.getInt("tag"));
    }

    public void acquireWakeLock() {
        PowerManager powerManager = (PowerManager) getApplicationContext().getSystemService(Context.POWER_SERVICE);
        if (mWakeLock == null) {
            mWakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "move_notes");
            mWakeLock.acquire();
        }
    }

    public void releaseWakeLock() {
        if (mWakeLock != null) {
            mWakeLock.release();
            mWakeLock = null;
        }
    }

    private class MoveTask extends AsyncTask<Integer, Integer, Boolean> {

        @Override
        protected void onPreExecute() {
            acquireWakeLock();
            Bundle bundle = new Bundle();
            bundle.putInt("tag", DIALOG_MOVE_PROGRESS);
            showDialog(bundle);
        }

        @Override
        protected Boolean doInBackground(Integer... params) {
            isMoving = true;
            NoteDataManager dataManager = getDataManager(NoteMove.this);
            int nowNotesCount = 0;
            if (params[0] == DIALOG_MOVE_FOLDER) {
                NoteItem folderItem = dataManager.getFolders().get(params[1]);
                nowNotesCount = dataManager.getNotesFromFolder(folderItem._id).size();
                for (NoteItem item : mItems) {
                    if (item.isSelected) {
                        if (nowNotesCount >= NoteActivity.MAX_NOTES) {
                            Toast.makeText(getApplicationContext(), R.string.toast_add_fail, Toast.LENGTH_SHORT).show();
                            return false;
                        }
                        item.parentFolderId = folderItem._id;
                        item.isSelected = false;
                        dataManager.updateItem(item);
                        nowNotesCount++;
                    }
                }
            } else {
                nowNotesCount = dataManager.getRootNotes().size();
                for (NoteItem item : mItems) {
                    if (item.isSelected) {
                        if (nowNotesCount >= NoteActivity.MAX_NOTES) {
                            Toast.makeText(getApplicationContext(), R.string.toast_add_fail, Toast.LENGTH_SHORT).show();
                            return false;
                        }
                        item.parentFolderId = -1;
                        item.isSelected = false;
                        dataManager.updateItem(item);
                        nowNotesCount++;
                    }
                }
            }
            return true;
        }

        @Override
        protected void onPostExecute(Boolean result) {
            AlertDialogFragment dialogFragment = (AlertDialogFragment) getFragmentManager().findFragmentByTag("" + DIALOG_MOVE_PROGRESS);
            if (dialogFragment != null) {
                dialogFragment.dismiss();
            }
            releaseWakeLock();
            if (result == false) {
                return;
            }
            Toast.makeText(getApplicationContext(), R.string.move_success, Toast.LENGTH_SHORT).show();
            isMoving = false;
            finish();
        }
    }
}
