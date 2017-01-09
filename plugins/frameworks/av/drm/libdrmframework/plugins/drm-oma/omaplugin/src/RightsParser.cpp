#include <RightsParser.hpp>
#include <WbXmlConverter.hpp>
#include <RightsManager.hpp>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <utils/Log.h>

using namespace android;


const static char* actionsName [] = {
    RO_ELEMENT_NIL,
    RO_ELEMENT_PLAY,
    RO_ELEMENT_NIL,
    RO_ELEMENT_NIL,
    RO_ELEMENT_NIL,
    RO_ELEMENT_NIL,
    RO_ELEMENT_NIL,
    RO_ELEMENT_DISPLAY,
};

const static char* constraintsName [] = {
    RO_ELEMENT_COUNT,
    RO_ELEMENT_START_TIME,
    RO_ELEMENT_EXPIRY_TIME,
    RO_ELEMENT_AVAILABLE_TIME,
};

RightsParser::RightsParser(const DrmRights& rights)
    :mimeType(rights.getMimeType()), state(UNKNOWN) {
    for (int i=0; i<ACTION_NUM; ++i) {
        for (int j=0; j<CONSTRAINT_NUM; ++j) {
            constraints[i][j] = UNSET;
        }
    }

    char* origData = rights.getData().data;
    int origLength = rights.getData().length;
    data = (char *) malloc(origLength+1);
    bzero(data, origLength+1);
    memcpy(data, origData, origLength);
    length = origLength;
}

RightsParser::~RightsParser() {
    if (data) {
        free(data);
        data = NULL;
    }
}

bool RightsParser::parse() {
    if (state == ERROR || state == NOT_ACQUIRED) {
        return false;
    }
    if (state != UNKNOWN) {
        return true;
    }

    bool ret = false;
    if (mimeType ==  String8(MIMETYPE_RO)) {
        ret = parseXML();
    } else if (mimeType == String8(MIMETYPE_WB_RO)) {
        ret = parseWBXML();
    } else {
        SUNWAY_NOISY(ALOGE("RightsParser::parse: unknown mimetype: %s", mimeType.string()));
        ret = false;
    }
    if (ret) {
        state = OK;
        if (key.isEmpty()) {
            // no key? means this rights is for CD, so add the fake
            // key...
            RightsManager rm;
            key = rm.getFakeKey();
            SUNWAY_NOISY(ALOGD("RightsParser::parse: no key, fake key = %s", key.string()));
        }
    } else {
        state = ERROR;
    }
    return ret;
}

