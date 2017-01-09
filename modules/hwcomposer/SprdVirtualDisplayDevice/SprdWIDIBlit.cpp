/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          Module              DESCRIPTION                             *
 ** 16/06/2014    Hardware Composer   Responsible for processing some         *
 **                                   Hardware layers. These layers comply    *
 **                                   with Virtual Display specification,     *
 **                                   can be displayed directly, bypass       *
 **                                   SurfaceFligner composition. It will     *
 **                                   improve system performance.             *
 ******************************************************************************
 ** File:SprdWIDIBlit.cpp             DESCRIPTION                             *
 **                                   WIDIBLIT: Wireless Display Blit         *
 **                                   Responsible for blit image data to      *
 **                                   Virtual Display.                        *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#include "MemoryHeapIon.h"
#include "SprdWIDIBlit.h"
#include "../SprdHWLayer.h"
#include "../SprdPrimaryDisplayDevice/SprdFrameBufferHAL.h"
#include "../SprdTrace.h"

#include "EGLUtils.h"

namespace android
{


SprdWIDIBlit:: SprdWIDIBlit(SprdVirtualPlane *plane)
    : mDisplayPlane(plane),
      mAccelerator(0),
      mFBInfo(0),
      mDebugFlag(0)
{

}

SprdWIDIBlit:: ~SprdWIDIBlit()
{
    if (mFBInfo)
    {
        free(mFBInfo);
        mFBInfo = NULL;
    }

#ifdef HWC_USE_GXP_BLIT
    if (mAccelerator)
    {
        delete mAccelerator;
        mAccelerator = NULL;
    }
#else
    destoryGraphics();
#endif
}

void SprdWIDIBlit:: onStart()
{
    sem_post(&startSem);
}

void SprdWIDIBlit:: onDisplay()
{
    HWC_TRACE_CALL;
    sem_wait(&doneSem);
}

int queryBlit()
{

    return 0;
}


/*
 *  Private interface
 * */
void SprdWIDIBlit:: onFirstRef()
{
    run("SprdWIDIBlit", PRIORITY_URGENT_DISPLAY + PRIORITY_MORE_FAVORABLE);
}

status_t SprdWIDIBlit:: readyToRun()
{
    sem_init(&startSem, 0, 0);
    sem_init(&doneSem, 0, 0);

    if (mDisplayPlane == NULL)
    {
        ALOGE("SprdWIDIBlit:: readyToRun mDisplayPlane is NULL");
        return -1;
    }

#ifdef HWC_USE_GXP_BLIT
    mFBInfo = (FrameBufferInfo *)malloc(sizeof(FrameBufferInfo));
    if (mFBInfo == NULL)
    {
        ALOGE("malloc FrameBufferInfo failed");
        return -1;
    }
    mAccelerator = new SprdUtil(mFBInfo);
    if (mAccelerator == NULL)
    {
        free(mFBInfo);
        ALOGE("new SprdUtil failed");
        return -1;
    }
    mAccelerator->getGSPCapability(NULL);
    mAccelerator->forceUpdateAddrType(GSP_ADDR_TYPE_PHYSICAL);
#else
    setupGraphics();
#endif

    return NO_ERROR;
}

bool SprdWIDIBlit:: threadLoop()
{
    sem_wait(&startSem);

    int ret = -1;
    bool SourcePhyAddrType = -1;
    bool DestPhyAddrType = -1;
    SprdHWLayer *SprdHWSourceLayer = NULL;
    private_handle_t *DisplayHandle = NULL;
    struct sprdRect *SourceRect = NULL;
    unsigned int width = 0;
    unsigned int height = 0;
    int format = -1;
    size_t size = 0;
    size_t size2 = 0;


    HWC_TRACE_BEGIN_WIDIBLIT;

    DisplayHandle = mDisplayPlane->dequeueBuffer();
    if (DisplayHandle == NULL)
    {
        ALOGE("SprdWIDIBlit:: threadLoop DisplayHanle is NULL");
        sem_post(&doneSem);
        return true;
    }


    mDisplayPlane->getPlaneGeometry(&width, &height, &format);
#if HWC_USE_GXP_BLIT
    if (mFBInfo == NULL)
    {
        ALOGE("SprdWIDIBlit:: threadLoop mFBInfo is NULL");
        sem_post(&doneSem);
        return true;
    }

    mFBInfo->fb_width = width;
    mFBInfo->fb_height = height;
    mFBInfo->format = format;

    mAccelerator->UpdateFBInfo(mFBInfo);
#endif

    SprdHWSourceLayer = mDisplayPlane->getSprdHWSourceLayer();
    if (SprdHWSourceLayer == NULL)
    {
        ALOGE("SprdWIDIBlit:: threadLoop SprdHWSourceLayer is NULL");
        sem_post(&doneSem);
        return true;
    }

    hwc_layer_1_t *AndroidLayer = SprdHWSourceLayer->getAndroidLayer();
    if (AndroidLayer == NULL)
    {
        ALOGE("SprdWIDIBlit:: threadLoop FBTLayer is NULL");
        sem_post(&doneSem);
        return true;
    }
    struct private_handle_t *privateH = (struct private_handle_t *)AndroidLayer->handle;
    if (privateH == NULL)
    {
        ALOGE("SprdWIDIBlit:: threadLoop private handle is NULL");
        sem_post(&doneSem);
        return true;
    }

#if HWC_USE_GXP_BLIT
    SourcePhyAddrType = (privateH->flags & private_handle_t::PRIV_FLAGS_USES_PHY);

    DestPhyAddrType = (DisplayHandle->flags & private_handle_t::PRIV_FLAGS_USES_PHY);

    ALOGI_IF(mDebugFlag, "SprdWIDIBlit:: threadLoop source handle addr: %p, flag: 0x%x, dest handle addr: %p, flag: 0x%x", (void *)privateH, privateH->flags, (void *)DisplayHandle, DisplayHandle->flags);
    if (SourcePhyAddrType && DestPhyAddrType)
    {
        MemoryHeapIon::Get_phy_addr_from_ion(DisplayHandle->share_fd, &(DisplayHandle->phyaddr), &size);
        MemoryHeapIon::Get_phy_addr_from_ion(privateH->share_fd, &(privateH->phyaddr), &size2);

        mAccelerator->UpdateOutputFormat(GSP_DST_FMT_YUV420_2P);

        if (SprdHWSourceLayer->checkYUVLayerFormat())
        {
            ret = mAccelerator->composerLayers(SprdHWSourceLayer, NULL, NULL, DisplayHandle, GSP_DST_FMT_MAX_NUM);
        }
        else if (SprdHWSourceLayer->checkRGBLayerFormat())
        {
            ret = mAccelerator->composerLayers(NULL, SprdHWSourceLayer, NULL, DisplayHandle, GSP_DST_FMT_MAX_NUM);
        }
    }
    else
    {
        ALOGI_IF(mDebugFlag, "SprdWIDIBlit:: threadLoop Source(SourcePhyAddrType: %d) or Dest(DestPhyAddrType: %d) do not use ION_PhyAddr, will Use NEON to Blit", SourcePhyAddrType, DestPhyAddrType);
        if ((void *)(privateH->base) == NULL || (void *)(DisplayHandle->base) == NULL)
        {
            ALOGE("SprdWIDIBlit:: threadLoop Source virtual address: %p or Dest virtual addr: %p is NULL",
                  (void *)(privateH->base), (void *)(DisplayHandle->base));
            sem_post(&doneSem);
            return true;
        }

        /*
         *  Blit with NEON
         * */
        /*
         *  Source Information
         * */
        uint8_t *inrgb = (uint8_t *)(privateH->base);
        int32_t width_org = privateH->width;
        int32_t height_org = privateH->height;

        /*
         *  Destination Information
         * */
        uint8_t *outy = (uint8_t *)(DisplayHandle->base);
        uint8_t *outuv = (uint8_t *)(DisplayHandle->base + DisplayHandle->stride * DisplayHandle->height);
        int32_t width_dst = DisplayHandle->width;
        int32_t height_dst = DisplayHandle->height;

        ret = NEONBlit(inrgb, outy, outuv, width_org, height_org, width_dst, height_dst);
    }
#else
    sp<GraphicBuffer> Source;
    sp<GraphicBuffer> Target;
    ret = setupYuvTexSurface(AndroidLayer, DisplayHandle, Source, Target);
    ret = renderImage(Source, Target);

    //int dumpFlag = -1;
    //queryDumpFlag(&dumpFlag);
    //if (dumpFlag & HWCOMPOSER_DUMP_VD_OVERLAY_FLAG)
    //{
    //    dumpOverlayImage(privateH, "VDSource");
    //}

    Source = 0;
    Target = 0;
#endif

    if (ret != 0)
    {
        ALOGE("SprdWIDIBlit:: threadLoop Accelerator composerLayers failed");
        //return true;
    }

    mDisplayPlane->queueBuffer();

    queryDebugFlag(&mDebugFlag);
    ALOGI_IF(mDebugFlag, "SprdWIDIBlit Source Layer width: %d, height: %d, format: %d, Destination width: %d, heigh: %d, format: %d",
             privateH->width, privateH->height, privateH->format,
             DisplayHandle->width, DisplayHandle->height, DisplayHandle->format);

    sem_post(&doneSem);

    HWC_TRACE_END;

    return true;
}

int SprdWIDIBlit:: NEONBlit(uint8_t *inrgb, uint8_t *outy, uint8_t *outuv, int32_t width_org, int32_t height_org, int32_t width_dst, int32_t height_dst)
{
    HWC_TRACE_CALL;
    if (inrgb == NULL || outy == NULL || outuv == NULL)
    {
        ALOGE("SprdWIDIBlit:: NEONBlit input is NULL");
        return -1;
    }
    uint32_t i, j;
    uint8_t *argb_ptr = inrgb;
    uint8_t *y_ptr = outy;
    uint8_t *temp_y_ptr = y_ptr;
    uint8_t *uv_ptr = outuv;
    uint8_t *argb_tmpptr;
    uint8x8_t r1fac = vdup_n_u8(66);

    uint8x8_t g1fac = vdup_n_u8(129);
    ///////// uint8x8_t g11fac = vdup_n_u8(1);   ///////128+1 =129

    uint8x8_t b1fac = vdup_n_u8(25);
    uint8x8_t r2fac = vdup_n_u8(38);
    uint8x8_t g2fac = vdup_n_u8(74);
    uint8x8_t b2fac = vdup_n_u8(112);
    // int8x8_t r3fac = vdup_n_s16(112);
    uint8x8_t g3fac = vdup_n_u8(94);
    uint8x8_t b3fac = vdup_n_u8(18);

    uint8x8_t y_base = vdup_n_u8(16);
    uint8x8_t uv_base = vdup_n_u8(128);


    for (i=height_org; i>0; i-=2)    /////  line
    {
       for (j=(width_org>>3); j>0; j-=2)   ///// col
       {
           uint8_t y, cb, cr;
           int8_t r, g, b;
           uint8_t p_r[16],p_g[16],p_b[16];
           uint16x8_t temp;
           uint8x8_t result;
           uint8x8_t result_cr;
           uint8x8x2_t result_uv;


           // y = RGB2Y(r, g, b);
           uint8x8x4_t argb = vld4_u8(argb_ptr);
           temp = vmull_u8(argb.val[0],r1fac);    ///////////////////////y  0,1,2
           temp = vmlal_u8(temp,argb.val[1],g1fac);
           temp = vmlal_u8(temp,argb.val[2],b1fac);
           result = vshrn_n_u16(temp,8);
           result = vadd_u8(result,y_base);
           vst1_u8(y_ptr,result);     ////*y_ptr = y;


           argb_tmpptr= argb_ptr + 32;
           temp_y_ptr = y_ptr + 8;
           uint8x8x4_t argb1 = vld4_u8(argb_tmpptr);
           // y = RGB2Y(r, g, b);
           temp = vmull_u8(argb1.val[0],r1fac);    ///////////////////////y
           temp = vmlal_u8(temp,argb1.val[1],g1fac);
           temp = vmlal_u8(temp,argb1.val[2],b1fac);
           result = vshrn_n_u16(temp,8);
           result = vadd_u8(result,y_base);
           vst1_u8(temp_y_ptr,result);     ////*y_ptr = y;

           vst1_u8(p_r,argb.val[0]);
           vst1_u8(p_r+8,argb1.val[0]);
           vst1_u8(p_g,argb.val[1]);
           vst1_u8(p_g+8,argb1.val[1]);
           vst1_u8(p_b,argb.val[2]);
           vst1_u8(p_b+8,argb1.val[2]);
           uint8x8x2_t rgb_r = vld2_u8(p_r);
           uint8x8x2_t rgb_g = vld2_u8(p_g);
           uint8x8x2_t rgb_b = vld2_u8(p_b);

           //cb = RGB2CR(r, g, b);
           temp = vmull_u8(rgb_b.val[0],b2fac);    ///////////////////////cb
           temp = vmlsl_u8(temp,rgb_g.val[0],g2fac);
           temp = vmlsl_u8(temp,rgb_r.val[0],r2fac);
           result = vshrn_n_u16(temp,8);
           result = vadd_u8(result,uv_base);

           //cr = RGB2CB(r, g, b);
           temp = vmull_u8(rgb_r.val[0],b2fac);    ///////////////////////cr
           temp = vmlsl_u8(temp,rgb_g.val[0],g3fac);
           temp = vmlsl_u8(temp,rgb_b.val[0],b3fac);
           result_cr = vshrn_n_u16(temp,8);
           result_cr = vadd_u8(result_cr,uv_base);

           result_uv = vzip_u8(result_cr,result);  /////uuuuuuuuvvvvvvvv -->> uvuvuvuvuvuvuvuvuv
           vst1_u8(uv_ptr,result_uv.val[0]);
           uv_ptr += 8;
           vst1_u8(uv_ptr,result_uv.val[1]);
           uv_ptr += 8;

           argb_tmpptr= argb_ptr + (width_org<<2);
           temp_y_ptr = y_ptr + width_dst;
           argb = vld4_u8(argb_tmpptr);

           // y = RGB2Y(r, g, b);
           temp = vmull_u8(argb.val[0],r1fac);    ///////////////////////y
           temp = vmlal_u8(temp,argb.val[1],g1fac);
           temp = vmlal_u8(temp,argb.val[2],b1fac);
           result = vshrn_n_u16(temp,8);
           result = vadd_u8(result,y_base);
           vst1_u8(temp_y_ptr,result);   ////*y_ptr = y;


           argb_tmpptr =argb_ptr +( width_org<<2)+32;
           temp_y_ptr = y_ptr + width_dst + 8;
           argb = vld4_u8(argb_tmpptr);

           // y = RGB2Y(r, g, b);
           temp = vmull_u8(argb.val[0],r1fac);    ///////////////////////y
           temp = vmlal_u8(temp,argb.val[1],g1fac);
           temp = vmlal_u8(temp,argb.val[2],b1fac);
           result = vshrn_n_u16(temp,8);
           result = vadd_u8(result,y_base);
           vst1_u8(temp_y_ptr,result);     ////*y_ptr = y;

           y_ptr += 16;
           argb_ptr += 64;
       }

       y_ptr += width_dst;
       argb_ptr += width_org<<2;
    }

    return 0;
}



#define DebugGFX 0

GLuint gProgramY;
GLuint gProgramUV;
GLuint gProgramRGB;
GLuint gProgramY_YUVSource;
GLuint gProgramUV_YUVSource;
GLuint gProgramY_YUVSource_Clear;
GLuint gProgramUV_YUVSource_Clear;
GLint iLocPosition = 0;
GLint iLocTexcoord = 1;
GLint iLocTexSampler = 0;
GLint iLocTexMatrixY = 0;
GLint iLocTexMatrixUV = 0;
GLint iLocTexMatrixYSource = 0;
GLint iLocTexMatrixUVSource = 0;
GLint iLocTexMatrixRGB = 0;

/*
 *  Matrix for ratation.
 * */
static float mtxIdentity[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
};
static float mtxFlipH[16] = {
    -1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    1, 0, 0, 1,
};
static float mtxFlipV[16] = {
    1, 0, 0, 0,
    0, -1, 0, 0,
    0, 0, 1, 0,
    0, 1, 0, 1,
};
static float mtxRot90[16] = {
    0, -1, 0, 0,
    1, 0, 0, 0,
    0, 0, 1, 0,
    0, 1, 0, 1,
};
static float mtxRot270[16] = {
    0, 1, 0, 0,
    -1, 0, 0, 0,
    0, 0, 1, 0,
    1, 0, 0, 1,
};
static float gTexMatrix[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
};

#define Y_FIRST 1

#define EGL_IMAGE_FORMAT_SPRD 0x8800
#define EGL_IMAGE_OFFSET_SPRD 0x8801
#define EGL_IMAGE_WIDTH_SPRD  0x8802
#define EGL_IMAGE_HEIGHT_SPRD 0x8803
#define EGL_IMAGE_FORMAT_L8_SPRD   0x1
#define EGL_IMAGE_FORMAT_A8L8_SPRD 0x2


GLint tex_src=1;
GLint tex_src_uv = 2;
GLint tex_dst_y=3, tex_dst_uv=4;
GLint tex_dst_rgb = 5;
GLint fbo_y=1,     fbo_uv=2;
GLint fbo_rgb = 3;

EGLImageKHR img_src, img_src_uv, img_dstY, img_dstUV, img_dstRGB;

bool YUVToYUV = false;
bool TargetOutputRGB = false;

static struct sprdRect TargetRegion;

static const char gVertexShader[] = "attribute vec4 aPosition;\n"
    "attribute vec4 aTexCoords;                               \n"
    "uniform mat4 textureMatrix;                              \n"
    "varying vec2 vTexCoords;                                 \n"
    "void main() {                                            \n"
    "  vTexCoords = (textureMatrix * aTexCoords).st;          \n"
    "  gl_Position = aPosition;               \n"
    "}                                                        \n";

static const char gVertexShader_Clear[] = "attribute vec4 aPosition;\n"
    "void main() {                                            \n"
    "  gl_Position = aPosition;                               \n"
    "}                                                        \n";

static const char gFragmentShaderY[] =
    "#extension GL_OES_EGL_image_external : require               \n"
    "precision mediump float;                                     \n"
    "uniform sampler2D uTexSampler;                               \n"
    "varying vec2 vTexCoords;                                     \n"
    "void main() {                                                \n"
    "  vec4 color = texture2D(uTexSampler, vTexCoords);           \n"
    "  gl_FragColor.r = 0.2578*color.r+0.5039*color.g+0.0977*color.b + 0.0625;   \n"
    "}                                                            \n";

static const char gFragmentShaderRGB[] =
    "#extension GL_OES_EGL_image_external : require               \n"
    "precision mediump float;                                     \n"
    "uniform sampler2D uTexSampler;                               \n"
    "varying vec2 vTexCoords;                                     \n"
    "void main() {                                                \n"
    "  gl_FragColor = texture2D(uTexSampler, vTexCoords);           \n"
    "}                                                            \n";

static const char gFragmentShaderUV[] =
    "#extension GL_OES_EGL_image_external : require               \n"
    "precision mediump float;                                     \n"
    "uniform sampler2D uTexSampler;                               \n"
    "varying vec2 vTexCoords;                                     \n"
    "void main() {                                                \n"
    "  vec4 color = texture2D(uTexSampler, vTexCoords);           \n"
    "  gl_FragColor.r = -0.1484*color.r - 0.2930 * color.g + 0.4375 * color.b + 0.5;\n"  //u
    "  gl_FragColor.a = 0.4375*color.r - 0.3672 * color.g - 0.0703 * color.b + 0.5; \n"  //v
    "}                                                            \n";

static const char gFragmentShaderY_YUVSource[] =
    "#extension GL_OES_EGL_image_external : require               \n"
    "precision mediump float;                                     \n"
    "uniform sampler2D uTexSampler;                               \n"
    "varying vec2 vTexCoords;                                     \n"
    "void main() {                                                \n"
    "  vec4 color = texture2D(uTexSampler, vTexCoords);           \n"
    "  gl_FragColor.r = color.r;                                  \n"
    "}                                                            \n";

static const char gFragmentShaderUV_YUVSource[] =
    "#extension GL_OES_EGL_image_external : require               \n"
    "precision mediump float;                                     \n"
    "uniform sampler2D uTexSampler;                               \n"
    "varying vec2 vTexCoords;                                     \n"
    "void main() {                                                \n"
    "  vec4 color = texture2D(uTexSampler, vTexCoords);           \n"
    "  gl_FragColor.r = color.a;                                  \n"
    "  gl_FragColor.a = color.r;                                  \n"
    "}                                                            \n";

static const char gFragmentShaderY_YUVSource_Clear[] =
    "#extension GL_OES_EGL_image_external : require               \n"
    "precision mediump float;                                     \n"
    "void main() {                                                \n"
    "  gl_FragColor.r = 0.0625;                                  \n"
    "}                                                            \n";

static const char gFragmentShaderUV_YUVSource_Clear[] =
    "#extension GL_OES_EGL_image_external : require               \n"
    "precision mediump float;                                     \n"
    "void main() {                                                \n"
    "  gl_FragColor.r = 0.5;                                  \n"
    "  gl_FragColor.a = 0.5;                                  \n"
    "}                                                            \n";

static void mtxMul(float out[16], const float a[16], const float b[16]) {
    out[0] = a[0]*b[0] + a[4]*b[1] + a[8]*b[2] + a[12]*b[3];
    out[1] = a[1]*b[0] + a[5]*b[1] + a[9]*b[2] + a[13]*b[3];
    out[2] = a[2]*b[0] + a[6]*b[1] + a[10]*b[2] + a[14]*b[3];
    out[3] = a[3]*b[0] + a[7]*b[1] + a[11]*b[2] + a[15]*b[3];

    out[4] = a[0]*b[4] + a[4]*b[5] + a[8]*b[6] + a[12]*b[7];
    out[5] = a[1]*b[4] + a[5]*b[5] + a[9]*b[6] + a[13]*b[7];
    out[6] = a[2]*b[4] + a[6]*b[5] + a[10]*b[6] + a[14]*b[7];
    out[7] = a[3]*b[4] + a[7]*b[5] + a[11]*b[6] + a[15]*b[7];

    out[8] = a[0]*b[8] + a[4]*b[9] + a[8]*b[10] + a[12]*b[11];
    out[9] = a[1]*b[8] + a[5]*b[9] + a[9]*b[10] + a[13]*b[11];
    out[10] = a[2]*b[8] + a[6]*b[9] + a[10]*b[10] + a[14]*b[11];
    out[11] = a[3]*b[8] + a[7]*b[9] + a[11]*b[10] + a[15]*b[11];

    out[12] = a[0]*b[12] + a[4]*b[13] + a[8]*b[14] + a[12]*b[15];
    out[13] = a[1]*b[12] + a[5]*b[13] + a[9]*b[14] + a[13]*b[15];
    out[14] = a[2]*b[12] + a[6]*b[13] + a[10]*b[14] + a[14]*b[15];
    out[15] = a[3]*b[12] + a[7]*b[13] + a[11]*b[14] + a[15]*b[15];
}

void SprdWIDIBlit:: printGLString(const char *name, GLenum s)
{
    const char *v = (const char *) glGetString(s);

    ALOGI("GL %s = %s\n", name, v);
}

void SprdWIDIBlit:: printEGLConfiguration(EGLDisplay dpy, EGLConfig config)
{
#define X(VAL) {VAL, #VAL}
    struct {EGLint attribute; const char* name;} names[] = {
    X(EGL_BUFFER_SIZE),
    X(EGL_ALPHA_SIZE),
    X(EGL_BLUE_SIZE),
    X(EGL_GREEN_SIZE),
    X(EGL_RED_SIZE),
    X(EGL_DEPTH_SIZE),
    X(EGL_STENCIL_SIZE),
    X(EGL_CONFIG_CAVEAT),
    X(EGL_CONFIG_ID),
    X(EGL_LEVEL),
    X(EGL_MAX_PBUFFER_HEIGHT),
    X(EGL_MAX_PBUFFER_PIXELS),
    X(EGL_MAX_PBUFFER_WIDTH),
    X(EGL_NATIVE_RENDERABLE),
    X(EGL_NATIVE_VISUAL_ID),
    X(EGL_NATIVE_VISUAL_TYPE),
    X(EGL_SAMPLES),
    X(EGL_SAMPLE_BUFFERS),
    X(EGL_SURFACE_TYPE),
    X(EGL_TRANSPARENT_TYPE),
    X(EGL_TRANSPARENT_RED_VALUE),
    X(EGL_TRANSPARENT_GREEN_VALUE),
    X(EGL_TRANSPARENT_BLUE_VALUE),
    X(EGL_BIND_TO_TEXTURE_RGB),
    X(EGL_BIND_TO_TEXTURE_RGBA),
    X(EGL_MIN_SWAP_INTERVAL),
    X(EGL_MAX_SWAP_INTERVAL),
    X(EGL_LUMINANCE_SIZE),
    X(EGL_ALPHA_MASK_SIZE),
    X(EGL_COLOR_BUFFER_TYPE),
    X(EGL_RENDERABLE_TYPE),
    X(EGL_CONFORMANT),
   };
#undef X

    for (size_t j = 0; j < sizeof(names) / sizeof(names[0]); j++) {
        EGLint value = -1;
        EGLint returnVal = eglGetConfigAttrib(dpy, config, names[j].attribute, &value);
        EGLint error = eglGetError();
        if (returnVal && error == EGL_SUCCESS) {
            ALOGI(" %s: ", names[j].name);
            ALOGI("%d (0x%x)", value, value);
        }
    }
    ALOGI("\n");
}

void SprdWIDIBlit:: checkEglError(const char* op, EGLBoolean returnVal)
{
    if (returnVal != EGL_TRUE) {
        ALOGE("%s() returned %d\n", op, returnVal);
    }

    for (EGLint error = eglGetError(); error != EGL_SUCCESS; error
            = eglGetError()) {
         ALOGE("after %s() eglError %s (0x%x)\n", op, EGLUtils::strerror(error),
                error);
    }
}

void SprdWIDIBlit:: checkGlError(const char* op)
{
    for (GLint error = glGetError(); error; error
            = glGetError()) {
         ALOGE("after %s() glError (0x%x)\n", op, error);
    }
}

GLuint SprdWIDIBlit:: loadShader(GLenum shaderType, const char* pSource)
{
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    ALOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
            } else {
                ALOGE("Guessing at GL_INFO_LOG_LENGTH size\n");
                char* buf = (char*) malloc(0x1000);
                if (buf) {
                    glGetShaderInfoLog(shader, 0x1000, NULL, buf);
                    ALOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
            }
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}


GLuint SprdWIDIBlit:: createProgram(const char* pVertexSource, const char* pFragmentSource)
{
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    ALOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

sp<GraphicBuffer> SprdWIDIBlit:: wrapGraphicsBuffer(private_handle_t *handle)
{
    sp<GraphicBuffer> GFXBuffer = NULL;

    if (handle == NULL)
    {
        ALOGE("SprdWIDIBlit:: wrapGraphicsBuffer handle is NULL");
        return NULL;
    }

    GFXBuffer = new GraphicBuffer(handle->width, handle->height, handle->format,
                               GraphicBuffer::USAGE_HW_TEXTURE, handle->stride,
                               (native_handle_t *)handle, false);
    if (GFXBuffer->initCheck() != NO_ERROR)
    {
        ALOGE("SprdWIDIBlit:: wrapGraphicsBuffer alloc GraphicBuffer failed");
        return NULL;
    }

    return GFXBuffer;
}

int SprdWIDIBlit:: setupGraphics()
{
    /*
     *  Init EGL ENV
     * */
    EGLBoolean returnValue;
    EGLConfig Configs =  { 0 };
    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
    };

    EGLint s_configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_NONE, 0,
        EGL_NONE
    };
    EGLint majorVersion;
    EGLint minorVersion;
    EGLContext context;
    EGLSurface surface;
    int format = HAL_PIXEL_FORMAT_RGBA_8888;

