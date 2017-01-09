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

//#define LOG_NDEBUG 0
#define LOG_TAG "DrmOmaPlugIn"
#include <utils/Log.h>

#include <drm/DrmRights.h>
#include <drm/DrmConstraints.h>
#include <drm/DrmMetadata.h>
#include <drm/DrmInfo.h>
#include <drm/DrmInfoEvent.h>
#include <drm/DrmInfoStatus.h>
#include <drm/DrmConvertedStatus.h>
#include <drm/DrmInfoRequest.h>
#include <drm/DrmSupportInfo.h>
#include <DrmOmaPlugIn.hpp>
#include <RightsManager.hpp>
#include <DcfParser.hpp>
#include <DcfCreator.hpp>
#include <DmParser.hpp>
#include <UUID.hpp>
#include <private/android_filesystem_config.h>

using namespace android;

const String8 DrmOmaPlugIn::supportedFileSuffix[] = {String8(".dm"),String8(".dcf"),String8(".dr"),String8(".drc")};

// This extern "C" is mandatory to be managed by TPlugInManager
extern "C" IDrmEngine* create() {
    return new DrmOmaPlugIn();
}

// This extern "C" is mandatory to be managed by TPlugInManager
extern "C" void destroy(IDrmEngine* pPlugIn) {
    delete pPlugIn;
    pPlugIn = NULL;
}

DrmOmaPlugIn::DrmOmaPlugIn()
    : DrmEngineBase() {

}

DrmOmaPlugIn::~DrmOmaPlugIn() {

}

DrmMetadata* DrmOmaPlugIn::onGetMetadata(int uniqueId, const String8* path) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onGetMetadata From Path: %s", path->string()));
    DcfParser parser(path->string());
    if (! parser.parse()) {
        return NULL;
    }
    return parser.getMetadata();
}

DrmConstraints* DrmOmaPlugIn::onGetConstraints(int uniqueId, const String8* path, int action) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onGetConstraints From Path: %s", path->string()));
    DcfParser parser(path->string());
    if (! parser.parse()) {
        return new DrmConstraints();
    }
    return rightsManager.getConstraints(parser.contentUri, action);
}

#ifdef USE_LEGACY
DrmInfoStatus* DrmOmaPlugIn::onProcessDrmInfo(int uniqueId, const DrmInfo* drmInfo) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onProcessDrmInfo - Enter :  %d", uniqueId));
    /* mem leak? */
    const DrmBuffer* emptyBuffer = new DrmBuffer();
    DrmInfoStatus* errorStatus = new DrmInfoStatus(DrmInfoStatus::STATUS_ERROR,
           DrmInfoRequest::TYPE_REGISTRATION_INFO, emptyBuffer, drmInfo->getMimeType());

    if (drmInfo == NULL) {
        return errorStatus;
    }

    String8 drmFilePath = drmInfo->get(String8(KEY_REQUEST_FILE_IN));
    String8 outputFilePath = drmInfo->get(String8(KEY_REQUEST_FILE_OUT));

