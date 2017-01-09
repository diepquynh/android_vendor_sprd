
package com.sprd.engineermode.connectivity.fm;

import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.File;
import java.io.FileOutputStream;

import android.app.Fragment;
import android.app.FragmentTransaction;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;
import android.util.Log;
import com.sprd.engineermode.R;

import com.android.fmradio.FmManagerSelect;
import android.os.Handler;
import android.os.Message;

public class FMFragment extends AbsFMFragment implements OnItemClickListener {
    public static final String TAG = "FMFragment";
    public static final int TURN = 0;
    public static final int SEEK = 1;
    public static final int SET_EARPHONE = 0;
    private static final String HEADSET_STATE_PATH = "/sys/class/switch/h2w/state";
  //begin 562936 add by suyan.yang 2016.05.14
    private ListView fmListView=null;

   //end 562936 add by suyan.yang 2016.05.14
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return initView();
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        switch (position) {
            //case TURN:
//                replaceFragment(new TurnFragment());
//               break;
            //case SEEK:
//                replaceFragment(new SeekFragment());
//                break;
            case SET_EARPHONE:
                replaceFragment(new EarphoneFragment());
                break;
            default:
                break;
        }
    }

    //begin 562936 add by suyan.yang 2016.05.14
    @Override
    public void onDestroy() {
        super.onDestroy();
    }
    //end 562936 add by suyan.yang 2016.05.14

    private boolean isHeadsetExists() {
        char[] buffer = new char[1024];
        int newState = 0;
        FileReader file = null;
        try {
            file = new FileReader(HEADSET_STATE_PATH);
            int len = file.read(buffer, 0, 1024);
            newState = Integer.valueOf((new String(buffer, 0, len)).trim());
        } catch (FileNotFoundException e) {
            Log.e("FMTest", "This kernel does not have wired headset support");
        } catch (Exception e) {
            Log.e("FMTest", "", e);
        } finally {
            if (file != null) {
                try {
                    file.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        return newState != 0;
    }

    public View initView() {
        View view = View.inflate(getActivity(), R.layout.fm_fragment_main, null);
      //begin 562936 modify by suyan.yang 2016.05.14
        fmListView = (ListView) view.findViewById(R.id.fm_listview);
        fmListView.setOnItemClickListener(this);
//        String[] adapterData = new String[] {
//                getResources().getString(R.string.turn), getResources().getString(R.string.seek),
//                getResources().getString(R.string.earphone)
//        };
        String[] adapterData = new String[] {
                getResources().getString(R.string.earphone)
        };
        ArrayAdapter<String> fmAdapter = new ArrayAdapter<String>(getActivity(),
                android.R.layout.simple_list_item_1, adapterData);
        fmListView.setAdapter(fmAdapter);
      //end 562936 modify by suyan.yang 2016.05.14
        return view;
    }

    public void replaceFragment(Fragment fragment) {
        FragmentTransaction fragmentTransaction = getActivity().getFragmentManager()
                .beginTransaction();
        fragmentTransaction.replace(R.id.fm_framelayout, fragment);
        fragmentTransaction.addToBackStack(null);
        fragmentTransaction.commitAllowingStateLoss();
    }

}