    EGLDisplay dpy;

    checkEglError("<ini>");

    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    checkEglError("eglGetDisplay");
    if (dpy == EGL_NO_DISPLAY)
    {
        ALOGE("SprdWIDIBlit:: setupGraphics eglGetDisplay returned EGL_NO_DISPLAY");
        goto err1;
    }

    returnValue = eglInitialize(dpy, &majorVersion, &minorVersion);
    checkEglError("eglInitialize", returnValue);
    ALOGI_IF(DebugGFX, "EGL version %d.%d\n", majorVersion, minorVersion);
    if (returnValue != EGL_TRUE) {
        ALOGE("eglInitialize failed\n");
        goto err1;
    }

    returnValue = EGLUtils::selectConfigForPixelFormat(dpy, s_configAttribs, format, &Configs);
    if (returnValue)
    {
        ALOGE("SprdWIDIBlit:: setupGraphics EGLUtils::selectConfigForPixelFormat return %d", returnValue);
        goto err1;
    }
    checkEglError("EGLUtils::selectConfigForPixelFormat");

    if (DebugGFX)
    {
        ALOGI_IF(DebugGFX, "Choose this configuration:");
        printEGLConfiguration(dpy, Configs);
    }

    surface = eglCreatePbufferSurface(dpy, Configs, NULL);
    checkEglError("eglCreatePbufferSurface");
    if (surface == EGL_NO_SURFACE)
    {
        ALOGE("SprdWIDIBlit:: setupGraphics eglCreatePbufferSurface EGL_NO_SURFACE");
        goto err1;
    }

