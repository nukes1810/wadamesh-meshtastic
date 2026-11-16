// Minimal protobuf codec for Meshtastic's mesh.proto `Data` message.
// Field numbers from meshtastic/protobufs mesh.proto:
//   portnum=1 (enum varint), payload=2 (bytes), want_response=3 (bool),
//   dest=4 (fixed32), source=5 (fixed32), request_id=6 (fixed32),
//   reply_id=7 (fixed32), emoji=8 (fixed32).
// proto3 rules: zero/false/empty fields are not written. Unknown fields are skipped on decode.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace meshport {

constexpr uint32_t kPortTextMessage = 1; // TEXT_MESSAGE_APP
constexpr uint32_t kPortPosition = 3;    // POSITION_APP
constexpr uint32_t kPortNodeInfo = 4;    // NODEINFO_APP

struct Data {
    uint32_t portnum = 0;
    std::vector<uint8_t> payload;
    bool wantResponse = false;
    uint32_t dest = 0;
    uint32_t source = 0;
    uint32_t requestId = 0;
    uint32_t replyId = 0;
    uint32_t emoji = 0;
};

// Serialize Data to protobuf wire format.
std::vector<uint8_t> encodeData(const Data &d);

// Parse protobuf wire format. Returns false on malformed input.
bool decodeData(const uint8_t *buf, size_t len, Data &out);

} // namespace meshport
