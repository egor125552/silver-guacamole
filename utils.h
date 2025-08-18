#pragma once

#include <random>
#include <SFML/Graphics.hpp>
#include <cmath>
#include <limits>
#include <algorithm>

inline std::mt19937& get_random_engine() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

inline int getInt(int min, int max) {
    std::uniform_int_distribution<> distrib(min, max);
    return distrib(get_random_engine());
}

inline float getFloat(float min, float max) {
    std::uniform_real_distribution<> distrib(min, max);
    return static_cast<float>(distrib(get_random_engine()));
}

inline sf::Vector2f rayIntersectsRect(const sf::Vector2f& rayOrigin, const sf::Vector2f& rayDir, const sf::FloatRect& rect) {
    float t_near = -std::numeric_limits<float>::infinity();
    float t_far = std::numeric_limits<float>::infinity();

    for (int i = 0; i < 2; ++i) {
        float origin_i = (i == 0) ? rayOrigin.x : rayOrigin.y;
        float dir_i = (i == 0) ? rayDir.x : rayDir.y;
        
        float rect_min_i = (i == 0) ? rect.position.x : rect.position.y;
        float rect_max_i = (i == 0) ? rect.position.x + rect.size.x : rect.position.y + rect.size.y;

        if (std::abs(dir_i) < 1e-6) {
            if (origin_i < rect_min_i || origin_i > rect_max_i) return {-1, -1};
        } else {
            float t1 = (rect_min_i - origin_i) / dir_i;
            float t2 = (rect_max_i - origin_i) / dir_i;

            if (t1 > t2) std::swap(t1, t2);
            t_near = std::max(t_near, t1);
            t_far = std::min(t_far, t2);

            if (t_near > t_far || t_far < 0) return {-1, -1};
        }
    }

    if (t_near > 0) return {rayOrigin.x + t_near * rayDir.x, rayOrigin.y + t_near * rayDir.y};
    return {-1, -1};
}