    context = eglCreateContext(dpy, Configs, EGL_NO_CONTEXT, context_attribs);
    checkEglError("eglCreateContext");
    if (context == EGL_NO_CONTEXT)
    {
        ALOGE("SprdWIDIBlit:: setupGraphics eglCreateContext failed");
        goto err2;
    }

    returnValue = eglMakeCurrent(dpy, surface, surface, context);
    checkEglError("eglMakeCurrent", returnValue);
    if (returnValue != EGL_TRUE)
    {
        ALOGE("SprdWIDIBlit:: setupGraphics eglMakeCurrent failed");
        goto err3;
    }

    /*
     *  Init OpenGLSL
     * */
    gProgramY = createProgram(gVertexShader, gFragmentShaderY);
    if (!gProgramY) {
        ALOGE("SprdWIDIBlit:: setupGraphics createProgram gFragmentShaderY failed");
        goto err4;
    }

    iLocPosition = glGetAttribLocation(gProgramY, "aPosition");
    iLocTexcoord = glGetAttribLocation(gProgramY, "aTexcoord");
    checkGlError("glGetAttribLocation");
    iLocTexMatrixY = glGetUniformLocation(gProgramY, "textureMatrix");
    checkGlError("glGetUniformLocation");
    ALOGI_IF(DebugGFX, "aPosition, aTexcoord = %d, %d, iLocTexMatrixY: %d",
             iLocPosition, iLocTexcoord, iLocTexMatrixY);

