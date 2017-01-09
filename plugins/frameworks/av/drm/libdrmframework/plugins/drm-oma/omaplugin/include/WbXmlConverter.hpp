#ifndef WB_XML_CONVERTER_HPP
#define WB_XML_CONVERTER_HPP
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <assert.h>
#include <common.hpp>
#include <utils/Vector.h>
#include <utils/String8.h>

#define TOKEN_SWITCH_PAGE       0x0
#define TOKEN_END               0x1
#define TOKEN_ENTITY            0x2
#define TOKEN_STR_I             0x3
#define TOKEN_LITERAL           0x4
#define TOKEN_EXT_I_0           0x40
#define TOKEN_EXT_I_1           0x41
#define TOKEN_EXT_I_2           0x42
#define TOKEN_PI                0x43
#define TOKEN_LITERAL_C         0x44
#define TOKEN_EXT_T_0           0x80
#define TOKEN_EXT_T_1           0x81
#define TOKEN_EXT_T_2           0x82
#define TOKEN_STR_T             0x83
#define TOKEN_LITERAL_A         0x84
#define TOKEN_EXT_0             0xc0
#define TOKEN_EXT_1             0xc1
#define TOKEN_EXT_2             0xc2
#define TOKEN_OPAQUE            0xc3
#define TOKEN_LITERAL_AC        0xc4

#define WBXML_EXT_OFFSET        5

// for test
#define CARD                    6
#define BR                      5
#define XYZ                     7

namespace android {
    struct Pair {
        char* name;
        char* value;
    };

    class WbXmlConverter {

    private:
        static const char* DRM_ELEMENT_CODE_PAGE[19];
        enum ParserState {
            EXPECT_HEADER,
            EXPECT_ELEMENT_START,
            EXPECT_ELEMENT_END,
            ELEMENT_END,
            EXPECT_CONTENT,
            EXPECT_BODY_END,
        };

        ParserState state;

        char* buffer;
        int length;
        int position;

        Vector<String8> strtbl;

        int readByte();
        int peekByte();
        int readUint32();
        String8 readString();
        Pair readAttribute();
        int read(char* buffer, int size);

        char* lookupTagName(int stag);
        char* lookupAttrName(int start, char** prefix);
        char* lookupAttrValue(int start);

        void startElement(String8& out, char* name, Vector<Pair> attributes);
        void endElement(String8& out, char* name);
        void onText(String8& out, char* name);
        void onOpaque(String8& out, char* buffer, int length);
        static bool isTagStart(int token);
        static bool isAttrStart(int token);
        static bool isAttrValue(int token);
    public:
        WbXmlConverter(char* buffer, int length);
        virtual ~WbXmlConverter();
        char* convert();
    };
};

#endif // WB_XML_CONVERTER_HPP
