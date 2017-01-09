#ifndef __SPRDOEMCRYPTO_API_H__
#define __SPRDOEMCRYPTO_API_H__

#ifdef __cplusplus
extern "C" {
#endif  

typedef unsigned char OEMCrypto_UINT8;
typedef char          OEMCrypto_INT8;
typedef unsigned int  OEMCrypto_UINT32;
typedef unsigned int  OEMCrypto_SECURE_BUFFER;

typedef enum _OEMCryptoResult {
    OEMCrypto_SUCCESS = 0,
    OEMCrypto_FAILURE
}OEMCryptoResult;

#define OEMCrypto_GetkeyboxData          _oec01
#define OEMCrypto_EncryptAndStoreKeyBox  _oec02
#define OEMCrypto_IdentifyDevice         _oec03
#define OEMCrypto_GetRandom              _oec04

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
                          OEMCrypto_UINT32 keyboxLength);

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
                          OEMCrypto_UINT32 idLength);

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
                          OEMCrypto_UINT32 length);

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
                          OEMCrypto_UINT32 dataLength);

#ifdef __cplusplus
}
#endif 
#endif