    iLocTexSampler = glGetUniformLocation(gProgramY, "uTexSampler");
    checkGlError("glGetUniformLocation");
    ALOGI_IF(DebugGFX, "uTexSampler = %d\n", iLocTexSampler);

    glUseProgram(gProgramY);
    glUniform1i(iLocTexSampler, 0);

    gProgramUV = createProgram(gVertexShader, gFragmentShaderUV);
    if (!gProgramUV) {
        ALOGE("SprdWIDIBlit:: setupGraphics createProgram gFragmentShaderUV failed");
        goto err5;
    }

    iLocPosition = glGetAttribLocation(gProgramUV, "aPosition");
    iLocTexcoord = glGetAttribLocation(gProgramUV, "aTexcoord");
    checkGlError("glGetAttribLocation");
    iLocTexMatrixUV = glGetUniformLocation(gProgramUV, "textureMatrix");
    checkGlError("glGetUniformLocation");
    ALOGI_IF(DebugGFX, "UV aPosition, aTexcoord = %d, %d, iLocTexMatrixUV: %d",
             iLocPosition, iLocTexcoord, iLocTexMatrixUV);

    iLocTexSampler = glGetUniformLocation(gProgramUV, "uTexSampler");
    checkGlError("glGetUniformLocation");
    ALOGI_IF(DebugGFX, "uTexSampler = %d\n", iLocTexSampler);

