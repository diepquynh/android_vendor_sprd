package com.sprd.appbackup;

import java.util.List;
import android.graphics.drawable.Drawable;
import com.sprd.appbackup.service.Agent;
import com.sprd.appbackup.service.Account;

public class DataItem {

    private boolean isEnabled = false;
    private boolean isChecked = false;
    private Drawable icon = null;
    private String categoryName = null;
    private int categoryCode = 0;
    private String agentName = null;
    private Agent agent = null;
    private List<Account> mAccounts;
    private boolean isDefault = true;

    public DataItem(){}

    public DataItem(boolean isEnabled, boolean isChecked, Drawable icon,
            String categoryName, int categoryCode, String agentName,
            Agent agent, List<Account> mAccounts, boolean isDefault) {
        super();
        this.isEnabled = isEnabled;
        this.isChecked = isChecked;
        this.icon = icon;
        this.categoryName = categoryName;
        this.categoryCode = categoryCode;
        this.agentName = agentName;
        this.agent = agent;
        this.mAccounts = mAccounts;
        this.isDefault = isDefault;
    }

    public boolean isEnabled() {
        return isEnabled;
    }
    public void setEnabled(boolean isEnabled) {
        this.isEnabled = isEnabled;
    }
    public boolean isChecked() {
        return isChecked;
    }
    public void setChecked(boolean isChecked) {
        this.isChecked = isChecked;
    }
    public Drawable getIcon() {
        return icon;
    }
    public void setIcon(Drawable icon) {
        this.icon = icon;
    }
    public String getCategoryName() {
        return categoryName;
    }
    public void setCategoryName(String categoryName) {
        this.categoryName = categoryName;
    }
    public int getCategoryCode() {
        return categoryCode;
    }
    public void setCategoryCode(int categoryCode) {
        this.categoryCode = categoryCode;
    }
    public String getAgentName() {
        return agentName;
    }
    public void setAgentName(String agentName) {
        this.agentName = agentName;
    }

    public Agent getAgent() {
        return agent;
    }

    public void setAgent(Agent agent) {
        this.agent = agent;
    }
    public String toString(){
        StringBuffer sb = new StringBuffer();
        sb.append("/cateName = "+categoryName);
        sb.append("/cateCode = "+categoryCode);
        sb.append("/isEnabled = "+isEnabled);
        sb.append("/isChecked = "+isChecked);
        sb.append("/List<Account> = "+mAccounts);
        return sb.toString();
    }

    public List<Account> getAccounts() {
        return mAccounts;
    }

    public void setAccounts(List<Account> mAccounts) {
        this.mAccounts = mAccounts;
    }

    public DataItem copy(){
        return new DataItem(isEnabled, isChecked, icon, categoryName, categoryCode, agentName, agent, mAccounts, isDefault);
    }

    public boolean isDefaultSelect() {
        return isDefault;
    }

    public void setDefaultSelect(boolean isDefault) {
        this.isDefault = isDefault;
    }
}
