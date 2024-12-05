#pragma once

#include <sparsepp/spp.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <mutex>
#include <random>

#include "epix/common.h"

namespace epix::world::sand::components {
struct Element {
    std::string name;
    std::string description;
    /**
     * @brief GravType is an enum class that defines how the element reacts to
     * gravity. Powder won't spread out as much as liquid, and gas will rise to
     * the top.
     */
    enum class GravType {
        POWDER,
        LIQUID,
        SOLID,
        GAS,
    } grav_type = GravType::SOLID;
    std::function<glm::vec4()> color_gen;
    float density;
};
struct CellDef {
    enum class DefIdentifier { Name, Id } identifier;
    union {
        std::string elem_name;
        int elem_id;
    };

    CellDef(const CellDef& other) : identifier(other.identifier) {
        if (identifier == DefIdentifier::Name) {
            new (&elem_name) std::string(other.elem_name);
        } else {
            elem_id = other.elem_id;
        }
    }
    CellDef(CellDef&& other) : identifier(other.identifier) {
        if (identifier == DefIdentifier::Name) {
            new (&elem_name) std::string(std::move(other.elem_name));
        } else {
            elem_id = other.elem_id;
        }
    }
    CellDef& operator=(const CellDef& other) {
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
    CellDef& operator=(CellDef&& other) {
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
    CellDef(const std::string& name)
        : identifier(DefIdentifier::Name), elem_name(name) {}
    CellDef(int id) : identifier(DefIdentifier::Id), elem_id(id) {}
    ~CellDef() {
        if (identifier == DefIdentifier::Name) {
            elem_name.~basic_string();
        }
    }
};
struct Cell {
    int elem_id     = -1;
    glm::vec4 color = glm::vec4(0.0f);

    bool valid() const { return elem_id >= 0; }
    operator bool() const { return valid(); }
    bool operator!() const { return !valid(); }
};
struct ElemRegistry {
   private:
    std::mutex mutex;
    spp::sparse_hash_map<std::string, uint32_t> elemId_map;
    std::vector<Element> elements;

   public:
    EPIX_API ElemRegistry();
    EPIX_API ElemRegistry(const ElemRegistry& other);
    EPIX_API ElemRegistry(ElemRegistry&& other);
    EPIX_API ElemRegistry& operator=(const ElemRegistry& other);
    EPIX_API ElemRegistry& operator=(ElemRegistry&& other);

    EPIX_API int register_elem(const std::string& name, const Element& elem);
    EPIX_API int elem_id(const std::string& name);
    EPIX_API const Element& get_elem(const std::string& name);
    EPIX_API const Element& get_elem(int id) const;
    EPIX_API const Element& operator[](int id) const;
    EPIX_API void add_equiv(const std::string& name, const std::string& equiv);
};
struct Simulation {
    struct Chunk {
        std::vector<Cell> cells;
        const int width;
        const int height;

        Chunk(int width, int height)
            : cells(width * height, Cell{}), width(width), height(height) {}
        Chunk(const Chunk& other)
            : cells(other.cells), width(other.width), height(other.height) {}
        Chunk(Chunk&& other)
            : cells(std::move(other.cells)),
              width(other.width),
              height(other.height) {}
        Chunk& operator=(const Chunk& other) {
            assert(width == other.width && height == other.height);
            cells = other.cells;
            return *this;
        }
        Chunk& operator=(Chunk&& other) {
            assert(width == other.width && height == other.height);
            cells = std::move(other.cells);
            return *this;
        }
        Cell& get(int x, int y) { return cells[y * width + x]; }
        const Cell& get(int x, int y) const { return cells[y * width + x]; }
        Cell& create(
            int x, int y, const CellDef& def, ElemRegistry& m_registry
        ) {
            Cell& cell   = get(x, y);
            cell.elem_id = def.identifier == CellDef::DefIdentifier::Name
                               ? m_registry.elem_id(def.elem_name)
                               : def.elem_id;
            if (cell.elem_id < 0) {
                return cell;
            }
            cell.color = m_registry.get_elem(cell.elem_id).color_gen();
            return cell;
        }
        operator bool() const { return width && height && !cells.empty(); }
        bool operator!() const { return !width || !height || cells.empty(); }

        // size and contains function for find_outline algorithm in
        // physics2d::utils to work.
        /**
         * @brief Get the size of the chunk
         *
         * @return glm::ivec2
         */
        glm::ivec2 size() const { return glm::ivec2(width, height); }
        /**
         * @brief Check if the chunk contains a cell at the given coordinates
         *
         * @param x x-coordinate
         * @param y y-coordinate
         * @return true if the cell is within the chunk
         * @return false if the cell is outside the chunk
         */
        bool contains(int x, int y) const {
            return x >= 0 && x < width && y >= 0 && y < height &&
                   cells[y * width + x].elem_id >= 0;
        }
    };

    struct ChunkMap {
        struct ivec2_hash {
            std::size_t operator()(const glm::ivec2& k) const {
                return std::hash<uint64_t>()(
                    (static_cast<uint64_t>(k.x) << 32) | k.y
                );
            }
        };

        spp::sparse_hash_map<glm::ivec2, Chunk, ivec2_hash> chunks;

        void load_chunk(int x, int y, const Chunk& chunk) {
            if (chunks.find(glm::ivec2(x, y)) != chunks.end()) {
                chunks.at(glm::ivec2(x, y)) = chunk;
            } else {
                chunks.emplace(glm::ivec2(x, y), chunk);
            }
        }
        void load_chunk(int x, int y, Chunk&& chunk) {
            if (chunks.find(glm::ivec2(x, y)) != chunks.end()) {
                chunks.at(glm::ivec2(x, y)) = std::move(chunk);
            } else {
                chunks.emplace(glm::ivec2(x, y), std::move(chunk));
            }
        }
        void load_chunk(int x, int y) {
            if (chunks.find(glm::ivec2(x, y)) == chunks.end()) {
                chunks.emplace(glm::ivec2(x, y), Chunk(64, 64));
            }
        }
        bool contains(int x, int y) const {
            return chunks.find(glm::ivec2(x, y)) != chunks.end();
        }
        Chunk& get_chunk(int x, int y) { return chunks.at(glm::ivec2(x, y)); }
        const Chunk& get_chunk(int x, int y) const {
            return chunks.at(glm::ivec2(x, y));
        }

        auto begin() { return chunks.begin(); }
        const auto begin() const { return chunks.begin(); }
        auto end() { return chunks.end(); }
        const auto end() const { return chunks.end(); }
    };

   private:
    ElemRegistry m_registry;
    const int m_chunk_size;
    ChunkMap m_chunk_map;

    Cell& create_def(int x, int y, const CellDef& def) {
        assert(valid(x, y));
        int chunk_x = x / m_chunk_size - (x < 0);
        int chunk_y = y / m_chunk_size - (y < 0);
        int cell_x  = x % m_chunk_size + m_chunk_size * (x < 0);
        int cell_y  = y % m_chunk_size + m_chunk_size * (y < 0);
        return m_chunk_map.get_chunk(chunk_x, chunk_y)
            .create(cell_x, cell_y, def, m_registry);
    }

   public:
    Simulation(const ElemRegistry& registry, int chunk_size = 64)
        : m_registry(registry), m_chunk_size(chunk_size) {}
    Simulation(ElemRegistry&& registry, int chunk_size = 64)
        : m_registry(std::move(registry)), m_chunk_size(chunk_size) {}

    int chunk_size() const { return m_chunk_size; }
    ElemRegistry& registry() { return m_registry; }
    const ElemRegistry& registry() const { return m_registry; }
    ChunkMap& chunk_map() { return m_chunk_map; }
    const ChunkMap& chunk_map() const { return m_chunk_map; }
    void load_chunk(int x, int y, const Chunk& chunk) {
        m_chunk_map.load_chunk(x, y, chunk);
    }
    void load_chunk(int x, int y, Chunk&& chunk) {
        m_chunk_map.load_chunk(x, y, std::move(chunk));
    }
    void load_chunk(int x, int y) { m_chunk_map.load_chunk(x, y); }
    std::pair<int, int> to_chunk_pos(int x, int y) const {
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
    std::pair<int, int> to_in_chunk_pos(int x, int y) const {
        auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
        return {x - chunk_x * m_chunk_size, y - chunk_y * m_chunk_size};
    }
    bool contain_cell(int x, int y) const {
        auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
        auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
        return m_chunk_map.contains(chunk_x, chunk_y) &&
               m_chunk_map.get_chunk(chunk_x, chunk_y).contains(cell_x, cell_y);
    }
    bool valid(int x, int y) const {
        auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
        return m_chunk_map.contains(chunk_x, chunk_y);
    }
    std::tuple<Cell&, const Element&> get(int x, int y) {
        assert(contain_cell(x, y));
        auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
        auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
        Cell& cell =
            m_chunk_map.get_chunk(chunk_x, chunk_y).get(cell_x, cell_y);
        const Element& elem = m_registry.get_elem(cell.elem_id);
        return {cell, elem};
    }
    Cell& get_cell(int x, int y) {
        assert(valid(x, y));
        auto [chunk_x, chunk_y] = to_chunk_pos(x, y);
        auto [cell_x, cell_y]   = to_in_chunk_pos(x, y);
        return m_chunk_map.get_chunk(chunk_x, chunk_y).get(cell_x, cell_y);
    }
    const Element& get_elem(int x, int y) {
        assert(contain_cell(x, y));
        auto id = get_cell(x, y).elem_id;
        assert(id >= 0);
        return m_registry.get_elem(id);
    }
    template <typename... Args>
    Cell& create(int x, int y, Args&&... args) {
        return create_def(x, y, CellDef(std::forward<Args>(args)...));
    }
    void update(int steps = 1) {
        std::vector<glm::ivec2> chunks_to_update;
        for (auto& [pos, chunk] : m_chunk_map.chunks) {
            chunks_to_update.push_back(pos);
        }
        std::sort(
            chunks_to_update.begin(), chunks_to_update.end(),
            [](const glm::ivec2& a, const glm::ivec2& b) {
                return a.y < b.y || (a.y == b.y && a.x < b.x);
            }
        );
        for (int i = 0; i < steps; i++)
            for (auto& pos : chunks_to_update) {
                auto& chunk = m_chunk_map.get_chunk(pos.x, pos.y);
                for (int y = 0; y < chunk.height; y++) {
                    for (int x = 0; x < chunk.width; x++) {
                        Cell& cell = chunk.get(x, y);
                        if (!cell) continue;
                        static std::random_device rd;
                        static std::mt19937 gen(rd());
                        static std::uniform_int_distribution<int> dis(0, 1);
                        const Element& elem = m_registry.get_elem(cell.elem_id);
                        auto cx             = pos.x * m_chunk_size + x;
                        auto cy             = pos.y * m_chunk_size + y;
                        if ((x == 0 || x == chunk.width - 1 || y == 0 ||
                             y == chunk.height - 1) &&
                            !valid(cx, cy)) {
                            cell.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                            continue;
                        } else if (cell.color ==
                                   glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)) {
                            cell.color = elem.color_gen(
                            );  // reset color to element color
                        }
                        if (elem.grav_type == Element::GravType::POWDER) {
                            int dir = 2 * dis(gen) - 1;
                            // try go down
                            if (valid(cx, cy - 1) &&
                                (!get_cell(cx, cy - 1) ||
                                 get_elem(cx, cy - 1).grav_type ==
                                     Element::GravType::LIQUID)) {
                                std::swap(cell, get_cell(cx, cy - 1));
                            } else if (valid(cx - dir, cy - 1) &&
                                       (!get_cell(cx - dir, cy - 1) ||
                                        get_elem(cx - dir, cy - 1).grav_type ==
                                            Element::GravType::LIQUID)) {
                                std::swap(cell, get_cell(cx - dir, cy - 1));
                            } else if (valid(cx + dir, cy - 1) &&
                                       (!get_cell(cx + dir, cy - 1) ||
                                        get_elem(cx + dir, cy - 1).grav_type ==
                                            Element::GravType::LIQUID)) {
                                std::swap(cell, get_cell(cx + dir, cy - 1));
                            }
                        } else if (elem.grav_type ==
                                   Element::GravType::LIQUID) {
                            int dir = 2 * dis(gen) - 1;
                            // try go down
                            if (valid(cx, cy - 1) && !get_cell(cx, cy - 1)) {
                                std::swap(cell, get_cell(cx, cy - 1));
                            } else if (valid(cx - dir, cy - 1) &&
                                       !get_cell(cx - dir, cy - 1)) {
                                std::swap(cell, get_cell(cx - dir, cy - 1));
                            } else if (valid(cx + dir, cy - 1) &&
                                       !get_cell(cx + dir, cy - 1)) {
                                std::swap(cell, get_cell(cx + dir, cy - 1));
                            } else if (valid(cx - dir, cy) &&
                                       !get_cell(cx - dir, cy)) {
                                std::swap(cell, get_cell(cx - dir, cy));
                            } else if (valid(cx + dir, cy) &&
                                       !get_cell(cx + dir, cy)) {
                                std::swap(cell, get_cell(cx + dir, cy));
                            }
                        }
                    }
                }
            }
    }
};
constexpr int s = (-1) % 64;
}  // namespace epix::world::sand::components