#include <cutils/log.h>
#include <hardware/gralloc.h>
#include "gralloc_priv.h"
#include <ui/GraphicBuffer.h>
#include "gpu_transform.h"
#include <pthread.h>
#include <semaphore.h>
#define GLES2_TRANSFORM 0

#include <EGL/egl.h>
#include <EGL/eglext.h>
#if GLES2_TRANSFORM
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define glFramebufferTexture2DOES glFramebufferTexture2D
#define glGenFramebuffersOES glGenFramebuffers
#define glBindFramebufferOES glBindFramebuffer
#define GL_FRAMEBUFFER_OES GL_FRAMEBUFFER
#define GL_COLOR_ATTACHMENT0_OES GL_COLOR_ATTACHMENT0
#else
#include <GLES/gl.h>
#include <GLES/glext.h>
#endif


static EGLDisplay   egl_dpy;
static EGLContext   egl_context;
static EGLSurface   egl_surface;
static int is_init = 0;
static unsigned int transform_thread_init_flag = 0;
static volatile unsigned int transform_thread_exit = 0;
pthread_t  transform_thread;
sem_t      t_cmd_sem;
sem_t      t_done_sem;

static GLuint tex_src;
static GLuint tex_dst;
GLuint fbo;
static GLuint tex_src_osd;
static GLuint tex_dst_osd;
GLuint fbo2;

#ifdef _DEBUG
#define GL_CHECK(x) \
    x; \
    { \
        GLenum err = glGetError(); \
        if(err != GL_NO_ERROR) { \
            ALOGE("glGetError() = %i (0x%.8x) at line %i\n", err, err, __LINE__); \
        } \
    }
#else
#define GL_CHECK(x) x
#endif




static GLfloat vertices[] = {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    1.0f,  1.0f,
    -1.0f,  1.0f
};

static GLfloat texcoords[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f
};

void install()
{
    GL_CHECK(glDisable(GL_CULL_FACE));
    GL_CHECK(glDisable(GL_SCISSOR_TEST));
    GL_CHECK(glDisable(GL_DEPTH_TEST));
    GL_CHECK(glDisable(GL_BLEND));
    GL_CHECK(glDisable(GL_DITHER));

    glGenTextures(1, &tex_src);
    glGenTextures(1, &tex_dst);

    glGenTextures(1, &tex_src_osd);
    glGenTextures(1, &tex_dst_osd);

    glActiveTexture(GL_TEXTURE0);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

#if GLES2_TRANSFORM
    const char* vert_src[] =  {
        "\
        attribute highp   vec4 vert_position;\
        attribute mediump vec2 vert_texcoord;\
        varying   mediump vec2 tcoord;\
        void main() \
        { \
            tcoord = vert_texcoord; \
            gl_Position = vert_position; \
        } \
        "
    };

    const char* frag_src[] = {
        "\
        #extension GL_OES_EGL_image_external:require\n\
        precision mediump float; \
        varying mediump vec2 tcoord; \
        uniform samplerExternalOES tex; \
        void main() \
        { \
            gl_FragColor = texture2D(tex, tcoord); \
        }",
    };

    GLuint vert_shader;
    GLuint frag_shader;
    GLuint convert_program;
    int status;
    char buffer[1024];

    GL_CHECK(vert_shader = glCreateShader(GL_VERTEX_SHADER));
    GL_CHECK(glShaderSource(vert_shader, 1, vert_src, 0));
    GL_CHECK(glCompileShader(vert_shader));
    GL_CHECK(glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &status));
    GL_CHECK(glGetShaderInfoLog(vert_shader, 1024, NULL, buffer));
    ALOGI("Shader Log: %s\n", buffer);

    GL_CHECK(frag_shader = glCreateShader(GL_FRAGMENT_SHADER));
    GL_CHECK(glShaderSource(frag_shader, 1, frag_src, 0));
    GL_CHECK(glCompileShader(frag_shader));
    GL_CHECK(glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &status));
    GL_CHECK(glGetShaderInfoLog(frag_shader, 1024, NULL, buffer));
    ALOGI("Shader Log: %s\n", buffer);

    GL_CHECK(convert_program = glCreateProgram());
    GL_CHECK(glAttachShader(convert_program, vert_shader));
    GL_CHECK(glAttachShader(convert_program, frag_shader));
    GL_CHECK(glLinkProgram(convert_program));
    GL_CHECK(glGetProgramiv(convert_program, GL_COMPILE_STATUS, &status));
    GL_CHECK(glGetProgramInfoLog(convert_program, 1024, NULL, buffer));
    ALOGI("Program Log: %s\n", buffer);

    GL_CHECK(glUseProgram(convert_program));

    GLuint i_position0;
    GLuint i_texcoord0;
    GL_CHECK(i_position0 = glGetAttribLocation(convert_program, "vert_position"));
    GL_CHECK(i_texcoord0 = glGetAttribLocation(convert_program, "vert_texcoord"));
    GL_CHECK(glEnableVertexAttribArray(i_position0));
    GL_CHECK(glEnableVertexAttribArray(i_texcoord0));
    GL_CHECK(glVertexAttribPointer(i_position0, 2, GL_FLOAT, GL_FALSE, 0, vertices));
    GL_CHECK(glVertexAttribPointer(i_texcoord0, 2, GL_FLOAT, GL_FALSE, 0, texcoords));

    GLuint texture_location;
    GL_CHECK(texture_location = glGetUniformLocation(convert_program, "tex"));
    glUniform1i(texture_location, 0);
