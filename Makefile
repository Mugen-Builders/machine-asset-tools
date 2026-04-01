
MAJOR := 0
MINOR := 1
PATCH := 0
LABEL :=
VERSION ?= $(MAJOR).$(MINOR).$(PATCH)$(LABEL)
TOOLS_DEB ?= build/_deb/machine-asset-tools_riscv64.deb
TOOLS_TAR_NAME ?= build/_tar/machine-asset-tools
BOOST_VERSION ?= 1.90.0
BOOST_VERSION_U := $(subst .,_,$(BOOST_VERSION))
MACHINE_EMULATOR_VERSION ?= 0.19.0

CC := $(TOOLCHAIN_PREFIX)gcc
CXX := $(TOOLCHAIN_PREFIX)g++
AR := $(TOOLCHAIN_PREFIX)ar

third_party_DIR := third-party
INCLUDE_FLAGS = -Iinclude -I$(third_party_DIR)

CFLAGS += -Wvla -O2 -g -Wall -Wextra \
       		-fno-strict-aliasing -fno-strict-overflow \
       		$(INCLUDE_FLAGS)
# C++ warning flags
WARN_CXXFLAGS = -Wall -Wextra -Wpedantic -Wformat \
			-Werror=format-security -Wno-missing-field-initializers

OPT_CXXFLAGS = -O2

# This fixes issues when running tests with QEMU,
# where interprocess attempts to call pthread_mutex_init(), which fails making interproces not work properly
DEFS += -DBOOST_INTERPROCESS_FORCE_GENERIC_EMULATION

# This fixes reproducibility issues with Boost.Unordered libraries across riscv64/x86_64/aarch64
DEFS += -DBOOST_UNORDERED_DISABLE_SSE2 -DBOOST_UNORDERED_DISABLE_NEON

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

DOCKERFILE ?= ubuntu.dockerfile
BUILDER_IMAGE ?= libcma-builder
SH ?= 'bash'

ifneq ($(ARCH),riscv64)
all: docker
docker-image:
	docker build --platform linux/riscv64 -f $(DOCKERFILE) -t $(BUILDER_IMAGE) --target builder .
docker: docker-image
	docker run --platform linux/riscv64 --rm -v $(PWD):/mnt/app -w /mnt/app -u $(shell id -u):$(shell id -g) $(BUILDER_IMAGE) make $(ARGS)
docker-shell: docker-image
	docker run --platform linux/riscv64 -it --rm -v $(PWD):/mnt/app -w /mnt/app -u $(shell id -u):$(shell id -g) $(BUILDER_IMAGE) $(SH) $(ARGS)
else
all: libcma
endif

#-------------------------------------------------------------------------------

ledger_SRC := \
	src/ledger_impl.cpp \
	src/ledger.cpp \
	src/utils.cpp

parser_SRC := \
	src/parser.cpp \
	src/parser_impl.cpp \

libcma_OBJDIR    := build/riscv64
libcma_OBJ       := $(patsubst %.cpp,$(libcma_OBJDIR)/%.o,$(ledger_SRC) $(parser_SRC))
libcma_LIB       := $(libcma_OBJDIR)/libcma.a
libcma_SO        := $(libcma_OBJDIR)/libcma.so

