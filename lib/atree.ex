defmodule Atree.Native do
  @moduledoc """
  Native Interface (NIF) bindings for A-Tree operations.

  Low-level native functions are defined here. Users should typically
  use the higher-level Atree module instead.
  """

  @on_load   {:on_load, 0}  # Load the native compiled library

  def on_load do
    options =
      case Application.get_env(:atree, :default_max_match) do
        nil                                                    -> %{}
        max_match when is_integer(max_match) and max_match > 0 -> %{max_match: max_match}
      end

    :erlang.load_nif(to_charlist(:code.priv_dir(:atree) ++ ~c"/atree"), options)
  end

  @doc false
  def build(_unused), do: :erlang.nif_error(:not_loaded)

  @doc false
  def insert_order(_tree_ref, _campaign_id, _bid_cpm, _attributes),
    do: :erlang.nif_error(:not_loaded)

  @doc false
  def match(_tree_ref, _impression, _options \\ nil), do: :erlang.nif_error(:not_loaded)
end

defmodule Atree do
  @moduledoc """
  A-Tree (Attribute Tree) - Multi-dimensional constraint matching library
  for efficient order filtering by attributes.

  ## Features

  - Sub-millisecond evaluation of thousands of orders
  - Multi-dimensional constraint matching (8 dimensions)
  - Hierarchical structure for content categories
  - Fast dynamic updates
  - Brand safety and frequency cap evaluation

  ## Example

      iex> tree = Atree.new()
      iex> order = %{
      ...>   campaign_id: "nike-001",
      ...>   bid_cpm: 32.50,
      ...>   attributes: %{
      ...>     age_range: "18-49",
      ...>     interest: "sports",
      ...>     content_category: "sports",
      ...>     geography: "US-EAST"
      ...>   }
      ...> }
      iex> tree = Atree.insert_order(tree, order)
      iex> impression = %{
      ...>   age_range: "34",
      ...>   interest: "sports",
      ...>   content_category: "sports",
      ...>   geography: "US-EAST"
      ...> }
      iex> _matches = Atree.match(tree, impression)
  """

  @type standing_order :: %{
          campaign_id:   String.t(),
          bid_cpm:       float(),
          attributes:    map(),
          frequency_cap: frequency_cap() | nil,
          brand_safety:  brand_safety() | nil
        }

  @type frequency_cap :: %{
          hourly_limit: non_neg_integer(),
          daily_limit:  non_neg_integer(),
          weekly_limit: non_neg_integer()
        }

  @type brand_safety :: %{
          excluded_categories: [String.t()],
          excluded_keywords:   [String.t()],
          require_age_gate:    boolean()
        }

  @type impression :: %{
          String.t() => String.t() | integer() | float()
        }

  @type tree :: reference() | nil
  @type matcher :: reference() | nil

  # ============================================================================
  # Public API
  # ============================================================================

  @doc  """
  Create a new empty A-Tree.

  Returns `tree()` on success.
  """
  @spec new() :: tree()
  def new do
    Atree.Native.build(nil)
  end

  @doc  """
  Insert a order into the tree.

  The order map should contain:
  - `:campaign_id` - Unique identifier for the campaign
  - `:bid_cpm` - Bid price per thousand impressions
  - `:attributes` - Map of attribute constraints (age_range, interest, etc.)
  - `:frequency_cap` (optional) - Frequency limits
  - `:brand_safety` (optional) - Brand safety rules

  Returns `tree()` on success.
  """
  @spec insert_order(tree(), standing_order()) :: tree()
  def insert_order(tree, order) when is_reference(tree) and is_map(order) do
    campaign_id = order[:campaign_id] || raise "campaign_id required"
    bid_cpm     = order[:bid_cpm] || raise "bid_cpm required"
    attrs       = order[:attributes] || %{}

    # Convert attributes map to list of tuples for NIF
    attr_list = attrs |> Map.to_list() |> Enum.map(fn {k, v} -> {to_string(k), to_string(v)} end)

    Atree.Native.insert_order(tree, campaign_id, bid_cpm, attr_list)
  end

  @doc """
  Matches impressions against a tree structure.

  ## Parameters

    * `tree` - A reference to the tree structure to match against
    * `impression` - A map containing the impression data to match
    * `options` - Optional map that may contain:
      * `:max_match` - Integer to limit the maximum number of matches returned

  ## Returns

  Returns list of matched_orders where matched_orders is a list of
  orders sorted by bid price (highest first), limited by `max_match`
  if specified in options.
  """
  @spec match(tree(), impression(), %{optional(:max_match) => non_neg_integer()} | nil) :: [map()]
  def match(tree, impression, options \\ nil) when is_reference(tree) and is_map(impression) do
    impression_map =  # Convert impression to string-keyed map for NIF
      impression
      |> Enum.map(fn {k, v} -> {to_string(k), to_string(v)} end)
      |> Enum.into(%{})

    Atree.Native.match(tree, impression_map, options)
  end

  @doc  """
  Insert multiple orders at once.

  Returns `tree()` if all orders are inserted successfully or raises an exception.
  """
  @spec insert_orders(tree(), [standing_order()]) :: tree()
  def insert_orders(tree, orders) when is_reference(tree) and is_list(orders) do
    Enum.reduce(orders, tree, fn order, current_tree -> insert_order(current_tree, order) end)
  end

  @doc  """
  Batch match multiple impressions.

  Returns a list of `{impression, orders}` tuples. Raises an exception on error.
  """
  @spec batch_match(tree(), [impression()]) :: [{impression(), [map()]}]
  def batch_match(tree, impressions) when is_reference(tree) and is_list(impressions) do
    Enum.map(impressions, &{&1, match(tree, &1)})
  end

  # ============================================================================
  # Utility Functions
  # ============================================================================

  @doc  """
  Pretty-print a order.
  """
  @spec format_order(map()) :: String.t()
  def format_order(%{campaign_id: id, bid_cpm: bid}) do
    "#{id} @ $#{bid |> Float.round(2)}"
  end

  @doc  """
  Get the top N matched orders by bid price.
  """
  @spec top_n(tree(), impression(), non_neg_integer()) :: [map()]
  def top_n(tree, impression, n) when is_integer(n) and n > 0 do
    tree
    |> match(impression)
    |> Enum.take(n)
  end

  @doc  """
  Filter matched orders by minimum bid price.
  """
  @spec filter_by_min_bid(tree(), impression(), float()) :: [map()]
  def filter_by_min_bid(tree, impression, min_bid)
      when is_float(min_bid) or is_integer(min_bid) do
    tree
    |> match(impression)
    |> Enum.filter(fn order -> order.bid_cpm >= min_bid end)
  end
end