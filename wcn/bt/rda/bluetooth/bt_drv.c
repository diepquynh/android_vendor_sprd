#include "bt_vendor_lib.h"
#include "bt_rda.h"

//===============        I N T E R F A C E S      =======================

int rda_bt_init(const bt_vendor_callbacks_t* p_cb, unsigned char *local_bdaddr)
{
    DBG_PRINT("rda_bt_init\n");
    set_callbacks(p_cb);
    return 0;
}

int rda_bt_op(bt_vendor_opcode_t opcode, void *param)
{
    int ret = 0;
    
    switch(opcode) 
    {
    	case BT_VND_OP_POWER_CTRL:
    	{
    	    DBG_PRINT("BT_VND_OP_POWER_CTRL\n");
    	    ret = set_bluetooth_power(*((int*)param));
    	    break;
	    }
    	    
    	case BT_VND_OP_USERIAL_OPEN:
    	{
    	    DBG_PRINT("BT_VND_OP_USERIAL_OPEN\n");
    	    
    	    ((int*)param)[0] = init_uart();
    	    ret = 1; // CMD/EVT/ACL-In/ACL-Out via the same fd
    	    break;
	    }
    	    
    	case BT_VND_OP_USERIAL_CLOSE:
    	{
    	    DBG_PRINT("BT_VND_OP_USERIAL_CLOSE\n");
    	    close_uart();
    	    break;
	    }
    	    
    	case BT_VND_OP_FW_CFG:
    	{
    	    DBG_PRINT("BT_VND_OP_FW_CFG\n");
    	    ret = rda_fw_cfg();
    	    break;
	    }
    	    
    	case BT_VND_OP_SCO_CFG:
    	{
    	    DBG_PRINT("BT_VND_OP_SCO_CFG\n");
    	    ret = rda_sco_cfg();
    	    break;
	    }
    	    
    	case BT_VND_OP_GET_LPM_IDLE_TIMEOUT:
    	{
    	    DBG_PRINT("BT_VND_OP_GET_LPM_IDLE_TIMEOUT\n");
    	    *((uint32_t*)param) = 3000; //ms
    	    break;
	    }
    	    
    	case BT_VND_OP_LPM_SET_MODE:
    	{
    	    DBG_PRINT("BT_VND_OP_LPM_SET_MODE\n");
    	    ret = rda_sleep_cfg();
    	    break;
	    }
    	    
    	case BT_VND_OP_LPM_WAKE_SET_STATE:
		{
            uint8_t *state = (uint8_t *) param;
            
            uint8_t wake_assert = (*state == BT_VND_LPM_WAKE_ASSERT) ?  TRUE : FALSE;

    	    ret = rda_wake_chip(wake_assert);
    	    break;
        }
    	    
    	default:
    	{
    	    ERR_PRINT("Unknown operation %d\n", opcode);
    	    break;
	    }
    }
    
    return ret;
}

void rda_bt_cleanup()
{
    clean_callbacks();
    return;
}

const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
    sizeof(bt_vendor_interface_t),
    rda_bt_init,
    rda_bt_op,
    rda_bt_cleanup
};



