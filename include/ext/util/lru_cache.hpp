// Copyright - 2020 - Jan Christoph Uhde <Jan@UhdeJC.com>
// Please see LICENSE.md for license or visit https://github.com/extcpp/basics

#include <functional>
#include <list>
#include <mutex>
#include <unordered_map>

#ifndef EXT_UTIL_LRU_CACHE_HEADER
#define EXT_UTIL_LRU_CACHE_HEADER

namespace ext::util {

template <typename Key, typename Value>
class lru_cache {
public:
    lru_cache(size_t max_size) : _max_size{max_size} { }
    lru_cache(lru_cache const&) = delete;

    static void default_update(Value&){};
    static bool default_remove(Value const&){ return false; };

    void put(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> guard(_mut);
        auto found = _map.find(key);

        if (found == _map.end()) {
            if (_map.size() + 1 > _max_size) {
                auto last = _list.crbegin();
                _map.erase(last->first);
                _list.pop_back();
            }

            _list.emplace_front(key, value);
            _map[key] = _list.begin();
        } else {
            found->second->second = value;
            _list.splice(_list.cbegin(), _list, found->second);
        }
    }

    template<typename UpdatePredicate = decltype(default_update), typename RemovePredicate = decltype(default_remove)>
    const std::pair<Value, bool> get(const Key& key, UpdatePredicate update_pred = &default_update, RemovePredicate remove_pred = &default_remove) {
        static_assert(std::is_convertible_v<UpdatePredicate, std::function<void(Value&)>>, "Wrong Singnature for UpdatePredicate");
        static_assert(std::is_convertible_v<RemovePredicate, std::function<bool(Value&)>>, "Wrong Singnature for RemovePredicate");

        std::lock_guard<std::mutex> guard(_mut);

        auto found = _map.find(key);
        if (found == _map.end()) {
            return {Value{}, false};
        } else {
            Value& value = found->second->second;
            if (remove_pred(value)) {
                // move value to end of list to end of list
                _list.splice(_list.cend(), _list, found->second);
                //remove item from map
                _map.erase(found);
                //delete element
                _list.pop_back();
                return {Value{}, false};
            }
            update_pred(value);
            _list.splice(_list.begin(), _list, found->second);
            return {value, true};
        }
    }

    bool exists(const Key& key) const noexcept {
        std::lock_guard<std::mutex> guard(_mut);

        return _map.find(key) != _map.end();
    }

    template<typename UpdatePredicate = decltype(default_update), typename RemovePredicate = decltype(default_remove)>
    bool exists(const Key& key, UpdatePredicate update_pred = default_update, RemovePredicate remove_pred = default_remove) {
        static_assert(std::is_convertible_v<UpdatePredicate, std::function<void(Value&)>>, "Wrong Singnature for UpdatePredicate");
        static_assert(std::is_convertible_v<RemovePredicate, std::function<bool(Value&)>>, "Wrong Singnature for RemovePredicate");
        std::lock_guard<std::mutex> guard(_mut);

        auto found = _map.find(key);
        if (found != _map.end()) {
            Value& value = found->second->second;
            if (remove_pred(value)) {
                // move value to end of list to end of list
                _list.splice(_list.cend(), _list, found->second);
                //remove item from map
                _map.erase(found);
                //delete element
                _list.pop_back();
                return false;
            }
            update_pred(value);
            return true;
        } else {
            return false;
        }
    }

    bool remove(const Key& key) {
        std::lock_guard<std::mutex> guard(_mut);

        auto found = _map.find(key);
        if (found != _map.end()) {
            // move value to end of list to end of list
            _list.splice(_list.cend(), _list, found->second);
            //remove item from map
            _map.erase(found);
            //delete element
            _list.pop_back();
            return true;
        }
        return false;
    }


    template<typename RemovePredicate, typename = std::enable_if_t<!std::is_same_v<RemovePredicate,Value>>>
    std::size_t remove(RemovePredicate remove_pred) {
        static_assert(std::is_convertible_v<RemovePredicate, std::function<bool(Value&)>>, "Wrong Singnature for RemovePredicate");
        std::lock_guard<std::mutex> guard(_mut);

        std::size_t n = 0;
        for (auto it = _list.begin(); it != _list.end();) {
            Value& value = it->second;
            if (remove_pred(value)) {
                _map.erase(value);
                ++it;
                _list.pop_front();
                ++n;
            } else {
                ++it;
            }
        }
        return n;
    }

    size_t size() const noexcept {
        std::lock_guard<std::mutex> guard(_mut);
        return _map.size();
    }

private:
    std::list<std::pair<Key, Value>> _list;
    std::unordered_map<Key, decltype(_list.begin())> _map;
    size_t _max_size;
    mutable std::mutex _mut;
};

} // namespace ext::util
#endif // EXT_UTIL_LRU_CACHE_HEADER
