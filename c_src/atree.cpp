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
    if (!discrete_values.empty()) {
        return std::find(discrete_values.begin(), discrete_values.end(), value) 
               != discrete_values.end();
    }
    
    // Check numeric range bounds
    if (!min_val.empty() || !max_val.empty()) {
        // Try to parse value as a number
        try {
            int64_t val = std::stoll(value);
            
            if (!min_val.empty()) {
                int64_t min = std::stoll(min_val);
                if (val < min) return false;
            }
            if (!max_val.empty()) {
                int64_t max = std::stoll(max_val);
                if (val > max) return false;
            }
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

void Impression::set_string_attr(const std::string& key, const std::string& value) {
    string_attrs[key] = value;
}

void Impression::set_int_attr(const std::string& key, int64_t value) {
    int_attrs[key] = value;
}

void Impression::set_float_attr(const std::string& key, float value) {
    float_attrs[key] = value;
}

std::string Impression::get_string_attr(const std::string& key) const {
    auto it = string_attrs.find(key);
    if (it != string_attrs.end()) return it->second;
    return "";
}

int64_t Impression::get_int_attr(const std::string& key) const {
    auto it = int_attrs.find(key);
    if (it != int_attrs.end()) return it->second;
    return 0;
}

float Impression::get_float_attr(const std::string& key) const {
    auto it = float_attrs.find(key);
    if (it != float_attrs.end()) return it->second;
    return 0.0f;
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

std::shared_ptr<ATreeNode> ATreeBuilder::create_node(DimensionType dim, 
                                                     const std::string& dimension_name) {
    auto node = std::make_shared<ATreeNode>();
    node->dimension = dim;
    node->dimension_name = dimension_name;
    return node;
}

std::shared_ptr<ATreeNode> ATreeBuilder::get_or_create_child(
    std::shared_ptr<ATreeNode> parent,
    DimensionType dim,
    const std::string& value) {
    
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
        new_child->value_range.discrete_values.push_back(value);
    }
    
    // Check if child with this configuration already exists
    for (auto& existing_child : parent->children) {
        // If both are range nodes with same bounds, reuse
        if (!existing_child->value_range.min_val.empty() || !existing_child->value_range.max_val.empty()) {
            if (existing_child->value_range.min_val == new_child->value_range.min_val &&
                existing_child->value_range.max_val == new_child->value_range.max_val) {
                return existing_child;  // Reuse existing range node
            }
        }
        // If both are discrete nodes with same values
        if (existing_child->value_range.discrete_values.size() > 0 &&
            new_child->value_range.discrete_values.size() > 0) {
            if (existing_child->value_range.discrete_values[0] == new_child->value_range.discrete_values[0]) {
                return existing_child;  // Reuse existing discrete node
            }
        }
    }
    
    // No existing match, add new child
    parent->children.push_back(new_child);
    return new_child;
}

void ATreeBuilder::insert_order(ATree& root, const StandingOrder& order,
                                const std::map<std::string, std::string>& attribute_map) {
    auto current = root;
    
    // Map attribute names to dimension types
    std::vector<std::pair<DimensionType, std::string>> attrs_to_traverse = {
        {DimensionType::AGE_RANGE, "age_range"},
        {DimensionType::INTEREST, "interest"},
        {DimensionType::CONTENT_CATEGORY, "content_category"},
        {DimensionType::CONTENT_EVENT, "content_event"},
        {DimensionType::GEOGRAPHY, "geography"},
        {DimensionType::TIME_OF_DAY, "time_of_day"},
        {DimensionType::DEVICE_TYPE, "device_type"}
    };
    
    // Traverse and insert using proper dimensions only
    for (const auto& [dim_type, dim_name] : attrs_to_traverse) {
        auto it = attribute_map.find(dim_name);
        if (it != attribute_map.end()) {
            auto child = get_or_create_child(current, dim_type, it->second);
            current = child;
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
    const std::vector<DimensionType>& dimension_order) {
    
    // Traverse tree to find matching nodes
    auto matching_nodes = traverse(root, impression, dimension_order);
    
    // Collect all standing orders from matching nodes
    std::vector<StandingOrder> candidates;
    for (const auto& node : matching_nodes) {
        candidates.insert(candidates.end(), 
                         node->leaf_orders.begin(), 
                         node->leaf_orders.end());
    }
    
    // Filter by brand safety
    std::vector<StandingOrder> safe_candidates;
    for (const auto& order : candidates) {
        if (check_brand_safety(order, impression)) {
            safe_candidates.push_back(order);
        }
    }
    
    // Sort by bid price (descending)
    std::sort(safe_candidates.begin(), safe_candidates.end(),
              [](const StandingOrder& a, const StandingOrder& b) {
                  return a.bid_cppm > b.bid_cppm;
              });
    
    return safe_candidates;
}

std::vector<std::shared_ptr<ATreeNode>> ATreeMatcher::traverse(
    const ATree& root,
    const Impression& impression,
    const std::vector<DimensionType>& dimension_order) {
    
    std::vector<std::shared_ptr<ATreeNode>> current_nodes = {root};
    
    // Traverse each dimension level
    for (const auto& dimension : dimension_order) {
        std::vector<std::shared_ptr<ATreeNode>> next_nodes;
        
        for (const auto& node : current_nodes) {
            if (node->children.empty()) {
                // Already at leaf
                next_nodes.push_back(node);
                continue;
            }
            
            // Try to find matching children for this dimension
            std::string attr_value;
            
            switch (dimension) {
                case DimensionType::AGE_RANGE:
                    attr_value = impression.get_string_attr("age_range");
                    break;
                case DimensionType::INTEREST:
                    attr_value = impression.get_string_attr("interest");
                    break;
                case DimensionType::CONTENT_CATEGORY:
                    attr_value = impression.get_string_attr("content_category");
                    break;
                case DimensionType::CONTENT_EVENT:
                    attr_value = impression.get_string_attr("content_event");
                    break;
                case DimensionType::GEOGRAPHY:
                    attr_value = impression.get_string_attr("geography");
                    break;
                case DimensionType::TIME_OF_DAY:
                    attr_value = impression.get_string_attr("time_of_day");
                    break;
                case DimensionType::DEVICE_TYPE:
                    attr_value = impression.get_string_attr("device_type");
                    break;
                case DimensionType::CUSTOM:
                default:
                    continue;
            }
            
            if (!attr_value.empty()) {
                auto child = find_matching_child(node, dimension, attr_value);
                if (child) {
                    next_nodes.push_back(child);
                }
            }
        }
        
        current_nodes = next_nodes;
        if (current_nodes.empty()) break;
    }
    
    return current_nodes;
}

std::shared_ptr<ATreeNode> ATreeMatcher::find_matching_child(
    const std::shared_ptr<ATreeNode> parent,
    DimensionType dimension,
    const std::string& value) {
    
    for (const auto& child : parent->children) {
        if (child->value_range.contains(value)) {
            return child;
        }
    }
    return nullptr;
}

bool ATreeMatcher::check_brand_safety(const StandingOrder& order,
                                     const Impression& impression) {
    // Check excluded categories
    const auto& content_cat = impression.get_string_attr("content_category");
    for (const auto& excluded : order.brand_safety.excluded_categories) {
        if (content_cat == excluded) {
            return false;
        }
    }
    
    // Check age gate requirement
    if (order.brand_safety.require_age_gate) {
        const auto& age_range = impression.get_string_attr("age_range");
        if (age_range == "unknown") {
            return false;
        }
    }
    
    return true;
}

bool ATreeMatcher::check_frequency(const StandingOrder& order,
                                  const Impression& impression,
                                  const std::unordered_map<std::string, uint32_t>& frequency_map) {
    // Create frequency key: user_hash:campaign_id
    std::string freq_key = std::to_string(impression.user_hash) + ":" + order.campaign_id;
    
    auto it = frequency_map.find(freq_key);
    if (it == frequency_map.end()) {
        return true;  // No frequency data = allow
    }
    
    uint32_t current_count = it->second;
    
    // Check limits
    if (order.frequency_cap.hourly_limit > 0 && 
        current_count >= order.frequency_cap.hourly_limit) {
        return false;
    }
    
    if (order.frequency_cap.daily_limit > 0 && 
        current_count >= order.frequency_cap.daily_limit) {
        return false;
    }
    
    if (order.frequency_cap.weekly_limit > 0 && 
        current_count >= order.frequency_cap.weekly_limit) {
        return false;
    }
    
    return true;
}
