
CC := $(TOOLCHAIN_PREFIX)gcc
CXX := $(TOOLCHAIN_PREFIX)g++
AR := $(TOOLCHAIN_PREFIX)ar

INCLUDE_FLAGS = -Iinclude -I/tmp

CFLAGS += -Wvla -O2 -g -Wall -Wextra \
       		-fno-strict-aliasing -fno-strict-overflow \
       		$(INCLUDE_FLAGS)
# C++ warning flags
WARN_CXXFLAGS = -Wall -Wextra -Wpedantic -Wformat \
			-Werror=format-security -Wno-missing-field-initializers

OPT_CXXFLAGS = -O2

CXXFLAGS += \
	-std=c++20 \
	$(INCLUDE_FLAGS) \
	$(WARN_CXXFLAGS) \
	$(OPT_CXXFLAGS)

# C++ flags to improve safety
HARDEN_CXXFLAGS = \
	-fhardened \
	-fno-strict-aliasing \
	-fno-strict-overflow

LIBCMA_CFLAGS += -Wstrict-aliasing=3 -fPIC

# Current architecture
ARCH := $(shell uname -m)

BUILDER_IMAGE = libcma-builder

ifneq ($(ARCH),riscv64)
all: docker
docker-image:
	@docker build --quiet --platform linux/riscv64 -f Dockerfile -t $(BUILDER_IMAGE) --target builder .
docker: docker-image
	@docker run --platform linux/riscv64 --rm -v $(PWD):/app -w /app -u $(shell id -u):$(shell id -g) $(BUILDER_IMAGE) make $(ARGS)
docker-shell: docker-image
	@docker run --platform linux/riscv64 -it --rm -v $(PWD):/app -w /app -u $(shell id -u):$(shell id -g) $(BUILDER_IMAGE) sh
else
all: libcma
endif

#-------------------------------------------------------------------------------

libcma_SRC := \
	src/ledger.cpp \
	src/ledger_impl.cpp \
	src/parser.cpp \
	src/parser_impl.cpp \
	src/utils.cpp

libcma_OBJDIR    := build/riscv64
libcma_OBJ       := $(patsubst %.cpp,$(libcma_OBJDIR)/%.o,$(libcma_SRC))
libcma_LIB       := $(libcma_OBJDIR)/libcma.a
libcma_SO        := $(libcma_OBJDIR)/libcma.so

