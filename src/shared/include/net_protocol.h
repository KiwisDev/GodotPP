#ifndef GODOTPP_NET_PROTOCOL_H
#define GODOTPP_NET_PROTOCOL_H

#include <cstdint>
#include <vector>

#include "serialization/serializer.h"

using NetID = uint32_t;
using TypeID = uint32_t;

enum class PacketType : uint8_t
{
    HELLO = 1,
    INPUT = 2,
    PING = 3,
    PONG = 4,
    WORLD_SNAPSHOT = 5
};

struct WorldSnapshotPacket
{
    PacketType type;
    uint64_t frame_number;
    std::vector<uint8_t> data;

    void serialize(StreamWriter &w) const
    {
        w.write<uint8_t>(static_cast<uint8_t>(type));
        w.write<uint64_t>(frame_number);
        w.write_bytes(data);
    }

    static std::optional<WorldSnapshotPacket> deserialize(StreamReader &r)
    {
        const auto k_type = r.read<uint8_t>();
        const auto k_frame_number = r.read<uint64_t>();
        const auto k_data_size = r.read<uint32_t>();

        if (!k_type || k_frame_number || !k_data_size)
        {
            return std::nullopt;
        }

        const auto k_data = r.read_bytes(*k_data_size);
        std::vector<uint8_t> data(k_data->begin(), k_data->end());

        return WorldSnapshotPacket{static_cast<PacketType>(*k_type), *k_frame_number, std::move(data)};
    }
};

struct HelloPacket
{
    PacketType type;
    int16_t x;
    int16_t y;

    void serialize(StreamWriter &w) const
    {
        w.write<uint8_t>(static_cast<uint8_t>(type));
        w.write<int16_t>(x);
        w.write<int16_t>(y);
    }

    static std::optional<HelloPacket> deserialize(StreamReader &r)
    {
        const auto k_type = r.read<uint8_t>();
        const auto k_x = r.read<int16_t>();
        const auto k_y = r.read<int16_t>();

        if (!k_type || !k_x || !k_y)
        {
            return std::nullopt;
        }

        return HelloPacket{static_cast<PacketType>(*k_type), *k_x, *k_y};
    }
};

struct InputData {
    uint32_t sequence;
    uint8_t keys;
    float aim_x;
    float aim_y;

    void serialize(StreamWriter &w) const
    {
        w.write<uint32_t>(sequence);
        w.write<uint8_t>(keys);
        w.write_float(aim_x);
        w.write_float(aim_y);
    }

    static std::optional<InputData> deserialize(StreamReader &r)
    {
        const auto k_sequence = r.read<uint32_t>();
        const auto k_keys = r.read<uint8_t>();
        const auto k_aim_x = r.read_float();
        const auto k_aim_y = r.read_float();

        if (!k_sequence || !k_keys || !k_aim_x || !k_aim_y)
        {
            return std::nullopt;
        }

        return InputData{*k_sequence, *k_keys, *k_aim_x, *k_aim_y};
    }
};

struct InputPacket {
    PacketType type;
    InputData input_history[20];

    void serialize(StreamWriter &w) const
    {
        w.write<uint8_t>(static_cast<uint8_t>(type));
        for (auto i : input_history)
        {
            i.serialize(w);
        }
    }

    static std::optional<InputPacket> deserialize(StreamReader &r)
    {
        auto k_type = r.read<uint8_t>();
        if (!k_type) { return std::nullopt; }

        InputPacket packet;
        packet.type = static_cast<PacketType>(*k_type);

        for (size_t i = 0; i < 20; ++i)
        {
            auto k_input_data = InputData::deserialize(r);
            if (!k_input_data) { return std::nullopt; }
            packet.input_history[i] = *k_input_data;
        }

        return packet;
    }
};

struct PingRequest
{
    PacketType type;
    uint32_t id;
    uint64_t t0;

    void serialize(StreamWriter &w) const
    {
        w.write<uint8_t>(static_cast<uint8_t>(type));
        w.write<uint32_t>(id);
        w.write<uint64_t>(t0);
    }

    static std::optional<PingRequest> deserialize(StreamReader &r)
    {
        const auto k_type = r.read<uint8_t>();
        const auto k_id = r.read<uint32_t>();
        const auto k_t0 = r.read<uint64_t>();

        if (!k_type || !k_id || !k_t0)
        {
            return std::nullopt;
        }

        return PingRequest{static_cast<PacketType>(*k_type), *k_id, *k_t0};
    }
};

struct PingResponse
{
    PacketType type;
    uint32_t id;
    uint64_t t0;
    uint64_t t1;

    void serialize(StreamWriter &w) const
    {
        w.write<uint8_t>(static_cast<uint8_t>(type));
        w.write<uint32_t>(id);
        w.write<uint64_t>(t0);
        w.write<uint64_t>(t1);
    }

    static std::optional<PingResponse> deserialize(StreamReader &r)
    {
        const auto k_type = r.read<uint8_t>();
        const auto k_id = r.read<uint32_t>();
        const auto k_t0 = r.read<uint64_t>();
        const auto k_t1 = r.read<uint64_t>();

        if (!k_type || !k_id || !k_t0 || !k_t1)
        {
            return std::nullopt;
        }

        return PingResponse{static_cast<PacketType>(*k_type), *k_id, *k_t0, *k_t1};
    }
};

enum InputFlags : uint8_t {
    FLAG_UP     = 1 << 0, // 0000 0001
    FLAG_DOWN   = 1 << 1, // 0000 0010
    FLAG_LEFT   = 1 << 2, // 0000 0100
    FLAG_RIGHT  = 1 << 3, // 0000 1000
    FLAG_ACTION = 1 << 4  // 0001 0000
};

#endif //GODOTPP_NET_PROTOCOL_H