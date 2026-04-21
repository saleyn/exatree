# A-Tree Architecture Documentation

## Overview

A-Tree is a multi-dimensional attribute indexing library optimized for real-time standing order matching. It combines:

1. **C++ Core** - High-performance tree operations
2. **Erlang NIF Interface** - Seamless Erlang/Elixir integration
3. **Elixir API** - User-friendly high-level interface

## System Architecture

```
┌─────────────────────────────────────────┐
│        Elixir Application               │
│    (lib/atree.ex - High-level API)      │
└────────────────┬────────────────────────┘
                 │ Erlang function calls
┌────────────────▼────────────────────────┐
│    NIF Interface Layer                  │
│  (c_src/atree_nif.cpp)                  │
│  - Resource management                  │
│  - Term serialization/deserialization   │
│  - Error handling                       │
└────────────────┬────────────────────────┘
                 │ C++ function calls
┌────────────────▼────────────────────────┐
│    A-Tree Core (C++)                    │
│  (c_src/atree.h, atree.cpp)             │
│  - Tree data structures                 │
│  - Builder (insert)                     │
│  - Matcher (search)                     │
│  - Brand safety evaluation              │
│  - Frequency cap checking               │
└─────────────────────────────────────────┘
```

## Component Details

### 1. Elixir API Layer (`lib/atree.ex`)

**Purpose**: Provide a safe, user-friendly Erlang/Elixir interface

**Key Functions**:
- `new()` - Create new tree
- `insert_order(tree, order)` - Add single order
- `insert_orders(tree, orders)` - Batch insert
- `match(tree, impression)` - Find matching orders
- `batch_match(tree, impressions)` - Batch matching
- `top_n(tree, impression, n)` - Get top N results
- `filter_by_min_bid(tree, impression, min_bid)` - Price filter

**Type Safety**:
- Uses Elixir type specs for compile-time checking
- Validates input at Elixir level before passing to NIF

### 2. NIF Interface (`c_src/atree_nif.cpp`)

**Purpose**: Bridge between Erlang/Elixir and C++

**Key Responsibilities**:
- Manage Erlang resource types
- Serialize/deserialize Erlang terms to C++ objects
- Handle errors and convert to Erlang exceptions
- Manage memory and garbage collection

**NIF Functions**:
1. `builder_new() -> {ok, BuilderRef}`
2. `build(BuilderRef) -> {ok, TreeRef}`
3. `insert_order(TreeRef, campaign_id, bid, attrs) -> {ok, TreeRef}`
4. `matcher_new() -> {ok, MatcherRef}`
5. `match(TreeRef, MatcherRef, impression) -> {ok, [Orders]}`

**Resource Types**:
- `atree` - A-Tree root node
- `atree_builder` - Builder instance
- `atree_matcher` - Matcher instance

### 3. C++ Core (`c_src/atree.h` & `c_src/atree.cpp`)

**Data Structures**:

#### ATreeNode
```cpp
struct ATreeNode {
    DimensionType dimension;        // Type of this level
    ValueRange value_range;         // Min/max or discrete values
    std::vector<ATreeNode> children;  // Child nodes
    std::vector<StandingOrder> leaf_orders;  // Orders at leaf
};
```

#### StandingOrder
```cpp
struct StandingOrder {
    std::string campaign_id;
    float bid_cppm;
    int64_t daily_budget_remaining;
    FrequencyCap frequency_cap;
    BrandSafety brand_safety;
};
```

#### Impression
```cpp
struct Impression {
    std::unordered_map<> string_attrs;
    std::unordered_map<> int_attrs;
    std::unordered_map<> float_attrs;
    uint64_t user_hash;
};
```

**Key Classes**:

#### ATreeBuilder
- Constructs tree structure
- Inserts standing orders at correct leaf positions
- Maintains dimension hierarchy

#### ATreeMatcher
- Traverses tree following impression attributes
- Filters by brand safety rules
- Checks frequency caps
- Sorts results by bid price

## Data Flow

### Insertion Flow

