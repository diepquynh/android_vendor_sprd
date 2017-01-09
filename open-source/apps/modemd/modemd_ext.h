#ifndef  _MODEM_EXT_H_
#define  _MODEM_EXT_H_

/*
 * There are two types message betwen modemd and modem_control
 *
 * modem control report modem state to modemd
 * 1. Modem Alive State:
 *     _TD Modem _Alive
 *     LTE Modem _Alive
 *
 * 2. Modem Assert State:
 *     _TD Modem Assert in file %s at line %d
 *     LTE Modem Assert in file %s at line %d
 *
 * 3. Modem Reset State: 
 *     LTE Modem _Reset
 *     WDG Modem _Reset
 *
 * modemd send command to modem_control
 * 1. Reload modem command
 *     LTE Modem Reload,_CSFB
 *     LTE Modem Reload,SVLTE
 */

#define  TD_MODEM_ALIVE_STR          "_TD Modem _Alive"
#define  LTE_MODEM_ALIVE_STR         "LTE Modem _Alive"
#define  GEN_MODEM_ALIVE_STR         "Modem Alive"
#define  LTE_MODEM_ASSERT_STR        "LTE Modem Assert"
#define  TD_MODEM_ASSERT_STR         "_TD Modem Assert"
#define  GEN_MODEM_ASSERT_STR         "Modem Assert"
#define  LTE_MODEM_RESET_STR         "LTE Modem _Reset"
#define  WTD_MODEM_RESET_STR         "WDG Modem _Reset"
#define  GEN_MODEM_RESET_STR         "Modem Reset"

#define  LTE_RELOAD_SVLTE_STR        "LTE Modem Reload,SVLTE"
#define  LTE_RELOAD_CSFB_STR         "LTE Modem Reload,_CSFB"

typedef struct ext_modem_ops_tag {

     void (*start_modem_service)(void);

     void (*stop_modem_service)(void);

     int  (*load_modem_image)(void);

}ext_modem_ops_t;

ext_modem_ops_t *get_ext_modem_if(void);

void  start_ext_modem(void);

int   is_external_modem(void);

#endif

