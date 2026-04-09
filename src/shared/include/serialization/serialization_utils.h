#ifndef GODOTPP_SERIALIZATION_UTILS_H
#define GODOTPP_SERIALIZATION_UTILS_H

#include <concepts>
#include <cstdint>
#include <optional>

template<typename T>
concept NetIntegral = std::integral<T>;

template<typename T>
concept NetEnum = std::is_enum_v<T>;

template<typename T, typename Reader, typename Writer>
concept NetSerializable = requires(T t, const T ct, Reader& r, Writer& w) {
    { ct.serialize(w) } -> std::same_as<void>;
    { T::deserialize(r) } -> std::same_as<std::optional<T>>;
};

inline constexpr float k_one_over_sqrt_2 = 0.70710678118654752440F;
inline constexpr uint32_t k_quat_max_value = 1023;

#endif //GODOTPP_SERIALIZATION_UTILS_H
