defmodule AtreeBenchmark do
  @moduledoc """
  Performance benchmark for A-Tree with 10,000 orders and 1,000 impressions.

  This test measures:
  - Time to insert 10,000 orders into the tree
  - Time to match 1,000 impressions
  - Success rate (matched vs unmatched impressions)
  - Speed (matches per microsecond)
  """

# Age ranges - expanded for more diversity in matches
  @age_ranges ["18-34", "35-49", "50+"]

  def run do
    IO.puts("\n" <> String.duplicate("=", 70))
    IO.puts("A-Tree Performance Benchmark")
    IO.puts(String.duplicate("=", 70))

    # Create tree
    tree = Atree.new()
    IO.puts("\n✓ Created empty A-Tree")

    # Generate and insert orders
    IO.puts("\nGenerating and inserting 10,000 orders...")
    start_insert = System.monotonic_time(:microsecond)

    orders = generate_orders(10_000)

    tree = Atree.insert_orders(tree, orders)

    end_insert = System.monotonic_time(:microsecond)
    insert_time = end_insert - start_insert

    IO.puts("✓ Inserted 10,000 orders in #{format_number(insert_time)} μs")
    IO.puts("  (~#{Float.round(insert_time / 10_000, 2)} μs per order)")
    IO.puts("\nGenerating 1,000 test impressions...")
    impressions = generate_impressions(1_000)

    # Benchmark matching
    IO.puts("\nMatching impressions against tree...")
    start_match = System.monotonic_time(:microsecond)

    results = match_all(tree, impressions)

    end_match = System.monotonic_time(:microsecond)
    match_time = end_match - start_match

    # Calculate statistics
    matched_count = Enum.count(results, fn r -> r > 0 end)
    unmatched_count = Enum.count(results, fn r -> r == 0 end)
    total_matches = Enum.sum(results)

    IO.puts("\n" <> String.duplicate("-", 70))
    IO.puts("Benchmark Results")
    IO.puts(String.duplicate("-", 70))

    IO.puts("Matched Impressions (> 0 orders):  #{matched_count}")
    IO.puts("Unmatched Impressions (no orders): #{unmatched_count}")
    IO.puts("Total matches found:               #{format_number(total_matches)}")
    IO.puts("Total time:                        #{:io_lib.format("~.3f", [match_time / 1_000_000])} s")

    speed_per_impression = match_time / 1_000
    IO.puts("Speed per impression:              #{Float.round(speed_per_impression, 2)} μs")

    if total_matches > 0 do
      speed_per_match = match_time / total_matches
      IO.puts("Speed per match:                   #{Float.round(speed_per_match, 4)} μs")
    end

    avg_matches = total_matches / 1_000
    IO.puts("Average matches per impression:    #{Float.round(avg_matches, 2)}")

    match_rate = (matched_count / 1_000 * 100)
    IO.puts("Match rate:                        #{Float.round(match_rate, 1)}%")

    IO.puts("\n" <> String.duplicate("=", 70))
    IO.puts("Benchmark completed successfully ✓")
    IO.puts(String.duplicate("=", 70) <> "\n")
  end

  # Generate N random orders with varied attributes
  defp generate_orders(count) do
    1..count
    |> Enum.map(fn i ->
      # Vary age range to create both matches and mismatches
      age_idx = rem(i, 3)

      %{
        campaign_id: "campaign-#{i}",
        bid_cpm: Float.round(:rand.uniform() * 100, 2),
        attributes: %{
          age_range: Enum.at(@age_ranges, age_idx),
          interest: "sports",
          content_category: "news"
        }
      }
    end)
  end

  # Generate N random impressions with varied attributes - creates both matches and unmatches
  defp generate_impressions(count) do
    1..count
    |> Enum.map(fn i ->
      # Create a mix of matching and non-matching impressions
      age_cycle = rem(i, 10)

      # Age values: 8 out of 10 will fall within order ranges, 2 will not
      age_value = case age_cycle do
        0 -> "25"  # Matches "18-34"
        1 -> "40"  # Matches "35-49"
        2 -> "55"  # Matches "50+"
        3 -> "30"  # Matches "18-34"
        4 -> "45"  # Matches "35-49"
        5 -> "65"  # Matches "50+"
        6 -> "20"  # Matches "18-34"
        7 -> "52"  # Matches "50+"
        8 -> "10"  # Below "18-34" - no age match
        9 -> "99"  # Above "50+" - no age match
      end

      %{
        age_range: age_value,
        interest: "sports",
        content_category: "news"
      }
    end)
  end

  # Format number with commas for readability
  defp format_number(num) do
    num
    |> Integer.to_string()
    |> String.reverse()
    |> String.graphemes()
    |> Enum.chunk_every(3)
    |> Enum.join(",")
    |> String.reverse()
  end

  # Match all impressions and collect match counts
  defp match_all(tree, impressions) do
    Enum.map(impressions, fn impression ->
      results = Atree.match(tree, impression, %{max_match: 50})
      length(results)
    end)
  end
end

# Execute the benchmark
AtreeBenchmark.run()
