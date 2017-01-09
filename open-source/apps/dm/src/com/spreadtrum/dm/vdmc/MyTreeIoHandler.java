
package com.spreadtrum.dm.vdmc;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import android.content.ContentResolver;
import android.content.Context;
import android.util.Log;
import android.provider.Settings;

//import com.redbend.vdm.*;
//import com.redbend.vdm.VdmException.VdmError;
/*Start of  zhuzhongwei 2011.2.14*/
import android.net.Uri;
import android.database.Cursor;
import android.content.Intent;
import android.text.TextUtils; /*End   of  zhuzhongwei 2011.2.14*/

// xuguoqing modify, 2011-02-15
import android.provider.Telephony;
import android.content.ContentValues;

import com.android.internal.telephony.TelephonyProperties;
import com.spreadtrum.dm.DmService;

/*Start of  oasis_zp@hisense  2011.2.14*/
import android.location.LocationManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.content.SharedPreferences;
import com.android.internal.telephony.Phone;
//import com.android.internal.telephony.PhoneFactory;
import android.telephony.TelephonyManager;
/*End   of  oasis_zp@hisense  2011.2.14*/

/**
 * Sample class which demonstrates DM Tree IO handler. Each node requires a
 * different instance of the class.
 */
public class MyTreeIoHandler/* implements NodeIoHandler */{
    private String TAG = Vdmc.DM_TAG + "MyTreeIoHandler: ";

    // private static final String OEM_INFO_STR = "Redbend";
    private static final String OEM_INFO_STR = "spreadtrum";

    private static final String LANG_INFO_STR = "en-US";

    private static final String DM_VER_INFO_STR = "1.2";

    private static final String FW_VER_INFO_STR = "Android 2.2";

    private static final String HW_VER_INFO_STR = "8801h";

    private static final String PIM_CONFIG = "pimConfig";
	
    private static MyTreeIoHandler mInstance = null;

    private static final int DEVID_IO_HANDLER = 0;

    private static final int FROMFILE_IO_HANDLER = 1;

    private static final int MODEL_IO_HANDLER = 2;

    private static final int MAN_IO_HANDLER = 3;

    private static final int OEM_IO_HANDLER = 4;

    private static final int LANG_IO_HANDLER = 5;

    private static final int DMVERSION_IO_HANDLER = 6;

    private static final int FWVERSION_IO_HANDLER = 7;

    private static final int SWVERSION_IO_HANDLER = 8;

    private static final int HWVERSION_IO_HANDLER = 9;

    private static final int SERVER_ADDR_IO_HANDLER = 10;

    // DM Setting
    private static final int DM_CONN_PROFILE_IO_HANDLER = 11;

    private static final int DM_APN_IO_HANDLER = 12;

    private static final int DM_PROXY_IO_HANDLER = 13;

    private static final int DM_PORT_IO_HANDLER = 14;

    // GPRS-CMNET Setting
    private static final int GPRS_CMNET_APN_IO_HANDLER = 15;

    private static final int GPRS_CMNET_PROXY_IO_HANDLER = 16;

    private static final int GPRS_CMNET_PORT_IO_HANDLER = 17;

    private static final int GPRS_CMNET_USERNAME_IO_HANDLER = 18;

    private static final int GPRS_CMNET_PASSWORD_IO_HANDLER = 19;

    // GPRS-CMWAP Setting
    private static final int GPRS_CMWAP_APN_IO_HANDLER = 20;

    private static final int GPRS_CMWAP_PROXY_IO_HANDLER = 21;

    private static final int GPRS_CMWAP_PORT_IO_HANDLER = 22;

    private static final int GPRS_CMWAP_USERNAME_IO_HANDLER = 23;

    private static final int GPRS_CMWAP_PASSWORD_IO_HANDLER = 24;

    // WAP Setting
    private static final int WAP_CONNPROFILE_IO_HANDLER = 25;

    private static final int WAP_HOMEPAGE_IO_HANDLER = 26;

    private static final int WAP_PROXY_IO_HANDLER = 27;

    private static final int WAP_PORT_IO_HANDLER = 28;

    private static final int WAP_USERNAME_IO_HANDLER = 29;

    private static final int WAP_PASSWORD_IO_HANDLER = 30;

    // MMS Setting
    private static final int MMS_CONNPROFILE_IO_HANDLER = 31;

    private static final int MMS_MMSC_IO_HANDLER = 32;

    // PIM Setting
    private static final int PIM_CONNPROFILE_URI_IO_HANDLER = 33;

    private static final int PIM_SERVER_ADDR_IO_HANDLER = 34;

    private static final int PIM_ADDRESS_BOOK_URI_IO_HANDLER = 35;

    private static final int PIM_CALENDAR_URI_IO_HANDLER = 36;

    // PushMail Setting
    private static final int MAIL_CONNPROFILE_IO_HANDER = 37;

    private static final int MAIL_SEND_SERVER_IO_HANDER = 38;

    private static final int MAIL_SEND_PORT_IO_HANDER = 39;

    private static final int MAIL_SEND_USE_SEC_CON_IO_HANDER = 40;

    private static final int MAIL_RECV_SERVER_IO_HANDER = 41;

    private static final int MAIL_RECV_PORT_IO_HANDER = 42;

    private static final int MAIL_RECV_USE_SEC_CON_IO_HANDER = 43;

    private static final int MAIL_RECV_PROTOCAL_IO_HANDER = 44;

    // Streaming Setting
    private static final int STREAMING_CONNPROFILE_IO_HANDLER = 45;

    private static final int STREAMING_NAME_IO_HANDLER = 46;

    private static final int STREAMING_MAX_UDP_PORT_IO_HANDLER = 47;

    private static final int STREAMING_MIN_UDP_PORT_IO_HANDLER = 48;

    private static final int STREAMING_NET_INFO_IO_HANDLER = 49;

    // AGPS Setting
    public static final int AGPS_CONNPROFILE_IO_HANDLER = 50;

    public static final int AGPS_SERVER_IO_HANDLER = 51;

    public static final int AGPS_SERVER_NAME_IO_HANDLER = 52;

    public static final int AGPS_IAPID_IO_HANDLER = 53;

    public static final int AGPS_PORT_IO_HANDLER = 54;

    public static final int AGPS_PROVIDER_ID_IO_HANDLER = 55;
    
    public static final int AGPS_PREFCONREF_ID_IO_HANDLER=62;
    
    public static final int AGPS_CONREF_IO_HANDLER=63;

//DM WAP Setting
	private static final int DM_WAP_CONN_PROFILE_IO_HANDLER=56;
	private static final int DM_WAP_APN_IO_HANDLER=57;
	private static final int DM_WAP_PROXY_IO_HANDLER=58;
	private static final int DM_WAP_PORT_IO_HANDLER=59;
	private static final int DM_SAVE_CRED = 60;
	private static final int BROWSER_HOMEPAGE_IO_HANDLER = 61;

    /*
     * public enum HandlerType { DEVID_IO_HANDLER, FROMFILE_IO_HANDLER,
     * MODEL_IO_HANDLER, MAN_IO_HANDLER, OEM_IO_HANDLER, LANG_IO_HANDLER,
     * DMVERSION_IO_HANDLER, FWVERSION_IO_HANDLER, SWVERSION_IO_HANDLER,
     * HWVERSION_IO_HANDLER, SERVER_ADDR_IO_HANDLER, //DM Setting
     * DM_CONN_PROFILE_IO_HANDLER, DM_APN_IO_HANDLER, DM_PROXY_IO_HANDLER,
     * DM_PORT_IO_HANDLER, //GPRS-CMNET Setting GPRS_CMNET_APN_IO_HANDLER,
     * GPRS_CMNET_PROXY_IO_HANDLER, GPRS_CMNET_PORT_IO_HANDLER,
     * GPRS_CMNET_USERNAME_IO_HANDLER, GPRS_CMNET_PASSWORD_IO_HANDLER,
     * //GPRS-CMWAP Setting GPRS_CMWAP_APN_IO_HANDLER,
     * GPRS_CMWAP_PROXY_IO_HANDLER, GPRS_CMWAP_PORT_IO_HANDLER,
     * GPRS_CMWAP_USERNAME_IO_HANDLER, GPRS_CMWAP_PASSWORD_IO_HANDLER, //WAP
     * Setting WAP_CONNPROFILE_IO_HANDLER, WAP_HOMEPAGE_IO_HANDLER,
     * WAP_PROXY_IO_HANDLER, WAP_PORT_IO_HANDLER, WAP_USERNAME_IO_HANDLER,
     * WAP_PASSWORD_IO_HANDLER, //MMS Setting MMS_CONNPROFILE_IO_HANDLER,
     * MMS_MMSC_IO_HANDLER, //PIM Setting PIM_CONNPROFILE_URI_IO_HANDLER,
     * PIM_SERVER_ADDR_IO_HANDLER, PIM_ADDRESS_BOOK_URI_IO_HANDLER,
     * PIM_CALENDAR_URI_IO_HANDLER, //PushMail Setting
     * MAIL_CONNPROFILE_IO_HANDER, MAIL_SEND_SERVER_IO_HANDER,
     * MAIL_SEND_PORT_IO_HANDER, MAIL_SEND_USE_SEC_CON_IO_HANDER,
     * MAIL_RECV_SERVER_IO_HANDER, MAIL_RECV_PORT_IO_HANDER,
     * MAIL_RECV_USE_SEC_CON_IO_HANDER, MAIL_RECV_PROTOCAL_IO_HANDER,
     * //Streaming Setting STREAMING_CONNPROFILE_IO_HANDLER,
     * STREAMING_NAME_IO_HANDLER, STREAMING_MAX_UDP_PORT_IO_HANDLER,
     * STREAMING_MIN_UDP_PORT_IO_HANDLER, STREAMING_NET_INFO_IO_HANDLER, //AGPS
     * Setting AGPS_CONNPROFILE_IO_HANDLER, AGPS_SERVER_IO_HANDLER,
     * AGPS_SERVER_NAME_IO_HANDLER, AGPS_IAPID_IO_HANDLER, AGPS_PORT_IO_HANDLER,
     * AGPS_PROVIDER_ID_IO_HANDLER, };
     */

