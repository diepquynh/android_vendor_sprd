package com.sprd.fileexplorer.util;

import java.io.File;
import java.util.Locale;

public class FitSizeExpression {
    
    public static final int UNIT_SIZE = 1024;
    
    public static final Entry[] UNIT;
    
    public static class Entry {
        String unit;
        double maxValue;
        
        private Entry() {}
        
        public String getUnit() {
            return unit;
        }
        public double getMaxValue() {
            return maxValue;
        }

    }
    
    static {
        UNIT = new Entry[6];
        for(int i = 0; i < UNIT.length; i++) {
            UNIT[i] = new Entry();
            UNIT[i].maxValue = Math.pow(UNIT_SIZE, i+1);
        }
        UNIT[0].unit = "B";
        UNIT[1].unit = "KB";
        UNIT[2].unit = "MB";
        UNIT[3].unit = "GB";
        UNIT[4].unit = "TB";
        UNIT[5].unit = "PB";
    }
    
    private long size;
    private double value;
    private String unit;
    private int grade;
    
    public FitSizeExpression(long size) {
        super();
        this.size = size;
        init();
    }
    
    public FitSizeExpression(File file) {
        super();
        this.size = file.length();
        init();
    }
    
    private void init() {
        grade = -1;
        if(size < 0) {
            return;
        }
        if(size >= UNIT[UNIT.length-1].maxValue) {
            grade = UNIT.length;
            unit = UNIT[UNIT.length-1].unit;
            value = size / UNIT[UNIT.length-1].maxValue;
        }
        for(int i = 0; i < UNIT.length; i++) {
            if(size < UNIT[i].maxValue) {
                grade = i;
                break;
            }
        }
        if(grade == -1) {
            return;
        }
        unit = UNIT[grade].unit;
        if(grade == 0) {
            value = size;
        } else {
            value = size / UNIT[grade-1].maxValue;
        }
    }
    
    public long getSize() {
        return size;
    }

    public void changeSize(long size) {
        this.size = size;
        init();
    }
    
    public void changeFile(File file) {
        changeSize(file.length());
    }

    public double getValue() {
        return value;
    }

    public String getUnit() {
        return unit;
    }

    public int getGrade() {
        return grade;
    }
    
    public String getExpression() {
        if(grade == -1) {
            return "Unknow";
        } else {
            return String.format(Locale.US, "%.2f%s", value, unit);
        }
    }
    
    @Override
    public String toString() {
        return getExpression();
    }
}
