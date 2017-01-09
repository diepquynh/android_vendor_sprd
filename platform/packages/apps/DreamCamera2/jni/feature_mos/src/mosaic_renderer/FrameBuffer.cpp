#include "FrameBuffer.h"

FrameBuffer::FrameBuffer()
{
#ifdef OPTIMIZE_OPENGLES_CALL
	mGrapBuffer = NULL;
	mImageKHR = NULL;
#endif
	Reset();
}

FrameBuffer::~FrameBuffer() {
#ifdef OPTIMIZE_OPENGLES_CALL
	Destory_bind_EGLImageKHR_texture();
#endif
}

void FrameBuffer::Reset() {
    mFrameBufferName = -1;
    mTextureName = -1;
    mWidth = 0;
    mHeight = 0;
    mFormat = -1;
#ifdef OPTIMIZE_OPENGLES_CALL
	if(mGrapBuffer != NULL)
		Destory_bind_EGLImageKHR_texture();
#endif
}

bool FrameBuffer::InitializeGLContext() {
    Reset();
    return CreateBuffers();
}

bool FrameBuffer::Init(int width, int height, GLenum format) {
    if (mFrameBufferName == (GLuint)-1) {
        if (!CreateBuffers()) {
            return false;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferName);
    glBindTexture(GL_TEXTURE_2D, mTextureName);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 format,
                 width,
                 height,
                 0,
                 format,
                 GL_UNSIGNED_BYTE,
                 NULL);
    if (!checkGlError("bind/teximage")) {
        return false;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // This is necessary to work with user-generated frame buffers with
    // dimensions that are NOT powers of 2.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach texture to frame buffer.
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           mTextureName,
                           0);
    checkFramebufferStatus("FrameBuffer.cpp");
    checkGlError("framebuffertexture2d");

    if (!checkGlError("texture setup")) {
        return false;
    }
    mWidth = width;
    mHeight = height;
    mFormat = format;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}
#ifdef OPTIMIZE_OPENGLES_CALL
bool FrameBuffer::Init_bind_EGLImageKHR_texture(int width, int height, GLenum format) {
	if (mFrameBufferName == (GLuint)-1) {
		if (!CreateBuffers()) {
			return false;
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBufferName);
	glBindTexture(GL_TEXTURE_2D, mTextureName);

	if( mGrapBuffer == NULL ){
		sem_wait(&gEGLImageKHR_semaphore);
		mGrapBuffer = new GraphicBuffer(width, height, HAL_PIXEL_FORMAT_RGBA_8888, GraphicBuffer::USAGE_HW_RENDER);
		if(mGrapBuffer->initCheck() != NO_ERROR) {
			ALOGD("mGrapBuffer create fail");
			mGrapBuffer = NULL;
			return false;
		}
		mImageKHR = eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)mGrapBuffer -> getNativeBuffer(), 0);
		if(mImageKHR == EGL_NO_IMAGE_KHR){
			ALOGD("%d mImageKHR create fail, error = %x", __LINE__, eglGetError());
			goto failed;
		}
		sem_post(&gEGLImageKHR_semaphore);
	}
	glEGLImageTargetTexture2DOES( GL_TEXTURE_2D, mImageKHR );
	if (!checkGlError("bind/teximage")) {
			return false;
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// This is necessary to work with user-generated frame buffers with
// dimensions that are NOT powers of 2.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

// Attach texture to frame buffer.
	glFramebufferTexture2D(GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_2D,
				mTextureName,
				0);
	checkFramebufferStatus("FrameBuffer.cpp");
	checkGlError("framebuffertexture2d");

	if (!checkGlError("texture setup")) {
			return false;
	}
	mWidth = width;
	mHeight = height;
	mFormat = format;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ALOGD("function =%s, line =%d" , __FUNCTION__, __LINE__);
	return true;
failed:
	Destory_bind_EGLImageKHR_texture();
	return false;
}
void FrameBuffer::Destory_bind_EGLImageKHR_texture(){
	if( mGrapBuffer != NULL ){
			sem_wait(&gEGLImageKHR_semaphore);
			eglDestroyImageKHR(eglGetCurrentDisplay(), mImageKHR);
			mGrapBuffer = NULL;
			mImageKHR = NULL;
			sem_post(&gEGLImageKHR_semaphore);
	}
}	
#endif

bool FrameBuffer::CreateBuffers() {
    glGenFramebuffers(1, &mFrameBufferName);
    glGenTextures(1, &mTextureName);
    if (!checkGlError("texture generation")) {
        return false;
    }
    return true;
}

GLuint FrameBuffer::GetTextureName() const {
    return mTextureName;
}

GLuint FrameBuffer::GetFrameBufferName() const {
    return mFrameBufferName;
}

GLenum FrameBuffer::GetFormat() const {
    return mFormat;
}

int FrameBuffer::GetWidth() const {
    return mWidth;
}

int FrameBuffer::GetHeight() const {
    return mHeight;
}
