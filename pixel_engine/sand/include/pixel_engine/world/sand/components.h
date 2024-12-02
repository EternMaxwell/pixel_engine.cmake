#pragma once

#include <sparsepp/spp.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <mutex>

namespace pixel_engine::world::sand::components {
struct Element {
    std::string name;
    std::string description;
    glm::vec4 (*gen_color)();
    bool movable;
    float density;
};
struct CellDef {
    enum class DefIdentifier { Name, Id } identifier;
    union {
        std::string elem_name;
        int elem_id;
    };
    glm::vec4 color;

    CellDef(const std::string& name, const glm::vec4& color)
        : identifier(DefIdentifier::Name), elem_name(name), color(color) {}
    CellDef(int id, const glm::vec4& color)
        : identifier(DefIdentifier::Id), elem_id(id), color(color) {}
    ~CellDef() {
        if (identifier == DefIdentifier::Name) {
            elem_name.~basic_string();
        }
    }
};
struct Cell {
    int elem_id     = -1;
    glm::vec4 color = glm::vec4(0.0f);
};
struct ElemRegistry {
   private:
    std::mutex mutex;
    spp::sparse_hash_map<std::string, uint32_t> elemId_map;
    std::vector<Element> elements;

   public:
    ElemRegistry();
    ElemRegistry(const ElemRegistry& other);
    ElemRegistry(ElemRegistry&& other);
    ElemRegistry& operator=(const ElemRegistry& other);
    ElemRegistry& operator=(ElemRegistry&& other);

    int register_elem(const std::string& name, const Element& elem);
    int elem_id(const std::string& name);
    const Element& get_elem(const std::string& name);
    const Element& get_elem(int id) const;
    const Element& operator[](int id) const;
    void add_equiv(const std::string& name, const std::string& equiv);
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
        Cell& create(
            int x, int y, const CellDef& def, ElemRegistry& m_registry
        ) {
            Cell& cell   = get(x, y);
            cell.elem_id = def.identifier == CellDef::DefIdentifier::Name
                               ? m_registry.elem_id(def.elem_name)
                               : def.elem_id;
            cell.color   = def.color;
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
                return std::hash<int>()(k.x) ^ std::hash<int>()(k.y);
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
    };

   private:
    ElemRegistry m_registry;
    const int m_chunk_size;
    ChunkMap m_chunk_map;

   public:
    Simulation(const ElemRegistry& registry, int chunk_size = 64)
        : m_registry(registry), m_chunk_size(chunk_size) {}
    Simulation(ElemRegistry&& registry, int chunk_size = 64)
        : m_registry(std::move(registry)), m_chunk_size(chunk_size) {}

    ElemRegistry& registry() { return m_registry; }

    void load_chunk(int x, int y, const Chunk& chunk) {
        m_chunk_map.load_chunk(x, y, chunk);
    }
    void load_chunk(int x, int y, Chunk&& chunk) {
        m_chunk_map.load_chunk(x, y, std::move(chunk));
    }
    void load_chunk(int x, int y) { m_chunk_map.load_chunk(x, y); }
    bool contains(int x, int y) const {
        return m_chunk_map.contains(x / m_chunk_size, y / m_chunk_size);
    }
    std::tuple<Cell&, const Element&> get(int x, int y) {
        assert(contains(x, y));
        int chunk_x = x / m_chunk_size;
        int chunk_y = y / m_chunk_size;
        int cell_x  = x % m_chunk_size;
        int cell_y  = y % m_chunk_size;
        Cell& cell =
            m_chunk_map.get_chunk(chunk_x, chunk_y).get(cell_x, cell_y);
        const Element& elem = m_registry.get_elem(cell.elem_id);
        return {cell, elem};
    }
    Cell& create(int x, int y, const CellDef& def) {
        assert(contains(x, y));
        int chunk_x = x / m_chunk_size;
        int chunk_y = y / m_chunk_size;
        int cell_x  = x % m_chunk_size;
        int cell_y  = y % m_chunk_size;
        return m_chunk_map.get_chunk(chunk_x, chunk_y)
            .create(cell_x, cell_y, def, m_registry);
    }
    void update() {
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
        for (auto& pos : chunks_to_update) {
            auto& chunk = m_chunk_map.get_chunk(pos.x, pos.y);
            for (int y = 0; y < chunk.height; y++) {
                for (int x = 0; x < chunk.width; x++) {
                    Cell& cell = chunk.get(x, y);
                    if (cell.elem_id < 0) {
                        continue;
                    }
                    const Element& elem = m_registry.get_elem(cell.elem_id);
                    if (!elem.movable) {
                        continue;
                    }
                }
            }
        }
    }
};
}  // namespace pixel_engine::world::sand::components