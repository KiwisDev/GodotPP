#ifndef GODOTPP_COMPONENTS_H
#define GODOTPP_COMPONENTS_H

#include <cstdint>

#include "maths_types.h"
#include "net_protocol.h"

struct NetworkComponent
{
    NetID id;
    uint32_t client_owner_id;

    NetworkComponent(NetID id, uint32_t client_owner_id) : id(id), client_owner_id(client_owner_id) {}
};

struct TransformComponent
{
    Vec2 position;
    Vec2 scale;

    TransformComponent(Vec2 position = {0.0, 0.0}, Vec2 scale = {1.0, 1.0}) : position(position), scale(scale) {}
};

struct VelocityComponent
{
    Vec2 velocity;

    VelocityComponent(Vec2 velocity = {0.0, 0.0}) : velocity(velocity) {}
    operator Vec2() const & {return velocity;}
    operator const Vec2() const & { return velocity; }
};

#endif //GODOTPP_COMPONENTS_H