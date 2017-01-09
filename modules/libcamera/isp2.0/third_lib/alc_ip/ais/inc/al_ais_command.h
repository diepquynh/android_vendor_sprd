
#ifndef AL_AIS_COMMAND_H
#define AL_AIS_COMMAND_H

#include	"AlAisInterface.h"
#include	"AlAisCommand.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
UI_16	_ais_cmd_rtn_cvt(
		UI_16				auiRet		);

/*=============================================================================
	AIS Core - Command functions
==============================================================================*/

#if 0
UI_16	ais_cmd_set_awb_presetwb(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_32				 puiIdx  	);
#endif
UI_16	ais_cmd_set_awb_bestshot(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiIdx  	);
#if 0
UI_16	ais_cmd_set_awb_LockMode(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiMode	 );		 //0:unlock,1;lock
#endif
UI_16	_ais_cmd_set_ae_isomode(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiMode	 );
UI_16	ais_cmd_set_ae_maxiso(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiIdx 	 );
UI_16	_ais_cmd_set_ae_Brightness(
		TT_AlAisInterface*  pptAisIf	 ,
		SI_32				 psiIdx 	 );
UI_16	ais_set_ae_BracketList_Bias(
		TT_AlAisInterface*  pptAisIf	 ,
		SQ_32				 psqBias	 );
UI_16	ais_set_ae_BracketList_Idx(
		TT_AlAisInterface *		pptAisIf	,
		SI_32					psiIdx		);
UI_16	ais_set_ae_BracketList(
		TT_AlAisInterface *		pptAisIf	,
		int 	*				ppiList		,
		UI_16	 				puiCnt		);
UI_16	_ais_cmd_set_ae_weight(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiIdx 	 );
UI_16	ais_cmd_set_ae_Framing(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiType	 );		 //	Framing Type
UI_16	ais_cmd_set_ae_evp(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiIdx 	 );
UI_16	_ais_cmd_set_ae_bestshot(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiIdx  	);
UI_16	ais_cmd_set_ae_setExpMode(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_16				 puiMode  	);		 //  Exposure Mode Index
UI_16	_ais_cmd_set_ae_setFpsCtrl(
		TT_AlAisInterface*  pptAisIf	,
		UI_16				puiMinFps  	,		 //  min fps [u08.08] format
		UI_16				puiMaxFps  	);		 //  max fps [u08.08] format
UI_16	ais_cmd_set_ae_setBacklightCorrection(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiOnOff	 );		 //0:off,1:On
UI_16	_ais_cmd_set_ae_LockMode(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiMode	 );		 //0:unlock,1;lock
UI_16	ais_cmd_set_ae_ExpSpeed(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiSpeed	 );		 //	0-100:(Slow-Fast)
UI_16	_ais_cmd_set_ae_Target(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiTarget	 );		 //	3-36[U08.00]
UI_16	_ais_cmd_set_ae_FlickerMode(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiMode	 );
UI_16	ais_cmd_set_ae_FcsSeqTrigg(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiOnOff	 );		 //0:Off,1:On
UI_16	ais_set_LedSeqTrigg(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiOnOff	 );		 //0:Off,1:On
UI_16	ais_set_PebSeqTrigg(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiOnOff	 );		 //0:Off,1:On
UI_16	ais_set_zsl_mode(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_08				 puiOnOff	 );		 //0:ff,1:On

/*===========================================================================**
for LED Sequence
**===========================================================================*/

float	ais_get_bv(
		TT_AlAisInterface*  pptAisIf	 );
void	ais_set_preflash_index(
		TT_AlAisInterface*  pptAisIf	 ,
		UI_16				 puiIndexL	 ,	/** [I ] index of Lo Ctemp LED*/
		UI_16				 puiIndexH	 );	/** [I ] index of Hi Ctemp LED*/
void	ais_set_torch_trigger(
		TT_AlAisInterface*  pptAisIf	);
UI_08	ais_get_ae_convergence_status(
		TT_AlAisInterface*  pptAisIf	);
void	ais_set_preflash_trigger(
		TT_AlAisInterface*  pptAisIf	);
UI_08	ais_get_preflash_status(
		TT_AlAisInterface*  pptAisIf	);
void	ais_get_flash_control_info(
		TT_AlAisInterface*  pptAisIf	,
		UI_16*				ppiIndexL	,	/** [ O] index of Lo Ctemp LED*/
		UI_16*				ppiIndexH	,	/** [ O] index of Hi Ctemp LED*/
		float*				ppfTime	 	);	/** [ O] flash time [sec] */
UI_32	ais_get_flash_level(
		TT_AlAisInterface*  pptAisIf	);

/*===========================================================================**

**===========================================================================*/
void	_ais_set_touch_ae(
		TT_AlAisInterface*  pptAisIf	,
		SI_32				psiLeft		,		//	i4Left	(0 ~ + resolution_info.frame_size.w ïKê{)
		SI_32				psiRight	,		//	i4Right
		SI_32				psiTop		,		//	i4Top	(0 ~ + resolution_info.frame_size.h ïKê{)
		SI_32				psiBottom	,		//	i4Bottom
		SI_32				psiFrameW	,		//		resolution_info.frame_size.w
		SI_32				psiFrameH	);		//		resolution_info.frame_size.h
void	_ais_reset_touch_ae(
		TT_AlAisInterface*  pptAisIf	);
void	_ais_set_face_detection_posi(
		TT_AlAisInterface*  pptAisIf	,
		UI_32				puiIdx		,		//	0 ~ 4/*ALAIS_MULTI_FD_MAX-1*/
		SI_32				psiLeft		,		//	i4Left	(0 ~ + resolution_info.frame_size.w ïKê{)
		SI_32				psiRight	,		//	i4Right
		SI_32				psiTop		,		//	i4Top	(0 ~ + resolution_info.frame_size.h ïKê{)
		SI_32				psiBottom	,		//	i4Bottom
		SI_32				psiFrameW	,		//		resolution_info.frame_size.w
		SI_32				psiFrameH	);		//		resolution_info.frame_size.h
UI_16	_ais_set_face_detection(
		TT_AlAisInterface*  pptAisIf	,
		UI_32				puiNum		);		//	1 ~ 5/*ALAIS_MULTI_FD_MAX*/
void	_ais_reset_face_detection(
		TT_AlAisInterface*  pptAisIf	);


#ifdef __cplusplus
}
#endif

#endif	//#ifndef AL_AIS_COMMAND_H
