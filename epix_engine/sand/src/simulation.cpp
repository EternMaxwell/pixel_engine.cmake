#include <numbers>

#include "epix/world/sand.h"

using namespace epix::world::sand::components;

EPIX_API CellDef::CellDef(const CellDef& other) : identifier(other.identifier) {
    if (identifier == DefIdentifier::Name) {
        new (&elem_name) std::string(other.elem_name);
    } else {
        elem_id = other.elem_id;
    }
}
EPIX_API CellDef::CellDef(CellDef&& other) : identifier(other.identifier) {
    if (identifier == DefIdentifier::Name) {
        new (&elem_name) std::string(std::move(other.elem_name));
    } else {
        elem_id = other.elem_id;
    }
}
EPIX_API CellDef& CellDef::operator=(const CellDef& other) {
    if (identifier == DefIdentifier::Name) {
        elem_name.~basic_string();
    }
    identifier = other.identifier;
    if (identifier == DefIdentifier::Name) {
        new (&elem_name) std::string(other.elem_name);
    } else {
        elem_id = other.elem_id;
    }
    return *this;
}
EPIX_API CellDef& CellDef::operator=(CellDef&& other) {
    if (identifier == DefIdentifier::Name) {
        elem_name.~basic_string();
    }
    identifier = other.identifier;
    if (identifier == DefIdentifier::Name) {
        new (&elem_name) std::string(std::move(other.elem_name));
    } else {
        elem_id = other.elem_id;
    }
    return *this;
}
EPIX_API CellDef::CellDef(const std::string& name)
    : identifier(DefIdentifier::Name), elem_name(name) {}
EPIX_API CellDef::CellDef(int id)
    : identifier(DefIdentifier::Id), elem_id(id) {}
EPIX_API CellDef::~CellDef() {
    if (identifier == DefIdentifier::Name) {
        elem_name.~basic_string();
    }
}

EPIX_API bool Cell::valid() const { return elem_id >= 0; }
EPIX_API Cell::operator bool() const { return valid(); }
EPIX_API bool Cell::operator!() const { return !valid(); }

EPIX_API Simulation::Chunk::Chunk(int width, int height)
    : cells(width * height, Cell{}),
      width(width),
      height(height),
      updated(width * height, false) {}
EPIX_API Simulation::Chunk::Chunk(const Chunk& other)
    : cells(other.cells),
      width(other.width),
      height(other.height),
      updated(other.updated) {}
EPIX_API Simulation::Chunk::Chunk(Chunk&& other)
    : cells(std::move(other.cells)),
      width(other.width),
      height(other.height),
      updated(std::move(other.updated)) {}
EPIX_API Simulation::Chunk& Simulation::Chunk::operator=(const Chunk& other) {
    assert(width == other.width && height == other.height);
    cells   = other.cells;
    updated = other.updated;
    return *this;
}
EPIX_API Simulation::Chunk& Simulation::Chunk::operator=(Chunk&& other) {
    assert(width == other.width && height == other.height);
    cells   = std::move(other.cells);
    updated = std::move(other.updated);
    return *this;
}
EPIX_API void Simulation::Chunk::reset_updated() {
    std::fill(updated.begin(), updated.end(), false);
}
EPIX_API void Simulation::Chunk::count_time() { time_since_last_swap++; }
EPIX_API Cell& Simulation::Chunk::get(int x, int y) {
    return cells[y * width + x];
}
EPIX_API const Cell& Simulation::Chunk::get(int x, int y) const {
    return cells[y * width + x];
}
EPIX_API Cell& Simulation::Chunk::create(
    int x, int y, const CellDef& def, ElemRegistry& m_registry
) {
    Cell& cell = get(x, y);
    if (cell) return cell;
    cell.elem_id = def.identifier == CellDef::DefIdentifier::Name
                       ? m_registry.elem_id(def.elem_name)
                       : def.elem_id;
    if (cell.elem_id < 0) {
        return cell;
    }
    cell.color = m_registry.get_elem(cell.elem_id).color_gen();
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    static thread_local std::uniform_real_distribution<float> dis(-0.4f, 0.4f);
    cell.inpos = {dis(gen), dis(gen)};
    cell.velocity =
        def.default_vel + glm::vec2{dis(gen) * 0.1f, dis(gen) * 0.1f};
    if (m_registry.get_elem(cell.elem_id).grav_type !=
        Element::GravType::SOLID) {
        cell.freefall = true;
    } else {
        cell.velocity = {0.0f, 0.0f};
        cell.freefall = false;
    }
    return cell;
}
EPIX_API void Simulation::Chunk::swap_area() {
    std::swap(updating_area, updating_area_next);
    updating_area_next[0] = width;
    updating_area_next[1] = 0;
    updating_area_next[2] = height;
    updating_area_next[3] = 0;
}
EPIX_API void Simulation::Chunk::remove(int x, int y) {
    assert(x >= 0 && x < width && y >= 0 && y < height);
    get(x, y).elem_id = -1;
}
EPIX_API Simulation::Chunk::operator bool() const {
    return width && height && !cells.empty();
}
EPIX_API bool Simulation::Chunk::operator!() const {
    return !width || !height || cells.empty();
}
EPIX_API void Simulation::Chunk::mark_updated(int x, int y) {
    updated[y * width + x] = true;
}
EPIX_API bool Simulation::Chunk::is_updated(int x, int y) const {
    return updated[y * width + x];
}
EPIX_API void Simulation::Chunk::touch(int x, int y) {
    assert(x >= 0 && x < width && y >= 0 && y < height);
    if (x < updating_area[0]) updating_area[0] = x;
    if (x > updating_area[1]) updating_area[1] = x;
    if (y < updating_area[2]) updating_area[2] = y;
    if (y > updating_area[3]) updating_area[3] = y;
    if (x < updating_area_next[0]) updating_area_next[0] = x;
    if (x > updating_area_next[1]) updating_area_next[1] = x;
    if (y < updating_area_next[2]) updating_area_next[2] = y;
    if (y > updating_area_next[3]) updating_area_next[3] = y;
}
EPIX_API bool Simulation::Chunk::should_update() const {
    return updating_area[0] <= updating_area[1] &&
           updating_area[2] <= updating_area[3];
}
EPIX_API glm::ivec2 Simulation::Chunk::size() const {
    return glm::ivec2(width, height);
}
EPIX_API bool Simulation::Chunk::contains(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height &&
           cells[y * width + x].elem_id >= 0;
}

void offset_chunks(Simulation::ChunkMap& chunks, int x_offset, int y_offset) {
    assert(x_offset >= 0 && y_offset >= 0);
    auto& size = chunks.size;
    for (int x = size.x - 1; x >= 0; x--) {
        for (int y = size.y - 1; y >= 0; y--) {
            std::swap(
                chunks.chunks[x][y], chunks.chunks[x + x_offset][y + y_offset]
            );
        }
    }
}

EPIX_API Simulation::ChunkMap::ChunkMap(int chunk_size)
    : chunk_size(chunk_size) {}

