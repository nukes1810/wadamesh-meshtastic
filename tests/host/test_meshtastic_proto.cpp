// Host tests for src/meshport/MeshtasticProto.* and the encrypted Data round trip.
// Build with the crypto sources and openssl_aes_backend.cpp (see test_meshtastic_crypto.cpp).
#include "MeshtasticCrypto.h"
#include "MeshtasticProto.h"

#include <cstdio>
#include <cstring>
#include <string>
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

static void testTextMessageBytes()
{
    // Hand-computed: portnum=1 -> 08 01 ; payload "hi" -> 12 02 68 69
    Data d;
    d.portnum = kPortTextMessage;
    d.payload = {'h', 'i'};
    std::vector<uint8_t> expect = {0x08, 0x01, 0x12, 0x02, 0x68, 0x69};
    CHECK(encodeData(d) == expect);
}

static void testFixed32AndBoolBytes()
{
    // want_response=true -> 18 01 ; dest=0x11223344 fixed32 -> 25 44 33 22 11
    Data d;
    d.wantResponse = true;
    d.dest = 0x11223344;
    std::vector<uint8_t> expect = {0x18, 0x01, 0x25, 0x44, 0x33, 0x22, 0x11};
    CHECK(encodeData(d) == expect);
}

static void testDefaultsOmitted()
{
    Data d;
    CHECK(encodeData(d).empty());
}

static void testRoundTripAllFields()
{
    Data d;
    d.portnum = kPortPosition;
    d.payload.assign(300, 0x5A); // forces multi-byte length varint
    d.wantResponse = true;
    d.dest = 0xAAAAAAAA;
    d.source = 0xBBBBBBBB;
    d.requestId = 0x01020304;
    d.replyId = 0x05060708;
    d.emoji = 0x0000F09F;
    auto bytes = encodeData(d);
    Data back;
    CHECK(decodeData(bytes.data(), bytes.size(), back));
    CHECK(back.portnum == d.portnum);
    CHECK(back.payload == d.payload);
    CHECK(back.wantResponse == d.wantResponse);
    CHECK(back.dest == d.dest && back.source == d.source);
    CHECK(back.requestId == d.requestId && back.replyId == d.replyId);
    CHECK(back.emoji == d.emoji);
}

static void testSkipsUnknownFields()
{
    // Unknown field 15 varint: tag (15<<3)|0 = 0x78, value 0x07; then portnum=1.
    std::vector<uint8_t> bytes = {0x78, 0x07, 0x08, 0x01};
    Data d;
    CHECK(decodeData(bytes.data(), bytes.size(), d));
    CHECK(d.portnum == 1);
}

static void testRejectsTruncated()
{
    std::vector<uint8_t> bytes = {0x12, 0x05, 'a'}; // claims 5 bytes, has 1
    Data d;
    CHECK(!decodeData(bytes.data(), bytes.size(), d));
    std::vector<uint8_t> bad = {0x25, 0x01}; // fixed32 needs 4 bytes
    CHECK(!decodeData(bad.data(), bad.size(), d));
}

static void testEncryptedRoundTrip()
{
    uint8_t psk[16];
    for (int i = 0; i < 16; i++) psk[i] = static_cast<uint8_t>(0x30 + i);
    const uint32_t from = 0x1234ABCD;
    const uint64_t packetId = 0x0000000042;

    Data d;
    d.portnum = kPortTextMessage;
    std::string msg = "hello mesh";
    d.payload.assign(msg.begin(), msg.end());

    auto wire = encodeData(d);
    CHECK(channelCrypt(psk, 16, from, packetId, wire.data(), wire.size()));
    CHECK(std::string(wire.begin(), wire.end()) != msg); // ciphertext is not plaintext

    CHECK(channelCrypt(psk, 16, from, packetId, wire.data(), wire.size()));
    Data back;
    CHECK(decodeData(wire.data(), wire.size(), back));
    CHECK(back.portnum == kPortTextMessage);
    CHECK(std::string(back.payload.begin(), back.payload.end()) == msg);
}

int main()
{
    testTextMessageBytes();
    testFixed32AndBoolBytes();
    testDefaultsOmitted();
    testRoundTripAllFields();
    testSkipsUnknownFields();
    testRejectsTruncated();
    testEncryptedRoundTrip();
    if (failures == 0) {
        std::printf("all meshtastic proto tests passed\n");
        return 0;
    }
    std::printf("%d failure(s)\n", failures);
    return 1;
}
