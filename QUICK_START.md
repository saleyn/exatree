# A-Tree Quick Start Guide

## 5-Minute Setup

### 1. Install Dependencies
```bash
cd /home/serge/projects/atree2
mix deps.get
```

### 2. Compile Project
```bash
mix compile
```
This will compile the C++ code and link the NIF library.

### 3. Verify Installation
```bash
iex -S mix
```

In the Elixir shell:
```elixir
{:ok, tree} = Atree.new()
# Should print: {:ok, #Reference<...>}
```

If you see the reference, you're good to go! ✅

---

## Basic Example (Copy-Paste Ready)

```elixir
# Start Elixir shell
# iex -S mix

# 1. Create a new empty tree
{:ok, tree} = Atree.new()

# 2. Create some orders
orders = [
  %{
    campaign_id: "nike_sports",
    bid_cpm: 45.50,
    attributes: %{
      age_range: "18-49",
      interest: "sports",
      content_category: "sports",
      geography: "US-EAST",
      time_of_day: "18:00-23:59",
      device_type: "tv"
    }
  },
  %{
    campaign_id: "gatorade_fitness",
    bid_cpm: 32.00,
    attributes: %{
      age_range: "25-34",
      interest: "fitness",
      content_category: "entertainment",
      geography: "NATIONAL"
    }
  },
  %{
    campaign_id: "apple_tech",
    bid_cpm: 55.75,
    attributes: %{
      age_range: "18-49",
      interest: "shopping",
      content_category: "news",
      geography: "US-WEST"
    }
  }
]

# 3. Insert orders into the tree
{:ok, tree} = Atree.insert_orders(tree, orders)

# 4. Create an impression (incoming request)
impression = %{
  age_range: "28",
  interest: "sports",
  content_category: "sports",
  geography: "US-EAST",
  time_of_day: "20:30",
  device_type: "tv"
}

# 5. Match the impression against the tree
{:ok, matched} = Atree.match(tree, impression)

# 6. See the results
matched |> Enum.each(fn order ->
  IO.puts("#{order.campaign_id} @ $#{Float.round(order.bid_cpm, 2)}")
end)

# Output:
# nike_sports @ $45.5

# 7. Get top 1 bidder
{:ok, top_1} = Atree.top_n(tree, impression, 1)

# 8. Filter by minimum bid price
{:ok, above_40} = Atree.filter_by_min_bid(tree, impression, 40.0)

# 9. Match multiple impressions at once
other_impressions = [
  %{age_range: "35", interest: "fitness"},
  %{age_range: "50", interest: "shopping"}
]
{:ok, batch_results} = Atree.batch_match(tree, other_impressions)
```

---

## Common Tasks

### Insert orders from a CSV
```elixir
orders = File.stream!("orders.csv")
  |> CSV.decode!(headers: true)
  |> Enum.map(fn row ->
    %{
      campaign_id: row["campaign_id"],
      bid_cpm: String.to_float(row["bid_cpm"]),
      attributes: %{
        age_range: row["age_range"],
        interest: row["interest"],
        content_category: row["content_category"]
      }
    }
  end)
  |> then(fn orders -> Atree.insert_orders(tree, orders) end)
```

### Run benchmarks
```elixir
# In iex shell:
Atree.Example.benchmark_basic(1000, 100)
# Shows performance metrics for 1000 orders, 100 impressions
```

### Print comprehensive example
```elixir
Atree.Example.example_comprehensive()
# Detailed walkthrough of all features
```

---

## Attribute Dimensions

When creating impressions or order attributes, use these dimension values:

**Age Range**: `"18-24"`, `"25-34"`, `"35-49"`, `"50-64"`, `"65+"`

**Interest**: `"sports"`, `"fitness"`, `"travel"`, `"shopping"`, `"entertainment"`, `"news"`

**Content Category**: `"sports"`, `"news"`, `"entertainment"`, `"adult"`, `"educational"`

**Content Event**: `"live-event"`, `"on-demand"`, `"sponsored"`, `"clip"`

**Geography**: `"US-EAST"`, `"US-WEST"`, `"MIDWEST"`, `"SOUTH"`, `"NATIONAL"`, `"EU"`

**Time of Day**: `"06:00-12:00"`, `"12:00-18:00"`, `"18:00-23:59"`

**Device Type**: `"tv"`, `"mobile"`, `"tablet"`, `"desktop"`

---

## Run Tests

```bash
# Run all tests
mix test

# Run with coverage
mix test --cover

# Run specific test file
mix test test/atree_test.exs

# Run with verbose output
mix test --verbose
```

---

## Development Commands

```bash
# Format code
mix format

# Check for unused imports
mix compile --warnings-as-errors

# Get dependencies
mix deps.get

# Clean build artifacts
mix clean

# Rebuild from scratch
mix clean && mix compile

# Rebuild C++ only
make -C c_src clean && mix compile
```

---

## Troubleshooting

**Problem**: "NIF not loaded"
```elixir
# Solution: Make sure you've compiled
# In terminal:
mix compile

# Then restart iex:
iex -S mix
```

**Problem**: "No such file or directory - priv/libatre.so"
```bash
# Solution: Rebuild
mix compile --force
```

**Problem**: Compilation errors in C++
```bash
# Check you have a C++ compiler:
gcc --version  # or clang --version

# Check Erlang is installed:
erl -eval "io:format('~s', [code:root_dir()])" -s init stop -noshell
```

**Problem**: Tests fail
```bash
# Run tests with verbose output:
mix test --verbose

# Check Elixir version (need 1.14+):
elixir --version

# Check Erlang version (need OTP 24+):
erl -eval "erlang:system_info(otp_release), halt(0)." -noshell
```

---

## Performance Notes

On a modern CPU with 1000 orders:
- **Insert operation**: ~0.1ms per order
- **Match operation**: ~0.5ms per impression
- **Memory**: ~2KB per order

For production use with 10k+ orders, expect ~5-10ms matching time.

---

## Next Steps

1. **Read full documentation**: See [README.md](README.md)
2. **Understand architecture**: See [ARCHITECTURE.md](ARCHITECTURE.md)
3. **Explore examples**: Run `Atree.Example.example_comprehensive()`
4. **Run benchmarks**: Use `Atree.Example.benchmark_basic()`
5. **Integrate into your project**: Use in your application code

---

## Key Files Reference

| File | Purpose |
|------|---------|
| `lib/atree.ex` | Main API - start here |
| `lib/atree_example.ex` | Usage examples & benchmarks |
| `README.md` | Full documentation |
| `ARCHITECTURE.md` | Technical deep dive |
| `c_src/atree.h` | C++ data structures |
| `c_src/atree.cpp` | C++ implementation |
| `test/atree_test.exs` | Test examples |

---

## Support

For issues or questions:
1. Check [README.md](README.md) FAQ section
2. Review [ARCHITECTURE.md](ARCHITECTURE.md) for deep technical details
3. Look at test cases in [test/atree_test.exs](test/atree_test.exs)
4. Run examples in [lib/atree_example.ex](lib/atree_example.ex)

Happy matching! 🌳
