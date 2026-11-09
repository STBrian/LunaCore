# Variables
RUNTIME_DIR ?= LunaCoreRuntime
DIST_DIR    ?= dist

.PHONY: all build clean clean_all re

all: build

build:
	$(MAKE) -C $(RUNTIME_DIR)
	@mkdir -p $(DIST_DIR)
	@find $(RUNTIME_DIR) -maxdepth 1 -type f \( -name "*.3gx" -o -name "*.elf" \) -exec cp -v {} $(DIST_DIR)/ \;

clean:
	$(MAKE) -C $(RUNTIME_DIR) clean
	@rm -rf $(DIST_DIR)

clean_all:
	$(MAKE) -C $(RUNTIME_DIR) clean_all
	@rm -rf $(DIST_DIR)

re: clean all
