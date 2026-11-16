// Test-only AES block backend using OpenSSL. Not compiled into firmware.
#include "MeshtasticCrypto.h"

#include <openssl/aes.h>

namespace meshport {

void aesEncryptBlock(const uint8_t *key, size_t keyLen, const uint8_t in[16], uint8_t out[16])
{
    AES_KEY k;
    AES_set_encrypt_key(key, static_cast<int>(keyLen * 8), &k);
    AES_encrypt(in, out, &k);
}

} // namespace meshport
