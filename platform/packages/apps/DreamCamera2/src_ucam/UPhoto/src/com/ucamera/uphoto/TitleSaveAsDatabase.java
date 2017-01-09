/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.os.Environment;

import com.ucamera.uphoto.brush.BrushConstant;

import java.io.File;

/**
 * save common title into the database
 *
 */
public class TitleSaveAsDatabase {
    private static final String TAG = "TitleSaveAsDatabase";
    private static final String DATABASE_NAME = "title_save_as.db";
//    private static final int DATABASE_VERSION = 1;

    ///////////////title//////////////
    private static final String TABLE_TITLE_NAME = "common_title";

    private static final String KEY_TEXT_TITLE_ID = "titleId";
    private static final String KEY_TEXT_COLOR = "textColor";
    private static final String KEY_GRADIENT_END = "gradientEnd";
    private static final String KEY_OUTLINE = "outline";
    private static final String KEY_STROKE_WIDTH = "strokeWidth";
    private static final String KEY_TEXT_FONT = "textFont";
    private static final String KEY_INPUT_TEXT = "inputText";
    private static final String KEY_TYPE = "type";
    private static final String KEY_LABEL = "label";
    private static final String KEY_LABEL_HEAD = "labelHead";
    private static final String KEY_LABEL_TAIL = "labelTail";
    private static final String KEY_PANEL_FILL = "panelFill";
    private static final String KEY_BALLOON_STYLE = "balloonStyle";
    private static final String[] TITLE_PROJECTS = {KEY_TEXT_TITLE_ID, KEY_TEXT_COLOR, KEY_GRADIENT_END, KEY_OUTLINE, KEY_STROKE_WIDTH, KEY_TEXT_FONT, KEY_INPUT_TEXT, KEY_TYPE,KEY_LABEL,KEY_LABEL_HEAD,KEY_LABEL_TAIL,KEY_PANEL_FILL,KEY_BALLOON_STYLE};

    //////////////////brush/////////////////
    private static final String TABLE_BRUSH_NAME = "common_brush";

    private static final String KEY_TEXT_BRUSH_ID = "brushId";
    private static final String KEY_TEXT_BRUSH_STYLE = "brushStyle";
    private static final String KEY_TEXT_BRUSH_SIZE = "brushSize";
    private static final String KEY_TEXT_BRUSH_COLOR = "brushColor";
    private static final String KEY_TEXT_BRUSH_MODE = "brushMode";
    private static final String[] BRUSH_PROJECTS = {KEY_TEXT_BRUSH_ID, KEY_TEXT_BRUSH_STYLE, KEY_TEXT_BRUSH_SIZE, KEY_TEXT_BRUSH_COLOR, KEY_TEXT_BRUSH_MODE};

    private String TABLE_TITLE_CREATE = "CREATE TABLE IF NOT EXISTS common_title (" +
            "_id integer primary key autoincrement, " +
            "titleId long not null, " +
            "textColor text not null, " +
            "gradientEnd text, " +
            "outline text, " +
            "strokeWidth text, " +
            "textFont text not null, " +
            "inputText text not null);";

    private String TABLE_BRUSH_CREATE = "CREATE TABLE IF NOT EXISTS common_brush (" +
            "_id integer primary key autoincrement, " +
            "brushId long not null," +
            "brushStyle integer, " +
            "brushSize integer, " +
            "brushColor text, " +
            "brushMode integer);";

    private SQLiteDatabase mTitleDatabase;

    public TitleSaveAsDatabase(Context context) {
        File dbfile = context.getDatabasePath(DATABASE_NAME);
        if (!dbfile.exists()) {
            dbfile.getParentFile().mkdirs();
        }
        mTitleDatabase = SQLiteDatabase.openOrCreateDatabase(dbfile, null);
        if(mTitleDatabase != null) {
            if(mTitleDatabase.getVersion() == 0) {
                mTitleDatabase.execSQL(TABLE_TITLE_CREATE);
                if(!com.ucamera.ucam.variant.Build.isSourceNext()) {
                    insertTitle(System.currentTimeMillis(), "#9d5eff", "#dec9ff", "#281500", 2, "NothingYouCouldDo.ttf", "Powered by UCam");
                }

                mTitleDatabase.setVersion(1);
            }
            if(mTitleDatabase.getVersion() == 1) {
                mTitleDatabase.execSQL(TABLE_BRUSH_CREATE);
                insertBrush(System.currentTimeMillis(), BrushConstant.RainbowBrush, 8, "#ffe295", BrushConstant.BrushModeSelected);
                insertBrush(System.currentTimeMillis(), BrushConstant.PenInkBrush, 8, "#fff799", BrushConstant.BrushModeSelected);
                insertBrush(System.currentTimeMillis(), BrushConstant.GradientBrush, 8, "#f69679", BrushConstant.BrushModeSelected);
                insertBrush(System.currentTimeMillis(), BrushConstant.NeonBrush, 5, "#6acff6", BrushConstant.BrushModeSelected);
                insertBrush(System.currentTimeMillis(), BrushConstant.RectThornsBrush, 50, "#7FFFD4", BrushConstant.BrushModeSelected);

                mTitleDatabase.setVersion(2);
            }
            if(mTitleDatabase.getVersion() == 2) {
                if(!com.ucamera.ucam.variant.Build.isSourceNext()) {
                    insertTitle(System.currentTimeMillis(), "#66aa6f", "0", "#ffffff", 2, "Knewave-Regular.ttf", "Powered by UCam");
                }
                mTitleDatabase.setVersion(3);
            }
            if(mTitleDatabase.getVersion() == 3) {
                String sql[] = new String[]{
                    "type text",
                    "label text",
                    "labelHead text",
                    "labelTail text",
                    "panelFill text",
                    "balloonStyle text"
                };
                for(int i=0;i<sql.length;i++) {
                    try{
                        mTitleDatabase.execSQL("ALTER TABLE common_title ADD COLUMN " +sql[i]);
                    }catch(SQLiteException e){
                        LogUtils.error(TAG, "SQLiteException : "+e);
                    }
                }
                mTitleDatabase.setVersion(4);
            }
        }
    }