#else
    GL_CHECK(glEnable(GL_TEXTURE_EXTERNAL_OES));

    GL_CHECK(glClientActiveTexture(GL_TEXTURE0));
    GL_CHECK(glEnableClientState(GL_VERTEX_ARRAY));
    GL_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
    GL_CHECK(glVertexPointer(2, GL_FLOAT, 0, vertices));
    GL_CHECK(glTexCoordPointer(2, GL_FLOAT, 0, texcoords));

    GL_CHECK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE));
#endif

    //GLuint fbo;
    GL_CHECK(glGenFramebuffersOES(1, &fbo));
    GL_CHECK(glGenFramebuffersOES(1, &fbo2));
}

int init()
{
    static unsigned int init_flag = 0;
    EGLint attribs[] = {
        EGL_BUFFER_SIZE, 16,
        EGL_NONE };
    EGLint context_attribs[] = {
#if GLES2_TRANSFORM
        EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
        EGL_NONE };
    EGLConfig config;
    EGLint num_config;
    EGLBoolean ret;

    egl_dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_dpy == EGL_NO_DISPLAY) {
        ALOGE("eglGetDisplay get default display failed\n");
        goto err_out;
    }

    ret = eglChooseConfig(egl_dpy, attribs, &config, 1, &num_config);
    if (!ret) {
        ALOGE("eglChooseConfig() failed\n");
        goto err_out;
    }

    egl_surface = eglCreatePbufferSurface(egl_dpy, config, NULL);
    if (egl_surface == EGL_NO_SURFACE) {
        ALOGE("gelCreateWindowSurface failed.\n");
        goto err_out;
    }

    egl_context = eglCreateContext(egl_dpy, config, EGL_NO_CONTEXT, context_attribs);
    if (egl_context == EGL_NO_CONTEXT) {
        ALOGE("eglCreateContext failed\n");
        goto err_1;
    }

    ret = eglMakeCurrent(egl_dpy, egl_surface, egl_surface, egl_context);
    if (!ret) {
        ALOGE("eglMakeCurrent() failed\n");
        goto err_2;
    }

    install();

    is_init = 1;

    return is_init;

err_2:
    eglDestroyContext(egl_dpy, egl_context);
    egl_context = EGL_NO_CONTEXT;
err_1:
    eglDestroySurface(egl_dpy, egl_surface);
    egl_surface = EGL_NO_SURFACE;
err_out:
    egl_dpy = EGL_NO_DISPLAY;
