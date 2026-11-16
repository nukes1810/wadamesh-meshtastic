#include "MeshtasticProto.h"

namespace meshport {

namespace {

constexpr uint8_t kWireVarint = 0;
constexpr uint8_t kWireFixed64 = 1;
constexpr uint8_t kWireLen = 2;
constexpr uint8_t kWireFixed32 = 5;

void putVarint(std::vector<uint8_t> &out, uint64_t v)
{
    while (v >= 0x80) {
        out.push_back(static_cast<uint8_t>(v | 0x80));
        v >>= 7;
    }
    out.push_back(static_cast<uint8_t>(v));
}

void putTag(std::vector<uint8_t> &out, uint32_t field, uint8_t wire)
{
    putVarint(out, (static_cast<uint64_t>(field) << 3) | wire);
}

void putFixed32(std::vector<uint8_t> &out, uint32_t v)
{
    for (int i = 0; i < 4; i++) {
        out.push_back(static_cast<uint8_t>(v >> (8 * i)));
    }
}

bool getVarint(const uint8_t *buf, size_t len, size_t &pos, uint64_t &v)
{
    v = 0;
    for (int shift = 0; shift < 64; shift += 7) {
        if (pos >= len) {
            return false;
        }
        uint8_t b = buf[pos++];
        v |= static_cast<uint64_t>(b & 0x7F) << shift;
        if (!(b & 0x80)) {
            return true;
        }
    }
    return false; // varint longer than 10 bytes
}

bool getFixed32(const uint8_t *buf, size_t len, size_t &pos, uint32_t &v)
{
    if (pos > len || len - pos < 4) {
        return false;
    }
    v = 0;
    for (int i = 0; i < 4; i++) {
        v |= static_cast<uint32_t>(buf[pos + i]) << (8 * i);
    }
    pos += 4;
    return true;
}

bool skipField(const uint8_t *buf, size_t len, size_t &pos, uint8_t wire)
{
    uint64_t n = 0;
    switch (wire) {
    case kWireVarint:
        return getVarint(buf, len, pos, n);
    case kWireFixed64:
        if (pos > len || len - pos < 8) return false;
        pos += 8;
        return true;
    case kWireLen:
        if (!getVarint(buf, len, pos, n)) return false;
        if (n > len - pos) return false;
        pos += static_cast<size_t>(n);
        return true;
    case kWireFixed32: {
        uint32_t tmp;
        return getFixed32(buf, len, pos, tmp);
    }
    default:
        return false; // groups are not used by Meshtastic
    }
}

} // namespace

std::vector<uint8_t> encodeData(const Data &d)
{
    std::vector<uint8_t> out;
    if (d.portnum != 0) {
        putTag(out, 1, kWireVarint);
        putVarint(out, d.portnum);
    }
    if (!d.payload.empty()) {
        putTag(out, 2, kWireLen);
        putVarint(out, d.payload.size());
        out.insert(out.end(), d.payload.begin(), d.payload.end());
    }
    if (d.wantResponse) {
        putTag(out, 3, kWireVarint);
        putVarint(out, 1);
    }
    if (d.dest != 0) {
        putTag(out, 4, kWireFixed32);
        putFixed32(out, d.dest);
    }
    if (d.source != 0) {
        putTag(out, 5, kWireFixed32);
        putFixed32(out, d.source);
    }
    if (d.requestId != 0) {
        putTag(out, 6, kWireFixed32);
        putFixed32(out, d.requestId);
    }
    if (d.replyId != 0) {
        putTag(out, 7, kWireFixed32);
        putFixed32(out, d.replyId);
    }
    if (d.emoji != 0) {
        putTag(out, 8, kWireFixed32);
        putFixed32(out, d.emoji);
    }
    return out;
}

bool decodeData(const uint8_t *buf, size_t len, Data &out)
{
    out = Data{};
    size_t pos = 0;
    while (pos < len) {
        uint64_t tag = 0;
        if (!getVarint(buf, len, pos, tag)) return false;
        uint32_t field = static_cast<uint32_t>(tag >> 3);
        uint8_t wire = static_cast<uint8_t>(tag & 0x7);
        if (field == 0) return false;

        uint64_t v = 0;
        uint32_t f32 = 0;
        switch (field) {
        case 1:
            if (wire != kWireVarint || !getVarint(buf, len, pos, v)) return false;
            out.portnum = static_cast<uint32_t>(v);
            break;
        case 2: {
            if (wire != kWireLen || !getVarint(buf, len, pos, v)) return false;
            if (v > len - pos) return false;
            out.payload.assign(buf + pos, buf + pos + v);
            pos += static_cast<size_t>(v);
            break;
        }
        case 3:
            if (wire != kWireVarint || !getVarint(buf, len, pos, v)) return false;
            out.wantResponse = (v != 0);
            break;
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            if (wire != kWireFixed32 || !getFixed32(buf, len, pos, f32)) return false;
            if (field == 4) out.dest = f32;
            else if (field == 5) out.source = f32;
            else if (field == 6) out.requestId = f32;
            else if (field == 7) out.replyId = f32;
            else out.emoji = f32;
            break;
        default:
            if (!skipField(buf, len, pos, wire)) return false;
            break;
        }
    }
    return true;
}

} // namespace meshport
