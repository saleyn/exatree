#include "atree.h"
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <cctype>

// ============================================================================
// ValueRange Implementation
// ============================================================================

bool ValueRange::contains(const std::string& value) const {
    // Check discrete values first
    if (!discrete_values.empty())
        return discrete_values.find(value) != discrete_values.end();
    
    // Check numeric range bounds
    if (!min_val.empty() || !max_val.empty()) {
        // Try to parse value as a number
        try {
            int64_t val = std::stoll(value);
            
            // Parse and cache min value
            if (!min_val.empty() && !has_cached_min) {
                cached_min = std::stoll(min_val);
                has_cached_min = true;
            }
            // Parse and cache max value
            if (!max_val.empty() && !has_cached_max) {
                cached_max = std::stoll(max_val);
                has_cached_max = true;
            }
            
            if (has_cached_min && val < cached_min) return false;
            if (has_cached_max && val > cached_max) return false;
            return true;
        } catch (...) {
            // If parsing fails, fall through to string comparison
        }
    }
    
    // String comparison as fallback
    if (!min_val.empty() && value < min_val) return false;
    if (!max_val.empty() && value > max_val) return false;
    
    return true;
}

bool ValueRange::contains_number(int64_t value) const {
    // For numeric ranges
    if (!min_val.empty()) {
        int64_t min = std::stoll(min_val);
        if (value < min) return false;
    }
    if (!max_val.empty()) {
        int64_t max = std::stoll(max_val);
        if (value > max) return false;
    }
    return true;
}

// ============================================================================
// Impression Implementation
// ============================================================================

bool Impression::has_attr(const std::string& key) const {
    return attrs.find(key) != attrs.end();
}

bool Impression::remove_attr(const std::string& key) {
    return attrs.erase(key) > 0;
}

// Convenience method for get_string without explicit default
const std::string& Impression::get_string(const std::string& key) const {
    static const std::string empty_string;
    
    // Use cached values for the 7 standard dimensions
    if (key == "age_range")
        return get_cached("age_range", cached_age_range, has_cached_age_range);
    if (key == "interest")
        return get_cached("interest", cached_interest, has_cached_interest);
    if (key == "content_category")
        return get_cached("content_category", cached_content_category, has_cached_content_category);
    if (key == "content_event")
        return get_cached("content_event", cached_content_event, has_cached_content_event);
    if (key == "geography")
        return get_cached("geography", cached_geography, has_cached_geography);
    if (key == "time_of_day")
        return get_cached("time_of_day", cached_time_of_day, has_cached_time_of_day);
    if (key == "device_type")
        return get_cached("device_type", cached_device_type, has_cached_device_type);
    
    // Fallback for unknown keys (also without exception overhead)
    auto it = attrs.find(key);
    if (it != attrs.end() && std::holds_alternative<std::string>(it->second))
        return std::get<std::string>(it->second);
    return empty_string;
}

// ============================================================================
// ATreeBuilder Implementation
// ============================================================================

ATreeBuilder::ATreeBuilder() {}

ATree ATreeBuilder::build(const std::vector<DimensionType>& dimensions) {
    if (dimensions.empty()) {
        throw std::invalid_argument("Dimension list cannot be empty");
    }
    
    auto root = create_node(dimensions[0], "root");
    return root;
}

std::unique_ptr<ATreeNode> ATreeBuilder::create_node(DimensionType dim, 
                                                     const std::string& dimension_name) {
    auto node = std::make_unique<ATreeNode>();
    node->dimension = dim;
    node->dimension_name = dimension_name;
    return node;
}