```
Elixir: Atree.insert_order(tree, order)
  ↓
NIF: nif_insert_order(erlang_tree, erlang_order_data)
  ↓
C++: builder.insert_order(cpp_tree, cpp_order, attrs)
  ↓
Tree Structure: Navigate dimensions → Reach leaf → Insert order
  ↓
Return: Updated tree reference back to Erlang
```

### Matching Flow

```
Elixir: Atree.match(tree, impression)
  ↓
NIF: nif_match(erlang_tree, erlang_impression)
  ↓
C++: matcher.match(cpp_tree, cpp_impression, dimension_order)
  ↓
Tree Traversal:
  For each dimension in order:
    Find matching children
    Prune branches that don't match
  ↓
Collect Standing Orders: Gather all orders from leaf nodes
  ↓
Filter:
  1. Brand safety check
  2. Frequency cap evaluation
  ↓
Sort: By bid_cppm descending
  ↓
Return: List of matching orders as Erlang terms
```

## Dimension Hierarchy

The A-Tree uses a fixed dimension order for efficient traversal:

1. **Age Range** - Coarse demographic filter
2. **Interest** - User interest category
3. **Content Category** - Content type classification
4. **Content Event** - Event type (live/on-demand/etc)
5. **Geography** - Geographic region
6. **Time-of-Day** - Time bracket
7. **Device Type** - Device classification

This order is optimized for:
- High selectivity early (age narrows significantly)
- Progressive refinement (interest within age, etc.)
- Cache efficiency (spatial locality)

## Performance Optimization Strategies

### 1. Early Pruning
- Dimensions with high selectivity come first
- Empty branches are skipped immediately

### 2. Lazy Evaluation
- Frequency caps evaluated only on final candidates
- Brand safety checked after tree traversal

### 3. Memory Layout
- Orders stored contiguously in leaf nodes
- Siblings in same subtree → cache-friendly access

### 4. Type Safety
- C++ type checking at compile time
- Erlang guard clauses validate imports

## Extension Points

### Adding Custom Dimensions

1. Add to `DimensionType` enum:
```cpp
enum class DimensionType {
    // existing...
    CUSTOM_DIM,  // your new dimension
};
```

2. Update `traverse()` method:
```cpp
case DimensionType::CUSTOM_DIM:
    attr_value = impression.get_string_attr("custom_dim");
    break;
```

3. Expose in Elixir API

### Adding Frequency Cap Types

Add to `FrequencyCap` struct:
```cpp
struct FrequencyCap {
    uint32_t hourly_limit;
    uint32_t daily_limit;
    uint32_t weekly_limit;
    uint32_t monthly_limit;  // new
};
```

Update frequency checking logic accordingly.

## Error Handling

### C++ Level
- Exceptions caught and converted to error terms
- Resource cleanup guaranteed via destructors

### NIF Level
- Erlang exceptions for invalid arguments
- Proper resource deallocation on error

### Elixir Level
- Pattern matching on `{:ok, result}` and `{:error, reason}`
- Logger integration for debugging

## Testing Strategy

### Unit Tests (`test/atree_test.exs`)
- Tree creation and insertion
- Single and batch matching
- Utility function correctness
- Error handling

### Integration Tests
- Multi-threaded access patterns
- Large dataset handling (10k+ orders)

### Benchmarks
- Insertion rate: orders/millisecond
- Matching rate: impressions/millisecond
- Memory usage per order

## Build Process

1. **Elixir Compilation**
   - `mix compile` triggers Makefile
   
2. **Makefile Execution**
   - Detects platform (Linux/macOS/Windows)
   - Compiles C++ with appropriate flags
   - Links against Erlang NIF library
   - Outputs shared object (`priv/libatre.so`)

3. **NIF Loading**
   - `Atree.Native` module loads `.so` at runtime
   - Fallback error if compilation failed

## Thread Safety

- **A-Tree instances are immutable** after creation
- Each insert returns a new tree reference
- Safe for concurrent reads (no locks needed)
- Updates use functional data structure approach

## Future Optimization Opportunities

1. **Memory Pooling** - Allocate node chunks upfront
2. **Parallel Matching** - SIMD operations for dimension filtering
3. **Bloom Filters** - Pre-filter candidate sets
4. **Caching** - LRU cache for frequent dimension values
5. **Compression** - Dictionary encoding for dimension values
