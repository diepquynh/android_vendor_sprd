package com.sprdroid.note;

import java.util.HashMap;
import java.util.Map;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.LoaderManager;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.Loader;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnCreateContextMenuListener;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

/*
 * 主界面：显示所有的文件夹和没有父文件夹的便签
 */

public class NoteActivity extends Activity implements LoaderManager.LoaderCallbacks<Cursor>{
    private TextView title;
    private ListView list;
    private ImageButton imagebutton;

    // private BaseAdapter adapter;
    private NoteAdapter adapter;

    //LinearLayout softButtonPanel;
    LinearLayout delete_softkey;
	private Button delete_OK;
	private Button delete_CANCEL;
	private Button select_All;

    private Cursor mCursor;
    private DBOperations dbo;
    // 构建被选择的便签项position与id的映射
    private Map<Integer, Integer> itemmap;
    private int mposition;
    private int itemId;
    // 文件夹名称
    private String oldFolderName;
    private int folderId;
    private EditText et_folder_name;

	private LayoutInflater listContainer;
	private ListItemView listItemView;
    private Message msg;
   // private Handler myHandler;
    private ProgressDialog progressDialog = null; 
    
    // 最大便签数
    public static final int MAX_NOTES = 1000;
    // 最大文件夹数
    public static final int MAX_FOLDERS = 1000;
    // 文件夹名输入限制的最大字数
    public static final int MAX_NUM = 15;
    // 菜单
    private static final int Menu_new_note = Menu.FIRST;
    private static final int Menu_new_folder = Menu.FIRST + 1;
    private static final int Menu_delete = Menu.FIRST + 2;
    private static final int Menu_delete_ALL = Menu.FIRST + 6;
    
    private static final int Menu_movetoFolder = Menu.FIRST + 3;
    private static final int Menu_setAlarm = Menu.FIRST + 4;
    private static final int Menu_folder_rename = Menu.FIRST + 5;

    // 闹钟时间
    //private int mHour;
    //private int mMinutes;
    private static final String TAG = "NoteActivity";

    

	@Override
	public Loader<Cursor> onCreateLoader(int id, Bundle args) {
		return new FoldersAndNotesLoader(this, dbo);
	}

	@Override
	public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
		mCursor = data;
		if(adapter == null){
			adapter = new NoteAdapter(this, mCursor, false);
			initListener();
		}else{
			adapter.setListItems(mCursor);
		}
		adapter.notifyDataSetChanged();
		list.setAdapter(adapter);
	}

	@Override
	public void onLoaderReset(Loader<Cursor> loader) {
		
	}
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 取消标题栏
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.index_page);
        // wangsl
        Log.d("you", "onCreate start.R.id.page_list is "
                + R.id.page_list);
        // wangsl
        list = (ListView) this.findViewById(R.id.page_list);
        // 更新ListView数据
        dbo = new DBOperations();
        /*mCursor = dbo.queryFoldersAndNotes(NoteActivity.this);
        // This method allows the activity to take care of managing the given
        // Cursor's lifecycle for you based on the activity's lifecycle.
        startManagingCursor(mCursor);
        adapter = new NoteAdapter(this, mCursor, false);*/
        //start loader
