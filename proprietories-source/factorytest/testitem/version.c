#include "testitem.h"

extern char imei_buf1[128];
extern char imei_buf2[128];
char sn1[72] = {0};
char sn2[72] = {0};
unsigned char phrash_valid=0;
char *pSN1 = NULL, *pSN2 = NULL;
unsigned int pTestSign , pItem;
int max_station_num;
int max_sn_len;
char *pStationName[SP15_MAX_STATION_NUM] = {NULL};
SP09_PHASE_CHECK_T phase_check_sp09;
SP15_PHASE_CHECK_T phase_check_sp15;
struct struct_info
{
	int color;
	char* content;
}info[100];

int test_version_show(void)
{
	char androidver[64];
	char tmpbuf[PROPERTY_VALUE_MAX];
	char sprdver[256];
	char kernelver[256];
	char* modemver;
	int fd;
	int cur_row = 2;
	int ret=0;

	ui_fill_locked();

	ui_show_title(MENU_TITLE_VERSION);
	ui_set_color(CL_WHITE);

	//get android version---Andorid 5.1
	memset(androidver, 0, sizeof(androidver));
	memset(tmpbuf, 0, sizeof(tmpbuf));
	property_get(PROP_PRODUCT_VER, tmpbuf, "");
	sprintf(androidver, "BB Chip: %s",tmpbuf);
	cur_row = ui_show_text(cur_row, 0, androidver);
	memset(tmpbuf, 0, sizeof(tmpbuf));
	memset(androidver, 0, sizeof(androidver));
	property_get(PROP_ANDROID_VER, tmpbuf, "");
	sprintf(androidver, "Android Version:%s",tmpbuf);
	cur_row = ui_show_text(cur_row+1, 0, androidver);

	// get kernel version--Linux version
	fd = open(PATH_LINUX_VER, O_RDONLY);
	if(fd < 0){
		LOGE("open %s fail!", PATH_LINUX_VER);
	} else {
		memset(kernelver, 0, sizeof(kernelver));
		read(fd, kernelver, sizeof(kernelver));
		LOGD("kernelver: %s", kernelver);
		cur_row = ui_show_text(cur_row+1, 0, kernelver);
	}


	modemver = test_modem_get_ver();
	LOGD("modemver: %s", modemver);
	cur_row = ui_show_text(cur_row+1, 0, modemver);

	gr_flip();

	while(ui_handle_button(NULL,TEXT_NEXT_PAGE,TEXT_GOBACK)!=RL_FAIL);

	return ret;
}

