
package com.sprd.firewall.util;

import com.sprd.firewall.db.BlackColumns;
import com.sprd.firewall.model.BlackCallEntity;
import com.sprd.firewall.model.BlackEntity;
import com.sprd.firewall.model.BlackNumberEntity;
import com.sprd.firewall.model.BlackSmsEntity;

import android.database.Cursor;
import android.provider.BlockedNumberContract;

public class BlackEntityUtil {

    public static final String BLOCK_TYPE = "block_type";
    public static final String NAME = "name";

    private static void transform(BlackNumberEntity entity, Cursor cursor) {
        entity.setId(Integer.valueOf(cursor.getInt(
                cursor.getColumnIndex(BlockedNumberContract.BlockedNumbers.COLUMN_ID))));
        entity.setNumber(cursor.getString(cursor
                .getColumnIndex(BlockedNumberContract.BlockedNumbers.COLUMN_ORIGINAL_NUMBER)));
        entity.setType(Integer.valueOf(cursor.getInt(cursor.getColumnIndex(BLOCK_TYPE))));
        entity.setName(cursor.getString(cursor
                .getColumnIndex(NAME)));
    }

    private static void transform(BlackCallEntity entity, Cursor cursor) {
        entity.setId(Integer.valueOf(cursor.getInt(
                cursor.getColumnIndex(BlackColumns.BlockRecorder._ID))));
        entity.setNumber(cursor.getString(cursor.
                getColumnIndex(BlackColumns.BlockRecorder.NUMBER_VALUE)));
        entity.setType(Integer.valueOf(
                cursor.getInt(cursor.getColumnIndex(BlackColumns.BlockRecorder.CALL_TYPE))));
        entity.setTime(cursor.getLong(
                cursor.getColumnIndex(BlackColumns.BlockRecorder.BLOCK_DATE)));
        entity.setName(cursor.getString(cursor.
               getColumnIndex(BlackColumns.BlockRecorder.NAME)));
    }

    private static void transform(BlackSmsEntity entity, Cursor cursor) {
        entity.setId(Integer.valueOf(cursor.getInt(
                cursor.getColumnIndex(BlackColumns.SmsBlockRecorder._ID))));
        entity.setNumber(cursor.getString(cursor
                .getColumnIndex(BlackColumns.SmsBlockRecorder.NUMBER_VALUE)));
        entity.setContent(cursor.getString(cursor
                .getColumnIndex(BlackColumns.SmsBlockRecorder.BLOCK_SMS_CONTENT)));
        entity.setTime(cursor.getLong(cursor
                .getColumnIndex(BlackColumns.SmsBlockRecorder.BLOCK_DATE)));
        entity.setName(cursor.getString(cursor
                .getColumnIndex(BlackColumns.SmsBlockRecorder.NAME)));
    }

    public static boolean transform(BlackEntity entity, Cursor cursor) {
        if (entity == null || cursor == null) {
            return false;
        }
        if (entity instanceof BlackNumberEntity) {
            BlackNumberEntity numberEntity = (BlackNumberEntity) entity;
            transform(numberEntity, cursor);
            return true;
        } else if (entity instanceof BlackCallEntity) {
            BlackCallEntity callEntity = (BlackCallEntity) entity;
            transform(callEntity, cursor);
            return true;
        } else if (entity instanceof BlackSmsEntity) {
            BlackSmsEntity smsEntity = (BlackSmsEntity) entity;
            transform(smsEntity, cursor);
            return true;
        }
        return false;
    }
}
