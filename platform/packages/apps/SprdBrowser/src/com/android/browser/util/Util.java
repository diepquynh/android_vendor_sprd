package com.android.browser.util;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.util.Log;
import android.util.Patterns;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.os.SystemProperties;
import android.content.Context;
import android.content.Intent;
import android.provider.Browser;
import android.content.ContentResolver;
import android.database.Cursor;
import android.provider.BrowserContract.History;
import android.provider.BrowserContract.Searches;
import android.content.ContentUris;
import android.content.ContentValues;
import android.database.sqlite.SQLiteException;
import android.text.InputFilter;
import android.text.Spanned;
import android.widget.Toast;
import com.android.browser.R;
import com.sprd.common.Common;

public class Util {
    private static final String LOGTAG = "Util";
    private static final int MAX_HISTORY_COUNT = 250;

    /**
     *Add for bottom bar
     *@{
     */
    public static boolean SUPPORT_BOTTOM_BAR = SystemProperties.getBoolean("ro.browser.bottombar",true);
    /*@}*/

    /**
     * Add for navigation tab
     *@{
     */
    public static boolean SUPPORT_SAVE_SNAPSHOT = SystemProperties.getBoolean("ro.browser.snapshot",true);
    /*@}*/

    /**
     *Add for setting homepage via MCC\MNC
     *@{
     */
    public static boolean SUPPORT_CARRIER_HOMEPAGE = SystemProperties.getBoolean("ro.browser.carrier.homepage",true);
    /*@}*/

    /**
     * Add for navigation tab
     *@{
     */
    public static boolean BROWSER_SITE_NAVIGATION_SUPPORT = SystemProperties.getBoolean("ro.browser.sitnavigation",true);
    /*@}*/


    /*
     * for download_storage_save_path
     *@{
     */
    public static boolean SUPPORT_SELECT_DOWNLOAD_PATH;
    /*@}*/

    /*
     * for download_overlay_same_file
     *@{
     */
    public static boolean SUPPORT_OVERLAY_DOWNLOAD_FILE = SystemProperties.getBoolean("ro.browser.overlaydownload",true);
    /*@}*/

    /**
     * Add for horizontal screen
     *@{
     */
    public static boolean SUPPORT_HORIZONTAL_SCREEN = SystemProperties.getBoolean("ro.browser.horizontal",true);
    /*@}*/

    /**
     * Add for text format
     *@{
     */
    public static boolean SUPPORT_TEXT_FORMAT = SystemProperties.getBoolean("ro.browser.textformat",true);
    /*@}*/

    /*SPRD:modify for bug493554, add Pattern.WEB_URL @{ */
    public static final Pattern WEB_URL = Pattern.compile(
            "((?:(http|https|Http|Https|rtsp|Rtsp):\\/\\/(?:(?:[a-zA-Z0-9\\$\\-\\_\\.\\+\\!\\*\\'\\(\\)"
            + "\\,\\;\\?\\&\\=]|(?:\\%[a-fA-F0-9]{2})){1,64}(?:\\:(?:[a-zA-Z0-9\\$\\-\\_"
            + "\\.\\+\\!\\*\\'\\(\\)\\,\\;\\?\\&\\=]|(?:\\%[a-fA-F0-9]{2})){1,25})?\\@)?)?"
            + "((?:(?:[" + Patterns.GOOD_IRI_CHAR + "][" + Patterns.GOOD_IRI_CHAR + "\\-]{0,64}\\.)+"   // named host
            + Patterns.TOP_LEVEL_DOMAIN_STR_FOR_WEB_URL
            + "|(?:(?:25[0-5]|2[0-4]" // or ip address
            + "[0-9]|[0-1][0-9]{2}|[1-9][0-9]|[1-9])\\.(?:25[0-5]|2[0-4][0-9]"
            + "|[0-1][0-9]{2}|[1-9][0-9]|[1-9]|0)\\.(?:25[0-5]|2[0-4][0-9]|[0-1]"
            + "[0-9]{2}|[1-9][0-9]|[1-9]|0)\\.(?:25[0-5]|2[0-4][0-9]|[0-1][0-9]{2}"
            + "|[1-9][0-9]|[0-9])))"
            + "(?:\\:\\d{1,5})?)" // plus option port number
            + "(\\/(?:(?:[" + Patterns.GOOD_IRI_CHAR + "\\;\\/\\?\\:\\@\\&\\=\\#\\~"  // plus option query params
            + "\\-\\.\\+\\!\\*\\'\\(\\)\\,\\_])|(?:\\%[a-fA-F0-9]{2}))*)?"
            + "(?:\\b|$)",Pattern.CASE_INSENSITIVE); // and finally, a word boundary or end of
                            // input.  This is to stop foo.sure from
                            // matching as foo.su

