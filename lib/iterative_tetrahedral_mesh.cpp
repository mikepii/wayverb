#include "iterative_tetrahedral_mesh.h"

#include "logger.h"
#include "conversions.h"

#include <algorithm>

using namespace std;

IterativeTetrahedralMesh::Locator::Locator(const Vec3i & pos, int mod_ind)
        : pos(pos)
        , mod_ind(mod_ind) {
}

IterativeTetrahedralMesh::~IterativeTetrahedralMesh() noexcept {}

vector<Vec3f> IterativeTetrahedralMesh::get_scaled_cube() const {
    const vector<Vec3f> basic_cube{
        {0.00, 0.00, 0.00},
        {0.50, 0.00, 0.50},
        {0.25, 0.25, 0.25},
        {0.75, 0.25, 0.75},
        {0.00, 0.50, 0.50},
        {0.50, 0.50, 0.00},
        {0.25, 0.75, 0.75},
        {0.75, 0.75, 0.25},
    };
    vector<Vec3f> ret(basic_cube.size());
    transform(basic_cube.begin(),
              basic_cube.end(),
              ret.begin(),
              [this](auto i) { return i * spacing; });
    return ret;
}

Vec3i IterativeTetrahedralMesh::get_dim() const {
    auto dimensions = boundary.get_dimensions();
    return (dimensions / spacing).map([](auto i) { return ceil(i); }) + 1;
}

vector<Node> IterativeTetrahedralMesh::get_nodes(
    const Boundary & boundary) const {
    auto total_nodes = dim.product() * scaled_cube.size();
    vector<Node> ret(total_nodes);
    auto counter = 0u;
    transform(ret.begin(),
              ret.end(),
              ret.begin(),
              [this, &counter, &boundary](auto i) {
                  Node ret;
                  auto p = this->get_position(this->get_locator(counter));
                  auto neighbors = this->get_neighbors(counter);
                  copy(neighbors.begin(), neighbors.end(), begin(ret.ports));
                  ret.position = convert(p);
                  ret.inside = boundary.inside(p);
                  counter += 1;
                  return ret;
              });
    return ret;
}

IterativeTetrahedralMesh::IterativeTetrahedralMesh(const Boundary & boundary,
                                                   float spacing)
        : boundary(boundary.get_aabb())
        , spacing(spacing)
        , scaled_cube(get_scaled_cube())
        , dim(get_dim())
        , nodes(get_nodes(boundary)) {
}

IterativeTetrahedralMesh::size_type IterativeTetrahedralMesh::get_index(
    const Locator & loc) const {
    auto n = scaled_cube.size();
    return (loc.mod_ind) + (loc.pos.s[0] * n) + (loc.pos.s[1] * dim.s[0] * n) +
           (loc.pos.s[2] * dim.s[0] * dim.s[1] * n);
}

IterativeTetrahedralMesh::Locator IterativeTetrahedralMesh::get_locator(
    size_type index) const {
    auto mod_ind = div((int)index, (int)scaled_cube.size());
    auto x = div(mod_ind.quot, dim.s[0]);
    auto y = div(x.quot, dim.s[1]);
    auto z = div(y.quot, dim.s[2]);
    return IterativeTetrahedralMesh::Locator(Vec3i(x.rem, y.rem, z.rem),
                                             mod_ind.rem);
}

Vec3f IterativeTetrahedralMesh::get_position(const Locator & locator) const {
    auto cube_pos = locator.pos * spacing;
    auto node_pos = scaled_cube[locator.mod_ind];
    return cube_pos + node_pos + boundary.c0;
}

using Locator = IterativeTetrahedralMesh::Locator;

const array<array<Locator, IterativeTetrahedralMesh::PORTS>,
            IterativeTetrahedralMesh::CUBE_NODES>
    IterativeTetrahedralMesh::offset_table = {{
        {{Locator(Vec3i(0, 0, 0), 2),
          Locator(Vec3i(-1, 0, -1), 3),
          Locator(Vec3i(-1, -1, 0), 6),
          Locator(Vec3i(0, -1, -1), 7)}},
        {{Locator(Vec3i(0, 0, 0), 2),
          Locator(Vec3i(0, 0, 0), 3),
          Locator(Vec3i(0, -1, 0), 6),
          Locator(Vec3i(0, -1, 0), 7)}},
        {{Locator(Vec3i(0, 0, 0), 0),
          Locator(Vec3i(0, 0, 0), 1),
          Locator(Vec3i(0, 0, 0), 4),
          Locator(Vec3i(0, 0, 0), 5)}},
        {{Locator(Vec3i(1, 0, 1), 0),
          Locator(Vec3i(0, 0, 0), 1),
          Locator(Vec3i(0, 0, 1), 4),
          Locator(Vec3i(1, 0, 0), 5)}},
        {{Locator(Vec3i(0, 0, 0), 2),
          Locator(Vec3i(0, 0, -1), 3),
          Locator(Vec3i(0, 0, 0), 6),
          Locator(Vec3i(0, 0, -1), 7)}},
        {{Locator(Vec3i(0, 0, 0), 2),
          Locator(Vec3i(-1, 0, 0), 3),
          Locator(Vec3i(-1, 0, 0), 6),
          Locator(Vec3i(0, 0, 0), 7)}},
        {{Locator(Vec3i(1, 1, 0), 0),
          Locator(Vec3i(0, 1, 0), 1),
          Locator(Vec3i(0, 0, 0), 4),
          Locator(Vec3i(1, 0, 0), 5)}},
        {{Locator(Vec3i(0, 1, 1), 0),
          Locator(Vec3i(0, 1, 0), 1),
          Locator(Vec3i(0, 0, 1), 4),
          Locator(Vec3i(0, 0, 0), 5)}},
    }};

array<int, IterativeTetrahedralMesh::PORTS>
IterativeTetrahedralMesh::get_neighbors(size_type index) const {
    auto locator = get_locator(index);
    array<int, PORTS> ret;
    for (auto i = 0u; i != PORTS; ++i) {
        auto relative = offset_table[locator.mod_ind][i];
        auto summed = locator.pos + relative.pos;
        auto is_neighbor = (Vec3i(0) <= summed && summed < dim).all();
        ret[i] =
            is_neighbor ? get_index(Locator(summed, relative.mod_ind)) : -1;
    }
    return ret;
}
