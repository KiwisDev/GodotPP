#ifndef GODOTPP_NET_PROTOCOL_H
#define GODOTPP_NET_PROTOCOL_H

#include <cstdint>

using NetID = uint32_t;
using TypeID = uint32_t;

enum class PacketType : uint8_t
{
    WORLD = 1,
    HELLO = 2,
    INPUT = 3,
    PING = 4,
    PONG = 5
};

#pragma pack(push, 1)
struct WorldPacket
{
    PacketType type;
    NetID netID;
    TypeID typeID;
    int16_t x;
    int16_t y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct HelloPacket
{
    PacketType type;
    int16_t x;
    int16_t y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct InputData {
    uint32_t sequence;
    uint8_t keys;
    float aim_x;
    float aim_y;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct InputPacket {
    PacketType type;
    InputData input_history[20];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PingRequest
{
    PacketType type;
    uint32_t id;
    uint64_t t0;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PingResponse
{
    PacketType type;
    uint32_t id;
    uint64_t t0;
    uint64_t t1;
};
#pragma pack(pop)

enum InputFlags : uint8_t {
    FLAG_UP     = 1 << 0, // 0000 0001
    FLAG_DOWN   = 1 << 1, // 0000 0010
    FLAG_LEFT   = 1 << 2, // 0000 0100
    FLAG_RIGHT  = 1 << 3, // 0000 1000
    FLAG_ACTION = 1 << 4  // 0001 0000
};

#endif //GODOTPP_NET_PROTOCOL_H