    glUseProgram(gProgramUV);
    glUniform1i(iLocTexSampler, 0);


    /*
     *  For Source YUV.
     * */
    gProgramY_YUVSource = createProgram(gVertexShader, gFragmentShaderY_YUVSource);
    if (!gProgramY_YUVSource) {
        ALOGE("SprdWIDIBlit:: setupGraphics createProgram gFragmentShaderY_YUVSource failed");
        goto err6;
    }

    iLocPosition = glGetAttribLocation(gProgramY_YUVSource, "aPosition");
    iLocTexcoord = glGetAttribLocation(gProgramY_YUVSource, "aTexcoord");
    checkGlError("glGetAttribLocation");
    iLocTexMatrixYSource = glGetUniformLocation(gProgramY_YUVSource, "textureMatrix");
    checkGlError("glGetUniformLocation");
    ALOGI_IF(DebugGFX, "YSource aPosition, aTexcoord = %d, %d, iLocTexMatrixYSource: %d",
             iLocPosition, iLocTexcoord, iLocTexMatrixYSource);

    iLocTexSampler = glGetUniformLocation(gProgramY_YUVSource, "uTexSampler");
    checkGlError("glGetUniformLocation");
    ALOGI_IF(DebugGFX, "uTexSampler = %d\n", iLocTexSampler);