//    String8 drmFilePath = redirectPath(drmInfo->get(String8(KEY_REQUEST_FILE_IN)));
//    String8 outputFilePath = redirectPath(drmInfo->get(String8(KEY_REQUEST_FILE_OUT)));
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onProcessDrmInfo - parse %s", drmFilePath.string()));

    if (drmFilePath.isEmpty() || outputFilePath.isEmpty() || outputFilePath.getPathExtension() != String8(".dcf")) {
        return errorStatus;
    }

    DmParser parser(drmFilePath);
    if (! parser.parse()) {
        return errorStatus;
    }
    // save rights
    char* key = NULL;
    if (parser.hasRights()) {
        DrmRights rights = parser.getDrmRights();
        RightsParser rightsParser (rights);
        if (! rightsParser.parse()) {
            return errorStatus;
        }
        /* CD type should generate an unique uid, we generate it */
        if (strcmp(parser.dataContentId, FAKE_UID)) {
            UUID uuid;
            parser.dataContentId = uuid.generate();
            rightsParser.uid = parser.dataContentId;
        }
        if (rightsManager.saveRights(rightsParser) != DRM_NO_ERROR) {
            return errorStatus;
        }
        key = (char*)rightsParser.key.string();
    } else {
        // no rights? FL
        key = (char*)rightsManager.getFakeKey().string();
    }
    // convert content
    DcfCreator creator ((char*)(outputFilePath.string()), key, parser.dataContentType, parser.dataContentId);

    char buffer[1024] = {0};
    int i = 0;
    while ((i = parser.read(buffer, 1024)) > 0) {
        if (creator.write(buffer, i) == -1) {
            return errorStatus;
        }
    }
    if (! creator.save()) {
        return errorStatus;
    }

        chmod(outputFilePath.string(), 0760);
        if (chown(outputFilePath.string(), -1, AID_MEDIA_RW) < 0) {
            SUNWAY_NOISY(ALOGE("DrmOmaPlugIn::onProcessDrmInfo - fail to chown %s, %s", outputFilePath.string(), strerror(errno)));
        }

    DrmInfoStatus* okStatus = new DrmInfoStatus(DrmInfoStatus::STATUS_OK,
                                        DrmInfoRequest::TYPE_REGISTRATION_INFO, emptyBuffer, drmInfo->getMimeType());
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onProcessDrmInfo - Exit"));
    return okStatus;
}
#else
DrmInfoStatus* DrmOmaPlugIn::onProcessDrmInfo(int uniqueId, const DrmInfo* drmInfo) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onProcessDrmInfo - Enter :  %d", uniqueId));
    const DrmBuffer* emptyBuffer = new DrmBuffer();
    DrmInfoStatus* errorStatus = new DrmInfoStatus(DrmInfoStatus::STATUS_ERROR,
                                           DrmInfoRequest::TYPE_REGISTRATION_INFO, emptyBuffer, drmInfo->getMimeType());

    if (drmInfo == NULL) {
        SUNWAY_NOISY(ALOGE("DrmOmaPlugIn::onProcessDrmInfo - drminfo is null!"));
        return errorStatus;
    }

    String8 drmFilePath = drmInfo->get(String8(KEY_REQUEST_FILE_IN));
    String8 outputFilePath = drmInfo->get(String8(KEY_REQUEST_FILE_OUT));
//    String8 drmFilePath = redirectPath(drmInfo->get(String8(KEY_REQUEST_FILE_IN)));
//    String8 outputFilePath = redirectPath(drmInfo->get(String8(KEY_REQUEST_FILE_OUT)));
    String8 convertID = drmInfo->get(String8(KEY_CONVERT_ID));
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onProcessDrmInfo - parse %s, convertID = %s", drmFilePath.string(), convertID.string()));

    if (drmFilePath.isEmpty() || convertID.isEmpty()) {
        return errorStatus;
    }

    DmParser* parser = new DmParser(drmFilePath);
    if (! parser->parse()) {
        SUNWAY_NOISY(ALOGE("DrmOmaPlugIn::onProcessDrmInfo - dmparser failed!"));
        if (parser) delete parser;
            return errorStatus;
    }
    // save rights
    String8 key;
    if (parser->hasRights()) {
        DrmRights rights = parser->getDrmRights();
        RightsParser rightsParser (rights);
        if (! rightsParser.parse()) {
            SUNWAY_NOISY(ALOGE("DrmOmaPlugIn::onProcessDrmInfo - rightsParser failed!"));
            if (parser) delete parser;
            return errorStatus;
        }
        /* CD type should generate an unique uid, we generate it */
        if (strcmp(parser->dataContentId, FAKE_UID)) {
        UUID uuid;
        parser->dataContentId = uuid.generate();
        rightsParser.uid = parser->dataContentId;
        }
        if (rightsManager.saveRights(rightsParser) != DRM_NO_ERROR) {
            SUNWAY_NOISY(ALOGE("DrmOmaPlugIn::onProcessDrmInfo - saveRights failed!"));
            if (parser) delete parser;
            return errorStatus;
        }
        key = rightsParser.key;
    } else {
        // no rights? FL
        key = rightsManager.getFakeKey();
        free(parser->dataContentId);
        parser->dataContentId = strdup(rightsManager.getFakeUid().string());
    }
    // convert content
    DcfCreator* creator = new DcfCreator((char*)(outputFilePath.string()), (char*)key.string(), parser->dataContentType, parser->dataContentId);

    if (!creator->convertHeaders()) {
        if (parser) delete parser;
        if (creator) delete creator;
        SUNWAY_NOISY(ALOGE("DrmOmaPlugIn::onProcessDrmInfo - convertHeaders failed!"));
        return errorStatus;
    }

    delete emptyBuffer;
    delete errorStatus;
    size_t buffSize = creator->headers.size();
    char* buff = new char[buffSize];
    memcpy(buff, creator->headers.string(), buffSize);
    const DrmBuffer* headBuffer = new DrmBuffer(buff, buffSize);
    DrmInfoStatus* okStatus = new DrmInfoStatus(DrmInfoStatus::STATUS_OK,
                                                DrmInfoRequest::TYPE_REGISTRATION_INFO, headBuffer, drmInfo->getMimeType());
    DrmConvertEntity* entity = new DrmConvertEntity(creator, parser);
    int id = atoi(convertID.string());
    mConvertSessionMap.addValue(id, entity);
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onProcessDrmInfo - Exit"));
    return okStatus;
}
#endif

