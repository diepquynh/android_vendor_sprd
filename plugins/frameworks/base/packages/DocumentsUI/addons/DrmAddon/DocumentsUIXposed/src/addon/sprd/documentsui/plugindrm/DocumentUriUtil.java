package addon.sprd.documentsui.plugindrm;

import android.content.ContentUris;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.util.Log;
//SPRD: add for bug255680, can't show the file chooser dialog when upload file.
//      use to get the real path of the file from document uri.
public class DocumentUriUtil {
	/**
	 * Get a file path from a Uri. This will get the the path for Storage Access
	 * Framework Documents, as well as the _data field for the MediaStore and
	 * other file-based ContentProviders.
	 *
	 * @param context The context.
	 * @param uri The Uri to query.
	 */
        private static String TAG = "DocumentUriUtil";

	public static String getPath(final Context context, final Uri uri) {
            // DocumentProvider
            Log.d(TAG,"entent getPath uri = " + uri);
            // ExternalStorageProvider
            if (isExternalStorageDocument(uri)) {
                final String docId = DocumentsContract.getDocumentId(uri);
                final String[] split = docId.split(":");
                final String type = split[0];
                Log.d(TAG,"entent getPath isExternalStorageDocument type = " + type + "  docId = " + docId);

                String path;
                if ("primary".equalsIgnoreCase(type)) {
                    path = Environment.getInternalStoragePath() + "/";
                } else {
                    path = Environment.getExternalStoragePath() + "/";
                }
                // TODO handle non-primary volumes
                if (split.length > 1) {
                    path = path + split[1];
                }
                return path;
            }
            // DownloadsProvider
            else if (isDownloadsDocument(uri)) {

                final String id = DocumentsContract.getDocumentId(uri);
                final Uri contentUri = ContentUris.withAppendedId(
                        Uri.parse("content://downloads/public_downloads"), Long.valueOf(id));

                    Log.d(TAG,"entent getPath isDownloadsDocument contentUri = " + contentUri);

                return getDataColumn(context, contentUri, null, null);
            }
            // MediaProvider
            else if (isMediaDocument(uri)) {
                final String docId = DocumentsContract.getDocumentId(uri);
                final String[] split = docId.split(":");
                final String type = split[0];

                    Log.d(TAG,"entent getPath isMediaDocument type = " + type);

                Uri contentUri = null;
                if ("image".equals(type)) {
                    contentUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
                } else if ("video".equals(type)) {
                    contentUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
                } else if ("audio".equals(type)) {
                    contentUri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
                } else {
                    /* SPRD:519935 return null if not a content file*/
                    return null;
                }

                final String selection = "_id=?";
                final String[] selectionArgs = new String[] {
                        split[1]
                };
                return getDataColumn(context, contentUri, selection, selectionArgs);
            }

	    // MediaStore (and general)
	    else if ("content".equalsIgnoreCase(uri.getScheme())) {
                Log.d(TAG,"entent getPath content.equalsIgnoreCase(uri.getScheme()) ");
	        // Return the remote address
	        if (isGooglePhotosUri(uri))
	            return uri.getLastPathSegment();

	        return getDataColumn(context, uri, null, null);
	    }
	    // File
	    else if ("file".equalsIgnoreCase(uri.getScheme())) {
                Log.d(TAG,"entent getPath file.equalsIgnoreCase(uri.getScheme()) ");
	        return uri.getPath();
	    }
            Log.d(TAG,"entent getPath return null ");
	    return null;
	}

	/**
	 * Get the value of the data column for this Uri. This is useful for
	 * MediaStore Uris, and other file-based ContentProviders.
	 *
	 * @param context The context.
	 * @param uri The Uri to query.
	 * @param selection (Optional) Filter used in the query.
	 * @param selectionArgs (Optional) Selection arguments used in the query.
	 * @return The value of the _data column, which is typically a file path.
	 */
	public static String getDataColumn(Context context, Uri uri, String selection,
	        String[] selectionArgs) {

	    Cursor cursor = null;
	    final String column = "_data";
	    final String[] projection = {
	            column
	    };

            if (uri != null) {
                try {
                    cursor = context.getContentResolver().query(uri, projection, selection, selectionArgs,
                            null);
                    if (cursor != null && cursor.moveToFirst()) {
                        final int index = cursor.getColumnIndexOrThrow(column);
                        return cursor.getString(index);
                    }
                } finally {
                    if (cursor != null)
                    cursor.close();
                }
            }
            return null;
	}


    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is ExternalStorageProvider.
     */
    public static boolean isExternalStorageDocument(Uri uri) {
        return "com.android.externalstorage.documents".equals(uri.getAuthority());
    }

    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is DownloadsProvider.
     */
    public static boolean isDownloadsDocument(Uri uri) {
        return "com.android.providers.downloads.documents".equals(uri.getAuthority());
    }

    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is MediaProvider.
     */
    public static boolean isMediaDocument(Uri uri) {
        return "com.android.providers.media.documents".equals(uri.getAuthority());
    }

    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is Google Photos.
     */
    public static boolean isGooglePhotosUri(Uri uri) {
        return uri.getAuthority().startsWith("com.google.android.apps");
    }
}
