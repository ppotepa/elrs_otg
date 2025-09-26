#include "crsf_protocol.h"
#include <cmath>

namespace ELRS
{

    uint8_t CrsfProtocol::buildRcChannelsFrame(const uint16_t channels[CRSF_CHANNEL_COUNT],
                                               std::array<uint8_t, 26> &frame_out)
    {
        // CRSF frame structure:
        // [Address] [Length] [Type] [Payload...] [CRC]

        frame_out[0] = CRSF_ADDRESS_FLIGHT_CONTROLLER;       // Address
        frame_out[1] = CRSF_FRAME_CHANNELS_PAYLOAD_SIZE + 2; // Length (payload + type + crc)
        frame_out[2] = CRSF_FRAMETYPE_RC_CHANNELS_PACKED;    // Type

        // Pack 16 channels into 22 bytes
        packChannels(channels, &frame_out[3]);

        // Calculate CRC over length, type, and payload
        uint8_t crc = crc8(&frame_out[1], CRSF_FRAME_CHANNELS_PAYLOAD_SIZE + 1);
        frame_out[25] = crc;

        return 26; // Total frame size
    }

    void CrsfProtocol::packChannels(const uint16_t channels[CRSF_CHANNEL_COUNT],
                                    uint8_t packed_out[CRSF_FRAME_CHANNELS_PAYLOAD_SIZE])
    {
        // Pack 16 channels (11-bit each) into 22 bytes
        // This is a bit-packing operation where each channel uses 11 bits

        packed_out[0] = static_cast<uint8_t>(channels[0] & 0x07FF);
        packed_out[1] = static_cast<uint8_t>((channels[0] & 0x07FF) >> 8 | (channels[1] & 0x07FF) << 3);
        packed_out[2] = static_cast<uint8_t>((channels[1] & 0x07FF) >> 5 | (channels[2] & 0x07FF) << 6);
        packed_out[3] = static_cast<uint8_t>((channels[2] & 0x07FF) >> 2);
        packed_out[4] = static_cast<uint8_t>((channels[2] & 0x07FF) >> 10 | (channels[3] & 0x07FF) << 1);
        packed_out[5] = static_cast<uint8_t>((channels[3] & 0x07FF) >> 7 | (channels[4] & 0x07FF) << 4);
        packed_out[6] = static_cast<uint8_t>((channels[4] & 0x07FF) >> 4 | (channels[5] & 0x07FF) << 7);
        packed_out[7] = static_cast<uint8_t>((channels[5] & 0x07FF) >> 1);
        packed_out[8] = static_cast<uint8_t>((channels[5] & 0x07FF) >> 9 | (channels[6] & 0x07FF) << 2);
        packed_out[9] = static_cast<uint8_t>((channels[6] & 0x07FF) >> 6 | (channels[7] & 0x07FF) << 5);
        packed_out[10] = static_cast<uint8_t>((channels[7] & 0x07FF) >> 3);
        packed_out[11] = static_cast<uint8_t>((channels[8] & 0x07FF));
        packed_out[12] = static_cast<uint8_t>((channels[8] & 0x07FF) >> 8 | (channels[9] & 0x07FF) << 3);
        packed_out[13] = static_cast<uint8_t>((channels[9] & 0x07FF) >> 5 | (channels[10] & 0x07FF) << 6);
        packed_out[14] = static_cast<uint8_t>((channels[10] & 0x07FF) >> 2);
        packed_out[15] = static_cast<uint8_t>((channels[10] & 0x07FF) >> 10 | (channels[11] & 0x07FF) << 1);
        packed_out[16] = static_cast<uint8_t>((channels[11] & 0x07FF) >> 7 | (channels[12] & 0x07FF) << 4);
        packed_out[17] = static_cast<uint8_t>((channels[12] & 0x07FF) >> 4 | (channels[13] & 0x07FF) << 7);
        packed_out[18] = static_cast<uint8_t>((channels[13] & 0x07FF) >> 1);
        packed_out[19] = static_cast<uint8_t>((channels[13] & 0x07FF) >> 9 | (channels[14] & 0x07FF) << 2);
        packed_out[20] = static_cast<uint8_t>((channels[14] & 0x07FF) >> 6 | (channels[15] & 0x07FF) << 5);
        packed_out[21] = static_cast<uint8_t>((channels[15] & 0x07FF) >> 3);
    }

    uint8_t CrsfProtocol::crc8(const uint8_t *data, uint8_t length)
    {
        uint8_t crc = 0;
        for (uint8_t i = 0; i < length; i++)
        {
            crc = crc8_table[crc ^ data[i]];
        }
        return crc;
    }

    uint16_t CrsfProtocol::microsecondsToChannelValue(float microseconds)
    {
        // Convert from 1000-2000us range to CRSF 172-1811 range
        float normalized = (microseconds - 1000.0f) / 1000.0f; // 0.0 to 1.0
        return static_cast<uint16_t>(CRSF_CHANNEL_VALUE_MIN +
                                     normalized * (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN));
    }

    uint16_t CrsfProtocol::mapStickToChannel(float stick_value)
    {
        // Convert from -1.0 to +1.0 range to CRSF 172-1811 range
        float normalized = (stick_value + 1.0f) / 2.0f; // 0.0 to 1.0

        // Clamp to valid range
        if (normalized < 0.0f)
            normalized = 0.0f;
        if (normalized > 1.0f)
            normalized = 1.0f;

        return static_cast<uint16_t>(CRSF_CHANNEL_VALUE_MIN +
                                     normalized * (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN));
    }

    uint16_t CrsfProtocol::mapThrottleToChannel(float throttle_value)
    {
        // Convert from 0.0 to +1.0 range to CRSF 172-1811 range
        float normalized = throttle_value;

        // Clamp to valid range
        if (normalized < 0.0f)
            normalized = 0.0f;
        if (normalized > 1.0f)
            normalized = 1.0f;

        return static_cast<uint16_t>(CRSF_CHANNEL_VALUE_MIN +
                                     normalized * (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN));
    }

} // namespace ELRS