    // public MyTreeIoHandler(HandlerType t, Context c) {
    public MyTreeIoHandler(int t, Context c) {
        _context = c;
        _handlerType = t;
    }

    public MyTreeIoHandler() {

    }

    public MyTreeIoHandler(Context c) {
        _context = c;
        mInstance = this;
    }

    public static MyTreeIoHandler getInstance(Context mContext) {
         if (null == mInstance)
         {
         mInstance = new MyTreeIoHandler(mContext);
         }
        return mInstance;
    }

    /**
     * @return node URI handled by this instance.
     */
    public String getNodeUri() {
        String s = null;
        switch (_handlerType) {
            case DEVID_IO_HANDLER:
                s = "./DevInfo/DevId";
                break;
            case FROMFILE_IO_HANDLER:
                s = "./Ext/IOHandlerTest";
                break;

            case MODEL_IO_HANDLER:
                s = "./DevInfo/Mod";
                break;
            case MAN_IO_HANDLER:
                s = "./DevInfo/Man";
                break;
            case OEM_IO_HANDLER:
                s = "./DevDetail/OEM";
                break;
            case LANG_IO_HANDLER:
                s = "./DevInfo/Lang";
                break;
            case DMVERSION_IO_HANDLER:
                s = "./DevInfo/DmV";
                break;
            case FWVERSION_IO_HANDLER:
                s = "./DevDetail/FwV";
                break;
            case SWVERSION_IO_HANDLER:
                s = "./DevDetail/SwV";
                break;
            case HWVERSION_IO_HANDLER:
                s = "./DevDetail/HwV";
                break;
            case SERVER_ADDR_IO_HANDLER:
                s = "./DMAcc/AndroidAcc/AppAddr/SrvAddr/Addr";
                break;

            // DM setting
            case DM_CONN_PROFILE_IO_HANDLER:
                s = "./Settings/DM/ConnProfile";
                break;
            case DM_APN_IO_HANDLER:
                s = "./Settings/DM/APN";
                break;
            case DM_PROXY_IO_HANDLER:
                s = "./Settings/DM/Proxy";
                break;
            case DM_PORT_IO_HANDLER:
                s = "./Settings/DM/Port";
                break;

            // GPRS-CMNET Setting
            case GPRS_CMNET_APN_IO_HANDLER:
                s = "./Settings/GPRS/CMNet/APN";
                break;
            case GPRS_CMNET_PROXY_IO_HANDLER:
                s = "./Settings/GPRS/CMNet/ProxyAddr";
                break;
            case GPRS_CMNET_PORT_IO_HANDLER:
                s = "./Settings/GPRS/CMNet/ProxyPortNbr";
                break;
            case GPRS_CMNET_USERNAME_IO_HANDLER:
                s = "./Settings/GPRS/CMNet/UserName";
                break;
            case GPRS_CMNET_PASSWORD_IO_HANDLER:
                s = "./Settings/GPRS/CMNet/PassWord";
                break;

            // GPRS-CMWAP Setting
            case GPRS_CMWAP_APN_IO_HANDLER:
                s = "./Settings/GPRS/CMWap/APN";
                break;
            case GPRS_CMWAP_PROXY_IO_HANDLER:
                s = "./Settings/GPRS/CMWap/ProxyAddr";
                break;
            case GPRS_CMWAP_PORT_IO_HANDLER:
                s = "./Settings/GPRS/CMWap/ProxyPortNbr";
                break;
            case GPRS_CMWAP_USERNAME_IO_HANDLER:
                s = "./Settings/GPRS/CMWap/UserName";
                break;
            case GPRS_CMWAP_PASSWORD_IO_HANDLER:
                s = "./Settings/GPRS/CMWap/PassWord";
                break;

            // WAP Setting
            case WAP_CONNPROFILE_IO_HANDLER:
                s = "./Settings/WAP/ConnProfile";
                break;
            case WAP_HOMEPAGE_IO_HANDLER:
                s = "./Settings/WAP/StartPage";
                break;
            case WAP_PROXY_IO_HANDLER:
                s = "./Settings/WAP/ProxyAddr";
                break;
            case WAP_PORT_IO_HANDLER:
                s = "./Settings/WAP/ProxyPortNbr";
                break;
            case WAP_USERNAME_IO_HANDLER:
                s = "./Settings/WAP/UserName";
                break;
            case WAP_PASSWORD_IO_HANDLER:
                s = "./Settings/WAP/PassWord";
                break;

            // MMS Setting
            case MMS_CONNPROFILE_IO_HANDLER:
                s = "./Settings/MMS/ConnProfile";
                break;
            case MMS_MMSC_IO_HANDLER:
                s = "./Settings/MMS/MMSC";
                break;

            // PIM Setting
            case PIM_CONNPROFILE_URI_IO_HANDLER:
                s = "./Settings/PIM/ConnProfile";
                break;
            case PIM_SERVER_ADDR_IO_HANDLER:
                s = "./Settings/PIM/Addr";
                break;
            case PIM_ADDRESS_BOOK_URI_IO_HANDLER:
                s = "./Settings/PIM/AddressBookURI";
                break;
            case PIM_CALENDAR_URI_IO_HANDLER:
                s = "./Settings/PIM/CalendarURI";
                break;

            // PushMail Setting
            case MAIL_CONNPROFILE_IO_HANDER:
                s = "./Settings/PushMail/ConnProfile";
                break;
            case MAIL_SEND_SERVER_IO_HANDER:
                s = "./Settings/PushMail/SendServer";
                break;
            case MAIL_SEND_PORT_IO_HANDER:
                s = "./Settings/PushMail/SendPort";
                break;
            case MAIL_SEND_USE_SEC_CON_IO_HANDER:
                s = "./Settings/PushMail/SendUseSecCon";
                break;
            case MAIL_RECV_SERVER_IO_HANDER:
                s = "./Settings/PushMail/RecvServer";
                break;
            case MAIL_RECV_PORT_IO_HANDER:
                s = "./Settings/PushMail/RecvPort";
                break;
            case MAIL_RECV_USE_SEC_CON_IO_HANDER:
                s = "./Settings/PushMail/RecvUseSecCon";
                break;
            case MAIL_RECV_PROTOCAL_IO_HANDER:
                s = "./Settings/PushMail/RecvPro";
                break;

            // Streaming Setting
            case STREAMING_CONNPROFILE_IO_HANDLER:
                s = "./Settings/Streaming/ConnProfile";
                break;
            case STREAMING_NAME_IO_HANDLER:
                s = "./Settings/Streaming/Name";
                break;
            case STREAMING_MAX_UDP_PORT_IO_HANDLER:
                s = "./Settings/Streaming/MaxUdpPort";
                break;
            case STREAMING_MIN_UDP_PORT_IO_HANDLER:
                s = "./Settings/Streaming/MinUdpPort";
                break;
            case STREAMING_NET_INFO_IO_HANDLER:
                s = "./Settings/Streaming/NetInfo";
                break;

            // AGPS Setting
            case AGPS_CONNPROFILE_IO_HANDLER:
                s = "./Settings/AGPS/ConnProfile";
                break;
            case AGPS_SERVER_IO_HANDLER:
                s = "./Settings/AGPS/AGPSServer";
                break;
            case AGPS_SERVER_NAME_IO_HANDLER:
                s = "./Settings/AGPS/AGPSName";
                break;
            case AGPS_IAPID_IO_HANDLER:
                s = "./Settings/AGPS/IAPID";
                break;
            case AGPS_PORT_IO_HANDLER:
                s = "./Settings/AGPS/AGPSServerPort";
                break;
            case AGPS_PROVIDER_ID_IO_HANDLER:
                s = "./Settings/AGPS/ProviderIP";
                break;
        }
        return s;
    }

    // ///////////////////////////////////
    // NodeIoHandler implementation
    // ///////////////////////////////////

