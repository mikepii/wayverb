#pragma once

#include "combined/model/material.h"
#include "combined/model/output.h"
#include "combined/model/raytracer.h"
#include "combined/model/receiver.h"
#include "combined/model/source.h"
#include "combined/model/waveguide.h"

namespace wayverb {
namespace combined {
namespace model {

/// Holds basically all the app state that will get written to file,
/// other than presets, which will be stored separately.

class persistent final : public owning_member<persistent,
                                              sources_t,
                                              receivers_t,
                                              raytracer,
                                              waveguide,
                                              min_size_vector<material, 1>> {
public:
    using raytracer_t = class raytracer;
    using waveguide_t = class waveguide;
    using materials_t = class vector<material>;

    const auto& sources() const { return get<0>(); }
    auto& sources() { return get<0>(); }

    const auto& receivers() const { return get<1>(); }
    auto& receivers() { return get<1>(); }

    const auto& raytracer() const { return get<2>(); }
    auto& raytracer() { return get<2>(); }

    const auto& waveguide() const { return get<3>(); }
    auto& waveguide() { return get<3>(); }

    const auto& materials() const { return get<4>(); }
    auto& materials() { return get<4>(); }

    NOTIFYING_COPY_ASSIGN_DECLARATION(persistent)
private:
    inline void swap(persistent&) noexcept { using std::swap; };
};

}  // namespace model
}  // namespace combined
}  // namespace wayverb
