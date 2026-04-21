#include "erl_nif.h"
#include "atree.h"
#include <cstring>
#include <cstdio>

// ============================================================================
// NIF Helper Functions & Macros
// ============================================================================

#define UNUSED(x) (void)(x)

static ErlNifResourceType* ATREE_RESOURCE_TYPE = nullptr;

// Resource data struct - simple wrapper around tree pointer
struct TreeResource {
    ATree tree;
    
    TreeResource() : tree(nullptr) {}
    ~TreeResource() {
        tree.reset();
    }
};

// ============================================================================
// Resource Cleanup Callbacks
// ============================================================================

static void tree_destructor(ErlNifEnv* env, void* obj) {
    UNUSED(env);
    TreeResource* res = static_cast<TreeResource*>(obj);
    res->~TreeResource();
}


// ============================================================================
// Atom Cache
// ============================================================================

static struct {
    ERL_NIF_TERM atom_ok;
    ERL_NIF_TERM atom_error;
    ERL_NIF_TERM atom_null_tree;
    ERL_NIF_TERM atom_bid_cpm;
    ERL_NIF_TERM atom_campaign_id;
} ATOMS;

// ============================================================================
// NIF Functions
// ============================================================================

/**
 * build() -> TreeRef
 * Create a new empty A-Tree
 */
static ERL_NIF_TERM nif_build(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    
    TreeResource* res = static_cast<TreeResource*>(
        enif_alloc_resource(ATREE_RESOURCE_TYPE, sizeof(TreeResource)));
    
    if (!res) {
        return enif_make_tuple2(env, ATOMS.atom_error,
                              enif_make_string(env, "malloc failed", ERL_NIF_LATIN1));
    }
    
    try {
        // Call placement new to initialize the struct
        new (res) TreeResource();
        
        // Build empty tree with default dimensions
        std::vector<DimensionType> dimensions;
        dimensions.push_back(DimensionType::AGE_RANGE);
        dimensions.push_back(DimensionType::INTEREST);
        dimensions.push_back(DimensionType::CONTENT_CATEGORY);
        dimensions.push_back(DimensionType::CONTENT_EVENT);
        dimensions.push_back(DimensionType::GEOGRAPHY);
        dimensions.push_back(DimensionType::TIME_OF_DAY);
        dimensions.push_back(DimensionType::DEVICE_TYPE);
        
        ATreeBuilder builder;
        res->tree = builder.build(dimensions);
        
        ERL_NIF_TERM term = enif_make_resource(env, res);
        enif_release_resource(res);
        
        return term;
    } catch (const std::exception& e) {
        enif_release_resource(res);
        return raise_error(env, e.what());
    }
}

/**
 * insert_order(TreeRef, CampaignId, BidCppm, AttrsList) -> TreeRef (raise exception on error)
 */
static ERL_NIF_TERM nif_insert_order(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
    if (argc != 4) return enif_make_badarg(env);
    
    TreeResource* tree_res = nullptr;
    if (!enif_get_resource(env, argv[0], ATREE_RESOURCE_TYPE, (void**)&tree_res)) {
        return enif_make_badarg(env);
    }
    
    if (!tree_res || !tree_res->tree) {
        return enif_make_tuple2(env, ATOMS.atom_error,
                              enif_make_string(env, "null tree", ERL_NIF_LATIN1));
    }
    
    // Extract campaign_id (as binary}
    ErlNifBinary campaign_bin;
    char campaign_id[256];
    if (enif_is_binary(env, argv[1])) {
        if (!enif_inspect_binary(env, argv[1], &campaign_bin)) {
            return enif_make_badarg(env);
        }
        if (campaign_bin.size >= sizeof(campaign_id)) {
            return enif_make_badarg(env);
        }
        memset(campaign_id, 0, sizeof(campaign_id));
        memcpy(campaign_id, campaign_bin.data, campaign_bin.size);
    } else {
        return enif_make_badarg(env);
    }
    
    // Extract bid_cppm
    double bid_double;
    if (!enif_get_double(env, argv[2], &bid_double)) {
        return enif_make_badarg(env);
    }
    
    // Extract attributes list
    std::map<std::string, std::string> attr_map;
    ERL_NIF_TERM attr_list = argv[3];
    
    ERL_NIF_TERM head, tail;
    tail = attr_list;
    
    while (enif_get_list_cell(env, tail, &head, &tail)) {
        const ERL_NIF_TERM* tuple;
        int arity;
        
        if (!enif_get_tuple(env, head, &arity, &tuple) || arity != 2) {
            return enif_make_badarg(env);
        }
        
        char key[128];
        char value[256];
        
        // Extract key as binary
        ErlNifBinary key_bin;
        if (!enif_inspect_binary(env, tuple[0], &key_bin)) {
            return enif_make_badarg(env);
        }
        if (key_bin.size >= sizeof(key)) {
            return enif_make_badarg(env);
        }
        memset(key, 0, sizeof(key));
        memcpy(key, key_bin.data, key_bin.size);
        
        // Extract value as binary
        ErlNifBinary value_bin;
        if (!enif_inspect_binary(env, tuple[1], &value_bin)) {
            return enif_make_badarg(env);
        }
        if (value_bin.size >= sizeof(value)) {
            return enif_make_badarg(env);
        }
        memset(value, 0, sizeof(value));
        memcpy(value, value_bin.data, value_bin.size);
        
        attr_map[key] = value;
    }
    
    try {
        StandingOrder order;
        order.campaign_id = campaign_id;
        order.bid_cppm = static_cast<float>(bid_double);
        
        ATreeBuilder builder;
        builder.insert_order(tree_res->tree, order, attr_map);
        
        // Return the same tree reference (tree is modified in place)
        return argv[0];
    } catch (const std::exception& e) {
        return raise_error(env, e.what());
    }
}

