package com.sprdroid.note;

import java.util.HashMap;
import java.util.Map;

import com.sprdroid.note.NoteAdapter;
import com.sprdroid.note.DBOpenHelper;
import com.sprdroid.note.DBOperations;

import android.app.Activity;
import android.database.Cursor;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.ViewGroup.LayoutParams;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.AdapterView.OnItemClickListener;

/*
 * 删除记录页面
 */
public class DeleteRecordsActivity extends Activity {
    // 包裹ListView的LinearLayout
    private LinearLayout l;
    private NoteAdapter adapter;
    private ListView listView;
    private Button btnOK, btnCancel;

    private Cursor mCursor;
    private DBOperations dbo;

    private static final String TAG = "DeleteRecordsActivity";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 取消标题栏
        // requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.index_page);

        l = (LinearLayout) findViewById(R.id.page_list);
        // btnOK = (Button) findViewById(R.id.btnOK);
        // btnCancel = (Button) findViewById(R.id.btnCancelDel);
        // 取得屏幕高度
        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        // 设置ListView的高度和宽度
        l.setLayoutParams(new LinearLayout.LayoutParams(
                LayoutParams.FILL_PARENT, dm.heightPixels - 50 - 70));

        dbo = new DBOperations();
        // 判断删除主页的记录还是文件夹下的记录
        int folderId = getIntent().getIntExtra("folderId", -1);
        if (folderId == -1) {
            mCursor = dbo.queryFoldersAndNotes(getApplicationContext());
        } else {
            mCursor = dbo.queryFromFolder(getApplicationContext(), folderId);
        }
        startManagingCursor(mCursor);
        adapter = new NoteAdapter(getApplicationContext(), mCursor, true);

        // btnOK.setOnClickListener(listener);
        // btnCancel.setOnClickListener(listener);
    }
}