$(libcma_OBJ): $(libcma_OBJDIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(HARDEN_CXXFLAGS) $(LIBCMA_CFLAGS) $(DEFS) -MT $@ -MMD -MP -MF $(@:.o=.d) -c -o $@  $<

$(libcma_LIB): $(libcma_OBJ)
	$(AR) rcs $@ $^

$(libcma_SO): $(libcma_OBJ)
	$(CXX) -shared -o $@ $^ -lcmt

libcma: $(libcma_LIB) $(libcma_SO)

#-------------------------------------------------------------------------------

third_party_download_DIR := $(third_party_DIR)/downloads
boost_LIB       := $(third_party_DIR)/boost
tiny_sha3_LIB   := $(third_party_DIR)/tiny_sha3
emulator_LIB    := $(third_party_DIR)/machine-emulator

third_party_LIBS := $(boost_LIB) $(tiny_sha3_LIB) $(emulator_LIB)

third-party: $(third_party_LIBS)

third_party_DIR:
	mkdir -p $(third_party_DIR)

$(third_party_download_DIR):
	mkdir -p $(third_party_download_DIR)

third-party-boost: $(boost_LIB)
$(boost_LIB): third_party_DIR $(third_party_download_DIR)/boost_$(BOOST_VERSION_U).tar.gz
	tar zxvf $(third_party_download_DIR)/boost_$(BOOST_VERSION_U).tar.gz --strip-components 1 -C $(third_party_DIR) boost_$(BOOST_VERSION_U)/boost

$(third_party_download_DIR)/boost_$(BOOST_VERSION_U).tar.gz: $(third_party_download_DIR)
	wget https://archives.boost.io/release/$(BOOST_VERSION)/source/boost_$(BOOST_VERSION_U).tar.gz -O $(third_party_download_DIR)/boost_$(BOOST_VERSION_U).tar.gz

emulator_tar_FILES	:= complete-merkle-tree.cpp pristine-merkle-tree.cpp complete-merkle-tree.h pristine-merkle-tree.h i-hasher.h keccak-256-hasher.h meta.h merkle-tree-proof.h
emulator_SRCPATH 	:= machine-emulator-$(MACHINE_EMULATOR_VERSION)/src
emulator_FILES  	:= $(patsubst %,$(emulator_SRCPATH)/%,$(emulator_tar_FILES))
emulator_misc_src_FILES := complete-merkle-tree.cpp complete-merkle-tree.h concepts.h hash-tree-proof.h i-hasher.h keccak-256-hasher.h machine-hash.h pristine-merkle-tree.cpp pristine-merkle-tree.h
emulator_misc_FILES := $(patsubst %,misc/%,$(emulator_misc_src_FILES))
emulator_SRC		:= complete-merkle-tree.cpp pristine-merkle-tree.cpp

third-party-emulator: $(emulator_LIB)
$(emulator_LIB): third_party_DIR $(third_party_download_DIR)/machine-emulator_$(MACHINE_EMULATOR_VERSION).tar.gz
	mkdir -p $(emulator_LIB)
	# tar zxvf $(third_party_download_DIR)/machine-emulator_$(MACHINE_EMULATOR_VERSION).tar.gz --strip-components 2 -C $(emulator_LIB) $(emulator_FILES)
	cp -t $(emulator_LIB) $(emulator_misc_FILES)

$(third_party_download_DIR)/machine-emulator_$(MACHINE_EMULATOR_VERSION).tar.gz: $(third_party_download_DIR)
	wget https://github.com/cartesi/machine-emulator/archive/refs/tags/v$(MACHINE_EMULATOR_VERSION).tar.gz -O $(third_party_download_DIR)/machine-emulator_$(MACHINE_EMULATOR_VERSION).tar.gz

tiny_sha3_HEADERS 	:= sha3.h
tiny_sha3_SRC 		:= sha3.c
tiny_sha3_SRCPATH 	:= tiny_sha3-master
tiny_sha3_FILES     := $(patsubst %,$(tiny_sha3_SRCPATH)/%,$(tiny_sha3_SRC) $(tiny_sha3_HEADERS))
tiny_sha3_emulator_SRCPATH 	:= machine-emulator-$(MACHINE_EMULATOR_VERSION)/third-party/tiny_sha3
tiny_sha3_emulator_FILES     := $(patsubst %,$(tiny_sha3_emulator_SRCPATH)/%,$(tiny_sha3_SRC) $(tiny_sha3_HEADERS))

third-party-tiny-sha3: $(tiny_sha3_LIB)
$(tiny_sha3_LIB): third_party_DIR $(third_party_download_DIR)/machine-emulator_$(MACHINE_EMULATOR_VERSION).tar.gz
	mkdir -p $(tiny_sha3_LIB)
	tar zxvf $(third_party_download_DIR)/machine-emulator_$(MACHINE_EMULATOR_VERSION).tar.gz --strip-components 3 -C $(tiny_sha3_LIB) $(tiny_sha3_emulator_FILES)

# $(tiny_sha3_LIB): third_party_DIR $(third_party_download_DIR)/tiny_sha3.zip
# 	unzip -f -j $(third_party_download_DIR)/tiny_sha3.zip $(tiny_sha3_FILES) -d $(tiny_sha3_LIB)
# $(third_party_download_DIR)/tiny_sha3.zip: $(third_party_download_DIR)
# 	wget https://github.com/mjosaarinen/tiny_sha3/archive/refs/heads/master.zip -O $(third_party_download_DIR)/tiny_sha3.zip

#-------------------------------------------------------------------------------

# CXXFLAGS = -MMD --std=c++23 -O2 -Wall -pedantic $(INC)
# CFLAGS = -MMD -O2 -Wall -pedantic $(INC)

tools_OBJDIR    := build/tools
tools_SRC := \
	tools/account-driver-reader.cpp

SHA3_INC = -I$(tiny_sha3_LIB)
EMULATOR_INC = -I$(emulator_LIB)

TOOLS_INC = $(SHA3_INC) $(EMULATOR_INC)

SHA3_SRC     := $(patsubst %,$(tiny_sha3_LIB)/%,$(tiny_sha3_SRC))
SHA3_OBJ     := $(patsubst %.c,$(tools_OBJDIR)/%.o,$(SHA3_SRC))
EMULATOR_SRC := $(patsubst %,$(emulator_LIB)/%,$(emulator_SRC))
EMULATOR_OBJ := $(patsubst %.cpp,$(tools_OBJDIR)/%.o,$(EMULATOR_SRC))

$(SHA3_SRC): $(tiny_sha3_LIB)
$(EMULATOR_SRC): $(emulator_LIB)

$(tools_OBJDIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(TOOLS_INC) -MMD -MP -MF $(@:.o=.d) -c -o $@  $<

$(tools_OBJDIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(TOOLS_INC) -MMD -MP -MF $(@:.o=.d) -c -o $@  $<

tools_OBJDIR    := build/tools
tools_OBJ        := $(patsubst %.cpp,$(tools_OBJDIR)/%.o,$(tools_SRC))

$(tools_OBJ): $(tools_OBJDIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(TOOLS_INC) $(HARDEN_CXXFLAGS) $(LIBCMA_CFLAGS) $(DEFS) -MT $@ -MMD -MP -MF $(@:.o=.d) -c -o $@  $<

ADREADER_OBJ = $(tools_OBJDIR)/account-driver-reader.o
SRC_INC = -Isrc

ledger_OBJ       := $(patsubst %.cpp,$(libcma_OBJDIR)/%.o,$(ledger_SRC))

$(ADREADER_OBJ): $(tools_OBJDIR)/%.o: tools/%.cpp
	mkdir -p $(@D)
	$(CXX) -std=c++23 $(INCLUDE_FLAGS) $(WARN_CXXFLAGS) $(OPT_CXXFLAGS) $(TOOLS_INC) $(SRC_INC) $(HARDEN_CXXFLAGS) $(LIBCMA_CFLAGS) $(DEFS) -MT $@ -MMD -MP -MF $(@:.o=.d) -c -o $@  $<

account-driver-reader: $(ADREADER_OBJ) $(SHA3_OBJ) $(EMULATOR_OBJ) $(ledger_OBJ)
	$(CXX) -o $(tools_OBJDIR)/$@ $^


#-------------------------------------------------------------------------------

PREFIX ?= /usr
DESTDIR ?= .

install-run: $(libcma_SO)
	@mkdir -p $(DESTDIR)$(PREFIX)/lib
	@cp -f $(libcma_SO) $(DESTDIR)$(PREFIX)/lib

install-dev: $(libcma_LIB) build/ffi.h
	@mkdir -p $(DESTDIR)$(PREFIX)/lib
	@cp -f $(libcma_LIB) $(DESTDIR)$(PREFIX)/lib
	@mkdir -p $(DESTDIR)$(PREFIX)/include/libcma/
	@cp -f include/libcma/*.h $(DESTDIR)$(PREFIX)/include/libcma/
	@cp -f build/ffi.h $(DESTDIR)$(PREFIX)/include/libcma/
	@mkdir -p $(DESTDIR)$(PREFIX)/lib/pkgconfig
	@sed -e 's|@PREFIX@|$(PREFIX)|g' -e 's|@ARG_VERSION@|$(VERSION)|g'\
	    tools/libcma.pc.in > $(DESTDIR)$(PREFIX)/lib/pkgconfig/libcma.pc

install: install-run install-dev

tar-files-dev: PREFIX = /usr
tar-files-dev: DESTDIR = ./build/_install
tar-files-dev: install-dev
	mkdir -p $(shell dirname ${TOOLS_TAR_NAME})
	tar -czf ${TOOLS_TAR_NAME}-dev.tar.gz -C $(DESTDIR) .

tar-files-run: PREFIX = /usr
tar-files-run: DESTDIR = ./build/_install
tar-files-run: install-run
	mkdir -p $(shell dirname ${TOOLS_TAR_NAME})
	tar -czf ${TOOLS_TAR_NAME}.tar.gz -C $(DESTDIR) .

tar-files: PREFIX = /usr
tar-files: DESTDIR = ./build/_install
tar-files: tar-files-run tar-files-dev

control: build/control

build/control: tools/control.in
	@sed 's|ARG_VERSION|$(VERSION)|g' tools/control.in > build/control

deb: PREFIX = /usr
deb: DESTDIR = ./build/_install
deb: control install
	mkdir -p $(DESTDIR)/DEBIAN
	mkdir -p $(shell dirname ${TOOLS_DEB})
	cp build/control copyright $(DESTDIR)/DEBIAN/
	dpkg-deb -Zxz --root-owner-group --build $(DESTDIR) ${TOOLS_DEB}

#-------------------------------------------------------------------------------

test_OBJDIR := build/test
unittests_BINS := \
	$(test_OBJDIR)/ledger \
	$(test_OBJDIR)/file-ledger \
	$(test_OBJDIR)/buffer-ledger \
	$(test_OBJDIR)/parser

$(test_OBJDIR)/%: tests/%.c $(libcma_LIB)
	mkdir -p $(test_OBJDIR)
	$(CC) $(CFLAGS) -o $@ $^ -lm -lstdc++

$(test_OBJDIR)/parser: tests/parser.c $(libcma_LIB)
	mkdir -p $(test_OBJDIR)
	$(CC) $(CFLAGS) -o $@ $^ -lcmt -lstdc++

# Note: To compile and use shared libcma.so use:
# $(CC) $(CFLAGS) -o $@ $^ -lstdc++ -L$(libcma_OBJDIR) -lcma
# LD_LIBRARY_PATH=$(libcma_OBJDIR) ./$<

test: $(unittests_BINS)
	@echo "Running all tests..."
	for bin in $(unittests_BINS); do \
		echo "Running $$bin..."; \
		./$$bin; \
	done

test-%: $(test_OBJDIR)/%
	./$<

#-------------------------------------------------------------------------------
SAMPLE=wallet_app

sample: $(libcma_LIB)
	$(MAKE) -C sample_apps/$(SAMPLE) EXTRA_FLAGS="-I$(PWD)/include -L$(PWD)/$(libcma_OBJDIR)"

#-------------------------------------------------------------------------------

HDRS := $(patsubst %,include/libcma/%, types.h ledger.h parser.h)
build/ffi.h: $(HDRS)
	cat $^ | sed \
		-e 's/\/\*.*\*\///g' \
		-e '/\/\*/,/\*\//d' \
		-e '/#if\s/,/#endif/d' \
		-e '/#define/d' \
		-e '/#endif/d' \
		-e '/#ifndef/d' \
		-e '/#ifdef/d' \
		-e '/extern/d' \
		-e '/#pragma/d' \
		-e 's/__attribute__((__packed__))//g' \
		-e 's/CMA_LEDGER_API //g' \
		-e 's/CMA_PARSER_API //g' \
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
	$(CLANG_TIDY) --header-filter='$(CLANG_TIDY_HEADER_FILTER)' $< -- $(CXXFLAGS) 2>/dev/null
	$(CXX) $(CXXFLAGS) $< -MM -MT $@ -MF $@.d > /dev/null 2>&1
	touch $@

clangd-config:
	echo "$(CXXFLAGS)" | sed -e $$'s/ \{1,\}/\\\n/g' | grep -v "MMD" > compile_flags.txt

format:
	$(CLANG_FORMAT) -i $(CLANG_FORMAT_FILES)

check-format:
	$(CLANG_FORMAT) -Werror --dry-run $(CLANG_FORMAT_FILES)

lint: $(CLANG_TIDY_TARGETS)

#-------------------------------------------------------------------------------

help:
	@echo "Targets: (default: '*')"
	@echo "* all          - Build libcma targets"
	@echo "  libcma       - Build the library; to run on the cartesi-machine."
	@echo "                 (requires the cartesi Linux headers to build)"
	@echo "  test         - Build and run tests on top of the target library on the riscv system."
	@echo "  install      - Install the library and C headers; on the host system."
	@echo "                 Use DESTDIR and PREFIX to customize the installation."
	@echo "  clean        - remove the binaries and objects."
	@echo "  docker-image - create docker image to build, run, and test library."
	@echo "  docker       - run make in the docker image (set ARGS for additional options)."

clean:
	@rm -rf build
	@rm -rf src/*.clang-tidy src/*.d
	@rm -rf tests/*.clang-tidy tests/*.d

distclean: clean
	@rm -rf compile_flags.txt
	@rm -rf $(third_party_DIR)

OBJ := $(libcma_OBJ)

.PHONY: all clean libcma third-party install \
	install-run install-dev tar-files control deb \
	docker docker-image docker-shell \
	test test-% control sample \
	clangd-config format check-format lint
-include $(OBJ:%.o=%.d)
