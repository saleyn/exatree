defmodule Atree.Example do
  @moduledoc """
  Example usage and performance benchmarks for A-Tree library.
  """

  require Logger

  @doc       """
  Generate sample orders for demonstration.
  """
  def generate_sample_orders(count \\ 100) do
    age_ranges  = ["18-24", "25-34", "35-49", "50-64", "65+"]
    interests   = ["sports", "fitness", "travel", "shopping", "entertainment", "news"]
    categories  = ["sports", "news", "entertainment", "adult", "educational"]
    events      = ["live-event", "on-demand", "sponsored", "clip"]
    geographies = ["US-EAST", "US-WEST", "MIDWEST", "SOUTH", "NATIONAL"]
    times       = ["06:00-12:00", "12:00-18:00", "18:00-23:59"]
    devices     = ["tv", "mobile", "tablet"]

    for i <- 1..count do
      %{
        campaign_id: "campaign-#{i}",
        bid_cpm:     10.0 + :rand.uniform(40) + :rand.uniform(100) / 100,
        attributes:  %{
          age_range:        Enum.random(age_ranges),
          interest:         Enum.random(interests),
          content_category: Enum.random(categories),
          content_event:    Enum.random(events),
          geography:        Enum.random(geographies),
          time_of_day:      Enum.random(times),
          device_type:      Enum.random(devices)
        },
        frequency_cap: %{
          hourly_limit: Enum.random(1..5),
          daily_limit:  Enum.random(10..30)
        },
        brand_safety: %{
          excluded_categories: if(:rand.uniform() < 0.3, do: ["adult"], else: []),
          excluded_keywords:   [],
          require_age_gate:    :rand.uniform() < 0.1
        }
      }
    end
  end

  @doc """
  Generate sample impressions for matching.
  """
  def generate_sample_impressions(count \\ 10) do
    age_ranges  = ["18-24", "25-34", "35-49", "50-64", "65+"]
    interests   = ["sports", "fitness", "travel", "shopping", "entertainment"]
    categories  = ["sports", "news", "entertainment"]
    events      = ["live-event", "on-demand", "sponsored"]
    geographies = ["US-EAST", "US-WEST", "MIDWEST"]
    times       = ["06:00-12:00", "12:00-18:00", "18:00-23:59"]
    devices     = ["tv", "mobile", "tablet"]

    for i <- 1..count do
      %{
        user_hash:        i,
        age_range:        Enum.random(age_ranges),
        interest:         Enum.random(interests),
        content_category: Enum.random(categories),
        content_event:    Enum.random(events),
        geography:        Enum.random(geographies),
        time_of_day:      Enum.random(times),
        device_type:      Enum.random(devices)
      }
    end
  end

  @doc """
  Simple benchmark: Create tree, insert orders, match impressions.
  """
  def benchmark_basic(order_count \\ 1000, impression_count \\ 100) do
    Logger.info("===== A-Tree Benchmark =====")
    Logger.info("Orders: #{order_count}, Impressions: #{impression_count}")

    # Generate test data
    Logger.info("\nGenerating test data...")
    orders      = generate_sample_orders(order_count)
    impressions = generate_sample_impressions(impression_count)

    # Create and populate tree
    Logger.info("Creating tree...")
    start       = System.monotonic_time(:millisecond)
    tree        = Atree.new() |> Atree.insert_orders(orders)
    insert_time = System.monotonic_time(:millisecond) - start

    Logger.info(
      "Insert #{order_count} orders: #{insert_time}ms avg #{insert_time / order_count}ms/order"
    )

    # Match impressions
    Logger.info("\nMatching #{impression_count} impressions...")
    start = System.monotonic_time(:millisecond)
    results = Enum.map(impressions, fn imp -> Atree.match(tree, imp) end)
    match_time = System.monotonic_time(:millisecond) - start

    Logger.info(
      "Match #{impression_count} impressions: #{match_time}ms avg #{match_time / impression_count}ms/impression"
    )

    total_matches =  # Analyze results
      results
      |> Enum.map(fn orders -> length(orders) end)
      |> Enum.sum()

    Logger.info("Total matches: #{total_matches}")

    Logger.info(
      "Avg matches per impression: #{(total_matches / impression_count) |> Float.round(2)}"
    )

    %{
      order_count:                order_count,
      impression_count:           impression_count,
      insert_time_ms:             insert_time,
      match_time_ms:              match_time,
      total_matches:              total_matches,
      avg_matches_per_impression: total_matches / impression_count
    }
  end

  @doc """
  Detailed example showing all library features.
  """
  def example_comprehensive do
    Logger.info("===== A-Tree Comprehensive Example =====\n")

    # 1. Create tree
    Logger.info("1. Creating A-Tree...")
    tree   = Atree.new()
    Logger.info("   ✓ Tree created\n")

    # 2. Define orders
    Logger.info("2. Defining orders...")

    orders = [
      %{
        campaign_id: "nike-sports-live",
        bid_cpm:     45.50,
        attributes:  %{
          age_range:        "18-49",
          interest:         "sports",
          content_category: "sports",
          content_event:    "live-event",
          geography:        "US-EAST",
          time_of_day:      "18:00-23:59",
          device_type:      "tv"
        },
        frequency_cap: %{hourly_limit: 3, daily_limit: 20},
        brand_safety:  %{required_age_gate: false, excluded_categories: []}
      },
      %{
        campaign_id: "gatorade-fitness",
        bid_cpm:     32.00,
        attributes:  %{
          age_range:        "25-34",
          interest:         "fitness",
          content_category: "entertainment",
          geography:        "NATIONAL"
        },
        frequency_cap: %{hourly_limit: 5, daily_limit: 30}
      },
      %{
        campaign_id: "apple-tech",
        bid_cpm:     55.75,
        attributes:  %{
          age_range:        "18-49",
          interest:         "shopping",
          content_category: "news",
          geography:        "US-WEST"
        },
        frequency_cap: %{daily_limit: 15}
      }
    ]

    Logger.info("   ✓ #{length(orders)} orders defined\n")

    # 3. Insert orders
    Logger.info("3. Inserting orders into tree...")
    tree       = Atree.insert_orders(tree, orders)
    Logger.info("   ✓ All orders inserted\n")

    # 4. Create test impression
    Logger.info("4. Matching impression...")

    impression = %{
      age_range:        "28",
      interest:         "sports",
      content_category: "sports",
      content_event:    "live-event",
      geography:        "US-EAST",
      time_of_day:      "20:30",
      device_type:      "tv"
    }

    Logger.info("   Impression: #{inspect(impression)}\n")

    matched = Atree.match(tree, impression) # 5. Match and display results
    Logger.info("5. Matching results:")
    Logger.info("   Found #{length(matched)} matching orders\n")

    Enum.each(matched, fn order -> Logger.info("   - #{Atree.format_order(order)}") end)

    Logger.info("\n6. Advanced queries:")

    top_1       = Atree.top_n(tree, impression, 1)                # Top N
    Logger.info("   Top 1 bid: #{Atree.format_order(hd(top_1))}")

    above_40    = Atree.filter_by_min_bid(tree, impression, 40.0) # Min bid filter
    Logger.info("   Orders >= $40 CPM: #{length(above_40)} orders")

    impressions = [                                               # Batch matching
      %{age_range: "25", interest: "sports", content_category: "sports"},
      %{age_range: "35", interest: "fitness", content_category: "entertainment"}
    ]

    batch_results = Atree.batch_match(tree, impressions)

    Logger.info(
      "   Batch match (#{length(impressions)} impressions): #{length(batch_results)} results"
    )

    Logger.info("\n✓ Example completed successfully!")
  end
end