static int checkPhaseValue(void){
	FILE *fd = NULL;
	int readnum;
	unsigned int magic;
	int i=0;

	fd=fopen("/dev/block/platform/sdio_emmc/by-name/miscdata","r");
	if(NULL != fd){
		readnum = fread(&magic, sizeof(unsigned int), 1, fd);
		fclose(fd);
		LOGD("mmitest magic: 0x%x!;sizeof(phase_check_sp15)= %d, %d IN", magic, sizeof(phase_check_sp15), __LINE__);
	}else{
		LOGE("fail to open miscdata! %d IN", __LINE__);
		goto end;
	}
	if(magic == SP09_SPPH_MAGIC_NUMBER || magic == SP05_SPPH_MAGIC_NUMBER){
		fd=fopen("/dev/block/platform/sdio_emmc/by-name/miscdata","r");
		if(NULL != fd){
			readnum = fread(&phase_check_sp09, sizeof(SP09_PHASE_CHECK_T), 1, fd);
			fclose(fd);
			LOGD("mmitest check phase value %d IN", __LINE__);
		}else{
			LOGE("fail to open miscdata! %d IN", __LINE__);
			goto end;
		}

		max_station_num = phase_check_sp09.StationNum;	//SP09_MAX_STATION_NUM;
		max_sn_len = SP09_MAX_SN_LEN;
		pSN1 = phase_check_sp09.SN1;
		pSN2 = phase_check_sp09.SN2;
		pTestSign = phase_check_sp09.iTestSign;
		pItem = phase_check_sp09.iItem;
		LOGD("StationNum = %d; iTestSign: %u; iItem: %u; %d IN \n"\
			, phase_check_sp09.StationNum, phase_check_sp09.iTestSign, phase_check_sp09.iItem, __LINE__);
		for(i=0; i < max_station_num; i++){
			pStationName[i] = phase_check_sp09.StationName[i];
			LOGD("pStationName[%d]= %s; %d IN\n", i, pStationName[i], __LINE__);
		}
	}else if(magic == SP15_SPPH_MAGIC_NUMBER){
		fd=fopen("/dev/block/platform/sdio_emmc/by-name/miscdata","r");
		if(NULL != fd){
			readnum = fread(&phase_check_sp15, sizeof(SP15_PHASE_CHECK_T), 1, fd);
			fclose(fd);
			LOGD("mmitest check phase value %d IN", __LINE__);
		}else{
			LOGE("fail to open miscdata! %d IN", __LINE__);
			goto end;
		}

		max_station_num = phase_check_sp15.StationNum;	//SP15_MAX_STATION_NUM;
		max_sn_len = SP15_MAX_SN_LEN;
		pSN1 = phase_check_sp15.SN1;
		pSN2 = phase_check_sp15.SN2;
		pTestSign = phase_check_sp15.iTestSign;
		pItem = phase_check_sp15.iItem;
		LOGD("StationNum = %d; iTestSign: %u; iItem: %u; %d IN \n"\
			, phase_check_sp15.StationNum, phase_check_sp15.iTestSign, phase_check_sp15.iItem, __LINE__);
		for(i=0; i < max_station_num; i++){
			pStationName[i] = phase_check_sp15.StationName[i];
			LOGD("pStationName[%d]= %s; %d IN\n", i, pStationName[i], __LINE__);
		}
	} else {
        goto end;
    }
	phrash_valid=1;
	return 1;

end:
	phrash_valid=0;
	return 0;
}

int  isAscii(unsigned char b)
{
    if ((b >= 48 && b <= 57) || (b >= 65 && b <= 90) || (b >= 97 && b <= 122))
    {
        return 1;
    }
    return 0;
}

void  getSn1(char *string)
{
    if ( 0 == phrash_valid) {
        strcpy(string,TEXT_INVALIDSN1);
        LOGD("mmitest out of sn11");
        return ;
    }
    if (!isAscii(*pSN1)) {
            strcpy(string,TEXT_INVALIDSN1);
			LOGD("mmitest out of sn12");
			return ;
    }

    memcpy(string, pSN1, max_sn_len);
    LOGD("mmitest out of sn13");
    return ;
}

void  getSn2(char *string)
{
    if (phrash_valid == 0) {
        strcpy(string,TEXT_INVALIDSN2);
        LOGD("mmitest out of sn21");
        return ;
    }
    if (!isAscii(*pSN2)) {
        strcpy(string,TEXT_INVALIDSN2);
        LOGD("mmitest out of sn22");
        return ;
    }

    memcpy(string, pSN2, max_sn_len);
    LOGD("mmitest out of sn23");
    return ;
}

static int isStationTest(int station)
{
	unsigned int flag = 1;

//	usleep(5*1000);
	LOGD("pTestSign[%d]:%u; return:%d; %d IN", station, pTestSign, (0 == ((flag << station) & pTestSign)), __LINE__);
	return (0 == ((flag << station) & pTestSign));
}

static int  isStationPass(int station)
{
	unsigned int flag = 1;

//	usleep(5*1000);
	LOGD("pItem[%d]:%u; return:%d; %d IN", station, pItem, (0 == ((flag << station) & pItem)), __LINE__);
	return (0 == ((flag << station) & pItem));
}