//        LoaderManager lm = getLoaderManager();
//        lm.restartLoader(0, null, this);
        
        list.setAdapter(adapter);
        list.setItemsCanFocus(false);
        list.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);
        // 点击文件夹或者便签执行该回调函数
        list.setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view,
                    int position, long id) {
                Intent intent = new Intent();
                // mCursor.moveToFirst();
                mCursor.moveToPosition(position);
                Log.e(TAG, "Position : " + position);
                // 传递被选中记录的ID
                intent.putExtra(DBOpenHelper.ID,
                        mCursor.getInt(mCursor.getColumnIndex(DBOpenHelper.ID)));
                // 取得此记录的DBOpenHelper.IS_FOLDER字段的值,用以判断选中文件夹还是便签
                String is_File = mCursor.getString(mCursor
                        .getColumnIndex(DBOpenHelper.IS_FOLDER));
                if (is_File.equals("no")) { // 不是文件夹,跳转到编辑内容页面
                    // 传递此记录的DBOpenHelper.CONTENT字段的值
                    intent.putExtra(DBOpenHelper.CONTENT, mCursor
                            .getString(mCursor
                                    .getColumnIndex(DBOpenHelper.CONTENT)));
                    // 告诉NoteActivity打开它是为了编辑便签
                    intent.putExtra("Open_Type", "editNote");
                    intent.setClass(NoteActivity.this, NoteEditor.class);
                } else if (is_File.equals("yes")) {
                    // 是文件夹,跳转到FolderActivity,显示选中的文件夹下所有的便签
                    intent.setClass(NoteActivity.this, FolderActivity.class);
                }
                //在删除状态，不能点击跳转进编辑界面
                if(adapter.getShowType() != NoteAdapter.SHOW_TYPE_DELETE) {
                    startActivity(intent);
                } else {
                	ListItemView listItems = (ListItemView) view.getTag();
    				listItems.check.toggle();
                   // list.setItemsCanFocus(false); 				
                }
            }
        });

        // 长按文件夹或者便签执行该回调函数
        list.setOnCreateContextMenuListener(new OnCreateContextMenuListener() {
            @Override
            public void onCreateContextMenu(ContextMenu conMenu, View view,
                    ContextMenuInfo menuInfo) {

                AdapterView.AdapterContextMenuInfo info;
                try {
                    info = (AdapterView.AdapterContextMenuInfo) menuInfo;
                } catch (ClassCastException e) {
                    Log.e(TAG, "bad menuInfo", e);
                    return;
                }
                dbo = new DBOperations();
                // 查询所有文件夹和顶级便签项
                Cursor cursor = (Cursor) dbo
                        .queryFoldersAndNotes(NoteActivity.this);
                Log.v(TAG, "ContextMenu:Position : " + info.position);

                if (cursor == null) {
                    // For some reason the requested item isn't available, do
                    // nothing
                    return;
                }
                // 将选中的ITEM的标题作为弹出菜单的TITLE
                // menu.setHeaderTitle(cursor.getString(COLUMN_INDEX_TITLE));
                conMenu.setHeaderTitle(R.string.contextmenu);
                // Add a contextmenu item to delete the note
                conMenu.add(0, Menu_delete, 0, R.string.delete);

                // 如果点击的是便签项，则增加“移进文件夹”选项
                cursor.moveToPosition(info.position);
                String is_Folder = cursor.getString(cursor
                        .getColumnIndex(DBOpenHelper.IS_FOLDER));

                Log.v(TAG, "is_Folder : " + is_Folder);
                itemmap = new HashMap<Integer, Integer>();
                // 获取对应位置上的记录的ID
                int itemId = cursor.getInt(cursor
                        .getColumnIndex(DBOpenHelper.ID));
                itemmap.put(info.position, itemId);
                mposition = info.position;       

                if (is_Folder.equals("no")) {
                    Log.v(TAG, "info.position : " + info.position);
                    conMenu.add(0, Menu_movetoFolder, 0, R.string.movetoFolder);
                    conMenu.add(0, Menu_setAlarm, 0, R.string.setAlarm);

                } else { // 点击的是文件夹，添加“修改文件夹名”项
                    conMenu.add(0, Menu_folder_rename, 0,
                            R.string.edit_folder_title);
                }
				if (cursor != null) {
					cursor.close();
				}
                //dbo.close();
            }
        });

        // 执行一些初始化的操作:title设置
        title = (TextView) findViewById(R.id.tvTitle);
        // title.setBackgroundDrawable(getResources().getDrawable(R.drawable.title_background));
        /*
         * // 取得屏幕宽度, DisplayMetrics dm = new DisplayMetrics();
         * getWindowManager().getDefaultDisplay().getMetrics(dm); int width =
         * dm.widthPixels; title.setWidth(7 / 8 * width);
         */
        imagebutton = (ImageButton) findViewById(R.id.imageButton);
        imagebutton.setBackgroundResource(R.drawable.imagebutton_background);
        imagebutton.setOnTouchListener(new Button.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_DOWN) {
                    // 更改为按下时的背景图片
                    v.setBackgroundResource(R.drawable.imagebutton_pressed);
                } else if (event.getAction() == MotionEvent.ACTION_UP) {
                    // 改为抬起时的图片
                    v.setBackgroundResource(R.drawable.imagebutton_background);
                }
                return false;
            }
        });
               
        //wangsl
        // 设置删除softkey
