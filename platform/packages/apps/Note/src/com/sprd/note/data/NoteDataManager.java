
package com.sprd.note.data;

import java.util.List;

import android.content.Context;
import android.database.Cursor;

public interface NoteDataManager {

    public void setCursor(Cursor cursor);

    public void initData(Context context);

    public List<NoteItem> getFolderAndAllItems();

    public NoteItem getNoteItem(int id);

    public NoteItem getNoteItemFromDB(int id);

    public int insertItem(NoteItem item);

    public int updateItem(NoteItem item);

    public List<NoteItem> getFolders();

    /**
     * get notes, not include note forld.
     */
    public List<NoteItem> getNotes();

    public List<NoteItem> getRootFoldersAndNotes();

    public List<NoteItem> getRootNotes();

    public List<NoteItem> getNotesFromFolder(int folderID);

    public void deleteNoteItem(NoteItem item);

    public void deleteAllNotes();

    public List<NoteItem> getNotesIncludeContent(String content);

    public Cursor query(String[] columns, String selection, String[] selectionArgs, String groupBy,
            String having, String sortOrder, String limit);
}