    glUseProgram(gProgramY_YUVSource);
    glUniform1i(iLocTexSampler, 0);

    gProgramUV_YUVSource = createProgram(gVertexShader, gFragmentShaderUV_YUVSource);
    if (!gProgramUV_YUVSource) {
        ALOGE("SprdWIDIBlit:: setupGraphics createProgram gFragmentShaderUV_YUVSource failed");
        goto err7;
    }

    iLocPosition = glGetAttribLocation(gProgramUV_YUVSource, "aPosition");
    iLocTexcoord = glGetAttribLocation(gProgramUV_YUVSource, "aTexcoord");
    checkGlError("glGetAttribLocation");
    iLocTexMatrixUVSource = glGetUniformLocation(gProgramUV_YUVSource, "textureMatrix");
    checkGlError("glGetUniformLocation");
    ALOGI_IF(DebugGFX, "UVSource aPosition, aTexcoord = %d, %d, iLocTexMatrixUVSource: %d",
             iLocPosition, iLocTexcoord, iLocTexMatrixUVSource);

    iLocTexSampler = glGetUniformLocation(gProgramUV_YUVSource, "uTexSampler");
    checkGlError("glGetUniformLocation");
    ALOGI_IF(DebugGFX, "uTexSampler = %d\n", iLocTexSampler);

    glUseProgram(gProgramUV_YUVSource);
    glUniform1i(iLocTexSampler, 0);

    /*
     *  For clear screen shader
     * */
    gProgramY_YUVSource_Clear = createProgram(gVertexShader_Clear, gFragmentShaderY_YUVSource_Clear);
    if (!gProgramY_YUVSource_Clear)
    {
        ALOGE("SprdWIDIBlit:: setupGraphics createProgram gProgramY_YUVSource_Clear failed");
        goto err8;
    }
    iLocPosition = glGetAttribLocation(gProgramY_YUVSource_Clear, "aPosition");
    checkGlError("glGetAttribLocation");
    ALOGI_IF(DebugGFX, "Y Clear aPosition = %d", iLocPosition);
    glUseProgram(gProgramY_YUVSource_Clear);

    gProgramUV_YUVSource_Clear = createProgram(gVertexShader_Clear, gFragmentShaderUV_YUVSource_Clear);
    if (!gProgramUV_YUVSource_Clear)
    {
        ALOGE("SprdWIDIBlit:: setupGraphics createProgram gProgramUV_YUVSource_Clear failed");
        goto err9;
    }
    iLocPosition = glGetAttribLocation(gProgramUV_YUVSource_Clear, "aPosition");
    checkGlError("glGetAttribLocation");
    ALOGI_IF(DebugGFX, "UV Clear aPosition = %d", iLocPosition);
    glUseProgram(gProgramUV_YUVSource_Clear);

    /*
     *  For Target RGB shader.
     * */
    gProgramRGB = createProgram(gVertexShader, gFragmentShaderRGB);
    if (!gProgramRGB)
    {
        ALOGE("SprdWIDIBlit:: setupGraphics createProgram gProgramRGB failed");
        goto err10;
    }
    iLocPosition = glGetAttribLocation(gProgramRGB, "aPosition");
    iLocTexcoord = glGetAttribLocation(gProgramRGB, "aTexcoord");
    checkGlError("glGetAttribLocationRGB");
    iLocTexMatrixRGB = glGetUniformLocation(gProgramRGB, "textureMatrix");
    checkGlError("glGetUniformLocation");
    ALOGI_IF(DebugGFX, "RGB target aPosition, aTexcoord = %d, %d, iLocTexMatrixRGB: %d",
             iLocPosition, iLocTexcoord, iLocTexMatrixRGB);

    iLocTexSampler = glGetUniformLocation(gProgramRGB, "uTexSampler");
    checkGlError("glGetUniformLocationRGB");
    ALOGI_IF(DebugGFX, "RGB target uTexSampler = %d\n", iLocTexSampler);
    glUseProgram(gProgramRGB);
    glUniform1i(iLocTexSampler, 0);

    return 0;

err10:
   glDeleteProgram(gProgramUV_YUVSource_Clear);
err9:
   glDeleteProgram(gProgramY_YUVSource_Clear);
err8:
    glDeleteProgram(gProgramUV_YUVSource);
err7:
    glDeleteProgram(gProgramY_YUVSource);
err6:
    glDeleteProgram(gProgramUV);
err5:
    glDeleteProgram(gProgramY);
err4:
    eglMakeCurrent(dpy, NULL, NULL, context);
err3:
    eglDestroyContext(dpy, context);
err2:
    eglDestroySurface(dpy, surface);
err1:
    return -1;
}