ATreeNode* ATreeBuilder::get_or_create_child(
    ATreeNode* parent,
    DimensionType dim,
    const std::string& value) {
    
    // Note: Erlang's resource system provides synchronization
    // Create new child
    auto new_child = create_node(dim, "");
    
    // Check for open-ended range format: "NUM+"
    bool is_open_range = false;
    if (value.length() > 1 && value[value.length() - 1] == '+') {
        std::string num_part = value.substr(0, value.length() - 1);
        if (!num_part.empty() && std::all_of(num_part.begin(), num_part.end(), ::isdigit)) {
            // Parse as range with no upper limit
            new_child->value_range.min_val = num_part;
            new_child->value_range.max_val = "";
            is_open_range = true;
        }
    }
    
    // If not an open range, check for closed range format: "min-max"
    if (!is_open_range) {
        size_t dash_pos = value.find('-');
        if (dash_pos != std::string::npos && dash_pos > 0 && dash_pos < value.length() - 1) {
            // Check if it looks like a range (numeric-numeric)
            std::string min_part = value.substr(0, dash_pos);
            std::string max_part = value.substr(dash_pos + 1);
            
            // Check if both parts are numeric
            bool min_numeric = !min_part.empty() && std::all_of(min_part.begin(), min_part.end(), ::isdigit);
            bool max_numeric = !max_part.empty() && std::all_of(max_part.begin(), max_part.end(), ::isdigit);
            
            if (min_numeric && max_numeric) {
                // Store as a numeric range
                new_child->value_range.min_val = min_part;
                new_child->value_range.max_val = max_part;
                is_open_range = true;  // Mark as range (closed or open)
            }
        }
    }
    
    // If it's not a range, store as discrete value
    if (!is_open_range) {
        new_child->value_range.discrete_values.insert(value);
    }
    
    // Check if child with this configuration already exists
    for (auto& existing_child : parent->children) {
        // If both are range nodes with same bounds, reuse
        if (!existing_child->value_range.min_val.empty() || !existing_child->value_range.max_val.empty()) {
            if (existing_child->value_range.min_val == new_child->value_range.min_val &&
                existing_child->value_range.max_val == new_child->value_range.max_val)
                return existing_child.get();  // Return raw pointer to existing
        }
        // If both are discrete nodes with same values
        if (existing_child->value_range.discrete_values.size() > 0 &&
            new_child->value_range.discrete_values.size() > 0) {
            if (existing_child->value_range.discrete_values.size() == 1 &&
                new_child->value_range.discrete_values.size() == 1) {
                auto existing_val = *existing_child->value_range.discrete_values.begin();
                auto new_val = *new_child->value_range.discrete_values.begin();
                if (existing_val == new_val)
                    return existing_child.get();  // Return raw pointer to existing
            }
        }
    }
    
    // No existing match, add new child - move ownership to parent
    auto* raw_ptr = new_child.get();
    parent->children.push_back(std::move(new_child));
    
    // Build key for map: use value for discrete, use min-max for ranges
    std::string map_key;
    if (!raw_ptr->value_range.discrete_values.empty()) {
        map_key = *raw_ptr->value_range.discrete_values.begin();
    } else if (!raw_ptr->value_range.min_val.empty() || !raw_ptr->value_range.max_val.empty()) {
        map_key = raw_ptr->value_range.min_val + "-" + raw_ptr->value_range.max_val;
    } else {
        map_key = value;
    }
    parent->children_map[map_key] = raw_ptr;
    
    return raw_ptr;
}

void ATreeBuilder::insert_order(ATree& root, const StandingOrder& order,
                                const std::map<std::string, std::string>& attribute_map) {
    // Note: Erlang's resource system provides synchronization, no per-node locking needed
    ATreeNode* current = root.get();
    
    // Map attribute names to dimension types
    std::vector<std::pair<DimensionType, std::string>> attrs_to_traverse = {
        {DimensionType::AGE_RANGE,        "age_range"},
        {DimensionType::INTEREST,         "interest"},
        {DimensionType::CONTENT_CATEGORY, "content_category"},
        {DimensionType::CONTENT_EVENT,    "content_event"},
        {DimensionType::GEOGRAPHY,        "geography"},
        {DimensionType::TIME_OF_DAY,      "time_of_day"},
        {DimensionType::DEVICE_TYPE,      "device_type"}
    };
    
    // Traverse and insert using proper dimensions only
    for (const auto& [dim_type, dim_name] : attrs_to_traverse) {
        auto it = attribute_map.find(dim_name);
        if (it != attribute_map.end()) {
            current = get_or_create_child(current, dim_type, it->second);
        }
    }
    
    // Add order to the final node in the path
    current->leaf_orders.push_back(order);
}

// ============================================================================
// ATreeMatcher Implementation
// ============================================================================

ATreeMatcher::ATreeMatcher() {}

