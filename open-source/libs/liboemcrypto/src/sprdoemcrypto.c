/*********************************************************************
File: sprdoemcrypto.c
Author: robert lu
Creation Date; 2012-5-22
descritpion: low level API
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include "sprdoemcrypto.h"
#include <openssl/aes.h>

#define LOG_TAG "SPRDOEMCRYPTO"
#undef LOG
#include <utils/Log.h>
#include <cutils/properties.h>

#define KEYBOX_LENGTH 128
#define IV_LENGTH     16
#define KEY_LENGTH    16
#define KEYBOXFILENAME    "/productinfo/keybox.dat"

const unsigned char OEMRootKey[KEY_LENGTH] = "abcdefgh12345678";
const unsigned char enc_iv[IV_LENGTH] ="\x3d\xcf\xba\x43\x9d\x9e\xb4\x30\xb4\x22\xda\x80\x2c\x9f\xac\x42";


/*
 * Encrypt and store the keybox to persistent memory. The device key or entire keybox must be
 * stored securely, encrypted by an OEM root key.
 * Parameters:
 *     keybox(in) - Pointer to clear keybox data.Must be encrypted with an OEM root key.
 *     keyboxLength(in) - Length of the keybox data in bytes.
 * Returns:
 *     OEMCryptoResult indicating success or failure
 */
OEMCryptoResult OEMCrypto_EncryptAndStoreKeyBox(
                          OEMCrypto_UINT8 *keybox,
                          OEMCrypto_UINT32 keyboxLength){
    AES_KEY akey;
    FILE *file;
    unsigned int retc;
    unsigned char iv[IV_LENGTH];
    unsigned char *out = NULL;
    unsigned char *outt = NULL;

    ALOGD("OEMCrypto_EncryptAndStoreKeyBox, keyboxLength = %d", keyboxLength);

    if ((keybox == NULL) || (keyboxLength != KEYBOX_LENGTH)) {
       ALOGD("OEMCrypto_EncryptAndStoreKeyBox,keyBoxLength is not equal to 128 or keybox is null");
       return OEMCrypto_FAILURE;
    }

    //ALOGD("OEMCrypto_EncryptAndStoreKeyBox, keybox=%s",keybox);

    out = (unsigned char *) malloc(keyboxLength);

    if (out == NULL) {
        ALOGD("OEMCrypto_EncryptAndStoreKeyBox, allocat out failed!");
        return OEMCrypto_FAILURE;
    }
    memcpy(iv, enc_iv, IV_LENGTH);
    AES_set_encrypt_key(OEMRootKey, 128, &akey);

    AES_cbc_encrypt(keybox, out, keyboxLength, &akey, &iv[0], AES_ENCRYPT);

    file = fopen(KEYBOXFILENAME, "wb");
    if (file == NULL) {
        ALOGD("OEMCrypto_EncryptAndStoreKeyBox, Could not open %s!",KEYBOXFILENAME);
        return OEMCrypto_FAILURE;
    }
    retc = fwrite(out, 1, keyboxLength, file);

    fclose(file);
    return retc != keyboxLength ? OEMCrypto_FAILURE : OEMCrypto_SUCCESS;
}

/*
 * Return the device's unique identifier. the device identifier shall not come from the Widevine
 * keybox.
 * Parameters:
 *    deviceID(out) - Points to the buffer that should receive the key data.
 *    idLength(in) - Length of the device ID buffer. Maximum of 32 bytes allowed
 * Returns:
 *     OEMCryptoResult indicating success or failure
 */
OEMCryptoResult OEMCrypto_IdentifyDevice(
                          OEMCrypto_UINT8 *deviceID,
                          OEMCrypto_UINT32 idLength)
{
    ALOGD("OEMCrypto_IdentifyDevice, idLength = %d", idLength);

    if ((deviceID == NULL) || (idLength > 32)) {
        ALOGD("OEMCrypto_IdentifyDevice, idLength less than 32 or deviceID is null!");
        return OEMCrypto_FAILURE;
    }

    char value[PROPERTY_VALUE_MAX] = { '\0' };
    property_get("ro.serialno", value, "123456789ABCDE");

    memset(deviceID, 0, idLength);

    int nLength = strlen(value);
    if (nLength < 31) {
        memcpy(deviceID, value, nLength);
    }
    else {
        memcpy(deviceID, value, 31);
    }

    ALOGD("OEMCrypto_IdentifyDevice,read imei=%s\n", deviceID);

    return OEMCrypto_SUCCESS;
}