    /*
     * (non-Javadoc)
     * @see com.redbend.vdm.NodeIoHandler#read(int, byte[])
     */
    // public int read(HandlerType handlerType,int offset, byte[] data) /*throws
    // VdmException*/ {
    public int read(int handlerType, int offset, byte[] data) /*
                                                               * throws
                                                               * VdmException
                                                               */{
        int ret = 0;
        offset = 0;
        Log.d(TAG, "read: handlerType new= " + handlerType + ", offset = " + offset);


		
        ByteBuffer buf = null;
        String str = null;
        _handlerType = handlerType;
        switch (_handlerType) {
            case DEVID_IO_HANDLER:
                // String imei = "IMEI:112222223333332";
                String imei = DmService.getInstance().getImei();
                Log.d(TAG, "read: imei =  " + imei);
                ret = imei.length();
                if (data == null) {
                    Log.d(TAG, "read: data is null!");
                    break;
                }
                buf = ByteBuffer.wrap(data);
                Log.d(TAG, "read: buf = " + buf);
                buf.put(imei.getBytes());
                break;
            case FROMFILE_IO_HANDLER:
                try {
                    FileInputStream is = _context.openFileInput("TreeExternalNode");
                    ret = is.available();
                    if (data != null) {
                        ret = is.read(data, offset, data.length);
                    }
                    is.close();
                } catch (IOException e) {
                    Log.e("DMC", "TreeHandler: Failed to read from external file");
                    // throw new VdmException(VdmError.STORAGE_READ);
                    return -1;
                }
                break;

            case MODEL_IO_HANDLER:
                str = DmService.getInstance().getModel();
                Log.d(TAG, "read: model =  " + str);
                ret = readData(str, offset, data);
                break;
            case MAN_IO_HANDLER:
                str = DmService.getInstance().getManufactory();
                Log.d(TAG, "read: manufactory =  " + str);
                ret = readData(str, offset, data);
                break;
            case OEM_IO_HANDLER:
                str = OEM_INFO_STR;
                Log.d(TAG, "read: OEM =  " + str);
                ret = readData(str, offset, data);
                break;
            case LANG_IO_HANDLER:
                str = LANG_INFO_STR;
                Log.d(TAG, "read: OEM =  " + str);
                ret = readData(str, offset, data);
                break;
            case DMVERSION_IO_HANDLER:
                str = DM_VER_INFO_STR;
                Log.d(TAG, "read: DM Version =  " + str);
                ret = readData(str, offset, data);
                break;
            case FWVERSION_IO_HANDLER:
                str = FW_VER_INFO_STR;
                Log.d(TAG, "read: FW Version =  " + str);
                ret = readData(str, offset, data);
                break;
            case SWVERSION_IO_HANDLER:
                str = DmService.getInstance().getSoftwareVersion();
                Log.d(TAG, "read: SW Version =  " + str);
                ret = readData(str, offset, data);
                break;
            case HWVERSION_IO_HANDLER:
                str = HW_VER_INFO_STR;
                Log.d(TAG, "read: HW Version =  " + str);
                ret = readData(str, offset, data);
                break;
            case SERVER_ADDR_IO_HANDLER:
                str = DmService.getInstance().getServerAddr();
                Log.d(TAG, "read: server addr =  " + str);
                ret = readData(str, offset, data);
                break;

            // DM Setting
            case DM_CONN_PROFILE_IO_HANDLER:
            case DM_APN_IO_HANDLER:
            case DM_PROXY_IO_HANDLER:
            case DM_PORT_IO_HANDLER:
                str = readDMParam(_handlerType);
                ret = readData(str, offset, data);
                break;

		case DM_WAP_CONN_PROFILE_IO_HANDLER:
		case DM_WAP_APN_IO_HANDLER:
		case DM_WAP_PROXY_IO_HANDLER:
		case DM_WAP_PORT_IO_HANDLER:
		Log.d(TAG, "read: cmccwap_dm handlerType = " + handlerType + ", offset = " + offset);
                str = readDMWAPParam(_handlerType);
                ret = readData(str, offset, data);
			break;

            // GPRS-CMNET Setting
            case GPRS_CMNET_APN_IO_HANDLER:
            case GPRS_CMNET_PROXY_IO_HANDLER:
            case GPRS_CMNET_PORT_IO_HANDLER:
            case GPRS_CMNET_USERNAME_IO_HANDLER:
            case GPRS_CMNET_PASSWORD_IO_HANDLER:
                str = readGprsCmnetParam(_handlerType);
                ret = readData(str, offset, data);
                break;

            // GPRS-CMWAP Setting
            case GPRS_CMWAP_APN_IO_HANDLER:
            case GPRS_CMWAP_PROXY_IO_HANDLER:
            case GPRS_CMWAP_PORT_IO_HANDLER:
            case GPRS_CMWAP_USERNAME_IO_HANDLER:
            case GPRS_CMWAP_PASSWORD_IO_HANDLER:
                str = readGprsCmwapParam(_handlerType);
                ret = readData(str, offset, data);
                break;


            // WAP Setting
            case WAP_CONNPROFILE_IO_HANDLER:
            case WAP_HOMEPAGE_IO_HANDLER:
            case WAP_PROXY_IO_HANDLER:
            case WAP_PORT_IO_HANDLER:
            case WAP_USERNAME_IO_HANDLER:
            case WAP_PASSWORD_IO_HANDLER:
	    case BROWSER_HOMEPAGE_IO_HANDLER:
				
                str = readWapParam(_handlerType);
                ret = readData(str, offset, data);
                break;

            // MMS Setting
            case MMS_CONNPROFILE_IO_HANDLER:
            case MMS_MMSC_IO_HANDLER:
                str = readMMSParam(_handlerType);
                ret = readData(str, offset, data);
                break;

            // PIM Setting
            case PIM_CONNPROFILE_URI_IO_HANDLER:
            case PIM_SERVER_ADDR_IO_HANDLER:
            case PIM_ADDRESS_BOOK_URI_IO_HANDLER:
            case PIM_CALENDAR_URI_IO_HANDLER:
	     case DM_SAVE_CRED:
                str = readPIMParam(_handlerType);
                ret = readData(str, offset, data);
                break;

            // PushMail Setting
            case MAIL_CONNPROFILE_IO_HANDER:
            case MAIL_SEND_SERVER_IO_HANDER:
            case MAIL_SEND_PORT_IO_HANDER:
            case MAIL_SEND_USE_SEC_CON_IO_HANDER:
            case MAIL_RECV_SERVER_IO_HANDER:
            case MAIL_RECV_PORT_IO_HANDER:
            case MAIL_RECV_USE_SEC_CON_IO_HANDER:
            case MAIL_RECV_PROTOCAL_IO_HANDER:
                str = readPushMailParam(_handlerType);
                ret = readData(str, offset, data);
                break;

            // Streaming Setting
            case STREAMING_CONNPROFILE_IO_HANDLER:
            case STREAMING_NAME_IO_HANDLER:
            case STREAMING_MAX_UDP_PORT_IO_HANDLER:
            case STREAMING_MIN_UDP_PORT_IO_HANDLER:
            case STREAMING_NET_INFO_IO_HANDLER:
                str = readStreamingParam(_handlerType);
                ret = readData(str, offset, data);
                break;

            // AGPS Setting
            case AGPS_CONNPROFILE_IO_HANDLER:
            case AGPS_SERVER_IO_HANDLER:
            case AGPS_SERVER_NAME_IO_HANDLER:
            case AGPS_IAPID_IO_HANDLER:
            case AGPS_PORT_IO_HANDLER:
            case AGPS_PROVIDER_ID_IO_HANDLER:
            case AGPS_PREFCONREF_ID_IO_HANDLER:
            case AGPS_CONREF_IO_HANDLER:
                str = readAGPSParam(_handlerType);
                ret = readData(str, offset, data);
                break;
            default:
                break;

        }

        return ret;
    }

    /*
     * (non-Javadoc)
     * @see com.redbend.vdm.NodeIoHandler#write(int, byte[], int)
     */

