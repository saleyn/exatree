# Contributing to A-Tree

We welcome contributions! This guide explains how to set up your development environment and contribute to A-Tree.

## Development Setup

### Prerequisites
- Erlang/OTP 24+
- Elixir 1.14+
- C++14 compatible compiler (GCC 5+, Clang 3.4+)
- Git

### Install Dependencies
```bash
cd /home/serge/projects/atree2
mix deps.get
```

### Build & Test
```bash
mix compile
mix test
```

## Code Organization

### C++ Code (`c_src/`)
- **atree.h** - Data structures and public interface
- **atree.cpp** - Implementation
- **atree_nif.cpp** - Erlang NIF bindings

**Guidelines**:
- Use `std::shared_ptr` for memory management
- Follow Google C++ style guide
- Add comments for complex algorithms
- Test with ASAN/UBSAN: `make clean && CFLAGS="-fsanitize=address -fsanitize=undefined" make`

### Elixir Code (`lib/`)
- **atree.ex** - Public API
- **atree/native.ex** - NIF stubs
- **atree_example.ex** - Examples and benchmarks

**Guidelines**:
- Use `mix format` before committing
- Add @doc and @spec for all public functions
- Use pattern matching and guards
- Test with various input types

### Tests (`test/`)
- **atree_test.exs** - Unit tests
- Use `doctest` for inline examples

## Making Changes

### Feature Development Process

1. **Create a branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make changes**
   - Update C++ code if needed
   - Update Elixir code
   - Add tests
   - Update documentation

3. **Test locally**
   ```bash
   mix test
   mix format
   ```

4. **Commit with clear messages**
   ```bash
   git commit -m "Add feature: Brief description"
   ```

5. **Push and open PR**
   ```bash
   git push origin feature/your-feature-name
   ```

### Adding a New Dimension

1. **Add to C++ enum** (`c_src/atree.h`):
   ```cpp
   enum class DimensionType {
       // ... existing dimensions
       MY_NEW_DIM,
   };
   ```

2. **Update traverse logic** (`c_src/atree.cpp`):
   ```cpp
   case DimensionType::MY_NEW_DIM:
       attr_value = impression.get_string_attr("my_new_dim");
       break;
   ```

3. **Add to Elixir** (`lib/atree.ex`):
   - Document in match/2 docstring
   - Update examples

4. **Add tests** (`test/atree_test.exs`):
   ```elixir
   test "match with my_new_dim" do
       # test code
   end
   ```

### Optimizing Performance

1. **Profile the code**
   ```bash
   # Use perf on Linux
   perf record --call-graph=dwarf ./compiled_binary
   perf report
   ```

2. **Benchmark before/after**
   ```elixir
   iex> Atree.Example.benchmark_basic(1000, 100)
   ```

3. **Document performance impact**
   - Update IMPLEMENTATION_SUMMARY.md
   - Add benchmark results

## Coding Standards

### C++
```cpp
// Good: Clear variable names, documented logic
std::vector<std::shared_ptr<ATreeNode>> matching_nodes;
for (const auto& node : current_nodes) {
    if (node->value_range.contains(attr_value)) {
        matching_nodes.push_back(node);
    }
}

// Bad: Unclear abbreviations
auto mn;
for (auto n : cn) {
    if (n->vr.c(av)) mn.push_back(n);
}
```

### Elixir
```elixir
# Good: Type specs, clear naming
@spec match(tree(), impression()) :: {:ok, [map()]} | {:error, term()}
def match(tree, impression) when is_reference(tree) and is_map(impression) do
  # implementation
end

# Bad: No specs, unclear naming
def m(t, i) do
  # implementation
end
```

## Testing

### Write Tests For
- ✅ New features
- ✅ Bug fixes
- ✅ Edge cases
- ✅ Error conditions

### Test Structure
```elixir
describe "feature category" do
  test "specific behavior" do
    # Arrange
    {:ok, tree} = Atree.new()
    
    # Act
    result = do_something()
    
    # Assert
    assert result == expected
  end
end
```

### Run Tests
```bash
# All tests
mix test

# Specific test
mix test test/atree_test.exs:123

# With coverage
mix test --cover

# Verbose output
mix test --verbose
```

## Documentation

### Update These Files For Changes
- **README.md** - User-facing features
- **ARCHITECTURE.md** - Technical changes
- **Code comments** - Non-obvious implementation details
- **Docstrings** - All public functions

### Documentation Style
```elixir
@doc """
One-line summary of what this does.

Longer description explaining:
- Why you'd use this
- Key behaviors
- Important edge cases

## Examples

    iex> Atree.new()
    {:ok, tree}

    iex> Atree.insert_order(tree, order)
    {:ok, updated_tree}
"""
```

## Performance Expectations

When you make changes, ensure performance doesn't regress:

| Operation | Expected | Must Be < |
|-----------|----------|-----------|
| Insert order | 0.1ms | 0.5ms |
| Match impression | 0.5ms | 2ms |
| Memory per order | 2KB | 5KB |

Run benchmarks:
```elixir
iex> Atree.Example.benchmark_basic(1000, 100)
```

## Debugging

### Erlang Tools
```elixir
# Trace function calls
:sys.trace(pid, true)

# Get process info
:sys.get_status(pid)

# Enable debug logging
Logger.configure([level: :debug])
```

### C++ Debugging
```bash
# Compile with debug symbols
CFLAGS="-g -O0" make

# Run with gdb
gdb erl
(gdb) run
(gdb) run -S mix test
```

### Print Debugging
```cpp
// In C++ code
fprintf(stderr, "Debug: value=%d, ptr=%p\n", value, ptr);
```

```elixir
# In Elixir code
Logger.debug("Debug: #{inspect(data)}")
```

## Commit Guidelines

- Use present tense: "Add feature" not "Added feature"
- Reference issues: "Fixes #123"
- Keep commits focused (one feature per commit)
- Write clear commit messages

### Example Commit Messages
```
Add support for custom dimensions

- Add custom dimension type to enum
- Update traverse logic
- Add tests for custom dimension matching
- Update documentation

Fixes #42
```

## Pull Request Process

1. **Update documentation** - If features changed
2. **Add tests** - For new code paths
3. **Run formatter** - `mix format`
4. **Run tests** - `mix test`
5. **Write clear PR description** - Explain what and why
6. **Link issues** - "Fixes #123"
7. **Request review** - Tag maintainers

## Code Review Expectations

Reviews will check for:
- ✅ Code quality and style
- ✅ Test coverage
- ✅ Performance impact
- ✅ Documentation completeness
- ✅ Backwards compatibility

## Release Process

1. Update version in `mix.exs`
2. Update CHANGELOG.md
3. Run full test suite
4. Tag release: `git tag -a v0.1.0 -m "Release 0.1.0"`
5. Push: `git push origin --tags`
6. Publish: `mix hex.publish`

## Questions?

- 📖 Check existing documentation
- 🔍 Search GitHub issues
- 💬 Open a discussion
- 📧 Email maintainers

---

Thank you for contributing! 🙏
