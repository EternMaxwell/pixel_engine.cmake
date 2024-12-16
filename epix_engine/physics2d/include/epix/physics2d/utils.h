#pragma once

#include <epix/common.h>

#include <earcut.hpp>
#include <glm/glm.hpp>
#include <stack>
#include <vector>

namespace mapbox {
namespace util {
template <>
struct nth<0, glm::vec2> {
    EPIX_API static float get(const glm::dvec2& t) { return t.x; }
};

template <>
struct nth<1, glm::vec2> {
    EPIX_API static float get(const glm::dvec2& t) { return t.y; }
};

template <>
struct nth<0, glm::ivec2> {
    EPIX_API static int get(const glm::dvec2& t) { return t.x; }
};
template <>
struct nth<1, glm::ivec2> {
    EPIX_API static int get(const glm::dvec2& t) { return t.y; }
};
}  // namespace util
}  // namespace mapbox

namespace epix::physics2d::utils {
template <typename T>
concept HasOutline = requires(T t) {
    { t.size() } -> std::same_as<glm::ivec2>;
    { t.contains(0i32, 0i32) } -> std::same_as<bool>;
};

struct binary_grid {
    std::vector<uint32_t> pixels;
    const int width, height;
    const int column;

    EPIX_API binary_grid(int width, int height);
    template <HasOutline T>
    binary_grid(const T& pixelbin)
        : width(pixelbin.size().x),
          height(pixelbin.size().y),
          column((width >> 5) + (width & 31 ? 1 : 0)) {
        pixels.resize(column * height);
        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                set(i, j, pixelbin.contains(i, j));
            }
        }
    }
    EPIX_API void set(int x, int y, bool value);
    EPIX_API bool contains(int x, int y) const;
    EPIX_API glm::ivec2 size() const;
    EPIX_API binary_grid& operator&=(const binary_grid& rhs);
    EPIX_API binary_grid& operator|=(const binary_grid& rhs);
    EPIX_API binary_grid& operator^=(const binary_grid& rhs);
    EPIX_API binary_grid sub(int x, int y, int w, int h) const;
};

template <typename T>
concept GridCombinable = requires(T t) {
    { t& binary_grid(1, 1) } -> std::same_as<T>;
};

template <typename T>
concept CreateSub = requires(T t) {
    { t.sub(0i32, 0i32, 1i32, 1i32) } -> std::same_as<T>;
};

EPIX_API binary_grid operator&(const binary_grid& lhs, const binary_grid& rhs);
EPIX_API binary_grid operator|(const binary_grid& lhs, const binary_grid& rhs);
EPIX_API binary_grid operator^(const binary_grid& lhs, const binary_grid& rhs);
EPIX_API binary_grid operator~(const binary_grid& lhs);

/**
 * @brief Get the outland of a binary grid
 *
 * @param grid The binary grid
 * @param include_diagonal Whether to include diagonal pixels connected to the
 * outland as part of the outland
 *
 * @return binary_grid The outland of the binary grid
 */
template <HasOutline T>
binary_grid get_outland(const T& grid, bool include_diagonal = false) {
    binary_grid outland(grid.width, grid.height);
    // use dimension first search
    std::stack<std::pair<int, int>> stack;
    for (int i = 0; i < grid.width; i++) {
        if (!grid.contains(i, 0)) stack.push({i, 0});
        if (!grid.contains(i, grid.height - 1))
            stack.push({i, grid.height - 1});
    }
    for (int i = 0; i < grid.height; i++) {
        if (!grid.contains(0, i)) stack.push({0, i});
        if (!grid.contains(grid.width - 1, i)) stack.push({grid.width - 1, i});
    }
    while (!stack.empty()) {
        auto [x, y] = stack.top();
        stack.pop();
        if (outland.contains(x, y)) continue;
        outland.set(x, y, true);
        if (x > 0 && !grid.contains(x - 1, y) && !outland.contains(x - 1, y))
            stack.push({x - 1, y});
        if (x < grid.width - 1 && !grid.contains(x + 1, y) &&
            !outland.contains(x + 1, y))
            stack.push({x + 1, y});
        if (y > 0 && !grid.contains(x, y - 1) && !outland.contains(x, y - 1))
            stack.push({x, y - 1});
        if (y < grid.height - 1 && !grid.contains(x, y + 1) &&
            !outland.contains(x, y + 1))
            stack.push({x, y + 1});
        if (include_diagonal) {
            if (x > 0 && y > 0 && !grid.contains(x - 1, y - 1) &&
                !outland.contains(x - 1, y - 1))
                stack.push({x - 1, y - 1});
            if (x < grid.width - 1 && y > 0 && !grid.contains(x + 1, y - 1) &&
                !outland.contains(x + 1, y - 1))
                stack.push({x + 1, y - 1});
            if (x > 0 && y < grid.height - 1 && !grid.contains(x - 1, y + 1) &&
                !outland.contains(x - 1, y + 1))
                stack.push({x - 1, y + 1});
            if (x < grid.width - 1 && y < grid.height - 1 &&
                !grid.contains(x + 1, y + 1) && !outland.contains(x + 1, y + 1))
                stack.push({x + 1, y + 1});
        }
    }
    return outland;
}

