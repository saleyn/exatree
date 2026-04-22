.PHONY: all clean

# Delegates to c_src/Makefile for actual compilation

all: compile

deps:
	mix deps.get

compile:
	$(MAKE) -C c_src
	@mix compile

clean distclean:
	$(MAKE) -C c_src $@
	@rm -fr _build .cover erl_crash.dump
	@mix clean

test:
	@mix $@

cover:
	mix test --cover

benchmark:
	@mix test test/benchmark_test.exs

.PHONY: test deps
