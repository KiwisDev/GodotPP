#ifndef GODOTPP_SERIALIZER_H
#define GODOTPP_SERIALIZER_H

#include "serialization_utils.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <ranges>
#include <span>
#include <vector>
#include <optional>
#include <stdexcept>

class StreamWriter
{
    std::vector<uint8_t> m_dataBuffer;
    std::vector<uint8_t> m_bitBuffer;
    uint8_t m_currentBitByte = 0;
    uint8_t m_currentBitIndex = 0;

    static constexpr float pow10(const uint32_t k_dec)
    {
        float res = 1.0F;
        for (uint32_t i = 0; i < k_dec; ++i)
        {
            res *= 10.0F;
        }
        return res;
    }

  public:
    explicit StreamWriter(const size_t k_capacity = 256)
    {
        m_dataBuffer.reserve(k_capacity);
    }

    [[nodiscard]] std::vector<uint8_t> finish()
    {
        if (m_currentBitIndex > 0)
        {
            m_bitBuffer.push_back(m_currentBitByte);
        }

        const auto k_bit_byte_count = static_cast<uint16_t>(m_bitBuffer.size());
        m_dataBuffer.insert(m_dataBuffer.end(), m_bitBuffer.begin(), m_bitBuffer.end());
        write_raw(&k_bit_byte_count, sizeof(uint16_t));

        return std::move(m_dataBuffer);
    }

    void write_bool(const bool k_value)
    {
        if (k_value)
        {
            m_currentBitByte |= static_cast<uint8_t>(1U << m_currentBitIndex);
        }

        if (++m_currentBitIndex >= 8)
        {
            m_bitBuffer.push_back(m_currentBitByte);
            m_currentBitByte = 0;
            m_currentBitIndex = 0;
        }
    }

    template<NetIntegral T>
    void write(T value)
    {
        write_raw(&value, sizeof(T));
    }

    void write_float(const float k_val) { write<uint32_t>(std::bit_cast<uint32_t>(k_val)); }

    template<NetEnum T>
    void write(T value)
    {
        write<uint32_t>(static_cast<uint32_t>(value));
    }

    template<std::ranges::input_range R,
                std::invocable<StreamWriter&, std::ranges::range_reference_t<R>> Func>
    void write_array(const R& range, Func writerFunc)
    {
        write<uint32_t>(static_cast<uint32_t>(std::ranges::distance(range)));
        for (auto&& item : range)
        {
            writerFunc(*this, item);
        }
    }

    void write_float_quantized(const float k_val, const float k_min, const float k_max, const uint32_t k_decimal)
    {
        const float k_clamped = std::clamp(k_val, k_min, k_max);
        const float k_normalized = k_clamped - k_min;
        const float k_mult = pow10(k_decimal);
        const auto k_quantized = static_cast<uint64_t>(std::trunc(k_normalized * k_mult));

        if (const float k_range_extents = (k_max - k_min) * k_mult; k_range_extents <= static_cast<float>(UINT8_MAX))
        {
            write<uint8_t>(static_cast<uint8_t>(k_quantized));
        }
        else if (k_range_extents <= static_cast<float>(UINT16_MAX))
        {
            write<uint16_t>(static_cast<uint16_t>(k_quantized));
        }
        else if (k_range_extents <= static_cast<float>(UINT32_MAX))
        {
            write<uint32_t>(static_cast<uint32_t>(k_quantized));
        }
        else
        {
            write<uint64_t>(k_quantized);
        }
    }

    void write_vec2_quantized(const glm::vec2& vec, const float k_min, const float k_max, const uint32_t k_decimal)
    {
        write_float_quantized(vec.x, k_min, k_max, k_decimal);
        write_float_quantized(vec.y, k_min, k_max, k_decimal);
    }

