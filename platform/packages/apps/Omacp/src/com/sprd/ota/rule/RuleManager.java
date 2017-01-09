
package com.sprd.ota.rule;

public class RuleManager {

    private RuleManager() {
        mRuleMaps = new RuleMaps();
    }

    synchronized public static RuleManager createInstance() {
        if (mIns == null) {
            mIns = new RuleManager();
        }
        return mIns;
    }

    synchronized public static void realeaseIns() {
        if (mIns != null) {
            mIns = null;
        }
    }

    public RuleMaps getRuleMaps() {
        return mRuleMaps;
    }

    public static boolean isEmpty() {
        return RuleManager.createInstance().getRuleMaps().isEmpty();
    }

    public RuleList getRuleList(String ruleStr) {
        return RuleManager.createInstance().getRuleMaps().get(ruleStr);
    }

    private static RuleManager mIns = null;
    private RuleMaps mRuleMaps = null;
}
