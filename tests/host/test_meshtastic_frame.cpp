// Host tests for src/meshport/MeshtasticFrame.* (seal/open, hop limit, seen cache).
#include "MeshtasticFrame.h"

#include <cstdio>
#include <cstring>
#include <string>

static int failures = 0;
#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                 \
        }                                                               \
    } while (0)

using namespace meshport;

static void testSealOpenRoundTrip()
{
    uint8_t psk[16];
    for (int i = 0; i < 16; i++) psk[i] = static_cast<uint8_t>(0x10 + i);

    Header h{};
    h.to = 0xFFFFFFFF;
    h.from = 0x1234ABCD;
    h.id = 0x0000BEEF;
    h.flags = 0x03 | (0x03 << 5); // hop limit 3, hop start 3
    h.channel = channelHash("LongFast", psk, 16);

    Data d;
    d.portnum = kPortTextMessage;
    std::string msg = "hi from wadamesh";
    d.payload.assign(msg.begin(), msg.end());

    std::vector<uint8_t> frame;
    CHECK(sealFrame(psk, 16, h, d, frame));
    CHECK(frame.size() == kHeaderLen + encodeData(d).size());
    // Header is in the clear; payload is not.
    CHECK(frame[0] == 0xFF && frame[3] == 0xFF);
    CHECK(std::string(frame.begin() + kHeaderLen, frame.end()) != msg);

    Header h2{};
    Data d2;
    CHECK(openFrame(psk, 16, frame.data(), frame.size(), h2, d2));
    CHECK(h2.from == h.from && h2.id == h.id && h2.to == h.to);
    CHECK(h2.channel == h.channel && h2.flags == h.flags);
    CHECK(d2.portnum == kPortTextMessage);
    CHECK(std::string(d2.payload.begin(), d2.payload.end()) == msg);
}

static void testWrongKeyFailsOrGarbles()
{
    uint8_t psk[16], other[16];
    for (int i = 0; i < 16; i++) { psk[i] = static_cast<uint8_t>(i); other[i] = static_cast<uint8_t>(i + 1); }
    Header h{};
    h.from = 7;
    h.id = 9;
    Data d;
    d.portnum = kPortTextMessage;
    d.payload = {'o', 'k'};
    std::vector<uint8_t> frame;
    CHECK(sealFrame(psk, 16, h, d, frame));
    Header h2{};
    Data d2;
    bool ok = openFrame(other, 16, frame.data(), frame.size(), h2, d2);
    // AES-CTR decrypts to noise under the wrong key: either protobuf rejects it,
    // or the payload differs from the original.
    CHECK(!ok || d2.payload != d.payload);
}

static void testRejectsBadInput()
{
    uint8_t psk[16] = {0};
    std::vector<uint8_t> frame;
    Header h{};
    Data d;
    CHECK(!sealFrame(psk, 15, h, d, frame));
    uint8_t tiny[4] = {0};
    CHECK(!openFrame(psk, 16, tiny, sizeof tiny, h, d));
    std::vector<uint8_t> big(kMaxFrameLen + 1, 0);
    CHECK(!openFrame(psk, 16, big.data(), big.size(), h, d));
    std::vector<uint8_t> huge(300, 'x');
    Data bigData;
    bigData.payload = huge;
    CHECK(!sealFrame(psk, 16, h, bigData, frame));
}

static void testHopLimit()
{
    Header h{};
    h.flags = 0x02 | (0x03 << 5);
    CHECK(decrementHopLimit(h));
    CHECK((h.flags & kFlagHopLimitMask) == 1);
    CHECK((h.flags & kFlagHopStartMask) == (0x03 << 5)); // hop start untouched
    CHECK(decrementHopLimit(h));
    CHECK((h.flags & kFlagHopLimitMask) == 0);
    CHECK(!decrementHopLimit(h)); // no hops left
}

static void testSeenCache()
{
    SeenCache c(2);
    CHECK(!c.checkAndInsert(1, 10));
    CHECK(c.checkAndInsert(1, 10));  // duplicate
    CHECK(!c.checkAndInsert(2, 10)); // same id, different sender
    CHECK(!c.checkAndInsert(3, 30)); // evicts oldest (1,10)
    CHECK(!c.checkAndInsert(1, 10)); // evicted, so seen as new again
}

int main()
{
    testSealOpenRoundTrip();
    testWrongKeyFailsOrGarbles();
    testRejectsBadInput();
    testHopLimit();
    testSeenCache();
    if (failures == 0) {
        std::printf("all meshtastic frame tests passed\n");
        return 0;
    }
    std::printf("%d failure(s)\n", failures);
    return 1;
}
