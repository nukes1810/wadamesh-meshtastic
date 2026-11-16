// Host tests for src/meshport/MeshtasticWire.*
// Build: g++ -std=c++17 -Wall -Wextra -I ../../src/meshport test_meshtastic_wire.cpp ../../src/meshport/MeshtasticWire.cpp -o t && ./t
#include "MeshtasticWire.h"

#include <cstdio>
#include <cstring>

static int failures = 0;
#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                 \
        }                                                               \
    } while (0)

using namespace meshport;

static void testHeaderLayout()
{
    Header h{};
    h.to = 0x11223344;
    h.from = 0xAABBCCDD;
    h.id = 0x01020304;
    h.flags = 0x63; // hop limit 3, want_ack, hop start 3
    h.channel = 0x7E;
    h.nextHop = 0x05;
    h.relayNode = 0x06;

    uint8_t buf[16];
    CHECK(packHeader(h, buf) == 16);
    // little-endian byte order on the wire
    CHECK(buf[0] == 0x44 && buf[1] == 0x33 && buf[2] == 0x22 && buf[3] == 0x11);
    CHECK(buf[4] == 0xDD && buf[7] == 0xAA);
    CHECK(buf[8] == 0x04 && buf[11] == 0x01);
    CHECK(buf[12] == 0x63 && buf[13] == 0x7E && buf[14] == 0x05 && buf[15] == 0x06);
}

static void testHeaderRoundTrip()
{
    Header h{0xFFFFFFFF, 0x00000001, 0xDEADBEEF, 0xE8, 0x00, 0x00, 0xFF};
    uint8_t buf[16];
    packHeader(h, buf);
    Header g = unpackHeader(buf);
    CHECK(g.to == h.to && g.from == h.from && g.id == h.id);
    CHECK(g.flags == h.flags && g.channel == h.channel);
    CHECK(g.nextHop == h.nextHop && g.relayNode == h.relayNode);
}

static void testXorHash()
{
    const uint8_t a[] = {0x0F, 0xF0, 0x33};
    CHECK(xorHash(a, 3) == (0x0F ^ 0xF0 ^ 0x33));
    CHECK(xorHash(a, 0) == 0);
}

static void testChannelHash()
{
    // Default LongFast-style setup: name "LongFast", 1-byte default PSK alias is not used here;
    // we test the raw formula name_xor ^ psk_xor against a value computed by hand.
    const uint8_t psk[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    uint8_t nameX = xorHash((const uint8_t *)"Ops", 3); // 'O'^'p'^'s'
    uint8_t pskX = xorHash(psk, 16);                  // 1^2^...^16 = 16
    CHECK(channelHash("Ops", psk, 16) == (uint8_t)(nameX ^ pskX));
    CHECK(channelHash("", psk, 0) == 0);
}

static void testChannelNonce()
{
    uint8_t n[16];
    buildChannelNonce(0x12345678, 0x0102030405060708ULL, n);
    // packetId, 8 bytes little-endian
    CHECK(n[0] == 0x08 && n[1] == 0x07 && n[2] == 0x06 && n[3] == 0x05);
    CHECK(n[4] == 0x04 && n[5] == 0x03 && n[6] == 0x02 && n[7] == 0x01);
    // fromNode, 4 bytes little-endian
    CHECK(n[8] == 0x78 && n[9] == 0x56 && n[10] == 0x34 && n[11] == 0x12);
    // extraNonce is zero for channel messages
    CHECK(n[12] == 0 && n[13] == 0 && n[14] == 0 && n[15] == 0);
}

int main()
{
    testHeaderLayout();
    testHeaderRoundTrip();
    testXorHash();
    testChannelHash();
    testChannelNonce();
    if (failures == 0) {
        std::printf("all meshtastic wire tests passed\n");
        return 0;
    }
    std::printf("%d failure(s)\n", failures);
    return 1;
}
