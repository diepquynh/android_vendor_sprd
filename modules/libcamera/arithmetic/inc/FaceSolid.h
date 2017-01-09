#ifndef _ACCESS_FACE_H_
#define _ACCESS_FACE_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct{
    int face_id;
    int sx;
    int sy;
    int srx;
    int sry;
    int ex;
    int ey;
    int elx;
    int ely;
    int brightness;
    int angle;
    int smile_level;
    int blink_level;
} ACCESS_FaceRect;

typedef ACCESS_FaceRect morpho_FaceRect;


int FaceSolid_Init(int width, int height, MallocFun Mfp, FreeFun Ffp);
int FaceSolid_Function(unsigned char *src, ACCESS_FaceRect ** ppDstFaces, int *pDstFaceNum ,int skip);
int FaceSolid_Finalize(FreeFun Ffp);

#ifdef __cplusplus
}
#endif

#endif
