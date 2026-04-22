#ifndef ATREE_H
#define ATREE_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include "xxhash.hpp"
#include <erl_nif.h>

/**
 * A-Tree (Attribute Tree) - Multi-dimensional constraint matching
 * Optimized for standing order filtering by attributes
 */

// ============================================================================
// Custom Hash Functions
// ============================================================================

namespace std {
    // Custom hash function for std::string using xxhash
    template <>
    struct hash<std::string> {
        size_t operator()(const std::string& s) const noexcept {
            return xxh::xxhash<64>(s);
        }
    };
}

// ============================================================================
// Data Structures
// ============================================================================

struct FrequencyCap {
    uint32_t hourly_limit;
    uint32_t daily_limit;
    uint32_t weekly_limit;
    
    FrequencyCap() : hourly_limit(0), daily_limit(0), weekly_limit(0) {}
};

struct BrandSafety {
    std::vector<std::string> excluded_categories;
    std::vector<std::string> excluded_keywords;
    bool require_age_gate;
    
    BrandSafety() : require_age_gate(false) {}
};

struct StandingOrder {
    std::string campaign_id;
    float bid_cpm;
    int64_t daily_budget_remaining;
    FrequencyCap frequency_cap;
    BrandSafety brand_safety;
    
    StandingOrder() : bid_cpm(0.0f), daily_budget_remaining(0) {}
};

// Dimension types for A-Tree
enum class DimensionType {
    AGE_RANGE,
    INTEREST,
    CONTENT_CATEGORY,
    CONTENT_EVENT,
    GEOGRAPHY,
    TIME_OF_DAY,
    DEVICE_TYPE,
    CUSTOM
};

// Value range for dimension matching
struct ValueRange {
    std::string min_val;
    std::string max_val;
    std::unordered_set<std::string> discrete_values;  // Use hash set for O(1) lookup
    
    // Cached parsed values for performance
    mutable int64_t  cached_min = std::numeric_limits<int64_t>::min();
    mutable int64_t  cached_max = std::numeric_limits<int64_t>::max();
    mutable bool has_cached_min = false;
    mutable bool has_cached_max = false;
    
    bool contains(const std::string& value) const;
    bool contains_number(int64_t value) const;
};

// A-Tree node structure - Thread-safe with per-node locking
struct ATreeNode {
    DimensionType dimension;
    std::string   dimension_name;
    ValueRange    value_range;
    
    std::vector<std::unique_ptr<ATreeNode>>     children;      // Changed from shared_ptr to unique_ptr
    std::unordered_map<std::string, ATreeNode*> children_map;  // Index for O(1) lookup (uses xxhash for std::string keys)
    std::vector<StandingOrder>                  leaf_orders;   // Only populated at leaves
    
    // Mutable mutex for thread-safe access
    mutable std::shared_mutex node_mutex;
    
    bool is_leaf() const { return children.empty() && !leaf_orders.empty(); }
};

// Type alias for tree root
using ATree = std::unique_ptr<ATreeNode>;

// ============================================================================
// Dimension Name Lookup Table
// ============================================================================

namespace DimensionNames {
    static constexpr std::string_view get(DimensionType dim) noexcept {
        switch (dim) {
            case DimensionType::AGE_RANGE:        return "age_range";
            case DimensionType::INTEREST:         return "interest";
            case DimensionType::CONTENT_CATEGORY: return "content_category";
            case DimensionType::CONTENT_EVENT:    return "content_event";
            case DimensionType::GEOGRAPHY:        return "geography";
            case DimensionType::TIME_OF_DAY:      return "time_of_day";
            case DimensionType::DEVICE_TYPE:      return "device_type";
            default:                              return "";
        }
    }
}

// ============================================================================
// Impression (incoming request)
// ============================================================================

struct Impression {
    // Unified attribute value type
    using AttrValue = std::variant<int64_t, std::string, double, bool>;
    
    std::unordered_map<std::string, AttrValue> attrs;
    uint64_t user_hash;
    
    // Cached string values for the 7 standard dimensions (performance optimization)
    mutable std::string cached_age_range;
    mutable std::string cached_interest;
    mutable std::string cached_content_category;
    mutable std::string cached_content_event;
    mutable std::string cached_geography;
    mutable std::string cached_time_of_day;
    mutable std::string cached_device_type;
    
