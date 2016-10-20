#include "common/attenuator/hrtf.h"

#include "common/hrtf_look_up_table.h"

namespace attenuator {

hrtf::hrtf(const glm::vec3& pointing,
           const glm::vec3& up,
           channel channel,
           float radius)
        : pointing_{glm::normalize(pointing)}
        , up_{glm::normalize(up)}
        , channel_{channel}
        , radius_{radius} {}

glm::vec3 hrtf::get_pointing() const { return pointing_; }
glm::vec3 hrtf::get_up() const { return up_; }
hrtf::channel hrtf::get_channel() const { return channel_; }
float hrtf::get_radius() const { return radius_; }

void hrtf::set_pointing(const glm::vec3& pointing) {
    pointing_ = glm::normalize(pointing);
}

void hrtf::set_up(const glm::vec3& up) { up_ = glm::normalize(up); }

void hrtf::set_channel(channel channel) { channel_ = channel; }

void hrtf::set_radius(float radius) {
    if (radius < 0 || 1 < radius) {
        throw std::runtime_error{"hrtf radius outside reasonable range"};
    }
    radius_ = radius;
}

//----------------------------------------------------------------------------//

glm::vec3 transform(const glm::vec3& pointing,
                    const glm::vec3& up,
                    const glm::vec3& d) {
    const auto x = glm::normalize(glm::cross(up, pointing));
    const auto y = glm::cross(pointing, x);
    const auto z = pointing;
    return glm::vec3{glm::dot(x, d), glm::dot(y, d), glm::dot(z, d)};
}

float degrees(float radians) { return radians * 180 / M_PI; }

az_el compute_look_up_angles(const glm::vec3& pt) {
    auto radians = compute_azimuth_elevation(pt);
    radians.azimuth = -radians.azimuth;
    while (radians.azimuth < 0) {
        radians.azimuth += M_PI * 2;
    }
    while (radians.elevation < 0) {
        radians.elevation += M_PI * 2;
    }
    return az_el{degrees(radians.azimuth), degrees(radians.elevation)};
}

bands_type attenuation(const hrtf& hrtf, const glm::vec3& incident) {
    if (const auto l = glm::length(incident)) {
        const auto transformed =
                transform(hrtf.get_pointing(), hrtf.get_up(), incident / l);
        const auto look_up_angles = compute_look_up_angles(transformed);
        const auto channels = hrtf_look_up_table::look_up_angles(
                look_up_angles.azimuth, look_up_angles.elevation);
        const auto channel =
                channels[hrtf.get_channel() == hrtf::channel::left ? 0 : 1];
        return to_cl_float_vector(channel);
    }
    return bands_type{};
}

glm::vec3 get_ear_position(const hrtf& hrtf, const glm::vec3& base_position) {
    const auto x = hrtf.get_channel() == hrtf::channel::left
                           ? -hrtf.get_radius()
                           : hrtf.get_radius();
    return base_position +
           transform(hrtf.get_pointing(), hrtf.get_up(), glm::vec3{x, 0, 0});
}

}  // namespace attenuator