bool RightsParser::parseXML() {
    TiXmlDocument* myDocument = new TiXmlDocument();

    myDocument->Parse(data);
    if (myDocument->Error()) {
        SUNWAY_NOISY(ALOGE("RightsParser::parseXML: parse fail: %s", myDocument->ErrorDesc()));
        delete myDocument;
        return false;
    }

    TiXmlElement* rootElement = myDocument->RootElement();
    ASSERT_XML_EQUAL(rootElement->Value(), RO_ELEMENT_RIGHTS);

    TiXmlNode * contextNode = rootElement->FirstChild(RO_ELEMENT_CONTEXT);
    ASSERT_XML_NOT_NULL(contextNode);

    TiXmlNode * versionNode = contextNode->FirstChild(RO_ELEMENT_VERSION);
    ASSERT_XML_NOT_NULL(versionNode);

    version = versionNode->FirstChild()->Value();

    TiXmlNode * agreementNode = rootElement->FirstChild(RO_ELEMENT_AGREEMENT);
    ASSERT_XML_NOT_NULL(agreementNode);

    TiXmlNode * assetNode = agreementNode->FirstChild(RO_ELEMENT_ASSET);
    ASSERT_XML_NOT_NULL(assetNode);

    contextNode = assetNode->FirstChild(RO_ELEMENT_CONTEXT);
    ASSERT_XML_NOT_NULL(contextNode);

    TiXmlNode* uidNode = contextNode->FirstChild(RO_ELEMENT_UID);
    ASSERT_XML_NOT_NULL(uidNode);

    char* origUid = (char*)uidNode->FirstChild()->Value();
    SUNWAY_NOISY(ALOGD("RightsParser::parseXML: origUid = %s", origUid));
    if (! strstr(origUid, "cid:")) {
        SUNWAY_NOISY(ALOGE("RightsParser::parseXML: uid wrong: %s", uid.string()));
        delete myDocument;
        return false;
    }

    // skip the leading "cid:"
    uid = origUid+4;

    TiXmlNode*  keyInfoNode = assetNode->FirstChild(RO_ELEMENT_KEYINFO);
    if (keyInfoNode) {
        TiXmlNode* keyValueNode = keyInfoNode->FirstChild(RO_ELEMENT_KEYVALUE);
        if (keyValueNode) {
            key = keyValueNode->FirstChild()->Value();
        }
    }

    TiXmlNode* permissionNode = agreementNode->FirstChild(RO_ELEMENT_PERMISSION);

    if (permissionNode) {
        SUNWAY_NOISY(ALOGD("RightsParser::parseXML: got permissionNode"));
        for (int i=0; i<sizeof(actionsName)/sizeof(char*); ++i) {
            TiXmlNode* actionNode = permissionNode->FirstChild(actionsName[i]);
            SUNWAY_NOISY(ALOGD("RightsParser::parseXML: checking for %s", actionsName[i]));
            if (actionNode) {
                SUNWAY_NOISY(ALOGD("RightsParser::parseXML: got node for %s", actionsName[i]));
                TiXmlNode* tmpNode = actionNode->FirstChild(RO_ELEMENT_CONSTRAINT);
                if (tmpNode) {
                    for (int j=0; j<sizeof(constraintsName)/sizeof(char*); ++j) {
                        TiXmlNode* constraintNode = NULL;
                        if (String8(constraintsName[j]) == String8(RO_ELEMENT_START_TIME) ||
                            String8(constraintsName[j]) == String8(RO_ELEMENT_EXPIRY_TIME)) {
                                constraintNode = tmpNode->FirstChild(RO_ELEMENT_DATE_TIME);
                            if (constraintNode) {
                                constraintNode = constraintNode->FirstChild(constraintsName[j]);
                            }
                        } else {
                            constraintNode = tmpNode->FirstChild(constraintsName[j]);
                        }

                        if (constraintNode) {
                            const char* tmpValue = constraintNode->FirstChild()->Value();
                            SUNWAY_NOISY(ALOGD("RightsParser::parseXML: got node for %s = %s", constraintsName[j], tmpValue));
                            if (String8(constraintsName[j]) == String8(RO_ELEMENT_COUNT)) {
                                if (atoi(tmpValue) < 0) {
                                    constraints[i][j] = 0;
                                } else {
                                    constraints[i][j] = atoi(tmpValue);
                                }
                            } else if (String8(constraintsName[j]) == String8(RO_ELEMENT_AVAILABLE_TIME)) {
                                constraints[i][j+1] = parseRELInterval(tmpValue);
                            } else if ((String8(constraintsName[j]) == String8(RO_ELEMENT_START_TIME))
                                        || (String8(constraintsName[j]) == String8(RO_ELEMENT_EXPIRY_TIME))) {
                                constraints[i][j] = parseRELDateTime(tmpValue);
                            }
                        }
                    }
                }
            }
        }
    }
    delete myDocument;
    return true;
}
int RightsParser::checkRightsStatusForAction(int action) {
    SUNWAY_NOISY(ALOGD("RightsParser::checkRightsStatusForAction: %d", action));
    int64_t* constraintsForAction = constraints[action];
    if (state == NOT_ACQUIRED) {
        SUNWAY_NOISY(ALOGE("RightsParser::checkRightsStatusForAction: not acquired"));
        return RightsStatus::RIGHTS_NOT_ACQUIRED;
    }
    if (state != OK) {
        SUNWAY_NOISY(ALOGE("RightsParser::checkRightsStatusForAction: parse state error"));
        return RightsStatus::RIGHTS_INVALID;
    }

    bool countLimited = (constraintsForAction[COUNT] != UNSET);
    bool intervalLimited = (constraintsForAction[INTERVAL] != UNSET);
    bool timeLimited = (constraintsForAction[DATETIME_START] != UNSET || constraintsForAction[DATETIME_END] != UNSET);

    /* check for COUNT constraint */
    if (countLimited && constraintsForAction[COUNT] != EXPIRED) {
        return RightsStatus::RIGHTS_VALID;
    }

    /* check for INTERVAL constraint */
    if (intervalLimited) {
        if (constraintsForAction[INTERVAL_START] == UNSET) {
            return (constraintsForAction[INTERVAL] == 0 ? RightsStatus::RIGHTS_EXPIRED : RightsStatus::RIGHTS_VALID);
        } else {
            if (0xffffffffffffffff - (uint64_t)constraintsForAction[INTERVAL_START] <= (uint64_t)constraintsForAction[INTERVAL]) {
                return RightsStatus::RIGHTS_VALID;
            } else {
                uint64_t cTime = (uint64_t)getCurrentTime();
                SUNWAY_NOISY(ALOGD("RightsParser::INTERVAL_START: %lld, INTERVAL: %llu, cTime: %llu", constraintsForAction[INTERVAL_START], constraintsForAction[INTERVAL], cTime));
                if (cTime < (uint64_t)constraintsForAction[INTERVAL_START] + (uint64_t)constraintsForAction[INTERVAL]) {
                    return RightsStatus::RIGHTS_VALID;
                }
            }
        }
    }

    /* check for DATETIME constraint */
    time64_t startTime = 0;
    time64_t endTime = 0;

    if (timeLimited) {
        if (constraintsForAction[DATETIME_START] == INFINITY || constraintsForAction[DATETIME_START] == UNSET) {
            startTime = 1;
        } else {
            startTime = constraintsForAction[DATETIME_START];
        }
        if (constraintsForAction[DATETIME_END] == INFINITY || constraintsForAction[DATETIME_END] == UNSET) {
            endTime = ~(1<<63);
        } else {
            endTime = constraintsForAction[DATETIME_END];
        }
        time64_t currentTime = getCurrentTime();
        if (currentTime != TIME_INVALID) {
            if (startTime !=0 && currentTime >= startTime && ((currentTime <= endTime) ||
               (constraintsForAction[DATETIME_END] == INFINITY || constraintsForAction[DATETIME_END] == UNSET))) {
                return RightsStatus::RIGHTS_VALID;
            }
        }
    }

    SUNWAY_NOISY(ALOGE("RightsParser::checkRightsStatusForAction: can't find any constraint, invalid"));
    return RightsStatus::RIGHTS_EXPIRED;
}