out:
    return -1;
}

#define ALIGN(value, base) (((value) + ((base) - 1)) & ~((base) - 1))

void get_size_stride(uint32_t width, uint32_t height, uint32_t format, uint32_t &size, uint32_t &stride)
{
    switch (format)
    {
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
        stride = width;
        // mali GPU hardware requires uv-plane 64byte-alignment
        size = GRALLOC_ALIGN(height * stride, 64) + height * GRALLOC_ALIGN(stride/2,16);
        size = ALIGN(size, 4096);
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
        stride = width;
        // mali GPU hardware requires u/v-plane 64byte-alignment
        size = GRALLOC_ALIGN(height * stride, 64) + GRALLOC_ALIGN(height/2 * GRALLOC_ALIGN(stride/2,16), 64) + height/2 * GRALLOC_AL
        size = ALIGN(size, 4096);
        break;
     case HAL_PIXEL_FORMAT_YV12:
        stride = GRALLOC_ALIGN(width, 16);
        // mali GPU hardware requires u/v-plane 64byte-alignment
        size = GRALLOC_ALIGN(height * stride, 64) + GRALLOC_ALIGN(height/2 * GRALLOC_ALIGN(stride/2,16), 64) + height/2 * GRALLOC_AL
        size = ALIGN(size, 4096);
        break;

    default:
        {
            int bpp = 0;
            switch (format)
            {
            case HAL_PIXEL_FORMAT_RGBA_8888:
            case HAL_PIXEL_FORMAT_RGBX_8888:
            case HAL_PIXEL_FORMAT_BGRA_8888:
                bpp = 4;
                break;
            case HAL_PIXEL_FORMAT_RGB_888:
                bpp = 3;
                break;
            case HAL_PIXEL_FORMAT_RGB_565:
            case HAL_PIXEL_FORMAT_RGBA_5551:
            case HAL_PIXEL_FORMAT_RGBA_4444:
                bpp = 2;
                break;
            default:
                return;
            }
            uint32_t bpr = ALIGN(width*bpp, 8);
            size = bpr * height;
            stride = bpr / bpp;
            size = ALIGN(size, 4096);
        }
    }
}

using namespace android;
#ifdef __cplusplus
extern "C"
{
#endif

static void *transform_video_layer(void *t_data);
static void *transform_osd_layer(void *t_data);
static void *transform_thread_body(void *data);
static gpu_transform_info_t g_data;

int gpu_transform_layers(gpu_transform_info_t *data)
{
    memcpy(&g_data , data , sizeof(gpu_transform_info_t));
    if (transform_thread_init_flag == 0) {
        sem_init(&t_cmd_sem, 0, 0);
        sem_init(&t_done_sem, 0, 0);
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&transform_thread, &attr, transform_thread_body, (void*)(&g_data));
        pthread_attr_destroy(&attr);
        transform_thread_init_flag = 1;
    }

    sem_post(&t_cmd_sem);
    sem_wait(&t_done_sem);

    return 0;
}

static void *transform_thread_body(void *data)
{
    while (1) {
        if(!is_init && (init() == -1))
        {
           ALOGE("transform thread init EGL ENV failed");
           goto Failed;
        }

        sem_wait(&t_cmd_sem);

        if (transform_thread_exit == 1) {
            ALOGD("transform_thread_body exit");
            transform_thread_exit--;
            goto Failed;
        }

        if (data == NULL) {
           ALOGE("transform data is NULL");
           goto Failed;
        }

        gpu_transform_info_t *t_data = (gpu_transform_info_t *)data;
        transform_data_t *video = &(t_data->video);
        transform_data_t *osd = &(t_data->osd);

        if (t_data->flag & VIDEO_LAYER_EXIST)
        {
            transform_video_layer((void *)video);
        }

        if (t_data->flag & OSD_LAYER_EXIST)
        {
            transform_osd_layer((void *)osd);
        }
        sem_post(&t_done_sem);
    }

    return NULL;

Failed:
    sem_post(&t_done_sem);
    return NULL;
}

