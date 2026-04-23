defmodule AtreeTest do
  use ExUnit.Case
  doctest Atree

  setup do
    {:ok, tree: Atree.new()}
  end

  describe "tree creation and operations" do
    test "create new empty tree", %{tree: tree} do
      assert is_reference(tree)
    end

    test "insert single order", %{tree: tree} do
      order = %{
        campaign_id: "test-campaign-001",
        bid_cpm:     25.50,
        attributes:  %{
          age_range:        "18-49",
          interest:         "sports",
          content_category: "sports"
        }
      }

      updated_tree = Atree.insert_order(tree, order)
      assert is_reference(updated_tree)
    end

    test "insert multiple orders", %{tree: tree} do
      orders = [
        %{
          campaign_id: "nike-001",
          bid_cpm:     32.50,
          attributes:  %{
            age_range:        "18-49",
            interest:         "sports",
            content_category: "sports"
          }
        },
        %{
          campaign_id: "gatorade-001",
          bid_cpm:     30.50,
          attributes:  %{
            age_range:        "18-49",
            interest:         "fitness",
            content_category: "sports"
          }
        }
      ]

      updated_tree = Atree.insert_orders(tree, orders)
      assert is_reference(updated_tree)
    end
  end

  describe "impression matching" do
    test "match impression against tree", %{tree: tree} do
      order = %{
        campaign_id: "test-001",
        bid_cpm:     25.50,
        attributes:  %{
          age_range:        "18-49",
          interest:         "sports",
          content_category: "sports",
          geography:        "US-EAST"
        }
      }

      tree       = Atree.insert_order(tree, order)

      impression = %{
        age_range:        "34",
        interest:         "sports",
        content_category: "sports",
        geography:        "US-EAST"
      }

      results = Atree.match(tree, impression)
      assert is_list(results)
    end

    test "match returns orders sorted by bid price", %{tree: tree} do
      orders = [
        %{
          campaign_id: "high-bid",
          bid_cpm:     50.00,
          attributes:  %{
            age_range: "18-49",
            interest:  "sports"
          }
        },
        %{
          campaign_id: "medium-bid",
          bid_cpm:     30.00,
          attributes:  %{
            age_range: "18-49",
            interest:  "sports"
          }
        },
        %{
          campaign_id: "low-bid",
          bid_cpm:     10.00,
          attributes:  %{
            age_range: "18-49",
            interest:  "sports"
          }
        }
      ]

      tree       = Atree.insert_orders(tree, orders)

      impression = %{
        age_range: "34",
        interest:  "sports"
      }

      results = Atree.match(tree, impression)
      assert length(results) >= 1

      # Results should be sorted by bid (highest first)
      if length(results) >= 2 do
        [first | rest] = results
        assert first.bid_cpm >= hd(rest).bid_cpm
      end
    end

    test "batch match multiple impressions", %{tree: tree} do
      orders = [
        %{
          campaign_id: "order-1",
          bid_cpm:     25.50,
          attributes:  %{age_range: "18-49", interest: "sports"}
        }
      ]

      tree        = Atree.insert_orders(tree, orders)

      impressions = [
        %{age_range: "34", interest: "sports"},
        %{age_range: "50", interest: "fitness"},
        %{age_range: "25", interest: "sports"}
      ]

      results = Atree.batch_match(tree, impressions)
      assert is_list(results)
      assert length(results) == 3
    end
  end

  describe "utility functions" do
    test "top_n returns top n matched orders", %{tree: tree} do
      orders = [
        %{campaign_id: "1", bid_cpm: 50.0, attributes: %{age_range: "18-49"}},
        %{campaign_id: "2", bid_cpm: 40.0, attributes: %{age_range: "18-49"}},
        %{campaign_id: "3", bid_cpm: 30.0, attributes: %{age_range: "18-49"}}
      ]

      tree  = Atree.insert_orders(tree, orders)

      top_2 = Atree.top_n(tree, %{age_range: "25"}, 2)
      assert is_list(top_2)
      assert length(top_2) <= 2
    end

    test "filter_by_min_bid filters matched orders by minimum price", %{tree: tree} do
      orders = [
        %{campaign_id: "1", bid_cpm: 50.0, attributes: %{age_range: "18-49"}},
        %{campaign_id: "2", bid_cpm: 30.0, attributes: %{age_range: "18-49"}},
        %{campaign_id: "3", bid_cpm: 10.0, attributes: %{age_range: "18-49"}}
      ]

      tree     = Atree.insert_orders(tree, orders)

      filtered = Atree.filter_by_min_bid(tree, %{age_range: "25"}, 25.0)
      assert is_list(filtered)

      # All results should meet minimum bid
      Enum.each(filtered, fn order -> assert order.bid_cpm >= 25.0 end)
    end

    test "format_order returns formatted string" do
      order     = %{campaign_id: "test-001", bid_cpm: 32.50}
      formatted = Atree.format_order(order)
      assert is_binary(formatted)
      assert String.contains?(formatted, ["test-001", "$32.5"])
    end
  end
end
