#pragma once

#include <map>
#include <queue>
#include <set>
#include <string>
#include <typeinfo>
#include <vector>

typedef uint64_t Entity;

class Data {
   private:
    std::vector<uint8_t> data_raw;

   public:
    Data() : data_raw(){};
    Data(int size) : data_raw(size){};

    template <typename T>
    T* get_data() {
        return reinterpret_cast<T*>(data_raw.data());
    }

    template <typename T>
    T get_data(Entity entity) {
        return *reinterpret_cast<T*>(&data_raw[entity * sizeof(T)]);
    }
};

class entity_component {
   private:
    std::map<std::string, Data> component_datas;
    std::queue<size_t> available_entity_id;
    std::set<Entity> entities;
    size_t size;

    template <typename T>
    void insert_compenent_entity_single(const Entity& entity, T arg1) {
        if (component_datas.find(typeid(T).name()) == component_datas.end()) {
            Data data(size * sizeof(T));
            // component_datas.insert({id, data});
            component_datas.insert({typeid(arg1).name(), data});
        }
        T* data = component_datas[typeid(arg1).name()].get_data<T>();
        data[entity] = arg1;
    }

    void insertComponentToEntity(const Entity& entity) {}

    template <typename T, typename... Args>
    void insertComponentToEntity(const Entity& entity, T head, Args... args) {
        insert_compenent_entity_single(entity, head);
        insertComponentToEntity(entity, args...);
    }

   public:
    entity_component() {
        available_entity_id = std::queue<size_t>();
        size = 512;
        for (size_t i = 0; i < size; i++) {
            available_entity_id.push(i);
        }
    }

    template <typename... Args>
    Entity spawn(Args... args) {
        Entity entity = available_entity_id.front();
        entities.insert(entity);
        insertComponentToEntity(entity, args...);
        return entity;
    }

    void despawn(Entity entity) {
        for (auto iter = component_datas.begin(); iter != component_datas.end();
             iter++) {
            // iter->second.erase(entity);
        }
        entities.erase(entity);
        available_entity_id.push(entity);
    }

    template <typename T>
    T* retrieve_component(Entity entity) {
        if (component_datas.find(typeid(T).name()) == component_datas.end())
            return nullptr;
        T* data = component_datas[typeid(T).name()].get_data<T>();
        return &data[entity];
    }
};