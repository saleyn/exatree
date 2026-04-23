defmodule Atree.Dev.Guide do
  @moduledoc """
  Development and troubleshooting guide for A-Tree library.
  """

  @doc       """
  Quick reference for common operations.
  """
  def quick_reference do
    """
    ╔═══════════════════════════════════════════════════════════════════════════╗
    ║                      A-Tree Quick Reference Guide                         ║
    ╚═══════════════════════════════════════════════════════════════════════════╝

    COMPILATION
    ───────────
    mix deps.get       # Install dependencies
    mix compile        # Compile C++ and Elixir code
    mix clean          # Clean all artifacts

    TESTING
    ───────
    mix test                    # Run all tests
    mix test test/atree_test.exs  # Run specific test file
    mix test --cover            # Run with coverage

    BASIC USAGE
    ──────────
    tree  = Atree.new() # Create a tree

    order = %{          # Insert a single order
      campaign_id: "campaign-1",
      bid_cpm:     32.50,
      attributes:  %{
        age_range:        "18-49",
        interest:         "sports",
        content_category: "sports"
      }
    }
    tree       = Atree.insert_order(tree, order)

    tree       = Atree.insert_orders(tree, [order1, order2, order3]) # Insert multiple orders

    impression = %{                                                  # Match impression
      age_range:        "25",
      interest:         "sports",
      content_category: "sports"
    }
    matched_orders = Atree.match(tree, impression)

    top_5          = Atree.top_n(tree, impression, 5)                # Get top 5 results

    above_30       = Atree.filter_by_min_bid(tree, impression, 30.0) # Filter by minimum bid

    impressions    = [imp1, imp2, imp3]                              # Batch match multiple impressions
    results        = Atree.batch_match(tree, impressions)

    PERFORMANCE TIPS
    ────────────────
    1. Use batch operations when possible
    2. Reuse tree instances across requests
    3. Keep impressions simple (only needed dimensions)
    4. Consider impression caching for repeated patterns
    5. Monitor order count growth (rebuild if > 100k)

    DEBUGGING
    ────────
    # Enable logging
    Logger.configure([level: :debug])

    # Check if NIF loaded properly
    catch :error, _ -> IO.puts "NIF not loaded"

    # Print tree statistics
    tree_stats(tree)

    # Benchmark performance
    Atree.Example.benchmark_basic(1000, 100)

    DIMENSION REFERENCE
    ───────────────────
    Dimension          | Example Values
    ───────────────────┼────────────────────────────────
    age_range          | "18-24", "25-34", "35-49", "50-64", "65+"
    interest           | "sports", "fitness", "travel", "shopping"
    content_category   | "sports", "news", "entertainment", "adult"
    content_event      | "live-event", "on-demand", "sponsored", "clip"
    geography          | "US-EAST", "US-WEST", "MIDWEST", "SOUTH", "NATIONAL"
    time_of_day        | "06:00-12:00", "12:00-18:00", "18:00-23:59"
    device_type        | "tv", "mobile", "tablet", "desktop"

    BUILD TROUBLESHOOTING
    ────────────────────
    Problem: NIF won't load
    Solution: Check priv/libatre.so exists
             Run: ls -la priv/

    Problem: Compilation errors
    Solution: Check C++ compiler installed
             Run: gcc --version or clang --version
             Check Erlang headers: erl -eval "code:root_dir()."

    Problem: Test failures
    Solution: Run individual test
             Try: mix test test/atree_test.exs --verbose
             Check: Erlang/OTP version >= 24

    ACCESSING NATIVE LIBRARY
    ────────────────────────
    From Elixir:
      {:ok, tree} = Atree.new()  # Calls NIF automatically

    From NIF directly (advanced):
      Atree.Native.build(nil)         # Low-level
      Atree.Native.insert_order(...)  # Low-level
      Atree.Native.match(...)         # Low-level

    EXTENDING THE LIBRARY
    ─────────────────────
    Adding custom dimensions:
      1. Edit c_src/atree.h - add to DimensionType enum
      2. Edit c_src/atree.cpp - update traverse() method
      3. Recompile: mix compile

    Adding frequency cap types:
      1. Edit c_src/atree.h - update FrequencyCap struct
      2. Edit c_src/atree.cpp - update check_frequency()
      3. Recompile: mix compile

    PERFORMANCE PROFILING
    ─────────────────────
    Run benchmarks:
      iex) Atree.Example.benchmark_basic(1000, 100)

    Expected results (Intel i7):
      - Insert 1000 orders: ~100ms (0.1ms each)
      - Match 1 impression: ~0.5ms
      - Memory per order: ~2KB

    MEMORY LIMITS
    ─────────────
    Safe operating range:
      - Orders: < 100,000
      - Impressions/sec: < 10,000
      - Memory: < 200MB per tree
    """
  end

  @doc """
  Print development guide
  """
  def print_guide do
    IO.puts(quick_reference())
  end
end

# Uncomment to print guide:
# Atree.Dev.Guide.print_guide()