status_t DrmOmaPlugIn::onSetOnInfoListener(
    int uniqueId, const IDrmEngine::OnInfoListener* infoListener) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onSetOnInfoListener : %d", uniqueId));
    return DRM_NO_ERROR;
}

status_t DrmOmaPlugIn::onInitialize(int uniqueId) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onInitialize : %d", uniqueId));
    return DRM_NO_ERROR;
}

status_t DrmOmaPlugIn::onTerminate(int uniqueId) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onTerminate : %d", uniqueId));
    return DRM_NO_ERROR;
}

DrmSupportInfo* DrmOmaPlugIn::onGetSupportInfo(int uniqueId) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onGetSupportInfo : %d", uniqueId));
    DrmSupportInfo* drmSupportInfo = new DrmSupportInfo();
    // add mimetype's
    drmSupportInfo->addMimeType(String8(MIMETYPE_DM)); // .dm
    drmSupportInfo->addMimeType(String8(MIMETYPE_DCF)); // .dcf
    drmSupportInfo->addMimeType(String8(MIMETYPE_RO)); // .dr
    drmSupportInfo->addMimeType(String8(MIMETYPE_WB_RO)); // .drc
    // add file suffixes
    for (int i = 0; i < sizeof(supportedFileSuffix)/sizeof(String8); ++i) {
        drmSupportInfo->addFileSuffix(supportedFileSuffix[i]);
    }
    // Add plug-in description
    drmSupportInfo->setDescription(String8("omav1 plug-in"));
    return drmSupportInfo;
}

status_t DrmOmaPlugIn::onSaveRights(int uniqueId, const DrmRights& drmRights,
                                    const String8& rightsPath, const String8& contentPath) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onSaveRights : %d, mimetype = %s", uniqueId, drmRights.getMimeType().string()));
    // rightsPath and contentPath are just not used
    RightsParser parser(drmRights);
    if (! parser.parse()) {
        return DRM_ERROR_CANNOT_HANDLE;
    }
    return rightsManager.saveRights(parser);
}

#ifdef USE_LEGACY
DrmInfo* DrmOmaPlugIn::onAcquireDrmInfo(int uniqueId, const DrmInfoRequest* drmInfoRequest) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onAcquireDrmInfo : %d", uniqueId));
    DrmInfo* drmInfo = NULL;

    if (drmInfoRequest == NULL) {
        return NULL;
    }

    String8 drmFilePath = drmInfoRequest->get(String8(KEY_REQUEST_FILE_IN));
    String8 outputFilePath = drmInfoRequest->get(String8(KEY_REQUEST_FILE_OUT));