/**
 * @brief Shrink a binary grid to the smallest size that contains all the
 * pixels
 *
 * @tparam T The type of the binary grid, must have a `size() -> glm::ivec2`
 * @param grid The binary grid
 * @param offset The offset of the new grid from the original grid
 *
 * @return binary_grid The shrunken grid
 */
template <typename T>
    requires CreateSub<T> && HasOutline<T>
T shrink(const T& grid, glm::ivec2* offset = nullptr) {
    auto size  = grid.size();
    auto width = size.x, height = size.y;
    glm::ivec2 min(width, height), max(-1, -1);
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            if (grid.contains(i, j)) {
                min.x = std::min(min.x, i);
                min.y = std::min(min.y, j);
                max.x = std::max(max.x, i);
                max.y = std::max(max.y, j);
            }
        }
    }
    if (offset) *offset = min;
    return grid.sub(min.x, min.y, max.x - min.x + 1, max.y - min.y + 1);
}

/**
 * @brief Split a binary grid into multiple binary grids if there are multiple
 *
 * @param grid The binary grid
 * @param include_diagonal Whether to include diagonal pixels connected to a
 * subset as part of the subset
 *
 * @return `std::vector<binary_grid>` The splitted binary grids
 */
template <typename T>
    requires GridCombinable<T>
std::vector<T> split_if_multiple(
    const T& grid_t, bool include_diagonal = false
) {
    binary_grid grid(grid_t);
    std::vector<binary_grid> result_grid;
    binary_grid visited = get_outland(grid);
    while (true) {
        glm::ivec2 current(-1, -1);
        for (int i = 0; i < visited.size().x; i++) {
            for (int j = 0; j < visited.size().y; j++) {
                if (!visited.contains(i, j) && grid.contains(i, j)) {
                    current = {i, j};
                    break;
                }
            }
            if (current.x != -1) break;
        }
        if (current.x == -1) break;
        result_grid.push_back(binary_grid(grid.width, grid.height));
        auto& current_grid = result_grid.back();
        std::stack<std::pair<int, int>> stack;
        stack.push({current.x, current.y});
        while (!stack.empty()) {
            auto [x, y] = stack.top();
            stack.pop();
            if (current_grid.contains(x, y)) continue;
            current_grid.set(x, y, true);
            visited.set(x, y, true);
            if (x > 0 && !visited.contains(x - 1, y) && grid.contains(x - 1, y))
                stack.push({x - 1, y});
            if (x < grid.width - 1 && !visited.contains(x + 1, y) &&
                grid.contains(x + 1, y))
                stack.push({x + 1, y});
            if (y > 0 && !visited.contains(x, y - 1) && grid.contains(x, y - 1))
                stack.push({x, y - 1});
            if (y < grid.height - 1 && !visited.contains(x, y + 1) &&
                grid.contains(x, y + 1))
                stack.push({x, y + 1});
            if (include_diagonal) {
                if (x > 0 && y > 0 && !visited.contains(x - 1, y - 1) &&
                    grid.contains(x - 1, y - 1))
                    stack.push({x - 1, y - 1});
                if (x < grid.width - 1 && y > 0 &&
                    !visited.contains(x + 1, y - 1) &&
                    grid.contains(x + 1, y - 1))
                    stack.push({x + 1, y - 1});
                if (x > 0 && y < grid.height - 1 &&
                    !visited.contains(x - 1, y + 1) &&
                    grid.contains(x - 1, y + 1))
                    stack.push({x - 1, y + 1});
                if (x < grid.width - 1 && y < grid.height - 1 &&
                    !visited.contains(x + 1, y + 1) &&
                    grid.contains(x + 1, y + 1))
                    stack.push({x + 1, y + 1});
            }
        }
    }
    std::vector<T> result;
    for (auto& grid : result_grid) {
        result.emplace_back(grid_t & grid);
    }
    return result;
}

