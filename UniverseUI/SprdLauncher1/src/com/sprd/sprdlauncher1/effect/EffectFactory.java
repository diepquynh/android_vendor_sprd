/** Created by Spreadst */
package com.sprd.launcher3.effect;

import android.content.Context;
import android.content.SharedPreferences;

import java.util.ArrayList;
import java.util.List;

import com.android.sprdlauncher1.Workspace;
import com.sprd.launcher3.WorkspaceSettings;

public class EffectFactory {

    private static List<EffectInfo> allEffects = new ArrayList<EffectInfo>();
    public static List<EffectInfo> getAllEffects(){
      return loadEffectsList();
    }
    public static EffectInfo getEffect(int id){
        if(id==0){
            return null; // if is is 0 , we return null , and mean we do not need animation for workspace
        }
        if(allEffects.isEmpty()){
            loadEffectsList();
        }
        for (int i = 0,count = allEffects.size(); i < count; i++) {
            EffectInfo eInfo = allEffects.get(i);
            if(eInfo.id == id){
                return eInfo;
            }
        }
        return null;
    }
    public static EffectInfo getCurrentEffect(Context context ){
        SharedPreferences mSpaceTypeShared = context.getSharedPreferences(WorkspaceSettings.WORKSPACE_STYLE, Context.MODE_PRIVATE);
        int id = mSpaceTypeShared.getInt(WorkspaceSettings.KEY_ANIMATION_STYLE, 0);

        for (int i = 0,count = allEffects.size(); i < count; i++) {
            EffectInfo eInfo = allEffects.get(i);
            if(eInfo.id == id){
                return eInfo;
            }
        }
        return null;
    }
    private static List<EffectInfo> loadEffectsList(){
            allEffects.clear();
            CubeEffect cubeEffect = new CubeEffect(1);
            allEffects.add(cubeEffect);
            RotateEffect rotateEffect = new RotateEffect(2);
            allEffects.add(rotateEffect);
            LayerEffect layerEffect = new LayerEffect(3);
            allEffects.add(layerEffect);
            FadeEffect fadeEffect = new FadeEffect(4);
            allEffects.add(fadeEffect);
            PageEffect pageEffect = new PageEffect(5);
            allEffects.add(pageEffect);
            CrossEffect crossEffect = new CrossEffect(6);
            allEffects.add(crossEffect);
            return allEffects;
    }
}
