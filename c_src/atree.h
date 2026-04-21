#ifndef ATREE_H
#define ATREE_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <algorithm>

/**
 * A-Tree (Attribute Tree) - Multi-dimensional constraint matching
 * Optimized for standing order filtering by attributes
 */

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
    float bid_cppm;
    int64_t daily_budget_remaining;
    FrequencyCap frequency_cap;
    BrandSafety brand_safety;
    
    StandingOrder() : bid_cppm(0.0f), daily_budget_remaining(0) {}
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
    std::vector<std::string> discrete_values;  // For enum-like dimensions
    
    bool contains(const std::string& value) const;
    bool contains_number(int64_t value) const;
};

// A-Tree node structure
struct ATreeNode {
    DimensionType dimension;
    std::string dimension_name;
    ValueRange value_range;
    
    std::vector<std::shared_ptr<ATreeNode>> children;
    std::vector<StandingOrder> leaf_orders;  // Only populated at leaves
    
    bool is_leaf() const { return children.empty() && !leaf_orders.empty(); }
};

// Type alias for tree root
using ATree = std::shared_ptr<ATreeNode>;

// ============================================================================
// Impression (incoming request)
// ============================================================================

struct Impression {
    std::unordered_map<std::string, std::string> string_attrs;
    std::unordered_map<std::string, int64_t> int_attrs;
    std::unordered_map<std::string, float> float_attrs;
    uint64_t user_hash;
    
    void set_string_attr(const std::string& key, const std::string& value);
    void set_int_attr(const std::string& key, int64_t value);
    void set_float_attr(const std::string& key, float value);
    
    std::string get_string_attr(const std::string& key) const;
    int64_t get_int_attr(const std::string& key) const;
    float get_float_attr(const std::string& key) const;
};

// ============================================================================
// A-Tree Operations
// ============================================================================

class ATreeBuilder {
public:
    ATreeBuilder();
    
    // Build tree from scratch with dimension order
    ATree build(const std::vector<DimensionType>& dimensions);
    
    // Add standing order at correct leaf position
    void insert_order(ATree& root, const StandingOrder& order, 
                      const std::map<std::string, std::string>& attribute_map);
    
    // Helper to create or get child node
    std::shared_ptr<ATreeNode> get_or_create_child(
        std::shared_ptr<ATreeNode> parent,
        DimensionType dim,
        const std::string& value);
    
private:
    std::shared_ptr<ATreeNode> create_node(DimensionType dim, 
                                           const std::string& dimension_name);
};

class ATreeMatcher {
public:
    ATreeMatcher();
    
    // Match impression against tree, return ranked standing orders
    std::vector<StandingOrder> match(
        const ATree& root,
        const Impression& impression,
        const std::vector<DimensionType>& dimension_order);
    
    // Traverse tree following impression attributes
    std::vector<std::shared_ptr<ATreeNode>> traverse(
        const ATree& root,
        const Impression& impression,
        const std::vector<DimensionType>& dimension_order);
    
    // Check brand safety rules
    bool check_brand_safety(const StandingOrder& order, 
                           const Impression& impression);
    
    // Evaluate frequency constraints (atomic lookup)
    bool check_frequency(const StandingOrder& order,
                        const Impression& impression,
                        const std::unordered_map<std::string, uint32_t>& frequency_map);
    
private:
    std::shared_ptr<ATreeNode> find_matching_child(
        const std::shared_ptr<ATreeNode> parent,
        DimensionType dimension,
        const std::string& value);
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
  return enif_raise_exception(env, make_binary(env, reason)));
}

#endif // ATREE_H
