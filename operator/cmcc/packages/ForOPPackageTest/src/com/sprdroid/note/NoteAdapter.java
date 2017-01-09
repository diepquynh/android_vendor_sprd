package com.sprdroid.note;

import java.util.HashMap;
import java.util.Map;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.util.Log;

import com.sprdroid.note.ListItemView;
import com.sprdroid.note.R;
import com.sprdroid.note.DBOpenHelper;
import com.sprdroid.note.DBOperations;
//import com.sprdroid.note.MusicPicker.MusicListAdapter.ViewHolder;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.ImageView;
import android.widget.AdapterView.OnItemClickListener;

public class NoteAdapter extends BaseAdapter {

    // wangsl
    public static final int SHOW_TYPE_NORMAL = 0;
    public static final int SHOW_TYPE_DELETE = 1;
    // wangsl

    private Cursor listItems; // 数据库中的记录集
    // This class is used to instantiate layout XML file into its corresponding
    // View objects.
    private LayoutInflater listContainer; // 视图容器
    private Context mcontext;
    // 便签项缩略显示字符数
    public static final int MAX_NUM = 16;

//    DBOperations dbo;
//    Cursor clk_cursor;
    public Map<Integer, Boolean> isSelected;

	Date date;
	SimpleDateFormat dateFormat;
    private int mShowType;

    // 构造器
    public NoteAdapter(Context context, Cursor listItems, boolean isDelOrMove) {
        listContainer = LayoutInflater.from(context); // 创建视图容器并设置上下文
        this.listItems = listItems;
        this.mcontext = context;

//        dbo = new DBOperations();
        // 这儿定义isSelected这个map是记录每个listitem的状态，初始状态全部为false。
        isSelected = new HashMap<Integer, Boolean>();
        for (int i = 0; i < getCount(); i++) {
            isSelected.put(i, false);
        }

        mShowType = SHOW_TYPE_NORMAL;
    }

    public void setShowType(int type) {
        mShowType = type;
        if (type == SHOW_TYPE_DELETE) {
            isSelected.clear();
        }
    }

    public int getShowType() {
        return mShowType;
    }

    @Override
    public int getCount() {
		if (listItems != null) {
			return listItems.getCount();
		}
    	return 0;
    }

