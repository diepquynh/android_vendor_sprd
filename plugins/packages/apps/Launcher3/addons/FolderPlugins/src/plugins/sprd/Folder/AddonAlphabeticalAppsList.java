package plugins.sprd.Folder;

import com.android.launcher3.Folderplugins.PluginAlphabeticalApplist;

import android.app.AddonManager;
import android.content.Context;
import java.util.List;
import com.android.launcher3.ItemInfo;
import android.view.View;
import com.android.launcher3.AppInfo;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Locale;
import java.util.HashMap;
import java.util.Map;
import java.util.TreeMap;
import com.android.launcher3.FolderInfo;

import com.android.launcher3.Folderplugins.AppDbInfo;
import com.android.launcher3.Folderplugins.AppFolderDbInfo;
import com.android.launcher3.VolteAppsProvider;
import com.android.launcher3.util.ComponentKey;
import com.android.launcher3.Launcher;
import com.android.launcher3.LauncherAppState;
import com.android.launcher3.model.AppNameComparator;
import com.android.launcher3.allapps.AlphabeticalAppsList;
import com.android.launcher3.allapps.AlphabeticalAppsList.SectionInfo;
import com.android.launcher3.allapps.AlphabeticalAppsList.FastScrollSectionInfo;
import com.android.launcher3.allapps.AlphabeticalAppsList.AdapterItem;
import com.android.launcher3.allapps.AllAppsGridAdapter;
import android.support.v7.widget.RecyclerView;

import android.util.Log;

public class AddonAlphabeticalAppsList extends PluginAlphabeticalApplist implements AddonManager.InitialCallback {

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public void setApps2Folder(AlphabeticalAppsList al, List<ItemInfo> items,
            HashMap<ComponentKey, ItemInfo> componentToAppMap) {
        componentToAppMap.clear();
        al.addApps(items, 0);
    }

    public void setApps2Folder(AlphabeticalAppsList al, List<AppInfo> apps, List<ItemInfo> items,
            HashMap<ComponentKey, ItemInfo> mComponentToAppMap) {
        mComponentToAppMap.clear();
        al.addApps(apps, items);
    }

    public void addApps(AlphabeticalAppsList al, List<AppInfo> apps, List<ItemInfo> items) {
        al.updateApps(apps, items);
    }

    public void addApps(AlphabeticalAppsList al, List<ItemInfo> items, int dummy) {
        al.updateApps(items, 0);
    }

    public void updateApps(AlphabeticalAppsList al, List<AppInfo> apps, List<ItemInfo> items,
            HashMap<ComponentKey, ItemInfo> mComponentToAppMap) {
        for (ItemInfo app : apps) {
            mComponentToAppMap.put(app.toComponentKey(), app);
        }
        for (ItemInfo item : items) {
            mComponentToAppMap.put(item.toComponentKey(), item);
        }
        al.onAppsUpdated();
    }

    public void updateApps(AlphabeticalAppsList al, List<ItemInfo> items, int dummy,
            HashMap<ComponentKey, ItemInfo> mComponentToAppMap) {
        for (ItemInfo item : items) {
            mComponentToAppMap.put(item.toComponentKey(), item);
        }
        al.onAppsUpdated();
    }

    /**
     * Updates internals when the set of apps are updated.
     */
    public void onAppsUpdated(List<ItemInfo> mApps, HashMap<ComponentKey, ItemInfo> mComponentToAppMap,
                 AppNameComparator mAppNameComparator, Launcher mLauncher, AlphabeticalAppsList al) {
        // Sort the list of apps
        mApps.clear();
        mApps.addAll(mComponentToAppMap.values());
        Collections.sort(mApps, mAppNameComparator.getAppInfoComparator());

        // As a special case for some languages (currently only Simplified Chinese), we may need to
        // coalesce sections
        Locale curLocale = mLauncher.getResources().getConfiguration().locale;
        TreeMap<String, ArrayList<ItemInfo>> sectionMap = null;
        boolean localeRequiresSectionSorting = curLocale.equals(Locale.SIMPLIFIED_CHINESE);
        if (localeRequiresSectionSorting) {
            // Compute the section headers.  We use a TreeMap with the section name comparator to
            // ensure that the sections are ordered when we iterate over it later
            sectionMap = new TreeMap<>(mAppNameComparator.getSectionNameComparator());
            for (ItemInfo info : mApps) {
                // Add the section to the cache
                String sectionName = "";
                if (info.itemType == 0/*VolteAppsProvider.Apps.ITEM_TYPE_APPLICATION*/) {
                    AppInfo appInfo = null;
                    if (info instanceof AppInfo) {
                        appInfo = (AppInfo) info;
                    } else {
                        appInfo = ((AppDbInfo) info).appInfo;
                    }
                    sectionName = al.getAndUpdateCachedSectionName(appInfo.title);
                }else{
                    FolderInfo folderInfo = null;
                    if (info instanceof FolderInfo) {
                        folderInfo = (FolderInfo) info;
                    } else {
                        folderInfo = ((AppFolderDbInfo) info).folderInfo;
                    }
                    if(folderInfo != null){
                        sectionName = al.getAndUpdateCachedSectionName(folderInfo.title);
                    }
                }

                // Add it to the mapping
                ArrayList<ItemInfo> sectionApps = sectionMap.get(sectionName);
                if (sectionApps == null) {
                    sectionApps = new ArrayList<>();
                    sectionMap.put(sectionName, sectionApps);
                }
                sectionApps.add(info);
            }

            // Add each of the section apps to the list in order
            List<ItemInfo> allApps = new ArrayList<>(mApps.size());
            for (Map.Entry<String, ArrayList<ItemInfo>> entry : sectionMap.entrySet()) {
                allApps.addAll(entry.getValue());
            }

            mApps.clear();
            mApps.addAll(allApps);

        } else {
            // Just compute the section headers for use below
            for (ItemInfo info : mApps) {
                // Add the section to the cache
                //getAndUpdateCachedSectionName(info.title);
                if (info.itemType == 0/*VolteAppsProvider.Apps.ITEM_TYPE_APPLICATION*/) {
                    AppInfo appInfo = null;
                    if (info instanceof AppInfo) {
                        appInfo = (AppInfo) info;
                    } else {
                        appInfo = ((AppDbInfo) info).appInfo;
                    }
                    al.getAndUpdateCachedSectionName(info.title);
                }else {
                    FolderInfo folderInfo = null;
                    if (info instanceof FolderInfo) {
                        folderInfo = (FolderInfo) info;
                    } else {
                        folderInfo = ((AppFolderDbInfo) info).folderInfo;
                    }
                    al.getAndUpdateCachedSectionName(folderInfo.title);
                }
            }
        }

        // Recompose the set of adapter items from the current set of apps
        al.updateAdapterItems();
    }