    mutable bool has_cached_age_range        = false;
    mutable bool has_cached_interest         = false;
    mutable bool has_cached_content_category = false;
    mutable bool has_cached_content_event    = false;
    mutable bool has_cached_geography        = false;
    mutable bool has_cached_time_of_day      = false;
    mutable bool has_cached_device_type      = false;
    
    // Generic template methods for variant values
    template<typename T>
    void set(const std::string& key, T value) {
        attrs[key] = value;
    }
    
    // Generic get template - returns copies for primitives
    template<typename T>
    T get(const std::string& key, const T default_val = T()) const {
        auto it = attrs.find(key);
        if (it == attrs.end())
            return default_val;
        // Use holds_alternative to avoid exception overhead
        if (std::holds_alternative<T>(it->second)) {
            return std::get<T>(it->second);
        }
        return default_val;
    }
    
    // Convenience method for string values without explicit default
    const std::string& get_string(const std::string& key) const;
    
    // Helper methods
    bool has_attr(const std::string& key) const;
    bool remove_attr(const std::string& key);
    
    // Clear all attributes
    void clear_attrs() { attrs.clear(); }

private:
    // Private helper for cached string lookups
    const std::string& get_cached(const std::string& attr, std::string& cache, bool& cached) const {
        static const std::string empty_string;
        if (!cached) {
            auto it = attrs.find(attr);
            if (it != attrs.end() && std::holds_alternative<std::string>(it->second))
                cache = std::get<std::string>(it->second);
            // Mark lookup as done: found, not found, or wrong type all return empty
            cached = true;
        }
        return cache;
    }
};

// ============================================================================
// A-Tree Operations
// ============================================================================

class ATreeBuilder {
public:
    ATreeBuilder();
    
    // Build tree from scratch with dimension order (thread-safe)
    ATree build(const std::vector<DimensionType>& dimensions);
    
    // Add standing order at correct leaf position (thread-safe)
    // Note: Caller optionally acquires exclusive lock on root's node_mutex
    void insert_order(ATree& root, const StandingOrder& order, 
                      const std::map<std::string, std::string>& attribute_map);
    
    // Helper to create or get child node (requires parent lock)
    // Returns raw pointer to newly created or existing child
    ATreeNode* get_or_create_child(
        ATreeNode* parent,
        DimensionType dim,
        const std::string& value);
    
private:
    std::unique_ptr<ATreeNode> create_node(DimensionType dim, 
                                           const std::string& dimension_name);
};

class ATreeMatcher {
public:
    ATreeMatcher();
    
    // Match impression against tree, return ranked standing orders (thread-safe with read locks)
    // max_match: 0 means no limit, >0 limits results
    std::vector<StandingOrder> match(
        const ATree& root,
        const Impression& impression,
        const std::vector<DimensionType>& dimension_order,
        uint32_t max_match = 0) const;
    
    // Traverse tree following impression attributes (thread-safe with read locks)
    std::vector<ATreeNode*> traverse(
        const ATree& root,
        const Impression& impression,
        const std::vector<DimensionType>& dimension_order) const;
    
    // Check brand safety rules
    bool check_brand_safety(const StandingOrder& order, 
                           const Impression& impression) const;
    
    // Evaluate frequency constraints (atomic lookup)
    bool check_frequency(const StandingOrder& order,
                        const Impression& impression,
                        const std::unordered_map<std::string, uint32_t>& frequency_map) const;
    
private:
    // Recursive tree traversal with read locks
    ATreeNode* find_matching_child(
        const ATreeNode* parent,
        DimensionType dimension,
        const std::string& value) const;
};

inline std::tuple<ERL_NIF_TERM, unsigned char*>
make_binary(ErlNifEnv* env, size_t size)
{
  ERL_NIF_TERM term;
  auto   p = enif_make_new_binary(env, size, &term);
  return std::make_tuple(term, p);
}

inline ERL_NIF_TERM
make_binary(ErlNifEnv* env, const char* str)
{
  ERL_NIF_TERM term;
  auto sz = strlen(str);
  auto p  = enif_make_new_binary(env, sz, &term);
  memcpy(p, str, sz);
  return term;
}

inline ERL_NIF_TERM raise_error(ErlNifEnv* env, const char* reason)
{
  return enif_raise_exception(env, make_binary(env, reason));
}

inline ERL_NIF_TERM raise_error(ErlNifEnv* env, ERL_NIF_TERM reason)
{
  return enif_raise_exception(env, reason);
}

#endif // ATREE_H