EPIX_API void Simulation::ChunkMap::load_chunk(
    int x, int y, const Chunk& chunk
) {
    if (size == glm::ivec2(0)) {
        size.x = 1;
        size.y = 1;
        origin = glm::ivec2(x, y);
    }
    if (x < origin.x) {
        size.x += origin.x - x;
        origin.x = x;
        offset_chunks(*this, origin.x - x, 0);
    }
    if (y < origin.y) {
        size.y += origin.y - y;
        origin.y = y;
        offset_chunks(*this, 0, origin.y - y);
    }
    if (x >= origin.x + size.x) {
        size.x = x - origin.x + 1;
        chunks.resize(size.x);
    }
    if (y >= origin.y + size.y) {
        size.y = y - origin.y + 1;
        for (int x = 0; x < size.x; x++) {
            chunks[x].resize(size.y);
        }
    }
    chunks[x - origin.x][y - origin.y] =
        std::move(std::make_shared<Chunk>(chunk));
}
EPIX_API void Simulation::ChunkMap::load_chunk(int x, int y, Chunk&& chunk) {
    if (size == glm::ivec2(0)) {
        size.x = 1;
        size.y = 1;
        origin = glm::ivec2(x, y);
    }
    if (x < origin.x) {
        size.x += origin.x - x;
        origin.x = x;
        offset_chunks(*this, origin.x - x, 0);
    }
    if (y < origin.y) {
        size.y += origin.y - y;
        origin.y = y;
        offset_chunks(*this, 0, origin.y - y);
    }
    if (x >= origin.x + size.x) {
        size.x = x - origin.x + 1;
        chunks.resize(size.x);
    }
    if (y >= origin.y + size.y) {
        size.y = y - origin.y + 1;
        for (int x = 0; x < size.x; x++) {
            chunks[x].resize(size.y);
        }
    }
    chunks[x - origin.x][y - origin.y] =
        std::move(std::make_shared<Chunk>(std::forward<Chunk>(chunk)));
}
EPIX_API void Simulation::ChunkMap::load_chunk(int x, int y) {
    if (size == glm::ivec2(0)) {
        size.x = 1;
        size.y = 1;
        origin = glm::ivec2(x, y);
        chunks.resize(1);
        chunks[0].resize(1);
    }
    if (x < origin.x) {
        size.x += origin.x - x;
        origin.x = x;
        offset_chunks(*this, origin.x - x, 0);
    }
    if (y < origin.y) {
        size.y += origin.y - y;
        origin.y = y;
        offset_chunks(*this, 0, origin.y - y);
    }
    if (x >= origin.x + size.x) {
        size.x = x - origin.x + 1;
        chunks.resize(size.x);
        for (int x = 0; x < size.x; x++) {
            chunks[x].resize(size.y);
        }
    }
    if (y >= origin.y + size.y) {
        size.y = y - origin.y + 1;
        for (int x = 0; x < size.x; x++) {
            chunks[x].resize(size.y);
        }
    }
    chunks[x - origin.x][y - origin.y] =
        std::move(std::make_shared<Chunk>(chunk_size, chunk_size));
}
EPIX_API bool Simulation::ChunkMap::contains(int x, int y) const {
    return x >= origin.x && x < origin.x + size.x && y >= origin.y &&
           y < origin.y + size.y && chunks[x - origin.x][y - origin.y];
}
EPIX_API Simulation::Chunk& Simulation::ChunkMap::get_chunk(int x, int y) {
    return *chunks[x - origin.x][y - origin.y];
}
EPIX_API const Simulation::Chunk& Simulation::ChunkMap::get_chunk(int x, int y)
    const {
    return *chunks[x - origin.x][y - origin.y];
}

EPIX_API Simulation::ChunkMap::iterator::iterator(
    ChunkMap* chunks, int x, int y
)
    : chunk_map(chunks), x(x), y(y) {}

EPIX_API Simulation::ChunkMap::iterator&
Simulation::ChunkMap::iterator::operator++() {
    int start_y = y + 1;
    for (int tx = x; tx < chunk_map->origin.x + chunk_map->size.x; tx++) {
        for (int ty = start_y; ty < chunk_map->origin.y + chunk_map->size.y;
             ty++) {
            if (chunk_map->contains(tx, ty)) {
                x = tx;
                y = ty;
                return *this;
            }
        }
        start_y = chunk_map->origin.y;
    }
    x = chunk_map->origin.x + chunk_map->size.x;
    y = chunk_map->origin.y + chunk_map->size.y;
    return *this;
}

EPIX_API bool Simulation::ChunkMap::iterator::operator==(const iterator& other
) const {
    return x == other.x && y == other.y;
}

EPIX_API bool Simulation::ChunkMap::iterator::operator!=(const iterator& other
) const {
    return !(x == other.x && y == other.y);
}

EPIX_API std::pair<glm::ivec2, Simulation::Chunk&>
Simulation::ChunkMap::iterator::operator*() {
    return {{x, y}, chunk_map->get_chunk(x, y)};
}

EPIX_API Simulation::ChunkMap::const_iterator::const_iterator(
    const ChunkMap* chunks, int x, int y
)
    : chunk_map(chunks), x(x), y(y) {}

EPIX_API Simulation::ChunkMap::const_iterator&
Simulation::ChunkMap::const_iterator::operator++() {
    int start_y = y + 1;
    for (int tx = x; tx < chunk_map->origin.x + chunk_map->size.x; tx++) {
        for (int ty = start_y; ty < chunk_map->origin.y + chunk_map->size.y;
             ty++) {
            if (chunk_map->contains(tx, ty)) {
                x = tx;
                y = ty;
                return *this;
            }
        }
        start_y = chunk_map->origin.y;
    }
    x = chunk_map->origin.x + chunk_map->size.x;
    y = chunk_map->origin.y + chunk_map->size.y;
    return *this;
}

EPIX_API bool Simulation::ChunkMap::const_iterator::operator==(
    const const_iterator& other
) const {
    return x == other.x && y == other.y;
}

EPIX_API bool Simulation::ChunkMap::const_iterator::operator!=(
    const const_iterator& other
) const {
    return !(x == other.x && y == other.y);
}

EPIX_API std::pair<glm::ivec2, const Simulation::Chunk&>
Simulation::ChunkMap::const_iterator::operator*() {
    return {{x, y}, chunk_map->get_chunk(x, y)};
}

EPIX_API Simulation::ChunkMap::iterator Simulation::ChunkMap::begin() {
    for (int y = origin.y; y < origin.y + size.y; y++) {
        for (int x = origin.x; x < origin.x + size.x; x++) {
            if (contains(x, y)) {
                return iterator(this, x, y);
            }
        }
    }
    return end();
}
EPIX_API Simulation::ChunkMap::const_iterator Simulation::ChunkMap::begin(
) const {
    for (int y = origin.y; y < origin.y + size.y; y++) {
        for (int x = origin.x; x < origin.x + size.x; x++) {
            if (contains(x, y)) {
                return const_iterator(this, x, y);
            }
        }
    }
    return end();
}
EPIX_API Simulation::ChunkMap::iterator Simulation::ChunkMap::end() {
    return iterator(this, origin.x + size.x, origin.y + size.y);
}
EPIX_API Simulation::ChunkMap::const_iterator Simulation::ChunkMap::end(
) const {
    return const_iterator(this, origin.x + size.x, origin.y + size.y);
}

EPIX_API void Simulation::ChunkMap::reset_updated() {
    for (auto&& [pos, chunk] : *this) {
        chunk.reset_updated();
    }
}
EPIX_API void Simulation::ChunkMap::count_time() {
    for (auto&& [pos, chunk] : *this) {
        chunk.count_time();
        if (chunk.time_since_last_swap > chunk.time_threshold) {
            chunk.time_since_last_swap = 0;
            chunk.swap_area();
        }
        chunk.time_threshold = 8;
    }
}

EPIX_API Cell& Simulation::create_def(int x, int y, const CellDef& def) {
    assert(valid(x, y));
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
    touch(x, y);
    touch(x - 1, y);
    touch(x + 1, y);
    touch(x, y - 1);
    touch(x, y + 1);
    return m_chunk_map.get_chunk(chunk_x, chunk_y)
        .create(cell_x, cell_y, def, m_registry);
}
EPIX_API Simulation::Simulation(const ElemRegistry& registry, int chunk_size)
    : m_registry(registry),
      m_chunk_size(chunk_size),
      m_chunk_map{chunk_size},
      max_travel({chunk_size / 2, chunk_size / 2}) {}
EPIX_API Simulation::Simulation(ElemRegistry&& registry, int chunk_size)
    : m_registry(std::move(registry)),
      m_chunk_size(chunk_size),
      m_chunk_map{chunk_size},
      max_travel({chunk_size / 2, chunk_size / 2}) {}

