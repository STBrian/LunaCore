# Makefile for compiling .patch files into .ips and unifying them into patches.ips

PYTHON       ?= python3
ROOT_DIR     ?= ..
BUILDER      ?= $(ROOT_DIR)/patchBuilder.py
OUT_DIR      ?= $(ROOT_DIR)/out
DIST_DIR     ?= $(ROOT_DIR)/dist

PATCH_SRCS   := $(wildcard *.patch)
IPS_TARGETS  := $(patsubst %.patch,$(OUT_DIR)/%.ips,$(PATCH_SRCS))
UNIFIED_IPS  := $(DIST_DIR)/patches.ips

.PHONY: all clean

all: $(UNIFIED_IPS)

ifneq ($(strip $(PATCH_SRCS)),)

# Unified IPS target: rebuilds if any individual IPS file changes
$(UNIFIED_IPS): $(IPS_TARGETS) $(BUILDER)
	@mkdir -p $(DIST_DIR)
	@$(PYTHON) $(BUILDER) --merge $(IPS_TARGETS) -o $(UNIFIED_IPS)

# Individual IPS rule: incremental build (only rebuilds if %.patch changed)
$(OUT_DIR)/%.ips: %.patch $(BUILDER)
	@mkdir -p $(OUT_DIR)
	@$(PYTHON) $(BUILDER) $< -o $@

else

$(UNIFIED_IPS):
	@echo "No .patch files found in $(CURDIR)"

endif

clean:
	@rm -rf $(OUT_DIR)
	@rm -f $(UNIFIED_IPS)
