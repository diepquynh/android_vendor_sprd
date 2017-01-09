#include "common.h"
#include "netlink.h"

unsigned int fsm_init = FSM_IDLE;
struct ss_cmd ss_cmd_pri;
struct nl80211_global g_nl80211;
unsigned int g_channelist[15] = { 0,
	2412,2417,2422,2427,2432,
	2437,2442,2447,2452,2457,
	2462,2467,2472,2484
};
char *ss_cmd_name[CMD_ID_MAX] = {
	"OpenDUT",
	"CloseDUT",
	"SetChannel",
	"SetDataRate",
	"SetLongPreamble",
	"SetShortGuardInterval",
	"TxGain",
	"SetBurstInterval",
	"SetPayload",
	"SetBand",
	"SetBandWidth",
	"TxStartWithMod",
	"TxStartWoMod",
	"TxStop",
	"RxStart",
	"RxStop",
	"GetGoodFrame",
	"GetErrorFrame",
	"JoinWithSSID",
	"JoinWithSSID2G",
	"JoinWithSSID5G",
	"SetAntenna",
	"GetErrorString",
	"GetRSSI",
	"SetTxCount",
	"StartStaMode",
	"StopStaMode",
	"StartNpiMode",
	"StopNpiMode",
	"TriggerScan",
	"SetCountryCode",
	"GetChannelist",
	"AddInterface",
	"DelInterface",
	"SetScanTime",
	"SetWlanCap",
	"SetLnaOn",
	"SetLnaOff",
	/*samsung cmd end*/
};
char *sprd_cmd_name[CMD_ID_MAX] = {
	"start",
	"stop",
	"set_channel",
	"set_rate",
	"set_preamble",
	"set_guard_interval",
	"set_tx_power",
	"set_burst_interval",
	"set_pkt_length",
	"set_band",
	"set_bandwidth",
	"tx_start",
	"tx_start",
	"tx_stop",
	"rx_start",
	"rx_stop",
	"get_rx_ok",
	"get_rx_ok",
	"set_join_ssid",
	"set_join_ssid",
	"join_with_ssid5g",
	"set_antenna",
	"get_errorstring",
	"get_rssi",
	"set_tx_count",
	"start_sta_mode_null",
	"stop_sta_mode_null",
	"start_npi_mode_null",
	"stop_npi_mode_null",
	"trigger_scan",
	"set_countrycode",
	"get_channelist",
	"add_interface",
	"del_interface",
	"set_scan_time",
	"set_wlan_cap",
	"lna_on",
	"lna_off",
	/*samsung cmd end*/
};
const int rate_table[] = {
	0,/*DATA_RATE_UNSUPPORT, //for index 0*/
	1,/*DATA_RATE_1M,*/
	2,/*DATA_RATE_2M,*/
	5,/*DATA_RATE_5_5M,*/
	6,/*DATA_RATE_6M,*/
	9,/*DATA_RATE_9M,*/
	11,/*DATA_RATE_11M,*/
	12,/*DATA_RATE_12M,*/
	18,/*DATA_RATE_18M,*/
	22,/*DATA_RATE_22M,*/
	24,/*DATA_RATE_24M,*/
	33,/*DATA_RATE_33M,*/
	36,/*DATA_RATE_36M,*/
	48,/*DATA_RATE_48M,*/
	54,/*DATA_RATE_54M,*/
	7,/*DATA_RATE_MCS0,*/
	13,/*DATA_RATE_MCS1,*/
	19,/*DATA_RATE_MCS2,*/
	26,/*DATA_RATE_MCS3,*/
	39,/*DATA_RATE_MCS4,*/
	52,/*DATA_RATE_MCS5,*/
	58,/*DATA_RATE_MCS6,*/
	65,/*DATA_RATE_MCS7,*/
	65,/*DATA_RATE_MCS8,*/
	65,/*DATA_RATE_MCS9,*/
	65,/*DATA_RATE_MCS10,*/
	65,/*DATA_RATE_MCS11,*/
	65,/*DATA_RATE_MCS12,*/
	65,/*DATA_RATE_MCS13,*/
	65,/*DATA_RATE_MCS14,*/
	65/*DATA_RATE_MCS15,*/
};
int fsm_handle(int cmd_id,struct ss_cmd *pri)
{
	int ret = -1;
	SS_FSM *state_ptr = &pri->state;
	SS_FSM state = *state_ptr;

	SSPRINT("FSM INIT:%d\n",state);
	/*clear error buffer*/
	memset(pri->errbuf, '\0', sizeof(pri->errbuf));
	switch (state) {
	case FSM_UNDEFINE:
	{
		if (cmd_id == OpenDUT_ID) {
			ret = StartNpiMode();
			if(!ret)
				*state_ptr = FSM_NPI_MODE;
		}
	}
		break;
	case FSM_NPI_MODE:
	{
		if (cmd_id == CloseDUT_ID) {
			ret = StopNpiMode();
			if (!ret)
				*state_ptr = FSM_UNDEFINE;
		}
		else if (cmd_id == JoinWithSSID_ID || cmd_id == SetCountryCode_ID
					|| cmd_id == GetChannelist_ID || cmd_id == AddInterface_ID
					|| cmd_id == DelInterface_ID || cmd_id == SetScanTime_ID
					|| cmd_id == TriggerScan_ID) {
			ret = StopNpiMode();
			if(ret) {
				SSPRINT("Stop npi mode failed\n");
				goto OUT;
			}
			ret = StartStaMode();
			if(!ret)
				*state_ptr = FSM_STA_MODE;
		}
		else if(cmd_id >= SetChannel_ID && (cmd_id <= SetTxCount_ID)) {
			ret = SS_SUCCESS;
		}
	}
		break;
	case FSM_STA_MODE:
	{
		if(cmd_id == CloseDUT_ID){
			ret = StopStaMode();
			if(!ret)
				*state_ptr = FSM_UNDEFINE;
		}
		else if(cmd_id == JoinWithSSID_ID || cmd_id == SetCountryCode_ID
				|| cmd_id == GetChannelist_ID || cmd_id == AddInterface_ID
				|| cmd_id == DelInterface_ID || cmd_id == SetScanTime_ID
				|| cmd_id == TriggerScan_ID) {
			ret = SS_SUCCESS;
		}
		else if(cmd_id >= SetChannel_ID && (cmd_id <= SetTxCount_ID)){
			ret = StopStaMode();
			if(ret){
				SSPRINT("Stop normal(station) mode failed\n");
				goto OUT;
			}
			ret = StartNpiMode();
			if(!ret)
				*state_ptr = FSM_NPI_MODE;
		}
	}
		break;
	default:
		SSPRINT("FSM undefine\n");
		break;
	}
OUT:
	SSPRINT("FSM state:%d ret:%d\n",*state_ptr ,ret);
	return ret;
}
/**
 ** handle_ss_cmd handle samsung private cmd.
 **
 ** @param argc is the cmd and the params totoal count
 ** @param argv is the cmd param pointer's pointer
 **
 ** @return 0 on success, < 0 on failure.
 **/
