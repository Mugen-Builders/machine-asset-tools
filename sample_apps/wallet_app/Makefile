
CXX := $(TOOLCHAIN_PREFIX)g++

# C++ warning flags
WARN_CXXFLAGS = -Wall -Wextra -Wpedantic -Wformat \
			-Werror=format-security -Wno-missing-field-initializers

OPT_CXXFLAGS = -O2

EXTRA_FLAGS =

CXXFLAGS += \
	-std=c++20 \
	$(INCLUDE_FLAGS) \
	$(WARN_CXXFLAGS) \
	$(OPT_CXXFLAGS) \
	$(EXTRA_FLAGS)

# C++ flags to improve safety
HARDEN_CXXFLAGS = \
	-fhardened \
	-fno-strict-aliasing \
	-fno-strict-overflow

# Current architecture
ARCH := $(shell uname -m)

DOCKERFILE ?= Dockerfile
BUILDER_IMAGE ?= libcma-builder
SH ?= 'sh'

ifneq ($(ARCH),riscv64)
all: docker
docker-image:
	@docker build --quiet --platform linux/riscv64 -f $(DOCKERFILE) -t $(BUILDER_IMAGE) --target builder .
docker: docker-image
	@docker run --platform linux/riscv64 --rm -v $(PWD):/app -w /app -u $(shell id -u):$(shell id -g) $(BUILDER_IMAGE) make $(ARGS)
docker-shell: docker-image
	@docker run --platform linux/riscv64 -it --rm -v $(PWD):/app -w /app -u $(shell id -u):$(shell id -g) $(BUILDER_IMAGE) $(SH) $(ARGS)
else
all: sample-app
endif

#-------------------------------------------------------------------------------

app_OBJDIR := build
app_BINS := \
	$(app_OBJDIR)/app

$(app_OBJDIR)/%: %.cpp
	mkdir -p $(shell dirname $@)
	$(CXX) $(CXXFLAGS) $(HARDEN_CXXFLAGS) -o $@ $< -lcma -lcmt

sample-app: $(app_BINS)

clean:
	@rm -rf build
