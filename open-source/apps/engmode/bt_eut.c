#include <stdio.h>
#include <stdlib.h>
#include "eut_opt.h"

#define LOG_TAG	"SPRDENG"

static int eut_mode;

struct eng_bt_eutops bt_eutops = {bteut,
    bteut_req};

int bteut(int command_code,char *rsp)
{
    ALOGI("bteut");
    if( command_code == 1)
        start_bteut(rsp);
    else if(command_code == 0)
        end_bteut(rsp);
    return 0;
}
int bteut_req(char *rsp)
{
    sprintf(rsp, "%s%d",EUT_BT_REQ,eut_mode);
    return 0;
}
int start_bteut(char *result)
{
    ALOGI("start_bteut----------------------");
    char *data_sucess = "sucessful";
    char *data_fail  = "fail";
    {
        ALOGI("=== receive BTUT test requirement! ===\n");
        ALOGI("=== BTUT test start! ===\n");
        int error = system("setprop ctl.stop bluetoothd");
        if(error == -1 || error == 127){
            ALOGE("=== BTUT test failed on cmd : setprop ctl.stop bluetoothd! ===\n");
            sprintf(result,"%s%d",EUT_BT_ERROR,error);
        }else{
            error = system("hciconfig uart0 up");
            if(error == -1 || error == 127){
                ALOGE("=== BTUT test failed on cmd : hciconfig uart0 up! ===\n");
                sprintf(result,"%s%d",EUT_BT_ERROR,error);
            }else{
                error = system("hciconfig hci0 up");
                ALOGI("hciconfig hci0 up");
                if(error == -1 || error == 127)
                {
                    ALOGE("=== BTUT test failed on cmd : hciconfig hci0 up! ===\n");
                    sprintf(result,"%s%d",EUT_BT_ERROR,error);
                }
                else
                {
                    ALOGI("hcitool cmd 0x03 0x03 start");
                    error = system("hcitool cmd 0x03 0x03");
                    ALOGI("hcitool cmd 0x03 0x03");
                    if(error == -1 || error == 127)
                    {
                        ALOGE("=== BTUT test failed on cmd : hcitool cmd 0x03 0x03! ===\n");
                        sprintf(result,"%s%d",EUT_BT_ERROR,error);
                    }
                    else
                    {

                        error = system("hcitool cmd 0x03 0x05  0x02 0x00 0x02");
                        ALOGI("hcitool cmd 0x03 0x1a 0x03");
                        if(error == -1 || error == 127)
                        {
                            ALOGE("=== BTUT test failed on cmd : hcitool cmd 0x03 0x1a 0x03! ===\n");
                            sprintf(result,"%s%d",EUT_BT_ERROR,error);
                        }
                        else
                        {

                            error = system("hcitool cmd 0x03 0x1a 0x03");
                            ALOGI("hcitool cmd 0x03 0x05  0x02 0x00 0x02");
                            if(error == -1 || error == 127)
                            {
                                ALOGE("=== BTUT test failed on cmd : hcitool cmd 0x03 0x05  0x02 0x00 0x02! ===\n");
                                sprintf(result,"%s%d",EUT_BT_ERROR,error);
                            }
                            else
                            {

                                error = system("hcitool cmd 0x06 0x03");
                                ALOGI("hcitool cmd 0x06 0x03");
                                if(error == -1 || error == 127)
                                {
                                    ALOGE("=== BTUT test failed on cmd : hcitool cmd 0x06 0x03! ===\n");
                                    sprintf(result,"%s%d",EUT_BT_ERROR,error);
                                }
                                else
                                {
                                    ALOGI("=== BTUT test succeed! ===\n");
                                    eut_mode=1;
                                    strcpy(result,EUT_BT_OK);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
int end_bteut(char *result)
{
    int error = system("hciconfig hci0 down");
    if(error == -1 || error == 27){
        sprintf(result,"%s%d",EUT_BT_ERROR,error);
    }else{
        strcpy(result,EUT_BT_OK);
        eut_mode=0;
    }
    return 0;
}