int handle_ss_cmd(int cmd_id,int argc, char **argv)
{
	int i = 0;
	int cmd_c = argc + 1; //add a param ->wlan0
	char *cmd_v[cmd_c];
	int ret;

	ret = fsm_handle(cmd_id,&ss_cmd_pri);
	if(ret){
		pr_err("%s,fsm_handle error\n",__func__);
		return ret;
	}

	construct_cmd(cmd_c,cmd_v,sprd_cmd_name[cmd_id],argv);/*param posit skip cmd_id*/
	for(i = 0; i < cmd_c; i++)
	{
		SSPRINT("ATTR[%d]: %s\n",i,cmd_v[i]);
	}

	ret = __handle_ss_cmd(cmd_c, (char **)cmd_v, (void *)&ss_cmd_pri);
	return ret;
}

/**
 ** Set 'Device under Test(DUT)' mode up
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_OpenDUT(void)
{
	int ret = SS_SUCCESS;

	if (fsm_init == FSM_IDLE)
		memset(&ss_cmd_pri, 0, sizeof(ss_cmd_pri));

	if (ss_cmd_pri.open == true) {
		SSPRINT("DUT is already open\n");
		return !ret;
	}

	ret = fsm_handle(OpenDUT_ID,&ss_cmd_pri);
	if (ret) {
		pr_err("%s fsm check failed %d\n", __func__, ret);
		return !ret;
	}
	fsm_init = FSM_WORK;
	ss_cmd_pri.open = true;

#ifdef CONFIG_SAMSUNG_EXT
	usleep(500000);
#endif

	return !ret;
}

/**
 ** Close DUT.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_CloseDUT(void)
{
	int ret;

	ret = fsm_handle(CloseDUT_ID,&ss_cmd_pri);
	if (ret) {
		pr_err("%s fsm check failed %d\n", __func__, ret);
		return !ret;
	}

	ss_cmd_pri.open = false;
	fsm_init = FSM_IDLE;

	return !ret;
}

/**
 ** SetChannel sets the frequency of channel.
 **
 ** @param channel is the channel number which will be set (1 ~ 14).
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_SetChannel(int channel)
{
	int ret;

	char param_str[10];
	char *str = (char *)param_str;

	if((channel > 15) || (channel < 1)){
		pr_err("Channel num shoulde be between 1 to 14\n");
		return RET_ERR;
	}
	sprintf(param_str,"%d",channel);
	ret = handle_ss_cmd(SetChannel_ID, 2,&str);

	return !ret;
}

/**
 ** SetDataRate sets the data rate of 802.11b,g,n mode.
 **
 ** @param rate should be the member of enum 'DataRate'.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_SetDataRate(DataRate rate)
{
	int ret;
	char param_str[10];
	char *str = (char *)param_str;

	if((rate > DATA_RATE_MCS7) || (rate <= DATA_RATE_UNSUPPORT)){
		pr_err("Data rate shoulde be between DATA_RATE_1M to DATA_RATE_MCS7\n");
		return RET_ERR;
	}
	sprintf(param_str,"%d",rate_table[rate]);
	ret = handle_ss_cmd(SetDataRate_ID, 2,&str);
#ifndef CONFIG_SAMSUNG_EXT
	if(!ret){
		ss_cmd_pri.rate = rate;
		if(ss_cmd_pri.rate == DATA_RATE_1M){
			/*The rate of 1M only allowed long preamble*/
			if(!ss_cmd_pri.long_preamble)
				ret = !RFT_SetLongPreamble(1);
		}
		/*Non 1M,2M,5.5M 11M rate only allows a short preamble*/
		else if(!((ss_cmd_pri.rate == DATA_RATE_2M) || (ss_cmd_pri.rate == DATA_RATE_5_5M)
			|| (ss_cmd_pri.rate == DATA_RATE_11M))){
			if(ss_cmd_pri.long_preamble)
				ret = !RFT_SetLongPreamble(0);
		}
	}
