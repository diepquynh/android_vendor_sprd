package com.sprd.appbackup.service;

interface IScanAgentAndArchiveListener {
    void onScanComplete();
    void onServiceConnectionChanged(String agentName, boolean connected);
}