// LoRa driver for Meshtastic frames on the SX1262 (T-Deck). Uses RadioLib, which the
// Wadamesh build already links. Compiled only when MESHPORT_RADIO_ENABLED is defined,
// so the default firmware build is unchanged until the device test passes.
#pragma once

#ifdef MESHPORT_RADIO_ENABLED

#include <RadioLib.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace meshport {

struct RadioSettings {
    float freqMHz;
    float bwKHz;
    uint8_t sf;
    uint8_t cr;
    uint8_t syncWord;
    uint16_t preambleLen;
    int8_t powerDbm;
    float tcxoVoltage;
    bool useLdo;
};

class MeshtasticRadio {
public:
    explicit MeshtasticRadio(SX1262 &radio);

    // Configure the chip. Returns false if RadioLib reports an error.
    bool begin(const RadioSettings &s);

    // RadioLib error code from the last begin() call (RADIOLIB_ERR_NONE on success).
    int16_t lastBeginError() const { return lastBeginError_; }

    // Blocking transmit of one frame (header + encrypted payload). Returns false on error.
    bool send(const std::vector<uint8_t> &frame);

    // Start listening. Call again after each received frame.
    bool startReceive();

    // Call from loop(). Returns true when a frame arrived; fills frame, rssi and snr.
    bool poll(std::vector<uint8_t> &frame, float &rssiDbm, float &snrDb);

private:
    SX1262 &radio_;
    int16_t lastBeginError_ = RADIOLIB_ERR_NONE;
    static void onDio1();
    static volatile bool rxFlag_;
};

} // namespace meshport

#endif // MESHPORT_RADIO_ENABLED