std::vector<StandingOrder> ATreeMatcher::match(
    const ATree& root,
    const Impression& impression,
    const std::vector<DimensionType>& dimension_order,
    uint32_t max_match) const {
    
    // Traverse tree to find matching nodes
    auto matching_nodes = traverse(root, impression, dimension_order);
    
    // Collect all standing orders from matching nodes
    // Estimate capacity: assume 10 orders per node on average
    std::vector<StandingOrder> candidates;
    candidates.reserve(matching_nodes.size() * 10);
    
    for (const auto& node : matching_nodes) {
        candidates.insert(candidates.end(), 
                         node->leaf_orders.begin(), 
                         node->leaf_orders.end());
    }
    
    // Filter by brand safety (but skip if no brands have rules)
    std::vector<StandingOrder> safe_candidates;
    bool has_brand_safety_rules = false;
    
    // Quick check: does any order have brand safety rules?
    for (const auto& order : candidates) {
        if (!order.brand_safety.excluded_categories.empty() || 
            !order.brand_safety.excluded_keywords.empty() ||
            order.brand_safety.require_age_gate) {
            has_brand_safety_rules = true;
            break;
        }
    }
    
    if (has_brand_safety_rules) {
        safe_candidates.reserve(candidates.size());
        for (const auto& order : candidates) {
            if (check_brand_safety(order, impression))
                safe_candidates.push_back(order);
        }
    } else {
        safe_candidates = std::move(candidates);
    }
    
    // Sort by bid price (descending)
    std::sort(safe_candidates.begin(), safe_candidates.end(),
              [](const StandingOrder& a, const StandingOrder& b) {
                  return a.bid_cpm > b.bid_cpm;
              });
    
    // Truncate to max_match if specified (0 means no limit)
    if (max_match > 0 && safe_candidates.size() > max_match)
        safe_candidates.resize(max_match);
    
    return safe_candidates;
}

std::vector<ATreeNode*> ATreeMatcher::traverse(
    const ATree& root,
    const Impression& impression,
    const std::vector<DimensionType>& dimension_order) const {
    
    std::vector<ATreeNode*> current_nodes = {root.get()};
    
    // Traverse each dimension level
    for (const auto& dimension : dimension_order) {
        std::vector<ATreeNode*> next_nodes;
        
        // Get dimension name using lookup table instead of switch
        auto dim_name = DimensionNames::get(dimension);
        if (dim_name.empty()) continue;  // Skip unknown dimensions
        
        std::string dim_str(dim_name);  // Convert to std::string
        
        for (const auto& node : current_nodes) {
            // Note: Erlang's resource system provides synchronization
            if (node->children.empty()) {
                // Already at leaf
                next_nodes.push_back(node);
                continue;
            }
            
            // Get attribute value (uses cached lookup)
            const auto& attr_value = impression.get_string(dim_str);
            
            if (!attr_value.empty()) {
                auto child = find_matching_child(node, dimension, attr_value);
                if (child)
                    next_nodes.push_back(child);
            }
        }
        
        current_nodes = next_nodes;
        if (current_nodes.empty()) break;
    }
    
    return current_nodes;
}

ATreeNode* ATreeMatcher::find_matching_child(
    const ATreeNode* parent,
    DimensionType dimension,
    const std::string& value) const {
    
    // First try exact match in discrete values map
    auto it = parent->children_map.find(value);
    if (it != parent->children_map.end())
        return it->second;
    
    // Fall back to iterating through children for range matches
    // (ranges can't be pre-indexed exactly)
    for (const auto& child : parent->children) {
        if (child->value_range.contains(value))
            return child.get();
    }
    return nullptr;
}

bool ATreeMatcher::check_brand_safety(const StandingOrder& order,
                                     const Impression& impression) const {
    // Check excluded categories
    const auto& content_cat = impression.get_string("content_category");
    for (const auto& excluded : order.brand_safety.excluded_categories) {
        if (content_cat == excluded)
            return false;
    }
    
    // Check age gate requirement
    if (order.brand_safety.require_age_gate) {
        const auto& age_range = impression.get_string("age_range");
        if (age_range == "unknown")
            return false;
    }
    
    return true;
}

bool ATreeMatcher::check_frequency(const StandingOrder& order,
                                  const Impression& impression,
                                  const std::unordered_map<std::string, uint32_t, StringHasher>& frequency_map) const {
    // Create frequency key: user_hash:campaign_id
    std::string freq_key = std::to_string(impression.user_hash) + ":" + order.campaign_id;
    
    auto it = frequency_map.find(freq_key);
    if (it == frequency_map.end())
        return true;  // No frequency data = allow
    
    uint32_t current_count = it->second;
    
    // Check limits
    if (order.frequency_cap.hourly_limit > 0 && 
        current_count >= order.frequency_cap.hourly_limit)
        return false;
    
    if (order.frequency_cap.daily_limit > 0 && 
        current_count >= order.frequency_cap.daily_limit)
        return false;
    
    if (order.frequency_cap.weekly_limit > 0 && 
        current_count >= order.frequency_cap.weekly_limit)
        return false;
    
    return true;
}