/*
 * Retrieve a range of bytes from the Widevine keybox. This function should decrypt the keybox and
 * return the specified bytes.
 * parameters:
 *     buffer(out) - Pointers to the buffer that should receive the keybox data
 *     offset(in) - Byte offet from the beginning of the keybox of the first byte to return
 *     length(in) - Number of bytes of data to return
 * Returns:
 *     OEMCryptoResult indicating success or failure
 */
OEMCryptoResult OEMCrypto_GetkeyboxData(
                          OEMCrypto_UINT8 *buffer,
                          OEMCrypto_UINT32 offset,
                          OEMCrypto_UINT32 length)
{
    AES_KEY akey;
    FILE *file;
    unsigned int retc;
    unsigned char iv[IV_LENGTH];
    unsigned char out[KEYBOX_LENGTH+1];
    unsigned char keybox[KEYBOX_LENGTH+1];

    ALOGD("OEMCrypto_GetkeyboxData, offset = %d, length = %d", offset, length);

    if ((buffer == NULL) || (length > KEYBOX_LENGTH)) {
        ALOGD("OEMCrypto_GetkeyboxData,length is greater than 128 or buffer is null!");
        return OEMCrypto_FAILURE;
    }

    if ((offset +length) > KEYBOX_LENGTH) {
        ALOGD("OEMCrypto_GetkeyboxData,(offset +length) is greater than 128!");
        return OEMCrypto_FAILURE;
    }
    memset(out , 0, KEYBOX_LENGTH);
    memset(keybox, 0, KEYBOX_LENGTH);
    memcpy(iv, enc_iv, IV_LENGTH);
    AES_set_decrypt_key(OEMRootKey, 128, &akey);

    file = fopen(KEYBOXFILENAME, "rb");
    if (file == NULL) {
        ALOGD("OEMCrypto_GetkeyboxData, Could not open %s!",KEYBOXFILENAME);
        return OEMCrypto_FAILURE;
    }

    retc = fread(keybox, 1, KEYBOX_LENGTH, file);
    fclose(file);
    if (retc != KEYBOX_LENGTH) {
        ALOGD("OEMCrypto_GetkeyboxData, read data failed %d!", retc);
        return OEMCrypto_FAILURE;
    }

    AES_cbc_encrypt(keybox, out, KEYBOX_LENGTH, &akey, &iv[0], AES_DECRYPT);

    ALOGD("OEMCrypto_GetkeyboxData, out=%s", out);

    memcpy(buffer, &out[offset], length);

    return OEMCrypto_SUCCESS;
}

/*
 * Return a buffer filled with hardware-generated random bytes, if suported by the hardware.
 * Parameters:
 *    randomData(out) - Pointed to the buffer that receives random data
 *    datalength(in) - Length of the random data buffer in bytes
 * Returns:
 *    OEMCrypto_SUCCESS success
 *    OEMCrypto_ERROR_RNG_FAILED failed to generate random number
 *    OEMCrypto_ERROR_RNG_NOT_SUPPORTED function not supported
 */
OEMCryptoResult OEMCrypto_GetRandom(
                          OEMCrypto_UINT8 *randomData,
                          OEMCrypto_UINT32 dataLength)
{
    FILE *file;
    unsigned int retc;

    ALOGD("OEMCrypto_GetRandom, dataLength = %d", dataLength);

    if (randomData == NULL) {
        ALOGD("OEMCrypto_GetRandom, randomData is null!");
        return OEMCrypto_FAILURE;
    }

    file = fopen("/dev/urandom", "rb");
    if (file == NULL) {
        ALOGD("OEMCrypto_GetRandom, Could not open /dev/urandom!");
        return OEMCrypto_FAILURE;
    }

    retc = fread(randomData, 1, dataLength, file);

    if (retc != dataLength) {
        ALOGD("OEMCrypto_GetRandom, read again!");
        retc = fread(randomData, 1, dataLength, file);
    }

    fclose(file);

    return retc != dataLength ? OEMCrypto_FAILURE : OEMCrypto_SUCCESS;
}