    /* Fix Bug:495017 Could not save the ICBC URL to the bookmark 2015.11.06 start */
    public static Bitmap compressImage(Bitmap image) {
        if (image == null)
            return null;
        Bitmap bitmap = Bitmap.createBitmap(image);
        int current = bitmap.getAllocationByteCount();
        int appoint = 200000;
        int appoint_mini = 150000;
        while (current > appoint) {
            Log.d(LOGTAG, "before compressImage current = " + current);
            float scale = (float) Math.sqrt((float) appoint_mini
                    / (float) current);
            Matrix matrix = new Matrix();
            matrix.postScale(scale, scale);
            bitmap = Bitmap.createBitmap(bitmap, 0, 0, bitmap.getWidth(),
                    bitmap.getHeight(), matrix, true);
            current = bitmap.getAllocationByteCount();
            Log.d(LOGTAG, "after compressImage current = " + current);
        }
        return bitmap;
    }
    /* Fix Bug:495017 Could not save the ICBC URL to the bookmark 2015.11.06 end */

    public static boolean isMatches(Matcher matcher){
        try{
            return matcher.matches();
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }
    /*add Pattern.WEB_URL @} */

    /*some interface have been removed from framework, add them to browser*/
    /**
     *  Open an activity to save a bookmark. Launch with a title
     *  and/or a url, both of which can be edited by the user before saving.
     *
     *  @param c        Context used to launch the activity to add a bookmark.
     *  @param title    Title for the bookmark. Can be null or empty string.
     *  @param url      Url for the bookmark. Can be null or empty string.
     */
    public static final void saveBookmark(Context c,
                                          String title,
                                          String url) {
        Intent i = new Intent(Intent.ACTION_INSERT, Browser.BOOKMARKS_URI);
        i.putExtra("title", title);
        i.putExtra("url", url);
        c.startActivity(i);
    }

    /**
     *  Returns all the URLs in the history.
     *  Requires {@link android.Manifest.permission#READ_HISTORY_BOOKMARKS}
     *  @param cr   The ContentResolver used to access the database.
     *  @hide pending API council approval
     */
    public static final String[] getVisitedHistory(ContentResolver cr) {
        Cursor c = null;
        String[] str = null;
        try {
            String[] projection = new String[] {
                    History.URL,
            };
            c = cr.query(History.CONTENT_URI, projection, History.VISITS + " > 0", null, null);
            if (c == null) return new String[0];
            str = new String[c.getCount()];
            int i = 0;
            while (c.moveToNext()) {
                str[i] = c.getString(0);
                i++;
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "getVisitedHistory", e);
            str = new String[0];
        } finally {
            if (c != null) c.close();
        }
        return str;
    }

    /**
     * If there are more than MAX_HISTORY_COUNT non-bookmark history
     * items in the bookmark/history table, delete TRUNCATE_N_OLDEST
     * of them.  This is used to keep our history table to a
     * reasonable size.  Note: it does not prune bookmarks.  If the
     * user wants 1000 bookmarks, the user gets 1000 bookmarks.
     *  Requires {@link android.Manifest.permission#READ_HISTORY_BOOKMARKS}
     *  Requires {@link android.Manifest.permission#WRITE_HISTORY_BOOKMARKS}
     *
     * @param cr The ContentResolver used to access the database.
     */
    public static final void truncateHistory(ContentResolver cr) {
        // TODO make a single request to the provider to do this in a single transaction
        Cursor cursor = null;
        try {

            // Select non-bookmark history, ordered by date
            cursor = cr.query(History.CONTENT_URI,
                    new String[] { History._ID, History.URL, History.DATE_LAST_VISITED },
                    null, null, History.DATE_LAST_VISITED + " ASC");

            if (cursor.moveToFirst() && cursor.getCount() >= MAX_HISTORY_COUNT) {
                /* eliminate oldest history items */
                for (int i = 0; i < Browser.TRUNCATE_N_OLDEST; i++) {
                    cr.delete(ContentUris.withAppendedId(History.CONTENT_URI, cursor.getLong(0)),
                        null, null);
                    if (!cursor.moveToNext()) break;
                }
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "truncateHistory", e);
        } finally {
            if (cursor != null) cursor.close();
        }
    }

    /**
     *  Delete all entries from the bookmarks/history table which are
     *  not bookmarks.  Also set all visited bookmarks to unvisited.
     *  Requires {@link android.Manifest.permission#WRITE_HISTORY_BOOKMARKS}
     *  @param cr   The ContentResolver used to access the database.
     */
    public static final void clearHistory(ContentResolver cr) {
        deleteHistoryWhere(cr, null);
    }
    /**
     * Returns whether there is any history to clear.
     *  Requires {@link android.Manifest.permission#READ_HISTORY_BOOKMARKS}
     * @param cr   The ContentResolver used to access the database.
     * @return boolean  True if the history can be cleared.
     */
    public static final boolean canClearHistory(ContentResolver cr) {
        Cursor cursor = null;
        boolean ret = false;
        try {
            cursor = cr.query(History.CONTENT_URI,
                new String [] { History._ID, History.VISITS },
                null, null, null);
            ret = cursor.getCount() > 0;
        } catch (Exception e) {
            Log.e(LOGTAG, "canClearHistory", e);
        } finally {
            if (cursor != null) cursor.close();
        }
        return ret;
    }

    /**
     * Delete all history items from begin to end.
     *  Requires {@link android.Manifest.permission#WRITE_HISTORY_BOOKMARKS}
     * @param cr    The ContentResolver used to access the database.
     * @param begin First date to remove.  If -1, all dates before end.
     *              Inclusive.
     * @param end   Last date to remove. If -1, all dates after begin.
     *              Non-inclusive.
     */
    public static final void deleteHistoryTimeFrame(ContentResolver cr,
            long begin, long end) {
        String whereClause;
        String date = "date";
        if (-1 == begin) {
            if (-1 == end) {
                clearHistory(cr);
                return;
            }
            whereClause = date + " < " + Long.toString(end);
        } else if (-1 == end) {
            whereClause = date + " >= " + Long.toString(begin);
        } else {
            whereClause = date + " >= " + Long.toString(begin) + " AND " + date
                    + " < " + Long.toString(end);
        }
        deleteHistoryWhere(cr, whereClause);
    }

    /**
     * Helper function to delete all history items and release the icons for them in the
     * {@link WebIconDatabase}.
     *
     * Requires {@link android.Manifest.permission#READ_HISTORY_BOOKMARKS}
     * Requires {@link android.Manifest.permission#WRITE_HISTORY_BOOKMARKS}
     *
     * @param cr   The ContentResolver used to access the database.
     * @param whereClause   String to limit the items affected.
     *                      null means all items.
     */
    private static final void deleteHistoryWhere(ContentResolver cr, String whereClause) {
        Cursor cursor = null;
        try {
            cursor = cr.query(History.CONTENT_URI, new String[] { History.URL }, whereClause,
                    null, null);
            if (cursor.moveToFirst()) {
                cr.delete(History.CONTENT_URI, whereClause, null);
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "deleteHistoryWhere", e);
            return;
        } finally {
            if (cursor != null) cursor.close();
        }
    }

    /**
     * Remove a specific url from the history database.
     *  Requires {@link android.Manifest.permission#WRITE_HISTORY_BOOKMARKS}
     * @param cr    The ContentResolver used to access the database.
     * @param url   url to remove.
     */
    public static final void deleteFromHistory(ContentResolver cr,
                                               String url) {
        try {
            cr.delete(History.CONTENT_URI, History.URL + "=?", new String[] { url });
        } catch (SQLiteException e) {
            Log.e(LOGTAG, "deleteFromHistory:", e);
        }
    }

    /**
     * Add a search string to the searches database.
     *  Requires {@link android.Manifest.permission#READ_HISTORY_BOOKMARKS}
     *  Requires {@link android.Manifest.permission#WRITE_HISTORY_BOOKMARKS}
     * @param cr   The ContentResolver used to access the database.
     * @param search    The string to add to the searches database.
     */
    public static final void addSearchUrl(ContentResolver cr, String search) {
        try {
            // The content provider will take care of updating existing searches instead of duplicating
            ContentValues values = new ContentValues();
            values.put(Searches.SEARCH, search);
            values.put(Searches.DATE, System.currentTimeMillis());
            cr.insert(Searches.CONTENT_URI, values);
        } catch (SQLiteException e) {
            Log.e(LOGTAG, "addSearchUrl:", e);
        }
    }

    /**
     * Remove all searches from the search database.
     *  Requires {@link android.Manifest.permission#WRITE_HISTORY_BOOKMARKS}
     * @param cr   The ContentResolver used to access the database.
     */
    public static final void clearSearches(ContentResolver cr) {
        // FIXME: Should this clear the urls to which these searches lead?
        // (i.e. remove google.com/query= blah blah blah)
        try {
            cr.delete(Searches.CONTENT_URI, null, null);
        } catch (Exception e) {
            Log.e(LOGTAG, "clearSearches", e);
        }
    }
    /*some interface have been removed from framework, add them to browser*/

    public static InputFilter[] getLengthFilter(final Context context, final int size){
        InputFilter[] lengthFilter = new InputFilter[1];
        lengthFilter[0] = new InputFilter.LengthFilter(size) {

            @Override
            public CharSequence filter(CharSequence source, int start, int end,
                    Spanned dest, int dstart, int dend) {
                // TODO Auto-generated method stub
                //bug 175305&175307: Converity CID 30039&30040 begin
                if(source == null || dest == null){
                    return null;
                }
                //bug 175305&175307: Converity CID 30039&30040 end
                if(source != null && source.length() > 0 && dest != null && dest.length() >= size) {
                    Toast.makeText(context, R.string.input_too_long, Toast.LENGTH_SHORT).show();
                }
                return super.filter(source, start, end, dest, dstart, dend);
            }
        };
        return lengthFilter;
    }
}
