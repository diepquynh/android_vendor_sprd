#ifndef __STRING_CN_H__
#define __STRING_CN_H__

#define MENU_TITLE_ROOT				" 展讯工厂测试模式"
#define MENU_TITLE_PHONETEST			" 手机测试项 "
#define MENU_TITLE_VERSION			" 手机软件版本信息"

#define MENU_BOARD_AUTOTEST   			"- PCBA自动测试"
#define MENU_BOARD_SINGLETEST  			"- PCBA单项测试"
#define MENU_PHONE_AUTOTEST			"- 整机自动测试"
#define MENU_PHONE_SINGLETEST			"- 整机单项测试"
#define MENU_NOT_AUTO_TEST      		"- 建议抽测项"
#define MENU_PHONE_INFO         		"- 手机信息"
#define MENU_BOARD_REPORT       		"- PCBA测试结果"
#define MENU_PHONE_REPORT       		"- 整机测试结果"
#define MENU_NOT_AUTO_REPORT    		"- 抽测项结果"
#define MENU_PHONE_REBOOT       		"< 重启"
#define MENU_FACTORY_RESET      		"< 恢复出厂设置"

#define MENU_CALI_INFO				"- RF CALI测试"
#define MENU_VERSION				"- 软件版本信息"
#define MENU_PHONE_INFO_TEST    		"- 手机信息测试"
#define TEST_REPORT             		"- 测试信息"

#define MENU_TEST_KEY				"- 按键"
#define MENU_TEST_LCD				"- 屏幕"
#define MENU_TEST_VIBRATOR			"- 震动"
#define MENU_TEST_BACKLIGHT			"- 背光"
#define MENU_TEST_VB_BL				"- 震动和背光"
#define MENU_TEST_LED				"- LED指示灯"
#define MENU_TEST_TP				"- 触摸屏"
#define MENU_TEST_MULTI_TOUCH                   "- 多点触摸"
#define MENU_TEST_PHONE_LOOPBACK   	        "- 手机回路"
#define MENU_TEST_SDCARD			"- 存储卡"
#define MENU_TEST_EMMC			        "- EMMC"
#define MENU_TEST_RTC			        "- 实时时钟"
#define MENU_TEST_MAINLOOP      		"- 回环主路测试"
#define MENU_TEST_ASSISLOOP     		"- 回环辅路测试"
#define MENU_TEST_SENSOR			"- 传感器测试"
#define MENU_TEST_ASENSOR           "- 加速度传感器"
#define MENU_TEST_GSENSOR			"- 陀螺仪传感器"
#define MENU_TEST_MSENSOR			"- 磁传感器"
#define MENU_TEST_LSENSOR			"- 光距离传感器"
#define MENU_TEST_PSENSOR           "- 压力传感器"
#define MENU_TEST_FCAMERA			"- 前置摄像头"
#define MENU_TEST_FACAMERA			"- 前置辅摄像头"
#define MENU_TEST_BCAMERA_FLASH			"- 后置摄像头"
#define MENU_TEST_ACAMERA			"- 后置辅摄像头"
#define MENU_TEST_BCAMERA			"- 后置摄像头"
#define MENU_TEST_FLASH				"- 闪光灯"
#define MENU_TEST_CHARGE			"- 充电"
#define MENU_TEST_HEADSET			"- 耳机"
#define MENU_TEST_FM				"- 收音机"
#define MENU_TEST_BT				"- 蓝牙"
#define MENU_TEST_WIFI				"- WIFI无线网络"
#define MENU_TEST_GPS				"- GPS定位"
#define MENU_TEST_TEL				"- 电话测试"
#define MENU_TEST_OTG				"- OTG测试"
#define MENU_TEST_SIMCARD			"- SIM卡"
#define MENU_TEST_RECEIVER			"- 听筒"
#define MENU_TEST_SPEAKER			"- 扬声器"
#define MENU_CALI_ACCSOR			"- 加速度传感器校准"
#define MENU_CALI_CYRSOR                       "- 陀螺仪校准"
#define MENU_CALI_MAGSOR			"- 磁传感器校准"
#define MENU_CALI_PROSOR			"- 距离传感器手动校准"
#define MENU_CALI_AUTOPROSOR			"- 距离传感器自动校准"
#define MENU_TEST_SOUNDTRIGGER			"- 语音唤醒"
#define MENU_TEST_FINGERSOR			"- 指纹传感器"
#define MENU_MULTI_TEST				"- 多项测试结果"

