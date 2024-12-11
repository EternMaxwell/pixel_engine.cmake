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

EPIX_API Cell& Simulation::create_def(int x, int y, const CellDef& def) {
    assert(valid(x, y));
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
    return m_chunk_map.get_chunk(chunk_x, chunk_y)
        .create(cell_x, cell_y, def, m_registry);
}
EPIX_API Simulation::Simulation(const ElemRegistry& registry, int chunk_size)
    : m_registry(registry), m_chunk_size(chunk_size), m_chunk_map{chunk_size} {}
EPIX_API Simulation::Simulation(ElemRegistry&& registry, int chunk_size)
    : m_registry(std::move(registry)),
      m_chunk_size(chunk_size),
      m_chunk_map{chunk_size} {}

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
EPIX_API Cell& Simulation::get_cell(int x, int y) {
    assert(valid(x, y));
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
    return m_chunk_map.get_chunk(chunk_x, chunk_y).get(cell_x, cell_y);
}
EPIX_API const Element& Simulation::get_elem(int x, int y) {
    assert(contain_cell(x, y));
    auto id = get_cell(x, y).elem_id;
    assert(id >= 0);
    return m_registry.get_elem(id);
}
EPIX_API void Simulation::remove(int x, int y) {
    assert(valid(x, y));
    auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
    auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
    m_chunk_map.get_chunk(chunk_x, chunk_y).remove(cell_x, cell_y);
}
EPIX_API std::pair<float, float> Simulation::get_grav(int x, int y) {
    assert(valid(x, y));
    return {0.f, -98.0f};
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
    bool yorder = dis(gen) > 0;
    bool xorder = dis(gen) > 0;
    std::sort(
        chunks_to_update.begin(), chunks_to_update.end(),
        [&](auto& a, auto& b) {
            return a.y < b.y && (xorder ? a.x < b.x : a.x > b.x);
        }
    );
    for (auto& pos : chunks_to_update) {
        auto& chunk = m_chunk_map.get_chunk(pos.x, pos.y);
        for (int y = yorder ? 0 : chunk.height - 1;
             yorder ? y < chunk.height : y >= 0; y += yorder ? 1 : -1) {
            for (int x = xorder ? 0 : chunk.width - 1;
                 xorder ? x < chunk.width : x >= 0; x += xorder ? 1 : -1) {
                int x_      = pos.x * m_chunk_size + x;
                int y_      = pos.y * m_chunk_size + y;
                int final_x = x_;
                int final_y = y_;
                if (chunk.is_updated(x, y)) continue;
                if (!contain_cell(x_, y_)) continue;
                auto [cell, elem] = get(x_, y_);
                if (elem.grav_type == Element::GravType::POWDER) {
                    auto [grav_x, grav_y] = get_grav(x_, y_);
                    if (!cell.freefall) {
                        float angle = std::atan2(grav_y, grav_x);
                        // into a 8 direction
                        angle = std::round(angle / (std::numbers::pi / 4)) *
                                (std::numbers::pi / 4);
                        glm::ivec2 dir = {std::cos(angle), std::sin(angle)};
                        int below_x    = x_ + dir.x;
                        int below_y    = y_ + dir.y;
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
                        cell.velocity = default_vel;
                        cell.freefall = true;
                    }
                    cell.velocity.x += grav_x * delta;
                    cell.velocity.y += grav_y * delta;
                    cell.velocity *= 0.99f;
                    cell.inpos += cell.velocity * delta;
                    int delta_x =
                        cell.inpos.x + (cell.inpos.x > 0 ? 0.5f : -0.5f);
                    int delta_y =
                        cell.inpos.y + (cell.inpos.y > 0 ? 0.5f : -0.5f);
                    if (delta_x == 0 && delta_y == 0) {
                        continue;
                    }
                    cell.inpos.x -= delta_x;
                    cell.inpos.y -= delta_y;
                    int tx              = x_ + delta_x;
                    int ty              = y_ + delta_y;
                    bool moved          = false;
                    auto raycast_result = raycast_to(x_, y_, tx, ty);
                    auto ncell =
                        &get_cell(raycast_result.new_x, raycast_result.new_y);
                    if (raycast_result.moved) {
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
                                float grav_angle = std::atan2(grav_y, grav_x);
                                if (std::abs(vel_angle - grav_angle) <
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
                        touch(x_ - 1, y_);
                        touch(x_ + 1, y_);
                        touch(x_, y_ - 1);
                        touch(x_, y_ + 1);
                        touch(final_x - 1, final_y);
                        touch(final_x + 1, final_y);
                        touch(final_x, final_y - 1);
                        touch(final_x, final_y + 1);
                    }
                    mark_updated(final_x, final_y);
                }
            }
        }
    }
}
EPIX_API Simulation::RaycastResult Simulation::raycast_to(
    int x, int y, int tx, int ty
) {
    if (!valid(x, y)) {
        return RaycastResult{false, x, y, std::nullopt};
    }
    if (x == tx && y == ty) {
        return RaycastResult{false, x, y, std::nullopt};
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
                step_count > 0, last_x, last_y, std::make_pair(new_x, new_y)
            };
        }
        step_count++;
        last_x = new_x;
        last_y = new_y;
    }
    return RaycastResult{true, last_x, last_y, std::nullopt};
}
EPIX_API bool Simulation::collide(int x, int y, int tx, int ty) {
    if (!valid(x, y) || !valid(tx, ty)) return false;
    if (!contain_cell(x, y) || !contain_cell(tx, ty)) return false;
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
        m1 = 0;
    }
    if (elem.grav_type == Element::GravType::SOLID) {
        m2 = 0;
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
    return true;
}

EPIX_API void Simulation::touch(int x, int y) {
    if (!valid(x, y)) return;
    auto& cell = get_cell(x, y);
    if (!cell) return;
    auto& elem = get_elem(x, y);
    if (elem.grav_type == Element::GravType::SOLID) return;
    cell.freefall = true;
}