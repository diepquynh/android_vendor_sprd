#ifndef __BTAPP__HCIUTILS__H__
#define __BTAPP__HCIUTILS__H__


typedef struct
{
    UINT32                          t_type;
    UINT16                          nOpCode;
}tBTAPP_HCIUTILS_NOTIFICATION_INFO;


typedef struct
{
    int                                     fenabled;           //indicates if HCIUTILS is enabled.  default is false
    tBTAPP_HCIUTILS_NOTIFICATION_INFO   *   p_notification_info;
    int                                     n_items;

} tBTAPP_HCIUTILS_CB;


extern void btapp_hciutils_init(void);
extern void btapp_hciutils_enable(void);
extern void btapp_hciutils_disable(void);
extern void btapp_hciutils_send_hci_notification(UINT16 n_op_code, void * p_buf, UINT8 len );
extern void btapp_hciutils_sendhcicommand(UINT16 n_op_code, void * p_buf, UINT8 len );
extern void btapp_hciutils_sendhcievent(UINT16 n_op_code, void * p_buf, UINT8 len );
extern void btapp_hciutils_set_afh_channels(UINT8 first, UINT8 last);
extern void btapp_hciutils_set_afh_channel_assessment(BOOLEAN enable_or_disable);
extern void btapp_hciutils_add_filter(UINT32 t_type, UINT16 n_opcode);
extern void btapp_hciutils_remove_filter(UINT32 t_type, UINT16 n_opcode);




#endif //__BTAPP__HCIUTILS__H__


