defmodule Atree.MixProject do
  use Mix.Project

  def project do
    [
      app: :atree,
      version: "0.1.0",
      elixir: "~> 1.14",
      start_permanent: Mix.env() == :prod,
      deps: deps(),
      name: "A-Tree",
      description: "Multi-dimensional A-Tree for standing order filtering",
      package: package()
    ]
  end

  def application do
    [
      extra_applications: [:logger]
    ]
  end

  defp deps do
    [
      {:nimble_parsec, "~> 1.4", only: :dev}
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
      maintainers: ["A-Tree Contributors"],
      licenses: ["MIT"],
      links: %{
        "GitHub" => "https://github.com/yourusername/atree"
      }
    ]
  end
end