void destroy_transform_thread()
{
    if (transform_thread_init_flag == 1) {
        transform_thread_exit++;
        sem_post(&t_cmd_sem);
        void *dummy_null;
        pthread_join(transform_thread, &dummy_null);
        sem_destroy(&t_cmd_sem);
        sem_destroy(&t_done_sem);
    }
}

static void* transform_video_layer(void *t_data)
{
    EGLImageKHR img_src = NULL;
    EGLImageKHR img_dst = NULL;
    ump_handle ump_h = NULL;

    {
       struct transform_data *data = (struct transform_data *)t_data;

       uint32_t size;
       uint32_t stride;

       get_size_stride(data->srcWidth, data->srcHeight, data->srcFormat, size, stride);
       ump_h = ump_handle_create_from_phys_block(data->srcPhy, size);
       if(ump_h == NULL)
       {
           ALOGE("ump_h_src create fail");
           goto failed;
       }
       private_handle_t handle_src(private_handle_t::PRIV_FLAGS_USES_UMP | private_handle_t::PRIV_FLAGS_USES_PHY,
                            size,
                            data->srcVirt,
                            private_handle_t::LOCK_STATE_MAPPED,
                            ump_secure_id_get(ump_h),
                            ump_h);
       handle_src.width = stride;
       handle_src.height = data->srcHeight;
       handle_src.format = data->srcFormat;
       sp<GraphicBuffer> buf_src = new GraphicBuffer(data->srcWidth, data->srcHeight, data->srcFormat,
                                            GraphicBuffer::USAGE_HW_TEXTURE,
                                            stride,
                                            (native_handle_t*)&handle_src, false);
       if(buf_src->initCheck() != NO_ERROR)
       {
           ALOGE("buf_src create fail");
           goto failed;
       }

       get_size_stride(data->dstWidth, data->dstHeight, data->dstFormat, size, stride);
       ump_h = ump_handle_create_from_phys_block(data->dstPhy, size);
       if(ump_h == NULL)
       {
           ALOGE("ump_h_dst create fail");
           goto failed;
       }
       private_handle_t handle_dst(private_handle_t::PRIV_FLAGS_USES_UMP | private_handle_t::PRIV_FLAGS_USES_PHY,
                            size,
                            data->dstVirt,
                            private_handle_t::LOCK_STATE_MAPPED,
                            ump_secure_id_get(ump_h),
                            ump_h);
       handle_dst.width = stride;
       handle_dst.height = data->dstHeight;
       handle_dst.format = data->dstFormat;
       sp<GraphicBuffer> buf_dst = new GraphicBuffer(data->dstWidth, data->dstHeight, data->dstFormat,
                                            GraphicBuffer::USAGE_HW_RENDER,
                                            stride,
                                            (native_handle_t*)&handle_dst, false);
       if(buf_dst->initCheck() != NO_ERROR)
       {
           ALOGE("buf_dst create fail");
           goto failed;
       }

#if !GLES2_TRANSFORM
           switch(data->transform) {
           case 0:
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                break;
            default:
            case HAL_TRANSFORM_ROT_90:
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glRotatef(90.0, 0.0f, 0.0f, 1.0f);
                break;
            case HAL_TRANSFORM_ROT_180:
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glRotatef(180.0, 0.0f, 0.0f, 1.0f);
                break;
            case HAL_TRANSFORM_ROT_270:
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glRotatef(270.0, 0.0f, 0.0f, 1.0f);
                break;
            case HAL_TRANSFORM_FLIP_H:
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glRotatef(180.0, 0.0f, 1.0f, 0.0f);
                break;
           }
#endif

	texcoords[0] = texcoords[6] = data->trim_rect.x / (float)data->srcWidth;
	texcoords[1] = texcoords[3] = data->trim_rect.y / (float)data->srcHeight;
	texcoords[2] = texcoords[4] = (data->trim_rect.x + data->trim_rect.w) / (float)data->srcWidth;
	texcoords[5] = texcoords[7] = (data->trim_rect.y + data->trim_rect.h) / (float)data->srcHeight;


      static EGLint attribs[] = {
            EGL_IMAGE_PRESERVED_KHR,    EGL_TRUE,
            EGL_NONE};

      img_src = eglCreateImageKHR(egl_dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)buf_src->getNativeBuffer(), attribs);
      if(img_src == EGL_NO_IMAGE_KHR)
      {
          ALOGD("video img_src create fail, error = %x", eglGetError());
          goto failed;
      }
      GL_CHECK(glBindTexture(GL_TEXTURE_EXTERNAL_OES, tex_src));
      GL_CHECK(glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)img_src));

      img_dst = eglCreateImageKHR(egl_dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)buf_dst->getNativeBuffer(), attribs);
      if(img_dst == EGL_NO_IMAGE_KHR)
      {
          ALOGD("img_dst create fail, error = %x", eglGetError());
          goto failed;
      }

      GL_CHECK(glBindFramebufferOES(GL_FRAMEBUFFER_OES, fbo));
      GL_CHECK(glBindTexture(GL_TEXTURE_2D, tex_dst));
      GL_CHECK(glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)img_dst));

      GL_CHECK(glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, tex_dst, 0));

      GL_CHECK(glViewport(0, 0, data->dstWidth, data->dstHeight));

      GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));

      GL_CHECK(glFinish());

      GL_CHECK(glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0));
      eglDestroyImageKHR(egl_dpy, img_src);
      eglDestroyImageKHR(egl_dpy, img_dst);

      ump_free_handle_from_mapped_phys_block((ump_handle)handle_src.ump_mem_handle);
      ump_free_handle_from_mapped_phys_block((ump_handle)handle_dst.ump_mem_handle);
   }

   return NULL;

