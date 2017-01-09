
package com.sprd.note;

import java.util.Date;
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
import android.content.Intent;
import android.content.Loader;
import android.database.Cursor;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Debug;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.text.Editable;
import android.text.InputFilter;
import android.text.TextWatcher;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.note.data.NoteDataManager;
import com.sprd.note.data.NoteItem;
import com.sprd.note.data.NoteProvider;
import com.sprd.note.utils.IntentUtils;
import com.sprd.note.utils.LogUtils;

public class NoteActivity extends BaseActivity implements OnItemClickListener,
         View.OnCreateContextMenuListener {
    private static final String TAG = "NoteActivity";

    protected ListView mlistView;
    protected NoteAdapter mAdapter;

    public static final int MAX_NOTES = 1000;
    public static final int MAX_FOLDERS = 100;
    public static final int MAX_FOLDER_NAME_NUM = 15;

    protected static final int MENU_NEW_NOTE = Menu.FIRST;
    protected static final int MENU_NEW_FOLDER = Menu.FIRST + 1;
    protected static final int MENU_DELETE = Menu.FIRST + 2;
    protected static final int MENU_MOVE_TO_FOLDER = Menu.FIRST + 3;
    protected static final int MENU_FOLDER_RENAME = Menu.FIRST + 4;
    protected static final int MENU_DELETE_ALL = Menu.FIRST + 5;
    protected static final int MENU_UPDATE_FOLDER = Menu.FIRST + 6;
    protected static final int MENU_MOVE_OUT_FOLDER = Menu.FIRST + 7;
    protected static final int MENU_SHARE = Menu.FIRST + 8;

    public static final int FOLDER_NO = -1;
    public static final int LOADER_NOTES = 1;

    // this flag indicate to display the notes in the root or one folder.
    int mFolderID = FOLDER_NO;
    protected List<NoteItem> mItems = null;

    int selectedItemID = -1;
    public int mFragmentTag;
    private NoteItem oldFolderItem;
    private AlertDialogFragment mDialogFragment;
    private static final int DIALOG_DELTE_PROGRESS = 77;
    private static Boolean isDeleting = false;
    private WakeLock mWakeLock;

    private static final boolean DEBUG = LogUtils.DEBUG;


    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.v(TAG, "OnCreate");
        setContentView(R.layout.index_page);
        /* SPRD: modify 20140425 Spreadtrum of 305470 modify actionbar title onClickListener @{ */
        getActionBar().setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP | ActionBar.DISPLAY_SHOW_TITLE);
        /* @} */
        /*SPRD: 557004 check permisson @{ */
        if(RequestPermissionsActivity.startPermissionActivity(this)){
            return;
        }
        /* @} */
        initViews();
        getLoaderManager().restartLoader(LOADER_NOTES, null, mNoteLoaderListener);
    }

    private void initViews() {
        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            actionBar.setDisplayOptions(ActionBar.DISPLAY_SHOW_TITLE);
        }
        mlistView = (ListView) this.findViewById(R.id.page_list);
        mlistView.setOnItemClickListener(this);
        mlistView.setOnCreateContextMenuListener(this);
        View emptyView = (View) findViewById(R.id.note_empty_view);
        mlistView.setEmptyView(emptyView);
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.i(TAG, "onResume");
        /*SPRD: 557004 check permisson @{ */
        if(RequestPermissionsActivity.startPermissionActivity(this)){
            return;
        }
        /* @} */
        updateDisplay();
    }

    // update listview data
    protected void updateDisplay() {
        Log.i(TAG, "updateDisplay displayFoldID-->" + mFolderID);
        if (isDeleting == true) {
            return;
        }
        clearItems();
        if (mFolderID == FOLDER_NO) { // root
            getActionBar().setDisplayOptions(ActionBar.DISPLAY_SHOW_TITLE);
            mItems = getDataManager(this).getRootFoldersAndNotes();
            getActionBar().setTitle(R.string.app_name);
            getActionBar().setHomeButtonEnabled(false);
        } else { // in folder
            getActionBar().setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP | ActionBar.DISPLAY_SHOW_TITLE);
            getActionBar().setIcon(R.drawable.ic_group_sprd);
            NoteItem noteItem = getDataManager(this).getNoteItem(mFolderID);
            if (noteItem != null) {
                getActionBar().setTitle(noteItem.getShowTitle());
            }
            mItems = getDataManager(this).getNotesFromFolder(mFolderID);
        }

        if (mAdapter == null) {
            mAdapter = new NoteAdapter(this, mItems);
            mlistView.setAdapter(mAdapter);
        } else {
            mAdapter.setListItems(mItems);
            mAdapter.notifyDataSetChanged();
        }

    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.v(TAG, "onPause");
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.v(TAG, "onStop");
    }

    @Override
    protected void onDestroy() {
        Log.v(TAG, "onDestory");
        super.onDestroy();
        clearItems();
    }

    public void clearItems() {
        if (mItems != null) {
            mItems.clear();
            mItems = null;
        }
    }

    @Override
    public void finish() {
        if (mAdapter != null && (mAdapter.getShowType() == NoteAdapter.SHOW_TYPE_DELETE ||
                mAdapter.getShowType() == NoteAdapter.SHOW_TYPE_MOVETOFOLDER)) {
            mAdapter.setShowType(NoteAdapter.SHOW_TYPE_NORMAL);
            mAdapter.setAllItemCheckedAndNotify(false);// add for bug 130292
            mAdapter.notifyDataSetChanged();
            return;
        }

        if (mFolderID != FOLDER_NO) {
            mFolderID = FOLDER_NO;
            Log.d(TAG, "finish---mFolderID = FOLDER_NO");
            updateDisplay();
            return;
        }
        super.finish();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // set new_note item ,show as ACTION_ALWAYS;
        MenuItem actionItemAdd = menu.add(Menu.NONE, MENU_NEW_NOTE, 1, R.string.new_note);
        actionItemAdd.setIcon(R.drawable.note_new_sprd);
        actionItemAdd.setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS);
        // add others menu item
        menu.add(Menu.NONE, MENU_NEW_FOLDER, 2, R.string.new_folder).setIcon(R.drawable.new_folder);
        menu.add(Menu.NONE, MENU_UPDATE_FOLDER, 2, R.string.edit_folder_title).setIcon(
                R.drawable.edit_folder_title);
        menu.add(Menu.FIRST, MENU_DELETE, 3, R.string.delete).setIcon(R.drawable.delete);
        menu.add(Menu.FIRST, MENU_DELETE_ALL, 4, R.string.delete_all).setIcon(R.drawable.delete);
        menu.add(Menu.NONE, MENU_MOVE_TO_FOLDER, 5, R.string.movetoFolder).setIcon(
                R.drawable.menu_move);
        menu.add(Menu.NONE, MENU_MOVE_OUT_FOLDER, 5, R.string.moveoutFolder).setIcon(
                R.drawable.menu_move);
        return super.onCreateOptionsMenu(menu);
    }

    public boolean onPrepareOptionsMenu(Menu menu) {
        Log.v(TAG, "onPrepareOptionsMenu menu");
        if (null == mAdapter) {
            return true;
        }
        if (mAdapter.getShowType() == NoteAdapter.SHOW_TYPE_DELETE ||
                mAdapter.getShowType() == NoteAdapter.SHOW_TYPE_MOVETOFOLDER) {
            return false;
        }

        if (mAdapter.getCount() == 0) {
            menu.setGroupVisible(Menu.FIRST, false);
        } else {
            menu.setGroupVisible(Menu.FIRST, true);
        }

        if (mFolderID == FOLDER_NO) {
            menu.findItem(MENU_UPDATE_FOLDER).setVisible(false);
            menu.findItem(MENU_MOVE_OUT_FOLDER).setVisible(false);
            menu.findItem(MENU_NEW_FOLDER).setVisible(true);

            if (mAdapter.getFolderCount() == 0 || mAdapter.getNotesCount() == 0) {
                menu.findItem(MENU_MOVE_TO_FOLDER).setVisible(false);
            } else {
                menu.findItem(MENU_MOVE_TO_FOLDER).setVisible(true);
            }

        } else {
            menu.findItem(MENU_NEW_FOLDER).setVisible(false);
            menu.findItem(MENU_MOVE_TO_FOLDER).setVisible(false);
            menu.findItem(MENU_UPDATE_FOLDER).setVisible(true);

            if (mAdapter.getCount() == 0) {
                menu.findItem(MENU_MOVE_OUT_FOLDER).setVisible(false);
            } else {
                menu.findItem(MENU_MOVE_OUT_FOLDER).setVisible(true);
            }
        }

        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                onBackPressed();
                break;
            case MENU_NEW_NOTE:
                newNote();
                break;
            case MENU_NEW_FOLDER:
                newFolder();
                break;

            case MENU_DELETE:
                Intent delIntent = new Intent(this, NoteDelete.class);
                delIntent.putExtra(NoteDelete.CURRENT_FOLDER_ID, mFolderID);
                startActivity(delIntent);
                break;

            case MENU_DELETE_ALL:
                deleteAllNotes();
                break;
            case MENU_MOVE_TO_FOLDER:
                Intent mvToIntent = new Intent(this, NoteMove.class);
                mvToIntent.putExtra(NoteDelete.CURRENT_FOLDER_ID, mFolderID);
                mvToIntent.putExtra(NoteDelete.CURRENT_AB_TITLE, getActionBar().getTitle());
                startActivity(mvToIntent);
                Log.i(TAG, "movetoFolder");
                break;

            case MENU_MOVE_OUT_FOLDER:
                Intent mvOutIntent = new Intent(this, NoteMove.class);
                mvOutIntent.putExtra(NoteDelete.CURRENT_FOLDER_ID, mFolderID);
                mvOutIntent.putExtra(NoteDelete.CURRENT_AB_TITLE, getActionBar().getTitle());
                startActivity(mvOutIntent);
                break;
            case MENU_UPDATE_FOLDER:
                selectedItemID = mFolderID;
                renameFolder();
                break;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    private void newFolder() {
        if (getDataManager(this).getFolders().size() >= MAX_FOLDERS) {
            Toast.makeText(this, R.string.toast_add_fail, Toast.LENGTH_SHORT).show();
            return;
        }
        mFragmentTag = DIALOG_NEW_FOLDER;
        showDialog(new Bundle());
    }

    private void insertFolder(String folderName) {
        List<NoteItem> folders = getDataManager(this).getFolders();
        for (NoteItem noteItem : folders) {
            if (noteItem.content.equals(folderName)) {
                Toast.makeText(NoteActivity.this, R.string.Thisfolderalreadyexists, Toast.LENGTH_SHORT).show();
                return;
            }
        }
        NoteItem item = new NoteItem(folderName, new Date().getTime(), true, -1);
        int id = getDataManager(this).insertItem(item);
        if (id > 0) {
            dismissDialog();
        }
    }

    private void deleteAllNotes() {
        mFragmentTag = DIALOG_DELTE_ALL_NOTES;
        showDialog(new Bundle());
    }

    private void moveToFolder() {
        List<NoteItem> folders = getDataManager(this).getFolders();
        if (folders.size() == 0) {
            Toast.makeText(getApplicationContext(), R.string.no_folder_found, Toast.LENGTH_LONG).show();
            return;
        }

        String[] folderNameStr = new String[folders.size()];
        for (int i = 0; i < folders.size(); i++) {
            folderNameStr[i] = folders.get(i).content;
        }
        Bundle bundle = new Bundle();
        bundle.putStringArray(NoteMove.DIALOG_KEY_MOVE_TO, folderNameStr);
        mFragmentTag = NoteMove.DIALOG_MOVE_FOLDER;
        showDialog(bundle);
    }

    private void moveOutOfFolder() {
        NoteDataManager dataManager = getDataManager(NoteActivity.this);

        if (mAdapter.getSelectedCount() == 0) { // select is empty.
            Toast.makeText(getApplicationContext(), R.string.pleasechoosenote, Toast.LENGTH_LONG).show();
        }
        for (NoteItem item : mItems) {
            if (item.isSelected) {
                item.parentFolderId = -1;
                item.isSelected = false;
                dataManager.updateItem(item);
            }
        }

        mAdapter.setShowType(NoteAdapter.SHOW_TYPE_NORMAL);
        updateDisplay();
    }

    private void renameFolder() {
        mFragmentTag = DIALOG_RENAME_FOLDER;
        oldFolderItem = getDataManager(NoteActivity.this).getNoteItem(selectedItemID);
        Bundle bundle = new Bundle();
        if (oldFolderItem != null) {
            bundle.putString("old_folder_name", oldFolderItem.content);
        } else {
            bundle.putString("old_folder_name", "");
        }
        showDialog(bundle);
    }

    private void newNote() {
        if (getNotesCount() >=  MAX_NOTES) {
            Toast.makeText(this, R.string.toast_add_fail, Toast.LENGTH_SHORT).show();
            return;
        }
        Intent intent = new Intent();
        intent.setClass(NoteActivity.this, NoteEditor.class);
        intent.putExtra(NoteEditor.OPEN_TYPE, NoteEditor.TYPE_NEW_NOTE);
        if (mFolderID != FOLDER_NO) { // start intent in the folder.
            intent.putExtra(NoteEditor.NOTE_FOLDER_ID, mFolderID);
        }
        startActivity(intent);
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        if (DEBUG) {
            Log.v(TAG, "list click position : " + position);
        }

        // in normal mode
        NoteItem clickedItem = mItems.get(position);
        if (clickedItem == null) {
            return;
        }
        if (clickedItem.isFileFolder) {
            oldFolderItem = clickedItem;
            mFolderID = oldFolderItem._id;
            updateDisplay();
            NoteActivity.this.invalidateOptionsMenu();// must invalidate after update adapter
        } else {
            Intent intent = new Intent();
            intent.setClass(this, NoteEditor.class);
            intent.putExtra(NoteEditor.ID, clickedItem._id);
            intent.putExtra(NoteEditor.OPEN_TYPE, NoteEditor.TYPE_EDIT_NOTE);
            startActivity(intent);
        }
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
        if (mAdapter.getShowType() == NoteAdapter.SHOW_TYPE_DELETE
                || mAdapter.getShowType() == NoteAdapter.SHOW_TYPE_MOVETOFOLDER) {
            return;
        }

        AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) menuInfo;
        NoteItem contextNoteItem = mItems.get(info.position);
        selectedItemID = contextNoteItem._id;

        menu.setHeaderTitle(contextNoteItem.getShortTitle());

        menu.add(0, MENU_DELETE, 0, R.string.delete);
        if (contextNoteItem.isFileFolder) {
            menu.add(0, MENU_FOLDER_RENAME, 0, R.string.edit_folder_title);
            return;
        }

        if (mFolderID == FOLDER_NO) {
            menu.add(0, MENU_MOVE_TO_FOLDER, 1, R.string.movetoFolder);
        } else {
            menu.add(0, MENU_MOVE_OUT_FOLDER, 1, R.string.moveoutFolder);
        }
        menu.add(0, MENU_SHARE, 3, R.string.share);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        AdapterContextMenuInfo menuInfo = (AdapterContextMenuInfo) item.getMenuInfo();
        switch (item.getItemId()) {
            case MENU_DELETE:
                mFragmentTag = DIALOG_DELETE;
                showDialog(new Bundle());
                return true;

            case MENU_MOVE_TO_FOLDER: {
                //SPRD:bug 532227 move note into folder function error
                //mAdapter.setItemChecked(menuInfo.position, true);
                moveToFolder();
                return true;
            }
            case MENU_FOLDER_RENAME:
                renameFolder();
                return true;
            case MENU_MOVE_OUT_FOLDER:
                mAdapter.setItemChecked(menuInfo.position, true);
                moveOutOfFolder();
                return true;
            case MENU_SHARE:
                NoteItem noteItem = mItems.get(menuInfo.position);
                IntentUtils.sendSharedIntent(this, noteItem);
                return true;
        }

        return false;
    }

    public static final int DIALOG_DELETE = 0;
    public static final int DIALOG_NEW_FOLDER = 1;
    public static final int DIALOG_DELTE_ALL_NOTES = 2;
    public static final int DIALOG_RENAME_FOLDER = 3;

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        selectedItemID = savedInstanceState.getInt("selectedItemID");
        mFolderID = savedInstanceState.getInt("mFolderId");
        mFragmentTag = savedInstanceState.getInt("fragment_tag");
        mDialogFragment = (AlertDialogFragment) getFragmentManager().findFragmentByTag("" + mFragmentTag);
        oldFolderItem = getDataManager(NoteActivity.this).getNoteItem(selectedItemID);
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putInt("selectedItemID", selectedItemID);
        outState.putInt("mFolderId", mFolderID);
        outState.putInt("fragment_tag", mFragmentTag);
    }

    private final LoaderManager.LoaderCallbacks<Cursor> mNoteLoaderListener = new LoaderCallbacks<Cursor>() {
        public CursorLoader onCreateLoader(int id, Bundle args) {
            // use given URI to new a CursorLoader sample and return
            Log.d(TAG, "LoaderCallbacks");
            return new CursorLoader(NoteActivity.this, NoteProvider.NOTE_CONTENT_URI, null, null,
                    null, null);
        }

        public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
            Log.d(TAG, "onLoadFinished");
            if(data != null && data.isClosed()){
                data = null;
            }
            bindCursor(data);
            updateDisplay();
        }

        public void onLoaderReset(Loader<Cursor> loader) {
        }
    };

    private void bindCursor(Cursor cursor) {
        if (cursor == null) {
            Log.d(TAG, "cursor == null");
            return;
        }
        getDataManager(this).setCursor(cursor);
    }

    private int getNotesCount() {
        int notesOrFolderCount = 0;
        if (mFolderID == FOLDER_NO) {
            notesOrFolderCount = getDataManager(this).getRootNotes().size();
        }else {
            notesOrFolderCount = getDataManager(this).getNotesFromFolder(mFolderID).size();
        }
        return notesOrFolderCount;
    }

    public static class AlertDialogFragment extends DialogFragment {
        private TextView mEditorCharCountView;
        private EditText folderNameEditText;
        private Bundle mBundle;
        /* SPRD: modify 20131203 Spreadtrum of 245676 maxLength toast always display when it is end maxLen @{ */
        private Toast toast = null;
        private int TOAST_SHOW_MAX_TIME = 3000;

        private AlertDialog dialog;

        class TextWatch implements TextWatcher{
            public void beforeTextChanged(CharSequence s, int start, int count,
                    int after) {
            }

            public void onTextChanged(CharSequence s, int start, int before,
                    int count) {
            }

            public void afterTextChanged(Editable s) {
                int length = s.toString().length();
                mEditorCharCountView.setText("" + length + "/" + MAX_FOLDER_NAME_NUM);
                if (length > MAX_FOLDER_NAME_NUM ) {
                    String toastMessage = getActivity().getString(R.string.tilte_input_more, MAX_FOLDER_NAME_NUM);
                    if (toast == null) {
                        toast = Toast.makeText(getActivity(), toastMessage, Toast.LENGTH_SHORT);
                    } else {
                        toast.setDuration(TOAST_SHOW_MAX_TIME);
                    }
                    s.delete(MAX_FOLDER_NAME_NUM, length);
                    toast.show();
                }
            }
        }
        /* @} */
        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            mBundle = getArguments();
            final int fragmentTag = mBundle.getInt("tag");
            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            switch (fragmentTag) {
                case DIALOG_DELETE:
                    builder.setNegativeButton(R.string.Cancel, null);
                    builder.setTitle(R.string.prompt);
                    builder.setMessage(R.string.delete_selected_items);
                    builder.setPositiveButton(R.string.Ok,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                        ((NoteActivity)getActivity()).doPositiveClick(which, dialog, mBundle);
                                }
                            });
                    break;
                case DIALOG_DELTE_ALL_NOTES:
                    builder.setNegativeButton(R.string.Cancel, null);
                    builder.setTitle(R.string.prompt);
                    builder.setMessage(R.string.delete_all_items);
                    builder.setPositiveButton(R.string.Ok,new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    ((NoteActivity)getActivity()).doPositiveClick(which, dialog, mBundle);
                                }
                            });
                    break;
                case DIALOG_DELTE_PROGRESS:
                    ProgressDialog mProgressDialog = new ProgressDialog(getActivity());
                    mProgressDialog.setTitle(R.string.delete);
                    mProgressDialog.setMessage(getActivity().getResources().getString(R.string.Deleting));
                    mProgressDialog.setCanceledOnTouchOutside(false);
                    return mProgressDialog;
                case DIALOG_NEW_FOLDER:
                    View customTitleView = LayoutInflater.from(getActivity()).inflate(R.layout.dialog_title_layout, null);
                    TextView titleText = (TextView) customTitleView.findViewById(R.id.note_editor_title);
                    titleText.setText(R.string.new_folder);
                    builder.setCustomTitle(customTitleView);
                    mEditorCharCountView = (TextView) customTitleView.findViewById(R.id.char_count);
                    View layout = LayoutInflater.from(getActivity()).inflate(R.layout.dialog_layout_new_folder, null);
                    builder.setView(layout);
                    folderNameEditText = (EditText) layout.findViewById(R.id.et_dialog_new_folder);

                    builder.setPositiveButton(R.string.Ok, new DialogInterface.OnClickListener() {

                        public void onClick(DialogInterface dialog, int which) {
                            mBundle.putString("new_name", folderNameEditText.getText().toString());
                            ((NoteActivity)getActivity()).doOkButtonClick(mBundle);
                        }
                    });
                    builder.setNegativeButton(R.string.Cancel, new DialogInterface.OnClickListener() {

                        public void onClick(DialogInterface dialog, int which) {
                            ((NoteActivity)getActivity()).doCancelButtonClick(fragmentTag);
                        }
                    });
                    folderNameEditText.setText("");
                    mEditorCharCountView.setText(folderNameEditText.getText().toString().length() +  "/" + MAX_FOLDER_NAME_NUM);
                    dialog = builder.create();
                    /* SPRD: modify 20140428 Spreadtrum of 305693 maxLength toast always display when it is end maxLen @{ */
                    folderNameEditText.addTextChangedListener(new TextWatcher() {
                        Toast toast;
                        @Override
                        public void onTextChanged(CharSequence s, int start, int before, int count) {

                        }

                        @Override
                        public void beforeTextChanged(CharSequence s, int start, int count, int after) {

                        }

                        @Override
                        public void afterTextChanged(Editable s) {
                            int length = s.toString().length();
                            mEditorCharCountView.setText(length + "/" + MAX_FOLDER_NAME_NUM);
                            if (length > MAX_FOLDER_NAME_NUM) {
                                String toastMessage = getActivity().getString(R.string.tilte_input_more, MAX_FOLDER_NAME_NUM);
                                if (toast == null) {
                                    toast = Toast.makeText(getActivity(), toastMessage, Toast.LENGTH_SHORT);
                                }
                                toast.show();
                                s.delete(MAX_FOLDER_NAME_NUM, length);
                            }
                        }
                    });
                    /* @} */
                    return dialog;
                case DIALOG_RENAME_FOLDER:
                    View customRenameTitleView = LayoutInflater.from(getActivity()).inflate(R.layout.dialog_title_layout, null);
                    TextView renameTitle = (TextView) customRenameTitleView.findViewById(R.id.note_editor_title);
                    renameTitle.setText(R.string.edit_folder_title);
                    builder.setCustomTitle(customRenameTitleView);
                    mEditorCharCountView = (TextView) customRenameTitleView.findViewById(R.id.char_count);
                    View renameLayout = LayoutInflater.from(getActivity()).inflate(R.layout.dialog_layout_new_folder, null);
                    builder.setView(renameLayout);
                    folderNameEditText = (EditText) renameLayout.findViewById(R.id.et_dialog_new_folder);
                    builder.setPositiveButton(R.string.Ok, new DialogInterface.OnClickListener() {

                        public void onClick(DialogInterface dialog, int which) {
                            mBundle.putString("new_name", folderNameEditText.getText().toString());
                            ((NoteActivity)getActivity()).doOkButtonClick(mBundle);
                        }
                    });
                    builder.setNegativeButton(R.string.Cancel, new DialogInterface.OnClickListener() {

                        public void onClick(DialogInterface dialog, int which) {
                            ((NoteActivity)getActivity()).doCancelButtonClick(fragmentTag);
                        }
                    });
                    dialog = builder.create();
                    String oldFolderName = mBundle.getString("old_folder_name");
                    folderNameEditText.setText(oldFolderName);
                    folderNameEditText.setSelection(oldFolderName.length());
                    mEditorCharCountView.setText(oldFolderName.length() +  "/" + MAX_FOLDER_NAME_NUM);
                    /* SPRD: modify 20131203 Spreadtrum of 245676 maxLength toast always display when it is end maxLen @{ */
                    if (oldFolderName.length() > MAX_FOLDER_NAME_NUM) {//FirstTime in Dial
                        String toastMessage = getActivity().getString(R.string.tilte_input_more, MAX_FOLDER_NAME_NUM);
                        if (toast == null) {
                            toast = Toast.makeText(getActivity(), toastMessage, Toast.LENGTH_SHORT);
                        } else {
                            toast.setDuration(TOAST_SHOW_MAX_TIME);
                        }
                        toast.show();
                    }

                    TextWatcher modifyTextWatch = new TextWatch();
                    folderNameEditText.addTextChangedListener(modifyTextWatch);
                    /* @} */
                    return dialog;
                case NoteMove.DIALOG_MOVE_FOLDER:// move in main interface
                    builder.setTitle(R.string.movetoFolder);
                    String[] folderTitle = null;
                    folderTitle = mBundle.getStringArray(NoteMove.DIALOG_KEY_MOVE_TO);
                    builder.setItems(folderTitle,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    ((NoteActivity)getActivity()).doPositiveClick(which, dialog, mBundle);
                                }
                            });
                    break;
                default:
                    break;
            }
           return builder.create();
        }

        @Override
        public void onDismiss(DialogInterface dialog) {
            /* SPRD: 441378 check getActivity(), avoid NPE @{ */
            if (mBundle.getInt("tag") == NoteMove.DIALOG_MOVE_FOLDER && getActivity() != null) {
                /* @} */
                ((NoteActivity)getActivity()).mAdapter.setAllItemCheckedAndNotify(false);
            }
            super.onDismiss(dialog);
        }
    }

    public void doPositiveClick (int which, DialogInterface dialog, Bundle bundle) {
        switch (bundle.getInt("tag")) {
            case DIALOG_DELETE:
                final NoteItem noteItem = getDataManager(NoteActivity.this).getNoteItem(selectedItemID);
                getDataManager(NoteActivity.this).deleteNoteItem(noteItem);
                Toast.makeText(getApplicationContext(), R.string.delete_success, Toast.LENGTH_SHORT).show();
                updateDisplay();
                break;
            case DIALOG_DELTE_ALL_NOTES:
                DeleteAllTask deleteAllTask = new DeleteAllTask();
                deleteAllTask.execute();
                break;
            case NoteMove.DIALOG_MOVE_FOLDER:
                NoteDataManager dataManager = getDataManager(NoteActivity.this);
                if (dataManager.getFolders().size() == 0) { // list is empty.
                    Toast.makeText(getApplicationContext(), R.string.no_folder_found, Toast.LENGTH_SHORT).show();
                    return;
                }

                if (mAdapter.getSelectedCount() == 0) { // select is empty.
                    /* SPRD: Add for 457423 @{ */
                    // Toast.makeText(getApplicationContext(), R.string.pleasechoosenote, Toast.LENGTH_SHORT).show();
                    // return;
                    final NoteItem selectedItem = getDataManager(NoteActivity.this).getNoteItem(selectedItemID);
                    if(selectedItem != null){
                        selectedItem.isSelected = true;
                    }else{
                        Toast.makeText(getApplicationContext(), R.string.pleasechoosenote, Toast.LENGTH_SHORT).show();
                        return;
                    }
                    /* @} */
                }
                NoteItem folderItem = dataManager.getFolders().get(which);
                int  nowNotesCount = dataManager.getNotesFromFolder(folderItem._id).size();
                for (NoteItem item : mItems) {
                    if (item.isSelected) {
                        if (nowNotesCount >= NoteActivity.MAX_NOTES) {
                            Toast.makeText(getApplicationContext(), R.string.toast_add_fail, Toast.LENGTH_SHORT).show();
                            break;
                        }
                        item.parentFolderId = folderItem._id;
                        item.isSelected = false;
                        dataManager.updateItem(item);
                        nowNotesCount++;
                    }
                }
                updateDisplay();
                break;
            default:
                break;
        }

    }

    public void doCancelButtonClick (int fragmentTag) {
        switch (fragmentTag) {
            case DIALOG_NEW_FOLDER:
                dismissDialog();
                break;
            case DIALOG_RENAME_FOLDER:
                dismissDialog();
                break;
            default:
                break;
        }

    }

    public void doOkButtonClick (Bundle bundle) {
        switch (bundle.getInt("tag")) {
            case DIALOG_NEW_FOLDER:
                String newFolderName = bundle.getString("new_name").trim();
                if (newFolderName.length() > 0) {
                    insertFolder(newFolderName);
                    NoteActivity.this.updateDisplay();
                } else {
                    Toast.makeText(NoteActivity.this, R.string.folder_add_fail, Toast.LENGTH_SHORT).show();
                }
                break;
            case DIALOG_RENAME_FOLDER:
                String newName = bundle.getString("new_name").trim();
                if (newName.length() == 0) {
                    Toast.makeText(NoteActivity.this, R.string.folder_add_fail, Toast.LENGTH_SHORT).show();
                    break;
                }
                if (oldFolderItem == null) {
                    break;
                }
                if (newName.equals(oldFolderItem.content)) {
                    Toast.makeText(NoteActivity.this, R.string.not_changed, Toast.LENGTH_SHORT).show();
                    dismissDialog();
                    break;
                }
                List<NoteItem> folders = getDataManager(NoteActivity.this).getFolders();
                Boolean fail = false;
                for (NoteItem item : folders) {
                    if (newName.equals(item.content)) {
                        Toast.makeText(NoteActivity.this, R.string.Thisfolderalreadyexists, Toast.LENGTH_SHORT).show();
                        fail = true;
                        break;
                    }
                }
                if (fail == true) {
                    break;
                }
                dismissDialog();
                oldFolderItem.content = newName;
                oldFolderItem.date = new Date().getTime();
                getDataManager(NoteActivity.this).updateItem(oldFolderItem);
                updateDisplay();
                break;
            default:
                break;
        }
    }

    void showDialog (Bundle bundle) {
        bundle.putInt("tag", mFragmentTag);
        bundle.putInt("folder_id", mFolderID);
        mDialogFragment = new AlertDialogFragment();
        mDialogFragment.setArguments(bundle);
        if (mFragmentTag == DIALOG_DELTE_PROGRESS) {
            mDialogFragment.setCancelable(false);
        }else {
            mDialogFragment.setCancelable(true);
        }
        //SPRD : 524023 can not show dialog after onSavedInstanceState
        if (!isFinishing() && isResumed()) {
            mDialogFragment.show(getFragmentManager(), "" + mFragmentTag);
        }
    }

    void dismissDialog() {
        if (mDialogFragment != null) {
            /* SPRD: 456511 Can not perform this action after onSaveInstanceState @{ */
            mDialogFragment.dismissAllowingStateLoss();
            /* @} */
        }
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


    private class DeleteAllTask extends AsyncTask<Integer, Integer, Boolean> {

        @Override
        protected void onPreExecute() {
            acquireWakeLock();
            mFragmentTag = DIALOG_DELTE_PROGRESS;
            showDialog(new Bundle());
        }

        @Override
        protected Boolean doInBackground(Integer... params) {
            isDeleting = true;
            List<NoteItem> items = getDataManager(NoteActivity.this).getNotesFromFolder(mFolderID);
            for (NoteItem item : items) {
                getDataManager(NoteActivity.this).deleteNoteItem(item);
            }
            return true;
        }

        @Override
        protected void onPostExecute(Boolean result) {
            AlertDialogFragment dialogFragment = (AlertDialogFragment) getFragmentManager().findFragmentByTag("" + mFragmentTag);
            if (dialogFragment != null) {
                dialogFragment.dismissAllowingStateLoss();
            }
            releaseWakeLock();
            Toast.makeText(getApplicationContext(), R.string.delete_success, Toast.LENGTH_SHORT).show();
            isDeleting = false;
            updateDisplay();
        }

    }
}
