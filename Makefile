# Variables
RUNTIME_DIR ?= LunaCoreRuntime
DIST_DIR    ?= dist

# Build configuration: default to debug mode unless specified or target is release
DEBUG        ?= 1
EXPERIMENTAL ?= 0
BUILD_JIT    ?= 0

ifeq ($(RELEASE), 1)
DEBUG := 0
endif

.PHONY: all build release debug clean clean_all re

all: build

build:
	$(MAKE) -C $(RUNTIME_DIR) DEBUG=$(DEBUG) EXPERIMENTAL=$(EXPERIMENTAL) BUILD_JIT=$(BUILD_JIT)
	@mkdir -p $(DIST_DIR)
	@find $(RUNTIME_DIR) -maxdepth 1 -type f \( -name "*.3gx" -o -name "*.elf" \) -exec cp -v {} $(DIST_DIR)/ \;

release:
	$(MAKE) build DEBUG=0

debug:
	$(MAKE) build DEBUG=1

clean:
	$(MAKE) -C $(RUNTIME_DIR) clean
	@rm -rf $(DIST_DIR)

clean_all:
	$(MAKE) -C $(RUNTIME_DIR) clean_all
	@rm -rf $(DIST_DIR)

re: clean all
