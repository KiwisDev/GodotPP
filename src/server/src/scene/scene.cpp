#include "scene/scene.h"

#include <net_serializer.h>

Scene::Scene() = default;

Scene::~Scene() = default;

std::vector<uint8_t> Scene::SerializeWorld() {
    NetWriter writer;

    auto view = registry.view<NetworkComponent, TransformComponent>();

    uint32_t entity_count = view.size_hint();
    writer << entity_count;

    for (auto entity : view) {
        const auto& net = view.get<NetworkComponent>(entity);
        const auto& transform = view.get<TransformComponent>(entity);

        TypeID typeID = 1;

        writer << net.id;
        writer << typeID;
        writer << transform.position;
    }

    return writer.Finalize();
}
