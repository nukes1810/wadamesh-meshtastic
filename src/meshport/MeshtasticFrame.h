// Over-the-air Meshtastic frame: 16-byte header followed by the channel-encrypted
// protobuf Data payload. Built from MeshtasticWire (header, hash, nonce),
// MeshtasticCrypto (AES-CTR) and MeshtasticProto (Data codec).
#pragma once

#include "MeshtasticProto.h"
#include "MeshtasticWire.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace meshport {

// LongFast defaults used by Meshtastic's default preset (per Meshtastic
// RadioInterface / modem presets). The frequency slot is chosen per channel name.
constexpr uint8_t kLoRaSyncWordPrivate = 0x2B;
constexpr uint16_t kLoRaPreambleLen = 16;
constexpr uint8_t kLoRaCodingRateDenom = 5; // CR 4/5
constexpr uint8_t kLoRaSpreadingFactor = 11;
constexpr float kLoRaBandwidthKHz = 250.0f;
constexpr size_t kMaxFrameLen = 255;

// Build an over-the-air frame. The payload is encrypted with the channel PSK.
// Returns false if the PSK length is invalid or the frame would exceed kMaxFrameLen.
bool sealFrame(const uint8_t *psk, size_t pskLen, const Header &h, const Data &d,
               std::vector<uint8_t> &frame);

// Parse and decrypt a frame. Returns false on bad length, bad PSK, or malformed payload.
bool openFrame(const uint8_t *psk, size_t pskLen, const uint8_t *frame, size_t len,
               Header &h, Data &d);

// Relay rule: decrement hop limit when forwarding. Returns false when the packet
// has no hops left and must not be rebroadcast.
bool decrementHopLimit(Header &h);

// Remembers recently seen (from, id) pairs so a rebroadcast is not handled twice.
class SeenCache {
public:
    explicit SeenCache(size_t capacity = 64);
    // Returns true if the packet was already seen; otherwise records it and returns false.
    bool checkAndInsert(uint32_t from, uint32_t id);

private:
    struct Entry {
        uint32_t from;
        uint32_t id;
    };
    std::vector<Entry> entries_;
    size_t next_ = 0;
    size_t capacity_;
};

} // namespace meshport