/*
        LayoutInflater mInflater = LayoutInflater.from(NoteActivity.this);
        softButtonPanel = (LinearLayout) mInflater.inflate(
                R.layout.delete_softkey, null);

        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.FILL_PARENT,
                LinearLayout.LayoutParams.FILL_PARENT);
        Log.v("you", "softButtonPanel");
        softButtonPanel.setGravity(Gravity.BOTTOM);
        softButtonPanel.setLayoutParams(p);

        NoteActivity.this.getWindow().addContentView(softButtonPanel, p);
        softButtonPanel.setVisibility(View.GONE);
        
 */       
        delete_softkey = (LinearLayout)this.findViewById(R.id.delete_softkey);
        delete_OK = (Button) findViewById(R.id.Delete_OK); 
        
        delete_OK.setText(R.string.delete);
        //delete_OK.setBackgroundResource(R.drawable.imagebutton_background);
        
		select_All = (Button) findViewById(R.id.Select_All);
		select_All.setText(R.string.select_all);

        
        delete_CANCEL = (Button) findViewById(R.id.Delete_CANCEL);
        delete_CANCEL.setText(R.string.Cancel);
        //delete_CANCEL.setBackgroundResource(R.drawable.imagebutton_background);
        
        delete_softkey.setVisibility(View.GONE); 
        //wangsl
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.v("you", "NoteActivity.onResume()");  
        // 更新ListView中的数据
        updateDisplay();
    }

    @Override
    protected void onDestroy() {
    	Log.v("you", "NoteActivity.onDestroy():  mCursor.close()");
		if (mCursor != null) {
			mCursor.close();
			mCursor = null;
		}
        super.onDestroy();

    }
  
    public void finish() {
        if(adapter != null && adapter.getShowType() == NoteAdapter.SHOW_TYPE_DELETE) {
            delete_softkey.setVisibility(View.GONE); 
            adapter.setShowType(NoteAdapter.SHOW_TYPE_NORMAL);
            adapter.notifyDataSetChanged();
            return;
        }
        
        super.finish();
    }
    
    // 创建菜单
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
		if (adapter != null) {
			// 新建便签
			menu.add(Menu.NONE, Menu_new_note, 1, R.string.new_note).setIcon(R.drawable.new_note);
			// 新建文件夹
			menu.add(Menu.NONE, Menu_new_folder, 2, R.string.new_folder).setIcon(R.drawable.new_folder);
			// 删除
			menu.add(Menu.NONE, Menu_delete, 3, R.string.delete).setIcon(R.drawable.delete);
			// 删除全部
			menu.add(Menu.NONE, Menu_delete_ALL, 4, R.string.delete_all).setIcon(R.drawable.delete);
		}

        return super.onCreateOptionsMenu(menu);
    }

    /*
     * @Override public boolean onPrepareOptionsMenu(Menu menu) {
     * super.onPrepareOptionsMenu(menu); 
     * final boolean haveItems =adapter.getCount() > 0;
     * 
     * // If there are any notes in the list (which implies that one of // them
     * is selected), then we need to generate the actions that // can be
     * performed on the current selection. This will be a combination // of our
     * own specific actions along with any extensions that can be found. if
     * (haveItems) {
     * 
     * Intent intent = new Intent(null, getIntent().getData());
     * intent.addCategory(Intent.CATEGORY_ALTERNATIVE);
     * menu.addIntentOptions(Menu.CATEGORY_ALTERNATIVE, 0, 0, new
     * ComponentName(this, NoteActivity.class), null, intent, 0, null);
     * 
     * } else { 
     * menu.removeGroup(Menu.CATEGORY_ALTERNATIVE); }
     * 
     * return true; }
     */

    // 菜单选中事件处理函数
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case Menu_new_note:
            newNote();
            break;
        case Menu_new_folder:
            newFolder();
            break;

        case Menu_delete: // 删除多项（checkbox）
            // TODO
            adapter.setShowType(NoteAdapter.SHOW_TYPE_DELETE);
            list.setAdapter(adapter);
            //adapter.notifyDataSetChanged();
            //softButtonPanel.setVisibility(View.VISIBLE);
            
            delete_softkey.setVisibility(View.VISIBLE);
            break;

        case Menu_delete_ALL: // 删除全部
            // TODO
                      
            deleteAllNotes();        // 执行耗时操作  
      
            break;
        default:
            break;
        }
        return super.onOptionsItemSelected(item);
    }