    @Override
    public Object getItem(int position) {
        return null;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    public void setListItems(Cursor listItems) {
        this.listItems = listItems;
//        isSelected.clear();
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        // 自定义视图
        Log.d("wangsl", "getView Start,position = " + position);
        ListItemView listItemView = null;
        boolean isNeedCreateView = false;

        if (isSelected.get(position) == null) {
            Log.d("wangsl", "position is " + position);
            isSelected.put(position, false);
        }

        if (convertView == null) {
            isNeedCreateView = true;
        } else {
            listItemView = (ListItemView) convertView.getTag();
            if (listItemView.position != position) {
                isNeedCreateView = true;
            }
        }

        if (isNeedCreateView) {
            listItemView = new ListItemView();
            Log.d("wangsl", "getView trace1 ");

            // 获取listview中每个item的布局文件的视图
            convertView = listContainer.inflate(R.layout.listview_item_layout,
                    null);
            // 获取控件对象
            listItemView.tv_left = (TextView) convertView
                    .findViewById(R.id.tv_left);
            listItemView.tv_right = (TextView) convertView
                    .findViewById(R.id.tv_right);
            listItemView.linearlayout = (LinearLayout) convertView
                    .findViewById(R.id.listview_linearlayout);
            // 如果有设置闹钟，显示闹钟图标
            listItemView.clock_image = (ImageView) convertView
                    .findViewById(R.id.clock_image);

            listItemView.check = (CheckBox) convertView
                    .findViewById(R.id.check);

            // wangsl
            listItemView.position = position;
            // 设置勾选状态
            // Log.d("wangsl", "listItemView.check="+ listItemView.check);
            // Log.d("wangsl", "isSelected=" + isSelected);
            // Log.d("wangsl", "isSelected.get(position)=" +isSelected.get(position) );
            // Log.d("wangsl", "position=" + position );

            listItemView.check.setChecked(isSelected.get(position));
            // wangsl
            // 设置控件集到convertView
            convertView.setTag(listItemView);
        } else {
            listItemView = (ListItemView) convertView.getTag();
        }

        // wangsl
        Log.d("wangsl", "mShowType is " + mShowType);
        if (mShowType == SHOW_TYPE_NORMAL) {
            listItemView.tv_right.setVisibility(View.VISIBLE);
            listItemView.check.setVisibility(View.GONE);
        } else {
            listItemView.tv_right.setVisibility(View.GONE);
            listItemView.check.setVisibility(View.VISIBLE);
            // listItemView.check.setFocusable(true);
            listItemView.check.setClickable(true);
            // listItemView.check.setFocusableInTouchMode(true);
            // listItemView.check.requestFocusFromTouch();
        }
        // wangsl

        // 将Cursor对象移到position,取出该位置上的数据
        listItems.moveToPosition(position);
        // 取出字段的值,判断该记录是否为文件夹
        String is_Folder = listItems.getString(listItems
                .getColumnIndex(DBOpenHelper.IS_FOLDER));
        // 设置标题(或内容)
        String content = listItems.getString(listItems
                .getColumnIndex(DBOpenHelper.CONTENT));
        // 获得闹铃状态：是否打开
        int clock_enable = listItems.getInt(listItems
                .getColumnIndex(DBOpenHelper.ALARM_ENABLE));
        Log.v("you", " clock_enable=" +clock_enable);
        
//		if (mShowType == SHOW_TYPE_DELETE) 
//		{
//			listItemView.linearlayout
//					.setOnClickListener(new listOnClickListener(listItemView));
//		}
        listItemView.check.setOnCheckedChangeListener(new deleteCheckListener(
                position));
        // clk_cursor.close();
        // clk_cursor =null;
        // dbo.close();

        if (is_Folder.equals("no")) { // 不是文件夹
            // 应该判断数据库中颜色字段的值,根据该值设置相应的背景图片
            listItemView.linearlayout
                    .setBackgroundResource(R.drawable.item_light_blue);
            if (content.length() > MAX_NUM) {// 如果内容太长,则采用如下方式显示
                listItemView.tv_left.setText(content.substring(0, MAX_NUM - 1)
                        + "...");
            } else {
                listItemView.tv_left.setText(content);
            }

            // 设置闹铃图标
            //if (clock_enable != null) {
                if (clock_enable == 1) {
                    Log.v("you", " ************setclockimage");

                    listItemView.clock_image.setImageResource(R.drawable.clock);
                    listItemView.clock_image.setVisibility(View.VISIBLE);
                } else {
                    // listItemView.clock_image.setVisable(false);
                    listItemView.clock_image.setVisibility(View.INVISIBLE);
                }
            //} else {
            //    listItemView.clock_image.setVisibility(View.INVISIBLE);
            //}

        } else if (is_Folder.equals("yes")) { // 是文件夹
            listItemView.linearlayout
                    .setBackgroundResource(R.drawable.folder_background);
            // 查询某文件夹下的所有便签项
            int folderID = listItems.getInt(listItems
                    .getColumnIndex(DBOpenHelper.ID));
            DBOperations dbo1 = new DBOperations();
            Cursor cursor = dbo1.queryFromFolder(mcontext, folderID);
            int count = cursor.getCount();
            listItemView.tv_left.setText(content + "(" + count + ")");
			if (cursor != null) {
				cursor.close();
				cursor = null;
			}
            // 不显示闹钟图标
            listItemView.clock_image.setVisibility(View.INVISIBLE);
        }

        // 使用TextView来显示记录的创建或最后更新时间
        /*
        listItemView.tv_right.setText(listItems.getString(
                listItems.getColumnIndex(DBOpenHelper.UPDATE_DATE))
                .substring(2) + "\t"
                + listItems.getString(
                        listItems.getColumnIndex(DBOpenHelper.UPDATE_TIME))
                        .substring(0, 5));
        Log.v("you", " listitem updata date: " + listItems.getString(listItems.getColumnIndex(DBOpenHelper.UPDATE_DATE)));
         */
        
		ContentResolver cv = mcontext.getContentResolver();
				
		String strTimeFormat = android.provider.Settings.System.getString(cv, 
				android.provider.Settings.System.TIME_12_24);
		String ctime;
		if (strTimeFormat != null && strTimeFormat.equals("12")) {
			ctime = listItems.getString(
					listItems.getColumnIndex(DBOpenHelper.UPDATE_TIME))
					.substring(0, 5);
			//if (!ctime.contains("AM") || !ctime.contains("PM")) {
				String strhour = listItems.getString(
						listItems.getColumnIndex(DBOpenHelper.UPDATE_TIME))
						.substring(0, 2);
				int hour = Integer.parseInt(strhour);
				if (hour >= 12) {
					if (hour > 12){
						hour = hour % 12;
						ctime = hour + ctime.substring(2, 5)
								+ mcontext.getString(R.string.PM);
					}
					else{
						ctime = ctime.substring(0, 5)
								+ mcontext.getString(R.string.PM);
					}

				} else {
					if(hour == 0)
						hour = 12;
					ctime = hour + ctime.substring(2, 5)
							+ mcontext.getString(R.string.AM);
				}
				//对时间格式“上午”“下午”移到前面
				if (ctime.substring(0, 2).equals("12")
						|| ctime.substring(0, 2).equals("11")
						|| ctime.substring(0, 2).equals("10")) {
					if (ctime.contains("上午")) {
						ctime = "       " + mcontext.getString(R.string.AM)
								+ ctime.substring(0, 5);
					} else if (ctime.contains("下午")) {
						ctime = "       " + mcontext.getString(R.string.PM)
								+ ctime.substring(0, 5);
					}
				} else {
					if (ctime.contains("上午")) {
						ctime = "       " + mcontext.getString(R.string.AM)
								+ ctime.substring(0, 4);
					} else if (ctime.contains("下午")) {
						ctime = "       " + mcontext.getString(R.string.PM)
								+ ctime.substring(0, 4);
					}
				}

		} else {
			ctime = listItems.getString(listItems
					.getColumnIndex(DBOpenHelper.UPDATE_TIME)).substring(0, 5);
		}

		String strDateFormat = android.provider.Settings.System.getString(cv,
				android.provider.Settings.System.DATE_FORMAT);

		if (strDateFormat != null)
		dateFormat = new SimpleDateFormat(strDateFormat);
		String olddate = listItems.getString(listItems
				.getColumnIndex(DBOpenHelper.UPDATE_DATE));
		Log.i("olddate",
				Integer.parseInt(olddate.substring(0, 4)) + " "
						+ Integer.parseInt(olddate.substring(5, 7)) + " "
						+ Integer.parseInt(olddate.substring(8, 10)));
		date = new Date(Integer.parseInt(olddate.substring(0, 4)) - 1900,
				Integer.parseInt(olddate.substring(5, 7)) - 1,
				Integer.parseInt(olddate.substring(8, 10)));
		String str= "";
		if(dateFormat != null)
		str = dateFormat.format(date);
		if (!str.equals("")) {//假如采用普通Short格式 如2012-07-21 写成2012-7-21，
			                  //此格式用format后str似乎得不到。得到的str应该为"".此处绕了很多弯路
			listItemView.tv_right.setText(str
					+ "\t"
					+ ctime);

		} else {
			listItemView.tv_right.setText(listItems.getString(
					listItems.getColumnIndex(DBOpenHelper.UPDATE_DATE))
					.substring(0)
					+ "\t"
					+ ctime);
		}
        // listItems.close();
        return convertView;
    }
    
    
    class deleteCheckListener implements OnCheckedChangeListener {
        private int mPosition = -1;

        public deleteCheckListener(int position) {
            mPosition = position;
        }

        @Override
        public void onCheckedChanged(CompoundButton buttonView,
                boolean isChecked) {
            // if (isChecked) {
            // TODO
            isSelected.put(mPosition, isChecked);
            // } else if (!isChecked) {
            // TODO
            // }
        }
    }
    
    class listOnClickListener implements OnClickListener
    {
    	ListItemView listItemView;
    	
    	listOnClickListener(ListItemView listItemView)
    	{
    		this.listItemView = listItemView;
    	}
		@Override
		public void onClick(View v) {
			// TODO Auto-generated method stub
			if(listItemView.check.isChecked())
				listItemView.check.setChecked(false);
			else
				listItemView.check.setChecked(true);
		}
    }
    
}
