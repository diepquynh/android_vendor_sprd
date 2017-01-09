/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.quicksearchbox;

import com.google.common.annotations.VisibleForTesting;

import android.database.DataSetObservable;
import android.database.DataSetObserver;
import android.util.Log;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;

/**
 * A SuggestionCursor that is backed by a list of Suggestions.
 */
public class ListSuggestionCursor extends AbstractSuggestionCursorWrapper {

    private static final int DEFAULT_CAPACITY = 16;

    private final DataSetObservable mDataSetObservable = new DataSetObservable();

    private final ArrayList<Entry> mSuggestions;

    private HashSet<String> mExtraColumns;

    private int mPos = 0;

    public ListSuggestionCursor(String userQuery) {
        this(userQuery, DEFAULT_CAPACITY);
    }

    @VisibleForTesting
    public ListSuggestionCursor(String userQuery, Suggestion...suggestions) {
        this(userQuery, suggestions.length);
        for (Suggestion suggestion : suggestions) {
            add(suggestion);
        }
    }

    public ListSuggestionCursor(String userQuery, int capacity) {
        super(userQuery);
        mSuggestions = new ArrayList<Entry>(capacity);
    }

    /**
     * Adds a suggestion from another suggestion cursor.
     *
     * @return {@code true} if the suggestion was added.
     */
    public boolean add(Suggestion suggestion) {
        mSuggestions.add(new Entry(suggestion));
        return true;
    }

    public void close() {
        mSuggestions.clear();
    }

    public int getPosition() {
        return mPos;
    }

    public synchronized void moveTo(int pos) {
        /*modify 20130417 Spreadst of 150064 IndexOutOfBoundsException start*/
        if (pos >= 0 && pos < mSuggestions.size()) {
            mPos = pos;
        }
        /*modify 20130417 Spreadst of 150064 IndexOutOfBoundsException end*/
    }

    public synchronized boolean moveToNext() {
        int size = mSuggestions.size();
        /*modify 20130417 Spreadst of 150064 IndexOutOfBoundsException start*/
//      if (mPos >= size) {
        if (mPos >= size-1) {
            // Already past the end
            return false;
        }
        /*modify 20130417 Spreadst of 150064 IndexOutOfBoundsException end*/
        mPos++;
        return mPos < size;
    }

    public synchronized void removeRow() {
        mSuggestions.remove(mPos);
    }

    public void replaceRow(Suggestion suggestion) {
        mSuggestions.set(mPos, new Entry(suggestion));
    }

    public int getCount() {
        return mSuggestions.size();
    }

    @Override
    protected Suggestion current() {
        // SPRD: add if indexoutofbounds, return null
        if (mPos < mSuggestions.size()) {
            return mSuggestions.get(mPos).get();
        } else {
            return null;
        }
    }

    @Override
    public String toString() {
        return getClass().getSimpleName() + "{[" + getUserQuery() + "] " + mSuggestions + "}";
    }

    /**
     * Register an observer that is called when changes happen to this data set.
     *
     * @param observer gets notified when the data set changes.
     */
    public void registerDataSetObserver(DataSetObserver observer) {
        mDataSetObservable.registerObserver(observer);
    }

    /**
     * Unregister an observer that has previously been registered with
     * {@link #registerDataSetObserver(DataSetObserver)}
     *
     * @param observer the observer to unregister.
     */
    public void unregisterDataSetObserver(DataSetObserver observer) {
        mDataSetObservable.unregisterObserver(observer);
    }

    protected void notifyDataSetChanged() {
        mDataSetObservable.notifyChanged();
    }

    @Override
    public SuggestionExtras getExtras() {
        // override with caching to avoid re-parsing the extras
        return mSuggestions.get(mPos).getExtras();
    }

   public Collection<String> getExtraColumns() {
        if (mExtraColumns == null) {
            mExtraColumns = new HashSet<String>();
            for (Entry e : mSuggestions) {
                SuggestionExtras extras = e.getExtras();
                Collection<String> extraColumns = extras == null ? null
                        : extras.getExtraColumnNames();
                if (extraColumns != null) {
                    for (String column : extras.getExtraColumnNames()) {
                        mExtraColumns.add(column);
                    }
                }
            }
        }
        return mExtraColumns.isEmpty() ? null : mExtraColumns;
    }

    /**
     * This class exists purely to cache the suggestion extras.
     */
    private static class Entry {
        private final Suggestion mSuggestion;
        private SuggestionExtras mExtras;
        public Entry(Suggestion s) {
            mSuggestion = s;
        }
        public Suggestion get() {
            return mSuggestion;
        }
        public SuggestionExtras getExtras() {
            if (mExtras == null) {
                mExtras = mSuggestion.getExtras();
            }
            return mExtras;
        }
    }

}