    public void closeDatabase() {
        if(mTitleDatabase != null) {
            mTitleDatabase.close();
        }
    }

    public boolean isOpened() {
        if(mTitleDatabase != null) {
            return mTitleDatabase.isOpen();
        }

        return false;
    }

    public boolean insertTitle(ViewAttributes attributes, int type) {
        return insertTitle(attributes.getTitleId(), attributes.getTextColor(), attributes.getGradientEnd(), attributes.getOutline(),
                attributes.getStrokeWidth(), attributes.getFont(), attributes.getDrawText(), type, attributes.getLabel(), attributes.getLabelHead(), attributes.getLabelTail(), attributes.getPanelFill(), String.valueOf(attributes.getBalloonStyle()));
    }

    public boolean insertTitle(long titleId, String textColor, String gradientEnd, String outline, int strokeWidth, String textFont,
String inputText, int type, String label, String labelHead, String labelTail, String panelFill, String balloonStyle) {
        ContentValues values = new ContentValues(12);
        values.put(KEY_TEXT_TITLE_ID, titleId);
        values.put(KEY_TEXT_COLOR, textColor);
        values.put(KEY_GRADIENT_END, gradientEnd);
        values.put(KEY_OUTLINE, outline);
        values.put(KEY_STROKE_WIDTH, strokeWidth);
        values.put(KEY_TEXT_FONT, textFont);
        values.put(KEY_INPUT_TEXT, inputText);
        values.put(KEY_TYPE, type);
        values.put(KEY_LABEL, label);
        values.put(KEY_LABEL_HEAD, labelHead);
        values.put(KEY_LABEL_TAIL, labelTail);
        values.put(KEY_PANEL_FILL, panelFill);
        values.put(KEY_BALLOON_STYLE, balloonStyle);

        long insertRow = mTitleDatabase.insertOrThrow(TABLE_TITLE_NAME, null, values);
        if(insertRow > 0) {
            return true;
        }

        return false;
    }

    public boolean insertTitle(long titleId, String textColor, String gradientEnd, String outline, int strokeWidth, String textFont,
            String inputText) {
                    ContentValues values = new ContentValues(12);
                    values.put(KEY_TEXT_TITLE_ID, titleId);
                    values.put(KEY_TEXT_COLOR, textColor);
                    values.put(KEY_GRADIENT_END, gradientEnd);
                    values.put(KEY_OUTLINE, outline);
                    values.put(KEY_STROKE_WIDTH, strokeWidth);
                    values.put(KEY_TEXT_FONT, textFont);
                    values.put(KEY_INPUT_TEXT, inputText);

                    long insertRow = mTitleDatabase.insertOrThrow(TABLE_TITLE_NAME, null, values);
                    if(insertRow > 0) {
                        return true;
                    }
                    return false;
                }

    public boolean deleteTitle(ViewAttributes attributes) {
        int deleteRowId = mTitleDatabase.delete(TABLE_TITLE_NAME, KEY_TEXT_TITLE_ID + "=" + attributes.getTitleId(), null);
        if(deleteRowId > 0) {
            return true;
        }

        return false;
    }

    public Cursor getAllTitles() {
        return mTitleDatabase.query(TABLE_TITLE_NAME, TITLE_PROJECTS, null, null, null, null, null);
    }

    public Cursor getTitleById(ViewAttributes attributes) {
        Cursor cursor = mTitleDatabase.query(TABLE_TITLE_NAME, TITLE_PROJECTS, KEY_TEXT_TITLE_ID + " = " + attributes.getTitleId(), null, null, null, null);
        if(cursor != null) {
            cursor.moveToFirst();
        }

        return cursor;
    }


    public boolean insertBrush(long brushId, int brushStyle, int brushSize, String brushColor, int brushMode) {
        ContentValues values = new ContentValues(5);
        values.put(KEY_TEXT_BRUSH_ID, brushId);
        values.put(KEY_TEXT_BRUSH_STYLE, brushStyle);
        values.put(KEY_TEXT_BRUSH_SIZE, brushSize);
        values.put(KEY_TEXT_BRUSH_COLOR, brushColor);
        values.put(KEY_TEXT_BRUSH_MODE, brushMode);

        long insertRow = mTitleDatabase.insertOrThrow(TABLE_BRUSH_NAME, null, values);
        if(insertRow > 0) {
            return true;
        }

        return false;
    }

    public boolean deleteBrush(long brushId) {
        int deleteRowId = mTitleDatabase.delete(TABLE_BRUSH_NAME, KEY_TEXT_BRUSH_ID + "=" + brushId, null);
        if(deleteRowId > 0) {
            return true;
        }

        return false;
    }

    public Cursor getAllBrushes() {
        return mTitleDatabase.query(TABLE_BRUSH_NAME, BRUSH_PROJECTS, null, null, null, null, null);
    }

    public Cursor getBrushById(int brushId) {
        Cursor cursor = mTitleDatabase.query(TABLE_BRUSH_NAME, BRUSH_PROJECTS, KEY_TEXT_BRUSH_ID + " = " + brushId, null, null, null, null);
        if(cursor != null) {
            cursor.moveToFirst();
        }

        return cursor;
    }
}