void SprdWIDIBlit:: destoryGraphics()
{
    EGLDisplay dpy;
    EGLContext context;
    EGLSurface surface;

    context = eglGetCurrentContext();
    surface = eglGetCurrentSurface(EGL_DRAW);
    dpy     = eglGetCurrentDisplay();

    eglMakeCurrent(dpy, NULL, NULL, context);

    glDeleteProgram(gProgramUV_YUVSource_Clear);
    glDeleteProgram(gProgramY_YUVSource_Clear);
    glDeleteProgram(gProgramUV_YUVSource);
    glDeleteProgram(gProgramY_YUVSource);
    glDeleteProgram(gProgramUV);
    glDeleteProgram(gProgramY);
    glDeleteProgram(gProgramRGB);

    eglDestroySurface(dpy, surface);
    eglDestroyContext(dpy, context);
}

int SprdWIDIBlit:: setupYuvTexSurface(hwc_layer_1_t *AndroidLayer, private_handle_t *TargetHandle, sp<GraphicBuffer>& Source, sp<GraphicBuffer>& Target)
{
    HWC_TRACE_CALL;
    if (AndroidLayer == NULL)
    {
        ALOGE("SprdWIDIBlit:: setupYuvTexSurface AndroidLayer is NULL");
        return -1;
    }
    struct private_handle_t *SourceHandle = (struct private_handle_t *)AndroidLayer->handle;

    if (SourceHandle == NULL || TargetHandle == NULL)
    {
        ALOGE("SprdWIDIBlit:: setupYuvTexSurface intput handle is NULL");
        return -1;
    }

    hwc_layer_1_t *layer = AndroidLayer;

    Source = wrapGraphicsBuffer(SourceHandle);
    Target = wrapGraphicsBuffer(TargetHandle);
    if (Source == NULL || Target == NULL)
    {
        ALOGE("SprdWIDIBlit:: setupYuvTexSurface GFX buffer is NULL");
        return -1;
    }

    EGLDisplay dpy = eglGetCurrentDisplay();

    EGLint image_attrib_source[] = {
        EGL_IMAGE_FORMAT_SPRD, EGL_IMAGE_FORMAT_L8_SPRD,
#if Y_FIRST
        EGL_IMAGE_OFFSET_SPRD, 0,
#else
        EGL_IMAGE_OFFSET_SPRD, SourceHandle->width * SoruceHandle->height / 2,
#endif
        EGL_IMAGE_WIDTH_SPRD,  SourceHandle->width,
        EGL_IMAGE_HEIGHT_SPRD, SourceHandle->height,
        EGL_NONE
    };

    EGLint image_attrib[] = {
        EGL_IMAGE_FORMAT_SPRD, EGL_IMAGE_FORMAT_L8_SPRD,
#if Y_FIRST
        EGL_IMAGE_OFFSET_SPRD, 0,
#else
        EGL_IMAGE_OFFSET_SPRD, TargetHandle->width * TargetHandle->height / 2,
#endif
        EGL_IMAGE_WIDTH_SPRD,  TargetHandle->width,
        EGL_IMAGE_HEIGHT_SPRD, TargetHandle->height,
        EGL_NONE
    };

    if (((Target->getPixelFormat() == HAL_PIXEL_FORMAT_YCbCr_420_SP)
         || (Target->getPixelFormat() == HAL_PIXEL_FORMAT_YCrCb_420_SP))
         && (Source->getPixelFormat() == HAL_PIXEL_FORMAT_YCbCr_420_SP))
    {
        YUVToYUV = true;
        TargetOutputRGB = false;
    }
    else if ((Target->getPixelFormat() == HAL_PIXEL_FORMAT_RGBA_8888)
         ||  (Target->getPixelFormat() == HAL_PIXEL_FORMAT_RGBX_8888)
         ||  (Target->getPixelFormat() == HAL_PIXEL_FORMAT_BGRA_8888)
         ||  (Target->getPixelFormat() == HAL_PIXEL_FORMAT_RGB_888)
         ||  (Target->getPixelFormat() == HAL_PIXEL_FORMAT_RGB_565))
    {
        YUVToYUV = false;
        TargetOutputRGB = true;
        ALOGI_IF(DebugGFX, "SprdWIDIBlit GFX output RGB");
    }
    else
    {
        YUVToYUV = false;
        TargetOutputRGB = false;
    }

    if (YUVToYUV)
    {
        img_src = eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                  (EGLClientBuffer)Source->getNativeBuffer(), image_attrib_source);
    }
    else
    {
        img_src = eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                  (EGLClientBuffer)Source->getNativeBuffer(), NULL);
    }

    glBindTexture(GL_TEXTURE_2D, tex_src);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)img_src);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    checkGlError("glEGLImageTargetTexture 1");


    if (TargetOutputRGB)
    {
        img_dstRGB = eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                   (EGLClientBuffer)Target->getNativeBuffer(), NULL);
        glBindTexture(GL_TEXTURE_2D, tex_dst_rgb);
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)img_dstRGB);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        checkGlError("glEGLImageTargetTexture 2");
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_rgb);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_dst_rgb, 0);
        checkGlError("glFramebufferTexture2DOES 1");
    }
    else
    {
        img_dstY = eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                   (EGLClientBuffer)Target->getNativeBuffer(), image_attrib);
        glBindTexture(GL_TEXTURE_2D, tex_dst_y);
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)img_dstY);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        checkGlError("glEGLImageTargetTexture 2");
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_y);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_dst_y, 0);
        checkGlError("glFramebufferTexture2DOES 1");
    }



    image_attrib_source[1] = EGL_IMAGE_FORMAT_A8L8_SPRD;
#if Y_FIRST
    image_attrib_source[3] = SourceHandle->width * SourceHandle->height;
#else
    image_attrib_source[3] = 0;
#endif
    image_attrib_source[5] = SourceHandle->width / 2;
    image_attrib_source[7] = SourceHandle->height / 2;


    image_attrib[1] = EGL_IMAGE_FORMAT_A8L8_SPRD;
#if Y_FIRST
    image_attrib[3] = TargetHandle->width * TargetHandle->height;
#else
    image_attrib[3] = 0;
