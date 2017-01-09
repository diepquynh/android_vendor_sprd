
package com.sprd.note;

import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.List;

import android.app.ActionBar;
import android.app.ActionBar.LayoutParams;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.DashPathEffect;
import android.graphics.Paint;
import android.graphics.PathEffect;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.Debug;
import android.text.Editable;
import android.text.InputFilter;
import android.text.Layout;
import android.text.Selection;
import android.text.Spanned;
import android.text.TextWatcher;
import android.text.style.URLSpan;
import android.util.AttributeSet;
import android.net.Uri;
import android.util.Log;
import android.view.ContextMenu;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
/* SPRD: 442035 import @{ */
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.text.TextUtils;
/* @} */

import com.sprd.note.data.MyLinkify;
import com.sprd.note.data.NoteDataManager;
import com.sprd.note.data.NoteItem;
import com.sprd.note.utils.IntentUtils;
import com.sprd.note.utils.LogUtils;

public class NoteEditor extends BaseActivity{
    private static final int MAX_CONTENT_LEN = 1000;//char len
    public static final int MAX_FOLDER_NAME_NUM = 15;
    public static final String TAG = "NoteEditor";
    public static final String OPEN_TYPE = "open_type";
    public static final String ID = "id";
    public static final String NOTE_FOLDER_ID = "folder_id";
    /*SPRD: add for 631901 @{*/
    private static final String TEL_URI = "tel:";
    private static final String EMAIL_URI = "mailto:";
    private static final String WEB_HTTP_URI = "http:";
    private static final String WEB_HTTPS_URI = "https:";
    /*}@*/

    public static final int TYPE_NEW_NOTE = 0;
    public static final int TYPE_EDIT_NOTE = 1;

    public static final int DIALOG_DELETE = 0;
    public static final int Dialog_CANCLE = 1;
    public static final int DIALOG_EDIT_TITLE = 2;

    public static final int TITLE_MAX_LEN = 15;

    private static final int INSERT_ITEM_FAIL = -1;

    private static final int MENU_DELETE = Menu.FIRST;
    private static final int MENU_SAVE = Menu.FIRST + 2;
    private static final int MENU_CANCEL = Menu.FIRST + 3;
    private static final int MENU_SHARE = Menu.FIRST + 4;
    private static final int MENU_EDIT_TITLE = Menu.FIRST + 5;

    /* SPRD: modify/add 20131122 Spreadtrum of 241526 maxLength toast display when it is not end 1000 @{ */
    private Toast textLenghtToast = null;
    /* @} */
    private Intent mIntent;
    private int mOpenType;

    private TextView mDateTimeTextView;
    private EditText mNowContent;
    private TextView mNoteTitleView;
    private TextView mRemainNumTextView;// used to display characters left
    private String mTemporaryContent ;
    /*SPRD: same notes error bug611835  {@*/
    private static final String RECORDED = "recorded";
    private SharedPreferences mRecordedPref;
    private boolean isSaved = false;
    private int mId = INSERT_ITEM_FAIL;
    /*@}*/
    private String mOldContent;
    private boolean needSave = true;
    Date date;
    java.text.DateFormat dateFormat;

    int num = 0;
    NoteDataManager mDataManager = null;
    private NoteItem mNoteItem = null;
    private int mParentFolderID = -1;
    private String mNoteTitle = "";

