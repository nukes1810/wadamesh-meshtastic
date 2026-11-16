// Host tests for src/meshport/MeshtasticCrypto.*
// Uses OpenSSL as the AES block backend (test only).
// Build: see the g++ line in the commit message or run with -lcrypto and openssl_aes_backend.cpp.
#include "MeshtasticCrypto.h"
#include "MeshtasticWire.h"

#include <cstdio>
#include <cstring>
#include <vector>

static int failures = 0;
#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                 \
        }                                                               \
    } while (0)

using namespace meshport;

static std::vector<uint8_t> hex(const char *s)
{
    std::vector<uint8_t> v;
    for (size_t i = 0; s[i] && s[i + 1]; i += 2) {
        unsigned int b;
        std::sscanf(s + i, "%2x", &b);
        v.push_back(static_cast<uint8_t>(b));
    }
    return v;
}

// NIST SP 800-38A, F.5.1 CTR-AES128.Encrypt (4 blocks)
static void testNistCtr128()
{
    auto key = hex("2b7e151628aed2a6abf7158809cf4f3c");
    auto iv = hex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    auto pt = hex("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
                  "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710");
    auto ct = hex("874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff"
                  "5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee");
    std::vector<uint8_t> buf = pt;
    CHECK(aesCtrCrypt(key.data(), 16, iv.data(), buf.data(), buf.size()));
    CHECK(buf == ct);
}

// NIST SP 800-38A, F.5.5 CTR-AES256.Encrypt (first block)
static void testNistCtr256()
{
    auto key = hex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
    auto iv = hex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    auto pt = hex("6bc1bee22e409f96e93d7e117393172a");
    auto ct = hex("601ec313775789a5b7a7f504bbf3d228");
    std::vector<uint8_t> buf = pt;
    CHECK(aesCtrCrypt(key.data(), 32, iv.data(), buf.data(), buf.size()));
    CHECK(buf == ct);
}

static void testCounterCarry()
{
    // IV ending in 0xFF must carry into the byte before it (big-endian counter).
    uint8_t key[16] = {0};
    uint8_t iv[16] = {0};
    iv[15] = 0xFF;
    std::vector<uint8_t> zeros(32, 0);
    std::vector<uint8_t> a = zeros, b = zeros;
    CHECK(aesCtrCrypt(key, 16, iv, a.data(), a.size()));
    // Keystream block 2 must equal encrypting counter 0x0...0100 directly.
    uint8_t iv2[16] = {0};
    iv2[14] = 0x01;
    std::vector<uint8_t> z2(16, 0);
    CHECK(aesCtrCrypt(key, 16, iv2, z2.data(), z2.size()));
    CHECK(std::memcmp(a.data() + 16, z2.data(), 16) == 0);
}

static void testRoundTripAndLengths()
{
    uint8_t psk[32];
    for (int i = 0; i < 32; i++) psk[i] = static_cast<uint8_t>(i * 7 + 3);
    for (size_t len : {size_t(0), size_t(1), size_t(15), size_t(16), size_t(17), size_t(237)}) {
        std::vector<uint8_t> plain(len);
        for (size_t i = 0; i < len; i++) plain[i] = static_cast<uint8_t>(i);
        std::vector<uint8_t> work = plain;
        CHECK(channelCrypt(psk, 32, 0x12345678, 0xABCDEF0102ULL, work.data(), work.size()));
        if (len > 0) CHECK(work != plain);
        CHECK(channelCrypt(psk, 32, 0x12345678, 0xABCDEF0102ULL, work.data(), work.size()));
        CHECK(work == plain);
    }
}

static void testNonceSeparatesMessages()
{
    // Same key and plaintext, different packet id or sender -> different ciphertext.
    uint8_t psk[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t a[8] = {0}, b[8] = {0}, c[8] = {0};
    CHECK(channelCrypt(psk, 16, 1, 100, a, 8));
    CHECK(channelCrypt(psk, 16, 1, 101, b, 8));
    CHECK(channelCrypt(psk, 16, 2, 100, c, 8));
    CHECK(std::memcmp(a, b, 8) != 0);
    CHECK(std::memcmp(a, c, 8) != 0);
}

static void testRejectsBadKeyLength()
{
    uint8_t key[24] = {0};
    uint8_t iv[16] = {0};
    uint8_t data[4] = {0};
    CHECK(!aesCtrCrypt(key, 24, iv, data, 4));
    CHECK(!channelCrypt(key, 0, 1, 1, data, 4));
}

int main()
{
    testNistCtr128();
    testNistCtr256();
    testCounterCarry();
    testRoundTripAndLengths();
    testNonceSeparatesMessages();
    testRejectsBadKeyLength();
    if (failures == 0) {
        std::printf("all meshtastic crypto tests passed\n");
        return 0;
    }
    std::printf("%d failure(s)\n", failures);
    return 1;
}
