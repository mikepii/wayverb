#include "waveguide.h"

#include <iostream>
#include <algorithm>
#include <cstdlib>

using namespace std;

RectangularWaveguide::RectangularWaveguide(const RectangularProgram & program,
                                           cl::CommandQueue & queue,
                                           cl_int3 p)
        : Waveguide(program, queue, p.x * p.y * p.z)
        , p(p) {
}

RectangularWaveguide::size_type RectangularWaveguide::get_index(
    cl_int3 pos) const {
    return pos.x + pos.y * p.x + pos.z * p.x * p.y;
}

cl_float RectangularWaveguide::run_step(cl_float i,
                                        size_type e,
                                        size_type o,
                                        cl_float attenuation,
                                        cl::CommandQueue & queue,
                                        kernel_type & kernel,
                                        size_type nodes,
                                        cl::Buffer & previous,
                                        cl::Buffer & current,
                                        cl::Buffer & next,
                                        cl::Buffer & output) {
    vector<cl_float> out(1);

    kernel(cl::EnqueueArgs(queue, cl::NDRange(p.x, p.y, p.z)),
           e,
           i,
           attenuation,
           next,
           current,
           previous,
           -1,
           o,
           output);

    cl::copy(queue, output, out.begin(), out.end());

    return out.front();
}

TetrahedralWaveguide::TetrahedralWaveguide(const TetrahedralProgram & program,
                                           cl::CommandQueue & queue,
                                           std::vector<Node> nodes)
        : Waveguide<TetrahedralProgram>(program, queue, nodes.size())
        , node_buffer(program.getInfo<CL_PROGRAM_CONTEXT>(),
                      nodes.begin(),
                      nodes.end(),
                      false) {
#ifdef TESTING
    auto fname = build_string("./file-positions.txt");
    cout << "writing file " << fname << endl;
    ofstream file(fname);
    for (const auto & i : nodes) {
        file << i.position.x << " " << i.position.y << " " << i.position.z
             << " " << i.inside << endl;
    }
#endif
}

cl_float TetrahedralWaveguide::run_step(cl_float i,
                                        size_type e,
                                        size_type o,
                                        cl_float attenuation,
                                        cl::CommandQueue & queue,
                                        kernel_type & kernel,
                                        size_type nodes,
                                        cl::Buffer & previous,
                                        cl::Buffer & current,
                                        cl::Buffer & next,
                                        cl::Buffer & output) {
    vector<cl_float> out(1);

    kernel(cl::EnqueueArgs(queue, cl::NDRange(nodes)),
           e,
           i,
           attenuation,
           next,
           current,
           previous,
           node_buffer,
           o,
           output);

    cl::copy(queue, output, out.begin(), out.end());

#ifdef TESTING
    static size_type ind = 0;

    vector<cl_float> node_values(nodes);
    cl::copy(queue, next, node_values.begin(), node_values.end());
    auto fname = build_string("./file-", ind++, ".txt");
    cout << "writing file " << fname << endl;
    ofstream file(fname);
    for (auto j = 0u; j != nodes; ++j) {
        file << node_values[j] << endl;
    }
#endif

    return out.front();
}

RecursiveTetrahedralWaveguide::RecursiveTetrahedralWaveguide(
    const TetrahedralProgram & program,
    cl::CommandQueue & queue,
    const Boundary & boundary,
    Vec3f start,
    float spacing)
        : TetrahedralWaveguide(
              program, queue, tetrahedral_mesh(boundary, start, spacing)) {
}

IterativeTetrahedralWaveguide::IterativeTetrahedralWaveguide(
    const TetrahedralProgram & program,
    cl::CommandQueue & queue,
    const Boundary & boundary,
    float cube_side)
        : TetrahedralWaveguide(
              program,
              queue,
              IterativeTetrahedralMesh(boundary, cube_side).nodes) {
}