#define TEXT_IMEI               		"IMEI:"
#define TEXT_SN                 		"SN:"
#define TEXT_UID				"UID:"
#define TEXT_INVALIDSN1         		"Invalid sn1"
#define TEXT_INVALIDSN2         		"Invalid sn2"
#define TEXT_WIFI_ADDR          		"Wifi Mac Address:"
#define TEXT_INVALID_WIFI         		"Invalid WIFI Mac"
#define TEXT_BT_ADDR				    "Bluetooth Mac Address:"
#define TEXT_PHASE_CHECK_RESULT 		"Phasecheck Result:"
#define TEXT_INVALID            		"Invalid"
#define TEXT_VALID              		"Valid"
#define TEXT_PHASE_PASS         		"PASS"
#define TEXT_PHASE_FAILED       		"FAILED"
#define TEXT_PHASE_NOTTEST      		"NOT TEST"

#define TEXT_SIM2				"检测SIM卡2"
#define TEXT_PRE_PAGE				"上一页"
#define TEXT_NEXT_PAGE			        "下一页"
#define TEXT_PASS				"成功"
#define TEXT_FAIL				"失败"
#define TEXT_GOBACK				"返回"
#define TEXT_START				"开始"
#define TEXT_TEST_FAIL_CASE			"重测失败项"
#define TEXT_NA					"未测"
#define TEXT_NS					"不支持"
#define TEXT_TEST_PASS				"测试成功!"
#define TEXT_TEST_FAIL				"测试失败!"
#define TEXT_TEST_TIMEOUT			"连接超时!"
#define TEXT_TEST_NA				"测试未完成!"
#define TEXT_ANY_KEY				"按任意键继续!"
#define TEXT_FINISH				"测试结束!!!"
#define TEXT_OPEN_DEV_FAIL			"打开设备失败!"
#define TEXT_WAIT_TIPS         			"请等待..."
#define TEXT_GETFAIL        "获取失败！"
#define TEXT_CALI_PASS				"校准成功！"
#define TEXT_CALI_FAIL				"校准失败，请尝试重新校准！"
#define TEXT_CALI_NA				"校准未完成！"
#define TEXT_SENSOR_OPEN			"请等待传感器打开..."
#define TEXT_SENSOR_CALI			"正在进行校准，请等待..."

//LCD
#define LCD_TEST_TIPS               		"是否看到了白黑红绿蓝5种颜色？"

//TOUCH
#define TEXT_MULTI_TOUCH_START       		"请同时触摸移动两个方框,使之靠近"

//VB&BL
#define TEXT_VIB_START				"震动开始"
#define TEXT_VIB_FINISH				"震动结束"
#define TEXT_BL_ILLUSTRATE			"请观察背光亮度是否变化！"
#define TEXT_BL_OVER                		"亮度变化结束！！"

//LED
#define TEXT_LED_TIPS				"请观察LED指示灯是否按照红绿蓝3种颜色顺序显示?"
#define TEXT_LED_RED				"状态指示灯红色"
#define TEXT_LED_GREEN				"状态指示灯绿色"
#define TEXT_LED_BLUE				"状态指示灯蓝色"
#define TEST_LED_NOTSUPPORT    			"手机无LED指示灯,不支持LED指示灯测试"

//KEY
#define TEXT_KEY_ILLUSTRATE			"请分别点击如下按键："
#define TEXT_KEY_VOLUMEDOWN			"音量下键"
#define TEXT_KEY_VOLUMEUP			"音量上键"
#define TEXT_KEY_POWER				"电源键"
#define TEXT_KEY_MENU				"菜单键"
#define TEXT_KEY_BACK				"返回键"
#define TEXT_KEY_HOMEPAGE			"主页键"
#define TEXT_KEY_CAMERA				"相机键"

