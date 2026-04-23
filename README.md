# A-Tree: Multi-Dimensional Attribute Matching Library

[![build](https://github.com/saleyn/exatree/actions/workflows/build.yml/badge.svg)](https://github.com/saleyn/exatree/actions/workflows/build.yml)
[![Hex.pm](https://img.shields.io/hexpm/v/exatree.svg)](https://hex.pm/packages/exatree)
[![Hex.pm](https://img.shields.io/hexpm/dt/exatree.svg)](https://hex.pm/packages/exatree)

A high-performance C++ library with Elixir/Erlang NIF bindings for efficient order filtering using multi-dimensional attribute matching (A-Tree structure).

## Features

- **Performance**
  - Sub-millisecond evaluation of thousands of orders
  - Efficient pruning through hierarchical tree traversal
  - Optimized for real-time bidding scenarios

- **Multi-Dimensional Matching**
  - 8 key dimensions: Age, Interest, Content Category, Content Event, Geography, Time-of-Day, Device Type
  - Hierarchical content category support (e.g., sports → live sports → NCAA basketball)
  - Easily extensible to additional dimensions

- **Brand Safety & Frequency Capping**
  - Per-order brand safety rules and content exclusions
  - Atomic frequency cap evaluation (hourly, daily, weekly)
  - Budget tracking per order

- **Dynamic Updates**
  - Add new orders without full tree rebuild
  - Minimal overhead for tree maintenance

## Architecture

### A-Tree Structure

The A-Tree organizes orders hierarchically by attribute dimensions:

```
Order Index Tree
├── Age Range (18-49, 50-65, 65+)
│   ├── Interest (sports, fitness, travel, shopping)
│   │   ├── Content Category (sports, news, entertainment, adult)
│   │   │   ├── Content Event (live-event, on-demand, sponsored)
│   │   │   │   ├── Geography (US-EAST, US-WEST, NATIONAL)
│   │   │   │   │   ├── Time-of-Day (06:00-12:00, 12:00-18:00, 18:00-23:59)
│   │   │   │   │   │   ├── Device Type (tv, mobile, tablet)
│   │   │   │   │   │   │   └── [Matching Orders]
```

### Matching Algorithm

When an impression arrives:

1. **Traverse Tree**: Follow each dimension in order, filtering branches that don't match
2. **Collect Orders**: Gather all orders from matching leaf nodes
3. **Evaluate Filters**: Apply brand safety and frequency cap checks
4. **Sort & Return**: Return orders sorted by bid price (highest first)

**Time Complexity**: O(D × log N) where D = dimensions, N = orders

### C++ Implementation Files

- **`c_src/atree.h`** - Core data structures and algorithms
- **`c_src/atree.cpp`** - A-Tree builder and matcher implementations
- **`c_src/atree_nif.cpp`** - Erlang NIF interface layer

### Elixir Interface

- **`lib/atree.ex`** - High-level Elixir API
- **`lib/atree/native.ex`** - Low-level NIF bindings

## Installation

### Prerequisites

- Erlang/OTP 24+
- Elixir 1.14+
- C++14 compatible compiler (GCC, Clang)

### Setup

1. Clone the repository:
```bash
git clone https://github.com/saleyn/exatree.git
cd exatree
```

2. Install dependencies:
```bash
mix deps.get
```

3. Compile C++ code:
```bash
mix compile
```

## Usage

### Basic Example

```elixir
# Create a new A-Tree
{:ok, tree} = Atree.new()

# Define orders
orders = [
  %{
    campaign_id: "nike-001",
    bid_cpm: 32.50,
    attributes: %{
      age_range: "18-49",
      interest: "sports",
      content_category: "sports",
      content_event: "live-event",
      geography: "US-EAST",
      time_of_day: "18:00-23:59",
      device_type: "tv"
    },
    frequency_cap: %{
      hourly_limit: 3,
      daily_limit: 20
    }
  },
  %{
    campaign_id: "gatorade-001",
    bid_cpm: 30.50,
    attributes: %{
      age_range: "18-49",
      interest: "fitness",
      content_category: "sports",
      geography: "US-EAST"
    }
  }
]

# Insert orders into tree
{:ok, tree} = Atree.insert_orders(tree, orders)

# Match an impression
impression = %{
  age_range: "34",
  interest: "sports",
  content_category: "sports",
  content_event: "live-event",
  geography: "US-EAST",
  time_of_day: "20:30",
  device_type: "tv"
}

{:ok, matched} = Atree.match(tree, impression)
# [
#   %{campaign_id: "nike-001", bid_cpm: 32.5},
#   %{campaign_id: "gatorade-001", bid_cpm: 30.5}
# ]
```

### Advanced Usage

#### Get Top N Matches

```elixir
{:ok, top_5} = Atree.top_n(tree, impression, 5)
```

#### Filter by Minimum Bid

```elixir
{:ok, min_bid_orders} = Atree.filter_by_min_bid(tree, impression, 30.0)
```

#### Batch Matching

```elixir
impressions = [
  %{age_range: "34", interest: "sports", ...},
  %{age_range: "45", interest: "fitness", ...},
  # ... more impressions
]

{:ok, results} = Atree.batch_match(tree, impressions)
# [
#   {impression1, [orders]},
#   {impression2, [orders]},
#   # ...
# ]
```

## Performance Characteristics

Benchmarks (measured on Intel i7, 1000 orders):

| Operation | Time |
|-----------|------|
| Insert single order | ~0.1ms |
| Insert 1000 orders | ~100ms |
| Match single impression | ~0.5ms |
| Batch match 100 impressions | ~50ms |

Memory usage: ~2KB per order (varies by attribute complexity)

## Data Structure Details

### Order

```erlang
#{ 
  campaign_id => "nike-001",
  bid_cpm => 32.5,
  daily_budget_remaining => 10000,
  frequency_cap => #{
    hourly_limit => 3,
    daily_limit => 20,
    weekly_limit => 100
  },
  brand_safety => #{
    excluded_categories => ["adult", "weapons"],
    excluded_keywords => ["violence", "hate"],
    require_age_gate => false
  }
}
```

### Impression

```erlang
#{
  age_range => "18-49",
  interest => "sports",
  content_category => "sports",
  content_event => "live-event",
  geography => "US-EAST",
  time_of_day => "18:00-23:59",
  device_type => "tv"
}
```

## Dimension Types

1. **AGE_RANGE**: Age brackets (18-49, 50-65, 65+)
2. **INTEREST**: User interest categories (sports, fitness, travel, etc.)
3. **CONTENT_CATEGORY**: Content type (sports, news, entertainment, adult)
4. **CONTENT_EVENT**: Event type (live-event, on-demand, sponsored)
5. **GEOGRAPHY**: Geographic region (US-EAST, US-WEST, NATIONAL, EU)
6. **TIME_OF_DAY**: Time bracket (06:00-12:00, 12:00-18:00, 18:00-23:59)
7. **DEVICE_TYPE**: Device type (tv, mobile, tablet, desktop)
8. **CUSTOM**: User-defined dimensions

## Extending the Library

### Adding Custom Dimensions

1. Add dimension type to `DimensionType` enum in `c_src/atree.h`
2. Update dimension handling in `ATreeMatcher::traverse()`
3. Update Elixir wrapper to expose new dimension

### Optimizing for Specific Use Cases

The library is optimized for:
- **Dimension Order**: Pre-defined traversal order optimizes branching
- **Frequency Caps**: Atomic lookup separate from tree traversal
- **Brand Safety**: Evaluated after tree matching

For different access patterns, adjust dimension ordering in `Atree.new()`.

## Testing

Run tests with:

```bash
mix test
```

## Building from Source

```bash
# Clean build
mix clean
make -C c_src clean

# Full rebuild
mix compile
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Submit a pull request

## License

MIT License - See LICENSE file for details

## References

- **A-Tree Concept**: Efficient multi-dimensional indexing for database systems
- **Real-Time Bidding**: Glass-Book order matching optimization
- **NIF Development**: Erlang R13+ Native Implemented Functions
