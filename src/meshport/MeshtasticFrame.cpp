#include "MeshtasticFrame.h"

#include "MeshtasticCrypto.h"

namespace meshport {

bool sealFrame(const uint8_t *psk, size_t pskLen, const Header &h, const Data &d,
               std::vector<uint8_t> &frame)
{
    if (pskLen != 16 && pskLen != 32) {
        return false;
    }
    std::vector<uint8_t> body = encodeData(d);
    if (kHeaderLen + body.size() > kMaxFrameLen) {
        return false;
    }
    if (!channelCrypt(psk, pskLen, h.from, h.id, body.data(), body.size())) {
        return false;
    }
    frame.assign(kHeaderLen, 0);
    packHeader(h, frame.data());
    frame.insert(frame.end(), body.begin(), body.end());
    return true;
}

bool openFrame(const uint8_t *psk, size_t pskLen, const uint8_t *frame, size_t len,
               Header &h, Data &d)
{
    if (pskLen != 16 && pskLen != 32) {
        return false;
    }
    if (len <= kHeaderLen || len > kMaxFrameLen) {
        return false;
    }
    h = unpackHeader(frame);
    std::vector<uint8_t> body(frame + kHeaderLen, frame + len);
    if (!channelCrypt(psk, pskLen, h.from, h.id, body.data(), body.size())) {
        return false;
    }
    return decodeData(body.data(), body.size(), d);
}

bool decrementHopLimit(Header &h)
{
    uint8_t hopLimit = h.flags & kFlagHopLimitMask;
    if (hopLimit == 0) {
        return false;
    }
    h.flags = static_cast<uint8_t>((h.flags & ~kFlagHopLimitMask) | (hopLimit - 1));
    return true;
}

SeenCache::SeenCache(size_t capacity) : entries_(), capacity_(capacity ? capacity : 1) {}

bool SeenCache::checkAndInsert(uint32_t from, uint32_t id)
{
    for (const Entry &e : entries_) {
        if (e.from == from && e.id == id) {
            return true;
        }
    }
    if (entries_.size() < capacity_) {
        entries_.push_back({from, id});
    } else {
        entries_[next_] = {from, id};
        next_ = (next_ + 1) % capacity_;
    }
    return false;
}

} // namespace meshport
