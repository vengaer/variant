CXX      ?= g++

CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -c -g
CPPFLAGS := -I$(CURDIR)/include -I$(CURDIR)/tests

LDFLAGS  :=
LDLIBS   :=

QUIET    := @

target   := variant_test
srcdir   := tests
build    := build

src      := $(wildcard $(srcdir)/*.cc)
objs     := $(patsubst %,$(build)/%,$(notdir $(src:.cc=.o)))

.PHONY: all
all: $(target)

$(target): $(objs)
	$(QUIET)echo "[LD]  $@"
	$(QUIET)$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(build)/%.o: $(srcdir)/%.cc | $(build)
	$(QUIET)echo "[CXX] $@"
	$(QUIET)$(CXX) -o $@ $^ $(CXXFLAGS) $(CPPFLAGS)

$(build):
	$(QUIET)mkdir -p $@

.PHONY: run
run: $(target)
	$(QUIET)./$^

.PHONY: clean
clean:
	$(QUIET)rm -rf $(build) $(target)
