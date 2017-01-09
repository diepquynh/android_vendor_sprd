package com.sprd.appbackup.service;

import java.io.File;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CountDownLatch;

import com.sprd.appbackup.utils.DataUtil;
import com.sprd.appbackup.utils.StorageUtil;

import android.app.Service;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Binder;
import android.os.Bundle;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.text.TextUtils;
import android.util.Log;

class AppBackupRepository extends IAppBackupRepository.Stub {
    private boolean mIsOldVersionFile = false;
    private String mPath;
    private String mAgentName;

    public AppBackupRepository(Archive archive, Agent agent, int code) {
        mPath = archive.mPath;
        if(agent.getAgentName().equals("Mms")){
            if(code == 0){
                mAgentName = "Sms";
            }else if(code == 1){
                mAgentName = "Mms";
            }else{
                mAgentName = agent.mAgentName;
            }
        }else{
            mAgentName = agent.mAgentName;
        }
        if (mPath.contains(Config.OLD_VERSION_DATA_PATH_INTERNAL)
                || mPath.contains(Config.OLD_VERSION_DATA_PATH_EXTERNAL)) {
            mIsOldVersionFile = true;
            mAgentName = mAgentName.toLowerCase();
            if (mAgentName.equals("contact")) {
                mAgentName = "contacts";
            }
        }
        Log.i("AppBackupRepository", "mPath=  " + mPath + "/" + mAgentName);
        File f = new File(mPath + "/" + mAgentName);
        f.mkdirs();
    }