    /**
     * Updates the set of filtered apps with the current filter. At this point,
     * we expect mCachedSectionNames to have been calculated for the set of all
     * apps in mApps.
     */
    public void updateAdapterItems(AlphabeticalAppsList al, List<ItemInfo> mFilteredApps,
            List<FastScrollSectionInfo> mFastScrollerSections, List<AdapterItem> mAdapterItems,
            List<SectionInfo> mSections, List<AppInfo> mPredictedApps, List<ComponentKey> mPredictedAppComponents,
            HashMap<ComponentKey, ItemInfo> mComponentToAppMap, Launcher mLauncher, int mNumPredictedAppsPerRow,
            int mNumAppsPerRow, int mNumAppRowsInAdapter, RecyclerView.Adapter mAdapter,
            ArrayList<ComponentKey> mSearchResults, List<ItemInfo> mApps) {
        SectionInfo lastSectionInfo = null;
        String lastSectionName = null;
        FastScrollSectionInfo lastFastScrollerSectionInfo = null;
        int position = 0;
        int appIndex = 0;

        // Prepare to update the list of sections, filtered apps, etc.
        mFilteredApps.clear();
        mFastScrollerSections.clear();
        mAdapterItems.clear();
        mSections.clear();

        // Process the predicted app components
        mPredictedApps.clear();
        if (mPredictedAppComponents != null && !mPredictedAppComponents.isEmpty() && !al.hasFilter()) {
            for (ComponentKey ck : mPredictedAppComponents) {
                ItemInfo tmp = mComponentToAppMap.get(ck);
                if (!(tmp instanceof AppInfo)) {
                    continue;
                }
                AppInfo info = (AppInfo) tmp;
                if (info != null) {
                    mPredictedApps.add(info);
                } else {
                    if (LauncherAppState.isDogfoodBuild()) {
                        Log.e("AddonAlphabeticalAppsList", "Predicted app not found: " + ck.flattenToString(mLauncher));
                    }
                }
                // Stop at the number of predicted apps
                if (mPredictedApps.size() == mNumPredictedAppsPerRow) {
                    break;
                }
            }

            if (!mPredictedApps.isEmpty()) {
                // Add a section for the predictions
                lastSectionInfo = new SectionInfo();
                lastFastScrollerSectionInfo = new FastScrollSectionInfo("");
                AdapterItem sectionItem = AdapterItem.asSectionBreak(position++, lastSectionInfo);
                mSections.add(lastSectionInfo);
                mFastScrollerSections.add(lastFastScrollerSectionInfo);
                mAdapterItems.add(sectionItem);

                // Add the predicted app items
                for (ItemInfo info : mPredictedApps) {
                    AdapterItem appItem = AdapterItem.asPredictedApp(position++, lastSectionInfo, "",
                            lastSectionInfo.numApps++, info, appIndex++);
                    if (lastSectionInfo.firstAppItem == null) {
                        lastSectionInfo.firstAppItem = appItem;
                        lastFastScrollerSectionInfo.fastScrollToItem = appItem;
                    }
                    mAdapterItems.add(appItem);
                    mFilteredApps.add(info);
                }
            }
        }

        // Recreate the filtered and sectioned apps (for convenience for the
        // grid layout) from the
        // ordered set of sections
        for (ItemInfo info : al.getFiltersAppInfos()) {

            String sectionName = "";
            if (info.itemType == VolteAppsProvider.Apps.ITEM_TYPE_APPLICATION) {
                AppInfo appInfo = null;
                if (info instanceof AppInfo) {
                    appInfo = (AppInfo) info;
                } else {
                    appInfo = ((AppDbInfo) info).appInfo;
                }
                sectionName = al.getAndUpdateCachedSectionName(info.title);
            } else {
                FolderInfo folderInfo = null;
                if (info instanceof FolderInfo) {
                    folderInfo = (FolderInfo) info;
                } else {
                    folderInfo = ((AppFolderDbInfo) info).folderInfo;
                }
                sectionName = al.getAndUpdateCachedSectionName(folderInfo.title);
            }

            // Create a new section if the section names do not match
            if (lastSectionInfo == null || !sectionName.equals(lastSectionName)) {
                lastSectionName = sectionName;
                lastSectionInfo = new SectionInfo();
                lastFastScrollerSectionInfo = new FastScrollSectionInfo(sectionName);
                mSections.add(lastSectionInfo);
                mFastScrollerSections.add(lastFastScrollerSectionInfo);

                // Create a new section item to break the flow of items in the
                // list
                if (!al.hasFilter()) {
                    AdapterItem sectionItem = AdapterItem.asSectionBreak(position++, lastSectionInfo);
                    mAdapterItems.add(sectionItem);
                }
            }

            // Create an app item
            AdapterItem appItem = AdapterItem.asApp(position++, lastSectionInfo, sectionName,
                    lastSectionInfo.numApps++, info, appIndex++);
            if (lastSectionInfo.firstAppItem == null) {
                lastSectionInfo.firstAppItem = appItem;
                lastFastScrollerSectionInfo.fastScrollToItem = appItem;
            }
            mAdapterItems.add(appItem);
            mFilteredApps.add(info);
        }

        // Merge multiple sections together as requested by the merge strategy
        // for this device
        al.mergeSections();

        if (mNumAppsPerRow != 0) {
            // Update the number of rows in the adapter after we do all the
            // merging (otherwise, we
            // would have to shift the values again)
            int numAppsInSection = 0;
            int numAppsInRow = 0;
            int rowIndex = -1;
            for (AdapterItem item : mAdapterItems) {
                item.rowIndex = 0;
                if (item.viewType == AllAppsGridAdapter.SECTION_BREAK_VIEW_TYPE) {
                    numAppsInSection = 0;
                } else if (item.viewType == AllAppsGridAdapter.ICON_VIEW_TYPE
                        || item.viewType == AllAppsGridAdapter.PREDICTION_ICON_VIEW_TYPE) {
                    if (numAppsInSection % mNumAppsPerRow == 0) {
                        numAppsInRow = 0;
                        rowIndex++;
                    }
                    item.rowIndex = rowIndex;
                    item.rowAppIndex = numAppsInRow;
                    numAppsInSection++;
                    numAppsInRow++;
                }
            }
            mNumAppRowsInAdapter = rowIndex + 1;

            // Pre-calculate all the fast scroller fractions based on the number
            // of rows
            float rowFraction = 1f / mNumAppRowsInAdapter;
            for (FastScrollSectionInfo info : mFastScrollerSections) {
                AdapterItem item = info.fastScrollToItem;
                if (item.viewType != AllAppsGridAdapter.ICON_VIEW_TYPE
                        && item.viewType != AllAppsGridAdapter.PREDICTION_ICON_VIEW_TYPE) {
                    info.touchFraction = 0f;
                    continue;
                }

                float subRowFraction = item.rowAppIndex * (rowFraction / mNumAppsPerRow);
                info.touchFraction = item.rowIndex * rowFraction + subRowFraction;
            }
        }

        // Refresh the recycler view
        if (mAdapter != null) {
            mAdapter.notifyDataSetChanged();
        }
    }

    public List<ItemInfo> getFiltersAppInfos(ArrayList<ComponentKey> mSearchResults,
        List<ItemInfo> mApps, HashMap<ComponentKey, ItemInfo> mComponentToAppMap) {
        if (mSearchResults == null) {
            ArrayList<ItemInfo> ret = new ArrayList<>();
            for (ItemInfo item : mApps) {
                ret.add(item);
            }
            return ret;
        }

        ArrayList<ItemInfo> result = new ArrayList<>();
        for (ComponentKey key : mSearchResults) {
            ItemInfo tmp = mComponentToAppMap.get(key);
            if (!(tmp instanceof  AppInfo)) {
                continue;
            }
            AppInfo match = (AppInfo) tmp;
            if (match != null) {
                result.add(match);
            }
        }
        return result;
    }

    public int setItemViewType(ItemInfo appInfo) {
        if (appInfo.toComponentKey() != null) {
            return AllAppsGridAdapter.ICON_VIEW_TYPE;
        } else {
            return AllAppsGridAdapter.FOLDER_VIEW_TYPE;
        }
    }
}
