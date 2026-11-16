#include "MeshtasticCrypto.h"

#include "MeshtasticWire.h"

#include <cstring>

namespace meshport {

bool aesCtrCrypt(const uint8_t *key, size_t keyLen, const uint8_t iv[16], uint8_t *data, size_t len)
{
    if (keyLen != 16 && keyLen != 32) {
        return false;
    }
    uint8_t counter[16];
    std::memcpy(counter, iv, 16);

    size_t off = 0;
    while (off < len) {
        uint8_t stream[16];
        aesEncryptBlock(key, keyLen, counter, stream);
        size_t n = (len - off < 16) ? (len - off) : 16;
        for (size_t i = 0; i < n; i++) {
            data[off + i] ^= stream[i];
        }
        off += n;
        // Big-endian increment over the whole 16-byte counter block.
        for (int i = 15; i >= 0; i--) {
            if (++counter[i] != 0) {
                break;
            }
        }
    }
    return true;
}

bool channelCrypt(const uint8_t *psk, size_t pskLen, uint32_t fromNode, uint64_t packetId,
                  uint8_t *data, size_t len)
{
    uint8_t nonce[16];
    buildChannelNonce(fromNode, packetId, nonce);
    return aesCtrCrypt(psk, pskLen, nonce, data, len);
}

} // namespace meshport