static int getTestsAndResult(int row)
{
	int i;
	static char testResult[50][64];
	char testname[32];

	if (phrash_valid == 0 || max_station_num == 0)
	{
		//strcpy(testResult,TEXT_PHASE_NOTTEST);
		LOGD("mmitest out of Result1");
		//ui_set_color(CL_RED);
		//ui_show_text(row, 0, testResult);
		info[row].color = CL_RED;
		info[row++].content = TEXT_PHASE_NOTTEST;
		return row;
	}
	else
	{
		LOGD("pStationName[0]: %s; pStationName[0][0]: %c! %d IN", pStationName[0], pStationName[0][0], __LINE__);
		if (!isAscii((unsigned char)pStationName[0][0]))
		{
			LOGD("mmitest out of Result2 %d IN", __LINE__);
			//strcpy(testResult,TEXT_PHASE_NOTTEST);
			//ui_set_color(CL_RED);
			//ui_show_text(row, 0, testResult);
			info[row].color = CL_RED;
			info[row++].content = TEXT_PHASE_NOTTEST;
			return row;
		}

		for (i = 0; i < max_station_num && i < 50; i++)
		{
			//memset(testname,0,sizeof(testname));
			memset(testResult[i],0,sizeof(testResult[i]));
			//strcpy(testname, pStationName[i]);
			if (!isStationTest(i))
			{
				sprintf(testResult[i],"%s %s\n",pStationName[i],TEXT_PHASE_NOTTEST);
				//ui_set_color(CL_WHITE);
				info[row].color = CL_WHITE;
			}
			else if (isStationPass(i))
			{
				sprintf(testResult[i],"%s %s\n",pStationName[i],TEXT_PHASE_PASS);
				//ui_set_color(CL_GREEN);
				info[row].color = CL_GREEN;
			}
			else
			{
				sprintf(testResult[i],"%s %s\n",pStationName[i],TEXT_PHASE_FAILED);
				//ui_set_color(CL_RED);
				info[row].color = CL_RED;
			}
			//ui_show_text(row+i, 0, testResult);
			info[row++].content = testResult[i];
		}
		return row;
	}
}

long get_seed()
{
	struct timeval t;
	unsigned long seed = 0;
	gettimeofday(&t, NULL);
	seed = 1000000 * t.tv_sec + t.tv_usec;
	LOGD("generate seed: %lu", seed);
	return seed;
}

/* This function is for internal test only */
void create_random_mac(unsigned char *mac)
{
	int i;

	LOGD("generate random mac");
	memset(mac, 0, MAC_LEN);

	srand(get_seed()); /* machine run time in us */
	for(i=0; i<MAC_LEN; i++) {
		mac[i] = rand() & 0xFF;
	}

	//mac[0] &= 0xFE; /* clear multicast bit */
	//mac[0] &= 0xFD; /* clear local assignment bit, p2p MAC will be auto generated by set this bit to 1 */
	/* Set Spreadtrum 24bit OUI */
	mac[0] = 0x40;
	mac[1] = 0x45;
	mac[2] = 0xDA;
}

bool is_zero_ether_addr(const unsigned char *mac)
{
	return !(mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]);
}

bool is_file_exists(const char *file_path)
{
	return access(file_path, 0) == 0;
}

bool is_wifimac_invalid(char * mac)
{
	return (mac[0] & 1) == 0;
}

void force_replace_file(const char *dst_file_path, const char *src_file_path)
{
	FILE *f_src, *f_dst;
	char buf[100];
	int ret = -1;

	f_src = fopen(src_file_path, "r");
	if (f_src == NULL) return;
	fgets(buf, sizeof(buf), f_src);
	fclose(f_src);

	if(strcmp(src_file_path, BT_MAC_FACTORY_CONFIG_FILE_PATH) == 0){
	    if (0 != access("/data/misc/bluedroid/",F_OK) ){
	        ret = mkdir("/data/misc/bluedroid/",0666);
	        if ( ret < 0) {
	            LOGD("mkdir /data/misc/bluedroid/ failed.");
	            return;
	        }
	    }
	}else{
	    if (0 != access("/data/misc/wifi/",F_OK) ){
	        ret = mkdir("/data/misc/wifi/",0666);
	        if ( ret < 0) {
	            LOGD("mkdir /data/misc/wifi/ failed.");
	            return;
	        }
	    }
	}
	f_dst = fopen(dst_file_path, "w");
	if (f_dst == NULL) {
	    LOGE("force_replace_config_file open fail.");
	    return;
	}
	fputs(buf, f_dst);
	fclose(f_dst);

	sprintf(buf, "chmod 666 %s", dst_file_path);
	system(buf);
	LOGD("force_replace_config_file: %s", buf);
}