    void write_quat_smallest_three(const glm::quat& q)
    {
        static constexpr size_t k_number_of_comps = 4;
        const std::array<float, k_number_of_comps> k_elements = {q.x, q.y, q.z, q.w};
        uint32_t largestIdx = 0;
        float maxVal = -1.0F;

        for (uint32_t i = 0; i < k_number_of_comps; ++i)
        {
            if (const float k_abs_val = std::abs(k_elements[i]); k_abs_val > maxVal)
            {
                maxVal = k_abs_val;
                largestIdx = i;
            }
        }

        const float k_sign = (k_elements[largestIdx] < 0.0F) ? -1.0F : 1.0F;
        uint32_t packed = largestIdx << 30;
        uint32_t shift = 20;

        for (uint32_t i = 0; i < 4; ++i)
        {
            if (i == largestIdx)
            {
                continue;
            }

            const float k_val = k_elements[i] * k_sign;
            const float k_normalized = (k_val + k_one_over_sqrt_2) / (2.0f * k_one_over_sqrt_2);
            const uint32_t k_quantized =
                std::clamp(static_cast<uint32_t>(std::round(k_normalized * k_quat_max_value)), 0U, k_quat_max_value);

            packed |= (k_quantized << shift);
            shift = (shift > 10) ? shift - 10 : 0;
        }

        write<uint32_t>(packed);
    }

    template<typename T>
    void write_struct(const T& item)
        requires NetSerializable<T, class StreamReader, StreamWriter>
    {
        item.serialize(*this);
    }

    void write_bytes(std::span<const uint8_t> bytes)
    {
        write<uint32_t>(static_cast<uint32_t>(bytes.size()));
        write_raw(bytes.data(), bytes.size());
    }

  private:
    void write_raw(const void* data, const size_t k_size)
    {
        const auto* bytes = static_cast<const uint8_t*>(data);
        m_dataBuffer.insert(m_dataBuffer.end(), bytes, bytes + k_size);
    }
};

class StreamReader
{
    std::span<const uint8_t> m_dataCursor;
    std::span<const uint8_t> m_bitCursor;
    uint8_t m_currentBitByte = 0;
    uint8_t m_currentBitIndex = 8;

    static constexpr float pow10(uint32_t dec)
    {
        float res = 1.0F;
        for (uint32_t i = 0; i < dec; ++i)
        {
            res *= 10.0F;
        }
        return res;
    }

  public:
    explicit StreamReader(const std::span<const uint8_t> k_buffer)
    {
        if (k_buffer.size() < 2)
        {
            throw std::runtime_error("Unexpected EOF");
        }

        uint16_t bitByteCount = 0;
        std::memcpy(&bitByteCount, k_buffer.data() + k_buffer.size() - 2, sizeof(uint16_t));

        if (k_buffer.size() < 2ULL + bitByteCount)
        {
            throw std::runtime_error("Corrupt Footer");
        }

        const size_t k_data_len = k_buffer.size() - 2 - bitByteCount;
        m_dataCursor = k_buffer.subspan(0, k_data_len);
        m_bitCursor = k_buffer.subspan(k_data_len, bitByteCount);
    }

    std::optional<bool> read_bool()
    {
        if (m_currentBitIndex >= 8)
        {
            if (m_bitCursor.empty())
            {
                return std::nullopt;
            }

            m_currentBitByte = m_bitCursor.front();
            m_bitCursor = m_bitCursor.subspan(1);
            m_currentBitIndex = 0;
        }

        const bool k_val = (m_currentBitByte & (1U << m_currentBitIndex)) != 0;
        ++m_currentBitIndex;
        return k_val;
    }

    [[nodiscard]] std::optional<float> read_float()
    {
        const auto k_raw = read<uint32_t>();
        if (!k_raw) return std::nullopt;
        return std::bit_cast<float>(*k_raw);
    }

    [[nodiscard]] std::optional<std::span<const uint8_t>> read_bytes(const uint32_t k_n)
    {
        if (m_dataCursor.size() < k_n)
        {
            return std::nullopt;
        }
        const auto k_result = m_dataCursor.subspan(0, k_n);
        m_dataCursor = m_dataCursor.subspan(k_n);
        return k_result;
    }

    template<NetIntegral T>
    std::optional<T> read()
    {
        if (m_dataCursor.size() < sizeof(T))
        {
            return std::nullopt;
        }

        T val;
        std::memcpy(&val, m_dataCursor.data(), sizeof(T));
        m_dataCursor = m_dataCursor.subspan(sizeof(T));
        return val;
    }