    private static final boolean DEBUG = LogUtils.DEBUG;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mIntent = getIntent();
        if (DEBUG) {
            Log.v(TAG, "OnCreate intent-->" + getIntent() + "  data-->" + (mIntent == null ? "null" : mIntent.getExtras()));
        }
        setContentView(R.layout.note_editor);
        initData();
        initViews();
        mRecordedPref = PreferenceManager.getDefaultSharedPreferences(this);
    }

    private void initData() {
        Intent intent = getIntent();
        mOpenType = intent.getIntExtra(OPEN_TYPE, -1);
        int noteID = intent.getIntExtra(ID, -1);
        mParentFolderID = intent.getIntExtra(NOTE_FOLDER_ID, -1);

        switch (mOpenType) {
            case TYPE_EDIT_NOTE:
                mNoteItem = getDataManager(this).getNoteItem(noteID);
                if (null == mNoteItem) {
                    if (DEBUG) {
                        Log.d(TAG, "initData --> noteItem is null. Type: " + mOpenType + "ID: " + noteID + ", folderID" + mParentFolderID);
                    }
                    return;
                }
                break;

            case TYPE_NEW_NOTE:
                mOldContent = "";
                mNoteItem = new NoteItem(mParentFolderID);
                break;

            default:
                break;
        }
        mNoteTitle = mNoteItem.getTitle();
        num = mNoteItem.getContent().length();
        mOldContent = mNoteItem.content;
    }

    /*SPRD: add for 631901 @{*/
    private boolean hasExportIntentHandler(String uri) {
        if (uri != null && uri.length() > 0){
            Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(uri));
            final PackageManager pm = this.getPackageManager();
            final List<ResolveInfo> rl = pm.queryIntentActivities(intent,
                                            PackageManager.MATCH_DEFAULT_ONLY);
            if (rl.size()>0){
               return true;
            }
       }
       return false;
    }

    private int linkMask() {
        int mask = 0;
        if (hasExportIntentHandler(TEL_URI)){
            mask = MyLinkify.PHONE_NUMBERS;
        }
        if (hasExportIntentHandler(EMAIL_URI)){
           mask |= MyLinkify.EMAIL_ADDRESSES;
        }
        if (hasExportIntentHandler(WEB_HTTP_URI) ||hasExportIntentHandler(WEB_HTTPS_URI)){
           mask |= MyLinkify.WEB_URLS;
        }
        Log.i(TAG, "Link mask: "+mask);
        return mask;
    }
    /*}@*/

    private void initViews() {
        mRemainNumTextView = (TextView) findViewById(R.id.remain_num);
        mDateTimeTextView = (TextView) findViewById(R.id.tv_note_date_time);
        mNowContent = (EditText) findViewById(R.id.et_content);
        setActionBar();
        mNowContent.setAutoLinkMask(linkMask());
        mNowContent.setVisibility(View.VISIBLE);
        mNowContent.setCursorVisible(true);
        /* SPRD: modify/add 20131122 Spreadtrum of 241526 maxLength toast display when it is not end 1000 @{ */
        Textwatch mNowContentWath = new Textwatch(true, MAX_CONTENT_LEN);
        mNowContent.addTextChangedListener(mNowContentWath);
        /* @} */
        mNoteTitleView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                /* SPRD: modify 20140425 Spreadtrum of 301384 com.sprd.note show IllegalStateException @{ */
                if(v.hasWindowFocus()) {
                    editTitle();
                }
                /* @} */
            }
        });
    }
    /*SPRD: same notes error bug611835  {@*/
    private void putPrefValues(String key, String value){
        if (mRecordedPref != null) {
            SharedPreferences.Editor editor = mRecordedPref.edit();
            editor.putString(key, value);
            editor.commit();
        }
    }

    private String getPrefValues(String key, String def){
        if (mRecordedPref != null) {
           return mRecordedPref.getString(key, def);
        }
        return def;
    }
    /*@*/

    //SPRD ADD 346623: the screen flashed when the menu button of edit-title was clicked
    private void setActionBar() {
        LayoutInflater inflator = (LayoutInflater)this.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View layout = inflator.inflate(R.layout.actionbar_layout, null);
        mNoteTitleView = (TextView) layout.findViewById(R.id.note_title);
        ActionBar.LayoutParams params = new ActionBar.LayoutParams(LayoutParams.MATCH_PARENT
                , LayoutParams.MATCH_PARENT);
        ActionBar actionBar = getActionBar();
        actionBar.setDisplayShowCustomEnabled(true);
        actionBar.setCustomView(layout,params);
    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.v(TAG, "onStart");
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.v(TAG, "onResume");
        /*SPRD: same notes error bug611835  {@*/
        if (isSaved && mId != INSERT_ITEM_FAIL) {
           mOpenType = 1;
           mNoteItem = getDataManager(this).getNoteItem(mId);
        }
        /*@*/
        if (mNoteItem != null) {
            updateDisplay();
        }

        /* SPRD: 442035 register mHomeKeyEventreceiver @{ */
        registerReceiver(mHomeKeyEventReceiver,
            new IntentFilter(Intent.ACTION_CLOSE_SYSTEM_DIALOGS));
        /* @} */
    }

    private void updateDisplay() {
        mRemainNumTextView.setText(num + "" + "/" + MAX_CONTENT_LEN);
        mDateTimeTextView.setText(mNoteItem.getDate(this) + "   " + mNoteItem.getTime(this));
        Log.d(TAG, "mTemporaryContent=" + mTemporaryContent);
        if (mTemporaryContent != null) {
            mNowContent.setText(mTemporaryContent);
        }else {
            mNowContent.setText(mNoteItem.getContent());
        }
        /* SPRD 597764 identify information content for before and after the url of the character @{ */
        MyLinkify.addLinks(mNowContent, linkMask());
        /* @} */
        int selection_end = (mNowContent.getText().toString()).length();// cursor position
        mNowContent.setSelection(selection_end);
        mNoteTitleView.setText(mNoteTitle);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        menu.add(menu.NONE, MENU_EDIT_TITLE, 1, R.string.edit_title).setIcon(
                R.drawable.ic_menu_edit);
        menu.add(Menu.NONE, MENU_SAVE, 4, R.string.save).setIcon(R.drawable.save);
        menu.add(menu.NONE, MENU_SHARE, 5, R.string.share)
                .setIcon(android.R.drawable.ic_menu_share);
        menu.add(Menu.NONE, MENU_DELETE, 6, R.string.delete).setIcon(R.drawable.delete);
        menu.add(Menu.NONE, MENU_CANCEL, 7, R.string.Cancel).setIcon(R.drawable.delete);
        return super.onCreateOptionsMenu(menu);
    }

    public boolean onPrepareOptionsMenu(Menu menu) {
        switch (mOpenType) {
            case TYPE_NEW_NOTE:
                menu.findItem(MENU_CANCEL).setVisible(true);
                menu.findItem(MENU_DELETE).setVisible(false);
                break;
            case TYPE_EDIT_NOTE:
                menu.findItem(MENU_CANCEL).setVisible(false);
                menu.findItem(MENU_DELETE).setVisible(true);
                break;
            default:
                break;
        }
        return super.onPrepareOptionsMenu(menu);
    };

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (DEBUG) {
            Log.v(TAG, "onOptionsItemSelected: item.getItemId-->" + item.getItemId());
        }
        Bundle bundle = new Bundle();
        switch (item.getItemId()) {
            case MENU_DELETE:
                bundle.putInt("tag", DIALOG_DELETE);
                showDialog(bundle);
                break;
            case MENU_CANCEL:
                bundle.putInt("tag", Dialog_CANCLE);
                showDialog(bundle);
                break;
            case MENU_SAVE:
                saveNote();
                finish();
                break;
            case MENU_SHARE:
                if (null != mNoteItem && isNoteNotEmptyAndToast(R.string.input_content)) {
                    saveNote();
                    IntentUtils.sendSharedIntent(this, mNoteItem);
                } else {
                    Log.d(TAG, "noteItem is null");
                }
                break;
            case MENU_EDIT_TITLE:
                editTitle();
                break;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    private void editTitle() {
        Bundle bundle = new Bundle();
        bundle.putInt("tag", DIALOG_EDIT_TITLE);
        bundle.putString("now_title", getNowTitle2Edit());
        showDialog(bundle);
    }

    private String getNowTitle2Edit() {//edit title
        if (mNoteTitle == null) {
            mNoteTitle = "";
        }
        return mNoteTitle;
    }

    private boolean isNoteNotEmptyAndToast(int toastResID) {
        if (isNoteEmpty()) {
            Toast.makeText(NoteEditor.this, toastResID, Toast.LENGTH_SHORT).show();
            return false;
        } else {
            return true;
        }
    }

    //tile and content are empty will return true
    private boolean isNoteEmpty() {
        return mNoteTitleView.getText().toString().trim().isEmpty()
                && mNowContent.getText().toString().trim().isEmpty();
    }

    /* SPRD: modify/add 20131122 Spreadtrum of 241526 maxLength toast display when it is not end 1000  @{ */
    class Textwatch implements TextWatcher {
        boolean mMaxTip = true;
        int mMaxLen = 0;

        int mChangeStart = 0;
        int mChangeCount = 0;
        Textwatch(boolean maxTip, int maxLen) {
            this.mMaxTip = maxTip;
            this.mMaxLen = maxLen;
        }

        public void beforeTextChanged(CharSequence s, int start, int count,
                int after) {
        }

        public void onTextChanged(CharSequence s, int start, int before,
                int count) {
            mChangeStart = start;
            mChangeCount = count;
        }

        public void afterTextChanged(Editable s) {
            int number = s.length();
            mRemainNumTextView.setText("" + number + "/" + MAX_CONTENT_LEN);
            if (number > mMaxLen && mMaxTip) {
                mMaxTip = false;
                textLenghtToast = showMessage(textLenghtToast, MAX_CONTENT_LEN);
                int deleteCount = number - mMaxLen;
                s.delete(mChangeStart + mChangeCount - deleteCount,
                        mChangeStart + mChangeCount);
            } else {
                mMaxTip = true;
            }
        }
    };
    /* @} */

    @Override
    protected void onNewIntent(Intent intent) {
        Log.i(TAG, "onNewIntent() intent-->" + intent + " extars-->"
                + (intent == null ? "null" : intent.getExtras().toString()));
        setIntent(intent);
    }

    @Override
    protected void onPause() {
        mTemporaryContent = mNowContent.getText().toString();
        super.onPause();
        Log.i(TAG, "onPause() needSave-->" + needSave);

        /* SPRD: 442035 unregister mHomeKeyReceiver @{ */
        unregisterReceiver(mHomeKeyEventReceiver);
        /* @} */
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.i(TAG, "OnStop");
        /* SPRD: 603328 @{ */
        //finish();
        if(needSave){
            saveNote();
        }
        /*SPRD: same notes error bug611835  {@*/
        putPrefValues(ID, Integer.toString(mId));
        /*@*/
        /* @} */
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        outState.putString("now_content", mNowContent.getText().toString());
        outState.putString("now_title", getNowTitle2Edit());
        /*SPRD: same notes error bug611835  {@*/
        putPrefValues(RECORDED, "true");
        /*@*/
        super.onSaveInstanceState(outState);
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        mTemporaryContent = savedInstanceState.getString("now_content");
        mNoteTitle = savedInstanceState.getString("now_title");
        /*SPRD: same notes error bug611835  {@*/
        isSaved = Boolean.parseBoolean(getPrefValues(RECORDED, "false"));
        mId = Integer.parseInt(getPrefValues(ID, "-1"));
        putPrefValues(RECORDED, "false");
        /*@*/
    }

    @Override
    public void finish() {
        if (needSave) {
            saveNote();
        }
        Log.i(TAG, "finish");
        super.finish();
    }

    private void saveNote() {
        String content = mNowContent.getText().toString();
        if (DEBUG) {
            Log.v(TAG, "openType =" + mOpenType + " content-->" + content);
        }
        switch (mOpenType) {
            case TYPE_NEW_NOTE:
                insertNote(content);
                break;
            case TYPE_EDIT_NOTE:
                updateNote(content);
                break;
            default:
                break;
        }
    }

    private boolean isTitleValid() {
        switch (mOpenType) {
            case TYPE_NEW_NOTE:
                if (mNoteTitle != null && !mNoteTitle.equals("")) {
                    return true;
                }
                break;
            case TYPE_EDIT_NOTE:
                if (mNoteTitle != null && !mNoteTitle.equals(mNoteItem.getTitle())) {
                    return true;
                }
                break;
        }
        return false;
    }

    private void insertNote(String content) {
        if (content.length() > 0 || isTitleValid()) {//content is not empty or title need update
            if (null == mNoteItem) {
                Log.d(TAG, "insertNote --> noteItem is null");
                return;
            }
            mNoteItem.title = mNoteTitle;
            mNoteItem.content = content;
            mNoteItem.date = new Date().getTime();
            /* SPRD bug 520197 sqlitefull @{ */
            if ((mId = getDataManager(this).insertItem(mNoteItem)) == INSERT_ITEM_FAIL)
                return;
            /* @} */
            mOldContent = content;
            mOpenType = TYPE_EDIT_NOTE;
            Toast.makeText(NoteEditor.this, R.string.save_success, Toast.LENGTH_SHORT).show();
        }
    }

    private void updateNote(String content) {
        if (DEBUG) {
            Log.v(TAG, "_id =" + mId + " ; content =" + content + " ; mNoteTitle =" + mNoteTitle);
        }
        mNoteTitle = (mNoteTitle == null ? "" : mNoteTitle);
        content = (content == null ? "" : content);
        if (mNoteTitle.length() == 0 && content.length() == 0) {
            Toast.makeText(NoteEditor.this, R.string.save_failed, Toast.LENGTH_SHORT).show();
            return;
        } else if ((!content.equals(mOldContent)) || isTitleValid()) {//at least change the title or content
            if (null == mNoteItem) {
                Log.d(TAG, "updateNote --> noteItem is null");
                return;
            }
            mNoteItem.title = mNoteTitle;
            mNoteItem.content = content;
            mOldContent = content;
            mNoteItem.date = new Date().getTime();
            getDataManager(this).updateItem(mNoteItem);
            Toast.makeText(NoteEditor.this, R.string.save_success, Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.v(TAG, "onDestory");
    }

    /**
     * A custom EditText that draws lines between each line of text that is
     * displayed.
     */
    public static class LinedEditText extends EditText {
        private Rect mRect;
        private Paint mPaint;
        private static final String TAG = "NoteEditText";
        private static final String SCHEME_TEL = "tel:";
        private static final String SCHEME_HTTP = "http:";
        private static final String SCHEME_HTTPS = "https:";
        private static final String SCHEME_EMAIL = "mailto:";

        //Offset from the beginning of the EditText
        private int mOffset;

        private static final Map<String, Integer> sSchemaActionResMap = new HashMap<String, Integer>();
        static {
            sSchemaActionResMap.put(SCHEME_TEL, R.string.tel);
            sSchemaActionResMap.put(SCHEME_HTTP, R.string.web);
            sSchemaActionResMap.put(SCHEME_HTTPS, R.string.web);
            sSchemaActionResMap.put(SCHEME_EMAIL, R.string.email);
        }
        // we need this constructor for LayoutInflater
        public LinedEditText(Context context, AttributeSet attrs) {
            super(context, attrs);
            setWillNotDraw(false);
            setLinkTextColor(0xFF0069FF);
            setLinksClickable(false);
            mRect = new Rect();
            mPaint = new Paint();
            PathEffect effects = new DashPathEffect(new float[] {
                    1, 2, 4, 8
            }, 1);
            mPaint.setPathEffect(effects);
            mPaint.setStyle(Paint.Style.STROKE);
            mPaint.setStrokeWidth(1);
            mPaint.setColor(Color.parseColor("#dbdbdb"));

        }

        @Override
        protected void onCreateContextMenu(ContextMenu menu) {
            /* SPRD: add 20131206 Spreadtrum of 241544 Cannot click common text, mail address, and phone number in a note right @{ */
            Selection.setSelection(getEditableText(), Math.min(mOffset, getEditableText().length()));
            /* @} */
            int selStart = getSelectionStart();
            int selEnd = getSelectionEnd();
            Log.d(TAG, "LinedEditText onCreateContextMenu start "+selStart+" end "+selEnd);

            int min = Math.min(selStart, selEnd);
            int max = Math.max(selStart, selEnd);

            final URLSpan[] urls = ((Spanned) getText()).getSpans(min, max, URLSpan.class);
            if (urls.length == 1) {
                int defaultResId = 0;
                for (String schema : sSchemaActionResMap.keySet()) {
                    if (urls[0].getURL().indexOf(schema) >= 0) {
                        defaultResId = sSchemaActionResMap.get(schema);
                        break;
                    }
                }

                if (defaultResId == 0) {
                    defaultResId = R.string.note_link_other;
                }

                menu.add(0, 0, 0, defaultResId).setOnMenuItemClickListener(
                        new OnMenuItemClickListener() {
                            public boolean onMenuItemClick(MenuItem item) {
                                // goto a new intent
                                try{
                                    urls[0].onClick(LinedEditText.this);
                                }catch(Exception e){
                                    Log.e(TAG,"The website is not normal");
                                }
                                return true;
                            }
                        });
            }
            super.onCreateContextMenu(menu);
        }

        /* SPRD: add 20131206 Spreadtrum of 241544 Cannot click common text, mail address, and phone number in a note right @{ */
        @Override
        public boolean onTouchEvent(MotionEvent event) {
            super.onTouchEvent(event);
            int action = event.getAction();
            Layout layout = getLayout();
            int line = 0;
            switch (action) {
            case MotionEvent.ACTION_DOWN:
                line = layout.getLineForVertical(getScrollY() + (int) event.getY());
                mOffset = layout.getOffsetForHorizontal(line, (int) event.getX());
                break;
            }
            return true;
        }
        /* @} */
    }

    @Override
    public void onBackPressed() {
        try {
            super.onBackPressed();
        } catch (IllegalStateException e) {
            finish();
        }
    }


    public static class AlertDialogFragment extends DialogFragment {
        private AlertDialog dialog;
        private TextView mEditorCharCountView;
        private EditText titleView;
        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            final Bundle bundle = getArguments();
            if (bundle == null) {
                return null;
            }
            final int fragmentTag = bundle.getInt("tag");
            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            switch (fragmentTag) {
                case DIALOG_DELETE:
                    // SPRD: Modify for bug440521.
                    builder.setMessage(R.string.delete_note);
                    builder.setNegativeButton(R.string.Cancel, null);
                    builder.setPositiveButton(R.string.Ok,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    ((NoteEditor) getActivity()).doPositiveClick(bundle);
                                }
                            });
                    break;
                case Dialog_CANCLE:
                    // SPRD：459383 modify as message, not title
                    builder.setMessage(R.string.cancel_confirm);
                    builder.setNegativeButton(R.string.Cancel, null);
                    builder.setPositiveButton(R.string.Ok,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    ((NoteEditor)getActivity()).doPositiveClick(bundle);
                                }
                            });
                    break;
                case DIALOG_EDIT_TITLE:
                    View customTitleView = LayoutInflater.from(getActivity()).inflate(R.layout.dialog_title_layout, null);
                    TextView titleText = (TextView) customTitleView.findViewById(R.id.note_editor_title);
                    mEditorCharCountView = (TextView) customTitleView.findViewById(R.id.char_count);
                    View layout = LayoutInflater.from(getActivity()).inflate(R.layout.dialog_layout_new_folder, null);
                    titleView = (EditText) layout.findViewById(R.id.et_dialog_new_folder);
                    titleText.setText(R.string.edit_title);
                    builder.setCustomTitle(customTitleView);
                    builder.setView(layout);
                    builder.setPositiveButton(R.string.Ok, new DialogInterface.OnClickListener() {

                        public void onClick(DialogInterface dialog, int which) {
                            String title = titleView.getText().toString().trim();
                            bundle.putString("new_title", title);
                            ((NoteEditor) getActivity()).doPositiveClick(bundle);
                        }
                    });
                    builder.setNegativeButton(R.string.Cancel, new DialogInterface.OnClickListener() {

                        public void onClick(DialogInterface dialog, int which) {
                            ((NoteEditor)getActivity()).doNegativeClick(fragmentTag);
                        }
                    });
                    mEditorCharCountView.setText(bundle.getString("now_title").length() + "/" + MAX_FOLDER_NAME_NUM);
                    titleView.setText(bundle.getString("now_title"));
                    titleView.setSelection(titleView.getText().toString().length());
                    dialog = builder.create();
                    titleView.addTextChangedListener(new TextWatcher() {
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
                            mEditorCharCountView.setText(length + "/" + TITLE_MAX_LEN);
                            if (length > TITLE_MAX_LEN) {
                                String toastMessage = getActivity().getString(R.string.tilte_input_more, TITLE_MAX_LEN);
                                if (toast == null) {
                                    toast = Toast.makeText(getActivity(), toastMessage, Toast.LENGTH_SHORT);
                                }
                                toast.show();
                                s.delete(TITLE_MAX_LEN, length);
                            }
                        }
                    });
                    return dialog;

                default:
                    break;
            }
            return builder.create();
        }
    }

    void showDialog (Bundle bundle) {
        AlertDialogFragment dialogFragment = new AlertDialogFragment();
        dialogFragment.setArguments(bundle);
        /* SPRD: modify 20140425 Spreadtrum of 301384 com.sprd.note show IllegalStateException @{ */
        if (dialogFragment != null && isResumed()) {
            dialogFragment.show(getFragmentManager(), "" + bundle.getInt("tag"));
        }
        /* @} */
    }

    public void doPositiveClick (Bundle bundle) {
        switch (bundle.getInt("tag")) {
            case DIALOG_DELETE:
                if (null == mNoteItem) {
                    Log.d(TAG, "onCreateDialog --> noteItem is null");
                    return;
                }
                needSave = false;
                getDataManager(NoteEditor.this).deleteNoteItem(mNoteItem);
                Toast.makeText(getApplicationContext(), R.string.delete_success, Toast.LENGTH_SHORT).show();
                NoteEditor.this.finish();
                break;
            case Dialog_CANCLE:
                needSave = false;
                NoteEditor.this.finish();
                break;
            case DIALOG_EDIT_TITLE:
                mNoteTitle = bundle.getString("new_title");
                mNoteTitleView.setText(mNoteTitle);
                AlertDialogFragment dialogFragment = (AlertDialogFragment) getFragmentManager().findFragmentByTag("" + bundle.getInt("tag"));
                if (!(this.isFinishing() && dialogFragment != null && isResumed())) {
                    dialogFragment.dismiss();
                }
                break;
            default:
                break;
        }

    }

    public void doNegativeClick (int fragmentTag) {
        switch (fragmentTag) {
            case DIALOG_EDIT_TITLE:
                AlertDialogFragment dialogFragment = (AlertDialogFragment) getFragmentManager().findFragmentByTag("" + fragmentTag);
                if (!(this.isFinishing()) && dialogFragment != null && isResumed()) {
                    dialogFragment.dismiss();
                }
                break;
        }

    }

    /* SPRD: 442035 add receiver to monitor home key @{ */
    private BroadcastReceiver mHomeKeyEventReceiver = new BroadcastReceiver() {
        String SYSTEM_REASON = "reason";
        String SYSTEM_HOME_KEY = "homekey";
        String SYSTEM_HOME_KEY_LONG = "recentapps";

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)) {
                String reason = intent.getStringExtra(SYSTEM_REASON);
                if (TextUtils.equals(reason, SYSTEM_HOME_KEY)
                        || TextUtils.equals(reason, SYSTEM_HOME_KEY_LONG)) {
                    /* SPRD: 603328 @{ */
                    //finish();
                    if(needSave){
                        saveNote();
                    }
                    /* @} */
                }
            }
        }
    };
    /* @} */
}