void read_mac_from_file(const char *file_path, unsigned char *mac)
{
	FILE *f;
	unsigned char mac_src[MAC_LEN];
	char buf[20];

	f = fopen(file_path, "r");
	if (f == NULL) return;

	if (fscanf(f, "%02x:%02x:%02x:%02x:%02x:%02x", &mac_src[0], &mac_src[1], &mac_src[2], &mac_src[3], &mac_src[4], &mac_src[5]) == 6) {
		memcpy(mac, mac_src, MAC_LEN);
		sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", mac_src[0], mac_src[1], mac_src[2], mac_src[3], mac_src[4], mac_src[5]);
		LOGD("mac from configuration file %s: %s", file_path,buf);
	} else {
		memset(mac, 0, MAC_LEN);
	}

	fclose(f);
}

void write_mac_to_file(const char *file_path, const unsigned char *mac)
{
	FILE *f;
	unsigned char mac_src[MAC_LEN];
	char buf[100];
	int ret = -1;

	if(strcmp(file_path, BT_MAC_CONFIG_FILE_PATH) == 0){
	    if (0 != access("/data/misc/bluedroid/",F_OK) ){
	        ret = mkdir("/data/misc/bluedroid/",0666);
	        if ( ret < 0) {
	            LOGD("mkdir /data/misc/bluedroid/ failed.");
	            return;
	        }
	    }
	}else{
	    if (0 != access("/data/misc/wifi/",F_OK) ){
	        ret = mkdir("/data/misc/wifi/",0666);
	        if ( ret < 0) {
	            LOGD("mkdir /data/misc/wifi/ failed.");
	            return;
	        }
	    }
	}
	f = fopen(file_path, "w");
	if (f == NULL) return;

	sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	fputs(buf, f);
	LOGD("write mac to configuration file: %s", buf);

	fclose(f);

	sprintf(buf, "chmod 666 %s", file_path);
	system(buf);
}

void generate_bt_mac(char * mac)
{
    int fd_btaddr=0;
    int ret = -1;
    unsigned char bt_mac[MAC_LEN] = {0};
    // if vaild mac is in configuration file, use it
    if(is_file_exists(BT_MAC_CONFIG_FILE_PATH)) {
	LOGD("bt configuration file exists");
	read_mac_from_file(BT_MAC_CONFIG_FILE_PATH,(unsigned char *) mac);
	if(!is_zero_ether_addr((unsigned char *)mac)) return;
    }
    // force replace configuration file if vaild mac is in factory configuration file
    if(is_file_exists(BT_MAC_FACTORY_CONFIG_FILE_PATH)) {
	LOGD("bt factory configuration file exists");
	read_mac_from_file(BT_MAC_FACTORY_CONFIG_FILE_PATH, (unsigned char *)mac);
	if(!is_zero_ether_addr((unsigned char *)mac)) {
	    force_replace_file(BT_MAC_CONFIG_FILE_PATH, BT_MAC_FACTORY_CONFIG_FILE_PATH);
	    return;
	}
    }
    // generate random mac and write to configuration file
    create_random_mac((unsigned char *)mac);
    write_mac_to_file(BT_MAC_CONFIG_FILE_PATH, (const unsigned char *)mac);
}

