#ifndef GODOTPP_NET_SERIALIZER_H
#define GODOTPP_NET_SERIALIZER_H

#include <algorithm>
#include <cstdint>
#include <maths_types.h>
#include <cstring>
#include <type_traits>
#include <vector>

template <typename T> struct QuantizedWrite
{
    const T& val; T min_v; T max_v; uint8_t bits;
};

template <typename T> struct QuantizedRead
{
    T& val; T min_v; T max_v; uint8_t bits;
};

auto Quantize(const float& v, float min_v, float max_v, uint8_t bits) {
    return QuantizedWrite<float>{v, min_v, max_v, bits};
}
auto Quantize(float& v, float min_v, float max_v, uint8_t bits) {
    return QuantizedRead<float>{v, min_v, max_v, bits};
}

template <typename T, typename Archive, typename = void>
struct has_serialize : std::false_type {};

template <typename T, typename Archive>
struct has_serialize<T, Archive, std::void_t<decltype(std::declval<T>().serialize(std::declval<Archive&>()))>> : std::true_type {};

template<typename T, typename Archive>
inline constexpr bool has_serialize_v = has_serialize<T, Archive>::value;

template<typename T>
inline constexpr bool is_standard_arithmetic_v = std::is_arithmetic_v<T> && !std::is_same_v<T, bool>;

class NetWriter {
private:
    std::vector<uint8_t> buffer;
    std::vector<bool> bools;

public:
    static constexpr bool is_writing = true;

    // Int + standard floats
    template<typename T, std::enable_if_t<is_standard_arithmetic_v<T>, int> = 0>
    NetWriter& operator<<(const T& val) {
        size_t current_size = buffer.size();
        buffer.resize(current_size + sizeof(T));
        std::memcpy(buffer.data() + current_size, &val, sizeof(T));
        return *this;
    }

    // Bool
    NetWriter& operator<<(bool val) {
        bools.push_back(val);
        return *this;
    }

    // Enums
    template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
    NetWriter& operator<<(const T& val) {
        return *this << static_cast<std::underlying_type_t<T>>(val);
    }

    // Quantized floats
    NetWriter& operator<<(const QuantizedWrite<float>& q) {
        float clamped = std::clamp(q.val, q.min_v, q.max_v);
        float normalized = (clamped - q.min_v) / (q.max_v - q.min_v);
        uint32_t max_val = (1u << q.bits) - 1;
        uint32_t quantized = static_cast<uint32_t>(normalized * max_val + 0.5f);

        if (q.bits <= 8)       *this << static_cast<uint8_t>(quantized);
        else if (q.bits <= 16) *this << static_cast<uint16_t>(quantized);
        else                   *this << quantized;
        return *this;
    }

    // Maths
    NetWriter& operator<<(const Vec2& v) { return *this << v.x << v.y; }
    NetWriter& operator<<(const Vec3& v) { return *this << v.x << v.y << v.z; }
    NetWriter& operator<<(const Quat& q) { return *this << q.x << q.y << q.z << q.w; }

    // std::vector
    template<typename T>
    NetWriter& operator<<(const std::vector<T>& vec) {
        *this << static_cast<uint32_t>(vec.size());
        for (const auto& item : vec) *this << item;
        return *this;
    }

    // std::array
    template<typename T, std::size_t N>
    NetWriter& operator<<(const std::array<T, N>& arr) {
        for (const auto& item : arr) *this << item;
        return *this;
    }

    // Class / struct
    template<typename T, std::enable_if_t<has_serialize_v<T, NetWriter>, int> = 0>
    NetWriter& operator<<(const T& val) {
        const_cast<T&>(val).serialize(*this);
        return *this;
    }