//MIC
#define TEST_ASSIS_NOTSUPPORT 			"手机无辅MIC,不支持回环辅路测试"
#define TEXT_LB_MICSPEAKER			"麦克风与扬声器回路测试中..."
#define TEXT_LB_MICRECEIVER			"麦克风与SPK回路测试中..."

//RECIVER
#define TEXT_RECV_PLAYING			"听筒中正在播放..."
#define TEXT_SPE_PLAYING            		"扬声器正在播放..."

//CHARGE
#define TEXT_CHG_BATTER_VOL			"电池电压:"
#define TEXT_CHG_RUN_Y				"当前正在充电..."
#define TEXT_CHG_RUN_N				"当前未在充电"
#define TEXT_CHG_TYPE				"充电器类型:"
#define TEXT_CHG_VOL				"充电电压:"
#define TEXT_CHG_CUR				"充电电流:"
#define TEXT_BATTARY_CUR                	"流入电池电流:"

//SDCARD
#define TEXT_SD_START				"存储卡测试开始："
#define TEXT_SD_OPEN_OK				"检测到存储卡"
#define TEXT_SD_OPEN_FAIL			"未检测到存储卡"
#define TEXT_SD_STATE_OK			"存储卡容量:"
#define TEXT_SD_STATE_FAIL			"存储卡容量检测失败!"
#define TEXT_SD_RW_OK				"读写测试成功!"
#define TEXT_SD_RW_FAIL				"读写测试失败!"

//EMMC
#define TEXT_EMMC_STATE			    	"卡状态:"
#define TEXT_EMMC_CAPACITY			"内部存储容量:"
#define TEXT_EMMC_OK                		"有内部存储卡"
#define TEXT_EMMC_FAIL              		"无内部存储卡"

//SIM
#define TEXT_SIM1				"检测SIM卡1"
#define TEXT_SIM_SCANING			"检测SIM卡中..."
#define TEXT_SIM_RESULT				"检测结果："
#define TEXT_MODEM_INITING                      "后台正在打开SIM卡,请稍候测试"
//RTC
#define TEXT_RTC_TIME              		"系统时间"

//HEADSET
#define TEXT_HD_REINSERT			"请重新插入耳机"
#define TEXT_HD_UNINSERT			"请插入耳机!"
#define TEXT_HD_INSERTED			"耳机已插入!"
#define TEXT_HD_HAS_MIC				"耳机带有麦克风"
#define TEXT_HD_NO_MIC				"耳机不带麦克风"
#define TEXT_HD_MICHD				"麦克风与耳机回路测试中...."
#define TEXT_HD_KEY_STATE			"耳机按键状态:"
#define TEXT_HD_KEY_PRESSED			"按下"
#define TEXT_HD_KEY_RELEASE			"释放"

//FM
#define TEXT_FM_SEEK_TIMEOUT			"弱信号，搜台超时"
#define TEXT_FM_FAIL				"收音机测试失败"
#define TEXT_FM_OK				"收音机测试通过"
#define TEXT_FM_FREQ				"设置接收频率"
#define TEXT_FM_STRE				"当前信号强度"
#define TEXT_FM_SCANING             		"正在搜台..."
#define TEXT_FM_SET_SUCCESS			"设置成功"
#define TEXT_FM_SET_FAIL			"设置失败"

//ASENSOR
#define TEXT_AS_OPER1       "加速度传感器测试中..."
#define TEXT_AS_OPER2       "请将手机水平、垂直和横向摆放!"
#define AS_CALI_OPER1				"请将手机水平放置，开始加速度校准..."

//PSENSOR
#define TEXT_PrS_Value      "当前气压值:"
#define TEXT_PRS_OPER1      "气压传感器测试中..."

//PROSENSOR
#define PRO_CALI_OPER1				"平放手机，上方不要有遮挡"
//#define PRO_CALI_OPER2				"距离传感器校准失败，请尝试重新校准"
#define PRO_CALI_OPER2				"将挡板放置距离传感器3cm，点击开始按键进行校准"
#define PRO_CALI_OPER3				"将挡板放置距离传感器5cm，点击开始按键再次进行校准"

