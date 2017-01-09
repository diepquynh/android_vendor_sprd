// 
// Spreadtrum Auto Tester
//
// anli   2012-11-29
//
#ifndef _CAMERA_20121129_H__
#define _CAMERA_20121129_H__

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
//--namespace sci_cam {
//-----------------------------------------------------------------------------

#define CAM_ID_BACK   0
#define CAM_ID_FRONT  1
#define CAM_ID_ATV    5

int camOpen( int cam_id, int width, int height );

int camStart( void );

int camGetData( uchar * buffer, uint size );

int camStop( void );

int camClose( void );

int flashlightSetValue(int value);

//-----------------------------------------------------------------------------
//--};
#ifdef __cplusplus
}
#endif // __cplusplus
//-----------------------------------------------------------------------------

#endif // _CAMERA_20121129_H__
