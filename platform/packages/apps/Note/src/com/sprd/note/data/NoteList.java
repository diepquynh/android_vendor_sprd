
package com.sprd.note.data;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedList;
import java.util.List;

public class NoteList {

    private List<NoteItem> mNoteLists = new LinkedList<NoteItem>();

    public List<NoteItem> getList() {
        return mNoteLists;
    }

    public List<NoteItem> getNotes() {
        ArrayList<NoteItem> arrayList = new ArrayList<NoteItem>();
        for (NoteItem noteItem : mNoteLists) {
            if (!noteItem.isFileFolder) {
                arrayList.add(noteItem);
            }
        }
        return arrayList;
    }

    public List<NoteItem> getFolderList() {
        ArrayList<NoteItem> arrayList = new ArrayList<NoteItem>();
        for (NoteItem noteItem : mNoteLists) {
            if (noteItem.isFileFolder) {
                arrayList.add(noteItem);
            }
        }
        return arrayList;
    }

    public List<NoteItem> getNotesFromFolder(int folderID) {
        ArrayList<NoteItem> arrayList = new ArrayList<NoteItem>();
        for (NoteItem noteItem : mNoteLists) {
            if (noteItem.parentFolderId == folderID) {
                arrayList.add(noteItem);
            }
        }
        return arrayList;
    }

    public List<NoteItem> getRootFoldersAndNotes() {
        ArrayList<NoteItem> arrayList = new ArrayList<NoteItem>();
        for (int i = 0; i < mNoteLists.size(); i++) {
            NoteItem noteItem = mNoteLists.get(i);
            if (noteItem.isFileFolder || noteItem.parentFolderId == -1) {
                arrayList.add(noteItem);
            }
        }
        return arrayList;
    }

    public List<NoteItem> getRootNotes() {
        ArrayList<NoteItem> arrayList = new ArrayList<NoteItem>();
        for (NoteItem noteItem : mNoteLists) {
            if (!noteItem.isFileFolder && noteItem.parentFolderId == -1) {
                arrayList.add(noteItem);
            }
        }
        return arrayList;
    }

    public NoteItem getNoteItemByID(int ID) {
        for (NoteItem noteItem : mNoteLists) {
            if (noteItem._id == ID) {
                return noteItem;
            }
        }
        return null;
    }

    public List<NoteItem> getNotesIncludeContent(String content) {
        ArrayList<NoteItem> arrayList = new ArrayList<NoteItem>();
        for (NoteItem noteItem : mNoteLists) {
            if (!noteItem.isFileFolder && noteItem.content != null) { // file and not null
                if (noteItem.content.toLowerCase().contains(content.toLowerCase())) { // include content
                    arrayList.add(noteItem);
                } else if (noteItem.title != null
                        && noteItem.title.toLowerCase().contains(content.toLowerCase())) {
                    arrayList.add(noteItem);
                }
            }
        }
        return arrayList;
    }

    public void updateOneItem(NoteItem item) {
        mNoteLists.remove(item);
        int location = getNoteInsertLoction(item);
        mNoteLists.add(location, item);
    }

    public void addOneItem(NoteItem item) {
        int location = getNoteInsertLoction(item);
        mNoteLists.add(location, item);
    }

    public synchronized void deleteNoteItemOrFolder(NoteItem item) {
        List<NoteItem> removeList = new ArrayList<NoteItem>();
        for (NoteItem noteItem : mNoteLists) {
            if (noteItem.parentFolderId == item._id || noteItem._id == item._id) {
                removeList.add(noteItem);
            }
        }
        mNoteLists.removeAll(removeList);
        for (NoteItem removeItem : removeList) {
            removeItem = null;
        }
    }

    public void clear() {
        for (NoteItem noteItem : mNoteLists) {
            noteItem = null;
        }
        mNoteLists.clear();
        mNoteLists = null;
        mNoteLists = new LinkedList<NoteItem>();
    }

    public void add(NoteItem item) {
        mNoteLists.add(item);
    }

    private int getNoteInsertLoction(NoteItem item) {
        if (item.isFileFolder) {
            for (int location = 0; location < mNoteLists.size(); location++) {
                NoteItem sourceItem = mNoteLists.get(location);
                if (sourceItem.isFileFolder) {
                    if (item.date >= sourceItem.date) {
                        return location;
                    }
                } else {
                    return location;
                }
            }
        } else {
            for (int location = 0; location < mNoteLists.size(); location++) {
                NoteItem sourceItem = mNoteLists.get(location);
                if (!sourceItem.isFileFolder) {
                    if (item.date >= sourceItem.date) {
                        return location;
                    }
                }
            }
        }
        return mNoteLists.size();
    }

    public static class NoteComparetor implements Comparator<NoteItem> {

        public static final NoteComparetor comparetor = new NoteComparetor();

        @Override
        public int compare(NoteItem item1, NoteItem item2) {
            if (item1.isFileFolder != item2.isFileFolder) {
                if (item1.isFileFolder) {
                    return -1;
                } else {
                    return 1;
                }
            } else {
                if (item1.date > item2.date) {
                    return -1;
                } else if (item1.date < item2.date) {
                    return 1;
                } else {
                    if (item1._id > item2._id) {
                        return -1;
                    } else if (item1._id < item2._id) {
                        return 1;
                    } else {
                        return 0;
                    }
                }
            }
        }

    }

    @Override
    public String toString() {
        return mNoteLists.toString();
    }
}
