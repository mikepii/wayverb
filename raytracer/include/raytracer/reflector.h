#pragma once

#include "raytracer/cl_structs.h"

#include "common/aligned/vector.h"
#include "common/cl_include.h"

namespace raytracer {

class scene_buffers;

class reflector final {
public:
    reflector(const cl::Context&,
              const cl::Device&,
              const glm::vec3& source,
              const glm::vec3& receiver,
              size_t rays);

    aligned::vector<Reflection> run_step(scene_buffers& buffers);

private:
    cl::Context context;
    cl::Device device;
    cl_float3 receiver;
    size_t rays;

    cl::Buffer ray_buffer;
    cl::Buffer reflection_buffer;

    cl::Buffer rng_buffer;
};

}  // namespace raytracer