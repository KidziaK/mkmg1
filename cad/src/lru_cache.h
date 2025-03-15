#pragma once

#include <unordered_map>
#include <list>
#include <utility>

template <typename Key, typename Value>
class LRUCache {
public:
    LRUCache(size_t capacity) : capacity_(capacity) {}

    Value get(const Key& key) {
        if (cache_.find(key) != cache_.end()) {
            list_.erase(cache_[key].second);
            list_.push_front(key);
            cache_[key].second = list_.begin();
            return cache_[key].first;
        }
        return Value();
    }

    void put(const Key& key, const Value& value) {
        if (cache_.find(key) != cache_.end()) {
            list_.erase(cache_[key].second);
            list_.push_front(key);
            cache_[key].second = list_.begin();
            cache_[key].first = value;
        } else {
            if (cache_.size() >= capacity_) {
                cache_.erase(list_.back());
                list_.pop_back();
            }
            list_.push_front(key);
            cache_[key] = {value, list_.begin()};
        }
    }

private:
    size_t capacity_;
    std::list<Key> list_;
    std::unordered_map<Key, std::pair<Value, typename std::list<Key>::iterator>> cache_;
};