/**
 * @brief Find the outline of a binary grid
 *
 * @tparam T The type of the binary grid, must have a `size() -> glm::ivec2`
 * method and a `contains(int, int) -> bool` method
 *
 * @param pixelbin The binary grid
 * @param include_diagonal Whether to include diagonal pixels connected
 * as part of the target obj
 *
 * @return The outline of the binary grid, in cw order
 */
template <HasOutline T>
std::vector<glm::ivec2> find_outline(
    const T& pixelbin, bool include_diagonal = false
) {
    std::vector<glm::ivec2> outline;
    std::vector<glm::ivec2> move    = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};
    std::vector<glm::ivec2> offsets = {
        {-1, -1}, {-1, 0}, {0, 0}, {0, -1}
    };  // if dir is i then in ccw order, i+1 is the right pixel coord, i is
        // the left pixel coord, i = 0 means left.
    glm::ivec2 start(-1, -1);
    for (int i = 0; i < pixelbin.size().x; i++) {
        for (int j = 0; j < pixelbin.size().y; j++) {
            if (pixelbin.contains(i, j)) {
                start = {i, j};
                break;
            }
        }
        if (start.x != -1) break;
    }
    if (start.x == -1) return outline;
    glm::ivec2 current = start;
    int dir            = 0;
    do {
        outline.push_back(current);
        for (int ndir = (include_diagonal ? dir + 3 : dir + 1) % 4;
             ndir != (dir + 2) % 4;
             ndir = (include_diagonal ? ndir + 1 : ndir + 3) % 4) {
            auto outside = current + offsets[ndir];
            auto inside  = current + offsets[(ndir + 1) % 4];
            if (!pixelbin.contains(outside.x, outside.y) &&
                pixelbin.contains(inside.x, inside.y)) {
                current = current + move[ndir];
                if (dir == ndir) outline.pop_back();
                dir = ndir;
                break;
            }
        }
    } while (current != start);
    return outline;
}

/**
 * @brief Find holes in a binary grid
 *
 * @tparam T The type of the binary grid, must have a `size() -> glm::ivec2`
 * method and a `contains(int, int) -> bool` method
 *
 * @param pixelbin The binary grid
 * @param include_diagonal Whether to include diagonal pixels connected
 * as part of the target obj
 *
 * @return The holes in the binary grid, in cw order
 */
template <HasOutline T>
std::vector<std::vector<glm::ivec2>> find_holes(
    const T& pixelbin, bool include_diagonal = false
) {
    binary_grid binary_grid(pixelbin);
    auto outland = get_outland(binary_grid, !include_diagonal);
    auto holes_solid =
        split_if_multiple(~(outland | binary_grid), !include_diagonal);
    std::vector<std::vector<glm::ivec2>> holes;
    for (auto& hole : holes_solid) {
        holes.emplace_back(find_outline(hole, !include_diagonal));
    }
    return holes;
}

/**
 * @brief Douglas-Peucker algorithm for simplifying a line
 */
template <typename T>
    requires std::same_as<T, glm::ivec2> || std::same_as<T, glm::vec2>