//    
//    //处理消息，控制正在删除进度条
//    Handler myHandler = new Handler() {
//        public void handleMessage(Message msg) {
//        //弹出正在删除dialogprogress
////            String m =msg.getData().getString("key");
////            if(m== "delete_start") {//打开正在删除进度条
////            } 
////            else if(m== "delete_end") {//关闭正在删除进度条
////            }
//            switch (msg.what) {
//            case 1:                                       
//            //显示正在删除进度条   
//                Log.v("you"," progressDialog.show()********** " );
//                    progressDialog = new ProgressDialog(NoteActivity.this);
//                    //ProgressDialog.show(NoteActivity.this, "Delete", "deleteing, please waite！");  
//                    progressDialog.show(NoteActivity.this, "Delete", "deleteing, please waite！");
//                    break;
//
//            case 0:                                       
//            //刷新UI，显示数据，并关闭进度条  
//                Log.v("you"," progressDialog.dismiss()*************** " );
//                if(progressDialog != null) {
//                    progressDialog.dismiss(); 
//                }
//                    break;
//            } 
//            
//        }
//    };
    // add by gerry
    Handler mHandler = new Handler();
    ProgressDialog mProgressDialog = null;
    Runnable mDeleting = new Runnable() {
        @Override
        public void run() {
            // TODO Auto-generated method stub
            Log.v("you"," progressDialog.show()********** " );  
            String delete= NoteActivity.this.getString(R.string.delete);
            String deleting= NoteActivity.this.getString(R.string.Deleting);           
            mProgressDialog = ProgressDialog.show(NoteActivity.this, delete, deleting);          
        }
      
    };
    
    Runnable mFinishDelete = new Runnable() {
        @Override
        public void run() {
            // TODO Auto-generated method stub
            Log.v("you"," progressDialog.dismiss()*************** " );
              if(mProgressDialog.isShowing()) {
                  mProgressDialog.dismiss(); 
                  mProgressDialog = null;
                  //删除成功，更新UI
                  updateDisplay();
              }          
        }
      
    };
    // end
    
    @Override
    public boolean onContextItemSelected(MenuItem item) {
        AdapterView.AdapterContextMenuInfo info;
        try {
            info = (AdapterView.AdapterContextMenuInfo) item.getMenuInfo();
            Log.v("you", "context item seleted ID=" + info.id);
        } catch (ClassCastException e) {
            Log.e(TAG, "bad menuInfo", e);
            return false;
        }
        // 获取选中行位置
        int position = info.position;

        switch (item.getItemId()) {
            case Menu_delete: {
                // 获取选中的项的id，
                // getListAdapter().getItemId(menuInfo.position)返回的值是Long型，需要转化为int型
                // int id = (int) list.getAdapter().getItemId(position);
                Cursor cursor = dbo.queryFoldersAndNotes(NoteActivity.this);
                // 通过获取在LIST中的位置，得到数据库中的记录。
                cursor.moveToPosition(position);
                itemId = cursor.getInt(cursor.getColumnIndex(DBOpenHelper.ID));
                Log.v("you", "onContextItemSelected:delete:  position = "
                        + position + ",itemid= " + itemId);
    
                deleteNote(itemId);
                cursor.close();
                return true;
    
            }
            case Menu_movetoFolder: { // 弹出选项菜单，选择移进哪个文件夹
                chooseFolder();
                // updateDisplay();
                return true;
            }
            case Menu_setAlarm: { // 设置便签闹铃
                Intent intent = new Intent(this, AlarmActivity.class);
                intent.putExtra("noteid", itemmap.get(mposition));
                this.startActivityForResult(intent, 1);
    
                return true;
            }
            case Menu_folder_rename: { // 文件夹修改名称
                renameFolder();
    
                return true;
            }
        }
        return false;
    }

    // 新建便签函数
    private void newNote() {
        if (adapter.getCount() > MAX_NOTES) {
            Toast.makeText(this, R.string.toast_add_fail, Toast.LENGTH_SHORT)
                    .show();
            return;
        }

        Intent in = new Intent();
        in.putExtra("Open_Type", "newNote");
        in.setClass(NoteActivity.this, NoteEditor.class);
        startActivity(in);
    }

    // 新建文件夹函数
    private void newFolder() {
        if (adapter.getCount() > MAX_FOLDERS) {
            Toast.makeText(this, R.string.toast_add_fail, Toast.LENGTH_SHORT)
                    .show();
            return;
        }

        Context mContext = NoteActivity.this;
        // 使用AlertDialog来处理新建文件夹的动作
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(R.string.new_folder);
        // 自定义AlertDialog的布局
        LayoutInflater inflater = (LayoutInflater) mContext
                .getSystemService(LAYOUT_INFLATER_SERVICE);
        final View layout = inflater.inflate(R.layout.dialog_layout_new_folder,
                (ViewGroup) findViewById(R.id.dialog_layout_new_folder_root));
        builder.setView(layout);

        // 实例化AlertDialog中的EditText对象
        et_folder_name = (EditText) layout.findViewById(R.id.et_dialog_new_folder);
	//对EditText输入内容监听，如果超过最大字符数限制，则弹出提示
        et_folder_name.addTextChangedListener(textwatch);

        // 设置一个确定的按钮
        builder.setPositiveButton(R.string.Ok,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {

                        // 取得EditText对象的值
                        String newFolderName = et_folder_name.getText()
                                .toString();
                        // 判断文件夹名称是否为空
                        int FolderName_length = newFolderName.length();
                        if (FolderName_length > 0 && newFolderName != null) {

                                dbo.insert(NoteActivity.this, newFolderName,
                                        dbo.getDate(), dbo.getTime(), null,
                                        null, "yes", "no");
                                // 更新ListView的数据源
                                NoteActivity.this.updateDisplay();

                        } else {  //文件夾名稱为空时，保存失敗
                            Toast.makeText(NoteActivity.this,
                                    R.string.folder_add_fail,
                                    Toast.LENGTH_SHORT).show();
                        }
                    }
                });
        // 设置一个取消的按钮
        builder.setNegativeButton(R.string.Cancel,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        // 点击取消按钮,撤销新建文件夹对话框
                        dialog.dismiss();                      
                    }
                });
        AlertDialog ad = builder.create();
        ad.show();
    }

    TextWatcher textwatch = new TextWatcher() {
	private CharSequence temp;
	private int selectionStart;
	private int selectionEnd;
	public void beforeTextChanged(CharSequence s, int start, int count,
	        int after) {

	}

	public void onTextChanged(CharSequence s, int start, int before,
	        int count) {
	    temp = s;
	    // TextView.setText(s);
	}

	public void afterTextChanged(Editable s) {
	    int number = s.length();

	    selectionStart = et_folder_name.getSelectionStart();
	    selectionEnd = et_folder_name.getSelectionEnd();
	    if (temp.length() > MAX_NUM) {
	        Toast.makeText(NoteActivity.this, R.string.toast_edit_outbound,
	                Toast.LENGTH_SHORT).show();

	        // 截断超出的内容
	        int i = temp.length() - MAX_NUM; // 超出多少字符
	        s.delete(selectionStart - i, selectionEnd);
	        int tempSelection = selectionEnd;
	        et_folder_name.setText(s);
	        et_folder_name.setSelection(tempSelection); // 设置光标在最后
	    }
	}
    };


    // 删除单个便签
    private void deleteNote(final int note_id) {
        Context mContext = NoteActivity.this;
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(R.string.delete_note);

        builder.setPositiveButton(R.string.Ok,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {

                        dbo.delete(getApplicationContext(), note_id);
                        Toast.makeText(getApplicationContext(),
                                R.string.delete_success, Toast.LENGTH_LONG)
                                .show();
                        updateDisplay();
                    }
                });
        builder.setNegativeButton(R.string.Cancel,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        // 点击取消按钮,撤销删除便签对话框
                        dialog.dismiss();
                    }
                });

        AlertDialog ad = builder.create();
        ad.show();
    }

