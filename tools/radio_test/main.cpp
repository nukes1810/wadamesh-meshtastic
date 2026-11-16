// Device radio test for the Meshtastic LoRa path on the T-Deck (SX1262).
// Build and upload:  pio run -e meshport_radio_test -t upload
// Monitor:           pio device monitor -e meshport_radio_test
//
// ANTENNA MUST BE ATTACHED before running. Transmitting without an antenna can damage the radio.
//
// Two T-Decks running this send a text ping every 10 s on the default LongFast channel.
// Each prints what it receives with RSSI and SNR.
//
// Unity build: the meshport sources are included directly so this env needs no extra src files.
#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

#include "../../src/meshport/MeshtasticWire.cpp"
#include "../../src/meshport/MeshtasticCrypto.cpp"
#include "../../src/meshport/MeshtasticCryptoBackend_mbedtls.cpp"
#include "../../src/meshport/MeshtasticProto.cpp"
#include "../../src/meshport/MeshtasticFrame.cpp"
#include "../../src/meshport/MeshtasticChannelPlan.cpp"
#include "../../src/meshport/MeshtasticRadio.cpp"

using namespace meshport;

// Meshtastic T-Deck pins (variants/esp32s3/t-deck/variant.h).
#define TEST_LORA_SCK 40
#define TEST_LORA_MISO 38
#define TEST_LORA_MOSI 41
#define TEST_LORA_CS 9
#define TEST_LORA_RESET 17
#define TEST_LORA_DIO1 45
#define TEST_LORA_BUSY 13

// Default LongFast key (src/mesh/Channels.h defaultpsk, PSK index 1).
static const uint8_t kDefaultKey[16] = {0xd4, 0xf1, 0xbb, 0x3a, 0x20, 0x29, 0x07, 0x59,
                                        0xf0, 0xbc, 0xff, 0xab, 0xcf, 0x4e, 0x69, 0x01};

static const float kRegionStartMHz = 902.0f; // US band
static const float kRegionEndMHz = 928.0f;

static SPIClass loraSpi;
static SX1262 *radio = nullptr;
static MeshtasticRadio *driver = nullptr;
static SeenCache seen(64);
static uint32_t myNode = 0;
static uint32_t packetCounter = 0;
static uint8_t channelByte = 0;
static uint32_t lastTxMs = 0;

void setup()
{
    Serial.begin(115200);
    delay(1500);
    Serial.println("meshport radio test");
    Serial.println("ANTENNA MUST BE ATTACHED");

    uint64_t mac = ESP.getEfuseMac();
    myNode = static_cast<uint32_t>(mac ^ (mac >> 32));

    loraSpi.begin(TEST_LORA_SCK, TEST_LORA_MISO, TEST_LORA_MOSI, TEST_LORA_CS);
    Module *mod = new Module(TEST_LORA_CS, TEST_LORA_DIO1, TEST_LORA_RESET, TEST_LORA_BUSY, loraSpi);
    radio = new SX1262(mod);
    driver = new MeshtasticRadio(*radio);

    uint32_t slot = defaultSlotForChannel("LongFast", kRegionStartMHz, kRegionEndMHz, kLongFastBandwidthKHz / 1000.0f);
    float freq = slotCenterMHz(kRegionStartMHz, kLongFastBandwidthKHz / 1000.0f, slot);

    RadioSettings s{};
    s.freqMHz = freq;
    s.bwKHz = kLongFastBandwidthKHz;
    s.sf = kLongFastSpreadingFactor;
    s.cr = kLongFastCodingRateDenom;
    s.syncWord = kLongFastSyncWord;
    s.preambleLen = kLongFastPreambleLen;
    s.powerDbm = 14;
    s.tcxoVoltage = 1.8f;
    s.useLdo = false;

    if (!driver->begin(s)) {
        Serial.println("radio begin FAILED");
        while (true) {
            delay(1000);
        }
    }
    channelByte = static_cast<uint8_t>(channelHash("LongFast", kDefaultKey, sizeof(kDefaultKey)));
    Serial.printf("radio ok: %.3f MHz, slot %u, node %08lX, channel hash %02X\n", freq, (unsigned)slot,
                  (unsigned long)myNode, channelByte);
}

static void sendPing()
{
    packetCounter++;
    Data d;
    d.portnum = kPortTextMessage;
    char text[48];
    snprintf(text, sizeof(text), "ping %lu from %08lX", (unsigned long)packetCounter, (unsigned long)myNode);
    d.payload.assign(text, text + strlen(text));

    Header h{};
    h.to = 0xFFFFFFFF; // broadcast
    h.from = myNode;
    h.id = packetCounter;
    h.flags = 0x03 | (0x03 << kFlagHopStartShift); // hop limit 3, hop start 3
    h.channel = channelByte;

    std::vector<uint8_t> frame;
    if (!sealFrame(kDefaultKey, sizeof(kDefaultKey), h, d, frame)) {
        Serial.println("seal failed");
        return;
    }
    if (driver->send(frame)) {
        Serial.printf("tx #%lu (%u bytes)\n", (unsigned long)packetCounter, (unsigned)frame.size());
    } else {
        Serial.println("tx failed");
    }
}

void loop()
{
    if (millis() - lastTxMs >= 10000) {
        lastTxMs = millis();
        sendPing();
    }

    std::vector<uint8_t> rx;
    float rssi = 0, snr = 0;
    if (driver->poll(rx, rssi, snr)) {
        Header h{};
        Data d;
        if (!openFrame(kDefaultKey, sizeof(kDefaultKey), rx.data(), rx.size(), h, d)) {
            Serial.printf("rx %u bytes: not decodable (other channel or key)\n", (unsigned)rx.size());
            return;
        }
        if (seen.checkAndInsert(h.from, h.id)) {
            return; // duplicate
        }
        std::string text(d.payload.begin(), d.payload.end());
        Serial.printf("rx from %08lX id %lu rssi %.1f snr %.1f port %lu: %s\n", (unsigned long)h.from,
                      (unsigned long)h.id, rssi, snr, (unsigned long)d.portnum, text.c_str());
    }
}