EPIX_API int Simulation::chunk_size() const { return m_chunk_size; }
EPIX_API ElemRegistry& Simulation::registry() { return m_registry; }
EPIX_API const ElemRegistry& Simulation::registry() const { return m_registry; }
EPIX_API Simulation::ChunkMap& Simulation::chunk_map() { return m_chunk_map; }
EPIX_API const Simulation::ChunkMap& Simulation::chunk_map() const {
    return m_chunk_map;
}
EPIX_API void Simulation::reset_updated() { m_chunk_map.reset_updated(); }
EPIX_API void Simulation::mark_updated(int x, int y) {
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
    m_chunk_map.get_chunk(chunk_x, chunk_y).mark_updated(cell_x, cell_y);
}
EPIX_API bool Simulation::is_updated(int x, int y) const {
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
    return m_chunk_map.get_chunk(chunk_x, chunk_y).is_updated(cell_x, cell_y);
}
EPIX_API void Simulation::load_chunk(int x, int y, const Chunk& chunk) {
    m_chunk_map.load_chunk(x, y, chunk);
}
EPIX_API void Simulation::load_chunk(int x, int y, Chunk&& chunk) {
    m_chunk_map.load_chunk(x, y, std::move(chunk));
}
EPIX_API void Simulation::load_chunk(int x, int y) {
    m_chunk_map.load_chunk(x, y);
}
EPIX_API std::pair<int, int> Simulation::to_chunk_pos(int x, int y) const {
    std::pair<int, int> pos;
    if (x < 0) {
        pos.first = (x + 1) / m_chunk_size - 1;
    } else {
        pos.first = x / m_chunk_size;
    }
    if (y < 0) {
        pos.second = (y + 1) / m_chunk_size - 1;
    } else {
        pos.second = y / m_chunk_size;
    }
    return pos;
}
EPIX_API std::pair<int, int> Simulation::to_in_chunk_pos(int x, int y) const {
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    return {x - chunk_x * m_chunk_size, y - chunk_y * m_chunk_size};
}
EPIX_API bool Simulation::contain_cell(int x, int y) const {
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
    return m_chunk_map.contains(chunk_x, chunk_y) &&
           m_chunk_map.get_chunk(chunk_x, chunk_y).contains(cell_x, cell_y);
}
EPIX_API bool Simulation::valid(int x, int y) const {
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    return m_chunk_map.contains(chunk_x, chunk_y);
}
EPIX_API std::tuple<Cell&, const Element&> Simulation::get(int x, int y) {
    assert(contain_cell(x, y));
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
    Cell& cell = m_chunk_map.get_chunk(chunk_x, chunk_y).get(cell_x, cell_y);
    const Element& elem = m_registry.get_elem(cell.elem_id);
    return {cell, elem};
}
EPIX_API std::tuple<const Cell&, const Element&> Simulation::get(int x, int y)
    const {
    assert(contain_cell(x, y));
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
    const Cell& cell =
        m_chunk_map.get_chunk(chunk_x, chunk_y).get(cell_x, cell_y);
    const Element& elem = m_registry.get_elem(cell.elem_id);
    return {cell, elem};
}
EPIX_API Cell& Simulation::get_cell(int x, int y) {
    assert(valid(x, y));
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
    return m_chunk_map.get_chunk(chunk_x, chunk_y).get(cell_x, cell_y);
}
EPIX_API const Cell& Simulation::get_cell(int x, int y) const {
    assert(valid(x, y));
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
    return m_chunk_map.get_chunk(chunk_x, chunk_y).get(cell_x, cell_y);
}
EPIX_API const Element& Simulation::get_elem(int x, int y) const {
    assert(contain_cell(x, y));
    auto id = get_cell(x, y).elem_id;
    assert(id >= 0);
    return m_registry.get_elem(id);
}
EPIX_API void Simulation::remove(int x, int y) {
    assert(valid(x, y));
    touch(x, y);
    touch(x - 1, y);
    touch(x + 1, y);
    touch(x, y - 1);
    touch(x, y + 1);
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
    m_chunk_map.get_chunk(chunk_x, chunk_y).remove(cell_x, cell_y);
}
EPIX_API glm::vec2 Simulation::get_grav(int x, int y) {
    assert(valid(x, y));
    glm::vec2 grav = {0.0f, -98.0f};
    // float dist     = std::sqrt(x * x + y * y);
    // if (dist == 0) return grav;
    // glm::vec2 addition = {
    //     (float)x / dist / dist * 100000, (float)y / dist / dist * 100000
    // };
    // float len_addition =
    //     std::sqrt(addition.x * addition.x + addition.y * addition.y);
    // if (len_addition >= 200) {
    //     addition *= 200 / len_addition;
    // }
    // grav -= addition;
    return grav;
}
static float calculate_angle_diff(const glm::vec2& a, const glm::vec2& b) {
    float dot = a.x * b.x + a.y * b.y;
    float det = a.x * b.y - a.y * b.x;
    return std::atan2(det, dot);
}
EPIX_API glm::vec2 Simulation::get_default_vel(int x, int y) {
    assert(valid(x, y));
    auto grav = get_grav(x, y);
    return {grav.x * 0.4f, grav.y * 0.4f};
}
EPIX_API int Simulation::not_moving_threshold(int x, int y) {
    // the larger the gravity, the smaller the threshold
    auto grav = get_grav(x, y);
    auto len  = std::sqrt(grav.x * grav.x + grav.y * grav.y);
    if (len == 0) return std::numeric_limits<int>::max();
    return not_moving_threshold_setting.numerator /
           std::pow(len, not_moving_threshold_setting.power);
}
EPIX_API void Simulation::UpdateState::next() {
    uint8_t state = 0;
    state |= xorder ? 1 : 0;
    state |= yorder ? 2 : 0;
    state |= x_outer ? 4 : 0;
    state--;
    xorder  = state & 1;
    yorder  = state & 2;
    x_outer = state & 4;
}
void apply_viscosity(Simulation& sim, int x, int y) {
    if (!sim.valid(x, y)) return;
    auto [cell, elem]                     = sim.get(x, y);
    std::vector<std::pair<int, int>> dirs = {
        {0, 1}, {0, -1}, {1, 0}, {-1, 0}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    for (auto&& dir : dirs) {
        int tx = x + dir.first;
        int ty = y + dir.second;
        if (!sim.valid(tx, ty) || !sim.contain_cell(tx, ty)) continue;
        auto [tcell, telem] = sim.get(tx, ty);
        if (telem.grav_type == Element::GravType::SOLID ||
            telem.grav_type == Element::GravType::POWDER) {
            continue;
        }
        if (telem.grav_type == Element::GravType::LIQUID) {
            auto diff = cell.velocity - tcell.velocity;
            cell.velocity -= diff * 0.45f;
            tcell.velocity += diff * 0.45f;
        }
    }
}
EPIX_API void Simulation::update(float delta) {
    std::vector<glm::ivec2> chunks_to_update;
    for (auto&& [pos, chunk] : m_chunk_map) {
        chunks_to_update.push_back(pos);
    }
    reset_updated();
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    static thread_local std::uniform_real_distribution<float> dis(-0.3f, 0.3f);
    update_state.next();
    bool xorder  = update_state.xorder;
    bool yorder  = update_state.yorder;
    bool x_outer = update_state.x_outer;
    if (update_state.random_state) {
        xorder  = dis(gen) > 0;
        yorder  = dis(gen) > 0;
        x_outer = dis(gen) > 0;
    }
    std::sort(
        chunks_to_update.begin(), chunks_to_update.end(),
        [&](auto& a, auto& b) {
            if (x_outer) {
                return (xorder ? a.x < b.x : a.x > b.x) &&
                       (yorder ? a.y < b.y : a.y > b.y);
            } else {
                return (yorder ? a.y < b.y : a.y > b.y) &&
                       (xorder ? a.x < b.x : a.x > b.x);
            }
        }
    );
    for (auto& pos : chunks_to_update) {
        auto& chunk = m_chunk_map.get_chunk(pos.x, pos.y);
        if (!chunk.should_update()) {
            continue;
        }
        std::pair<std::tuple<int, int, int>, std::tuple<int, int, int>> bounds;
        auto& [xbounds, ybounds] = bounds;
        xbounds                  = {
            xorder ? chunk.updating_area[0] : chunk.updating_area[1],
            xorder ? chunk.updating_area[1] + 1 : chunk.updating_area[0] - 1,
            xorder ? 1 : -1
        };
        ybounds = {
            yorder ? chunk.updating_area[2] : chunk.updating_area[3],
            yorder ? chunk.updating_area[3] + 1 : chunk.updating_area[2] - 1,
            yorder ? 1 : -1
        };
        if (!x_outer) {
            std::swap(xbounds, ybounds);
        }
        for (int index1 = std::get<0>(bounds.first);
             index1 != std::get<1>(bounds.first);
             index1 += std::get<2>(bounds.first)) {
            for (int index2 = std::get<0>(bounds.second);
                 index2 != std::get<1>(bounds.second);
                 index2 += std::get<2>(bounds.second)) {
                const int x  = x_outer ? index1 : index2;
                const int y  = x_outer ? index2 : index1;
                const int x_ = pos.x * m_chunk_size + x;
                const int y_ = pos.y * m_chunk_size + y;
                int final_x  = x_;
                int final_y  = y_;
                if (chunk.is_updated(x, y)) continue;
                if (!contain_cell(x_, y_)) continue;
                auto [cell, elem] = get(x_, y_);
                auto grav         = get_grav(x_, y_);
                float grav_len_s  = grav.x * grav.x + grav.y * grav.y;
                if (grav_len_s == 0) {
                    chunk.time_threshold = std::numeric_limits<int>::max();
                } else {
                    chunk.time_threshold = std::max(
                        (int)(8 * 10000 / grav_len_s), chunk.time_threshold
                    );
                }
                if (elem.grav_type == Element::GravType::POWDER) {
                    if (!cell.freefall) {
                        float angle = std::atan2(grav.y, grav.x);
                        // into a 8 direction
                        angle = std::round(angle / (std::numbers::pi / 4)) *
                                (std::numbers::pi / 4);
                        glm::ivec2 dir = {
                            std::round(std::cos(angle)),
                            std::round(std::sin(angle))
                        };
                        int below_x = x_ + dir.x;
                        int below_y = y_ + dir.y;
                        if (!valid(below_x, below_y)) continue;
                        auto& bcell = get_cell(below_x, below_y);
                        if (bcell) {
                            auto& belem = get_elem(below_x, below_y);
                            if (belem.grav_type == Element::GravType::SOLID) {
                                continue;
                            }
                            if (belem.grav_type == Element::GravType::POWDER &&
                                !bcell.freefall) {
                                continue;
                            }
                        }
                        cell.velocity = get_default_vel(x_, y_);
                        cell.freefall = true;
                    }
                    cell.velocity += grav * delta;
                    cell.velocity *= 0.99f;
                    cell.inpos += cell.velocity * delta;
                    int delta_x = std::round(cell.inpos.x);
                    int delta_y = std::round(cell.inpos.y);
                    if (delta_x == 0 && delta_y == 0) {
                        continue;
                    }
                    cell.inpos.x -= delta_x;
                    cell.inpos.y -= delta_y;
                    if (max_travel) {
                        delta_x =
                            std::clamp(delta_x, -max_travel->x, max_travel->y);
                        delta_y =
                            std::clamp(delta_y, -max_travel->x, max_travel->y);
                    }
                    int tx              = x_ + delta_x;
                    int ty              = y_ + delta_y;
                    bool moved          = false;
                    auto raycast_result = raycast_to(x_, y_, tx, ty);
                    auto ncell =
                        &get_cell(raycast_result.new_x, raycast_result.new_y);
                    if (raycast_result.steps) {
                        std::swap(cell, *ncell);
                        final_x = raycast_result.new_x;
                        final_y = raycast_result.new_y;
                        moved   = true;
                    }
                    if (raycast_result.hit) {
                        auto [hit_x, hit_y]    = raycast_result.hit.value();
                        bool blocking_freefall = false;
                        bool collided          = false;
                        if (valid(hit_x, hit_y)) {
                            auto [tcell, telem] = get(hit_x, hit_y);
                            if (telem.grav_type == Element::GravType::SOLID ||
                                telem.grav_type == Element::GravType::POWDER) {
                                collided = collide(
                                    raycast_result.new_x, raycast_result.new_y,
                                    hit_x, hit_y
                                );
                                blocking_freefall = tcell.freefall;
                            } else {
                                std::swap(*ncell, tcell);
                                final_x = hit_x;
                                final_y = hit_y;
                                ncell   = &tcell;
                                moved   = true;
                            }
                        }
                        if (!moved) {
                            if (powder_always_slide ||
                                (valid(hit_x, hit_y) &&
                                 !get_cell(hit_x, hit_y).freefall)) {
                                // try go to left bottom and right bottom
                                float vel_angle = std::atan2(
                                    cell.velocity.y, cell.velocity.x
                                );
                                float grav_angle = std::atan2(grav.y, grav.x);
                                float angle_diff =
                                    calculate_angle_diff(cell.velocity, grav);
                                if (std::abs(angle_diff) <
                                    std::numbers::pi / 2) {
                                    glm::ivec2 lb = {
                                        std::round(std::cos(
                                            grav_angle - std::numbers::pi / 4
                                        )),
                                        std::round(std::sin(
                                            grav_angle - std::numbers::pi / 4
                                        ))
                                    };
                                    glm::ivec2 rb = {
                                        std::round(std::cos(
                                            grav_angle + std::numbers::pi / 4
                                        )),
                                        std::round(std::sin(
                                            grav_angle + std::numbers::pi / 4
                                        ))
                                    };
                                    glm::vec2 dirs[2];
                                    bool lb_first = dis(gen) > 0;
                                    if (lb_first) {
                                        dirs[0] = lb;
                                        dirs[1] = rb;
                                    } else {
                                        dirs[0] = rb;
                                        dirs[1] = lb;
                                    }
                                    for (auto&& vel : dirs) {
                                        int delta_x = vel.x;
                                        int delta_y = vel.y;
                                        if (delta_x == 0 && delta_y == 0) {
                                            continue;
                                        }
                                        int tx = final_x + delta_x;
                                        int ty = final_y + delta_y;
                                        if (!valid(tx, ty)) continue;
                                        auto& tcell = get_cell(tx, ty);
                                        if (!tcell) {
                                            std::swap(*ncell, tcell);
                                            ncell   = &tcell;
                                            final_x = tx;
                                            final_y = ty;
                                            moved   = true;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        if (!blocking_freefall && !moved) {
                            ncell->freefall = false;
                            ncell->velocity = {0.0f, 0.0f};
                        }
                    }
                    if (moved) {
                        ncell->not_move_count = 0;
                        touch(x_ - 1, y_);
                        touch(x_ + 1, y_);
                        touch(x_, y_ - 1);
                        touch(x_, y_ + 1);
                        touch(final_x - 1, final_y);
                        touch(final_x + 1, final_y);
                        touch(final_x, final_y - 1);
                        touch(final_x, final_y + 1);
                    } else {
                        cell.not_move_count++;
                        if (cell.not_move_count >=
                            not_moving_threshold(x_, y_)) {
                            cell.not_move_count = 0;
                            cell.freefall       = false;
                            cell.velocity       = {0.0f, 0.0f};
                        }
                    }
                    mark_updated(final_x, final_y);
                } else if (elem.grav_type == Element::GravType::LIQUID) {
                    if (!cell.freefall) {
                        float angle = std::atan2(grav.y, grav.x);
                        // into a 8 direction
                        angle = std::round(angle / (std::numbers::pi / 4)) *
                                (std::numbers::pi / 4);
                        glm::ivec2 dir = {
                            std::round(std::cos(angle)),
                            std::round(std::sin(angle))
                        };
                        glm::ivec2 lb = {
                            std::round(std::cos(angle - std::numbers::pi / 4)),
                            std::round(std::sin(angle - std::numbers::pi / 4))
                        };
                        glm::ivec2 rb = {
                            std::round(std::cos(angle + std::numbers::pi / 4)),
                            std::round(std::sin(angle + std::numbers::pi / 4))
                        };
                        int below_x = x_ + dir.x;
                        int below_y = y_ + dir.y;
                        int lb_x    = x_ + lb.x;
                        int lb_y    = y_ + lb.y;
                        int rb_x    = x_ + rb.x;
                        int rb_y    = y_ + rb.y;
                        if (!valid(below_x, below_y) && !valid(lb_x, lb_y) &&
                            !valid(rb_x, rb_y)) {
                            continue;
                        }
                        bool should_freefall = false;
                        if (valid(below_x, below_y)) {
                            auto& bcell = get_cell(below_x, below_y);
                            bool shouldnot_freefall = false;
                            if (bcell) {
                                auto& belem = get_elem(below_x, below_y);
                                if (belem.grav_type ==
                                    Element::GravType::SOLID) {
                                    shouldnot_freefall = true;
                                }
                                if ((belem.grav_type ==
                                         Element::GravType::POWDER &&
                                     !bcell.freefall) ||
                                    belem.grav_type ==
                                        Element::GravType::LIQUID) {
                                    shouldnot_freefall = true;
                                }
                            }
                            should_freefall |= !shouldnot_freefall;
                        }
                        if (valid(lb_x, lb_y)) {
                            auto& lbcell            = get_cell(lb_x, lb_y);
                            bool shouldnot_freefall = false;
                            if (lbcell) {
                                auto& lbelem = get_elem(lb_x, lb_y);
                                if (lbelem.grav_type ==
                                    Element::GravType::SOLID) {
                                    shouldnot_freefall = true;
                                }
                                if ((lbelem.grav_type ==
                                         Element::GravType::POWDER &&
                                     !lbcell.freefall) ||
                                    lbelem.grav_type ==
                                        Element::GravType::LIQUID) {
                                    shouldnot_freefall = true;
                                }
                            }
                            should_freefall |= !shouldnot_freefall;
                        }
                        if (valid(rb_x, rb_y)) {
                            auto& rbcell            = get_cell(rb_x, rb_y);
                            bool shouldnot_freefall = false;
                            if (rbcell) {
                                auto& rbelem = get_elem(rb_x, rb_y);
                                if (rbelem.grav_type ==
                                    Element::GravType::SOLID) {
                                    shouldnot_freefall = true;
                                }
                                if ((rbelem.grav_type ==
                                         Element::GravType::POWDER &&
                                     !rbcell.freefall) ||
                                    rbelem.grav_type ==
                                        Element::GravType::LIQUID) {
                                    shouldnot_freefall = true;
                                }
                            }
                            should_freefall |= !shouldnot_freefall;
                        }
                        if (!should_freefall) continue;

                        cell.velocity = get_default_vel(x_, y_);
                        cell.freefall = true;
                    }
                    cell.velocity += grav * delta;
                    cell.velocity *= 0.99f;
                    cell.inpos += cell.velocity * delta;
                    int delta_x = std::round(cell.inpos.x);
                    int delta_y = std::round(cell.inpos.y);
                    if (delta_x == 0 && delta_y == 0) {
                        continue;
                    }
                    cell.inpos.x -= delta_x;
                    cell.inpos.y -= delta_y;
                    if (max_travel) {
                        delta_x =
                            std::clamp(delta_x, -max_travel->x, max_travel->y);
                        delta_y =
                            std::clamp(delta_y, -max_travel->x, max_travel->y);
                    }
                    int tx              = x_ + delta_x;
                    int ty              = y_ + delta_y;
                    bool moved          = false;
                    auto raycast_result = raycast_to(x_, y_, tx, ty);
                    auto ncell =
                        &get_cell(raycast_result.new_x, raycast_result.new_y);
                    if (raycast_result.steps) {
                        std::swap(cell, *ncell);
                        final_x = raycast_result.new_x;
                        final_y = raycast_result.new_y;
                        moved   = true;
                    }
                    if (raycast_result.hit) {
                        auto [hit_x, hit_y]    = raycast_result.hit.value();
                        bool blocking_freefall = false;
                        bool collided          = false;
                        if (valid(hit_x, hit_y)) {
                            auto [tcell, telem] = get(hit_x, hit_y);
                            if (telem.grav_type == Element::GravType::SOLID ||
                                telem.grav_type == Element::GravType::POWDER ||
                                telem.grav_type == Element::GravType::LIQUID) {
                                collided = collide(
                                    raycast_result.new_x, raycast_result.new_y,
                                    hit_x, hit_y
                                );
                            } else {
                                std::swap(*ncell, tcell);
                                final_x = hit_x;
                                final_y = hit_y;
                                ncell   = &tcell;
                                moved   = true;
                            }
                        }
                        if (!moved) {
                            float vel_angle =
                                std::atan2(cell.velocity.y, cell.velocity.x);
                            float grav_angle = std::atan2(grav.y, grav.x);
                            float angle_diff =
                                calculate_angle_diff(cell.velocity, grav);
                            if (std::abs(angle_diff) < std::numbers::pi / 2) {
                                // try go to left bottom and right bottom
                                glm::ivec2 lb = {
                                    std::round(std::cos(
                                        grav_angle - std::numbers::pi / 4
                                    )),
                                    std::round(std::sin(
                                        grav_angle - std::numbers::pi / 4
                                    ))
                                };
                                glm::ivec2 rb = {
                                    std::round(std::cos(
                                        grav_angle + std::numbers::pi / 4
                                    )),
                                    std::round(std::sin(
                                        grav_angle + std::numbers::pi / 4
                                    ))
                                };
                                glm::vec2 dirs[2];
                                bool lb_first = angle_diff < 0;
                                if (lb_first) {
                                    dirs[0] = lb;
                                    dirs[1] = rb;
                                } else {
                                    dirs[0] = rb;
                                    dirs[1] = lb;
                                }
                                for (auto&& vel : dirs) {
                                    int delta_x = vel.x;
                                    int delta_y = vel.y;
                                    if (delta_x == 0 && delta_y == 0) {
                                        continue;
                                    }
                                    int tx = final_x + delta_x;
                                    int ty = final_y + delta_y;
                                    if (!valid(tx, ty)) continue;
                                    auto& tcell = get_cell(tx, ty);
                                    if (!tcell) {
                                        std::swap(*ncell, tcell);
                                        ncell   = &tcell;
                                        final_x = tx;
                                        final_y = ty;
                                        moved   = true;
                                        break;
                                    }
                                }
                                if (!moved) {
                                    // try go to left and right
                                    glm::ivec2 left = {
                                        std::round(
                                            std::cos(
                                                grav_angle -
                                                std::numbers::pi / 2
                                            ) *
                                            liquid_spread_setting.spread_len
                                        ),
                                        std::round(
                                            std::sin(
                                                grav_angle -
                                                std::numbers::pi / 2
                                            ) *
                                            liquid_spread_setting.spread_len
                                        )
                                    };
                                    glm::ivec2 right = {
                                        std::round(
                                            std::cos(
                                                grav_angle +
                                                std::numbers::pi / 2
                                            ) *
                                            liquid_spread_setting.spread_len
                                        ),
                                        std::round(
                                            std::sin(
                                                grav_angle +
                                                std::numbers::pi / 2
                                            ) *
                                            liquid_spread_setting.spread_len
                                        )
                                    };
                                    glm::vec2 dirs[2];
                                    bool left_first = angle_diff > 0;
                                    if (left_first) {
                                        dirs[0] = left;
                                        dirs[1] = right;
                                    } else {
                                        dirs[0] = right;
                                        dirs[1] = left;
                                    }
                                    int tx_1   = final_x + dirs[0].x;
                                    int ty_1   = final_y + dirs[0].y;
                                    int tx_2   = final_x + dirs[1].x;
                                    int ty_2   = final_y + dirs[1].y;
                                    auto res_1 = raycast_to(
                                        final_x, final_y, tx_1, ty_1
                                    );
                                    auto res_2 = raycast_to(
                                        final_x, final_y, tx_2, ty_2
                                    );
                                    if (res_1.steps >= res_2.steps) {
                                        if (res_1.steps) {
                                            auto& tcell = get_cell(
                                                res_1.new_x, res_1.new_y
                                            );
                                            std::swap(*ncell, tcell);
                                            final_x = res_1.new_x;
                                            final_y = res_1.new_y;
                                            ncell   = &tcell;
                                            ncell->velocity +=
                                                glm::vec2(dirs[0]) *
                                                liquid_spread_setting.prefix /
                                                delta;
                                            moved = true;
                                        }
                                    } else {
                                        if (res_2.steps) {
                                            auto& tcell = get_cell(
                                                res_2.new_x, res_2.new_y
                                            );
                                            std::swap(*ncell, tcell);
                                            final_x = res_2.new_x;
                                            final_y = res_2.new_y;
                                            ncell   = &tcell;
                                            ncell->velocity +=
                                                glm::vec2(dirs[1]) *
                                                liquid_spread_setting.prefix /
                                                delta / 2.0f;
                                            moved = true;
                                        }
                                    }
                                }
                            }
                        }
                        // if (!blocking_freefall && !moved && !collided) {
                        //     ncell->freefall = false;
                        //     ncell->velocity = {0.0f, 0.0f};
                        // }
                    }
                    apply_viscosity(*this, final_x, final_y);
                    if (moved) {
                        ncell->not_move_count = 0;
                        touch(x_ - 1, y_);
                        touch(x_ + 1, y_);
                        touch(x_, y_ - 1);
                        touch(x_, y_ + 1);
                        touch(final_x - 1, final_y);
                        touch(final_x + 1, final_y);
                        touch(final_x, final_y - 1);
                        touch(final_x, final_y + 1);
                    } else {
                        cell.not_move_count++;
                        if (cell.not_move_count >=
                            not_moving_threshold(x_, y_) / 15) {
                            cell.not_move_count = 0;
                            cell.freefall       = false;
                            cell.velocity       = {0.0f, 0.0f};
                        }
                    }
                    mark_updated(final_x, final_y);
                }
            }
        }
    }
    m_chunk_map.count_time();
}
EPIX_API bool Simulation::init_update_state() {
    if (updating_state.is_updating) return false;
    updating_state.is_updating    = true;
    updating_state.updating_index = -1;
    auto& chunks_to_update        = updating_state.updating_chunks;
    chunks_to_update.clear();
    for (auto&& [pos, chunk] : m_chunk_map) {
        chunks_to_update.push_back(pos);
    }
    reset_updated();
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    static thread_local std::uniform_real_distribution<float> dis(-0.3f, 0.3f);
    update_state.next();
    bool xorder  = update_state.xorder;
    bool yorder  = update_state.yorder;
    bool x_outer = update_state.x_outer;
    if (update_state.random_state) {
        xorder  = dis(gen) > 0;
        yorder  = dis(gen) > 0;
        x_outer = dis(gen) > 0;
    }
    std::sort(
        chunks_to_update.begin(), chunks_to_update.end(),
        [&](auto& a, auto& b) {
            if (x_outer) {
                return (xorder ? a.x < b.x : a.x > b.x) &&
                       (yorder ? a.y < b.y : a.y > b.y);
            } else {
                return (yorder ? a.y < b.y : a.y > b.y) &&
                       (xorder ? a.x < b.x : a.x > b.x);
            }
        }
    );
    return true;
}
EPIX_API bool Simulation::deinit_update_state() {
    if (!updating_state.is_updating) return false;
    updating_state.is_updating = false;
    return true;
}
EPIX_API bool Simulation::next_chunk() {
    updating_state.updating_index++;
    while (updating_state.updating_index < updating_state.updating_chunks.size()
    ) {
        auto& pos =
            updating_state.updating_chunks[updating_state.updating_index];
        auto& chunk = m_chunk_map.get_chunk(pos.x, pos.y);
        if (!chunk.should_update()) {
            updating_state.updating_index++;
            continue;
        }
        auto& [xbounds, ybounds] = updating_state.bounds;
        xbounds                  = {
            update_state.xorder ? chunk.updating_area[0]
                                                 : chunk.updating_area[1],
            update_state.xorder ? chunk.updating_area[1] + 1
                                : chunk.updating_area[0] - 1,
            update_state.xorder ? 1 : -1
        };
        ybounds = {
            update_state.yorder ? chunk.updating_area[2]
                                : chunk.updating_area[3],
            update_state.yorder ? chunk.updating_area[3] + 1
                                : chunk.updating_area[2] - 1,
            update_state.yorder ? 1 : -1
        };
        updating_state.updating_x = std::get<0>(xbounds);
        updating_state.updating_y = std::get<0>(ybounds);
        return true;
    }
    return false;
}
EPIX_API bool Simulation::next_cell() {
    if (update_state.x_outer) {
        updating_state.updating_y += std::get<2>(updating_state.bounds.second);
        if (updating_state.updating_y ==
            std::get<1>(updating_state.bounds.second)) {
            updating_state.updating_y =
                std::get<0>(updating_state.bounds.second);
            updating_state.updating_x +=
                std::get<2>(updating_state.bounds.first);
            if (updating_state.updating_x ==
                std::get<1>(updating_state.bounds.first)) {
                return false;
            }
        }
    } else {
        updating_state.updating_x += std::get<2>(updating_state.bounds.first);
        if (updating_state.updating_x ==
            std::get<1>(updating_state.bounds.first)) {
            updating_state.updating_x =
                std::get<0>(updating_state.bounds.first);
            updating_state.updating_y +=
                std::get<2>(updating_state.bounds.second);
            if (updating_state.updating_y ==
                std::get<1>(updating_state.bounds.second)) {
                return false;
            }
        }
    }
    return true;
}
EPIX_API void Simulation::update_cell(float delta) {
    const auto& pos =
        updating_state.updating_chunks[updating_state.updating_index];
    auto& chunk  = m_chunk_map.get_chunk(pos.x, pos.y);
    const int x_ = pos.x * m_chunk_size + updating_state.updating_x;
    const int y_ = pos.y * m_chunk_size + updating_state.updating_y;
    int final_x  = x_;
    int final_y  = y_;
    if (chunk.is_updated(updating_state.updating_x, updating_state.updating_y))
        return;
    if (!contain_cell(x_, y_)) return;
    auto [cell, elem] = get(x_, y_);
    auto grav         = get_grav(x_, y_);
    float grav_len_s  = grav.x * grav.x + grav.y * grav.y;
    if (grav_len_s == 0) {
        chunk.time_threshold = std::numeric_limits<int>::max();
    } else {
        chunk.time_threshold =
            std::max((int)(8 * 10000 / grav_len_s), chunk.time_threshold);
    }
    if (elem.grav_type == Element::GravType::POWDER) {
        if (!cell.freefall) {
            float angle = std::atan2(grav.y, grav.x);
            // into a 8 direction
            angle = std::round(angle / (std::numbers::pi / 4)) *
                    (std::numbers::pi / 4);
            glm::ivec2 dir = {
                std::round(std::cos(angle)), std::round(std::sin(angle))
            };
            int below_x = x_ + dir.x;
            int below_y = y_ + dir.y;
            if (!valid(below_x, below_y)) return;
            auto& bcell = get_cell(below_x, below_y);
            if (bcell) {
                auto& belem = get_elem(below_x, below_y);
                if (belem.grav_type == Element::GravType::SOLID) {
                    return;
                }
                if (belem.grav_type == Element::GravType::POWDER &&
                    !bcell.freefall) {
                    return;
                }
            }
            cell.velocity = get_default_vel(x_, y_);
            cell.freefall = true;
        }
        cell.velocity += grav * delta;
        cell.velocity *= 0.99f;
        cell.inpos += cell.velocity * delta;
        int delta_x = std::round(cell.inpos.x);
        int delta_y = std::round(cell.inpos.y);
        if (delta_x == 0 && delta_y == 0) {
            return;
        }
        cell.inpos.x -= delta_x;
        cell.inpos.y -= delta_y;
        if (max_travel) {
            delta_x = std::clamp(delta_x, -max_travel->x, max_travel->y);
            delta_y = std::clamp(delta_y, -max_travel->x, max_travel->y);
        }
        int tx              = x_ + delta_x;
        int ty              = y_ + delta_y;
        bool moved          = false;
        auto raycast_result = raycast_to(x_, y_, tx, ty);
        auto ncell = &get_cell(raycast_result.new_x, raycast_result.new_y);
        if (raycast_result.steps) {
            std::swap(cell, *ncell);
            final_x = raycast_result.new_x;
            final_y = raycast_result.new_y;
            moved   = true;
        }
        if (raycast_result.hit) {
            auto [hit_x, hit_y]    = raycast_result.hit.value();
            bool blocking_freefall = false;
            bool collided          = false;
            if (valid(hit_x, hit_y)) {
                auto [tcell, telem] = get(hit_x, hit_y);
                if (telem.grav_type == Element::GravType::SOLID ||
                    telem.grav_type == Element::GravType::POWDER) {
                    collided = collide(
                        raycast_result.new_x, raycast_result.new_y, hit_x, hit_y
                    );
                    blocking_freefall = tcell.freefall;
                } else {
                    std::swap(*ncell, tcell);
                    final_x = hit_x;
                    final_y = hit_y;
                    ncell   = &tcell;
                    moved   = true;
                }
            }
            if (!moved) {
                if (powder_always_slide ||
                    (valid(hit_x, hit_y) && !get_cell(hit_x, hit_y).freefall)) {
                    // try go to left bottom and right bottom
                    float vel_angle =
                        std::atan2(cell.velocity.y, cell.velocity.x);
                    float grav_angle = std::atan2(grav.y, grav.x);
                    float angle_diff =
                        calculate_angle_diff(cell.velocity, grav);
                    if (std::abs(angle_diff) < std::numbers::pi / 2) {
                        glm::ivec2 lb = {
                            std::round(
                                std::cos(grav_angle - std::numbers::pi / 4)
                            ),
                            std::round(
                                std::sin(grav_angle - std::numbers::pi / 4)
                            )
                        };
                        glm::ivec2 rb = {
                            std::round(
                                std::cos(grav_angle + std::numbers::pi / 4)
                            ),
                            std::round(
                                std::sin(grav_angle + std::numbers::pi / 4)
                            )
                        };
                        glm::vec2 dirs[2];
                        static thread_local std::random_device rd;
                        static thread_local std::mt19937 gen(rd());
                        static thread_local std::uniform_real_distribution<
                            float>
                            dis(-0.3f, 0.3f);
                        bool lb_first = dis(gen) > 0;
                        if (lb_first) {
                            dirs[0] = lb;
                            dirs[1] = rb;
                        } else {
                            dirs[0] = rb;
                            dirs[1] = lb;
                        }
                        for (auto&& vel : dirs) {
                            int delta_x = vel.x;
                            int delta_y = vel.y;
                            if (delta_x == 0 && delta_y == 0) {
                                continue;
                            }
                            int tx = final_x + delta_x;
                            int ty = final_y + delta_y;
                            if (!valid(tx, ty)) continue;
                            auto& tcell = get_cell(tx, ty);
                            if (!tcell) {
                                std::swap(*ncell, tcell);
                                ncell   = &tcell;
                                final_x = tx;
                                final_y = ty;
                                moved   = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (!blocking_freefall && !moved) {
                ncell->freefall = false;
                ncell->velocity = {0.0f, 0.0f};
            }
        }
        if (moved) {
            ncell->not_move_count = 0;
            touch(x_ - 1, y_);
            touch(x_ + 1, y_);
            touch(x_, y_ - 1);
            touch(x_, y_ + 1);
            touch(final_x - 1, final_y);
            touch(final_x + 1, final_y);
            touch(final_x, final_y - 1);
            touch(final_x, final_y + 1);
        } else {
            cell.not_move_count++;
            if (cell.not_move_count >= not_moving_threshold(x_, y_)) {
                cell.not_move_count = 0;
                cell.freefall       = false;
                cell.velocity       = {0.0f, 0.0f};
            }
        }
        mark_updated(final_x, final_y);
    } else if (elem.grav_type == Element::GravType::LIQUID) {
        if (!cell.freefall) {
            float angle = std::atan2(grav.y, grav.x);
            // into a 8 direction
            angle = std::round(angle / (std::numbers::pi / 4)) *
                    (std::numbers::pi / 4);
            glm::ivec2 dir = {
                std::round(std::cos(angle)), std::round(std::sin(angle))
            };
            glm::ivec2 lb = {
                std::round(std::cos(angle - std::numbers::pi / 4)),
                std::round(std::sin(angle - std::numbers::pi / 4))
            };
            glm::ivec2 rb = {
                std::round(std::cos(angle + std::numbers::pi / 4)),
                std::round(std::sin(angle + std::numbers::pi / 4))
            };
            int below_x = x_ + dir.x;
            int below_y = y_ + dir.y;
            int lb_x    = x_ + lb.x;
            int lb_y    = y_ + lb.y;
            int rb_x    = x_ + rb.x;
            int rb_y    = y_ + rb.y;
            if (!valid(below_x, below_y) && !valid(lb_x, lb_y) &&
                !valid(rb_x, rb_y)) {
                return;
            }
            bool should_freefall = false;
            if (valid(below_x, below_y)) {
                auto& bcell             = get_cell(below_x, below_y);
                bool shouldnot_freefall = false;
                if (bcell) {
                    auto& belem = get_elem(below_x, below_y);
                    if (belem.grav_type == Element::GravType::SOLID) {
                        shouldnot_freefall = true;
                    }
                    if ((belem.grav_type == Element::GravType::POWDER &&
                         !bcell.freefall) ||
                        belem.grav_type == Element::GravType::LIQUID) {
                        shouldnot_freefall = true;
                    }
                }
                should_freefall |= !shouldnot_freefall;
            }
            if (valid(lb_x, lb_y)) {
                auto& lbcell            = get_cell(lb_x, lb_y);
                bool shouldnot_freefall = false;
                if (lbcell) {
                    auto& lbelem = get_elem(lb_x, lb_y);
                    if (lbelem.grav_type == Element::GravType::SOLID) {
                        shouldnot_freefall = true;
                    }
                    if ((lbelem.grav_type == Element::GravType::POWDER &&
                         !lbcell.freefall) ||
                        lbelem.grav_type == Element::GravType::LIQUID) {
                        shouldnot_freefall = true;
                    }
                }
                should_freefall |= !shouldnot_freefall;
            }
            if (valid(rb_x, rb_y)) {
                auto& rbcell            = get_cell(rb_x, rb_y);
                bool shouldnot_freefall = false;
                if (rbcell) {
                    auto& rbelem = get_elem(rb_x, rb_y);
                    if (rbelem.grav_type == Element::GravType::SOLID) {
                        shouldnot_freefall = true;
                    }
                    if ((rbelem.grav_type == Element::GravType::POWDER &&
                         !rbcell.freefall) ||
                        rbelem.grav_type == Element::GravType::LIQUID) {
                        shouldnot_freefall = true;
                    }
                }
                should_freefall |= !shouldnot_freefall;
            }
            if (!should_freefall) return;

            cell.velocity = get_default_vel(x_, y_);
            cell.freefall = true;
        }
        cell.velocity += grav * delta;
        cell.velocity *= 0.99f;
        cell.inpos += cell.velocity * delta;
        int delta_x = std::round(cell.inpos.x);
        int delta_y = std::round(cell.inpos.y);
        if (delta_x == 0 && delta_y == 0) {
            return;
        }
        cell.inpos.x -= delta_x;
        cell.inpos.y -= delta_y;
        if (max_travel) {
            delta_x = std::clamp(delta_x, -max_travel->x, max_travel->y);
            delta_y = std::clamp(delta_y, -max_travel->x, max_travel->y);
        }
        int tx              = x_ + delta_x;
        int ty              = y_ + delta_y;
        bool moved          = false;
        auto raycast_result = raycast_to(x_, y_, tx, ty);
        auto ncell = &get_cell(raycast_result.new_x, raycast_result.new_y);
        if (raycast_result.steps) {
            std::swap(cell, *ncell);
            final_x = raycast_result.new_x;
            final_y = raycast_result.new_y;
            moved   = true;
        }
        if (raycast_result.hit) {
            auto [hit_x, hit_y]    = raycast_result.hit.value();
            bool blocking_freefall = false;
            bool collided          = false;
            if (valid(hit_x, hit_y)) {
                auto [tcell, telem] = get(hit_x, hit_y);
                if (telem.grav_type == Element::GravType::SOLID ||
                    telem.grav_type == Element::GravType::POWDER ||
                    telem.grav_type == Element::GravType::LIQUID) {
                    collided = collide(
                        raycast_result.new_x, raycast_result.new_y, hit_x, hit_y
                    );
                } else {
                    std::swap(*ncell, tcell);
                    final_x = hit_x;
                    final_y = hit_y;
                    ncell   = &tcell;
                    moved   = true;
                }
            }
            if (!moved) {
                float vel_angle  = std::atan2(cell.velocity.y, cell.velocity.x);
                float grav_angle = std::atan2(grav.y, grav.x);
                float angle_diff = calculate_angle_diff(cell.velocity, grav);
                if (std::abs(angle_diff) < std::numbers::pi / 2) {
                    // try go to left bottom and right bottom
                    glm::ivec2 lb = {
                        std::round(std::cos(grav_angle - std::numbers::pi / 4)),
                        std::round(std::sin(grav_angle - std::numbers::pi / 4))
                    };
                    glm::ivec2 rb = {
                        std::round(std::cos(grav_angle + std::numbers::pi / 4)),
                        std::round(std::sin(grav_angle + std::numbers::pi / 4))
                    };
                    glm::vec2 dirs[2];
                    bool lb_first = angle_diff < 0;
                    if (lb_first) {
                        dirs[0] = lb;
                        dirs[1] = rb;
                    } else {
                        dirs[0] = rb;
                        dirs[1] = lb;
                    }
                    for (auto&& vel : dirs) {
                        int delta_x = vel.x;
                        int delta_y = vel.y;
                        if (delta_x == 0 && delta_y == 0) {
                            continue;
                        }
                        int tx = final_x + delta_x;
                        int ty = final_y + delta_y;
                        if (!valid(tx, ty)) continue;
                        auto& tcell = get_cell(tx, ty);
                        if (!tcell) {
                            std::swap(*ncell, tcell);
                            ncell   = &tcell;
                            final_x = tx;
                            final_y = ty;
                            moved   = true;
                            break;
                        }
                    }
                    if (!moved) {
                        // try go to left and right
                        glm::ivec2 left = {
                            std::round(
                                std::cos(grav_angle - std::numbers::pi / 2) *
                                liquid_spread_setting.spread_len
                            ),
                            std::round(
                                std::sin(grav_angle - std::numbers::pi / 2) *
                                liquid_spread_setting.spread_len
                            )
                        };
                        glm::ivec2 right = {
                            std::round(
                                std::cos(grav_angle + std::numbers::pi / 2) *
                                liquid_spread_setting.spread_len
                            ),
                            std::round(
                                std::sin(grav_angle + std::numbers::pi / 2) *
                                liquid_spread_setting.spread_len
                            )
                        };
                        glm::vec2 dirs[2];
                        bool left_first = angle_diff > 0;
                        if (left_first) {
                            dirs[0] = left;
                            dirs[1] = right;
                        } else {
                            dirs[0] = right;
                            dirs[1] = left;
                        }
                        int tx_1   = final_x + dirs[0].x;
                        int ty_1   = final_y + dirs[0].y;
                        int tx_2   = final_x + dirs[1].x;
                        int ty_2   = final_y + dirs[1].y;
                        auto res_1 = raycast_to(final_x, final_y, tx_1, ty_1);
                        auto res_2 = raycast_to(final_x, final_y, tx_2, ty_2);
                        if (res_1.steps >= res_2.steps) {
                            if (res_1.steps) {
                                auto& tcell =
                                    get_cell(res_1.new_x, res_1.new_y);
                                std::swap(*ncell, tcell);
                                final_x = res_1.new_x;
                                final_y = res_1.new_y;
                                ncell   = &tcell;
                                ncell->velocity +=
                                    glm::vec2(dirs[0]) *
                                    liquid_spread_setting.prefix / delta;
                                moved = true;
                            }
                        } else {
                            if (res_2.steps) {
                                auto& tcell =
                                    get_cell(res_2.new_x, res_2.new_y);
                                std::swap(*ncell, tcell);
                                final_x = res_2.new_x;
                                final_y = res_2.new_y;
                                ncell   = &tcell;
                                ncell->velocity +=
                                    glm::vec2(dirs[1]) *
                                    liquid_spread_setting.prefix / delta / 2.0f;
                                moved = true;
                            }
                        }
                    }
                }
            }
            // if (!blocking_freefall && !moved && !collided) {
            //     ncell->freefall = false;
            //     ncell->velocity = {0.0f, 0.0f};
            // }
        }
        apply_viscosity(*this, final_x, final_y);
        if (moved) {
            ncell->not_move_count = 0;
            touch(x_ - 1, y_);
            touch(x_ + 1, y_);
            touch(x_, y_ - 1);
            touch(x_, y_ + 1);
            touch(final_x - 1, final_y);
            touch(final_x + 1, final_y);
            touch(final_x, final_y - 1);
            touch(final_x, final_y + 1);
        } else {
            cell.not_move_count++;
            if (cell.not_move_count >= not_moving_threshold(x_, y_) / 15) {
                cell.not_move_count = 0;
                cell.freefall       = false;
                cell.velocity       = {0.0f, 0.0f};
            }
        }
        mark_updated(final_x, final_y);
    }
}
EPIX_API Simulation::RaycastResult Simulation::raycast_to(
    int x, int y, int tx, int ty
) {
    if (!valid(x, y)) {
        return RaycastResult{0, x, y, std::nullopt};
    }
    if (x == tx && y == ty) {
        return RaycastResult{0, x, y, std::nullopt};
    }
    int w          = tx - x;
    int h          = ty - y;
    int max        = std::max(std::abs(w), std::abs(h));
    float dx       = (float)w / max;
    float dy       = (float)h / max;
    int last_x     = x;
    int last_y     = y;
    int step_count = 0;
    for (int i = 1; i <= max; i++) {
        int new_x = x + std::round(dx * i);
        int new_y = y + std::round(dy * i);
        if (new_x == last_x && new_y == last_y) {
            continue;
        }
        if (!valid(new_x, new_y) || contain_cell(new_x, new_y)) {
            return RaycastResult{
                step_count, last_x, last_y, std::make_pair(new_x, new_y)
            };
        }
        step_count++;
        last_x = new_x;
        last_y = new_y;
    }
    return RaycastResult{step_count, last_x, last_y, std::nullopt};
}
EPIX_API bool Simulation::collide(int x, int y, int tx, int ty) {
    // if (!valid(x, y) || !valid(tx, ty)) return false;
    // if (!contain_cell(x, y) || !contain_cell(tx, ty)) return false;
    auto [cell, elem]   = get(x, y);
    auto [tcell, telem] = get(tx, ty);
    float dx            = (float)(tx - x) + tcell.inpos.x - cell.inpos.x;
    float dy            = (float)(ty - y) + tcell.inpos.y - cell.inpos.y;
    float dist          = glm::length(glm::vec2(dx, dy));
    float dv_x          = cell.velocity.x - tcell.velocity.x;
    float dv_y          = cell.velocity.y - tcell.velocity.y;
    float v_dot_d       = dv_x * dx + dv_y * dy;
    if (v_dot_d <= 0) return false;
    float m1 = elem.density;
    float m2 = telem.density;
    if (telem.grav_type == Element::GravType::SOLID) {
        m1 = 0;
    }
    if (telem.grav_type == Element::GravType::POWDER && !tcell.freefall) {
        m1 *= 0.5f;
    }
    if (elem.grav_type == Element::GravType::SOLID) {
        m2 = 0;
    }
    if (elem.grav_type == Element::GravType::LIQUID &&
        telem.grav_type == Element::GravType::SOLID) {
        dx = (float)(tx - x);
        dy = (float)(ty - y);
    }
    if (m1 == 0 && m2 == 0) return false;
    float restitution = std::max(elem.bouncing, telem.bouncing);
    float j  = -(1 + restitution) * v_dot_d / (m1 + m2);  // impulse scalar
    float jx = j * dx / dist;
    float jy = j * dy / dist;
    cell.velocity.x += jx * m2;
    cell.velocity.y += jy * m2;
    tcell.velocity.x -= jx * m1;
    tcell.velocity.y -= jy * m1;
    float friction = std::sqrt(elem.friction * telem.friction);
    float dot2     = (dv_x * dy - dv_y * dx) / dist;
    float jf       = dot2 / (m1 + m2);
    float jfabs    = std::min(friction * std::abs(j), std::fabs(jf));
    float jfx_mod  = dot2 > 0 ? dy / dist : -dy / dist;
    float jfy_mod  = dot2 > 0 ? -dx / dist : dx / dist;
    float jfx      = jfabs * jfx_mod * 2.0f / 3.0f;
    float jfy      = jfabs * jfy_mod * 2.0f / 3.0f;
    cell.velocity.x -= jfx * m2;
    cell.velocity.y -= jfy * m2;
    tcell.velocity.x += jfx * m1;
    tcell.velocity.y += jfy * m1;
    if (elem.grav_type == Element::GravType::LIQUID &&
        telem.grav_type == Element::GravType::LIQUID) {
        auto new_cell_vel  = cell.velocity * 0.55f + tcell.velocity * 0.45f;
        auto new_tcell_vel = cell.velocity * 0.45f + tcell.velocity * 0.55f;
        cell.velocity      = new_cell_vel;
        tcell.velocity     = new_tcell_vel;
    }
    return true;
}

EPIX_API void Simulation::touch(int x, int y) {
    if (!valid(x, y)) return;
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
    auto& chunk             = m_chunk_map.get_chunk(chunk_x, chunk_y);
    chunk.touch(cell_x, cell_y);
    auto& cell = chunk.get(cell_x, cell_y);
    if (!cell) return;
    auto& elem = get_elem(x, y);
    if (elem.grav_type == Element::GravType::SOLID) return;
    cell.freefall = true;
}