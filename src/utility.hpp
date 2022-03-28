#include <utility>
#include <optional>
#include <string>
#include <chrono>
#include <unordered_map>
#include <iostream>
#include <set>
#include <tuple>
#include <functional>
#include <assert.h>

// #define claim(X)                                     \
//     {                                                \
//         if (!(X))                                    \
//         {                                            \
//             std::cerr << "Assert fail" << std::endl; \
//             std::abort();                            \
//         }                                            \
//     }

// void claim(bool cond)
// {
//     if (!cond)
//     {
//         std::cerr << "Assert fail" << std::endl;
//         std::abort();
//     }
// }

#define claim(X) assert(X)

enum VariableValue
{
    FALSE,
    TRUE,
    UNDECIDED,
};

bool exclusive_or(bool lhs, bool rhs);

std::optional<bool> variableValue2optional(VariableValue variableValue);

VariableValue optional2variableValue(std::optional<bool> value);

VariableValue bool2variableValue(bool value);

template <class T1, class T2>
struct hash_pair
{
    size_t operator()(const std::pair<T1, T2> &p) const
    {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);

        if (hash1 != hash2)
        {
            return hash1 ^ hash2;
        }

        // If hash1 == hash2, their XOR is zero.
        return hash1;
    }
};

template <class KeyTy, class ValueTy, class KeyHash = std::hash<KeyTy>, class ValueComp = std::less<ValueTy>, class KeyComp = std::less<KeyTy>>
class ModifiableHeap
{
private:
    using HeapElementTy = std::pair<ValueTy, KeyTy>;

    struct Comp
    {
        ValueComp valueComp;
        KeyComp keyComp;
        bool operator()(const HeapElementTy &lhs, const HeapElementTy &rhs) const
        {
            if (valueComp(lhs.first, rhs.first))
                return true;
            else if (valueComp(rhs.first, lhs.first))
                return false;
            else
                return keyComp(lhs.second, rhs.second);
        }
    };

    using HeapTy = std::set<HeapElementTy, Comp>;
    using MappingTy = std::unordered_map<KeyTy, typename HeapTy::iterator, KeyHash>;
    MappingTy key2heapiter;
    HeapTy heap;

public:
    ModifiableHeap() {}
    std::pair<KeyTy, bool> insert(KeyTy key, ValueTy &&value)
    {
        typename MappingTy::iterator insert_attempt = key2heapiter.find(key);
        if (insert_attempt != key2heapiter.end())
            return {insert_attempt->first, false};

        std::pair<typename HeapTy::iterator, bool> value_insert_attempt = heap.insert({std::forward<ValueTy>(value), key});
        claim(value_insert_attempt.second);
        key2heapiter[key] = value_insert_attempt.first;
        return {key, true};
    }

    bool modify(KeyTy key, ValueTy &&newValue)
    {
        // For DEBUG
        claim(heap.size() == key2heapiter.size());
        auto pre_size = heap.size();
        if (!has(key))
            return false;
        auto node = heap.extract(key2heapiter[key]);
        claim(!node.empty());
        node.value().first = std::forward<ValueTy>(newValue);
        heap.insert(std::move(node));
        return true;

        // For DEBUG
        claim(heap.size() == key2heapiter.size());
        claim(pre_size == heap.size());
    }

    bool has(KeyTy key) const
    {
        return key2heapiter.find(key) != key2heapiter.end();
    }

    const HeapElementTy &top() const
    {
        return heap.cbegin().operator*();
    }

    const ValueTy &operator[](KeyTy key)
    {
        return key2heapiter[key]->first;
    }
};