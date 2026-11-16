// AES-CTR channel payload encryption, matching Meshtastic firmware
// (src/mesh/CryptoEngine.cpp: mbedtls_aes_crypt_ctr with the 16-byte nonce as IV).
// The CTR counter is the full 128-bit block, incremented big-endian.
// The raw AES block primitive comes from a backend (mbedtls on device,
// OpenSSL in host tests), declared below.
#pragma once

#include <cstddef>
#include <cstdint>

namespace meshport {

// Raw AES single-block encryption. keyLen is 16 (AES-128) or 32 (AES-256).
// Implemented per platform; see MeshtasticCryptoBackend_*.cpp.
void aesEncryptBlock(const uint8_t *key, size_t keyLen, const uint8_t in[16], uint8_t out[16]);

// AES-CTR keystream XOR. Encryption and decryption are the same operation.
// keyLen must be 16 or 32. Returns false for any other key length.
bool aesCtrCrypt(const uint8_t *key, size_t keyLen, const uint8_t iv[16], uint8_t *data, size_t len);

// Encrypt or decrypt a channel payload in place. Builds the nonce from
// (fromNode, packetId) as Meshtastic does. psk must be 16 or 32 bytes.
bool channelCrypt(const uint8_t *psk, size_t pskLen, uint32_t fromNode, uint64_t packetId,
                  uint8_t *data, size_t len);

} // namespace meshport
