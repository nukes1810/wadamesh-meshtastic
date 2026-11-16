// Device backend: AES block primitive via mbedtls (bundled with ESP-IDF).
#include "MeshtasticCrypto.h"

#include "mbedtls/aes.h"

namespace meshport {

void aesEncryptBlock(const uint8_t *key, size_t keyLen, const uint8_t in[16], uint8_t out[16])
{
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_enc(&ctx, key, static_cast<unsigned int>(keyLen * 8));
    mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, in, out);
    mbedtls_aes_free(&ctx);
}

} // namespace meshport
