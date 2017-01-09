/* 
    Face Attribute Recognition Library API
*/
#ifndef __SPRD_FACE_ATTRIBUTE_H__
#define __SPRD_FACE_ATTRIBUTE_H__

#if (defined( WIN32 ) || defined( WIN64 )) && (defined FARAPI_EXPORTS)
#define FAR_EXPORTS __declspec(dllexport)
#else
#define FAR_EXPORTS
#endif

#ifndef FARAPI
#define FARAPI(rettype) extern FAR_EXPORTS rettype
#endif

/* The error codes */
#define FAR_OK                       0  /* Ok!                                      */
#define FAR_ERROR_INTERNAL          -1  /* Error: Unknown internal error            */
#define FAR_ERROR_NOMEMORY			-2  /* Error: Memory allocation error           */
#define FAR_ERROR_INVALIDARG		-3  /* Error: Invalid argument                  */

/* The gray-scale image structure */
typedef struct
{
    unsigned char *data;        /* Image data                        */
    int width;                  /* Image width                       */
    int height;                 /* Image height                      */
    int step;                   /* The byte count per scan line      */
}FAR_IMAGE;

typedef struct
{
    int x, y;
}FAR_POINT;

/* The face information structure */
typedef struct
{
    FAR_POINT landmarks[7]; /* The facial landmark points. The sequence is: left-eye left corner,
                            left-eye right corner, right-eye left corner, right-eye right corner,
                            nose tip,mouth left corner, mouth right corner */
}FAR_FACEINFO;

typedef struct
{
    int smile;           /* Smile degree: smile(>0); not smile(<0); unknown(0)   */
    int eyeClose;        /* Eye open degree: open(<0); close(>0); unknown(0)     */
    int infant;          /* Infant degree: infant(>0); not infant(<0); unknown(0)*/
    int gender;          /* Gender: Male(>0); Female(<0); unknown(0)             */
}FAR_ATTRIBUTE;

typedef struct
{
    unsigned char smileOn;          /* run smile degree estimation: 1-->ON; 0-->OFF    */
    unsigned char eyeOn;            /* run eye close/open recognition: 1-->ON; 0-->OFF */
    unsigned char infantOn;         /* run infant detection: 1-->ON; 0-->OFF           */
    unsigned char genderOn;         /* run gender detection: 1-->ON; 0-->OFF           */
}FAR_OPTION;

/* The recognizer handle */
typedef void * FAR_RECOGNIZER_HANDLE;

#ifdef  __cplusplus
extern "C" {
#endif

FARAPI(const char *) FarGetVersion();

/* Create a recognizer handle */
FARAPI(int)  FarCreateRecognizerHandle(FAR_RECOGNIZER_HANDLE *hFAR);

/* Release the recognizer handle */
FARAPI(void)  FarDeleteRecognizerHandle(FAR_RECOGNIZER_HANDLE *hFAR);

/* Recognize face attributes */
FARAPI(int)  FarRecognize(FAR_RECOGNIZER_HANDLE hFAR,
                          const FAR_IMAGE *grayImage,
                          const FAR_FACEINFO *face,
                          const FAR_OPTION *option,
                          FAR_ATTRIBUTE *attribute);

#ifdef  __cplusplus
}
#endif

#endif /* __SPRD_FACE_ATTRIBUTE_H__ */
