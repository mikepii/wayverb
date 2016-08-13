#pragma once

#include "glm/glm.hpp"

#include <random>

glm::vec3 sphere_point(double z, double theta);
glm::vec3 point_on_sphere(double az, double el);

class direction_rng final {
public:
    template <typename T>
    direction_rng(T& engine)
            : z(std::uniform_real_distribution<float>(-1, 1)(engine))
            , theta(std::uniform_real_distribution<float>(-M_PI,
                                                          M_PI)(engine)) {}

    inline float get_z() const { return z; }
    inline float get_theta() const { return theta; }

private:
    float z;      //  -1    to 1
    float theta;  //  -M_PI to M_PI
};

