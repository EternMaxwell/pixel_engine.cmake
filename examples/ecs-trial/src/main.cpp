#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <tuple>
#include <typeinfo>
#include <vector>

typedef uint64_t Entity;

class entity_component {
   private:
    std::map<std::string, std::vector<uint8_t>> component_datas;
    std::queue<size_t> available_entity_id;

    template <typename T, typename... Args>
    void insertComponentToEntity(Entity entity, T arg1, Args... args) {
        if (sizeof...(args) >= 0) insertComponentToEntity(entity, args...);
        if (component_datas.find(typeid(arg1).name()) ==
            component_datas.end()) {
            std::vector<uint8_t> data(sizeof(arg1) * 512);
            // component_datas.insert({id, data});
            component_datas[typeid(arg1).name()] = data;
        }
        T* data =
            reinterpret_cast<T*>(component_datas[typeid(arg1).name()].data());
        data[entity] = arg1;
    }

   public:
    entity_component() {
        available_entity_id = std::queue<size_t>();
        for (size_t i = 0; i < 512; i++) {
            available_entity_id.push(i);
        }
    }

    template <typename... Args>
    Entity spawn(Args... args) {
        Entity entity = available_entity_id.front();
        insertComponentToEntity(entity, args...);
    }

    void print_data() {
        for (auto iter = component_datas.begin(); iter != component_datas.end();
             iter++) {
            std::cout << iter->first << ":" << std::endl;
            std::cout << "[";
            for (auto it : iter->second) {
                std::cout << it;
            }
            std::cout << "]" << std::endl;
        }
    }
};

int main() {
    entity_component ecs;
    ecs.spawn(1, 2.0);
    ecs.print_data();
    return 0;
}