//    String8 drmFilePath = redirectPath(drmInfoRequest->get(String8(KEY_REQUEST_FILE_IN)));
//    String8 outputFilePath = redirectPath(drmInfoRequest->get(String8(KEY_REQUEST_FILE_OUT)));

    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onAcquireDrmInfo 1: %s->%s", drmFilePath.string(), outputFilePath.string()));
    if (drmFilePath.isEmpty() || outputFilePath.isEmpty() || outputFilePath.getPathExtension() != String8(".dcf")) {
        return NULL;
    }
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onAcquireDrmInfo 2"));

    DrmInfo* ret = new DrmInfo(drmInfoRequest->getInfoType(),DrmBuffer(strdup("nil"),3), drmInfoRequest->getMimeType());
    ret->put (String8(KEY_REQUEST_FILE_IN), drmFilePath);
    ret->put(String8(KEY_REQUEST_FILE_OUT), outputFilePath);

    return ret;
}
#else
DrmInfo* DrmOmaPlugIn::onAcquireDrmInfo(int uniqueId, const DrmInfoRequest* drmInfoRequest) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onAcquireDrmInfo : %d", uniqueId));
    DrmInfo* drmInfo = NULL;

    if (drmInfoRequest == NULL) {
        return NULL;
    }

    String8 drmFilePath = drmInfoRequest->get(String8(KEY_REQUEST_FILE_IN));
    String8 outputFilePath = drmInfoRequest->get(String8(KEY_REQUEST_FILE_OUT));
//    String8 drmFilePath = redirectPath(drmInfoRequest->get(String8(KEY_REQUEST_FILE_IN)));
//    String8 outputFilePath = redirectPath(drmInfoRequest->get(String8(KEY_REQUEST_FILE_OUT)));
    String8 convertID = drmInfoRequest->get(String8(KEY_CONVERT_ID));

    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onAcquireDrmInfo 1: %s->%s", drmFilePath.string(), outputFilePath.string()));
    if (drmFilePath.isEmpty() || convertID.isEmpty()) {
        return NULL;
    }
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onAcquireDrmInfo 2"));

    DrmInfo* ret = new DrmInfo(drmInfoRequest->getInfoType(),DrmBuffer(strdup("nil"),3), drmInfoRequest->getMimeType());
    ret->put (String8(KEY_REQUEST_FILE_IN), drmFilePath);
    ret->put(String8(KEY_REQUEST_FILE_OUT), outputFilePath);
    ret->put(String8(KEY_CONVERT_ID), convertID);

    return ret;
}
#endif

bool DrmOmaPlugIn::onCanHandle(int uniqueId, const String8& path) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::canHandle: %s ", path.string()));
    String8 extension = path.getPathExtension();
    extension.toLower();
    for (int i = 0; i < sizeof(supportedFileSuffix)/sizeof(String8); ++i) {
        if (supportedFileSuffix[i] == extension) {
            return true;
        }
    }
    return false;
}

String8 DrmOmaPlugIn::onGetOriginalMimeType(int uniqueId, const String8& path, int fd) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onGetOriginalMimeType() : %d", uniqueId));
        if (path.getPathExtension() != ".dcf") {
            SUNWAY_NOISY(ALOGE("DrmOmaPlugIn::onGetOriginalMimeType %s failed: can only parse dcf filetype!", path.string()));
            return String8(DCF_UNKNOWN_CONTENT_TYPE);
        }
    DcfParser parser = DcfParser(path.string());
    bool isDcf = parser.parse();
    String8 ret;
    if (isDcf) {
        ret = parser.contentType;
    } else {
        ret = String8(DCF_UNKNOWN_CONTENT_TYPE);
    }
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onGetOriginalMimeType() for %s returns %s", path.string(), ret.string()));
    return ret;
}

int DrmOmaPlugIn::onGetDrmObjectType(
    int uniqueId, const String8& path, const String8& mimeType) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onGetDrmObjectType() : %d for %s, mimeType: %s", uniqueId, path.string(), mimeType.string()));
    if (mimeType == String8("application/vnd.oma.drm.message")) {
        return DrmObjectType::TRIGGER_OBJECT;
    }
    if (mimeType == String8("application/vnd.oma.drm.content")) {
        return DrmObjectType::CONTENT;
    }
    if (mimeType == String8("application/vnd.oma.drm.rights+xml")
       || mimeType == String8("application/vnd.oma.drm.rights+wbxml")) {
        return DrmObjectType::RIGHTS_OBJECT;
    }

    String8 extension = path.getPathExtension();
    extension.toLower();

    if (extension == String8(".dcf")) {
        return DrmObjectType::CONTENT;
    }
    if (extension == String8(".dm")) {
        return DrmObjectType::TRIGGER_OBJECT;
    }
    if (extension == String8(".dr")
        || extension == String8(".drc")
    ) {
        return DrmObjectType::RIGHTS_OBJECT;
    }

    return DrmObjectType::UNKNOWN;
}

