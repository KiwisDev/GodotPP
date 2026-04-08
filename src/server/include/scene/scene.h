#ifndef GODOTPP_SCENE_H
#define GODOTPP_SCENE_H

#include "entt/entt.hpp"
#include "components.h"

class Scene
{
public:
    Scene();
    ~Scene();

    entt::registry registry;

    std::vector<uint8_t> SerializeWorld();
};

#endif //GODOTPP_SCENE_H