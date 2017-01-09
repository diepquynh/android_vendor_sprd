package com.sprdroid.note;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.os.Bundle;
import android.text.Editable;
import android.text.Layout;
import android.text.Spanned;
import android.text.TextWatcher;
import android.text.style.URLSpan;
import android.util.AttributeSet;
import android.util.Log;
import android.view.ContextMenu;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

/*
 * 一条便签的详细信息页面
 */
public class NoteEditor extends Activity {

    private DBOperations dbo;
    //private ImageButton ib_bgcolor;
    private TextView tv_date_time;
    private EditText et_content;
    // 用户创建便签的日期时间
    private String cdate;
    private String ctime;
    // 用于判断是新建便签还是更新便签
    private String openType;
    // 数据库中原有的便签的内容
    private String oldContent;
    // 接受传递过来的Intent对象
    private Intent intent;
    // 被编辑的便签的ID
    private int _id;
    // 被编辑便签所在的文件夹的ID
    private int folderId;
    // 设置shortcut时使用该字段
    // private final String ACTION_ADD_SHORTCUT =
    // "com.android.launcher.action.INSTALL_SHORTCUT";

    //便签闹铃是否开启
    int alarm_enable= 0;
    private static final String TAG = "NoteEditor";

    TextView remainnum;// 用来显示剩余字数
    int max_num = 1000;// 便签输入限制的最大字数
    int num = 0;
    // private TextWatcher textwatch;
    private Boolean save_success ;
    
	Date date;
	SimpleDateFormat dateFormat;
	String strTimeFormat ="";
    // 菜单
    private static final int Menu_delete = Menu.FIRST;
    private static final int Menu_setAlarm = Menu.FIRST + 1;
    private static final int Menu_send_home = Menu.FIRST + 2;
    private static final int Menu_save = Menu.FIRST + 3;

    /**
     * A custom EditText that draws lines between each line of text that is
     * displayed.
     */
    public static class LinedEditText extends EditText {
        private Rect mRect;
        private Paint mPaint;
        private static final String TAG = "NoteEditText";
        private static final String SCHEME_TEL = "tel:" ;
        private static final String SCHEME_HTTP = "http:" ;
        private static final String SCHEME_EMAIL = "mailto:" ;
        
        private static final Map<String, Integer> sSchemaActionResMap = new HashMap<String, Integer>();
        static {
            sSchemaActionResMap.put(SCHEME_TEL, R.string.tel);
            sSchemaActionResMap.put(SCHEME_HTTP, R.string.web);
            sSchemaActionResMap.put(SCHEME_EMAIL, R.string.email);
        }
//        private OnTextViewChangeListener mOnTextViewChangeListener;
        // we need this constructor for LayoutInflater
        public LinedEditText(Context context, AttributeSet attrs) {
            super(context, attrs);

            setWillNotDraw(false);
            setLinksClickable(false);
            mRect = new Rect();
            mPaint = new Paint();
            mPaint.setStyle(Paint.Style.STROKE);
            mPaint.setColor(0x800000FF);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            int count = getLineCount(); // method entends from TextView.
            Rect r = mRect;
            Paint paint = mPaint;

            for (int i = 0; i < count; i++) {
                // 来获取特定行的基准高度值，而且这个函数第二个参数会返回此行的“外包装”值。再利用这些值绘制这一行的线条.返回的是当前行的左上角Y坐标.
                // method entends from TextView.
                int baseline = getLineBounds(i, r);
                Log.d(TAG, "LinedEditText*******************************"
                        + "onDraw:" + baseline);
                canvas.drawLine(r.left, baseline + 5, r.right, baseline + 5,
                        paint);
            }

            super.onDraw(canvas);
        }
        
        @Override
        public boolean onTouchEvent(MotionEvent event) {
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    int x = (int) event.getX();
                    int y = (int) event.getY();
                    x -= getTotalPaddingLeft();
                    y -= getTotalPaddingTop();
                    x += getScrollX();
                    y += getScrollY();

                    Layout layout = getLayout();
                    int line = layout.getLineForVertical(y);
                    int off = layout.getOffsetForHorizontal(line, x);
                    //Selection.setSelection(getText(), off);
                    
                    break;
            }