#endif
	return !ret;
}

/**
 ** SetLongPreamble() determines the type of preamble.
 **
 ** @param enable should be 1 if long preamble, 0 if short preamble.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_SetLongPreamble(int enable)
{
	int ret;
	char param_str[10];
	char *str = (char *)param_str;

	if((enable != 1) && (enable != 0)){
		pr_err("Param must be 1 or 0\n");
		return RET_ERR;
	}
#ifndef CONFIG_SAMSUNG_EXT
	/*long preamble*/
	if(enable){
		if(!((ss_cmd_pri.rate == DATA_RATE_1M) || (ss_cmd_pri.rate == DATA_RATE_2M) ||
			(ss_cmd_pri.rate == DATA_RATE_5_5M) || (ss_cmd_pri.rate == DATA_RATE_11M))){

			pr_err("Only the data rate of 1M,5.5M,11M allowed to long preamble\n");
			return RET_ERR;
		}
	}
	/*short preambel*/
	else{
		if(ss_cmd_pri.rate == DATA_RATE_1M){
			pr_err("The rate of 1M only allowed long preamble\n");
			return RET_ERR;
		}
	}
#endif
	sprintf(param_str,"%d",enable);
	ret = handle_ss_cmd(SetLongPreamble_ID, 2, &str);
	if(!ret){
		ss_cmd_pri.long_preamble = enable;
	}

	return !ret;
}