    public ParcelFileDescriptor read(String name){
        try {
            return ParcelFileDescriptor.open(new File(mPath + "/" + mAgentName
                    + "/" + name), ParcelFileDescriptor.MODE_READ_ONLY);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    public ParcelFileDescriptor write(String name) {
        try {
            return ParcelFileDescriptor.open(new File(mPath + "/" + mAgentName
                    + "/" + name), ParcelFileDescriptor.MODE_READ_WRITE
                    | ParcelFileDescriptor.MODE_CREATE
                    | ParcelFileDescriptor.MODE_TRUNCATE);
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    public boolean isOldVersionFile() {
        return mIsOldVersionFile;
    }

    public String getOldVersionFilePath() {
        String filePath = mPath + "/" + mAgentName;
        if (mIsOldVersionFile) {
            File file = new File(filePath);
            if (file.exists() && file.isDirectory()) {
                return filePath;
            }
        }
        return null;
    }
}

public class AppBackupManagerService extends Service {

    private static final String KEY_SERVICE_NAME = "service_name";
    private static final String KEY_AGENT_NAME = "agent_name";
    private static boolean DEBUG = true;
    private static final String TAG = "AppBackupManagerService";

    // mAgents: the collection of already connected agents
    private ConcurrentHashMap<Agent, IAppBackupAgent> mAgents = new ConcurrentHashMap<Agent, IAppBackupAgent>();
    private Context mContext;

    private ConcurrentHashMap<Agent, ServiceConnection> mConnections;
    private static int mAgentCount = 0;
    private long mTimeStart, mTimeEnd;

    private IScanAgentAndArchiveListener mScanListener;  
    private Runnable mScanAgents = new Runnable() {

        @Override
        public void run() {
            // TODO Auto-generated method stub
            scanAgents();
        }
    };

    private Binder mBinder = new IAppBackupManager.Stub() {

        public int requestBackup(Archive archive, Agent agent,
                IAppBackupRestoreObserver observer, int code, List<Account> account) {
            IAppBackupAgent iagent = mAgents.get(agent);
            try {
                if(iagent != null){
                    return iagent.onBackup(new AppBackupRepository(archive, agent, code),
                            observer, code, account);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
            return -1;
        }

        public int requestRestore(Archive archive, Agent agent,
                IAppBackupRestoreObserver observer, int code) {
            IAppBackupAgent iagent = mAgents.get(agent);
            AppBackupRepository repository = new AppBackupRepository(archive,
                    agent, code);
            try {
                if(iagent != null){
                    return iagent.onRestore(repository, observer, code);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
            return -1;
        }

        public int requestCancel(Agent agent, int code) {
            try {
                if(mAgents != null && mAgents.get(agent) != null){
                    return mAgents.get(agent).onCancel(code);
                }
            } catch (Exception e) {
            }
            return 0;
        }

        public int requestDeduplicate(Agent agent, IAppBackupRestoreObserver observer, int code){
            try{
                if(mAgents != null && mAgents.get(agent) != null){
                    return mAgents.get(agent).onDeduplicate(observer, code);
                }
            }catch(Exception e){
                e.printStackTrace();
            }
            return 0;
        }

        public void setScanCompleteListener(IScanAgentAndArchiveListener listener) {

            mScanListener = listener;
            if (mScanListener == null) {
                return;
            }
            Log.i(TAG, "mAgentCount=" + mAgentCount + " || mAgents.size()=" + mAgents.size());
            if (mAgentCount != 0 && mAgents.size() == mAgentCount) {
                try {
                    mScanListener.onScanComplete();
                    Log.d(TAG, "onScanComplete while setCanCompleteListener !");
                } catch (RemoteException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
        }

        public boolean isEnabled(Agent agent, int code) {
            try {
                if(mAgents != null && mAgents.get(agent) != null){
                    return mAgents.get(agent).isEnabled(code);
                }
            } catch (RemoteException e) {
                e.printStackTrace();
            }
            return false;
        }

        public Category[] getCategory(Agent agent) {
            if (null == agent) {
                return null;
            }
            try {
                if(mAgents != null && mAgents.get(agent) != null){
                    return mAgents.get(agent).getCategory();
                }
            } catch (RemoteException e) {
                e.printStackTrace();
            }
            return null;
        }

        public List<Account> getAccounts(Agent agent, int code){
            try{
                if(mAgents != null && mAgents.get(agent) != null){
                    return mAgents.get(agent).getAccounts(code);
                }
            }catch(RemoteException e){
                e.printStackTrace();
            }
            return null;
        }
        public String getBackupInfo(Archive archive, Agent agent) {
            try {
                if(mAgents != null && mAgents.get(agent) != null){
                    return mAgents.get(agent).getBackupInfo(
                            new AppBackupRepository(archive, agent, 0));
                }
            } catch (Exception e) {
            }
            return null;
        }

        public Agent[] getAgents() {
            if (DEBUG) {
                Log.e(TAG, "mAgents.size()=" + mAgents.size());
            }
            return mAgents.keySet().toArray(new Agent[] {});
        }
    };

    public void onCreate() {
        if (DEBUG) {
            Log.e(TAG, "onCreate()");
        }
        mContext = this.getApplicationContext();
        new  Thread(mScanAgents).start();
    }

    public Binder onBind(Intent intent) {
        return mBinder;
    }

    private void scanAgents() {
        if (DEBUG) {
            Log.e(TAG, "scanAgents()");
        }
        // fill mAgents
        ArrayList<Agent> tmpArrayList = new ArrayList<Agent>();
        PackageManager pm = mContext.getPackageManager();
        List<PackageInfo> packages = pm.getInstalledPackages(0);
        for (PackageInfo pkgInfo : packages) {
            String pkgName = pkgInfo.packageName;
            ApplicationInfo aInfo = null;
            try {
                aInfo = pm.getApplicationInfo(pkgName,
                        PackageManager.GET_META_DATA | PackageManager.GET_UNINSTALLED_PACKAGES);

            } catch (PackageManager.NameNotFoundException e) {
                e.printStackTrace();
                continue;
            }

            Bundle bundle = aInfo.metaData;
            if (bundle == null) {
                continue;
            }

            String serviceName = bundle.getString(AppBackupManagerService.KEY_SERVICE_NAME);
            String agentName = bundle.getString(AppBackupManagerService.KEY_AGENT_NAME);
            if (TextUtils.isEmpty(serviceName) || TextUtils.isEmpty(agentName)) {
                continue;
            }
            String packageName = aInfo.packageName;
            String applicationLabel = (String) pm.getApplicationLabel(aInfo);
            Agent agent = new Agent(packageName, applicationLabel, serviceName,
                    agentName);
            tmpArrayList.add(agent);
            if (DEBUG) {
                Log.e(TAG, "scanAgents:" + agent);
            }
        }

        final ArrayList<Agent> agentList = new ArrayList<Agent>(tmpArrayList);
        mAgentCount = agentList.size();
        Log.i(TAG, "mAgentCount=" + mAgentCount);
        mConnections = new ConcurrentHashMap<Agent, ServiceConnection>();
        for (int i = 0; i < mAgentCount; i++) {
            final Agent agent = agentList.get(i);
            ServiceConnection srvConn = new ServiceConnection() {
                public void onServiceConnected(ComponentName name,
                        IBinder service) {
                    if (DEBUG) {
                        Log.e(TAG, agent.toString() + " connected");
                    }
                    IAppBackupAgent agentFromAsInterface = null;
                    agentFromAsInterface = IAppBackupAgent.Stub.asInterface(service);
                    if(mAgents != null && agentFromAsInterface != null){
                        mAgents.put(agent, agentFromAsInterface);
                    }else{
                        Log.w(TAG,"CountDownLatch, mAgents or agentFromAsInterface is null!");
                    }
                    if (mScanListener != null) {
                        try {
                            mScanListener.onServiceConnectionChanged(agent.getAgentName(), true);
                        } catch (RemoteException e) {
                            e.printStackTrace();
                        }
                    }
                }

                public void onServiceDisconnected(ComponentName name) {
                    Log.d(TAG, "onServiceDisconnected: " + agent.getAgentName());
                    if (mScanListener != null) {
                        try {
                            mScanListener.onServiceConnectionChanged(agent.getAgentName(), false);
                        } catch (RemoteException e) {
                            e.printStackTrace();
                        }
                    }
                    mAgents.remove(agent);
                    //rebind disconnected agentService
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            for (; ;) {
                                if (allAgentServiceConnected(1000, agentList)) {
                                    Log.i(TAG, "onServiceDisconnected,bind all service complete!");
                                    if (mScanListener != null) {
                                        try {
                                            mScanListener.onScanComplete();
                                            Log.d(TAG, "onServiceDisconnected,onScanComplete while scan done !");
                                        } catch (RemoteException e) {
                                            // TODO Auto-generated catch block
                                            e.printStackTrace();
                                        }
                                    }
                                    return;
                                }
                            }
                        }
                    }).start();
                }
            };
            mConnections.put(agent, srvConn);
        }
        Iterator iter = mConnections.keySet().iterator();
        while (iter.hasNext()) {
            Agent agent = (Agent)iter.next();
            Intent intent = new Intent();
            intent.setComponent(new ComponentName(agent.mPackageName,
                    agent.mServiceName));
            ServiceConnection srvConn = mConnections.get(agent);
            bindService(intent, srvConn, Context.BIND_AUTO_CREATE);
        }

        mTimeStart = mTimeEnd = System.currentTimeMillis();
        //wait all agentService to be binded.
        for (; ;) {
            if (allAgentServiceConnected(2500, agentList)) {
                Log.i(TAG, "bind all service complete!");
                if (mScanListener != null) {
                    try {
                        mScanListener.onScanComplete();
                        Log.d(TAG, "onScanComplete while scan done !");
                    } catch (RemoteException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                }
                break;
            }
        }
        if (DEBUG) {
            Log.e(TAG, "scanAgents() end");
        }
    }

    private boolean allAgentServiceConnected(long timeDelay, ArrayList<Agent> agentList) {
        mTimeEnd = System.currentTimeMillis();
        if (mAgentCount != 0 && mAgents.size() == mAgentCount) {
            return true;
        }
        if (mTimeEnd - mTimeStart > timeDelay) {
            Log.i(TAG, "mAgentCount=" + mAgentCount + " || mAgents.size()=" + mAgents.size());
            mTimeStart = mTimeEnd = System.currentTimeMillis();
            Log.i(TAG, "Look, look,this is a retry to bind agentService !");
            ArrayList<String> bindedAgents = new ArrayList<String>();
            Iterator iterator = mAgents.keySet().iterator();
            while (iterator.hasNext()) {
                Agent bindedAgent = (Agent)iterator.next();
                bindedAgents.add(bindedAgent.getAgentName());
            }
            for (Agent tmpAgent : agentList) {
                if (!bindedAgents.contains(tmpAgent.getAgentName())) {
                    Intent intent = new Intent();
                    intent.setComponent(new ComponentName(tmpAgent.mPackageName,
                            tmpAgent.mServiceName));
                    Log.d(TAG, "retry to bindService: " + tmpAgent.mServiceName);
                    bindService(intent, mConnections.get(tmpAgent), Context.BIND_AUTO_CREATE);
                }
            }
        }
        return false;
    }
    @Override
    public boolean onUnbind(Intent intent) {
        if(DEBUG) {Log.d(TAG, "onUnbind()");}
        return super.onUnbind(intent);
    }

    @Override
    public void onDestroy() {
        if(DEBUG) {Log.d(TAG, "onDestroy()");}
        if(mAgents != null && mConnections != null){
            Iterator iter = mAgents.keySet().iterator();
            while (iter.hasNext()) {
                Agent bindedAgent = (Agent)iter.next();
                ServiceConnection sc = mConnections.get(bindedAgent);
                if(sc != null){
                    Log.i(TAG, "unbindService of agent: " + bindedAgent.getAgentName());
                    unbindService(sc);
                }
            }
        }
        super.onDestroy();
    }

}