#endif
    image_attrib[5] = TargetHandle->width / 2;
    image_attrib[7] = TargetHandle->height / 2;

    if (YUVToYUV)
    {
        img_src_uv = eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                     (EGLClientBuffer)Source->getNativeBuffer(), image_attrib_source);
        glBindTexture(GL_TEXTURE_2D, tex_src_uv);
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)img_src_uv);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        checkGlError("glEGLImageTargetTexture source UV 1");
    }


    if (TargetOutputRGB == false)
    {
        img_dstUV = eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                    (EGLClientBuffer)Target->getNativeBuffer(), image_attrib);
        glBindTexture(GL_TEXTURE_2D, tex_dst_uv);
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)img_dstUV);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        checkGlError("glEGLImageTargetTexture 4");

        glBindFramebuffer(GL_FRAMEBUFFER, fbo_uv);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_dst_uv, 0);
        checkGlError("glFramebufferTexture2DOES 2");
    }

    static GLfloat vertex[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    static GLfloat texcoord[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    texcoord[0] = texcoord[6] = layer->sourceCropf.left / ((float)SourceHandle->width);
    texcoord[1] = texcoord[3] = layer->sourceCropf.top / ((float)SourceHandle->height);
    texcoord[2] = texcoord[4] = layer->sourceCropf.right / ((float)SourceHandle->width);
    texcoord[5] = texcoord[7] = layer->sourceCropf.bottom / ((float)SourceHandle->height);
    ALOGI_IF(DebugGFX, "texture coordinates(%f, %f, %f, %f)", texcoord[0], texcoord[1], texcoord[2], texcoord[5]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertex);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texcoord);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    TargetRegion.x = layer->displayFrame.left;
    TargetRegion.y = (layer->displayFrame.top) & 0xFFFFFFFE;
    TargetRegion.w = (layer->displayFrame.right - layer->displayFrame.left);
    TargetRegion.h = (layer->displayFrame.bottom - layer->displayFrame.top) & 0xFFFFFFFE;

    if (YUVToYUV == false)
    {
        if (TargetRegion.y > 1)
        {
            TargetRegion.y -= 1;
        }
    }

    /*
     *  Update the texture matrix.
     * */
    memcpy(gTexMatrix, mtxIdentity, sizeof(gTexMatrix));
    int tr = layer->transform;

    if (tr & NATIVE_WINDOW_TRANSFORM_FLIP_H)
    {
        float res[16];
        mtxMul(res, gTexMatrix, mtxFlipH);
        memcpy(gTexMatrix, res, sizeof(gTexMatrix));
    }

    if (tr & NATIVE_WINDOW_TRANSFORM_FLIP_V)
    {
        float res[16];
        mtxMul(res, gTexMatrix, mtxFlipV);
        memcpy(gTexMatrix, res, sizeof(gTexMatrix));
    }

    if (tr & NATIVE_WINDOW_TRANSFORM_ROT_90)
    {
        float res[16];
        mtxMul(res, gTexMatrix, mtxRot90);
        memcpy(gTexMatrix, res, sizeof(gTexMatrix));
    }

    return 0;
}

int SprdWIDIBlit:: renderImage(sp<GraphicBuffer> Source, sp<GraphicBuffer> Target)
{
    HWC_TRACE_CALL;
    char* buf = NULL;
    int w = Target->getStride();
    int h = Target->getHeight();

    int x2 = (int)((float)TargetRegion.x + 0.5) / 2.0;
    int y2 = (int)((float)TargetRegion.y + 0.5) / 2.0;
    int w2 = (int)((float)TargetRegion.w + 0.5) / 2.0;
    int h2 = (int)((float)TargetRegion.h + 0.5) / 2.0;

    EGLDisplay dpy = eglGetCurrentDisplay();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    if (YUVToYUV)
    {
        glUseProgram(gProgramY_YUVSource_Clear);
        glViewport(0, 0, w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_y);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    else if (TargetOutputRGB)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glBindTexture(GL_TEXTURE_2D, tex_src);

    if (YUVToYUV)
    {
        ALOGI_IF(DebugGFX, "SprdWIDIBlit:: renderImage use YUVSource shader");
        glUseProgram(gProgramY_YUVSource);
        glUniformMatrix4fv(iLocTexMatrixYSource, 1, GL_FALSE, static_cast<GLfloat const*>(gTexMatrix));
    }
    else if (TargetOutputRGB)
    {
        ALOGI_IF(DebugGFX, "SprdWIDIBlit:: renderImage use RGB target shader");
        glUseProgram(gProgramRGB);
        glUniformMatrix4fv(iLocTexMatrixRGB, 1, GL_FALSE, static_cast<GLfloat const*>(gTexMatrix));
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_rgb);
        checkGlError("glBindFramebufferOES RGB 1");
    }
    else
    {
        glUseProgram(gProgramY);
        glUniformMatrix4fv(iLocTexMatrixY, 1, GL_FALSE, static_cast<GLfloat const*>(gTexMatrix));
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_y);
        checkGlError("glBindFramebufferOES 1");
    }

    glViewport(TargetRegion.x, TargetRegion.y, TargetRegion.w, TargetRegion.h);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    checkGlError("glDrawArrays");

    if (TargetOutputRGB)
    {
        goto SchedualDone;
    }

    if (YUVToYUV)
    {
        /*
         *  For clear UV plane.
         * */
        glUseProgram(gProgramUV_YUVSource_Clear);
        glViewport(0, 0, w/2, h/2);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_uv);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glUseProgram(gProgramUV_YUVSource);
        glUniformMatrix4fv(iLocTexMatrixUVSource, 1, GL_FALSE, static_cast<GLfloat const*>(gTexMatrix));
        glBindTexture(GL_TEXTURE_2D, tex_src_uv);
    }
    else
    {
        glUseProgram(gProgramUV);
        glUniformMatrix4fv(iLocTexMatrixUV, 1, GL_FALSE, static_cast<GLfloat const*>(gTexMatrix));
    }

    glViewport(x2, y2, w2, h2);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_uv);
    checkGlError("glBindFramebufferOESuv");
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    checkGlError("glDrawArraysUV");

SchedualDone:
    glFinish();
    checkGlError("glFinish");

    Target->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, (void**)(&buf));
    if (TargetOutputRGB)
    {
        ALOGI_IF(DebugGFX, "renderImage get output R:%02x, G:%02x, B:%02x",
                 *(GLubyte*)buf, *(GLubyte*)(buf+1), *(GLubyte*)(buf+2));
    }
    else
    {
        ALOGI_IF(DebugGFX, "renderImage get output Y:%02x, CrCb:%04x",
                 *(GLubyte*)buf, *(GLushort*)(buf + Target->getStride() * h));
    }


    eglDestroyImageKHR(dpy, img_src);
    if (YUVToYUV)
    {
        eglDestroyImageKHR(dpy, img_src_uv);
    }
    if (TargetOutputRGB)
    {
        eglDestroyImageKHR(dpy, img_dstRGB);
    }
    else
    {
        eglDestroyImageKHR(dpy, img_dstY);
        eglDestroyImageKHR(dpy, img_dstUV);
    }

    return 0;
}

}