    std::vector<uint8_t> Finalize() {
        std::vector<uint8_t> packet;
        uint32_t data_size = static_cast<uint32_t>(buffer.size());
        uint32_t bool_count = static_cast<uint32_t>(bools.size());
        uint32_t bool_bytes = (bool_count + 7) / 8;

        packet.reserve(sizeof(uint32_t) * 2 + data_size + bool_bytes);

        // Header
        packet.insert(packet.end(), reinterpret_cast<uint8_t*>(&data_size), reinterpret_cast<uint8_t*>(&data_size) + 4);
        packet.insert(packet.end(), reinterpret_cast<uint8_t*>(&bool_count), reinterpret_cast<uint8_t*>(&bool_count) + 4);

        // Byte-aligned data
        packet.insert(packet.end(), buffer.begin(), buffer.end());

        // Bit-aligned data (bools)
        uint8_t current_byte = 0;
        for (size_t i = 0; i < bool_count; ++i) {
            if (bools[i]) current_byte |= (1 << (i % 8));
            if ((i % 8) == 7 || i == bool_count - 1) {
                packet.push_back(current_byte);
                current_byte = 0;
            }
        }
        return packet;
    }
};

class NetReader {
private:
    const std::vector<uint8_t>& packet;
    size_t byte_ptr = 8;

    std::vector<bool> bools;
    size_t bool_ptr = 0;

public:
    static constexpr bool is_writing = false;

    NetReader(const std::vector<uint8_t>& in_packet) : packet(in_packet) {
        if (packet.size() < 8) return;

        uint32_t data_size = *reinterpret_cast<const uint32_t*>(packet.data());
        uint32_t bool_count = *reinterpret_cast<const uint32_t*>(packet.data() + 4);

        size_t bool_offset = 8 + data_size;
        for (size_t i = 0; i < bool_count; ++i) {
            if (bool_offset + (i / 8) < packet.size()) {
                uint8_t byte_val = packet[bool_offset + (i / 8)];
                bools.push_back((byte_val & (1 << (i % 8))) != 0);
            }
        }
    }

    // Int + standard floats
    template<typename T, std::enable_if_t<is_standard_arithmetic_v<T>, int> = 0>
    NetReader& operator>>(T& val) {
        if (byte_ptr + sizeof(T) <= packet.size()) {
            std::memcpy(&val, packet.data() + byte_ptr, sizeof(T));
            byte_ptr += sizeof(T);
        }
        return *this;
    }

    // Bool
    NetReader& operator>>(bool& val) {
        if (bool_ptr < bools.size()) {
            val = bools[bool_ptr++];
        } else {
            val = false; // Fallback par sécurité
        }
        return *this;
    }

    // Enums
    template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
    NetReader& operator>>(T& val) {
        std::underlying_type_t<T> raw;
        *this >> raw;
        val = static_cast<T>(raw);
        return *this;
    }

    // Quantized floats
    NetReader& operator>>(const QuantizedRead<float>& q) {
        uint32_t quantized = 0;
        if (q.bits <= 8) { uint8_t v; *this >> v; quantized = v; }
        else if (q.bits <= 16) { uint16_t v; *this >> v; quantized = v; }
        else { *this >> quantized; }

        uint32_t max_val = (1u << q.bits) - 1;
        float normalized = static_cast<float>(quantized) / max_val;
        q.val = q.min_v + normalized * (q.max_v - q.min_v);
        return *this;
    }

    // Maths
    NetReader& operator>>(Vec2& v) { return *this >> v.x >> v.y; }
    NetReader& operator>>(Vec3& v) { return *this >> v.x >> v.y >> v.z; }
    NetReader& operator>>(Quat& q) { return *this >> q.x >> q.y >> q.z >> q.w; }

    // std::vector
    template<typename T>
    NetReader& operator>>(std::vector<T>& vec) {
        uint32_t size = 0;
        *this >> size;

        // Corrupted vector safety
        if (size < 100000) {
            vec.resize(size);
            for (auto& item : vec) *this >> item;
        }
        return *this;
    }

    // std::array
    template<typename T, std::size_t N>
    NetReader& operator>>(std::array<T, N>& arr) {
        for (auto& item : arr) *this >> item;
        return *this;
    }

    // Class / structs
    template<typename T, std::enable_if_t<has_serialize_v<T, NetReader>, int> = 0>
    NetReader& operator>>(T& val) {
        val.serialize(*this);
        return *this;
    }
};

#endif //GODOTPP_NET_SERIALIZER_H