int DrmOmaPlugIn::onCheckRightsStatus(int uniqueId, const String8& path, int action) {
    SUNWAY_NOISY (ALOGD("DrmOmaPlugIn::onCheckRightsStatus() : %d, path: %s, action: %d", uniqueId, path.string(), action));
    DcfParser parser(path);
    if (! parser.parse()) {
        return RightsStatus::RIGHTS_INVALID;
    }
    // special case for SD & TRANSFER
    if (action == Action::TRANSFER &&
// parser.contentUri != String8(FAKE_UID) &&
        parser.contentUri.find(FL_PREFIX, 0) == -1 &&
                !parser.rightsIssuer.isEmpty()){
        return RightsStatus::RIGHTS_VALID;
    }
    if (Action::DEFAULT == action) {
        return rightsManager.checkRightsStatus(parser.contentUri, parser.mDefaultAct);
    } else {
        return rightsManager.checkRightsStatus(parser.contentUri, action);
    }
}

status_t DrmOmaPlugIn::onConsumeRights(int uniqueId, DecryptHandle* decryptHandle,
                                       int action, bool reserve) {
    SUNWAY_NOISY (ALOGD("DrmOmaPlugIn::onConsumeRights() : %d", uniqueId));
    return DRM_NO_ERROR;
}

status_t DrmOmaPlugIn::onSetPlaybackStatus(int uniqueId, DecryptHandle* decryptHandle,
                                           int playbackStatus, int64_t position) {
    SUNWAY_NOISY (ALOGD("DrmOmaPlugIn::onSetPlaybackStatus() : %d", playbackStatus));
    DcfParser* parser = decodeSessionMap.getValue(decryptHandle->decryptId);
    RightsConsumer* consumer = consumeSessionMap.getValue(decryptHandle->decryptId);
    if (consumer) {
        consumer->setPlaybackStatus(playbackStatus);
        if (consumer->shouldConsume()) {
            RightsParser rightsParser = rightsManager.query(parser->contentUri);
            SUNWAY_NOISY (ALOGD("DrmOmaPlugIn::before parse: version: %s", rightsParser.version.string()));
            if (rightsParser.parse()) {
                SUNWAY_NOISY (ALOGD("DrmOmaPlugIn::before consume: version: %s", rightsParser.version.string()));
                if (consumer->consume(rightsParser)) {
                    SUNWAY_NOISY (ALOGD("DrmOmaPlugIn::after consume: version: %s", rightsParser.version.string()));
                    rightsManager.saveRights(rightsParser);
                }
            }
        }
    }
    return DRM_NO_ERROR;
}

bool DrmOmaPlugIn::onValidateAction(int uniqueId, const String8& path,
                                    int action, const ActionDescription& description) {
    SUNWAY_NOISY (ALOGD("DrmOmaPlugIn::onValidateAction() : %d", uniqueId));
    return true;
}

status_t DrmOmaPlugIn::onRemoveRights(int uniqueId, const String8& path) {
    SUNWAY_NOISY (ALOGD("DrmOmaPlugIn::onRemoveRights() : %d", uniqueId));
    return DRM_NO_ERROR;
}

status_t DrmOmaPlugIn::onRemoveAllRights(int uniqueId) {
    SUNWAY_NOISY (ALOGD("DrmOmaPlugIn::onRemoveAllRights() : %d", uniqueId));
    return DRM_NO_ERROR;
}

status_t DrmOmaPlugIn::onOpenConvertSession(int uniqueId, int convertId) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onOpenConvertSession() : %d", uniqueId));
    return DRM_NO_ERROR;
}