/**
 ** SetShortGuardInterval() determines guard interval by 'ns' unit.
 **
 ** @param enable should be 1 if 400ns GI, 0 if 800ns GI.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_SetShortGuardInterval(int enable)
{
	int ret;
	char param_str[10];
	char *str = (char *)param_str;

	if((enable != 1) && (enable != 0)){
		pr_err("Param must be 1 or 0\n");
		return RET_ERR;
	}
#ifndef CONFIG_SAMSUNG_EXT
	if(enable && (ss_cmd_pri.rate < DATA_RATE_MCS0)){
		pr_err("Only in 11N mode allows short guard interval of 400 ns\n");
		return RET_ERR;
	}
#endif
	sprintf(param_str,"%d",enable);
	ret = handle_ss_cmd(SetShortGuardInterval_ID, 2, &str);

	return !ret;
}

/**
 ** TxGain() controls the power level for tx signal.
 **
 ** @param txpwr is tx level (dBm unit).
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_TxGain(int txpwr)
{
	int ret;
	char param_str[10];
	char *str = (char *)param_str;

	sprintf(param_str, "%d", txpwr);
	ret = handle_ss_cmd(TxGain_ID, 2, &str);

	return !ret;
}

/**
 ** SetBurstInterval() sets the burst interval.
 **
 ** @param burstinterval
 **
 ** @return 0 on success or unsupport, < 0 on failure.
 **/
int RFT_SetBurstInterval(int burstinterval)
{
	int ret;
	char param_str[10];
	char *str = (char *)param_str;
	if((burstinterval > 512) || (burstinterval < 1)){
		pr_err("Burstinterval should be between 1 to 512\n");
	}
	if(burstinterval > 512)
		burstinterval = 512;
	if(burstinterval < 1)
        burstinterval = 1;

	sprintf(param_str,"%d",burstinterval);
	ret = handle_ss_cmd(SetBurstInterval_ID, 2, &str);

	return !ret;
}

/**
 ** SetPayload() sets the size of payload.
 ** For TxStart without this option,
 ** 1024 should be set internally as a default value.
 **
 ** @param size means payload length(Bytes).
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_SetPayload(int size)
{
	int ret;
	char param_str[10];
	char *str = (char *)param_str;

	if((size < 26) || (size > 1596)){
		pr_err("Param must between 26 and 1596\n");
		return RET_ERR;
	}
	sprintf(param_str,"%d",size);
	ret = handle_ss_cmd(SetPayload_ID, 2, &str);

	return !ret;
}

/**
 ** SetBand() sets the band with one of 2.4GHz and 5GHz.
 ** For TxStart or RxStart without this option,
 ** 2.4GHz should be set internally as a default value.
 **
 ** @param band should be 1 if 2.4GHz (b/g/n), 2 if 5GHz (a/n).
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_SetBand(int band)
{
	if (band == 1)
		return RET_SUC;
	else{
		pr_err("Only surpport 2.4GHz band\n");
		return RET_ERR;
	}
}


/**
 ** SetBandWidth() sets the bandwidth per channel, one of 20MHz and 40MHz
 ** For TxStart or RxStart without this option,
 ** 20MHz should be set internally as a default value.
 **
 ** @param enable should be 1 if 20MHz BW, 2 if 40MHz BW
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_SetBandWidth(int width)
{
	if (width == 1)
		return RET_SUC;
	else{
		pr_err("Only surpport 20MHz BW\n");
		return RET_ERR;
	}
}

/**
 ** Tx test with modulation begins when TxStartWithMod() is called.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_TxStartWithMod(void)
{
	int ret;

	ret = handle_ss_cmd(TxStartWithMod_ID, 1, NULL);

	return !ret;
}

/**
 ** Tx test without modulation begins when TxStartWoMod() is called.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_TxStartWoMod(void)
{
	int ret;

	ret = handle_ss_cmd(TxStartWoMod_ID, 1, NULL);

	return !ret;
}

/**
 ** TxStop() makes Tx test stop.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_TxStop(void)
{
	int ret;

	ret = handle_ss_cmd(TxStop_ID, 1, NULL);

	return !ret;
}

/**
 ** Rx test begins when RxStart() is called.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_RxStart(void)
{
	int ret;

	ret = handle_ss_cmd(RxStart_ID, 1, NULL);

	return !ret;
}

/**
 ** RxStop() makes Rx test stop.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_RxStop(void)
{
	int ret;

	ret = handle_ss_cmd(RxStop_ID, 1, NULL);

	return !ret;
}

/**
 ** GoodFrame() gets the number of good frames after the Rx test stopped.
 ** Good frames include some error frames which can be fixed.
 **
 ** @return : the number of good frames.
 **/
