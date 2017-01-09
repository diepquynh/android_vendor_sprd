/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit.callback;

public interface ProcessCallback {
    public void updateAapter(String[] result);
    public void beforeProcess();
    public void afterProcess(int cur);
}