void generate_wifi_mac(char * mac)
{
    int fd_wifiaddr=0;
    int ret = -1;
    unsigned char wifi_mac[MAC_LEN] = {0};
    // force replace configuration file if vaild mac is in factory configuration file
    if(is_file_exists(WIFI_MAC_FACTORY_CONFIG_FILE_PATH)) {
	LOGD("wifi factory configuration file exists");
	read_mac_from_file(WIFI_MAC_FACTORY_CONFIG_FILE_PATH, (unsigned char *)mac);
	if(!is_zero_ether_addr((unsigned char *)mac)) {
	    force_replace_file(WIFI_MAC_CONFIG_FILE_PATH, WIFI_MAC_FACTORY_CONFIG_FILE_PATH);
	    return;
	}
    }
	// if vaild mac is in configuration file, use it
    if(is_file_exists(WIFI_MAC_CONFIG_FILE_PATH)) {
	LOGD("wifi configuration file exists");
	read_mac_from_file(WIFI_MAC_CONFIG_FILE_PATH, (unsigned char *)mac);
	if(!is_zero_ether_addr((unsigned char *)mac)) return;
    }
    // generate random mac and write to configuration file
    create_random_mac((unsigned char *)mac);
    write_mac_to_file(WIFI_MAC_CONFIG_FILE_PATH, (unsigned char *)mac);
}

void  getUid(char *string)
{
	int fd ;
	int ret = -1 ;
	unsigned char data[50] = {0};
	char *cp = data, *cp1 = data;
	struct stat uid_stat;

	fd = open("sys/class/misc/sprd_efuse_otp/uid", O_RDONLY);
	if(fd < 0)
		fd = open("/sys/class/misc/sprd_otp_ap_efuse/uid", O_RDONLY);

	LOGD("fd: %d! %d IN", fd, __LINE__);
	if(fd < 0){
		LOGE("can't get fstat! %d IN\n", __LINE__);
		goto end;
	}
	ret = read(fd , cp , sizeof(data) ) ;
	LOGD("%s data read ret: %d, size: %d, data: %s", __func__, ret, sizeof(data), cp);
	eng_tok_start(&cp);
	cp1 = ++cp;
	eng_tok_nextline(&cp1);
	memcpy(string, cp, --cp1-cp);
	LOGD("mmitest out of uid: %s! uid length: %d! %d IN", string, cp1-cp, __LINE__);
end:
	if (!isAscii(*cp)) {
		strcpy(string,TEXT_INVALID);
		LOGD("mmitest out of uid: %s! %d IN", string, __LINE__);
	}
	close(fd);
	return ;
}

