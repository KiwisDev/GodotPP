#ifndef GODOTPP_UTILS_H
#define GODOTPP_UTILS_H

#include <chrono>

inline uint64_t now_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
        ).count();
}

#endif //GODOTPP_UTILS_H