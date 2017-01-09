package com.sprdroid.note;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Handler;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.View.OnCreateContextMenuListener;
import android.view.MotionEvent;

import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.Toast;
import android.text.Editable;
import android.text.TextWatcher;

import com.sprdroid.note.ListItemView;
import com.sprdroid.note.NoteAdapter;
import com.sprdroid.note.DBOpenHelper;
import com.sprdroid.note.DBOperations;

/*
 * 显示某一文件夹下的所有便签
 */
public class FolderActivity extends Activity {

    private ImageButton imagebutton;
    private TextView title;
    private ListView list;

    private NoteAdapter adapter;
    //LinearLayout softButtonPanel;
    Button delete_OK;
    Button delete_CANCEL;
	private Button select_All;
    LinearLayout delete_softkey;
    
    // private Cursor mCursor;
    // 得到点击修改文件夹名称Menu以前的名称
    private String oldFolderName;
    private EditText et_folder_name;
    // 最大便签数
    public static final int MAX_NOTES = 100;
    // 文件夹名输入限制的最大字数
    public static final int MAX_NUM = 15;

    // 菜单
    private static final int Menu_new_note = Menu.FIRST;
    private static final int Menu_update_folder = Menu.FIRST + 1; // 修改文件夹名
    private static final int Menu_delete = Menu.FIRST + 2;
    private static final int Menu_moveoutFolder = Menu.FIRST + 3;
    private static final int Menu_setAlarm = Menu.FIRST + 4;
    private static final int Menu_delete_ALL = Menu.FIRST + 5;  
    
    private DBOperations dbo = new DBOperations();
    private int _id; // 文件夹的ID
    private int itemId; // 便签ID

