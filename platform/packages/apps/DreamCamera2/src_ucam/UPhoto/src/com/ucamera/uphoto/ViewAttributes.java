/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

public class ViewAttributes {
    private StringBuffer textColor;
    private StringBuffer textSize;
    private StringBuffer gradientEnd;
    private StringBuffer outline;
    private StringBuffer strokeWidth;
    private StringBuffer font;
    private StringBuffer panelFill;
    private StringBuffer panelShadow;
    private StringBuffer label;
    private StringBuffer labelHead;
    private StringBuffer labelTail;
    private int balloonStyle;

    private String willDrawText;
    private long titleId;

    public ViewAttributes() {
        textColor = new StringBuffer();
        textSize = new StringBuffer();
        gradientEnd = new StringBuffer();
        outline = new StringBuffer();
        strokeWidth = new StringBuffer();
        font = new StringBuffer();
        panelFill = new StringBuffer();
        panelShadow = new StringBuffer();
        label = new StringBuffer();
        labelHead = new StringBuffer();
        labelTail = new StringBuffer();
    }

    public String getTextColor() {
        return textColor.toString();
    }

    public void appendTextColor(String textColor) {
        this.textColor.append(textColor);
    }

    public void setTextColor(String textColor) {
        this.textColor.setLength(0);
        this.textColor.append(textColor);
    }

    public String getTextSize() {
        return textSize.toString();
    }

    public void appendTextSize(String textSize) {
        this.textSize.append(textSize);
    }

    public void setTextSize(String textSize) {
        this.textSize.setLength(0);
        this.textSize.append(textSize);
    }

    /*
     * BUG FIX: 1062
     * FIX COMMENT: if strokwidth is emtpy return default 2
     * DATE: 2012-06-01
     */
    public int getStrokeWidth() {
        try {
            return Integer.parseInt(strokeWidth.toString());
        }catch ( NumberFormatException e) {
            android.util.Log.w("ViewAttribute",
                    String.format("Fail parse strokWidth \"%s\" to Int, use default 2", strokeWidth));
        }
        return 2;
    }

    public void appendStrokeWidth(String strokeWidth) {
        this.strokeWidth.append(strokeWidth);
    }

    public void setStrokeWidth(String strokeWidth) {
        this.strokeWidth.setLength(0);
        this.strokeWidth.append(strokeWidth);
    }

    public String getOutline() {
        return outline.toString();
    }

    public void appendOutline(String outline) {
        this.outline.append(outline);
    }

    public void setOutline(String outline) {
        this.outline.setLength(0);
        this.outline.append(outline);
    }

    public String getFont() {
        return font.toString();
    }

    public void appendFont(String font) {
        this.font.append(font);
    }

    public void setFont(String font) {
        this.font.setLength(0);
        this.font.append(font);
    }

    public String getPanelFill() {
        return panelFill.toString();
    }

    public void appendPanelFill(String panelFill) {
        this.panelFill.append(panelFill);
    }

    public void setPanelFill(String panelFill) {
        this.panelFill.setLength(0);
        this.panelFill.append(panelFill);
    }

    public String getPanelShadow() {
        return panelShadow.toString();
    }

    public void appendPanelShadow(String panelShadow) {
        this.panelShadow.append(panelShadow);
    }

    public void setPanelShadow(String panelShadow) {
        this.panelShadow.setLength(0);
        this.panelShadow.append(panelShadow);
    }

    public String getGradientEnd() {
        return gradientEnd.toString();
    }

    public void appendGradientEnd(String gradientEnd) {
        this.gradientEnd.append(gradientEnd);
    }

    public void setGradientEnd(String gradientEnd) {
        this.gradientEnd.setLength(0);
        this.gradientEnd.append(gradientEnd);
    }

    public String getLabel() {
        return this.label.toString();
    }

    public void appendLabel(String label) {
        this.label.append(label);
    }

    public void setLabel(String label) {
        this.label.setLength(0);
        this.label.append(label);
    }

    public String getLabelHead() {
        return labelHead.toString();
    }

    public void appendLabelHead(String labelHead) {
        this.labelHead.append(labelHead);
    }

    public void setLabelHead(String labelHead) {
        this.labelHead.setLength(0);
        this.labelHead.append(labelHead);
    }

    public String getLabelTail() {
        return this.labelTail.toString();
    }

    public void appendLabelTail(String labelTail) {
        this.labelTail.append(labelTail);
    }

    public void setLabelTail(String labelTail) {
        this.labelTail.setLength(0);
        this.labelTail.append(labelTail);
    }

    public void setDrawText(String willDrawText) {
        this.willDrawText = willDrawText;
    }

    public String getDrawText() {
        return willDrawText;
    }

    public void setTitleId(long titleId) {
        this.titleId = titleId;
    }

    public long getTitleId() {
        return titleId;
    }

    public void setBalloonStyle(int style) {
        this.balloonStyle = style;
    }

    public void setBalloonStyle(String style) {
        if(style != null) {
            this.balloonStyle = Integer.parseInt(style);
        }else{
            this.balloonStyle = 0;
        }
    }

    public int getBalloonStyle() {
        return this.balloonStyle;
    }
}