            return super.onTouchEvent(event);
        }

       @Override
        protected void onCreateContextMenu(ContextMenu menu) {
            if (getText() instanceof Spanned) {
                int selStart = getSelectionStart();
                int selEnd = getSelectionEnd();

                int min = Math.min(selStart, selEnd);
                int max = Math.max(selStart, selEnd);

                final URLSpan[] urls = ((Spanned) getText()).getSpans(min, max, URLSpan.class);
                if (urls.length == 1) {
                    int defaultResId = 0;
                    for(String schema: sSchemaActionResMap.keySet()) {
                        if(urls[0].getURL().indexOf(schema) >= 0) {
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
                                    urls[0].onClick(LinedEditText.this);
                                    return true;
                                }
                            });
                }
            }
            super.onCreateContextMenu(menu);
        }
    }

     @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 取消标题栏
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.note_editor);

        tv_date_time = (TextView) findViewById(R.id.tv_note_date_time);
        et_content = (EditText) findViewById(R.id.et_content);
        et_content.setAutoLinkMask(0x01|0x02|0x04);
        // et_content.setVisibility(View.GONE);
        et_content.setVisibility(View.VISIBLE);
        et_content.setCursorVisible(true);

        /*
         * if(!et_content.didTouchFocusSelect()) {
         * et_content.setAutoLinkMask(Linkify.ALL);
         * et_content.setMovementMethod(LinkMovementMethod.getInstance()); }
         * //set text selectable
         */
        // et_content.setAutoLinkMask(Linkify.ALL); //自动链接时，不能获得编辑焦点

        dbo = new DBOperations();
        // 得到前一个Activity传递过来的Intent对象
        intent = getIntent();
        // 如果没有传递Intent对象,则返回主页(NoteActivity)
        if (intent.equals(null)) {
            startActivity(new Intent(NoteEditor.this, NoteActivity.class));
        }
        // 取得Open_Type的值,判断是新建便签还是更新便签
        openType = intent.getStringExtra("Open_Type");
        // Log.e(TAG, String.valueOf(openType));
        // 被编辑的便签的ID
        _id = intent.getIntExtra(DBOpenHelper.ID, -1);
        // 得到文件夹的ID(如果从文件夹页面内新建或编辑便签则要求传递文件夹的ID)
        folderId = intent.getIntExtra("FolderId", -1);

        initViews();
        /*
         * //et_content.setSelectAllOnFocus(true) ;
         * et_content.requestFocus(View.FOCUS_DOWN);
         */
        int selection_end = (et_content.getText().toString()).length();
        // 定位光标位置
        et_content.setSelection(selection_end);

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        if (!(openType.equals("newNote") || openType.equals("newFolderNote"))) {
            // 删除(新建便签没有保存到数据库，没有删除菜单项)
            menu.add(Menu.NONE, Menu_delete, 1, R.string.delete).setIcon(
                    R.drawable.delete);
        }
        /*
         * //添加到桌面 
         * menu.add(Menu.NONE, Menu_send_home, 3, R.string.add_shortcut_to_home)
         * .setIcon(R.drawable.add_shortcut_to_home);
         */
        // 闹钟设置
        menu.add(Menu.NONE, Menu_setAlarm, 2, R.string.setAlarm).setIcon(
                R.drawable.alarm_clock);

        // 保存
        menu.add(Menu.NONE, Menu_save, 3, R.string.save).setIcon(
                R.drawable.save);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case Menu_delete:
            deleteNote();
            // finish();
            break;
        /*
         * case Menu_send_home: 
         *      addShortCut(); break;
         */
        case Menu_setAlarm:
            Intent intent = new Intent(this, AlarmActivity.class);
            saveNote();
            Log.d("you", "noteid =" + _id);     
            if( _id == -1 && !save_success) {
                //在便签新建且内容为空，不能进入设置闹铃界面
                Toast.makeText(NoteEditor.this, R.string.input_content,
                        Toast.LENGTH_SHORT).show();
            } else {         

                intent.putExtra("noteid", _id);
                //this.startActivityForResult(intent, 1);
                this.startActivity(intent);
            }
            break;
        case Menu_save:
            saveNote();

            break;
        default:
            break;
        }
        return super.onOptionsItemSelected(item);
    }

    // 初始化组件
    private void initViews() {
        // 显示剩余可输入的字符数
        remainnum = (TextView) findViewById(R.id.remain_num);
          // 取得屏幕宽度 
        /*
          DisplayMetrics dm = new DisplayMetrics();
          getWindowManager().getDefaultDisplay().getMetrics(dm); 
          int width =dm.widthPixels; 
          // 设置TextView对象的宽度 
          tv_date_time.setWidth(width);
         */

        // 判断打开方式
        if (openType.equals("newNote")) {// 新建"顶级便签",即没有放在文件夹内的便签
            // 初始化新建便签的日期时间
            cdate = dbo.getDate();
            ctime = dbo.getTime();

        } else if (openType.equals("editNote")) {// 编辑顶级便签(不在文件夹内的便签)
            // 根据便签的ID查询该便签的详细内容
            Cursor c = dbo.queryOneNote(this, _id);
            c.moveToFirst();

	    //获得编辑之前，便签闹铃是否开启
	    alarm_enable = c.getInt(c.getColumnIndex(DBOpenHelper.ALARM_ENABLE));
            // 最后更新便签 的日期时间及其内容
            oldContent = c.getString(c.getColumnIndex(DBOpenHelper.CONTENT));
            cdate = c.getString(c.getColumnIndex(DBOpenHelper.UPDATE_DATE));
            ctime = c.getString(c.getColumnIndex(DBOpenHelper.UPDATE_TIME));
            et_content.setText(oldContent);
            c.close();
        } else if (openType.equals("newFolderNote")) {// 在某文件夹下新建便签
            // 初始化新建便签的日期时间
            cdate = dbo.getDate();
            ctime = dbo.getTime();
        } else if (openType.equals("editFolderNote")) {// 编辑某文件夹下的便签
            Cursor c = dbo.queryOneNote(this, _id);
            c.moveToFirst();
	    //获得编辑之前，便签闹铃是否开启
	    alarm_enable = c.getInt(c.getColumnIndex(DBOpenHelper.ALARM_ENABLE));
            // 最后更新便签的日期时间及其内容
            oldContent = c.getString(c.getColumnIndex(DBOpenHelper.CONTENT));
            cdate = c.getString(c.getColumnIndex(DBOpenHelper.UPDATE_DATE));
            ctime = c.getString(c.getColumnIndex(DBOpenHelper.UPDATE_TIME));
            et_content.setText(oldContent);
            c.close();
        }
        Log.v(TAG, "  NoteEditor : initview()*****************");

        // 以11-09-09 9:10的样式显示日期时间
        //tv_date_time.setText(cdate.substring(0) + " \t" + ctime.substring(0, 5));
        //tv_date_time.setBackgroundResource(R.drawable.date_time);
		ContentResolver cv = this.getContentResolver();
		strTimeFormat = android.provider.Settings.System.getString(cv,
				android.provider.Settings.System.TIME_12_24);

		String strCtime = new String(ctime.toString());
        Log.v("you", " strTimeFormat ="+ strTimeFormat);		
        Log.v("you", " ctime ="+ ctime);		
		if (strTimeFormat != null && strTimeFormat.equals("12")) {
			// 获取小时数
			strCtime = strCtime.substring(0, 5);
			String strhour = strCtime.substring(0, 2);
			int hour = Integer.parseInt(strhour);
			// 判断是上午还是下午			
			if (hour >= 12) {
				if (hour > 12) {
					hour = hour % 12;
					strCtime = hour + strCtime.substring(2, 5)
							+ getString(R.string.PM);
				} else {

					strCtime = strCtime.substring(0, 5)
					+ getString(R.string.PM);
				}

			} else {
				if(hour == 0)
					hour = 12;
				strCtime = hour + strCtime.substring(2, 5)
					+ getString(R.string.AM);
			}

			if (strCtime.substring(0, 2).equals("12")
					|| strCtime.substring(0, 2).equals("11")
					|| strCtime.substring(0, 2).equals("10")) {
				if (strCtime.contains("上午")) {
					strCtime = getString(R.string.AM)
							+ strCtime.substring(0, 5);
				} else if (strCtime.contains("下午")) {
					strCtime = getString(R.string.PM)
							+ strCtime.substring(0, 5);
				}
			} else {
				if (strCtime.contains("上午")) {
					strCtime = getString(R.string.AM)
							+ strCtime.substring(0, 4);
				} else if (strCtime.contains("下午")) {
					strCtime = getString(R.string.PM)
							+ strCtime.substring(0, 4);
				}
			}
		} else {
			strCtime = strCtime.substring(0, 5);
		}
		String strDateFormat = android.provider.Settings.System.getString(cv,
				android.provider.Settings.System.DATE_FORMAT);

		date = new Date();
		if (strDateFormat != null)
		dateFormat = new SimpleDateFormat(strDateFormat);
		Log.i("Date",
				Integer.parseInt(cdate.substring(0, 4)) + " "
						+ Integer.parseInt(cdate.substring(5, 7)) + " "
						+ Integer.parseInt(cdate.substring(8, 10)));
		date.setYear(Integer.parseInt(cdate.substring(0, 4)) - 1900);// 0代表1900年
		date.setMonth(Integer.parseInt(cdate.substring(5, 7)) - 1);// 从0开始
		date.setDate(Integer.parseInt(cdate.substring(8, 10)));
		String str= "";
		if (dateFormat != null)
		str = dateFormat.format(date);

		if (!str.equals("")) {
			tv_date_time.setText(str + "\t" +" "+ strCtime);
			tv_date_time.setBackgroundResource(R.drawable.date_time);
		} else {
			tv_date_time.setText(cdate.substring(0) + "\t" +" "+ strCtime);
			tv_date_time.setBackgroundResource(R.drawable.date_time);
		}

        // 获得便签的字数
        if (oldContent != null)
            num = oldContent.length();
        remainnum.setText(num + "" + "/" + max_num);
        remainnum.setBackgroundResource(R.drawable.date_time);

        et_content.setBackgroundResource(R.drawable.et_content_bg);
        // 为EditText文本框添加监听
        et_content.addTextChangedListener(textwatch);
    }

    TextWatcher textwatch = new TextWatcher() {
        private CharSequence temp;
        private int selectionStart;
        private int selectionEnd;

        public void beforeTextChanged(CharSequence s, int start, int count,
                int after) {
            // temp = s;
        }

        public void onTextChanged(CharSequence s, int start, int before,
                int count) {
            temp = s;
            // TextView.setText(s);
        }

        public void afterTextChanged(Editable s) {
            int number = s.length();
            remainnum.setText("" + number + "/" + max_num);

            selectionStart = et_content.getSelectionStart();
            selectionEnd = et_content.getSelectionEnd();
            if (temp.length() > max_num) {
                Toast.makeText(NoteEditor.this, R.string.toast_edit_outbound,
                        Toast.LENGTH_SHORT).show();
                Log.v("you", " selectionStart=" + selectionStart
                        + " ; selectionEnd =" + selectionEnd);
                // 截断超出的内容
                int i = temp.length() - max_num; // 超出多少字符
                s.delete(selectionStart - i, selectionEnd);
                int tempSelection = selectionEnd;
                et_content.setText(s);
                et_content.setSelection(tempSelection); // 设置光标在最后
            }
        }
    };

    @Override
    protected void onPause() {
        super.onPause();
        saveNote();
    }

    @Override
    protected void onDestroy() {
        //dbo.close();
        super.onDestroy();

    }

    @Override
    protected void onResume() {
        super.onResume();
        //这句会导致编辑界面被打断，重新进入后，按back键使listitem获得高亮
        //this.getWindow().peekDecorView().requestFocusFromTouch();
       et_content.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub				
				 et_content.setEnabled(true);
				 et_content.requestFocus();
			}
		});
        Log.v("you", " openType =" + openType);
        // openType = "editNote";
    }

    // 删除便签
    private void deleteNote() {
        Context mContext = NoteEditor.this;
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(R.string.delete_note);

        builder.setPositiveButton(R.string.Ok,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        Log.v("you", " _id =" + _id);
                        if (_id > 0) { // 避免便签不存在时（新建未保存时）删除异常
                            dbo.delete(NoteEditor.this, _id);
                        }
                        /*
                         * // 返回上一级 Intent intent = new Intent(); if
                         * (openType.equals("editNote")||
                         * openType.equals("newNote")) { // 显示主页
                         * intent.setClass(NoteEditor.this, NoteActivity.class);
                         * } else if (openType.equals("editFolderNote") ||
                         * openType.equals("newFolderNote")) { // 显示便签所属文件夹下页面
                         * intent.putExtra(DBOpenHelper.ID, folderId);
                         * intent.setClass(NoteEditor.this,
                         * FolderActivity.class); } startActivity(intent);
                         */
                        finish();
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

    // 保存便签
    private void saveNote() {
        // 得到EditText中当前的内容
        String content = et_content.getText().toString();
        // 判断是更新还是新建便签
        if (openType.equals("newNote")) { // 创建主页上的便签(顶级便签)
            if ((content.length() > 0) && (content != null)) { // 不为空,插入记录
                _id = dbo.insert(this, content, cdate, ctime, null, null, "no", "no");
                openType = "editNote";
                Toast.makeText(NoteEditor.this, R.string.save_success,
                        Toast.LENGTH_SHORT).show();
                save_success = true;
            } else {
                // 如果编辑的内容为空，提示保存失败！
                Toast.makeText(NoteEditor.this, R.string.save_failed,
                        Toast.LENGTH_SHORT).show();
                save_success = false;
            }
        } else if (openType.equals("editNote")) { // 编辑主页上的便签
            Log.v("you", " oldContent =" + oldContent);
            if (!content.equals(oldContent) && (content.length() > 0)
                    && (content != null)) { // 内容不等于旧记录的内容且不为空,更新记录

                Log.v("you", "_id =" + _id + " ; content =" + content);
                dbo.update(this, _id, content, alarm_enable, null, "no");
                oldContent = content;
                Toast.makeText(NoteEditor.this, R.string.save_success,
                        Toast.LENGTH_SHORT).show();
                save_success = true;
            } else if (content.length() == 0) {
                // 如果编辑的内容为空，提示保存失败！
                Toast.makeText(NoteEditor.this, R.string.save_failed,
                        Toast.LENGTH_SHORT).show();
                save_success = false;
            }
        } else if (openType.equals("newFolderNote")) { // 创建文件夹下的便签
            if ((content.length() > 0) && (content != null)) {
                _id = dbo.insert(this, content, cdate, ctime, null, null, "no",
                        String.valueOf(folderId));
                openType = "editFolderNote";
                Toast.makeText(NoteEditor.this, R.string.save_success,
                        Toast.LENGTH_SHORT).show();
                save_success = true;
            } else {
                // 如果编辑的内容为空，提示保存失败！
                Toast.makeText(NoteEditor.this, R.string.save_failed,
                        Toast.LENGTH_SHORT).show();
                save_success = false;
            }
        } else if (openType.equals("editFolderNote")) { // 更新文件夹下的便签
            if (!content.equals(oldContent) && (content.length() > 0)
                    && (content != null)) {
                // 更新记录
                dbo.update(this, _id, content, alarm_enable, null,
                        String.valueOf(folderId));
                oldContent = content;
                Toast.makeText(NoteEditor.this, R.string.save_success,
                        Toast.LENGTH_SHORT).show();
                save_success = true;
            } else if (content.length() == 0) {
                // 如果编辑的内容为空，提示保存失败！
                Toast.makeText(NoteEditor.this, R.string.save_failed,
                        Toast.LENGTH_SHORT).show();
                save_success = false;
            }

        }
    }

    // 添加到左面快捷方式，留待以后完善
    private void addShortCut() {

    }

}