int RFT_GetGoodFrame(void)
{
	int ret;
	int good;

	ret = handle_ss_cmd(GetGoodFrame_ID, 1, NULL);
	if(ret == 0){
		good = ss_cmd_pri.good_rx_frame;
		SSPRINT("good frame = %d\n",good);
		return good ;
	}
	else{
		SSPRINT("Get Rx good frame failed\n");
		return !ret;/*get rx frame count failed*/
	}
}

/**
 ** ErrorFrame() gets the number of error frames after the Rx test stopped.
 **
 ** @return : the number of error frames.
 **/
int RFT_GetErrorFrame(void)
{
	int ret;
	int bad;

	ret = handle_ss_cmd(GetErrorFrame_ID, 1, NULL);
	if(ret == 0){
		bad = ss_cmd_pri.err_rx_frame;
		SSPRINT("bad frame = %d\n",bad);
		return bad;
	}
	else{
		SSPRINT("Get Rx error frame failed\n");
		return !ret;/*get rx frame count failed*/
	}
}

/**
 ** JoinWithSSID() joins with given ssid using normal firmware
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_JoinWithSSID(const char* ssid)
{
	int ret = SS_SUCCESS;
	struct nl80211_global *global = &g_nl80211;

	ret = fsm_handle(JoinWithSSID_ID,&ss_cmd_pri);
	if(ret){
		pr_err("FSM_CHECK ERROR %s\n", __func__);
		return !ret;
	}
	set_join_ssid(global,ssid);

	ret = nl80211_join_ssid(global);
	return !ret;
}

/**
 ** JoinWithSSID2G() joins 2.4G band with given ssid using normal firmware
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_JoinWithSSID2G(const char* ssid)
{
	return RFT_JoinWithSSID(ssid);
}

/**
 ** JoinWithSSID5G() joins 5G band with given ssid using normal firmware
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_JoinWithSSID5G(const char* ssid)
{
	ssid = ssid;
	pr_err("This feature not surpport\n");
	return RET_ERR;
}

/**
 ** SetAntenna determines the Antenna of ANT_1,ANT_2,both.
 **
 ** @param ant should be the member of enum 'AntennaType'.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_SetAntenna(AntennaType ant)
{
	ant = ant;
	pr_err("This feature not surpport\n");
	return RET_ERR;
}


/**
 ** GetErrorString gets the detailed error result when the above methods return failure.
 **/
const char *RFT_GetErrorString(void)
{
	SSPRINT("%s\n", ss_cmd_pri.errbuf);
	return ss_cmd_pri.errbuf;
}

/**
 ** GetRSSI() gets corrent RSSI
 **
 ** @return RSSI value on success, = 0 on failure.
 **/
int RFT_GetRSSI(void)
{
	int ret;
	int rssi = 0;

	ret = handle_ss_cmd(GetRSSI_ID, 1, NULL);
	if (ret == 0) {
		rssi = ss_cmd_pri.rssi;
		rssi = -rssi;
		SSPRINT("rssi = %d\n", rssi);
		return rssi;
	}

	return rssi;
}
/**
 ** SetTxCount() sets the size of tx count.
 ** For TxStart without this option,
 **
 ** @param size means number of tx.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int RFT_SetTxCount(unsigned int size)
{
	int ret;
	char param_str[10];
	char *str = (char *)param_str;

	sprintf(param_str,"%d",size);
	ret = handle_ss_cmd(SetTxCount_ID, 2, &str);

	return !ret;
}
/**
 ** boot_cmd_handle handle samsung private cmd.
 **
 ** @param argc is the cmd and the params totoal count
 ** @param argv is the cmd param pointer's pointer
 **
 ** @return 0 on success, < 0 on failure.
 **/
int boot_cmd_handle(int cmd_id,int argc, char **argv)
{
	int i = 0;
	int cmd_c = argc + 1; //add a param ->wlan0
	char *cmd_v[cmd_c];
	int ret;
	construct_cmd(cmd_c,cmd_v,sprd_cmd_name[cmd_id],argv);/*param posit skip cmd_id*/
	for(i = 0; i < cmd_c; i++)
	{
		SSPRINT("attr[%d] = %s\n",i,cmd_v[i]);
	}

	ret =  __handle_ss_cmd(cmd_c,(char **)cmd_v,(void *)&ss_cmd_pri);
	return ret;
}
/**
 ** Set 'Device under STA MODE
 **
 ** @return 0 on success, < 0 on failure.
 **/
