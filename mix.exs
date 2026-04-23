defmodule Atree.MixProject do
  use Mix.Project

  def project do
    [
      app:             :atree,
      version:         "0.1.1",
      elixir:          "~> 1.14",
      start_permanent: Mix.env() == :prod,
      deps:            deps(),
      name:            "A-Tree",
      description:     "Multi-dimensional A-Tree for order filtering",
      package:         package(),
      test_pattern:    "*_test.exs",
      test_coverage:   [
        output:         ".cover",
        ignore_modules: [Atree.Native],
        summary:        [threshold: 90]
      ]
    ]
  end

  def application do
    [
      extra_applications: [:logger]
    ]
  end

  defp deps do
    [
      {:nimble_parsec, "~> 1.4",   only: :dev},
      {:exalign,       "~> 0.1.7", only: :dev}
    ]
  end

  defp package do
    [
      files: [
        "lib",
        "c_src",
        "priv",
        "mix.exs",
        "Makefile",
        "README.md",
        "LICENSE"
      ],
      licenses: ["MIT"],
      links:    %{
        "GitHub" => "https://github.com/saleyn/exatree"
      }
    ]
  end
end