failed:
   if (img_dst != NULL) {
       eglDestroyImageKHR(egl_dpy, img_dst);
   }
   if (img_src != NULL) {
       eglDestroyImageKHR(egl_dpy, img_src);
   }
   if (ump_h != NULL) {
       ump_free_handle_from_mapped_phys_block(ump_h);
   }
   return NULL;
}

static void* transform_osd_layer(void *t_data)
{
    EGLImageKHR img_src2 = NULL;
    EGLImageKHR img_dst2 = NULL;
    ump_handle ump_h = NULL;

    {
        struct transform_data *data = (struct transform_data *)t_data;

        uint32_t size;
        uint32_t stride;

        get_size_stride(data->srcWidth, data->srcHeight, data->srcFormat, size, stride);
        ump_h = ump_handle_create_from_phys_block(data->srcPhy, size);
        if(ump_h == NULL)
        {
            ALOGE("ump_h_src create fail");
            goto failed;
        }
        private_handle_t handle_src2(private_handle_t::PRIV_FLAGS_USES_UMP | private_handle_t::PRIV_FLAGS_USES_PHY,
                            size,
                            data->srcVirt,
                            private_handle_t::LOCK_STATE_MAPPED,
                            ump_secure_id_get(ump_h),
                            ump_h);
        handle_src2.width = stride;
        handle_src2.height = data->srcHeight;
        handle_src2.format = data->srcFormat;
        sp<GraphicBuffer> buf_src = new GraphicBuffer(data->srcWidth, data->srcHeight, data->srcFormat,
                                            GraphicBuffer::USAGE_HW_TEXTURE,
                                            stride,
                                            (native_handle_t*)&handle_src2, false);
        if(buf_src->initCheck() != NO_ERROR)
        {
            ALOGE("buf_src create fail");
            goto failed;
        }

        get_size_stride(data->dstWidth, data->dstHeight, data->dstFormat, size, stride);
        ump_h = ump_handle_create_from_phys_block(data->dstPhy, size);
        if(ump_h == NULL)
        {
            ALOGE("ump_h_dst create fail");
            goto failed;
        }
        private_handle_t handle_dst2(private_handle_t::PRIV_FLAGS_USES_UMP | private_handle_t::PRIV_FLAGS_USES_PHY,
                            size,
                            data->dstVirt,
                            private_handle_t::LOCK_STATE_MAPPED,
                            ump_secure_id_get(ump_h),
                            ump_h);
        handle_dst2.width = stride;
        handle_dst2.height = data->dstHeight;
        handle_dst2.format = data->dstFormat;
        sp<GraphicBuffer> buf_dst = new GraphicBuffer(data->dstWidth, data->dstHeight, data->dstFormat,
                                            GraphicBuffer::USAGE_HW_RENDER,
                                            stride,
                                            (native_handle_t*)&handle_dst2, false);
        if(buf_dst->initCheck() != NO_ERROR)
        {
            ALOGE("buf_dst create fail");
            goto failed;
        }

#if !GLES2_TRANSFORM
            switch(data->transform) {
            case 0:
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                break;
            default:
            case HAL_TRANSFORM_ROT_90:
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glRotatef(90.0, 0.0f, 0.0f, 1.0f);
                break;
            case HAL_TRANSFORM_ROT_180:
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glRotatef(180.0, 0.0f, 0.0f, 1.0f);
                break;
            case HAL_TRANSFORM_ROT_270:
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glRotatef(270.0, 0.0f, 0.0f, 1.0f);
                break;
            case HAL_TRANSFORM_FLIP_H:
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glRotatef(180.0, 0.0f, 1.0f, 0.0f);
                break;
           }
#endif

	texcoords[0] = texcoords[6] = data->trim_rect.x / (float)data->srcWidth;
	texcoords[1] = texcoords[3] = data->trim_rect.y / (float)data->srcHeight;
	texcoords[2] = texcoords[4] = (data->trim_rect.x + data->trim_rect.w) / (float)data->srcWidth;
	texcoords[5] = texcoords[7] = (data->trim_rect.y + data->trim_rect.h) / (float)data->srcHeight;


        static EGLint attribs[] = {
            EGL_IMAGE_PRESERVED_KHR,    EGL_TRUE,
            EGL_NONE};

        img_src2 = eglCreateImageKHR(egl_dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)buf_src->getNativeBuffer(), attribs);
        if(img_src2 == EGL_NO_IMAGE_KHR)
        {
            ALOGE("OSD img_src create fail, error = %x", eglGetError());
            goto failed;
        }
        GL_CHECK(glBindTexture(GL_TEXTURE_EXTERNAL_OES, tex_src_osd));
        GL_CHECK(glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)img_src2));

        img_dst2 = eglCreateImageKHR(egl_dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)buf_dst->getNativeBuffer(), attribs);
        if(img_dst2 == EGL_NO_IMAGE_KHR)
        {
            ALOGE("img_dst create fail, error = %x", eglGetError());
            goto failed;
        }
        GL_CHECK(glBindFramebufferOES(GL_FRAMEBUFFER_OES, fbo2));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, tex_dst_osd));
        GL_CHECK(glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)img_dst2));

        GL_CHECK(glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, tex_dst_osd, 0));

        GL_CHECK(glViewport(0, 0, data->dstWidth, data->dstHeight));

        GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));

        GL_CHECK(glFinish());

        eglDestroyImageKHR(egl_dpy, img_src2);
        eglDestroyImageKHR(egl_dpy, img_dst2);

        ump_free_handle_from_mapped_phys_block((ump_handle)handle_src2.ump_mem_handle);
        ump_free_handle_from_mapped_phys_block((ump_handle)handle_dst2.ump_mem_handle);
    }

    return NULL;
failed:
    if (img_dst2 != NULL) {
        eglDestroyImageKHR(egl_dpy, img_dst2);
    }
    if (img_src2 != NULL) {
        eglDestroyImageKHR(egl_dpy, img_src2);
    }
    if (ump_h != NULL) {
        ump_free_handle_from_mapped_phys_block(ump_h);
    }
    return NULL;
}


#ifdef __cplusplus
}
#endif
