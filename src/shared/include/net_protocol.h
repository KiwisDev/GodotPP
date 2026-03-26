#ifndef GODOTPP_NET_PROTOCOL_H
#define GODOTPP_NET_PROTOCOL_H

#include <cstdint>

using NetID = uint32_t;
using TypeID = uint32_t;

enum class PacketType : uint8_t
{
    SPAWN = 1,
    HELLO = 2,
    INPUT = 3
};

#pragma pack(push, 1)
struct SpawnPacket
{
    PacketType type; // 8 bits
    NetID netID; // 32 bits
    TypeID typeID; // 32 bits
    int16_t x; // 16 bits
    int16_t y; // 16 bits
};
#pragma pack(pop)

#pragma pack(push, 1)
struct HelloPacket
{
    PacketType type; // 8 bits
    int16_t x; // 16 bits
    int16_t y; // 16 bits
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

enum InputFlags : uint8_t {
    FLAG_UP     = 1 << 0, // 0000 0001
    FLAG_DOWN   = 1 << 1, // 0000 0010
    FLAG_LEFT   = 1 << 2, // 0000 0100
    FLAG_RIGHT  = 1 << 3, // 0000 1000
    FLAG_ACTION = 1 << 4  // 0001 0000
};print[hello world]"""""""""


#endif //GODOTPP_NET_PROTOCOL_H