    private static final String TAG = "FolderActivity";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 取消标题栏
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.index_page);

        Intent i = getIntent();
        // 如果没有传递Intent对象,则返回主页
        if (i.equals(null)) {
            startActivity(new Intent(FolderActivity.this, NoteActivity.class));
        }
        _id = i.getIntExtra(DBOpenHelper.ID, -1);
        // 查询该文件夹记录.内容保存到Cursor对象中
        Cursor c2 = dbo.queryOneNote(this, _id);
        c2.moveToFirst();
        oldFolderName = c2.getString(c2.getColumnIndex(DBOpenHelper.CONTENT));
        c2.close();
        initViews();

        list.setOnItemClickListener(new OnItemClickListener() {
            // 点击文件夹下的便签执行该回调函数
            @Override
            public void onItemClick(AdapterView<?> parent, View view,
                    int position, long id) {
                Intent intent = new Intent();
                Cursor mCursor = dbo.queryFromFolder(FolderActivity.this, _id);
                mCursor.moveToPosition(position);
                // 传递被选中记录的ID
                intent.putExtra(DBOpenHelper.ID,
                        mCursor.getInt(mCursor.getColumnIndex(DBOpenHelper.ID)));
                // 传递当前文件夹的ID
                intent.putExtra("FolderId", _id);
                Log.v(TAG, "Folder id : " + _id + ";position:" + position);
                // 传递被编辑便签的内容
                intent.putExtra(DBOpenHelper.CONTENT, mCursor.getString(mCursor
                        .getColumnIndex(DBOpenHelper.CONTENT)));
                mCursor.close();
                // 编辑便签的方式
                intent.putExtra("Open_Type", "editFolderNote");
                // 跳转到NoteEditor
                intent.setClass(FolderActivity.this, NoteEditor.class);
                if(adapter.getShowType() != NoteAdapter.SHOW_TYPE_DELETE) {
                    startActivity(intent);
                }else {
                	ListItemView listItems = (ListItemView) view.getTag();
    				listItems.check.toggle();
                   // list.setItemsCanFocus(false); 
                }
            }
        });

        // 长按便签弹出菜单
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
                // dbo = new DBOperations();
                // 查询文件夹下的所有便签
                Cursor cursor = (Cursor) dbo.queryFromFolder(
                        FolderActivity.this, _id);
                cursor.moveToPosition(info.position);
                // 得到点击的便签项
                itemId = cursor.getInt(cursor.getColumnIndex(DBOpenHelper.ID));
                cursor.close();
                Log.v(TAG, "ContextMenu:Position : " + info.position);

                // 将选中的ITEM的标题作为弹出菜单的TITLE
                // menu.setHeaderTitle(cursor.getString(COLUMN_INDEX_TITLE));
                conMenu.setHeaderTitle(getResources().getString(R.string.contextmenu));
                // Add a contextmenu item to delete the note
                conMenu.add(0, Menu_delete, 0, R.string.delete);
                conMenu.add(0, Menu_moveoutFolder, 0, R.string.moveoutFolder);
                conMenu.add(0, Menu_setAlarm, 0, R.string.setAlarm);
            }
        });

        imagebutton = (ImageButton) findViewById(R.id.imageButton);
        imagebutton.setBackgroundResource(R.drawable.imagebutton_background);
        // 为按钮设置事件监听：增加一个便签
        imagebutton.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                newNote();
            }
        });
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
        
        // 设置删除softkey
        /*
        LayoutInflater mInflater = LayoutInflater.from(FolderActivity.this);
        softButtonPanel = (LinearLayout) mInflater.inflate(
                R.layout.delete_softkey, null);

        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.FILL_PARENT,
                LinearLayout.LayoutParams.FILL_PARENT);
        Log.v("you", "softButtonPanel");
        softButtonPanel.setGravity(Gravity.BOTTOM);
        softButtonPanel.setLayoutParams(p);

        FolderActivity.this.getWindow().addContentView(softButtonPanel, p);
        softButtonPanel.setVisibility(View.GONE);
 */       
        delete_softkey = (LinearLayout)this.findViewById(R.id.delete_softkey);
        
        delete_OK = (Button) findViewById(R.id.Delete_OK);
        delete_OK.setText(R.string.delete);
        //delete_OK.setBackgroundResource(R.drawable.imagebutton_background);
        delete_OK.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                Log.v("you", "delete_OK.setOnClickListener");                   
                deleteAnyNotes();
                //softButtonPanel.setVisibility(View.GONE);
                delete_softkey.setVisibility(View.GONE);
                adapter.setShowType(NoteAdapter.SHOW_TYPE_NORMAL);
                
            }
        });   
        
		select_All = (Button) findViewById(R.id.Select_All);
		select_All.setText(R.string.select_all);

		select_All.setOnClickListener(new Button.OnClickListener() {
			public void onClick(View v) {
				// TODO
				adapter.setShowType(NoteAdapter.SHOW_TYPE_DELETE);
				
				for (int i = 0; i < adapter.getCount(); i++)
				{
					adapter.isSelected.put(i, true);
				}
				list.setAdapter(adapter);

			}
		});
        
        delete_CANCEL = (Button) findViewById(R.id.Delete_CANCEL);
        delete_CANCEL.setText(R.string.Cancel);
        //delete_CANCEL.setBackgroundResource(R.drawable.imagebutton_background);
        delete_CANCEL.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                //TODO
                //softButtonPanel.setVisibility(View.GONE);
                delete_softkey.setVisibility(View.GONE);
                adapter.setShowType(NoteAdapter.SHOW_TYPE_NORMAL);
                adapter.notifyDataSetChanged();
            }
        });          

        delete_softkey.setVisibility(View.GONE);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // 新建便签
        menu.add(Menu.NONE, Menu_new_note, 1, R.string.new_note).setIcon(
                R.drawable.new_note);
        // 修改文件夹名称
        menu.add(Menu.NONE, Menu_update_folder, 2, R.string.edit_folder_title)
                .setIcon(R.drawable.edit_folder_title);
        // 删除多条便签
         menu.add(Menu.NONE, Menu_delete, 3, R.string.delete).setIcon(
         R.drawable.delete);
         // 删除全部便签
         menu.add(Menu.NONE, Menu_delete_ALL, 4, R.string.delete_all).setIcon(
         R.drawable.delete);            

        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case Menu_new_note:
            newNote();
            break;
        case Menu_update_folder:
            updateFolderName();
            break;

        case Menu_delete://删除多条便签
            adapter.setShowType(NoteAdapter.SHOW_TYPE_DELETE);
            list.setAdapter(adapter);
            //adapter.notifyDataSetChanged();
            //softButtonPanel.setVisibility(View.VISIBLE);
            delete_softkey.setVisibility(View.VISIBLE);

            break;
        case Menu_delete_ALL://删除全部便签
            deleteAllNotes();
            break;

        default:
            break;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onResume() {
        super.onResume();
        this.updateDisplay(oldFolderName);
    }

    @Override
    protected void onDestroy() {

        //dbo.close();
        super.onDestroy();
    }
    
    public void finish() {
        if(adapter.getShowType() == NoteAdapter.SHOW_TYPE_DELETE) {
            delete_softkey.setVisibility(View.GONE); 
            adapter.setShowType(NoteAdapter.SHOW_TYPE_NORMAL);
            adapter.notifyDataSetChanged();
            return;
        }
        
        super.finish();
    }
    
    // 初始化组件
    private void initViews() {
        // imagebutton = (ImageButton) findViewById(R.id.imageButton);
        list = (ListView) findViewById(R.id.page_list);
        title = (TextView) findViewById(R.id.tvTitle);
        title.setBackgroundDrawable(getResources().getDrawable(
                R.drawable.title_background));

        // 取得屏幕宽度,
        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        int width = dm.widthPixels;
        title.setWidth(7 / 8 * width);

        updateDisplay(oldFolderName);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        AdapterView.AdapterContextMenuInfo info;
        try {
            info = (AdapterView.AdapterContextMenuInfo) item.getMenuInfo();
            Log.v(TAG, "context item seleted ID=" + info.id);
        } catch (ClassCastException e) {
            Log.e(TAG, "bad menuInfo", e);
            return false;
        }
        // 获取选中行位置
        int position = info.position;

        switch (item.getItemId()) {
            case Menu_delete: {
                // 获取选中的项的id，
                // getListAdapter().getItemId(menuInfo.position)返回的值是Long型，
                // 需要转化为int型
                // int id = (int) list.getAdapter().getItemId(position);
                // 获得文件夹下的所有便签记录
                Cursor cursor = dbo.queryFromFolder(FolderActivity.this, _id);
                // 通过获取在LIST中的位置，得到数据库中的记录。
                cursor.moveToPosition(position);
    
                itemId = cursor.getInt(cursor.getColumnIndex(DBOpenHelper.ID));
                // Log.v(TAG,
                // "onContextItemSelected:delete: ****************** position = "+
                // position + ",itemid= " + itemId );
                cursor.close();
    
                deleteNote(itemId);
                // updateDisplay(oldFolderName);
                return true;
            }
    
            case Menu_moveoutFolder: {
                //int id = (int) list.getAdapter().getItemId(position);
                // 获得文件夹下的所有便签记录
                Cursor cursor = dbo.queryFromFolder(FolderActivity.this, _id);
                // 通过获取在LIST中的位置，得到数据库中的记录。
                cursor.moveToPosition(position);
                int noteId = cursor.getInt(cursor.getColumnIndex(DBOpenHelper.ID));
                // 更新该数据项的内容
                String content = cursor.getString(cursor
                        .getColumnIndex(DBOpenHelper.CONTENT));
                // String alarmID =
                // cursor.getString(cursor.getColumnIndex(DBOpenHelper.ALARM_ID));
                
                //wangsl
                int alarmEnable = cursor.getInt(cursor
                        .getColumnIndex(DBOpenHelper.ALARM_ENABLE));
                
                dbo.update(getApplicationContext(), noteId, content, alarmEnable, null,
                        "no");
                //wangsl
                cursor.close();
                updateDisplay(oldFolderName);
                return true;
            }
            case Menu_setAlarm: { // 设置便签闹铃
                Intent intent = new Intent(this, AlarmActivity.class);
                intent.putExtra("noteid", itemId);
                this.startActivityForResult(intent, 1);
    
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
        Intent i = new Intent();
        // 传递打开NoteActivity的方式
        i.putExtra("Open_Type", "newFolderNote");
        // 传递文件夹ID
        i.putExtra("FolderId", _id);
        i.setClass(FolderActivity.this, NoteEditor.class);
        startActivity(i);
    }

    // 修改文件夹名称
    private void updateFolderName() {
        Context mContext = FolderActivity.this;
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(R.string.edit_folder_title);

        LayoutInflater inflater = (LayoutInflater) mContext
                .getSystemService(LAYOUT_INFLATER_SERVICE);
        final View layout = inflater.inflate(R.layout.dialog_layout_new_folder,
                (ViewGroup) findViewById(R.id.dialog_layout_new_folder_root));
        builder.setView(layout);
        // 显示原有文件夹名称
        et_folder_name = (EditText) layout.findViewById(R.id.et_dialog_new_folder);
        et_folder_name.setText(oldFolderName);
        //设置光标位置
        int index = oldFolderName.length();
        et_folder_name.setSelection(index);
        et_folder_name.addTextChangedListener(textwatch);

        builder.setPositiveButton(R.string.Ok,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {

                        String newFolderName = et_folder_name.getText()
                                .toString();
                        // 新文件夹名称不为空,且不等于原有的名称,则更新
                        if (newFolderName.length() > 0 && newFolderName != null
                                && newFolderName != oldFolderName) {

                                dbo.update(FolderActivity.this, _id,
                                        newFolderName, 0, null, "no");
                                oldFolderName = newFolderName;

                            // 通知更新数据源
                            //FolderActivity.this.updateDisplay(oldFolderName);
				updateDisplay(oldFolderName);
                        } else {
                            Toast.makeText(FolderActivity.this,
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
	        Toast.makeText(FolderActivity.this, R.string.toast_edit_outbound,
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

    /*
     * // 删除函数,选择删除文件夹下的便签 private void delete() { Intent i = new
     * Intent(getApplicationContext(), DeleteRecordsActivity.class); // 传递文件夹的ID
     * i.putExtra("folderId", _id); startActivity(i); }
     */

    // 删除单条便签
    private void deleteNote(final int note_id) {
        Context mContext = FolderActivity.this;
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

                        updateDisplay(oldFolderName);
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
    
    // 删除多条便签
    private void deleteAnyNotes() {
       
        Context mContext = FolderActivity.this;
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(R.string.delete_note);
        builder.setPositiveButton(R.string.Ok,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        //删除isSelected数组中为true的项
                        Cursor cursor = dbo.queryFromFolder(FolderActivity.this,_id );
                        int count = cursor.getCount();
                        int selected_count = 0;  // 选择了（勾选）多少项
                        if(count > 0) {  
                            for(int i=0; i<adapter.getCount(); i++) {  
                                if(adapter.isSelected.get(i) != null && adapter.isSelected.get(i)) {
                                    // 通过获取在LIST中的位置，得到数据库中的记录。
                                    cursor.moveToPosition(i);
                                    itemId = cursor.getInt(cursor.getColumnIndex(DBOpenHelper.ID)); 
                                    Log.v("you", "item="+ itemId);                           
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

                        } else{
                            Toast.makeText(getApplicationContext(),
                                    R.string.list_is_empty, Toast.LENGTH_LONG).show();
                        }
                        cursor.close();

                        updateDisplay(oldFolderName);
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
    
    //全部删除时，弹出进度条。
    Handler mHandler = new Handler();
    ProgressDialog mProgressDialog = null;
    Runnable mDeleting = new Runnable() {
        @Override
        public void run() {
            // TODO Auto-generated method stub
            Log.v("you"," progressDialog.show()********** " );  
            String delete= FolderActivity.this.getString(R.string.delete);
            String deleting= FolderActivity.this.getString(R.string.Deleting);           
            mProgressDialog = ProgressDialog.show(FolderActivity.this, delete, deleting);          
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
                  updateDisplay(oldFolderName);
              }          
        }
      
    };  
  
    
    Runnable mDeleteThread = new Runnable() {
        //此为非ui线程，不能处理ui操作。
        @Override
        public void run() {
            // TODO Auto-generated method stub
                    
            //执行全部删除 耗时操作
            //Cursor cursor = dbo.queryFoldersAndNotes(NoteActivity.this);
            Cursor cursor = dbo.queryFromFolder(FolderActivity.this, _id);
            int count = cursor.getCount();
            // 通过获取在LIST中的位置，得到数据库中的记录。
            cursor.moveToFirst();
            Log.d("you","count = " + count);
            if(count > 0) {
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
          Toast.makeText(getApplicationContext(),
          R.string.delete_success, Toast.LENGTH_LONG).show();
        }
    };
    
    Runnable mToast_list_is_empty = new Runnable() {
        @Override
        public void run() {
          Toast.makeText(getApplicationContext(),
          R.string.list_is_empty, Toast.LENGTH_LONG).show();
        }
    }; 
    
    // 删除文件夹下全部便签
    private void deleteAllNotes() {      
        Context mContext = FolderActivity.this;
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
    
    // 负责更新数据
    private void updateDisplay(String folderName) {
        Cursor mCursor = dbo.queryFromFolder(this, _id);
        startManagingCursor(mCursor);
        //adapter = new NoteAdapter(this, mCursor, false);
        if(adapter == null) {
            adapter = new NoteAdapter(FolderActivity.this, mCursor, false);
        } else {  //避免在删除状态时，按HOME键，再次进入便签，checkbox消失
            adapter.setListItems(mCursor);
        }
        adapter.notifyDataSetChanged();
        list.setAdapter(adapter);
        // mCursor.close();
        title.setText(folderName);
    }
}
