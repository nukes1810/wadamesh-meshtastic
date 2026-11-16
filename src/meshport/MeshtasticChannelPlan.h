// Frequency-slot math for Meshtastic's default channel plan.
// Source: Meshtastic src/mesh/RadioInterface.cpp (djb2 name hash, slot width = bandwidth,
// freq = freqStart + bw/2 + padding + slot * bw). Pure math, host-testable.
#pragma once

#include <cstdint>

namespace meshport {

// djb2 hash used by Meshtastic's hash() in RadioInterface.cpp.
uint32_t meshtasticNameHash(const char *str);

// Number of frequency slots in a region: round((freqEnd - freqStart) / slotWidth).
// slotWidthMHz is the channel bandwidth in MHz (250 kHz = 0.25).
uint32_t frequencySlotCount(float freqStartMHz, float freqEndMHz, float slotWidthMHz);

// Center frequency of a slot (0-based): freqStart + bw/2 + slot * bw.
float slotCenterMHz(float freqStartMHz, float slotWidthMHz, uint32_t slot);

// Slot used by a channel name when the user has not chosen one: hash(name) % slots.
uint32_t defaultSlotForChannel(const char *channelName, float freqStartMHz, float freqEndMHz,
                               float slotWidthMHz);

// LongFast preset parameters (src/mesh/MeshRadio.h default case).
constexpr float kLongFastBandwidthKHz = 250.0f;
constexpr uint8_t kLongFastSpreadingFactor = 11;
constexpr uint8_t kLongFastCodingRateDenom = 5; // 4/5
constexpr uint8_t kLongFastSyncWord = 0x2B;
constexpr uint16_t kLongFastPreambleLen = 16;   // RadioInterface.h preambleLengthDefault

} // namespace meshport