    // public void write(int offset, byte[] data, int totalSize)/* throws
    // VdmException*/ {
    public void write(int handlerType, int offset, byte[] data, int totalSize)/*
                                                                               * throws
                                                                               * VdmException
                                                                               */{
        Log.d(TAG, "write: handlerType = " + handlerType + ", offset = " + offset);
        String str = null;
        _handlerType = handlerType;
        switch (_handlerType) {

            case DEVID_IO_HANDLER:
                // Changing Device Id is not allowed.
                // throw new VdmException(VdmError.MAY_TREE_NOT_REPLACE);
                return;

            case FROMFILE_IO_HANDLER:

                break;

            // DM Setting
            case DM_CONN_PROFILE_IO_HANDLER:
            case DM_APN_IO_HANDLER:
            case DM_PROXY_IO_HANDLER:
            case DM_PORT_IO_HANDLER:
                str = new String(data);
                writeDMParam(_handlerType, str);
                break;

            // GPRS-CMNET Setting
            case GPRS_CMNET_APN_IO_HANDLER:
            case GPRS_CMNET_PROXY_IO_HANDLER:
            case GPRS_CMNET_PORT_IO_HANDLER:
            case GPRS_CMNET_USERNAME_IO_HANDLER:
            case GPRS_CMNET_PASSWORD_IO_HANDLER:
                str = new String(data);
                writeGprsCmnetParam(_handlerType, str);
                break;

            // GPRS-CMWAP Setting
            case GPRS_CMWAP_APN_IO_HANDLER:
            case GPRS_CMWAP_PROXY_IO_HANDLER:
            case GPRS_CMWAP_PORT_IO_HANDLER:
            case GPRS_CMWAP_USERNAME_IO_HANDLER:
            case GPRS_CMWAP_PASSWORD_IO_HANDLER:
                str = new String(data);
                writeGprsCmwapParam(_handlerType, str);
                break;

            // WAP Setting
            case WAP_CONNPROFILE_IO_HANDLER:
            case WAP_HOMEPAGE_IO_HANDLER:
            case WAP_PROXY_IO_HANDLER:
            case WAP_PORT_IO_HANDLER:
            case WAP_USERNAME_IO_HANDLER:
            case WAP_PASSWORD_IO_HANDLER:
	    case BROWSER_HOMEPAGE_IO_HANDLER:				
                str = new String(data);
                writeWapParam(_handlerType, str);
                break;

            // MMS Setting
            case MMS_CONNPROFILE_IO_HANDLER:
            case MMS_MMSC_IO_HANDLER:
                str = new String(data);
                writeMMSParam(_handlerType, str);
                break;

            // PIM Setting
            case PIM_CONNPROFILE_URI_IO_HANDLER:
            case PIM_SERVER_ADDR_IO_HANDLER:
            case PIM_ADDRESS_BOOK_URI_IO_HANDLER:
            case PIM_CALENDAR_URI_IO_HANDLER:
    	     case DM_SAVE_CRED:
                str = new String(data);
                writePIMParam(_handlerType, str);
                break;

            // PushMail Setting
            case MAIL_CONNPROFILE_IO_HANDER:
            case MAIL_SEND_SERVER_IO_HANDER:
            case MAIL_SEND_PORT_IO_HANDER:
            case MAIL_SEND_USE_SEC_CON_IO_HANDER:
            case MAIL_RECV_SERVER_IO_HANDER:
            case MAIL_RECV_PORT_IO_HANDER:
            case MAIL_RECV_USE_SEC_CON_IO_HANDER:
            case MAIL_RECV_PROTOCAL_IO_HANDER:
                str = new String(data);
                writePushMailParam(_handlerType, str);
                break;

            // Streaming Setting
            case STREAMING_CONNPROFILE_IO_HANDLER:
            case STREAMING_NAME_IO_HANDLER:
            case STREAMING_MAX_UDP_PORT_IO_HANDLER:
            case STREAMING_MIN_UDP_PORT_IO_HANDLER:
            case STREAMING_NET_INFO_IO_HANDLER:
                str = new String(data);
                writeStreamingParam(_handlerType, str);
                break;

            // AGPS Setting
            case AGPS_CONNPROFILE_IO_HANDLER:
            case AGPS_SERVER_IO_HANDLER:
            case AGPS_SERVER_NAME_IO_HANDLER:
            case AGPS_IAPID_IO_HANDLER:
            case AGPS_PORT_IO_HANDLER:
            case AGPS_PROVIDER_ID_IO_HANDLER:
            case AGPS_PREFCONREF_ID_IO_HANDLER:
            case AGPS_CONREF_IO_HANDLER:
                str = new String(data);
                writeAGPSParam(_handlerType, str);
                break;
		case DM_WAP_CONN_PROFILE_IO_HANDLER:
		case DM_WAP_APN_IO_HANDLER:
		case DM_WAP_PROXY_IO_HANDLER:
		case DM_WAP_PORT_IO_HANDLER:
			 str = new String(data);
                writeDMWAPParam(_handlerType,str);
			break;
            default:
                break;
        }

    }

    public void writenull(int handlerType)/* throws VdmException */{
        Log.d(TAG, "write null: _handlerType = " + _handlerType);
        String str = "";
        _handlerType = handlerType;
        switch (_handlerType) {

            case DEVID_IO_HANDLER:
                // Changing Device Id is not allowed.
                // throw new VdmException(VdmError.MAY_TREE_NOT_REPLACE);
                return;

                // DM Setting
            case DM_CONN_PROFILE_IO_HANDLER:
            case DM_APN_IO_HANDLER:
            case DM_PROXY_IO_HANDLER:
            case DM_PORT_IO_HANDLER:
                writeDMParam(_handlerType, str);
                break;

            // GPRS-CMNET Setting
            case GPRS_CMNET_APN_IO_HANDLER:
            case GPRS_CMNET_PROXY_IO_HANDLER:
            case GPRS_CMNET_PORT_IO_HANDLER:
            case GPRS_CMNET_USERNAME_IO_HANDLER:
            case GPRS_CMNET_PASSWORD_IO_HANDLER:
                writeGprsCmnetParam(_handlerType, str);
                break;

            // GPRS-CMWAP Setting
            case GPRS_CMWAP_APN_IO_HANDLER:
            case GPRS_CMWAP_PROXY_IO_HANDLER:
            case GPRS_CMWAP_PORT_IO_HANDLER:
            case GPRS_CMWAP_USERNAME_IO_HANDLER:
            case GPRS_CMWAP_PASSWORD_IO_HANDLER:
                writeGprsCmwapParam(_handlerType, str);
                break;

            // WAP Setting
            case WAP_CONNPROFILE_IO_HANDLER:
            case WAP_HOMEPAGE_IO_HANDLER:
            case WAP_PROXY_IO_HANDLER:
            case WAP_PORT_IO_HANDLER:
            case WAP_USERNAME_IO_HANDLER:
            case WAP_PASSWORD_IO_HANDLER:
                writeWapParam(_handlerType, str);
                break;

            // MMS Setting
            case MMS_CONNPROFILE_IO_HANDLER:
            case MMS_MMSC_IO_HANDLER:
                writeMMSParam(_handlerType, str);
                break;

            // PIM Setting
            case PIM_CONNPROFILE_URI_IO_HANDLER:
            case PIM_SERVER_ADDR_IO_HANDLER:
            case PIM_ADDRESS_BOOK_URI_IO_HANDLER:
            case PIM_CALENDAR_URI_IO_HANDLER:
                writePIMParam(_handlerType, str);
                break;

            // PushMail Setting
            case MAIL_CONNPROFILE_IO_HANDER:
            case MAIL_SEND_SERVER_IO_HANDER:
            case MAIL_SEND_PORT_IO_HANDER:
            case MAIL_SEND_USE_SEC_CON_IO_HANDER:
            case MAIL_RECV_SERVER_IO_HANDER:
            case MAIL_RECV_PORT_IO_HANDER:
            case MAIL_RECV_USE_SEC_CON_IO_HANDER:
            case MAIL_RECV_PROTOCAL_IO_HANDER:
                writePushMailParam(_handlerType, str);
                break;

            // Streaming Setting
            case STREAMING_CONNPROFILE_IO_HANDLER:
            case STREAMING_NAME_IO_HANDLER:
            case STREAMING_MAX_UDP_PORT_IO_HANDLER:
            case STREAMING_MIN_UDP_PORT_IO_HANDLER:
            case STREAMING_NET_INFO_IO_HANDLER:
                writeStreamingParam(_handlerType, str);
                break;

            // AGPS Setting
            case AGPS_CONNPROFILE_IO_HANDLER:
            case AGPS_SERVER_IO_HANDLER:
            case AGPS_SERVER_NAME_IO_HANDLER:
            case AGPS_IAPID_IO_HANDLER:
            case AGPS_PORT_IO_HANDLER:
            case AGPS_PROVIDER_ID_IO_HANDLER:
            case AGPS_PREFCONREF_ID_IO_HANDLER:
            case AGPS_CONREF_IO_HANDLER:
                writeAGPSParam(_handlerType, str);
                break;

            default:
                break;
        }

    }

    private int readData(String str, int offset, byte[] data) {
        int ret = 0;
        if (str == null) {
            Log.d(TAG, "readData: str is null!");
            return ret;
        }
        ret = str.length();
        if (data == null) {
            Log.d(TAG, "readData: data is null!");
            return ret;
        }
        
        Log.d(TAG, "str = " + str + " : offset = " + offset + " : datalen" + data.length);
        ByteBuffer buf = ByteBuffer.wrap(data);
        buf.put(str.getBytes());
        return ret;
    }

    // read DM param
    // private String readDMParam(HandlerType type)
    private String readDMParam(int type) {
        String str = null;

        switch (type) {
            case DM_CONN_PROFILE_IO_HANDLER:
               if (null != DmService.getInstance().getSavedAPN()){
                   str = DmService.getInstance().getSavedAPN();
                   Log.d(TAG, "readDMParam  DM_CONN_PROFILE_IO_HANDLER value = " + str);
               }
               else {
                   str = DmService.getInstance().getAPN();
               }
                break;
            case DM_APN_IO_HANDLER:
               if (null != DmService.getInstance().getSavedAPN()){
                   str = DmService.getInstance().getSavedAPN();
                   Log.d(TAG, "readDMParam  DM_APN_IO_HANDLER value = " + str);
               }
               else {
                   str = DmService.getInstance().getAPN();
               }
                break;
            case DM_PROXY_IO_HANDLER:
               if (null != DmService.getInstance().getSavedProxy()){
                   str = DmService.getInstance().getSavedProxy();
                   Log.d(TAG, "readDMParam  DM_PROXY_IO_HANDLER value = " + str);
               }
               else {
			   	str = DmService.getInstance().getProxy(_context);
               	}
                break;
            case DM_PORT_IO_HANDLER:
               if (null != DmService.getInstance().getSavedProxyPort()){
                   str = DmService.getInstance().getSavedProxyPort();
                   Log.d(TAG, "readDMParam  DM_PORT_IO_HANDLER value = " + str);
               }
               else {
                str = DmService.getInstance().getProxyPort(_context);
               	}
                break;
            default:
                break;
        }
        Log.d(TAG, "readDMParam: type = " + type + ", value = " + str);
        return str;
    }