//GSENSOR
#define TEXT_SENSOR_OPEN_FAIL			"打开传感器失败!!!"
#define TEXT_SENSOR_OPEN_OK			"打开传感器成功!!!"
#define TEXT_SENSOR_DEV_INFO			"传感器设备信息:"
#define TEXT_SENSOR_PASS			"通过！"
#define TEXT_GS_OPER1				"陀螺仪传感器测试中..."
#define TEXT_GS_OPER2				"请摇动手机！"
#define GS_CALI_OPER1				"请将手机水平放置，开始陀螺仪校准..."
#define TEXT_GS_X				"X轴:"
#define TEXT_GS_Y				"Y轴:"
#define TEXT_GS_Z				"Z轴:"

//LENSOR
#define TEXT_ACC_OPER				"光距离传感器测试中..."
#define TEXT_PS_NEAR				"Proximity Sensor Near"
#define TEXT_PS_FAR				"Proximity Sensor Far"
#define TEXT_LS_LUX				"Light Sensor Lux: "

//MSENSOR
#define TEXT_MS_OPER1               		"磁传感器测试中..."
#define TEXT_MS_OPER2               		"请摇动手机！"
#define MAG_CALI_OPER1				"正在读取磁传感器校准数据"
#define MAG_CALI_OPER2				"请导入校准数据或尝试重新校准！"
#define TEXT_MS_X                   		"Azimuth:"
#define TEXT_MS_Y                   		"Pitch:"
#define TEXT_MS_Z                   		"Roll:"

//FINGERSOR
#define TEXT_FINGERSOR_OPER			"请勿触碰指纹检测模组，按“开始”按键开始模组初始化"
#define FINGER_INIT                 		"指纹模块正在初化..."
#define FINGER_DLOPEN_FAIL			"指纹库加载失败！"
#define FINGER_SENSOR_BAD			"传感器模组初始化失败！"
#define TEXT_FINGERSOR_OPER1			"手指放在指纹传感器上，按“开始”按键开始指纹检测..."

//SOUNDTRIGGER
#define TEXT_SOUNDTRIGGER_OPER			"语音唤醒正在初始化..."
#define TEXT_SOUNDTRIGGER_OPER1			"初始化完成，按“开始”按键，开始语音识别..."
#define TEXT_SOUNDTRIGGER_OPER2			"初始化失败，尝试重新测试！"
#define TEXT_SOUNDTRIGGER_OPER3			"语音识别超时！"
#define TEXT_SOUNDTRIGGER_OPER4			"正在进行检测..."

//CAMERA
#define CAMERA_START                		"注意观察图像"
#define CAMERA_LIGHT_ON             		"注意观察闪光灯"
#define CHARGE_TIPS                		"请使用未充满电池测试"

//BT
#define TEXT_BT_SCANING				"扫描中..."
#define BT_BACK_TEST                		"蓝牙正在后台测试,请稍候测试"

//WIFI
#define TEXT_WIFI_SCAN				"WIFI扫描结果:"

//OTG
#define OTG_TEST_START              		"OTG测试开始："
#define OTG_NOT_SUPPORT             		"不支持OTG功能"
#define OTG_SUPPORT                 		"支持OTG功能"
#define OTG_INSERT                  		"OTG线已插入"
#define OTG_UNSERT                  		"OTG线未插入"
#define OTG_DEVICE                  		"DEVICE端"
#define OTG_HOST                    		"HOST端"
#define OTG_DEVICE_INSERT           		"请在30s内插入OTG线及U盘"
#define TEXT_OTG_CAPACITY			"U盘容量:"
#define TEXT_OTG_UDISK_OK			"检测到U盘"
#define TEXT_OTG_UDISK_NO			"未检测到U盘"
#define TEXT_OTG_UDISK_SCANING      "正在检测U盘..."
//TEL
#define TEL_TEST_START              		"112拨打中....."
#define TEL_TEST_TIPS               		"请注意通话语音"
#define TEL_DIAL_OVER               		"已经拨出...."
#define TEL_DIAL_FAIL               		"打开协议栈失败,请稍后测试"

#define TEXT_CAPTURE                    "拍照"

#endif /*__STRING_CN_H__*/
