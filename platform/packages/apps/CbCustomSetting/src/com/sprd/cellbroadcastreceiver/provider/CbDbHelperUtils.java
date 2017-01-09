package com.sprd.cellbroadcastreceiver.provider;

import android.content.ContentValues;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.XmlResourceParser;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteOpenHelper;

import android.util.Log;

import com.sprd.cellbroadcastreceiver.provider.CellbroadcastDefine;
import com.sprd.cellbroadcastreceiver.provider.ChannelTableDefine;
import com.sprd.cellbroadcastreceiver.provider.CreateLangViewDefine;
import com.sprd.cellbroadcastreceiver.provider.LangMapTableDefine;
import com.sprd.cellbroadcastreceiver.provider.LangNameTableDefine;
import com.sprd.cellbroadcastreceiver.provider.MncMccTableDefine;
import com.sprd.cellbroadcastreceiver.provider.CommonSettingTableDefine;
//import com.sprd.cellbroadcastreceiver.provider.IcbDbHelperUtils;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Collections;
import org.xmlpull.v1.XmlPullParser;

import dalvik.system.DexClassLoader;

import android.content.Context;
import android.util.Log;
import android.util.Xml;

public class CbDbHelperUtils{    
    private static final String TAG = "CbDbHelperUtils";
    private final String[] mLangNames = new String[]{
            "german",
            "english",
            "italian",
            "french",
            "spanish",
            "dutch",
            "swedish",
            "danish",
            "portuguese",
            "finnish",
            "norwegian",
            "greek",
            "turkish",
            "hungarian",
            "polish",
            "czech",
            "hebrew",
            "arabic",
            "russian",
            "icelandic"
    };
            
    private void initLangNameData(SQLiteDatabase db, String[]langNames){
        if(langNames == null || langNames.length <= 0){
            Log.d(TAG, "langNames data is null !!! ");
            return;
        }
        int show = 0;
        int enable = 0;
        String description = "";
        ContentValues value = new ContentValues();
        Log.d(TAG, "langNames data is not null !!! ");
        for(int langId=0; langId<langNames.length;){
            description = langNames[langId];
            enable = 0;
            if("english".equals(description)){
                enable = 1;
                Log.d(TAG, "initLangNameData description= " + description);
            }
            show = 1;
            langId++;
            value.put(LangMapTableDefine.LANG_ID, langId);
            value.put(LangMapTableDefine.MNC_MCC_ID, 1);
            value.put(LangMapTableDefine.ENABLE, enable);
            value.put(LangMapTableDefine.SHOW, show);
            value.put(LangMapTableDefine.SUBID, -1);
            db.insert(LangMapTableDefine.LANG_MAP_TABLE_NAME, "",
                    value);
            value.clear();
            value.put(LangNameTableDefine.LANG_NAME_ID, langId);
            value.put(LangNameTableDefine.DESCRIPTION, description);
            db.insert(LangNameTableDefine.LANG_NAME_TABLE_NAME, "",
                    value);
            value.clear();
        }
        //value.clear();
        value.put(MncMccTableDefine.MNC, 1);
        value.put(MncMccTableDefine.MCC, 460);
        value.put(MncMccTableDefine.MNCMCC_ID, 1);
        db.insert(MncMccTableDefine.MNC_MCC_TABLE_NAME, "", value);
        
    }
    
    public void SqlCreatCbCustomTables(SQLiteDatabase db){
        db.execSQL("ALTER TABLE " + CellbroadcastDefine.TABLE_NAME
              + " ADD sub_id INTEGER DEFAULT 0");
            db.execSQL(ChannelTableDefine.CREATE_CHANNEL_TABLE);
        Log.d(TAG, "create database table lang_map:"+ LangMapTableDefine.LANG_MAP_TABLE_NAME);
        db.execSQL(LangMapTableDefine.CREATE_LANG_MAP_TABLE);
        db.execSQL(MncMccTableDefine.CREATE_MNC_MCC_TABLE);
        db.execSQL(LangNameTableDefine.CREATE_LANG_NAME_TABLE);
        db.execSQL(CreateLangViewDefine.CREATE_LANG_VIEW);
        db.execSQL(CommonSettingTableDefine.CREATE_COMMOMSETTING_TABLE);
        initLangNameData(db, mLangNames);
    }
    
