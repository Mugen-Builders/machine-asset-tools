
CC := $(TOOLCHAIN_PREFIX)gcc
CXX := $(TOOLCHAIN_PREFIX)g++
AR := $(TOOLCHAIN_PREFIX)ar
CFLAGS += -Wvla -O2 -g -Wall -Wextra -Iinclude \
          	-fno-strict-aliasing -fno-strict-overflow
# C++ warning flags
WARN_CXXFLAGS = -Wall -Wextra -Iinclude -Wpedantic -Wformat \
			-Werror=format-security -Wno-missing-field-initializers

OPT_CXXFLAGS = -O2

CXXFLAGS += \
	-std=c++20 \
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

#-------------------------------------------------------------------------------

libcma_SRC := \
	src/ledger.cpp \
	src/ledger_impl.cpp \
	src/parser.cpp \
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

# #-------------------------------------------------------------------------------

test_OBJDIR := build/test
unittests_BINS := \
	$(test_OBJDIR)/ledger \
	$(test_OBJDIR)/parser

$(test_OBJDIR)/%: $(test_OBJDIR)/%.o $(libcma_LIB)
	$(CXX) $(CXXFLAGS) -lstdc++ -o $@ $^

$(test_OBJDIR)/%.o: tests/%.c
	mkdir -p $(test_OBJDIR)
	$(CC) $(CFLAGS) -o $@ -c $^

# $(test_OBJDIR)/%: tests/%.c $(libcma_LIB)
# 	$(CC) $(CFLAGS) -lstdc++ -o $@ $^

test: $(unittests_BINS)
	$(foreach test,$(unittests_BINS),$(test) &&) true

test-%: $(test_OBJDIR)/%
	./$<

#-------------------------------------------------------------------------------
LINTER_IGNORE_SOURCES=
LINTER_IGNORE_HEADERS=
LINTER_SOURCES=$(filter-out $(LINTER_IGNORE_SOURCES),$(strip $(wildcard src/*.cpp) $(wildcard tests/*.c)))
LINTER_HEADERS=$(filter-out $(LINTER_IGNORE_HEADERS),$(strip $(wildcard src/*.h) $(wildcard include/libcma/*.h)))

CLANG_TIDY=clang-tidy
CLANG_TIDY_TARGETS=$(patsubst %.cpp,%.clang-tidy,$(LINTER_SOURCES))

CLANG_FORMAT=clang-format
CLANG_FORMAT_FILES:=$(wildcard src/*.cpp) $(wildcard src/*.h) $(wildcard tests/*.c) $(wildcard tools/*.c) $(wildcard include/libcma/*.h)
CLANG_FORMAT_IGNORE_FILES:=
CLANG_FORMAT_FILES:=$(strip $(CLANG_FORMAT_FILES))
CLANG_FORMAT_FILES:=$(filter-out $(CLANG_FORMAT_IGNORE_FILES),$(strip $(CLANG_FORMAT_FILES)))

EMPTY:=
SPACE:=$(EMPTY) $(EMPTY)
CLANG_TIDY_HEADER_FILTER=$(CURDIR)/($(subst $(SPACE),|,$(LINTER_HEADERS)))

%.clang-tidy: %.cpp
	@$(CLANG_TIDY) --header-filter='$(CLANG_TIDY_HEADER_FILTER)' $< -- $(CFLAGS) 2>/dev/null
	@$(CC) $(CFLAGS) $< -MM -MT $@ -MF $@.d > /dev/null 2>&1
	@touch $@

clangd-config:
	@echo "$(CFLAGS)" | sed -e $$'s/ \{1,\}/\\\n/g' | grep -v "MMD" > compile_flags.txt

format:
	@$(CLANG_FORMAT) -i $(CLANG_FORMAT_FILES)

check-format:
	@$(CLANG_FORMAT) -Werror --dry-run $(CLANG_FORMAT_FILES)

lint: $(CLANG_TIDY_TARGETS)

#-------------------------------------------------------------------------------

endif

help:
	@echo "Targets: (default: '*')"
	@echo "* all          - Build libcma and host targets"
	@echo "  host         - Build mock and tools targets"
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
	@rm -rf tools/*.clang-tidy tools/*.d
	@rm -rf *.bin

distclean: clean
	@rm -rf compile_flags.txt

OBJ := $(libcma_OBJ)

-include $(OBJ:%.o=%.d)
