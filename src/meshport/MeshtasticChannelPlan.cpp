#include "MeshtasticChannelPlan.h"

#include <cmath>

namespace meshport {

uint32_t meshtasticNameHash(const char *str)
{
    uint32_t h = 5381;
    int c;
    while ((c = static_cast<unsigned char>(*str++)) != 0) {
        h = ((h << 5) + h) + static_cast<uint32_t>(c);
    }
    return h;
}

uint32_t frequencySlotCount(float freqStartMHz, float freqEndMHz, float slotWidthMHz)
{
    return static_cast<uint32_t>(std::lround((freqEndMHz - freqStartMHz) / slotWidthMHz));
}

float slotCenterMHz(float freqStartMHz, float slotWidthMHz, uint32_t slot)
{
    return freqStartMHz + slotWidthMHz / 2.0f + static_cast<float>(slot) * slotWidthMHz;
}

uint32_t defaultSlotForChannel(const char *channelName, float freqStartMHz, float freqEndMHz,
                               float slotWidthMHz)
{
    uint32_t slots = frequencySlotCount(freqStartMHz, freqEndMHz, slotWidthMHz);
    return slots ? meshtasticNameHash(channelName) % slots : 0;
}

} // namespace meshport
