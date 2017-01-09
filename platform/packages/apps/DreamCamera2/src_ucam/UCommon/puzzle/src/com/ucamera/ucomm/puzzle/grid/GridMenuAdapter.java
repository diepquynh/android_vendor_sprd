/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.grid;

import java.lang.reflect.Method;
import java.util.ArrayList;

import com.ucamera.ucomm.puzzle.Puzzle;
import com.ucamera.ucomm.puzzle.PuzzleSpec;
import com.ucamera.ucomm.puzzle.R;
import com.ucamera.ucomm.puzzle.Puzzle.PuzzleMethod;
import com.ucamera.ucomm.puzzle.Puzzle.Type;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.RelativeLayout;
import android.widget.RelativeLayout.LayoutParams;

public class GridMenuAdapter extends BaseAdapter{
    private Context mContext;
    //private ArrayList<Puzzle> mPuzzles = new ArrayList<Puzzle>();
    private ArrayList<PuzzleSpec> mPuzzleSpecs = new ArrayList<PuzzleSpec>();
    // CID 109230 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private int mPuzzleCount;
    private int mSelected;
    public GridMenuAdapter(Context context, ArrayList<PuzzleSpec> puzzles, int puzzleCount) {
        this.mContext = context;
        //this.mPuzzles = puzzles;
        // CID 109230 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // this.mPuzzleCount = puzzleCount;
    }
    /*public void updatePuzzles(ArrayList<Puzzle> puzzles) {
        this.mPuzzles = puzzles;
        notifyDataSetChanged();
    }*/
    public void updatePuzzleSpecd(ArrayList<PuzzleSpec> mspecs) {
        this.mPuzzleSpecs = mspecs;
        notifyDataSetChanged();
    }
    @Override
    public int getCount() {
        return mPuzzleSpecs.size();
    }

    @Override
    public Object getItem(int position) {
        return position;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        Holder holder;
        if(convertView == null) {
            holder = new Holder();
            convertView = LayoutInflater.from(mContext).inflate(R.layout.grid_puzzle_select_item, null);
            holder.selectView = (GridPuzzleSelectView)convertView.findViewById(R.id.grid_puzzle_item);
            holder.relative = (RelativeLayout)convertView.findViewById(R.id.puzzle_select_relat);
            convertView.setTag(holder);
        }
        holder = (Holder)convertView.getTag();
        //holder.selectView.setPuzzle(mPuzzles.get(position));
        holder.selectView.setPuzzleSpec(mPuzzleSpecs.get(position));

        if(mSelected == position) {
            holder.relative.setBackgroundResource(R.drawable.puzzle_menu_item_bk_selected);
        }else {
            holder.relative.setBackgroundResource(R.drawable.puzzle_menu_item_bk);
        }

        return convertView;
    }
    public void setHightLight(int index) {
        mSelected = index;
        notifyDataSetChanged();
    }
    class Holder {
        GridPuzzleSelectView selectView;
        RelativeLayout relative;
    }
}