    //read DM WAP param
    //private String readDMParam(HandlerType type)
    private String readDMWAPParam(int type)
    {
        String str = null;
        final String selection = "(name = 'CMCCWAP DM') and numeric=\""
                + android.os.SystemProperties.get(
                        TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, DmService.getInstance().getCurrentPhoneID()), "")
                + "\"";
            Log.d(TAG, "readGprsCmwapParam     selection:  = " + selection);
        Cursor cursor = _context.getContentResolver().query(
                (DmService.getInstance().getCurrentPhoneID()==0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(DmService.getInstance().getCurrentPhoneID(),null),
                null, selection, null, null);

        if (cursor != null) {
            if (cursor.getCount() > 0 && cursor.moveToFirst()) {
                switch (type) {
                    case DM_WAP_APN_IO_HANDLER:
                    case DM_WAP_CONN_PROFILE_IO_HANDLER:
				str = "cmwap";
                       // str = cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.APN));
                        break;
                    case DM_WAP_PROXY_IO_HANDLER:
				str =  "10.0.0.172";
                        //str = cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.PROXY));
                        break;
                    case DM_WAP_PORT_IO_HANDLER:
				str = "80";
                        //str = cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.PORT));
                        break;      
                    default:
                        break;
                }        
            }
            cursor.close();
        }
        Log.d(TAG, "readGprsCmwapParam: type = " + type + ", value = " + str);
        return str;
    }  
	
