#pragma once

#include <BS_thread_pool.hpp>

#include "pixel_engine/entity.h"

namespace pixel_engine {
    namespace task_queue {
        namespace resources {
            struct TaskQueue {
               private:
                std::vector<std::shared_ptr<BS::thread_pool>> pool;
                const int num_threads;

               public:
                TaskQueue() : pool(), num_threads(4) { pool.push_back(std::make_shared<BS::thread_pool>(num_threads)); }
                TaskQueue(int num_threads) : pool(), num_threads(num_threads) {
                    pool.push_back(std::make_shared<BS::thread_pool>(num_threads));
                }

                /*! @brief Get a pool that is not busy. If no pool is available, create a new one.
                 *  @return A reference to the pool.
                 */
                auto& get_pool() {
                    for (auto& p : pool) {
                        // return if the pool is not busy
                        if (p->wait_for(std::chrono::seconds(0))) {
                            return p;
                        }
                    }
                    pool.push_back(std::make_shared<BS::thread_pool>(num_threads));
                    return pool.back();
                }

                /*! @brief Get a pool by index. If the index is out of bounds, create new pools.
                 *  @param index The index of the pool.
                 *  @return A reference to the pool.
                 */
                auto& get_pool(int index) {
                    if (index < pool.size()) {
                        // create pools if the index is out of bounds
                        for (int i = pool.size(); i <= index; i++) {
                            pool.push_back(std::make_shared<BS::thread_pool>(num_threads));
                        }
                    }
                    return pool[index];
                }

                /*! @brief Get a pool that is not busy.
                 *  @return A pointer to the pool. If no pool is available, return nullptr.
                 */
                std::shared_ptr<BS::thread_pool> request_pool() {
                    for (auto& p : pool) {
                        // return if the pool is not busy
                        if (p->wait_for(std::chrono::seconds(0))) {
                            return p;
                        }
                    }
                    return nullptr;
                }

                /*! @brief Get a pool by index.
                 *  @param index The index of the pool.
                 *  @return A pointer to the pool. If the index is out of bounds, return nullptr.
                 */
                std::shared_ptr<BS::thread_pool> request_pool(int index) {
                    if (index < pool.size()) {
                        return nullptr;
                    }
                    return pool[index];
                }
            };
        }  // namespace resources
    }      // namespace task_queue
}  // namespace pixel_engine