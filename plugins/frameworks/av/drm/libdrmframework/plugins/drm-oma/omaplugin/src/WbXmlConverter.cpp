#include <WbXmlConverter.hpp>
#include <utils/String8.h>
using namespace android;

const char* WbXmlConverter::DRM_ELEMENT_CODE_PAGE[] = {
    RO_ELEMENT_RIGHTS, RO_ELEMENT_CONTEXT, RO_ELEMENT_VERSION, RO_ELEMENT_UID, RO_ELEMENT_AGREEMENT,
    RO_ELEMENT_ASSET, RO_ELEMENT_KEYINFO, RO_ELEMENT_KEYVALUE, RO_ELEMENT_PERMISSION, RO_ELEMENT_PLAY,
    RO_ELEMENT_DISPLAY, RO_ELEMENT_EXECUTE, RO_ELEMENT_PRINT, RO_ELEMENT_CONSTRAINT, RO_ELEMENT_COUNT,
    RO_ELEMENT_DATE_TIME, RO_ELEMENT_START_TIME, RO_ELEMENT_EXPIRY_TIME, RO_ELEMENT_AVAILABLE_TIME
};

WbXmlConverter::WbXmlConverter(char* buffer, int length)
    :length(length), position(0), state(EXPECT_HEADER)
{
    this->buffer = (char*)malloc(length);
    memcpy(this->buffer, buffer, length);
}

WbXmlConverter::~WbXmlConverter() {
    if (buffer) {
        free(buffer);
        buffer = NULL;
    }

}
int WbXmlConverter::peekByte() {
    int ret = 0;
    memcpy(&ret, buffer+position, 1);
    return ret;
}

int WbXmlConverter::read(char* outBuffer, int size) {
    if (position >= length) {
        return 0;
    }
    if (position+size < length) {
        memcpy(outBuffer, buffer+position, size);
        position += size;
        return size;
    } else {
        memcpy(outBuffer, buffer+position, length-position);
        position = length;
        return length-position;
    }
}

int WbXmlConverter::readByte() {
    int ret = 0;
    read((char*)&ret, 1);
    return ret;
}

int WbXmlConverter::readUint32() {
    int ret = 0;
    char uintFragment=0;
    while (true) {
        read(&uintFragment,1);
        ret = (ret << 7) + (uintFragment & 0x7f);
        if ((uintFragment & (1<<7)) == 0) {
            break;
        }
    }
    return ret;
}

char* WbXmlConverter::convert() {
    String8 ret;
    Vector<char*> tagStack;
    String8 pendingText;
    while (true) {
        switch (state) {
            case EXPECT_HEADER:
            {
                // read header, public id, strtbl
                // version
                int version = readByte();
                SUNWAY_NOISY(ALOGD("WbXmlConverter::version:%d",version));
                // public id
                int publicId = readUint32();
                SUNWAY_NOISY(ALOGD("WbXmlConverter::public id:%d",publicId));
                if (publicId == 0) {
                    int index = readUint32();
                    SUNWAY_NOISY(ALOGD("WbXmlConverter::public id is 0,  str index:%d",index));
                }

                // charset
                int charset = readUint32();
                SUNWAY_NOISY(ALOGD("WbXmlConverter::charset:%d",charset));

                // strtbl
                int length = readUint32();
                SUNWAY_NOISY(ALOGD("WbXmlConverter::strtbl size:%d",length));
                if (length != 0) {
                    char buffer [length];
                    bzero(buffer, length);
                    read(buffer, length);
                    int prevousIndex = 0;
                    for (int i=0; i<length; ++i) {
                        char tmp = buffer[i];
                        if (tmp == NULL) {
                            SUNWAY_NOISY(ALOGD("WbXmlConverter::get strbtl entry: %s",buffer+prevousIndex));
                            strtbl.push(String8(buffer+prevousIndex));
                            prevousIndex = i+1;
                        }
                    }
                }

                state = EXPECT_ELEMENT_START;
                break;
            }
            case EXPECT_ELEMENT_START:
            {
                int stag = readByte();
                char * name = NULL;
                if ((stag & 0x3f) == TOKEN_LITERAL) {
                    name = (char*)strtbl.itemAt(readUint32()).string();
                } else {
                    name = lookupTagName(stag);
                }

                Vector<Pair> attributes;
                // have attributes
                if (stag & 0x80) {
                    while (readByte() != TOKEN_END) {
                    // FIXME skip attributes
                    }
                }
                startElement(ret, name, attributes);
                if (stag & 0x40) {
                    state = EXPECT_CONTENT;
                } else {
                    state = ELEMENT_END;
                }
                tagStack.push(name);
                break;
            }
            case EXPECT_CONTENT:
            {
                int byte = peekByte();
                if (isTagStart(byte) || byte == TOKEN_END) {
                    if (pendingText.size()) {
                        onText(ret, (char*)pendingText.string());
                        pendingText.clear();
                    }
                    if (byte == TOKEN_END) {
                         state = EXPECT_ELEMENT_END;
                    } else {
                         state = EXPECT_ELEMENT_START;
                    }
                } else if (byte == TOKEN_OPAQUE) {
                    // skip TOKEN_OPAQUE
                    readByte();

                    int length = readUint32();
                    char opaqueData[length];
                    bzero(opaqueData, length);
                    read(opaqueData, length);
                    onOpaque(ret, opaqueData, length);
                } else if (byte == TOKEN_ENTITY || byte == TOKEN_STR_I || byte == TOKEN_STR_T) {
                    pendingText.append(readString());
                }

                break;
            }
            case EXPECT_ELEMENT_END:
            {
                // must be end token
                readByte();
                char* tagName = tagStack.top();
                tagStack.pop();
                endElement(ret, tagName);
                if (tagStack.empty()) {
                    state = EXPECT_BODY_END;
                } else {
                    state = EXPECT_CONTENT;
                }
                break;
            }
            case EXPECT_BODY_END:
            {
                SUNWAY_NOISY(ALOGD("WbXmlConverter:: result: %s", ret.string()));
                return strdup(ret.string());
            }
        }
    }
}