    // read GPRS-CMNET param
    // xuguoqing modify, 2011-02-15
    // private String readGprsCmnetParam(HandlerType type)
    private String readGprsCmnetParam(int type) {
        String str = null;
        final String selection = "(name = 'CMCCNET' or name='CMNET' or name='CMCCNET_USIM') and numeric=\""
                + android.os.SystemProperties.get(
                        TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, DmService.getInstance().getCurrentPhoneID()), "") + "\"";
        Cursor cursor = _context.getContentResolver().query(
         (DmService.getInstance().getCurrentPhoneID()==0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(DmService.getInstance().getCurrentPhoneID(),null), null,
                selection, null, null);

        if (cursor != null) {
            if (cursor.getCount() > 0 && cursor.moveToFirst()) {
                switch (type) {
                    case GPRS_CMNET_APN_IO_HANDLER:
                        str = cursor
                                .getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.APN));
                        break;
                    case GPRS_CMNET_PROXY_IO_HANDLER:
                        str = cursor.getString(cursor
                                .getColumnIndexOrThrow(Telephony.Carriers.PROXY));
                        break;
                    case GPRS_CMNET_PORT_IO_HANDLER:
                        str = cursor.getString(cursor
                                .getColumnIndexOrThrow(Telephony.Carriers.PORT));
                        break;
                    case GPRS_CMNET_USERNAME_IO_HANDLER:
                        str = cursor.getString(cursor
                                .getColumnIndexOrThrow(Telephony.Carriers.USER));
                        break;
                    case GPRS_CMNET_PASSWORD_IO_HANDLER:
                        str = cursor.getString(cursor
                                .getColumnIndexOrThrow(Telephony.Carriers.PASSWORD));
                        break;
                    default:
                        break;
                }
            }
            cursor.close();
        }
        Log.d(TAG, "readGprsCmnetParam: type = " + type + ", value = " + str);
        return str;
    }

    // read GPRS-CMWAP param
    // xuguoqing modify, 2011-02-15
    // private String readGprsCmwapParam(HandlerType type)
    private String readGprsCmwapParam(int type) {
        String str = null;
        final String selection = "(name = 'CMCCWAP' or name='CMWAP' or name='CMCCWAP_USIM') and numeric=\""
                + android.os.SystemProperties.get(
                        TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, DmService.getInstance().getCurrentPhoneID()), "") + "\"";
        Log.d(TAG, "readGprsCmwapParam     selection:  = " + selection);
        Cursor cursor = _context.getContentResolver().query(
		 (DmService.getInstance().getCurrentPhoneID()==0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(DmService.getInstance().getCurrentPhoneID(),null), null,
                selection, null, null);

        if (cursor != null) {
            if (cursor.getCount() > 0 && cursor.moveToFirst()) {
                switch (type) {
                    case GPRS_CMWAP_APN_IO_HANDLER:
                        str = cursor
                                .getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.APN));
                        break;
                    case GPRS_CMWAP_PROXY_IO_HANDLER:
                        str = cursor.getString(cursor
                                .getColumnIndexOrThrow(Telephony.Carriers.PROXY));
                        break;
                    case GPRS_CMWAP_PORT_IO_HANDLER:
                        str = cursor.getString(cursor
                                .getColumnIndexOrThrow(Telephony.Carriers.PORT));
                        break;
                    case GPRS_CMWAP_USERNAME_IO_HANDLER:
                        str = cursor.getString(cursor
                                .getColumnIndexOrThrow(Telephony.Carriers.USER));
                        break;
                    case GPRS_CMWAP_PASSWORD_IO_HANDLER:
                        str = cursor.getString(cursor
                                .getColumnIndexOrThrow(Telephony.Carriers.PASSWORD));
                        break;
                    default:
                        break;
                }
            }
            cursor.close();
        }
        Log.d(TAG, "readGprsCmwapParam: type = " + type + ", value = " + str);
        return str;
    }

    // read WAP param
	 private String readWapParam(int type)
		{
			String str = null;
	 ContentResolver resolver;
	 resolver = _context.getContentResolver();			
	 //not support @hong 2011-9-22
			switch (type)
			{
			case WAP_CONNPROFILE_IO_HANDLER:
	// ------------------ begin 11:02a 2/14/2011 added by zhh ------------------
	// for DM
				//add by wangxiaobin for CR142408 11-22 begin
				String re;
				ContentResolver cr = _context.getContentResolver();
				Uri uri = Uri.parse("content://browser/apn_setting");
				Cursor cursor = cr.query(uri, new String[]{"name"}, "_id=?", new String[]{"1"}, null);
				if(cursor.moveToNext()){
					re = cursor.getString(0);
					
					String where = "numeric=?" + " and name=?";
					
					Cursor c = cr.query(
						 (DmService.getInstance().getCurrentPhoneID()==0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(DmService.getInstance().getCurrentPhoneID(),null), new String[] {
							"apn"}, where,new String[]{android.os.SystemProperties.get(
							        TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, DmService.getInstance().getCurrentPhoneID()), ""),re},
							Telephony.Carriers.DEFAULT_SORT_ORDER);
					if(c.moveToNext()){
						str = c.getString(0);
						Log.d(TAG, "get wap param="+str);
					}
					c.close();
				}
				cursor.close();
				
				
	//			  str = android.provider.Settings.System.getString(_context.getContentResolver(), 
	//								"browser_datalink_type");
			  //add by wangxiaobin for CR142408 11-22 begin
	// ------------------ end	11:02a 2/14/2011 added by zhh ------------------
				break;
			case WAP_HOMEPAGE_IO_HANDLER:
				str =  Settings.System.getString(resolver, "CMCC_HOMEPAGE");
				if (str == null)
				{
			            str = "http://wap.monternet.com";
			            Settings.System.putString(resolver, "CMCC_HOMEPAGE", str);
				}
				break;
			case BROWSER_HOMEPAGE_IO_HANDLER:			    
				str =  Settings.System.getString(resolver, "BROWSER_HOMEPAGE");				
				if (str == null)
				{
			            str = "http://wap.monternet.com";
			            Settings.System.putString(resolver, "BROWSER_HOMEPAGE", str);
				}
				break;
				
			case WAP_PROXY_IO_HANDLER:
				break;
			case WAP_PORT_IO_HANDLER:
				break;
			case WAP_USERNAME_IO_HANDLER:
				break;		  
			case WAP_PASSWORD_IO_HANDLER:
				break;
			default:
				break;
			}		 
			Log.d(TAG, "readWapParam: type = " + type + ", value = " + str);
			return str;
		}

    // read MMS param
    // private String readMMSParam(HandlerType type)
    private String readMMSParam(int type) {
        String str = null;

        switch (type) {
            case MMS_CONNPROFILE_IO_HANDLER:
                str = "CMCC WAP MMS";
                break;
            case MMS_MMSC_IO_HANDLER: {
                String numric = android.os.SystemProperties.get(
                        TelephonyManager.getProperty(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, DmService.getInstance().getCurrentPhoneID()), "");
                final String selection = "(name='CMCCWAP' or name='CMWAP' or name='CMCCMMS_USIM') and numeric="
                        + numric;
                Log.d(TAG, "readMMSParam selection:" + selection);
                Cursor cursor = _context.getContentResolver().query(
			 (DmService.getInstance().getCurrentPhoneID()==0)?Telephony.Carriers.CONTENT_URI:Telephony.Carriers.getContentUri(DmService.getInstance().getCurrentPhoneID(),null),
                        null, selection, null, null);

                if (cursor != null) {
                    if (cursor.getCount() > 0 && cursor.moveToFirst()) {
                        str = cursor.getString(cursor
                                .getColumnIndexOrThrow(Telephony.Carriers.MMSC));
                    }
                    cursor.close();
                }
            }
                break;
            default:
                break;
        }
        Log.d(TAG, "readMMSParam: type = " + type + ", value = " + str);
        return str;
    }

    // read PIM param
    // private String readPIMParam(HandlerType type)
    private String readPIMParam(int type) {
        String str = null;
	 ContentResolver resolver;
	 resolver = _context.getContentResolver();
        switch (type)
        {
        case DM_SAVE_CRED:
            str = Settings.System.getString(resolver, "CREDNONCE");
		break;
        case PIM_CONNPROFILE_URI_IO_HANDLER:
            //str = "cmwap";
            str = Settings.System.getString(resolver, "PIM_APN");
            break;
        case PIM_SERVER_ADDR_IO_HANDLER:
        //    str = android.provider.Settings.System.getString(_context.getContentResolver(), "android.intent.action.SERVER_ADDRESS");
/*
            SharedPreferences sharedPreferences = _context.getSharedPreferences(PIM_CONFIG, Context.MODE_WORLD_READABLE);
		str = sharedPreferences.getString("pim server addr", "unkown server addr");
*/
		str =  Settings.System.getString(resolver, "PIM_SERVER");
            break;
        case PIM_ADDRESS_BOOK_URI_IO_HANDLER:
         //   str = "./contact";
		str =  Settings.System.getString(resolver, "PIM_CONTACT");
         
            break;
        case PIM_CALENDAR_URI_IO_HANDLER:
		str =  Settings.System.getString(resolver, "PIM_CALENDAR");
            break;
        default:
            break;
        }        
        Log.d(TAG, "readPIMParam: type = " + type + ", value = " + str);
        return str;
    }

    // read PushMail param
    // private String readPushMailParam(HandlerType type)
    private String readPushMailParam(int type) {
        String str = null;
        /* Start of zhuzhongwei 2011.2.14 */
        /*
         * //@hong 2011-9-22 String sendHost = "218.200.243.234"; String
         * sendPort = "18025"; String recvHost = "218.200.243.234"; String
         * recvPort = "18110";
         */
        /* End of zhuzhongwei 2011.2.14 */
        /*
         * //@hong 2011-9-22 switch (type) { case MAIL_CONNPROFILE_IO_HANDER:
         * return "CMWap"; case MAIL_SEND_SERVER_IO_HANDER: str = sendHost;
         * break; case MAIL_SEND_PORT_IO_HANDER: str = sendPort; break; case
         * MAIL_SEND_USE_SEC_CON_IO_HANDER: return "false"; case
         * MAIL_RECV_SERVER_IO_HANDER: str = recvHost; break; case
         * MAIL_RECV_PORT_IO_HANDER: str = recvPort; break; case
         * MAIL_RECV_USE_SEC_CON_IO_HANDER: return "false"; case
         * MAIL_RECV_PROTOCAL_IO_HANDER: return "CMPOP3"; default: break; }
         * final String[] CONTENT_PROJECTION = new String[] { "_id", "protocol",
         * "address", "port", "flags" }; final Uri CONTENT_URI =
         * Uri.parse("content://com.android.pushmail.provider/hostauth"); Cursor
         * cursor = _context.getContentResolver().query(CONTENT_URI,
         * CONTENT_PROJECTION, null, null, null); if (cursor == null) { return
         * str; } try { cursor.moveToFirst(); while (!cursor.isAfterLast()) {
         * int flags = cursor.getInt(4); int FLAG_SSL = 1; String protocol =
         * cursor.getString(1); boolean isEncrypt = (FLAG_SSL == (flags &
         * FLAG_SSL)); if (!isEncrypt) { if (protocol.equalsIgnoreCase("pop3"))
         * { if (type == MAIL_RECV_SERVER_IO_HANDER) { str =
         * cursor.getString(2); break; } else if (type ==
         * MAIL_RECV_PORT_IO_HANDER) { str = cursor.getString(3); break; } }
         * else if (protocol.equalsIgnoreCase("smtp")) { if (type ==
         * MAIL_SEND_SERVER_IO_HANDER) { str = cursor.getString(2); break; }
         * else if (type == MAIL_SEND_PORT_IO_HANDER) { str =
         * cursor.getString(3); break; } } } else { continue; }
         * cursor.moveToNext(); } } finally { cursor.close(); }
         */
        /* End of zhuzhongwei 2011.2.14 */
        // Log.d(TAG, "readPushMailParam: type = " + type + ", value = " + str);
        return str;
    }

    // read Streaming param
    // private String readStreamingParam(HandlerType type)
    private String readStreamingParam(int type) {
        String str = null;
        //final Uri CONTENT_URI = Uri.parse("content://com.android.camera/movieview");
		final Uri CONTENT_URI = Uri.parse("content://com.sprd.gallery3d/movieview");//Bug326289

		
        final String[] CONTENT_PROJECTION = new String[] {
                "name", "max_port", "min_port", "net_info", "conn_prof"
        };

        switch (type) {
            case STREAMING_CONNPROFILE_IO_HANDLER:
                /*
                 * str =android.provider.Settings.System.getString(_context.
                 * getContentResolver(), "apn"); if (TextUtils.isEmpty(str)) {
                 * str = "cmwap"; } else { str = str.toLowerCase(); }
                 */
                str = "cmwap";
                break;
            case STREAMING_NAME_IO_HANDLER:
                str = "Streaming";
                break;
            case STREAMING_MAX_UDP_PORT_IO_HANDLER:
                /*
                 * str =android.provider.Settings.System.getString(_context.
                 * getContentResolver(), "streaming_max_udp_port"); if
                 * (TextUtils.isEmpty(str))
                 */
            {
                str = "65535";
            }
                break;
            case STREAMING_MIN_UDP_PORT_IO_HANDLER:
                /*
                 * str =android.provider.Settings.System.getString(_context.
                 * getContentResolver(), "streaming_min_udp_port"); if
                 * (TextUtils.isEmpty(str))
                 */
            {
                str = "8192";
            }
                break;
            case STREAMING_NET_INFO_IO_HANDLER:
                str = "EGPRS,10,100";
                break;
            default:
                break;
        }
        Cursor cursor = _context.getContentResolver().query(CONTENT_URI, CONTENT_PROJECTION, null,
                null, null);
        if (cursor == null) {
            Log.d(TAG, "readStreamingParam cursor read nothing!");
            return str;
        }
        try {
            int colidx;
            cursor.moveToFirst();
            Log.d(TAG, "readStreamingParam cursor moveToFirst!");

            switch (type) {
                case STREAMING_CONNPROFILE_IO_HANDLER:
                    colidx = cursor.getColumnIndex("conn_prof");
                    str = cursor.getString(colidx);
                    break;
                case STREAMING_NAME_IO_HANDLER:
                    colidx = cursor.getColumnIndex("name");
                    str = cursor.getString(colidx);

                    break;
                case STREAMING_MAX_UDP_PORT_IO_HANDLER:
                    colidx = cursor.getColumnIndex("max_port");
                    str = cursor.getString(colidx);

                    break;
                case STREAMING_MIN_UDP_PORT_IO_HANDLER:
                    colidx = cursor.getColumnIndex("min_port");
                    str = cursor.getString(colidx);

                    break;
                case STREAMING_NET_INFO_IO_HANDLER:
                    colidx = cursor.getColumnIndex("net_info");
                    str = cursor.getString(colidx);

                    break;
                default:
                    break;
            }
        } finally {
            cursor.close();
        }

        Log.d(TAG, "readStreamingParam: type = " + type + ", value = " + str);
        return str;
    }

    // read AGPS param
    // private String readAGPSParam(HandlerType type)
    private String readAGPSParam(int type) {
        String str = null;
        // oasis_zp@hisense add readAgpsSettingInfo and parse it
        /*
         * LocationManager mLocationManager; mLocationManager =
         * (LocationManager)
         * _context.getSystemService(Context.LOCATION_SERVICE); Bundle data =
         * mLocationManager.readAgpsSettingInfo(); switch (type) { case
         * AGPS_CONNPROFILE_IO_HANDLER: str = data.getString("apn"); break; case
         * AGPS_SERVER_IO_HANDLER: str = data.getString("address"); break; case
         * AGPS_SERVER_NAME_IO_HANDLER: str = data.getString("name"); break;
         * case AGPS_IAPID_IO_HANDLER: str = data.getString("iapid"); break;
         * case AGPS_PORT_IO_HANDLER: str = data.getString("port"); break; case
         * AGPS_PROVIDER_ID_IO_HANDLER: str = data.getString("providerid");
         * break; default: break; } Log.d(TAG, "readAGPSParam: type = " + type +
         * ", value = " + str);
         */        
        // bug 292626 begin
        switch (type) {
        case AGPS_CONNPROFILE_IO_HANDLER:
            str = DmService.getInstance().getAGPSApn(_context);
            break;
        case AGPS_SERVER_IO_HANDLER:
        case AGPS_SERVER_NAME_IO_HANDLER:
        case AGPS_IAPID_IO_HANDLER:
        case AGPS_PORT_IO_HANDLER:
        case AGPS_PROVIDER_ID_IO_HANDLER:
        case AGPS_PREFCONREF_ID_IO_HANDLER:
        case AGPS_CONREF_IO_HANDLER:
            str = DmService.getInstance().getAGPSParam(_context, type);
            break;
        default:
            break;
        }
        Log.d(TAG, "readAGPSParam: type = " + type + ", value = " + str);
        // bug 292626 end
        return str;
    }

    // write DM param
    // private void writeDMParam(HandlerType type, String str)
    private void writeDMParam(int type, String str) {
        Vdmc.isDmSetting = true;
        switch (type) {
            case DM_CONN_PROFILE_IO_HANDLER:
		Vdmc.tmpdmwapapn=str;
                break;
            case DM_APN_IO_HANDLER:
                Vdmc.tmpdmwapapn=str;				
                break;
            case DM_PROXY_IO_HANDLER:
		Vdmc.tmpdmpwapproxy=str;
                break;
            case DM_PORT_IO_HANDLER:
		Vdmc.tmpdmpwapport=str;
                break;
            default:
                break;
        }
        Log.d(TAG, "writeDMParam: type = " + type + ", value = " + str);
        return;
    }

    //write DM WAP param
    //private void writeDMParam(HandlerType type, String str)
    private void writeDMWAPParam(int type, String str)
    {
        Vdmc.isDmSetting = true;
        switch (type)
        {
        case DM_WAP_CONN_PROFILE_IO_HANDLER:
            Vdmc.tmpdmwapapn=str;
            break;        
        case DM_WAP_APN_IO_HANDLER:
            Vdmc.tmpdmwapapn=str;
            break;
        case DM_WAP_PROXY_IO_HANDLER:
            Vdmc.tmpdmpwapproxy=str;
            break;
        case DM_WAP_PORT_IO_HANDLER:
            Vdmc.tmpdmpwapport=str;
            break;
        default:
            break;
        }        
        Log.d(TAG, "writeDMWAPParam: type = " + type + ", value = " + str);
        return;
    }   

    // write GPRS-CMNET param
    // xuguoqing modify, 2011-02-15
    // private void writeGprsCmnetParam(HandlerType type, String str)
    private void writeGprsCmnetParam(int type, String str) {
        switch (type) {
            case GPRS_CMNET_APN_IO_HANDLER:
                Vdmc.tmpnetapn = str;
                // values.put(Telephony.Carriers.APN, str);
                break;
            case GPRS_CMNET_PROXY_IO_HANDLER:
                Vdmc.tmpnetproxy = str;
                // values.put(Telephony.Carriers.PROXY, str);
                break;
            case GPRS_CMNET_PORT_IO_HANDLER:
                Vdmc.tmpnetport = str;
                // values.put(Telephony.Carriers.PORT, str);
                break;
            case GPRS_CMNET_USERNAME_IO_HANDLER:
                Vdmc.tmpnetuser = str;
                // values.put(Telephony.Carriers.USER, str);
                break;
            case GPRS_CMNET_PASSWORD_IO_HANDLER:
                Vdmc.tmpnetpwd = str;
                // values.put(Telephony.Carriers.PASSWORD, str);
                break;
            default:
                break;
        }

        Log.d(TAG, "writeGprsCmnetParam: NEW type = " + type + ", value = " + str);
        return;
    }

    // write GPRS-CMWAP param
    // xuguoqing modify, 2011-02-15
    // private void writeGprsCmwapParam(HandlerType type, String str)
    private void writeGprsCmwapParam(int type, String str) {
        switch (type) {
            case GPRS_CMWAP_APN_IO_HANDLER:
                Vdmc.tmpwapapn = str;
                // values.put(Telephony.Carriers.APN, str);
                break;
            case GPRS_CMWAP_PROXY_IO_HANDLER:
                Vdmc.tmpwapproxy = str;

                // values.put(Telephony.Carriers.PROXY, str);
                break;
            case GPRS_CMWAP_PORT_IO_HANDLER:
                Vdmc.tmpwapport = str;
                // values.put(Telephony.Carriers.PORT, str);
                break;
            case GPRS_CMWAP_USERNAME_IO_HANDLER:
                Vdmc.tmpwapuser = str;
                // values.put(Telephony.Carriers.USER, str);
                break;
            case GPRS_CMWAP_PASSWORD_IO_HANDLER:
                Vdmc.tmpwappwd = str;
                // values.put(Telephony.Carriers.PASSWORD, str);
                break;
            default:
                break;
        }

        Log.d(TAG, "writeGprsCmwapParam: NEW type = " + type + ", value = " + str);
        return;
    }


		//write WAP param
	private void writeWapParam(int type, String str)
	//private void writeWapParam(HandlerType type, String str)
	{
 //@hong 2011-9-22
  	 ContentResolver resolver;
	 resolver = _context.getContentResolver();
		switch (type)
		{
			case WAP_CONNPROFILE_IO_HANDLER:
	// ------------------ begin 11:09a 2/14/2011 added by zhh ------------------
	// for DM
				//add by wangxiaobin for CR142408 11-22 begin
//				  Intent intent = new Intent("android.intent.actioin.BROWSER_WRITE_SETTINGS");
//				  intent.putExtra("linktype", str);
//				  _context.sendBroadcast(intent);
				Log.d(TAG, "writewapParam str="+str);
//				  ContentResolver cr = _context.getContentResolver();
//				  Uri uri = Uri.parse("content://browser/apn_setting");
//				  ContentValues values = new ContentValues();
//				  if("CMNET".equals(str.toUpperCase())){
//					  values.put("name", "CMCCNET");
//				  }else{
//					  values.put("name", "CMCCWAP");
//				  }
//				  cr.update(uri, values, "_id=?", new String[]{"1"});
/*
				ContentResolver cr = _context.getContentResolver();
				Uri uri = Uri.parse("content://browser/apn_setting");
				Cursor cursor = cr.query(uri, new String[]{"name"}, "_id=?", new String[]{"1"}, null);
				if(cursor.moveToNext()){
					String name = cursor.getString(0);
					Bundle bundle = new Bundle();
					bundle.putString("name", name);
					bundle.putString("apn",str);
					Message msg = apnhandler.obtainMessage(1,bundle);
					apnhandler.sendMessageDelayed(msg, 10000);
				}
				cursor.close();
*/
				Vdmc.tmpbrowserapn = str;
				//add by wangxiaobin for CR142408 11-22 end
	// ------------------ end	11:09a 2/14/2011 added by zhh ------------------
				break;
			case WAP_HOMEPAGE_IO_HANDLER:
			        if (null == str)
			        {
			            String sApn = "http://wap.monternet.com";
			            Settings.System.putString(resolver, "CMCC_HOMEPAGE", sApn);
			        }
				else
				 Settings.System.putString(resolver, "CMCC_HOMEPAGE", str);
				{//for compatible with old version dm
			            Intent i= new Intent("com.spreadtrum.dm.waphomepage");
			            i.putExtra("homepage",str);
			            _context.sendBroadcast(i);
				}
				break;
			case  BROWSER_HOMEPAGE_IO_HANDLER:			     
				 Settings.System.putString(resolver, "BROWSER_HOMEPAGE", str);
				 { 
				 //
			            Intent i= new Intent("com.spreadtrum.dm.waphomepage");
			            i.putExtra("homepage",str);
			            _context.sendBroadcast(i);
				 }
				break;
				
			case WAP_PROXY_IO_HANDLER:
				break;
			case WAP_PORT_IO_HANDLER:
				break;
			case WAP_USERNAME_IO_HANDLER:
				break;		  
			case WAP_PASSWORD_IO_HANDLER:
				break;
			default:
				break;
		}		 

		Log.d(TAG, "writeWapParam: NEW type = " + type + ", value = " + str);

		return;
	}



    // write MMS param
    // private void writeMMSParam(HandlerType type, String str)
    private void writeMMSParam(int type, String str) {
        switch (type) {
            case MMS_CONNPROFILE_IO_HANDLER:
                break;
            case MMS_MMSC_IO_HANDLER: {
                Vdmc.tmpmmsmmsc = str;
                Log.d(TAG, "writeMMSParam Vdmc.tmpmmsmmsc: " + Vdmc.tmpmmsmmsc);

            }
                break;
            default:
                break;
        }
        Log.d(TAG, "writeMMSParam: type = " + type + ", value = " + str);
        return;
    }

    // write PIM param
    // private void writePIMParam(HandlerType type, String str)
    private void writePIMParam(int type, String str) {
 //@hong
 	 ContentResolver resolver;
	 resolver = _context.getContentResolver();
        switch (type)
        {
        case DM_SAVE_CRED:
	        if (null == str)
	        {
	            String sApn = "Jjg5TjE8KSgtJiBLW0hTQQ==";
	            Settings.System.putString(resolver, "CREDNONCE", sApn);
	        }
		else
	            Settings.System.putString(resolver, "CREDNONCE", str);
		break;
        case PIM_CONNPROFILE_URI_IO_HANDLER:
	        if (null == str)
	        {
	            String sApn = "cmwap";
	            Settings.System.putString(resolver, "PIM_APN", sApn);
	        }
		else
	            Settings.System.putString(resolver, "PIM_APN", str);
		
            break;
        case PIM_SERVER_ADDR_IO_HANDLER:
        {
		/*	
            Intent in = new Intent("android.intent.action.DM_SETTING_PIM_PARAMETER");
            in.putExtra("android.intent.action.SERVER_ADDRESS", str);
            _context.sendBroadcast(in);
            */
/*            
        SharedPreferences sharedPreferences = _context.getSharedPreferences(PIM_CONFIG, Context.MODE_WORLD_READABLE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putString("pim server addr", str);
        editor.commit();   
*/
	        if (null == str)
	        {
	            String sServer = "http://218.206.176.241:8888/sync";
	            Settings.System.putString(resolver, "PIM_SERVER", sServer);
	        }else
	            Settings.System.putString(resolver, "PIM_SERVER", str);
	        
        		
            break;
        }            
        case PIM_ADDRESS_BOOK_URI_IO_HANDLER:
	        if (null == str)
	        {
	            String sContact = "content://contacts/people";
	            Settings.System.putString(resolver, "PIM_CONTACT", sContact);
	        }else
	            Settings.System.putString(resolver, "PIM_CONTACT", str);
        
            break;
        case PIM_CALENDAR_URI_IO_HANDLER:
	        if (null == str)
	        {
	            String sCalendar = "content://calendar/events";
	            Settings.System.putString(resolver, "PIM_CALENDAR", sCalendar);
	        }else
	            Settings.System.putString(resolver, "PIM_CALENDAR", str);
	        
            break;
        default:
            break;
        }        
        Log.d(TAG, "readPIMParam: type = " + type + ", value = " + str);
        return;
    }

    // write PushMail param
    // private void writePushMailParam(HandlerType type, String str)
    private void writePushMailParam(int type, String str) {
        /*
         * //@hong 2011-9-22 int MAIL_CONNPROFILE_IO_HANDER_TYPE = 1; int
         * MAIL_SEND_SERVER_IO_HANDER_TYPE = 2; int
         * MAIL_SEND_PORT_IO_HANDER_TYPE = 3; int
         * MAIL_SEND_USE_SEC_CON_IO_HANDER_TYPE = 4; int
         * MAIL_RECV_SERVER_IO_HANDER_TYPE = 5; int
         * MAIL_RECV_PORT_IO_HANDER_TYPE = 6; int
         * MAIL_RECV_USE_SEC_CON_IO_HANDER_TYPE = 7; int
         * MAIL_RECV_PROTOCAL_IO_HANDER_TYPE = 8; int extraType = 0; switch
         * (type) { case MAIL_CONNPROFILE_IO_HANDER: extraType =
         * MAIL_CONNPROFILE_IO_HANDER_TYPE; break; case
         * MAIL_SEND_SERVER_IO_HANDER: extraType =
         * MAIL_SEND_SERVER_IO_HANDER_TYPE; break; case
         * MAIL_SEND_PORT_IO_HANDER: extraType = MAIL_SEND_PORT_IO_HANDER_TYPE;
         * break; case MAIL_SEND_USE_SEC_CON_IO_HANDER: extraType =
         * MAIL_SEND_USE_SEC_CON_IO_HANDER_TYPE; break; case
         * MAIL_RECV_SERVER_IO_HANDER: extraType =
         * MAIL_RECV_SERVER_IO_HANDER_TYPE; break; case
         * MAIL_RECV_PORT_IO_HANDER: extraType = MAIL_RECV_PORT_IO_HANDER_TYPE;
         * break; case MAIL_RECV_USE_SEC_CON_IO_HANDER: extraType =
         * MAIL_RECV_USE_SEC_CON_IO_HANDER_TYPE; break; case
         * MAIL_RECV_PROTOCAL_IO_HANDER: extraType =
         * MAIL_RECV_PROTOCAL_IO_HANDER_TYPE; break; default: break; }
         */
        /*
         * Start of zhuzhongwei 2011.2.14 Intent i = new
         * Intent("pushmail.action.WRITE_SETTINGS"); i.putExtra("type",
         * extraType); i.putExtra("value", str); _context.sendBroadcast(i); End
         * of zhuzhongwei 2011.2.14
         */
        // Log.d(TAG, "writePushMailParam: type = " + type + ", value = " +
        // str);

        return;
    }

    // write Streaming param
    // private void writeStreamingParam(HandlerType type, String str)
    private void writeStreamingParam(int type, String str) {
        int extraType = 0;
        String strtype = "";
	final Uri CONTENT_URI = Uri.parse("content://com.sprd.gallery3d/movieview");//Bug326289
		
        switch (type) {
            case STREAMING_CONNPROFILE_IO_HANDLER:
//                extraType = STREAMING_CONNPROFILE_IO_HANDLER_TYPE;
                strtype = "conn_prof";
                Vdmc.tmpstreamapn = str;
                // str = str.toUpperCase();
                break;
            case STREAMING_NAME_IO_HANDLER:
//                extraType = STREAMING_NAME_IO_HANDLER_TYPE;
                strtype = "name";
                break;
            case STREAMING_MAX_UDP_PORT_IO_HANDLER:
//                extraType = STREAMING_MAX_UDP_PORT_IO_HANDLER_TYPE;
                strtype = "max_port";
                break;
            case STREAMING_MIN_UDP_PORT_IO_HANDLER:
//                extraType = STREAMING_MIN_UDP_PORT_IO_HANDLER_TYPE;
                strtype = "min_port";
                break;
            case STREAMING_NET_INFO_IO_HANDLER:
//                extraType = STREAMING_NET_INFO_IO_HANDLER_TYPE;
                strtype = "net_info";
                break;
            default:
                break;
        }
        /* Start of zhuzhongwei 2011.2.14 */

        // if (extraType != STREAMING_NAME_IO_HANDLER) //name should not be
        // changed.
        if (type != STREAMING_CONNPROFILE_IO_HANDLER)
        {
         Log.d(TAG, "DM Broadcast streaming info entering....");
            Intent i= new Intent("com.android.dm.vdmc");
            i.putExtra("type", strtype);
            i.putExtra("value", str);
            _context.sendBroadcast(i);
            Log.d(TAG, "DM Broadcast streaming info");
//	    Vdmc.getInstance().registerVDMobserver();
        }
        // write direct to the database.
        
/*          ContentValues values = new ContentValues(); 
	   values.put(strtype, str);
          _context.getContentResolver().update(CONTENT_URI,values,null,null);
    */     
        /* End of zhuzhongwei 2011.2.14 */
        Log.d(TAG, "writeStreamingParam: type = " + strtype + ", value = " + str);
        return;
    }

    // write AGPS param
    // private void writeAGPSParam(HandlerType type, String str)
    private void writeAGPSParam(int type, String str) {
        // oasis_zp@hisense add syncDeviceManagementInfo to store settings
        /*
         * LocationManager mLocationManager; mLocationManager =
         * (LocationManager)
         * _context.getSystemService(Context.LOCATION_SERVICE); Bundle data =
         * new Bundle(); switch (type) { case AGPS_CONNPROFILE_IO_HANDLER:
         * data.putString("apn", str); break; case AGPS_SERVER_IO_HANDLER:
         * data.putString("address", str); break; case
         * AGPS_SERVER_NAME_IO_HANDLER: data.putString("name", str); break; case
         * AGPS_IAPID_IO_HANDLER: data.putString("iapid", str); break; case
         * AGPS_PORT_IO_HANDLER: data.putString("port", str); break; case
         * AGPS_PROVIDER_ID_IO_HANDLER: data.putString("providerid", str);
         * break; default: break; }
         * mLocationManager.syncDeviceManagementInfo(data); Log.d(TAG,
         * "writeAGPSParam: type = " + type + ", value = " + str);
         */
        // bug 292626 begin
        switch (type) {
        case AGPS_CONNPROFILE_IO_HANDLER:
            Vdmc.tmpCmsuplApn = str;

            break;
        case AGPS_SERVER_IO_HANDLER:
        case AGPS_SERVER_NAME_IO_HANDLER:
        case AGPS_IAPID_IO_HANDLER:
        case AGPS_PORT_IO_HANDLER:
        case AGPS_PROVIDER_ID_IO_HANDLER:
        case AGPS_PREFCONREF_ID_IO_HANDLER:
        case AGPS_CONREF_IO_HANDLER:
            DmService.getInstance().setAGPSParam(_context, str, type);
            break;
        default:
            break;
        }
        Log.d(TAG, "writeAGPSParam: type = " + type + ", value = " + str);
        // bug 292626 end
        return;
    }

    private static Context _context;

    // private HandlerType _handlerType;
    private int _handlerType;
}