DrmConvertedStatus* DrmOmaPlugIn::onConvertData(
            int uniqueId, int convertId, const DrmBuffer* inputData) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onConvertData() : uniqueId = %d, convertId = %d", uniqueId, convertId));
    DrmConvertEntity* entity = mConvertSessionMap.getValue(convertId);

    if (!entity) return new DrmConvertedStatus(DrmConvertedStatus::STATUS_ERROR, 0, 0);
    char inbuff[CONVERT_SIZE];
    bzero(inbuff, CONVERT_SIZE);
    ssize_t bytesReaded = entity->mParser->read(inbuff, CONVERT_SIZE);
    if (bytesReaded > 0) {
        ssize_t bytesToWrite = 0;
        char* tmp = entity->mCreater->convert(inbuff, bytesReaded, &bytesToWrite);
        if (tmp && bytesToWrite) {
            SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onConvertData() : bytes converted = %d", bytesToWrite));
            char* outbuff = new char[bytesToWrite];
            memcpy(outbuff, tmp, bytesToWrite);
            delete[] tmp;
            DrmBuffer* convertedData = new DrmBuffer(outbuff, bytesToWrite);
            return new DrmConvertedStatus(DrmConvertedStatus::STATUS_OK, convertedData, 0);
        } else {
            SUNWAY_NOISY(ALOGE("DrmOmaPlugIn::onConvertData() : convert error!"));
            return new DrmConvertedStatus(DrmConvertedStatus::STATUS_ERROR, 0, 0);
        }

    } else if (bytesReaded == 0) {
        SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onConvertData() : bytes converted final"));
        return new DrmConvertedStatus(DrmConvertedStatus::STATUS_OK, 0, 0);

    } else {
        SUNWAY_NOISY(ALOGE("DrmOmaPlugIn::onConvertData() : convert error!"));
        return new DrmConvertedStatus(DrmConvertedStatus::STATUS_ERROR, 0, 0);
    }
}

DrmConvertedStatus* DrmOmaPlugIn::onCloseConvertSession(int uniqueId, int convertId) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onCloseConvertSession() : uniqueId = %d, convertId = %d", uniqueId, convertId));
    DrmConvertEntity* entity = mConvertSessionMap.getValue(convertId);

    if (!entity) return new DrmConvertedStatus(DrmConvertedStatus::STATUS_ERROR, 0, 0);

    char buff[1024];
    bzero(buff, 1024);
    int bytesToWrite = entity->mCreater->convertend(buff);
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onCloseConvertSession() : padding size = %d", bytesToWrite));
    mConvertSessionMap.removeValue(convertId);
    if (bytesToWrite) {
        char* outbuff = new char[bytesToWrite];
        memcpy(outbuff, buff, bytesToWrite);
        DrmBuffer* convertedData = new DrmBuffer(outbuff, bytesToWrite);
        return new DrmConvertedStatus(DrmConvertedStatus::STATUS_OK, convertedData, 0);
    } else {
        return new DrmConvertedStatus(DrmConvertedStatus::STATUS_OK, 0, 0);
    }
}

status_t DrmOmaPlugIn::onOpenDecryptSession(
            int uniqueId, DecryptHandle* decryptHandle, int fd, off64_t offset, off64_t length) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onOpenDecryptSession() : offset:%d, length:%d, fd:%d", (int)offset,(int)length, fd));
    DcfParser* parser = new DcfParser(dup(fd));
    ::close(fd);
    if (!parser->parse()) {
        SUNWAY_NOISY(ALOGE("DrmOmaPlugIn::onOpenDecryptSession() : dcf parse error"));
        if (parser) delete parser;
        return DRM_ERROR_CANNOT_HANDLE;
    }
    RightsParser rightsParser = rightsManager.query(parser->contentUri);
    if (! rightsParser.parse()) {
        SUNWAY_NOISY(ALOGE("DrmOmaPlugIn::onOpenDecryptSession() : rights parse error"));
        if (parser) delete parser;
        return DRM_ERROR_CANNOT_HANDLE;
    }

    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn:: set key: %s", rightsParser.key.string()));
    parser->setKey((char*)rightsParser.key.string());

    decodeSessionMap.addValue(decryptHandle->decryptId, parser);
    consumeSessionMap.addValue(decryptHandle->decryptId, new RightsConsumer());

    decryptHandle->mimeType = parser->contentType;
    decryptHandle->decryptApiType = DecryptApiType::CONTAINER_BASED;
    decryptHandle->status = rightsManager.checkRightsStatus(parser->contentUri, parser->mDefaultAct);
    // decryptHandle->status = DRM_NO_ERROR;
    decryptHandle->decryptInfo = NULL;
    return DRM_NO_ERROR;
}