int StartStaMode(void)
{
	struct nl80211_global *global = &g_nl80211;

	SSPRINT("%s\n",__func__);
#ifndef CONFIG_SAMSUNG_EXT
	if(system(INSMOD_WLAN)){
		pr_err("sta insmod ko failed\n");
		return -1;
	}
#endif
	if(iw_nl80211_init(global)){
		pr_err("ss_nl80211_init_failed\n");
		return -1;
	}
	if(nl80211_set_interface(global, NL80211_IFTYPE_STATION)){
		pr_err("set interface STATION failed\n");
		return -1;
	}
	if (linux_set_iface_flags(global->ioctl_sock, IFNAME, DEV_UP)) {
		pr_err("ifconfig wlan0 up failed\n");
		return -1;
	}
	return SS_SUCCESS;
}

/**
 ** Close STA MODE.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int StopStaMode(void)
{
	struct nl80211_global *global = &g_nl80211;

	SSPRINT("%s\n",__func__);
	if (linux_set_iface_flags(global->ioctl_sock, IFNAME, DEV_DOWN)) {
		pr_err("ifconfig wlan0 down failed\n");
		return -1;
	}
#ifndef CONFIG_SAMSUNG_EXT
	if(system(RMMOD_WLAN)){
		pr_err("sta rmmod wlan failed\n");
		return -1;
	}
#endif
	nl80211_iw_cleanup(global);
	return SS_SUCCESS;
}

/**
 ** Set 'Device under NPI mode
 **
 ** @return 0 on success, < 0 on failure.
 **/
int StartNpiMode(void)
{
#define StartNpiMode_ID	(0x0)

	SSPRINT("%s\n",__func__);
#ifndef CONFIG_SAMSUNG_EXT
	if(system(INSMOD_WLAN)){
		pr_err("npi insmod ko failed\n");
		return -1;
	}
#endif
	if(boot_cmd_handle(StartNpiMode_ID,1,NULL)){
		pr_err("NPI mode failed, please close wifi!\n");
		return -1;
	}
        if (SetLnaOn()) {
		pr_err("send lna on failed\n");
		return -1;
	}
	ss_cmd_pri.rate = 0;
	ss_cmd_pri.long_preamble = 0;
	return SS_SUCCESS;
}

/**
 ** Close NpiMode.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int StopNpiMode(void)
{
#define StopNpiMode_ID	(0x1)
        if (SetLnaOff()) {
		pr_err("send lna off failed\n");
		return -1;
        }
	SSPRINT("%s\n",__func__);
	if(boot_cmd_handle(StopNpiMode_ID,1,NULL)){
		pr_err("send stop npi mode failed\n");
		return -1;
	}
#ifndef CONFIG_SAMSUNG_EXT
	if(system(RMMOD_WLAN)){
		pr_err("npi rmmod ko failed\n");
		return -1;
	}
#endif
	return SS_SUCCESS;
}
/**
 ** SetLnaOn() sets lna on.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int SetLnaOn(void)
{
	int ret;
	SSPRINT("%s\n",__func__);
	ret = boot_cmd_handle(SetLnaOn_ID, 1, NULL);

	return ret;
}
/**
 ** SetLnaOff() sets lna off.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int SetLnaOff(void)
{
	int ret;
	SSPRINT("%s\n",__func__);
	ret = boot_cmd_handle(SetLnaOff_ID, 1, NULL);

	return ret;
}
/**
 ** SetWlanCap() sets wlan psm cap.
 **
 ** @return 0 on success, < 0 on failure.
 **/
int SetWlanCap(unsigned int cap)
{
	int ret;
	char param_str[10];
	char *str = (char *)param_str;

	sprintf(param_str,"%d",cap);
	ret = boot_cmd_handle(SetWlanCap_ID, 2, &str);

	return ret;
}
int RFT_SetPsCap(void)
{
#define PSM_PATH "/data/.psm.info"
/**
 ** enable: 0x0
 ** disable: 0x1
 ** STA: bit 0
 ** GC: bit 1
 **/
#define STA_GC_EN_SLEEP        (0x3)
#define STA_GC_NO_SLEEP        (0x0)

	FILE *fs = NULL;
	char buf[4];
	unsigned int flag = 0;
	int num;

	fs = fopen(PSM_PATH,"r");
	if(fs == NULL) {
		flag = STA_GC_EN_SLEEP;
		goto LPC;
	}
	num = fread(buf, sizeof(unsigned int), 1, fs);
	fclose(fs);

	if (num == 1)
		flag = atoi(buf);
LPC:
	flag = flag ? STA_GC_EN_SLEEP : STA_GC_NO_SLEEP;
	printf("%s %s mode, flag = %d\n",
			__func__, flag ? "normal" : "rf", flag);
	/*set_wlan_cap*/
	return SetWlanCap(flag);
}
/**
 ** trigger scan
 **
 ** @return 0 on success, < 0 on failure.
 **/