    template<NetEnum T>
    std::optional<T> read_enum()
    {
        const auto k_val = read<uint32_t>();
        if (!k_val)
        {
            return std::nullopt;
        }
        return static_cast<T>(*k_val);
    }

    template<typename T, std::invocable<StreamReader&> Func>
    std::optional<std::vector<T>> read_array(Func readerFunc)
    {
        const auto k_len = read<uint32_t>();
        if (!k_len)
        {
            return std::nullopt;
        }

        std::vector<T> vec;
        vec.reserve(*k_len);
        for (uint32_t i = 0; i < *k_len; ++i)
        {
            auto item = readerFunc(*this);
            if (!item)
            {
                return std::nullopt;
            }
            vec.push_back(*std::move(item));
        }
        return vec;
    }

    std::optional<float> read_float_quantized(const float k_min, const float k_max, const uint32_t k_decimal)
    {
        const float k_mult = pow10(k_decimal);
        const float k_range_extents = (k_max - k_min) * k_mult;
        uint64_t quantized = 0;

        if (k_range_extents <= static_cast<float>(UINT8_MAX))
        {
            const auto k_value_opt = read<uint8_t>();
            if (!k_value_opt)
            {
                return std::nullopt;
            }
            quantized = *k_value_opt;
        }
        else if (k_range_extents <= static_cast<float>(UINT16_MAX))
        {
            const auto k_value_opt = read<uint16_t>();
            if (!k_value_opt)
            {
                return std::nullopt;
            }
            quantized = *k_value_opt;
        }
        else if (k_range_extents <= static_cast<float>(UINT32_MAX))
        {
            const auto k_value_opt = read<uint32_t>();
            if (!k_value_opt)
            {
                return std::nullopt;
            }
            quantized = *k_value_opt;
        }
        else
        {
            const auto k_value_opt = read<uint64_t>();
            if (!k_value_opt)
            {
                return std::nullopt;
            }
            quantized = *k_value_opt;
        }

        return (static_cast<float>(quantized) / k_mult) + k_min;
    }

    std::optional<glm::vec2> read_vec2_quantized(const float k_min, const float k_max, const uint32_t k_decimal)
    {
        const auto k_x = read_float_quantized(k_min, k_max, k_decimal);
        const auto k_y = read_float_quantized(k_min, k_max, k_decimal);
        if (!k_x || !k_y)
        {
            return std::nullopt;
        }
        return glm::vec2{*k_x, *k_y};
    }

    std::optional<glm::quat> read_quat_smallest_three()
    {
        const auto k_packed_opt = read<uint32_t>();
        if (!k_packed_opt)
        {
            return std::nullopt;
        }

        const uint32_t k_packed = *k_packed_opt;
        const uint32_t k_largest_idx = (k_packed >> 30) & 0b11;
        static constexpr size_t k_number_of_comps = 4;
        std::array<float, k_number_of_comps> components = {};
        uint32_t shift = 20;
        float sumSq = 0.0F;

        for (uint32_t i = 0; i < k_number_of_comps; ++i)
        {
            if (i == k_largest_idx)
            {
                continue;
            }

            const uint32_t k_quantized = (k_packed >> shift) & k_quat_max_value;
            const float k_normalized = static_cast<float>(k_quantized) / k_quat_max_value;
            const float k_val = (k_normalized * (2.0F * k_one_over_sqrt_2)) - k_one_over_sqrt_2;
            components[i] = k_val;
            sumSq += k_val * k_val;
            shift = (shift > 10) ? shift - 10 : 0;
        }

        const float k_missing_sq = 1.0F - sumSq;
        components[k_largest_idx] = (k_missing_sq > 0.0F) ? std::sqrt(k_missing_sq) : 0.0F;

        return glm::quat{components[3], components[0], components[1], components[2]};
    }

    template<typename T>
    std::optional<T> read_struct()
        requires NetSerializable<T, StreamReader, StreamWriter>
    {
        return T::deserialize(*this);
    }
};

#endif //GODOTPP_SERIALIZER_H
