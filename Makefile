# Variables
RUNTIME_DIR ?= LunaCoreRuntime
PATCHES_DIR ?= patches
DIST_DIR    ?= dist

# Installation variables
HOME        ?= $(shell echo $$HOME)
EMU_SDMC    ?= $(HOME)/.var/app/org.azahar_emu.Azahar/data/azahar-emu/sdmc
INSTALL_DIR ?= $(EMU_SDMC)
TITLE_ID    ?= 00040000001B8700

# Build configuration: default to debug mode unless specified or target is release
DEBUG        ?= 1
EXPERIMENTAL ?= 0
BUILD_JIT    ?= 0

ifeq ($(RELEASE), 1)
DEBUG := 0
endif

.PHONY: all build patches release debug clean clean_all re install

all: build

build: patches
	$(MAKE) -C $(RUNTIME_DIR) DEBUG=$(DEBUG) EXPERIMENTAL=$(EXPERIMENTAL) BUILD_JIT=$(BUILD_JIT)
	@mkdir -p $(DIST_DIR)
	@find $(RUNTIME_DIR) -maxdepth 1 -type f \( -name "*.3gx" -o -name "*.elf" \) -exec cp -v {} $(DIST_DIR)/ \;

patches:
	$(MAKE) -C $(PATCHES_DIR)

release:
	$(MAKE) build DEBUG=0

debug:
	$(MAKE) build DEBUG=1

install: build
	@echo "Installing LunaCore to $(INSTALL_DIR)..."
	@mkdir -p "$(INSTALL_DIR)/luma/plugins/$(TITLE_ID)"
	@mkdir -p "$(INSTALL_DIR)/Minecraft 3DS/LunaCore"
	@cp -v "$(DIST_DIR)/LunaCore.3gx" "$(INSTALL_DIR)/luma/plugins/$(TITLE_ID)/LunaCore.3gx"
	@if [ -d "assets" ]; then \
		echo "Copying assets to $(INSTALL_DIR)/Minecraft 3DS/..."; \
		cp -rv assets/. "$(INSTALL_DIR)/Minecraft 3DS/"; \
	fi
	@if [ -f "$(DIST_DIR)/patches.ips" ]; then \
		cp -v "$(DIST_DIR)/patches.ips" "$(INSTALL_DIR)/Minecraft 3DS/LunaCore/patches.ips"; \
	fi
	@echo "Installation completed successfully."

clean:
	$(MAKE) -C $(RUNTIME_DIR) clean
	$(MAKE) -C $(PATCHES_DIR) clean
	@rm -rf $(DIST_DIR)

clean_all:
	$(MAKE) -C $(RUNTIME_DIR) clean_all
	$(MAKE) -C $(PATCHES_DIR) clean
	@rm -rf $(DIST_DIR)

re: clean all
