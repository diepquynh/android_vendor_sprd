#include <RightsParser.hpp>
#include <drm/DrmRights.h>
#include <common.hpp>
#include <iostream>
#include <ReadWriteUtils.h>

#include <DcfParser.hpp>
#include <DcfCreator.hpp>
#include <RightsManager.hpp>
#include <DmParser.hpp>
#include <WbXmlConverter.hpp>
#include <UUID.hpp>

#include <utils/Log.h>
#include <sys/ioctl.h>
#include <gtest/gtest.h>
// #include <linux/android_alarm.h>

using namespace android;
using std::cout;
using std::endl;

TEST(DmParser, parse) {
    RightsManager manager;
    DmParser dmParser("/data/drm/test/test.dm");
    ASSERT_TRUE(dmParser.parse());
    ASSERT_TRUE(dmParser.hasRights());
    RightsParser rightsParser  (dmParser.getDrmRights());
    ASSERT_TRUE(rightsParser.parse());
}

TEST(RightsParser, parse) {
    String8 buffer = ReadWriteUtils::readBytes(String8("/data/drm/test/test.dr"));
    DrmRights rights (DrmBuffer((char*)(buffer.string()), buffer.length()), String8(MIMETYPE_RO));

    RightsParser parser(rights);
    ASSERT_TRUE(parser.parse());
    ASSERT_STREQ(parser.version, "1.0");
    cout << "version: "<<parser.version << " uid: " << parser.uid << " key: " << parser.key << endl;
    SUCCEED();
}

TEST(WbXmlConverter, convert) {
    String8 buffer = ReadWriteUtils::readBytes(String8("/data/drm/test/test.drc"));
    DrmRights rights (DrmBuffer((char*)(buffer.string()), buffer.length()), String8(MIMETYPE_WB_RO));

    RightsParser parser(rights);
    ASSERT_TRUE(parser.parse());
    cout << "version: "<<parser.version << " uid: " << parser.uid << " key: " << parser.key << endl;
    SUCCEED();
}

TEST(RightsManager, query) {
    RightsManager manager;
    // RightsParser parser = manager.query(String8(FAKE_UID));
    RightsParser parser = manager.query(String8("SD_MP4_Count5times"));
    ASSERT_TRUE(parser.parse());
    cout << "version: "<<parser.version << " uid: " << parser.uid << " key: " << parser.key << endl;

    RightsParser parser2 = manager.query(String8("cid:not_exist"));
    ASSERT_FALSE(parser2.parse());
}

TEST(RightsManager, save_and_query) {
    RightsManager manager;

    String8 buffer = ReadWriteUtils::readBytes(String8("/data/drm/test/test.dr"));
    DrmRights rights (DrmBuffer((char*)(buffer.string()), buffer.length()), String8(MIMETYPE_RO));

    RightsParser origParser(rights);
    ASSERT_TRUE(origParser.parse());
    ASSERT_STREQ(origParser.version, "1.0");
    manager.saveRights(origParser);

    RightsParser newParser = manager.query(String8(origParser.uid));
    ASSERT_TRUE(newParser.parse());
    ASSERT_STREQ(newParser.uid, origParser.uid);
}

TEST(DcfParser, create_and_parse) {
    // DcfCreator creator ("/data/drm/test/tmp.dcf", FAKE_KEY, "video/mp4", FAKE_UID);
    DcfCreator creator ("/data/drm/test/sd_14_3_5_15_3_5_avi_1666k_MP4V_mp3.dcf", FAKE_KEY, "video/x-msvideo", FAKE_UID);
    creator.write("test", 4);
    creator.save();
    // DcfParser dcfParser("/data/drm/test/tmp.dcf");
    DcfParser dcfParser("/data/drm/test/sd_14_3_5_15_3_5_avi_1666k_MP4V_mp3.dcf");
    ASSERT_TRUE(dcfParser.parse());
    // ASSERT_STREQ(dcfParser.contentType.string(), "video/mp4");
    ASSERT_STREQ(dcfParser.contentType.string(), "video/x-msvideo");
    // ASSERT_STREQ(dcfParser.contentUri.string(), FAKE_UID);
    ASSERT_STREQ(dcfParser.contentUri.string(), "sd_14_3_5_15_3_5_avi_1666k_MP4V_mp3");
    dcfParser.setKey(FAKE_KEY);
    char buffer[5] = {0};
    dcfParser.readAt(buffer, 4, 0);
    // ASSERT_STREQ(buffer, "test");
    ASSERT_STREQ(buffer, "Sz\xD9\xE4");
}

TEST(DcfParser, unsupported_encryption_method) {
    // DcfCreator creator ("/data/drm/test/tmp.dcf", FAKE_KEY, "video/mp4", FAKE_UID);
    DcfCreator creator ("/data/drm/test/sd_14_3_5_15_3_5_avi_1666k_MP4V_mp3.dcf ", FAKE_KEY, "video/x-msvideo", FAKE_UID);
    creator.encryptionMethod = "unsupported_encryption_method";
    creator.write("test", 4);
    creator.save();
    // DcfParser dcfParser("/data/drm/test/tmp.dcf");
    DcfParser dcfParser("/data/drm/test/sd_14_3_5_15_3_5_avi_1666k_MP4V_mp3.dcf ");
    ASSERT_FALSE(dcfParser.parse());
}

TEST(DcfParser, decode) {
    // DcfParser dcfParser("/data/drm/test/test_decode.dcf");
    DcfParser dcfParser("/data/drm/test/sd_5coun_midi_11k_zhendong_Blue_Ice.dcf");
    dcfParser.setKey("UvHIIfoSw5jdbJEymnvoAA==");
    ASSERT_TRUE(dcfParser.parse());
    int fd = open ("/data/drm/test/test_decode.out",O_TRUNC|O_CREAT|O_WRONLY, 0755);
    char buffer[1024] = {0};
    int offset = 0;
    int ret = dcfParser.readAt(buffer, 1024, offset);
    while (ret > 0) {
        write(fd,buffer,ret);
        offset += ret;
        ret = dcfParser.readAt(buffer, 1024, offset);
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
