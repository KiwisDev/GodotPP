#include "scene/scene.h"

#include <net_serializer.h>
#include "serialization/serializer.h"

Scene::Scene() = default;

Scene::~Scene() = default;

std::vector<uint8_t> Scene::SerializeWorld() {
    StreamWriter writer;

    auto view = registry.view<NetworkComponent, TransformComponent>();

    uint32_t entity_count = view.size_hint();
    writer.write<uint32_t>(entity_count);

    for (auto entity : view) {
        const auto& net = view.get<NetworkComponent>(entity);
        const auto& transform = view.get<TransformComponent>(entity);

        TypeID typeID = 1;

        writer.write<uint32_t>(net.id);
        writer.write<uint32_t>(typeID);
        writer.write_vec2_quantized(transform.position, -5000.0f, 5000.0f, 2);
    }

    return writer.finish();
}