int TriggerScan(void)
{
	int ret = SS_SUCCESS;
	struct nl80211_global *global = &g_nl80211;

	ret = fsm_handle(TriggerScan_ID,&ss_cmd_pri);
	if(ret){
		pr_err("FSM_CHECK ERROR %s\n", __func__);
		return ret;
	}

	ret = nl80211_trigger_scan(global);
	return ret;
}
/**
 ** SetCountryCode() using normal firmware
 **
 ** @return 0 on success, < 0 on failure.
 **/
int SetCountryCode(const char *s)
{
	int ret = SS_SUCCESS;
	struct nl80211_global *global = &g_nl80211;

	ret = fsm_handle(SetCountryCode_ID,&ss_cmd_pri);
	if(ret){
		pr_err("%s,FSM_CHECK ERROR %d\n", __func__, ret);
		return ret;
	}

	ret = wpa_driver_nl80211_set_country(global,s);
	if(ret){
		goto OUT;
	}
	ret = select_nl80211(global,1,NULL,false);
OUT:
	if(ret){
		pr_err("Set Country code error\n");
		return ret;
	}
	return ret;
}
/**
 ** GetChannelist() using normal firmware
 **
 ** @return 0 on success, < 0 on failure.
 **/
int GetChannelist(void)
{
	int i, ret = SS_SUCCESS;
	struct nl80211_global *global = &g_nl80211;

	ret = fsm_handle(GetChannelist_ID,&ss_cmd_pri);
	if(ret){
		pr_err("%s FSM_CHECK ERROR %d\n", __func__, ret);
		return ret;
	}
	global->chan_bitmap = 0;
	ret = nl80211_send_get_reg(global);
	if(ret){
		pr_err("set get channelist cmd error\n");
		return ret;
	}
	for(i = 1; i < 15; i++){
		if(global->chan_bitmap & CHANNEL(i))
			SSPRINT("channel_%d\n", i);
	}
	return global->chan_bitmap;
}
int AddInterface(const char *s)
{
	int ret = SS_SUCCESS;
	struct nl80211_global *global = &g_nl80211;
	const u8 mac_addr[6] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5};

	SSPRINT("mac_addr %s\n",__func__);
	ret = fsm_handle(AddInterface_ID,&ss_cmd_pri);
	if(ret){
		pr_err("%s FSM_CHECK ERROR %d\n", __func__, ret);
		return ret;
	}

	ret =  nl80211_create_iface_once(global, s,
			/*NL80211_IFTYPE_P2P_DEVICE*/NL80211_IFTYPE_STATION,
			mac_addr, 0, NULL, NULL);
	if(ret < 0){
		pr_err("Add interface failed\n");
	}
	return ret;
}
int DelInterface(const char *s)
{
	int ret = SS_SUCCESS;
	struct nl80211_global *global = &g_nl80211;

	SSPRINT("%s\n",__func__);
	ret = fsm_handle(DelInterface_ID,&ss_cmd_pri);
	if(ret){
		pr_err("%s FSM_CHECK ERROR %d\n", __func__, ret);
		return ret;
	}

	ret = nl80211_remove_iface(global, s);

	if(ret < 0){
		pr_err("Del interface failed\n");
	}
	return ret;
}
int SetScanTime(const char *s)
{
	int ret = SS_SUCCESS;
	struct nl80211_global *global = &g_nl80211;

	SSPRINT("%s\n",__func__);
	ret = fsm_handle(SetScanTime_ID,&ss_cmd_pri);
	if(ret){
		pr_err("%s FSM_CHECK ERROR %d\n", __func__, ret);
		return ret;
	}

	ret = android_priv_cmd(global, s);

	if(ret < 0){
		pr_err("Set Scan Time failed\n");
	}
	return !ret;
}