int test_phone_info_show(void)
{
	char mac[MAC_LEN];
	char bt_addr[18];
	char wifi_addre[18];
	char uid[50];
	char *phase_check;
	char *phase_result;
	FILE *fd = NULL;
	int row =2;
	int n = 0;
	int i = 0;
	int max_row = 0;
	int cwidth=0,cheight=0,ewidth=0,eheight=0;
	gr_font_size(&cwidth, &cheight, &ewidth, &eheight);

	#ifdef SPRD_VIRTUAL_TOUCH
		max_row = (gr_fb_height() - (gr_fb_height()>>3)) /eheight;
	#else
		max_row = gr_fb_height() /eheight;
	#endif

	ui_fill_locked();

	ui_show_title(MENU_PHONE_INFO_TEST);
	ui_set_color(CL_WHITE);
	memset(bt_addr, 0, sizeof(bt_addr));
	memset(wifi_addre,0,sizeof(wifi_addre));

	generate_bt_mac(mac);
	sprintf(bt_addr, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	LOGD("mmitest info_show Local MAC address:%s",bt_addr);

	generate_wifi_mac(mac);
	sprintf(wifi_addre, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	LOGD("mmitest info_show Local MAC address:%s",wifi_addre);

	if( 1 == checkPhaseValue())
		phase_check=TEXT_VALID;
	else
		phase_check=TEXT_INVALID;
	LOGD("mmitest phase_check=%s",phase_check);
	getSn1(sn1);
	LOGD("mmitest sn1=%s",sn1);
	getSn2(sn2);
	LOGD("mmitest sn2=%s",sn2);
	getUid(uid);

	//ui_set_color(CL_WHITE);
	//row=ui_show_text(row, 0, TEXT_SN);
	info[n].color = CL_WHITE;
	info[n++].content = TEXT_SN;
	if(strcmp(sn1, TEXT_INVALIDSN1) == 0)
		//ui_set_color(CL_RED);
		info[n].color = CL_RED;
	else
		//ui_set_color(CL_GREEN);
		info[n].color = CL_GREEN;
	//row=ui_show_text(row, 0, sn1);
	info[n++].content = sn1;

	if(strcmp(sn2, TEXT_INVALIDSN2) == 0)
		//ui_set_color(CL_RED);
		info[n].color = CL_RED;
	else
		//ui_set_color(CL_GREEN);
		info[n].color = CL_GREEN;
	//row=ui_show_text(row, 0, sn2);
	info[n++].content = sn2;

	//ui_set_color(CL_WHITE);
	//row=ui_show_text(row, 0, TEXT_IMEI);
	info[n].color = CL_WHITE;
	info[n++].content = TEXT_IMEI;
	//ui_set_color(CL_GREEN);
	//row=ui_show_text(row, 0, imei_buf1);
	//row=ui_show_text(row, 0, imei_buf2);
	info[n].color = CL_GREEN;
	info[n++].content = imei_buf1;
	info[n].color = CL_GREEN;
	info[n++].content = imei_buf2;

	//ui_set_color(CL_WHITE);
	//row=ui_show_text(row, 0, TEXT_BT_ADDR);
	info[n].color = CL_WHITE;
	info[n++].content = TEXT_BT_ADDR;

	//ui_set_color(CL_GREEN);
	//row=ui_show_text(row, 0, bt_addr);
	info[n].color = CL_GREEN;
	info[n++].content = bt_addr;

	//ui_set_color(CL_WHITE);
	//row=ui_show_text(row, 0, TEXT_WIFI_ADDR);
	info[n].color = CL_WHITE;
	info[n++].content = TEXT_WIFI_ADDR;
	if(is_wifimac_invalid(mac)){
		//ui_set_color(CL_GREEN);
		//row=ui_show_text(row, 0, wifi_addre);
		info[n].color = CL_GREEN;
		info[n++].content = wifi_addre;
	}else{
		//ui_set_color(CL_RED);
		//row=ui_show_text(row, 0, TEXT_INVALID_WIFI);
		info[n].color = CL_RED;
		info[n++].content = TEXT_INVALID_WIFI;
	}

	//ui_set_color(CL_WHITE);
	//row=ui_show_text(row, 0, TEXT_PHASE_CHECK_RESULT);
	info[n].color = CL_WHITE;
	info[n++].content = TEXT_PHASE_CHECK_RESULT;
	//ui_set_color(CL_GREEN);
	n = getTestsAndResult(n);

	//ui_set_color(CL_WHITE);
	//row=ui_show_text(row+rownum, 0, TEXT_UID);
	info[n].color = CL_WHITE;
	info[n++].content = TEXT_UID;
	//ui_set_color(CL_GREEN);
	//row=ui_show_text(row, 0, uid);
	info[n].color = CL_GREEN;
	info[n++].content = uid;

	for( i = 0; i < n; i++)
	{
		int flag  = 1;
		ui_set_color(info[i].color);
		row=ui_show_text(row, 0, info[i].content);

		while( row >= max_row && 1 == flag)
		{
		       gr_flip();

			switch(ui_handle_button(NULL,TEXT_NEXT_PAGE,TEXT_GOBACK))
			{
				case RL_FAIL:
					flag = 0;
					return 0;
				case RL_NEXT_PAGE:
					if( (n-1) != i)		//if equal the last row needn't turn the next page
					{
						ui_set_color(CL_TITLE_BG);
						gr_fill(0, 0, gr_fb_width(), gr_fb_height());
						ui_show_title(MENU_PHONE_INFO_TEST);
						row = 2;
						flag = 0;
					}
					break;
			}
               }
       }

       gr_flip();

	while(ui_handle_button(NULL,TEXT_NEXT_PAGE,TEXT_GOBACK)!=RL_FAIL);

	return 0;
}
