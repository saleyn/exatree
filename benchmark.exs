#!/usr/bin/env elixir
# Run A-Tree benchmark
# Usage: mix run benchmark.exs

Code.require_file("test/atree_benchmark.exs")
AtreeBenchmark.run()
