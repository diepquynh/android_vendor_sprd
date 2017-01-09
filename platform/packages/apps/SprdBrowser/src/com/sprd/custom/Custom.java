package com.sprd.custom;

import com.android.browser.R;
import android.app.AddonManager;
import com.sprd.custom.SprdBrowserBookmarkAddonStub;
import com.sprd.custom.SprdBrowserHomePageAddonStub;
import com.sprd.custom.SprdBrowserSearchEngineAddonStub;
import com.sprd.custom.SprdBrowserUserAgentAddonStub;

public class Custom {
    private static volatile SprdBrowserSiteNavigationAddonStub sSiteNavigationPlugin = null;
    private static volatile SprdBrowserBookmarkAddonStub sBookmarkPlugin = null;
    private static volatile SprdBrowserHomePageAddonStub sHomePagePlugin = null;
    private static volatile SprdBrowserSearchEngineAddonStub sSearchEnginePlugin = null;
    private static volatile SprdBrowserUserAgentAddonStub sUserAgentPlugin = null;

    private Custom() {
    }

    public static SprdBrowserSiteNavigationAddonStub getSiteNavigationPlugin() {
        if (sSiteNavigationPlugin != null)
            return sSiteNavigationPlugin;
        sSiteNavigationPlugin = (SprdBrowserSiteNavigationAddonStub) AddonManager.getDefault().getAddon(R.string.feature_browser_navigation_custom, SprdBrowserSiteNavigationAddonStub.class);
        return sSiteNavigationPlugin;
    }

    public static SprdBrowserBookmarkAddonStub getBookmarkPlugin() {
        if (sBookmarkPlugin != null)
            return sBookmarkPlugin;
        sBookmarkPlugin = (SprdBrowserBookmarkAddonStub) AddonManager.getDefault().getAddon(R.string.feature_browser_bookmark, SprdBrowserBookmarkAddonStub.class);
        return sBookmarkPlugin;
    }
    public static SprdBrowserHomePageAddonStub getHomePagePlugin() {
        if (sHomePagePlugin != null)
            return sHomePagePlugin;
        sHomePagePlugin = (SprdBrowserHomePageAddonStub) AddonManager.getDefault().getAddon(R.string.feature_browser_homepage, SprdBrowserHomePageAddonStub.class);
        return sHomePagePlugin;
    }
    public static SprdBrowserSearchEngineAddonStub getSearchEnginePlugin() {
        if (sSearchEnginePlugin != null)
            return sSearchEnginePlugin;
        sSearchEnginePlugin = (SprdBrowserSearchEngineAddonStub) AddonManager.getDefault().getAddon(R.string.feature_browser_searchengine, SprdBrowserSearchEngineAddonStub.class);
        return sSearchEnginePlugin;
    }
    public static SprdBrowserUserAgentAddonStub getUserAgentPlugin() {
        if (sUserAgentPlugin != null)
            return sUserAgentPlugin;
        sUserAgentPlugin = (SprdBrowserUserAgentAddonStub) AddonManager.getDefault().getAddon(R.string.feature_browser_useragent, SprdBrowserUserAgentAddonStub.class);
        return sUserAgentPlugin;
    }

}
