#include "waveguide/fitted_boundary.h"
#include "waveguide/mesh.h"
#include "waveguide/pcs.h"
#include "waveguide/postprocessor/node.h"
#include "waveguide/preprocessor/soft_source.h"
#include "waveguide/waveguide.h"

#include "core/callback_accumulator.h"
#include "core/dsp_vector_ops.h"

#include "frequency_domain/multiband_filter.h"

#include "utilities/map_to_vector.h"
#include "utilities/progress_bar.h"

#include "audio_file/audio_file.h"

#include <iostream>

int main() {
    const wayverb::core::geo::box box{glm::vec3{0}, glm::vec3{1, 1, 12}};
    const auto sample_rate = 5000.0;
    const auto speed_of_sound = 340.0;

    const wayverb::core::compute_context cc{};

    const glm::vec3 source{0.5, 0.5, 0.5};

    util::aligned::vector<glm::vec3> receivers;
    for (auto i = 0ul; i < dimensions(box).z; ++i) {
        receivers.emplace_back(source + glm::vec3{0, 0, i});
    }

    auto voxels_and_mesh = wayverb::waveguide::compute_voxels_and_mesh(
            cc,
            wayverb::core::geo::get_scene_data(
                    box,
                    wayverb::core::make_surface<
                            wayverb::core::simulation_bands>(0, 0)),
            source,
            sample_rate,
            speed_of_sound);

    voxels_and_mesh.m.set_coefficients(
            wayverb::waveguide::to_flat_coefficients(0));

    const auto input_node =
            compute_index(voxels_and_mesh.m.get_descriptor(), source);

    //  Set up receivers.

    auto output_holders =
            util::map_to_vector(begin(receivers), end(receivers), [&](auto i) {
                const auto receiver_index{compute_index(
                        voxels_and_mesh.m.get_descriptor(), i)};
                if (!wayverb::waveguide::is_inside(voxels_and_mesh.m,
                                                   receiver_index)) {
                    throw std::runtime_error{"receiver is outside of mesh!"};
                }
                return wayverb::core::callback_accumulator<
                        wayverb::waveguide::postprocessor::node>{
                        receiver_index};
            });

//  Set up a source signal.
#if 1
    const auto acoustic_impedance = 400.0;
    const auto input_signal =
            wayverb::waveguide::design_pcs_source(1 << 16,
                                                  acoustic_impedance,
                                                  speed_of_sound,
                                                  sample_rate,
                                                  0.05,
                                                  0.01,
                                                  100,
                                                  1)
                    .signal;
#else
    aligned::vector<float> input_signal{1.0f};
    input_signal.resize(1 << 15);
#endif
    auto prep = wayverb::waveguide::preprocessor::make_soft_source(
            input_node, input_signal.begin(), input_signal.end());

    //  Run the simulation.

    util::progress_bar pb;
    wayverb::waveguide::run(cc,
                            voxels_and_mesh.m,
                            prep,
                            [&](auto& a, const auto& b, auto c) {
                                for (auto& i : output_holders) {
                                    i(a, b, c);
                                }
                                set_progress(pb, c, input_signal.size());
                            },
                            true);

    auto outputs = util::map_to_vector(
            begin(output_holders), end(output_holders), [](const auto& i) {
                return i.get_output();
            });
    wayverb::core::normalize(outputs);
    const auto mag_values = util::map_to_vector(
            begin(outputs), end(outputs), [](const auto& i) {
                return wayverb::core::max_mag(i);
            });
    for (auto mag : mag_values) {
        std::cout << "mag: " << mag << '\n';
    }

    //  We expect to see a 1 / r^2 relationship between distance and rms.
    const auto rms_values = util::map_to_vector(
            begin(outputs), end(outputs), [](const auto& i) {
                return frequency_domain::rms(i.begin(), i.end());
            });
    for (auto rms : rms_values) {
        std::cout << "rms: " << rms << '\n';
    }

    auto count = 0;
    for (const auto& i : outputs) {
        //  Write out.
        write(util::build_string("distance_", count, ".wav").c_str(),
              i,
              sample_rate,
              audio_file::format::wav,
              audio_file::bit_depth::pcm16);
        count += 1;
    }

    return EXIT_SUCCESS;
}