DrmConstraints* RightsParser::getConstraintsForAction(int action) {
    DrmConstraints* ret = new DrmConstraints();
    if (state == NOT_ACQUIRED || state != OK) {
        return ret;
    }
    int64_t* constraintsForAction = constraints[action];
    if (constraintsForAction[COUNT] != UNSET) {
        ret->put(&(DrmConstraints::MAX_REPEAT_COUNT), String8::format("%lld",constraintsForAction[COUNT]).string());
        ret->put(&(DrmConstraints::REMAINING_REPEAT_COUNT), String8::format("%lld",constraintsForAction[COUNT]).string());
    }
    if (constraintsForAction[INTERVAL] != UNSET) {
        if (constraintsForAction[INTERVAL_START] != UNSET) {
            ret->put(&(DrmConstraints::EXTENDED_METADATA), String8::format("%lld",constraintsForAction[INTERVAL_START]).string());
        }
        ret->put(&(DrmConstraints::LICENSE_AVAILABLE_TIME), String8::format("%lld",constraintsForAction[INTERVAL]).string());
    }
    if (constraintsForAction[DATETIME_START] != UNSET) {
        ret->put(&(DrmConstraints::LICENSE_START_TIME), String8::format("%lld",constraintsForAction[DATETIME_START]).string());
    }
    if (constraintsForAction[DATETIME_END] != UNSET) {
        ret->put(&(DrmConstraints::LICENSE_EXPIRY_TIME), String8::format("%lld",constraintsForAction[DATETIME_END]).string());
    }
    SUNWAY_NOISY(ALOGD("RightsParser::getConstraintsForAction: counts = %lld, datetime_start = %lld, datetime_end = %lld, interval_start = %lld, interval = %lld", constraintsForAction[COUNT], constraintsForAction[DATETIME_START], constraintsForAction[DATETIME_END], constraintsForAction[INTERVAL_START], constraintsForAction[INTERVAL]));
    return ret;
}

time64_t RightsParser::parseRELDateTime(const char* date) {
    // 2010-01-01T08:00:00
    // 2010-06-16T19:23:00
    // CCYY-MM-DDThh:mm:ss
    struct tm tmp;
    bzero(&tmp, sizeof(tmp));
    char* ret = strptime(date, "%Y-%m-%dT%H:%M:%S", &tmp);
    if (!ret) {
        SUNWAY_NOISY(ALOGE("RightsParser::parseRELDateTime: wrong date format: %s", date));
        return EXPIRED;
    }
    return mktime64(&tmp);
}

int64_t RightsParser::parseRELInterval(const char* interval) {
    // P0Y0M0DT0H10M0S
    int year    = 0;
    int month   = 0;
    int day     = 0;
    int hour    = 0;
    int minute  = 0;
    int second  = 0;

    int ret = sscanf(interval, "P%dY%dM%dDT%dH%dM%dS", &year, &month, &day, &hour, &minute, &second);
    if (ret != 6) {
        SUNWAY_NOISY(ALOGE("RightsParser::parseRELInterval: wrong interval format: %s", interval));
        return EXPIRED;
    }
    int64_t relInterval = second + minute*60 + hour*3600 + day*24*3600 + month*31*24*3600 + year*366*24*3600;
    SUNWAY_NOISY(ALOGD("RightsParser::parseRELInterval: interval = %llu", relInterval));
//    return relInterval > 0 ? relInterval : EXPIRED;
    return relInterval;
}

bool RightsParser::parseWBXML() {
    WbXmlConverter converter(data, length);
    char* plainXml = converter.convert();
    if (! plainXml) {
        return false;
    }
    free(data);
    data = plainXml;
    length = strlen(plainXml);
    mimeType ==  String8(MIMETYPE_RO);
    return parseXML();
}
