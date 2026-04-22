.PHONY: all clean

# Delegates to c_src/Makefile for actual compilation

all: compile

deps:
	mix deps.get

compile:
	$(MAKE) -C c_src
	@mix compile

clean:
	$(MAKE) -C c_src clean
	@mix clean

test:
	@mix $@

cover:
	mix test --cover

benchmark:
	@mix test test/benchmark.ex

.PHONY: test deps
