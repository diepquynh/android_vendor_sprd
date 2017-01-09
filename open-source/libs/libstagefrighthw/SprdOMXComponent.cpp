/*
 * Copyright (C) 2011 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "SprdOMXComponent"
#include <utils/Log.h>

#include "include/SprdOMXComponent.h"

#include <media/stagefright/foundation/ADebug.h>

namespace android {

SprdOMXComponent::SprdOMXComponent(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component)
    : mName(name),
      mCallbacks(callbacks),
      mComponent(new OMX_COMPONENTTYPE),
      mLibHandle(NULL) {
    mComponent->nSize = sizeof(*mComponent);
    mComponent->nVersion.s.nVersionMajor = 1;
    mComponent->nVersion.s.nVersionMinor = 0;
    mComponent->nVersion.s.nRevision = 0;
    mComponent->nVersion.s.nStep = 0;
    mComponent->pComponentPrivate = this;
    mComponent->pApplicationPrivate = appData;

    mComponent->GetComponentVersion = NULL;
    mComponent->SendCommand = SendCommandWrapper;
    mComponent->GetParameter = GetParameterWrapper;
    mComponent->SetParameter = SetParameterWrapper;
    mComponent->GetConfig = GetConfigWrapper;
    mComponent->SetConfig = SetConfigWrapper;
    mComponent->GetExtensionIndex = GetExtensionIndexWrapper;
    mComponent->GetState = GetStateWrapper;
    mComponent->ComponentTunnelRequest = NULL;
    mComponent->UseBuffer = UseBufferWrapper;
    mComponent->AllocateBuffer = AllocateBufferWrapper;
    mComponent->FreeBuffer = FreeBufferWrapper;
    mComponent->EmptyThisBuffer = EmptyThisBufferWrapper;
    mComponent->FillThisBuffer = FillThisBufferWrapper;
    mComponent->SetCallbacks = NULL;
    mComponent->ComponentDeInit = NULL;
    mComponent->UseEGLImage = NULL;
    mComponent->ComponentRoleEnum = NULL;

    *component = mComponent;
}

SprdOMXComponent::~SprdOMXComponent() {
    delete mComponent;
    mComponent = NULL;
}

void SprdOMXComponent::setLibHandle(void *libHandle) {
    CHECK(libHandle != NULL);
    mLibHandle = libHandle;
}

void *SprdOMXComponent::libHandle() const {
    return mLibHandle;
}

OMX_ERRORTYPE SprdOMXComponent::initCheck() const {
    return OMX_ErrorNone;
}

const char *SprdOMXComponent::name() const {
    return mName.c_str();
}

void SprdOMXComponent::notify(
        OMX_EVENTTYPE event,
        OMX_U32 data1, OMX_U32 data2, OMX_PTR data) {
    (*mCallbacks->EventHandler)(
            mComponent,
            mComponent->pApplicationPrivate,
            event,
            data1,
            data2,
            data);
}

void SprdOMXComponent::notifyEmptyBufferDone(OMX_BUFFERHEADERTYPE *header) {
    (*mCallbacks->EmptyBufferDone)(
            mComponent, mComponent->pApplicationPrivate, header);
}

void SprdOMXComponent::notifyFillBufferDone(OMX_BUFFERHEADERTYPE *header) {
    (*mCallbacks->FillBufferDone)(
            mComponent, mComponent->pApplicationPrivate, header);
}

// static
OMX_ERRORTYPE SprdOMXComponent::SendCommandWrapper(
        OMX_HANDLETYPE component,
        OMX_COMMANDTYPE cmd,
        OMX_U32 param,
        OMX_PTR data) {
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->sendCommand(cmd, param, data);
}

// static
OMX_ERRORTYPE SprdOMXComponent::GetParameterWrapper(
        OMX_HANDLETYPE component,
        OMX_INDEXTYPE index,
        OMX_PTR params) {
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->getParameter(index, params);
}

// static
OMX_ERRORTYPE SprdOMXComponent::SetParameterWrapper(
        OMX_HANDLETYPE component,
        OMX_INDEXTYPE index,
        OMX_PTR params) {
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->setParameter(index, params);
}

// static
OMX_ERRORTYPE SprdOMXComponent::GetConfigWrapper(
        OMX_HANDLETYPE component,
        OMX_INDEXTYPE index,
        OMX_PTR params) {
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->getConfig(index, params);
}

// static
OMX_ERRORTYPE SprdOMXComponent::SetConfigWrapper(
        OMX_HANDLETYPE component,
        OMX_INDEXTYPE index,
        OMX_PTR params) {
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->setConfig(index, params);
}

// static
OMX_ERRORTYPE SprdOMXComponent::GetExtensionIndexWrapper(
        OMX_HANDLETYPE component,
        OMX_STRING name,
        OMX_INDEXTYPE *index) {
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->getExtensionIndex(name, index);
}

// static
OMX_ERRORTYPE SprdOMXComponent::UseBufferWrapper(
        OMX_HANDLETYPE component,
        OMX_BUFFERHEADERTYPE **buffer,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size,
        OMX_U8 *ptr) {
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->useBuffer(buffer, portIndex, appPrivate, size, ptr);
}

// static
OMX_ERRORTYPE SprdOMXComponent::AllocateBufferWrapper(
        OMX_HANDLETYPE component,
        OMX_BUFFERHEADERTYPE **buffer,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size) {
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->allocateBuffer(buffer, portIndex, appPrivate, size);
}

// static
OMX_ERRORTYPE SprdOMXComponent::FreeBufferWrapper(
        OMX_HANDLETYPE component,
        OMX_U32 portIndex,
        OMX_BUFFERHEADERTYPE *buffer) {
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->freeBuffer(portIndex, buffer);
}

// static
OMX_ERRORTYPE SprdOMXComponent::EmptyThisBufferWrapper(
        OMX_HANDLETYPE component,
        OMX_BUFFERHEADERTYPE *buffer) {
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->emptyThisBuffer(buffer);
}

// static
OMX_ERRORTYPE SprdOMXComponent::FillThisBufferWrapper(
        OMX_HANDLETYPE component,
        OMX_BUFFERHEADERTYPE *buffer) {
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->fillThisBuffer(buffer);
}

// static
OMX_ERRORTYPE SprdOMXComponent::GetStateWrapper(
        OMX_HANDLETYPE component,
        OMX_STATETYPE *state) {
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->getState(state);
}

////////////////////////////////////////////////////////////////////////////////

OMX_ERRORTYPE SprdOMXComponent::sendCommand(
        OMX_COMMANDTYPE cmd, OMX_U32 param, OMX_PTR data) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE SprdOMXComponent::getParameter(
        OMX_INDEXTYPE index, OMX_PTR params) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE SprdOMXComponent::setParameter(
        OMX_INDEXTYPE index, const OMX_PTR params) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE SprdOMXComponent::getConfig(
        OMX_INDEXTYPE index, OMX_PTR params) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE SprdOMXComponent::setConfig(
        OMX_INDEXTYPE index, const OMX_PTR params) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE SprdOMXComponent::getExtensionIndex(
        const char *name, OMX_INDEXTYPE *index) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE SprdOMXComponent::useBuffer(
        OMX_BUFFERHEADERTYPE **buffer,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size,
        OMX_U8 *ptr,
        BufferPrivateStruct* bufferPrivate) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE SprdOMXComponent::allocateBuffer(
        OMX_BUFFERHEADERTYPE **buffer,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE SprdOMXComponent::freeBuffer(
        OMX_U32 portIndex,
        OMX_BUFFERHEADERTYPE *buffer) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE SprdOMXComponent::emptyThisBuffer(
        OMX_BUFFERHEADERTYPE *buffer) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE SprdOMXComponent::fillThisBuffer(
        OMX_BUFFERHEADERTYPE *buffer) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE SprdOMXComponent::getState(OMX_STATETYPE *state) {
    return OMX_ErrorUndefined;
}

}  // namespace android
