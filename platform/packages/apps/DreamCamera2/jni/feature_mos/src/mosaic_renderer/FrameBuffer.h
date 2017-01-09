#pragma once

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#ifdef OPTIMIZE_OPENGLES_CALL
#include <EGL/eglext.h>
#include <semaphore.h>

#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
#include <ui/GraphicBuffer.h>
#include <ui/FramebufferNativeWindow.h>
#include <cutils/log.h>
using namespace android;
extern sem_t gEGLImageKHR_semaphore;
#endif

#define checkGlError(op)  checkGLErrorDetail(__FILE__, __LINE__, (op))

extern bool checkGLErrorDetail(const char* file, int line, const char* op);
extern void checkFramebufferStatus(const char* name);

class FrameBuffer {
  public:
    FrameBuffer();
    virtual ~FrameBuffer();

    bool InitializeGLContext();
    bool Init(int width, int height, GLenum format);
#ifdef OPTIMIZE_OPENGLES_CALL
    bool Init_bind_EGLImageKHR_texture(int width, int height, GLenum format);
    void Destory_bind_EGLImageKHR_texture();
#endif
    GLuint GetTextureName() const;
    GLuint GetFrameBufferName() const;
    GLenum GetFormat() const;

    int GetWidth() const;
    int GetHeight() const;

 private:
    void Reset();
    bool CreateBuffers();
    GLuint mFrameBufferName;
    GLuint mTextureName;
    int mWidth;
    int mHeight;
    GLenum mFormat;
#ifdef OPTIMIZE_OPENGLES_CALL
	public:
	sp< GraphicBuffer > mGrapBuffer;
	EGLImageKHR  mImageKHR;
#endif
};