status_t DrmOmaPlugIn::onOpenDecryptSession(
            int uniqueId, DecryptHandle* decryptHandle, const char* uri) {
    return DRM_ERROR_CANNOT_HANDLE;
}

status_t DrmOmaPlugIn::onCloseDecryptSession(int uniqueId, DecryptHandle* decryptHandle) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onCloseDecryptSession() : %d", uniqueId));
    if (NULL != decryptHandle) {
        decodeSessionMap.removeValue(decryptHandle->decryptId);
        consumeSessionMap.removeValue(decryptHandle->decryptId);
        if (NULL != decryptHandle->decryptInfo) {
            delete decryptHandle->decryptInfo; decryptHandle->decryptInfo = NULL;
        }
        delete decryptHandle; decryptHandle = NULL;
    }
    return DRM_NO_ERROR;
}

status_t DrmOmaPlugIn::onInitializeDecryptUnit(int uniqueId, DecryptHandle* decryptHandle,
            int decryptUnitId, const DrmBuffer* headerInfo) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onInitializeDecryptUnit() : %d", uniqueId));
    return DRM_NO_ERROR;
}

status_t DrmOmaPlugIn::onDecrypt(int uniqueId, DecryptHandle* decryptHandle,
            int decryptUnitId, const DrmBuffer* encBuffer, DrmBuffer** decBuffer, DrmBuffer* IV) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onDecrypt() : %d", uniqueId));
    /**
     * As a workaround implementation oma would copy the given
     * encrypted buffer as it is to decrypted buffer. Note, decBuffer
     * memory has to be allocated by the caller.
     */
    if (NULL != (*decBuffer) && 0 < (*decBuffer)->length) {
        if ((*decBuffer)->length >= encBuffer->length) {
            memcpy((*decBuffer)->data, encBuffer->data, encBuffer->length);
            (*decBuffer)->length = encBuffer->length;
        } else {
            SUNWAY_NOISY(ALOGE("decBuffer size (%d) too small to hold %d bytes", (*decBuffer)->length, encBuffer->length));
            return DRM_ERROR_UNKNOWN;
        }
    }
    return DRM_NO_ERROR;
}

status_t DrmOmaPlugIn::onFinalizeDecryptUnit(
            int uniqueId, DecryptHandle* decryptHandle, int decryptUnitId) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onFinalizeDecryptUnit() : %d", uniqueId));
    return DRM_NO_ERROR;
}

#define GET_SIZE_MAGIC          0xDEADBEEF
#define PADDING_SNIFF_SIZE      512
/*
 * In this func, we have a secreat code which is the "DEADBEEF".
 * If the incoming offset equals to the magic num, the pread will do NOT read anything,
 * but return the plain text length quickly.
 * This is a workaround as the google drm framework lack of a getSize like func.
 */
ssize_t DrmOmaPlugIn::onPread(int uniqueId, DecryptHandle* decryptHandle,
            void* buffer, ssize_t numBytes, off64_t offset) {
    SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onPread() : numBytes: %d, offset: %d", (int)numBytes, (int)offset));
    DcfParser* parser = decodeSessionMap.getValue(decryptHandle->decryptId);

    if (parser) {
        if (GET_SIZE_MAGIC == offset && parser->fd != -1) {
            off64_t rawsize = ::lseek(parser->fd, 0, SEEK_END);
            offset = 0;
            if (rawsize != -1) {
                char buf[1024] = {0};
                /*
                 * read the cipher text from the last padding sniff area, so that we can get
                 * the real plain text length more quickly
                 */
                int paddingSize = parser->readAt(buf, 1024, rawsize - PADDING_SNIFF_SIZE);
                        offset = rawsize - PADDING_SNIFF_SIZE + paddingSize;
            }

            SUNWAY_NOISY(ALOGD("DrmOmaPlugIn::onPread() for getSize only! rawsize = %lld, realsize = %lld", rawsize, offset));
            return offset;
        } else {
            return parser->readAt((char*)buffer, numBytes, (int)offset);
        }
    } else {
        SUNWAY_NOISY(ALOGE("DrmOmaPlugIn::onPread(): no parser"));
        return -1;
    }
}