//    // 删除全部便签和文件夹
//    private void deleteAllNotes() {
//        
//        Context mContext = NoteActivity.this;
//        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
//        builder.setTitle(R.string.delete_all_note);
//
//        builder.setPositiveButton(R.string.Ok,
//                new DialogInterface.OnClickListener() {
//                    @Override
//                    public void onClick(DialogInterface dialog, int which) {
//                        
//                        new Thread() {
//                            public void run() {                
//                                msg = myHandler.obtainMessage();
//                                msg.what = 1;
////                                Bundle b = new Bundle();
////                                b.putString("key", "delete_start");
////                                msg.setData(b);    // 向消息中添加数据
//                                myHandler.sendMessage(msg);    // 向Handler发送消息，更新UI
//                         
//                                //执行全部删除 耗时操作
//                                //Cursor cursor = dbo.queryFoldersAndNotes(NoteActivity.this);
//                                Cursor cursor = dbo.queryAllRecords(NoteActivity.this);
//                                int count = cursor.getCount();
//                                // 通过获取在LIST中的位置，得到数据库中的记录。
//                                cursor.moveToFirst();
//                                if(count > 0) {
//                                    for(int i=0;i<count;i++) {
//                                        if(cursor != null) {
//                                            itemId = cursor.getInt(cursor.getColumnIndex(DBOpenHelper.ID));                            
//                                            dbo.delete(getApplicationContext(), itemId);
//                                        }
//                                        cursor.moveToNext();
//                                    }
//                                    //发送消息，停止正在删除进度条
//                                    msg = myHandler.obtainMessage();
//                                    msg.what = 0;
//                                    myHandler.sendMessage(msg); 
//                                    updateDisplay();
//                                    
//                                    Toast.makeText(getApplicationContext(),
//                                            R.string.delete_success, Toast.LENGTH_LONG).show();
//                                    
//                                } else { //count = 0
//                                    Toast.makeText(getApplicationContext(),
//                                            R.string.list_is_empty, Toast.LENGTH_LONG).show();
//                                }
//                                cursor.close();
//                                
//                            }
//                        }.start();
//                        
//                    }
//                });
//        builder.setNegativeButton(R.string.Cancel,
//                new DialogInterface.OnClickListener() {
//                    @Override
//                    public void onClick(DialogInterface dialog, int which) {
//                        // 点击取消按钮,撤销删除便签对话框
//                        dialog.dismiss();
//
//                    }
//                });
//
//        AlertDialog ad = builder.create();
//        ad.show();  
//        
//    }
//add by gerry
    
    Runnable mDeleteThread = new Runnable() {
        //此为非ui线程，不能处理ui操作。
        @Override
        public void run() {
            // TODO Auto-generated method stub
                    
            //执行全部删除 耗时操作
            //Cursor cursor = dbo.queryFoldersAndNotes(NoteActivity.this);
            Cursor cursor = dbo.queryAllRecords(NoteActivity.this);
            int count = cursor.getCount();
            // 通过获取在LIST中的位置，得到数据库中的记录。
            cursor.moveToFirst();
            Log.d("you","count = " + count);
            if(count > 0) {
		//显示正在刪除进度条
		mHandler.removeCallbacks(mDeleting);
		mHandler.post(mDeleting);
                for(int i=0;i<count;i++) {
                    if(cursor != null) {
                        itemId = cursor.getInt(cursor.getColumnIndex(DBOpenHelper.ID));                            
                        dbo.delete(getApplicationContext(), itemId);
                    }
                    cursor.moveToNext();
                }
                //发送消息，停止正在删除进度条
                mHandler.removeCallbacks(mFinishDelete);
                mHandler.post(mFinishDelete);

                mHandler.post(mToast_delete_success);
            } 
               else { //count = 0
                mHandler.post(mToast_list_is_empty);  
                   
            }
            cursor.close();
        
        }
        
    };
    
    
    Runnable mToast_delete_success = new Runnable() {
        @Override
        public void run() {
            // TODO Auto-generated method stub
          Toast.makeText(getApplicationContext(),
          R.string.delete_success, Toast.LENGTH_LONG).show();
        }
    };
    
    Runnable mToast_list_is_empty = new Runnable() {
        @Override
        public void run() {
            // TODO Auto-generated method stub
          Toast.makeText(getApplicationContext(),
          R.string.list_is_empty, Toast.LENGTH_LONG).show();
        }
    }; 
    
