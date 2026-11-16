// Host tests for src/meshport/MeshtasticChannelPlan.*
#include "MeshtasticChannelPlan.h"

#include <cmath>
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

static void testHash()
{
    CHECK(meshtasticNameHash("") == 5381);
    // djb2: h = 5381*33 + 97 = 177670
    CHECK(meshtasticNameHash("a") == 177670u);
}

static void testUsSlotsAndFrequencies()
{
    // US band 902-928 MHz, LongFast 250 kHz: 26 / 0.25 = 104 slots.
    CHECK(frequencySlotCount(902.0f, 928.0f, 0.25f) == 104);
    // Slot 0 center: 902 + 0.125 = 902.125 MHz; last slot 103 center: 902.125 + 103*0.25 = 927.875.
    CHECK(std::fabs(slotCenterMHz(902.0f, 0.25f, 0) - 902.125f) < 1e-3f);
    CHECK(std::fabs(slotCenterMHz(902.0f, 0.25f, 103) - 927.875f) < 1e-3f);
}

static void testEuSlots()
{
    // EU868 band 869.4-869.65 MHz: 0.25 / 0.25 = 1 slot, center 869.525 MHz.
    CHECK(frequencySlotCount(869.4f, 869.65f, 0.25f) == 1);
    CHECK(std::fabs(slotCenterMHz(869.4f, 0.25f, 0) - 869.525f) < 1e-3f);
}

static void testDefaultSlotInRange()
{
    uint32_t s = defaultSlotForChannel("LongFast", 902.0f, 928.0f, 0.25f);
    CHECK(s < 104);
    // Same name always gives the same slot.
    CHECK(s == defaultSlotForChannel("LongFast", 902.0f, 928.0f, 0.25f));
    std::printf("LongFast US slot = %u, center = %.3f MHz\n", (unsigned)s,
                slotCenterMHz(902.0f, 0.25f, s));
}

static void testZeroSlotsSafe()
{
    CHECK(defaultSlotForChannel("x", 1.0f, 1.0f, 0.25f) == 0);
}

int main()
{
    testHash();
    testUsSlotsAndFrequencies();
    testEuSlots();
    testDefaultSlotInRange();
    testZeroSlotsSafe();
    if (failures == 0) {
        std::printf("all meshtastic channel plan tests passed\n");
        return 0;
    }
    std::printf("%d failure(s)\n", failures);
    return 1;
}
