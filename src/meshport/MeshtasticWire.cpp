#include "MeshtasticWire.h"

#include <cstring>

namespace meshport {

static void put32le(uint8_t *p, uint32_t v)
{
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
    p[2] = (v >> 16) & 0xFF;
    p[3] = (v >> 24) & 0xFF;
}

static uint32_t get32le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

size_t packHeader(const Header &h, uint8_t *out)
{
    put32le(out + 0, h.to);
    put32le(out + 4, h.from);
    put32le(out + 8, h.id);
    out[12] = h.flags;
    out[13] = h.channel;
    out[14] = h.nextHop;
    out[15] = h.relayNode;
    return kHeaderLen;
}

Header unpackHeader(const uint8_t *in)
{
    Header h;
    h.to = get32le(in + 0);
    h.from = get32le(in + 4);
    h.id = get32le(in + 8);
    h.flags = in[12];
    h.channel = in[13];
    h.nextHop = in[14];
    h.relayNode = in[15];
    return h;
}

uint8_t xorHash(const uint8_t *p, size_t len)
{
    uint8_t code = 0;
    for (size_t i = 0; i < len; i++)
        code ^= p[i];
    return code;
}

uint8_t channelHash(const char *name, const uint8_t *psk, size_t pskLen)
{
    uint8_t h = xorHash((const uint8_t *)name, strlen(name));
    h ^= xorHash(psk, pskLen);
    return h;
}

void buildChannelNonce(uint32_t fromNode, uint64_t packetId, uint8_t nonce[16])
{
    memset(nonce, 0, 16);
    for (int i = 0; i < 8; i++)
        nonce[i] = (uint8_t)(packetId >> (8 * i));
    put32le(nonce + 8, fromNode);
    // bytes 12..15 stay zero (extraNonce == 0 for channel messages)
}

} // namespace meshport