    public void importLangInfoTablesFromXmlPath(SQLiteDatabase db, String xmlPath){
        Log.d(TAG, "importLangInfoTablesFromXmlPath start  xmlPath = " + xmlPath + "\n");
        File xmlFile = new File(xmlPath);
        if (!xmlFile.exists() || !xmlFile.isFile()) {
            Log.d(TAG, "file is not exist or is not file, It's name is [" + xmlPath + "]\n");
            return;
        }
        try {
            db.beginTransaction();
            FileInputStream inputStream = new FileInputStream(xmlFile);
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(inputStream, "UTF-8");
            //int eventType = parser.getEventType();
            while (parser.getEventType() != XmlPullParser.END_DOCUMENT) {
                if (parser.getEventType() == XmlPullParser.START_TAG) {
                    String tagName = parser.getName();
                    Log.d(TAG, "tagName = " + tagName);
                    if (tagName.endsWith("mncmcc")) {
                        Log.d(TAG, "tagName.endsWith(mncmcc)");
                        int mnc = Integer.parseInt(parser.getAttributeValue(null,
                                "mnc"));
                        int mcc = Integer.parseInt(parser.getAttributeValue(null,
                                "mcc"));
                        int mnc_mcc_id = Integer.parseInt(parser.getAttributeValue(null,
                                "mncmcc_id"));
                        Log.d(TAG, "out: mnc = " + mnc
                                + " \n  mcc = " + mcc + "");
                        ContentValues value = new ContentValues();

                        value.put(MncMccTableDefine.MNC, mnc);
                        value.put(MncMccTableDefine.MCC, mcc);
                        value.put(MncMccTableDefine.MNCMCC_ID, mnc_mcc_id);
                        db.insert(MncMccTableDefine.MNC_MCC_TABLE_NAME, "", value);

                    } else if (tagName.endsWith("langname")) {
                        Log.d(TAG, "tagName.endsWith(langname)");
                        String description = parser.getAttributeValue(null, "description");
                        int lang_name_id = Integer.parseInt(parser.getAttributeValue(null,
                                "_id"));

                        Log.d(TAG, "out : description = " + description + "");
                        ContentValues value = new ContentValues();

                        value.put(LangNameTableDefine.DESCRIPTION, description);
                        value.put(LangNameTableDefine.LANG_NAME_ID, lang_name_id);
                        db.insert(LangNameTableDefine.LANG_NAME_TABLE_NAME, "",
                                value);

                    } else if (tagName.endsWith("langmap")) {
                        Log.d(TAG, "tagName.endsWith(langmap)");

                        int lang_id = Integer.parseInt(parser.getAttributeValue(
                                null, "lang_id"));
                        int mnc_mcc_id = Integer.parseInt(parser
                                .getAttributeValue(null, "mnc_mcc_id"));
                        int show = Integer.parseInt(parser.getAttributeValue(
                                null, "show"));
                        int enable = Integer.parseInt(parser.getAttributeValue(
                                null, "enable"));
                        Log.d(TAG, "out : lang_id = " + lang_id
                                + " \n  mnc_mcc_id = " + mnc_mcc_id + "");
                        ContentValues value = new ContentValues();

                        value.put(LangMapTableDefine.LANG_ID, lang_id);
                        value.put(LangMapTableDefine.MNC_MCC_ID, mnc_mcc_id);
                        value.put(LangMapTableDefine.SHOW, show);
                        value.put(LangMapTableDefine.ENABLE, enable);
                        value.put(LangMapTableDefine.SUBID, -1);
                        db.insert(LangMapTableDefine.LANG_MAP_TABLE_NAME, "",
                                value);

                    }
                }
                parser.next();
            }
            inputStream.close();
            db.setTransactionSuccessful();
        } catch (Exception e) {
            e.printStackTrace();
        } 
        finally{
            db.endTransaction();
        }
    }

}