/**
 * match(TreeRef, ImpressionMap) -> [Orders] (raise exception on error)
 */
static ERL_NIF_TERM nif_match(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
    if (argc != 2) return enif_make_badarg(env);
    
    TreeResource* tree_res = nullptr;
    if (!enif_get_resource(env, argv[0], ATREE_RESOURCE_TYPE, (void**)&tree_res))
        return enif_make_badarg(env);
    
    if (!tree_res || !tree_res->tree)
        return raise_error(env, atom_null_tree);
    
    // Extract impression map
    Impression impression;
    ERL_NIF_TERM map = argv[1];
    
    ErlNifMapIterator iter;
    if (enif_map_iterator_create(env, map, &iter, ERL_NIF_MAP_ITERATOR_FIRST)) {
        ERL_NIF_TERM key, value;
        
        while (enif_map_iterator_get_pair(env, &iter, &key, &value)) {
            char key_str[64];
            char val_str[256];
            
            // Extract key as binary
            ErlNifBinary key_bin;
            if (enif_inspect_binary(env, key, &key_bin) && key_bin.size < sizeof(key_str)) {
                memset(key_str, 0, sizeof(key_str));
                memcpy(key_str, key_bin.data, key_bin.size);
                
                // Try to extract value as binary
                ErlNifBinary val_bin;
                if (enif_inspect_binary(env, value, &val_bin) && val_bin.size < sizeof(val_str)) {
                    memset(val_str, 0, sizeof(val_str));
                    memcpy(val_str, val_bin.data, val_bin.size);
                    impression.set_string_attr(key_str, val_str);
                } else {
                    // Try to extract value as double
                    double dval;
                    if (enif_get_double(env, value, &dval)) {
                        impression.set_float_attr(key_str, static_cast<float>(dval));
                    }
                }
            }
            
            enif_map_iterator_next(env, &iter);
        }
        enif_map_iterator_destroy(env, &iter);
    }
    
    try {
        std::vector<DimensionType> dimension_order = {
            DimensionType::AGE_RANGE,
            DimensionType::INTEREST,
            DimensionType::CONTENT_CATEGORY,
            DimensionType::CONTENT_EVENT,
            DimensionType::GEOGRAPHY,
            DimensionType::TIME_OF_DAY,
            DimensionType::DEVICE_TYPE
        };
        
        ATreeMatcher matcher;
        auto matched_orders = matcher.match(tree_res->tree, impression, dimension_order);
        
        // Convert results to Erlang list of maps
        std::vector<ERL_NIF_TERM> order_terms;
        
        for (const auto& order : matched_orders) {
            if (order_terms.size() >= 100) break;
            
            ERL_NIF_TERM order_map = enif_make_new_map(env);
            
            enif_make_map_put(env, order_map,
                            atom_campaign_id,
                            enif_make_string(env, order.campaign_id.c_str(), ERL_NIF_LATIN1),
                            &order_map);
            
            enif_make_map_put(env, order_map,
                            atom_bid_cpm,
                            enif_make_double(env, order.bid_cppm),
                            &order_map);
            
            order_terms.push_back(order_map);
        }
        
        return order_terms.empty()
             ? enif_make_list(env, 0)
             : enif_make_list_from_array(env, order_terms.data(), order_terms.size())
    } catch (const std::exception& e) {
        return raise_error(env, e.what());
    }
}

// ============================================================================
// NIF Module Definition
// ============================================================================

static ErlNifFunc nif_funcs[] = {
    {"build",        1, nif_build},
    {"insert_order", 4, nif_insert_order},
    {"match",        2, nif_match},
};

static int on_load(ErlNifEnv* env, void** priv_data, ERL_NIF_TERM load_info) {
    UNUSED(priv_data);
    UNUSED(load_info);
    
    // Create resource type
    ATREE_RESOURCE_TYPE = enif_open_resource_type(
        env, nullptr, "atree", tree_destructor, ERL_NIF_RT_CREATE, nullptr);
    
    if (!ATREE_RESOURCE_TYPE)
        return 1;
    
    // Cache atoms
    ATOMS.atom_ok          = enif_make_atom(env, "ok");
    ATOMS.atom_error       = enif_make_atom(env, "error");
    ATOMS.atom_null_tree   = enif_make_atom(env, "null_tree");
    ATOMS.atom_campaign_id = enif_make_atom(env, "campaign_id");
    ATOMS.atom_bid_cpm     = enif_make_atom(env, "bid_cpm");
    
    return 0;
}

ERL_NIF_INIT(Elixir.Atree.Native, nif_funcs, on_load, nullptr, nullptr)
