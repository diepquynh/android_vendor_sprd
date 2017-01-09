#ifndef GPS_PC_MODE_H
#define GPS_PC_MODE_H

#define WARM_START   1
#define COLD_START   125
#define HOT_START    1024
#define FAC_START    65535
#define LOG_ENABLE   136
#define LOG_DISABLE  520

#define EUT_GPS_OK                  "+SPGPSTEST:OK"
#define EUT_GPS_ERROR               "+SPGPSTEST:ERR="
#define EUT_GPS_REQ                 "+SPGPSTEST:EUT="
#define EUT_GPS_PRN_REQ             "+SPGPSTEST:PRN="
#define EUT_GPS_SNR_REQ             "+SPGPSTEST:SNR="
#define EUT_GPS_RSSI_REQ            "+SPGPSTEST:RSSI="
#define EUT_GPS_TSXTEMP_REQ         "+SPGPSTEST:TSXTEMP="
#define EUT_GPS_TCXO_REQ            "+SPGPSTEST:TCXO="
#define EUT_GPS_READ_REQ            "+SPGPSTEST:READ="
#define EUT_GPS_SEARCH_REQ          "+SPGPSTEST:SEARCH="
#define EUT_GPS_SNR_NO_EXIST        "NO_EXIST"
#define EUT_GPS_NO_FOUND_STAELITE   "NO_FOUND_SATELLITE"
#define EUT_GPS_SV_ID               "SV_ID="
#define EUT_GPS_SV_NUMS             "SV_NUM="

#define EUT_GPSERR_SEARCH                   (153)
#define EUT_GPSERR_PRNSTATE                 (154)
#define EUT_GPSERR_PRNSEARCH                (155)

typedef void (*report_ptr)(const char* nmea, int length);
extern void set_report_ptr(report_ptr func);
extern void set_pc_mode(char input_pc_mode);
extern int gps_export_start(void);
extern int gps_export_stop(void);
extern int get_nmea_data(char *nbuff);
extern int set_gps_mode(unsigned int mode);
extern int get_init_mode(void);
extern int get_stop_mode(void);
extern void gps_eut_parse(char *buf,char *rsp);
extern int write_register(unsigned int addr,unsigned int value);
extern unsigned int read_register(unsigned int addr);
void cw_data_capture(const char* nmea, int length);

#endif
