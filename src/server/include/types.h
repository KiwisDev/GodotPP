#ifndef GODOTPP_TYPES_H
#define GODOTPP_TYPES_H

enum class ConnectionState : uint8_t
{
    CONNECTING = 1,
    CONNECTED = 2,
    SPURIOUS = 3,
};

#endif //GODOTPP_TYPES_H