// not used yet
Pair WbXmlConverter::readAttribute() {
    Pair ret;
    int attrStart = readByte();
    char * name = NULL;
    char * valuePrefix = NULL;

    if (attrStart == TOKEN_LITERAL) {
        name = (char*)strtbl.itemAt(readUint32()).string();
    } else {
        name = lookupAttrName(attrStart, &valuePrefix);
    }

    ret.name = name;
    ret.value = "";
    if (valuePrefix != NULL) {
        ret.value = valuePrefix;
    }

    for (;;) {
        int valueToken = peekByte();
        if (isAttrStart(valueToken) || valueToken == TOKEN_END) {
            // An attribute start token, a LITERAL token or the END token
            // indicates the end of an attribute value.
            return ret;
        }
        if (isAttrValue(valueToken)) {
            String8 tmp (ret.value);
            tmp.append(lookupAttrValue(valueToken));
            ret.value = (char*)tmp.string();
        } else if (valueToken == TOKEN_ENTITY || valueToken == TOKEN_STR_I || valueToken == TOKEN_STR_T) {
            String8 tmp (ret.value);
            tmp.append(readString());
            ret.value = (char*)tmp.string();
        } else {
            ret.value = "unknown";
            return ret;
        }
    }
}

char* WbXmlConverter::lookupTagName(int stag) {
    int realStag =  stag & 0x3f;
    return (char*)DRM_ELEMENT_CODE_PAGE[realStag - WBXML_EXT_OFFSET];
}

char* WbXmlConverter::lookupAttrValue(int valueToken) {
    return "attr_value";
}

char* WbXmlConverter::lookupAttrName(int start, char** prefix) {
    *prefix = NULL;
    return "attr_name";
}

bool WbXmlConverter::isTagStart(int token) {
    token &= 0x3f;
    return (token >= TOKEN_LITERAL && token < TOKEN_EXT_I_0);
}

bool WbXmlConverter::isAttrStart(int token) {
    return (token >= TOKEN_LITERAL && token < TOKEN_EXT_I_0) ||
        (token > TOKEN_LITERAL_C && token < 0x80);
}

bool WbXmlConverter::isAttrValue(int token) {
    return (token > TOKEN_LITERAL_A && token < 0xC0);
}

String8 WbXmlConverter::readString()
{
    String8 ret;
    int byte = readByte();
    switch (byte) {
        case TOKEN_STR_I:
            //TODO: assuming UTF-8
            while ((byte = readByte()) != 0) {
                ret.appendFormat("%c", byte);
            }
            SUNWAY_NOISY(ALOGD("WbXmlConverter:: readString TOKEN_STR_I: %s", ret.string()));
            break;

        case TOKEN_ENTITY:
        {
            int ch = readUint32();
            if (ch <= 0x7f) {
                ret.appendFormat("%c",(char)ch);
            } else if (ch <= 0x7ff) {
                ret.appendFormat("%c%c",(char)((ch >> 6) | 0xc0),(char)((ch & 0x3f) | 0x80));
            } else if (ch <= 0xffff) {
                ret.appendFormat("%c%c%c",(char)((ch >> 12) | 0xe0),(char)(((ch >> 6) & 0x3f) | 0x80),(char)((ch & 0x3f) | 0x80));
            } else if (ch <= 0x10ffff) {
                // 010000 - 10FFFF
                ret.appendFormat("%c%c%c%c",(char)((ch >> 18) | 0xf0),(char)(((ch >> 12) & 0x3f) | 0x80),(char)(((ch >> 6) & 0x3f) | 0x80),(char)((ch & 0x3f) | 0x80));
            } else {
                // not a valid UCS-4 character
            }
                SUNWAY_NOISY(ALOGD("WbXmlConverter:: readString TOKEN_ENTITY: %s", ret.string()));
            break;
        }

        case TOKEN_STR_T:
        {
            ret = String8(strtbl.itemAt(readUint32()));
            SUNWAY_NOISY(ALOGD("WbXmlConverter:: readString TOKEN_STR_T: %s", ret.string()));
            break;
        }

        default:
            // impossible
            printf ("Unknown token 0x%02x\n", byte);
            break;
    }
    return ret;
}

void WbXmlConverter::startElement(String8& out, char* name, Vector<Pair> attributes) {
    SUNWAY_NOISY(ALOGD("WbXmlConverter:: startElement: %s", name));
    out.appendFormat("<%s>", name);
}

void WbXmlConverter::endElement(String8& out, char* name) {
    SUNWAY_NOISY(ALOGD("WbXmlConverter:: endElement: %s", name));
    out.appendFormat("</%s>", name);
}

void WbXmlConverter::onText(String8& out, char* text) {
    SUNWAY_NOISY(ALOGD("WbXmlConverter:: onText: %s", text));
    out.appendFormat("%s",text);
}

void WbXmlConverter::onOpaque(String8& out, char* buffer, int length) {
    SUNWAY_NOISY(ALOGD("WbXmlConverter:: onOpaque: %d", length));
    char* tmp = base64(buffer, length);
    out.appendFormat("%s", tmp);
    free(tmp);
}