$(libcma_OBJ): $(libcma_OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(HARDEN_CXXFLAGS) $(LIBCMA_CFLAGS) -MT $@ -MMD -MP -MF $(@:.o=.d) -c -o $@ $<

$(libcma_LIB): $(libcma_OBJ)
	$(AR) rcs $@ $^

$(libcma_SO): $(libcma_OBJ)
	$(CXX) -shared -o $@ $^

libcma: $(libcma_LIB) $(libcma_SO)

#-------------------------------------------------------------------------------

test_OBJDIR := build/test
unittests_BINS := \
	$(test_OBJDIR)/ledger \
	$(test_OBJDIR)/parser

$(test_OBJDIR)/%: tests/%.c $(libcma_LIB)
	mkdir -p $(test_OBJDIR)
	$(CC) $(CFLAGS) -o $@ $^ -lstdc++

$(test_OBJDIR)/parser: tests/parser.c $(libcma_LIB)
	mkdir -p $(test_OBJDIR)
	$(CC) $(CFLAGS) -o $@ $^ -lcmt -lstdc++

# Note: To compile and use shared libcma.so use:
# $(CC) $(CFLAGS) -o $@ $^ -lstdc++ -L$(libcma_OBJDIR) -lcma
# LD_LIBRARY_PATH=$(libcma_OBJDIR) ./$<

test: $(unittests_BINS)
	$(foreach test,$(unittests_BINS),$(test) &&) true

test-%: $(test_OBJDIR)/%
	./$<

#-------------------------------------------------------------------------------

sample_OBJDIR := build/samples
sample_BINS := \
	$(sample_OBJDIR)/echo_voucher/echo_voucher \
	$(sample_OBJDIR)/wallet_app/wallet_app

$(sample_OBJDIR)/%: sample_apps/%.cpp $(libcma_LIB)
	mkdir -p $(shell dirname $@)
	$(CXX) $(CXXFLAGS) $(HARDEN_CXXFLAGS) -o $@ $< -L$(libcma_OBJDIR) -lcma -lcmt

samples: $(sample_BINS)

sample-%: $(sample_OBJDIR)/%/%

#-------------------------------------------------------------------------------

HDRS := $(patsubst %,include/libcma/%, types.h ledger.h parser.h)
build/ffi.h: $(HDRS)
	cat $^ | sed \
		-e '/\/\*/,/\*\//d' \
		-e '/#if\s/,/#endif/d' \
		-e '/#define/d' \
		-e '/#endif/d' \
		-e '/#ifndef/d' \
		-e '/#include/d' > $@

#-------------------------------------------------------------------------------
LINTER_IGNORE_SOURCES=
LINTER_IGNORE_HEADERS=
LINTER_SOURCES=$(filter-out $(LINTER_IGNORE_SOURCES),$(strip $(wildcard src/*.cpp) $(wildcard tests/*.c) $(wildcard sample_apps/**/*.cpp)))
LINTER_HEADERS=$(filter-out $(LINTER_IGNORE_HEADERS),$(strip $(wildcard src/*.h) $(wildcard include/libcma/*.h)))

CLANG_TIDY=clang-tidy
CLANG_TIDY_TARGETS=$(patsubst %.cpp,%.clang-tidy,$(LINTER_SOURCES))

CLANG_FORMAT=clang-format
CLANG_FORMAT_FILES:=$(wildcard src/*.cpp) $(wildcard src/*.h) $(wildcard tests/*.c) $(wildcard sample_apps/*.cpp) $(wildcard include/libcma/*.h)
CLANG_FORMAT_IGNORE_FILES:=
CLANG_FORMAT_FILES:=$(strip $(CLANG_FORMAT_FILES))
CLANG_FORMAT_FILES:=$(filter-out $(CLANG_FORMAT_IGNORE_FILES),$(strip $(CLANG_FORMAT_FILES)))

EMPTY:=
SPACE:=$(EMPTY) $(EMPTY)
CLANG_TIDY_HEADER_FILTER=$(CURDIR)/($(subst $(SPACE),|,$(LINTER_HEADERS)))

%.clang-tidy: %.cpp
	@$(CLANG_TIDY) --header-filter='$(CLANG_TIDY_HEADER_FILTER)' $< -- $(CXXFLAGS) 2>/dev/null
	@$(CXX) $(CXXFLAGS) $< -MM -MT $@ -MF $@.d > /dev/null 2>&1
	@touch $@

clangd-config:
	@echo "$(CXXFLAGS)" | sed -e $$'s/ \{1,\}/\\\n/g' | grep -v "MMD" > compile_flags.txt

format:
	@$(CLANG_FORMAT) -i $(CLANG_FORMAT_FILES)

check-format:
	@$(CLANG_FORMAT) -Werror --dry-run $(CLANG_FORMAT_FILES)

lint: $(CLANG_TIDY_TARGETS)

#-------------------------------------------------------------------------------

help:
	@echo "Targets: (default: '*')"
	@echo "* all          - Build libcma targets"
	@echo "  libcma       - Build the library; to run on the cartesi-machine."
	@echo "                 (requires the cartesi Linux headers to build)"
	@echo "  test         - Build and run tests on top of the target library on the riscv system."
	@echo "  clean        - remove the binaries and objects."
	@echo "  docker-image - create docker image to build, run, and test library."
	@echo "  docker       - run make in the docker image (set ARGS for additional options)."

clean:
	@rm -rf build
	@rm -rf src/*.clang-tidy src/*.d
	@rm -rf tests/*.clang-tidy tests/*.d

distclean: clean
	@rm -rf compile_flags.txt

OBJ := $(libcma_OBJ)

-include $(OBJ:%.o=%.d)
