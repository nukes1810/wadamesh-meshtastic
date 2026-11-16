// Meshtastic wire format helpers, ported from Meshtastic firmware
// (src/mesh/RadioInterface.h PacketHeader, src/mesh/Channels.cpp xorHash/generateHash,
//  src/mesh/CryptoEngine.cpp initNonce). No MeshCore code is used here.
#pragma once

#include <cstddef>
#include <cstdint>

namespace meshport {

constexpr size_t kHeaderLen = 16;

constexpr uint8_t kFlagHopLimitMask = 0x07;
constexpr uint8_t kFlagWantAckMask = 0x08;
constexpr uint8_t kFlagViaMqttMask = 0x10;
constexpr uint8_t kFlagHopStartMask = 0xE0;
constexpr uint8_t kFlagHopStartShift = 5;

// Exact 16-byte radio header. Multi-byte fields are little-endian on the wire.
struct Header {
    uint32_t to;
    uint32_t from;
    uint32_t id;
    uint8_t flags;
    uint8_t channel;   // channel hash (hint for decoders)
    uint8_t nextHop;   // last byte of next-hop node number
    uint8_t relayNode; // last byte of relaying node number
};

// Write the header into out[0..15]. Returns kHeaderLen.
size_t packHeader(const Header &h, uint8_t *out);

// Read a header from in[0..15]. Caller must ensure len >= kHeaderLen.
Header unpackHeader(const uint8_t *in);

// XOR of all bytes, as used by Meshtastic channel hashing.
uint8_t xorHash(const uint8_t *p, size_t len);

// Channel hash = xor(name bytes) ^ xor(PSK bytes). Matches Channels::generateHash.
uint8_t channelHash(const char *name, const uint8_t *psk, size_t pskLen);

// Build the 16-byte AES-CTR nonce used for channel payload encryption:
// packetId (8 bytes, LE) | fromNode (4 bytes, LE) | extraNonce (4 bytes, LE, 0 for channel msgs).
// Matches CryptoEngine::initNonce(fromNode, packetId) with extraNonce == 0.
void buildChannelNonce(uint32_t fromNode, uint64_t packetId, uint8_t nonce[16]);

} // namespace meshport