//  // 删除全部便签和文件夹
  private void deleteAllNotes() {
      
      Context mContext = NoteActivity.this;
      AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
      builder.setTitle(R.string.delete_all_note);

      builder.setPositiveButton(R.string.Ok,
              new DialogInterface.OnClickListener() {
                  @Override
                  public void onClick(DialogInterface dialog, int which) {                     
                      new Thread(mDeleteThread).start();
                      
                  }
              });
      builder.setNegativeButton(R.string.Cancel,
              new DialogInterface.OnClickListener() {
                  @Override
                  public void onClick(DialogInterface dialog, int which) {
                      // 点击取消按钮,撤销删除便签对话框
                      dialog.dismiss();

                  }
              });

      AlertDialog ad = builder.create();
      ad.show();  
      
  }
//end
  
  
    // 删除多条便签
    private void deleteAnyNotes() {

        /*
         * //获取CheckBox实例 CheckBox cb = (CheckBox)this.findViewById(R.id.cb);
         * //绑定监听器
         * list.setOnItemClickListener(new OnItemClickListener(){
         * 
         * @Override public void onItemClick(AdapterView<?> parent, View view,
         * int position, long id) { ViewHolder vHolder = (ViewHolder)
         * view.getTag(); //在每次获取点击的item时将对checkbox状态改变，同时修改map的值。
         * vHolder.cBox.toggle(); adapter.isSelected.put(position,
         * vHollder.cBox.isChecked()); } });
         */

        Context mContext = NoteActivity.this;
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(R.string.delete_note);

        builder.setPositiveButton(R.string.Ok,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                    //删除isSelected数组中为true的项
                        Cursor cursor = dbo.queryFoldersAndNotes(NoteActivity.this);  
                        int count = cursor.getCount();  // 列表有多少项
                        int selected_count = 0;  // 选择了（勾选）多少项
        		Log.v("you", " ************list count= " + count);
                        if(count > 0) {                   

                            for(int i=0; i<count; i++) {  
                                Log.v("you"," adapter.isSelected= " + adapter.isSelected);
                                Log.v("you","adapter.isSelected.get(i)= " + adapter.isSelected.get(i));

                                if(adapter.isSelected.get(i) != null && adapter.isSelected.get(i)) {
                                    // 通过获取在LIST中的位置，得到数据库中的记录。
                                    cursor.moveToPosition(i);
                                    itemId = cursor.getInt(cursor.getColumnIndex(DBOpenHelper.ID));                            
                                    dbo.delete(getApplicationContext(), itemId);
				    selected_count +=1;
                                }
                            }

				if(selected_count >0) {  //选择项不为空
                            		Toast.makeText(getApplicationContext(),
                                    R.string.delete_success, Toast.LENGTH_LONG).show();
				}else {  //列表不为空，选择项为空
                            		Toast.makeText(getApplicationContext(),
                                    R.string.selected_is_empty, Toast.LENGTH_LONG).show();
				}
                        } else {//列表为空
                            Toast.makeText(getApplicationContext(),
                                    R.string.list_is_empty, Toast.LENGTH_LONG).show();
                        }
                        cursor.close();

                        updateDisplay();
                    }
                });
        builder.setNegativeButton(R.string.Cancel,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        // 点击取消按钮,撤销删除便签对话框
                        dialog.dismiss();
                        adapter.setShowType(NoteAdapter.SHOW_TYPE_NORMAL);
                        adapter.notifyDataSetChanged();
                    }
                });

        AlertDialog ad = builder.create();
        ad.show();       
    }

    // 负责更新ListView中的数据
    private void updateDisplay() {
        dbo = new DBOperations();

        /*mCursor = dbo.queryFoldersAndNotes(NoteActivity.this);
        Log.v("you", " ************* mCursor.getcount() " + mCursor.getCount());
        // This method allows the activity to take care of managing the given
        // Cursor's lifecycle for you based on the activity's lifecycle.
        startManagingCursor(mCursor);
        if(adapter == null) {
            adapter = new NoteAdapter(NoteActivity.this, mCursor, false);
        } else {
            adapter.setListItems(mCursor);
        }
        adapter.notifyDataSetChanged();
        list.setAdapter(adapter);*/
        
        LoaderManager lm = getLoaderManager();
        lm.restartLoader(0, null, this);
        
        Log.v("you", " *******updatedisplay()  end " );  
    }

    // 选择目标文件夹
    private void chooseFolder() {

        // Log.e(TAG, "被选中的便签:" + id);
        final Cursor folderCursor = dbo
                .queryAllFolders(getApplicationContext());
        // 文件夹的数量
        int count = folderCursor.getCount();
        if (count > 0) {// 有文件夹
            // Log.e(TAG, "文件夹数量:" + count);
            // 将从数据库中查询到的文件夹的名称放入字符串数组
            String[] folders = new String[count];
//            startManagingCursor(folderCursor);
            for (int i = 0; i < count; i++) {
                folderCursor.moveToPosition(i);
                folders[i] = folderCursor.getString(folderCursor
                        .getColumnIndex(DBOpenHelper.CONTENT));
            }
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setItems(folders, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int item) {
                    folderCursor.moveToPosition(item);
                    // 取得被选中的文件夹的ID
                    int folderId = folderCursor.getInt(folderCursor
                            .getColumnIndex(DBOpenHelper.ID));
                    // 更新主界面所有记录
                    // int count = mCursor.getCount();

                    // 得到被选择的便签记录的ID
                    int noteId = itemmap.get(mposition);
                    Cursor oneNote = dbo.queryOneNote(getApplicationContext(),
                            noteId);
//                    startManagingCursor(oneNote);
                    oneNote.moveToFirst();
                    // 便签的内容
                    String content = oneNote.getString(oneNote
                            .getColumnIndex(DBOpenHelper.CONTENT));
                    // 便签是否开启
                    //String alarmID = oneNote.getString(oneNote
                    // .getColumnIndex(DBOpenHelper.ALARM_ID));
                    int alarmEnable = oneNote.getInt(oneNote
                             .getColumnIndex(DBOpenHelper.ALARM_ENABLE));
                    
                    // 便签的背景色
                    // String bgColor = oneNote.getString(oneNote
                    // .getColumnIndex(DBOpenHelper.BG_COLOR));
                    // 更新记录，将便签项parent属性设置
                    //wangsl。
                    dbo.update(getApplicationContext(), noteId, content, alarmEnable,
                            null, String.valueOf(folderId));
                    //wangsl
                    oneNote.close();
                    folderCursor.close();  
                    // 更新主界面的LIST显示。
                    updateDisplay();
                }
            });
            builder.create().show();
        } else {// 用户未曾创建文件夹
            Toast.makeText(getApplicationContext(), R.string.no_folder_found,
                    Toast.LENGTH_LONG).show();
        }

    }

    // 修改文件夹名称
    private void renameFolder() {
        // 得到被选择项记录的ID
        folderId = itemmap.get(mposition);

        Context mContext = NoteActivity.this;
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(R.string.edit_folder_title);

        LayoutInflater inflater = (LayoutInflater) mContext
                .getSystemService(LAYOUT_INFLATER_SERVICE);
        final View layout = inflater.inflate(R.layout.dialog_layout_new_folder,
                (ViewGroup) findViewById(R.id.dialog_layout_new_folder_root));
        builder.setView(layout);

        // 查询文件夹记录.内容保存到Cursor对象中
        Cursor c2 = dbo.queryOneNote(this, folderId);
        c2.moveToFirst();
        oldFolderName = c2.getString(c2.getColumnIndex(DBOpenHelper.CONTENT));
        c2.close();

        // 显示原有文件夹名称
        et_folder_name = (EditText) layout
                .findViewById(R.id.et_dialog_new_folder);
        et_folder_name.setText(oldFolderName);
        //设置光标位置
        int index = oldFolderName.length();
        et_folder_name.setSelection(index);
        et_folder_name.addTextChangedListener(textwatch);

        builder.setPositiveButton(R.string.Ok,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {

                        String newFolderName = et_folder_name.getText().toString();
                        // 新文件夹名称不为空,且不等于原有的名称,则更新
                        if (newFolderName.length() > 0 && newFolderName != null
                                && newFolderName != oldFolderName) {
                        
                                dbo.update(NoteActivity.this, folderId,
                                        newFolderName, 0, null, "no");
                                oldFolderName = newFolderName;

                            // 通知更新数据源
                            updateDisplay();
                        } else {
                            Toast.makeText(NoteActivity.this,
                                    R.string.folder_add_fail,
                                    Toast.LENGTH_SHORT).show();
                        }
                    }
                });
        builder.setNegativeButton(R.string.Cancel,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        // 点击取消按钮,撤销修改文件夹名称对话框
                        dialog.dismiss();
                    }
                });

        AlertDialog ad = builder.create();
        ad.show();

    }

	private void initListener() {
		// 为按钮设置事件监听：增加一个便签
		imagebutton.setOnClickListener(new Button.OnClickListener() {
			public void onClick(View v) {
				newNote();
			}
		});

		delete_OK.setOnClickListener(new Button.OnClickListener() {
			public void onClick(View v) {
				Log.v("you", "delete_OK.setOnClickListener");
				deleteAnyNotes();

				delete_softkey.setVisibility(View.GONE);
				adapter.setShowType(NoteAdapter.SHOW_TYPE_NORMAL);
			}
		});

		select_All.setOnClickListener(new Button.OnClickListener() {
			public void onClick(View v) {
				// TODO
				adapter.setShowType(NoteAdapter.SHOW_TYPE_DELETE);

				for (int i = 0; i < adapter.getCount(); i++) {
					adapter.isSelected.put(i, true);
				}
				list.setAdapter(adapter);

			}
		});

		delete_CANCEL.setOnClickListener(new Button.OnClickListener() {
			public void onClick(View v) {
				// TODO
				// softButtonPanel.setVisibility(View.GONE);
				delete_softkey.setVisibility(View.GONE);
				adapter.setShowType(NoteAdapter.SHOW_TYPE_NORMAL);
				adapter.isSelected.clear();
				adapter.notifyDataSetChanged();
			}
		});
	}

    /*
     * //设置初始当前闹铃时间 
     * private void showTimePicker() { 
     * Time time = new Time("GMT+8"); 
     * time.setToNow(); 
     * int year = time.year; 
     * int month = time.month; 
     * int day = time.monthDay; 
     * int minute = time.minute; 
     * int hour = time.hour; 
     * int sec = time.second;
     * 
     * //final Calendar mCalendar=Calendar.getInstance();
     * //mCalendar.setTimeInMillis(System.currentTimeMillis());
     * //mHour=mCalendar.get(Calendar.HOUR);
     * //mMinuts=mCalendar.get(Calendar.MINUTE);
     * 
     * mHour = hour; 
     * mMinutes = minute; 
     * new TimePickerDialog(this, this, mHour, mMinutes, DateFormat.is24HourFormat(this))
     * .show(); }
     * 
     * public void onTimeSet(TimePicker view, int hourOfDay, int minute) { 
     * //onTimeSet is called when the user clicks "Set"
     * 
     * mHour = hourOfDay; 
     * mMinutes = minute; 
     * //saveAlarm();
     * 
     * //如果设定时间小于当前时间，toast该时间不能设置 
     * //如果设定时间大于当前时间，toast该时间设置成功 
     * }
     * 
     * //将闹钟保存到数据库 
     * private long saveAlarm() { 
     * Alarm alarm = new Alarm();
     * alarm.id = mId; 
     * alarm.enabled = mEnabledPref.isChecked(); 
     * alarm.hour = mHour; 
     * alarm.minutes = mMinutes; 
     * alarm.daysOfWeek = mRepeatPref.getDaysOfWeek(); 
     * alarm.vibrate = mVibratePref.isChecked();
     * alarm.label = mLabel.getText(); 
     * alarm.alert = mAlarmPref.getAlert(); 
     * long time; 
     * if (alarm.id == -1) { 
     * time = Alarms.addAlarm(this, alarm); 
     * //addAlarm populates the alarm with the new id. Update mId so that 
     * //changes to other preferences update the new 
     * alarm. mId = alarm.id; 
     * } else { 
     * time = Alarms.setAlarm(this, alarm); 
     * } 
     * return time; 
     * }
     */

}