std::vector<T> douglas_peucker(const std::vector<T>& points, float epsilon) {
    if (points.size() < 3) return points;
    float dmax              = 0.0f;
    int index               = 0;
    int end                 = points.size() - 1;
    auto distance_from_line = [](T l1, T l2, T p) {
        if (l1 == l2) return glm::distance(glm::vec2(l1), glm::vec2(p));
        float x1 = l1.x, y1 = l1.y, x2 = l2.x, y2 = l2.y, x = p.x, y = p.y;
        return std::abs((y2 - y1) * x - (x2 - x1) * y + x2 * y1 - y2 * x1) /
               glm::distance(glm::vec2(x1, y1), glm::vec2(x2, y2));
    };
    for (int i = 1; i < end; i++) {
        float d = distance_from_line(points[0], points[end], points[i]);
        if (d > dmax) {
            index = i;
            dmax  = d;
        }
    }
    std::vector<glm::ivec2> results;
    if (dmax > epsilon) {
        auto rec_results1 = douglas_peucker(
            std::vector<glm::ivec2>(points.begin(), points.begin() + index + 1),
            epsilon
        );
        auto rec_results2 = douglas_peucker(
            std::vector<glm::ivec2>(points.begin() + index, points.end()),
            epsilon
        );
        results.insert(
            results.end(), rec_results1.begin(), rec_results1.end() - 1
        );
        results.insert(results.end(), rec_results2.begin(), rec_results2.end());
    } else {
        results.push_back(points[0]);
        results.push_back(points[end]);
    }
    return results;
}

/**
 * @brief Get the polygon of a binary grid
 *
 * @tparam T The type of the binary grid, must have a `size() -> glm::ivec2`
 * method and a `contains(int, int) -> bool` method
 *
 * @param pixelbin The binary grid
 * @param include_diagonal Whether to include diagonal pixels connected
 * as part of the target obj
 *
 * @return The polygon of the binary grid, all in cw order
 */
template <HasOutline T>
std::vector<std::vector<glm::ivec2>> get_polygon(
    const T& pixelbin, bool include_diagonal = false
) {
    auto outline        = find_outline(pixelbin, include_diagonal);
    auto holes          = find_holes(pixelbin, include_diagonal);
    auto earcut_polygon = std::vector<std::vector<glm::ivec2>>();
    earcut_polygon.emplace_back(std::move(outline));
    for (auto& hole : holes) {
        hole.push_back(hole[0]);
        earcut_polygon.emplace_back(std::move(hole));
    }
    return std::move(earcut_polygon);
}

template <typename T>
    requires HasOutline<T>
std::vector<std::vector<std::vector<glm::ivec2>>> get_polygon_multi(
    const T& pixelbin, bool include_diagonal = false
) {
    auto split_bin = split_if_multiple(binary_grid(pixelbin), include_diagonal);
    std::vector<std::vector<std::vector<glm::ivec2>>> result;
    for (auto& bin : split_bin) {
        result.emplace_back(std::move(get_polygon(bin, include_diagonal)));
    }
    return result;
}

/**
 * @brief Get the polygon of a binary grid
 *
 * @tparam T The type of the binary grid, must have a `size() -> glm::ivec2`
 * method and a `contains(int, int) -> bool` method
 *
 * @param pixelbin The binary grid
 * @param epsilon The epsilon value for the Douglas-Peucker algorithm
 * @param include_diagonal Whether to include diagonal pixels connected
 * as part of the target obj
 *
 * @return The polygon of the binary grid, all in cw order
 */
template <HasOutline T>
std::vector<std::vector<glm::ivec2>> get_polygon_simplified(
    const T& pixelbin, float epsilon = 0.5f, bool include_diagonal = false
) {
    auto outline        = find_outline(pixelbin, include_diagonal);
    auto holes          = find_holes(pixelbin, include_diagonal);
    auto earcut_polygon = std::vector<std::vector<glm::ivec2>>();
    earcut_polygon.emplace_back(std::move(douglas_peucker(outline, epsilon)));
    for (auto& hole : holes) {
        hole.push_back(hole[0]);
        earcut_polygon.emplace_back(std::move(douglas_peucker(hole, epsilon)));
    }
    return std::move(earcut_polygon);
}

template <typename T>
    requires HasOutline<T>
std::vector<std::vector<std::vector<glm::ivec2>>> get_polygon_simplified_multi(
    const T& pixelbin, float epsilon = 0.5f, bool include_diagonal = false
) {
    auto split_bin = split_if_multiple(binary_grid(pixelbin), include_diagonal);
    std::vector<std::vector<std::vector<glm::ivec2>>> result;
    for (auto& bin : split_bin) {
        result.emplace_back(
            std::move(get_polygon_simplified(bin, epsilon, include_diagonal))
        );
    }
    return result;
}
}  // namespace epix::physics2d::utils