-include scripts/emdawnwebgpu.env

BUILD_DIR      := build
WEB_BUILD_DIR  := build-web
BUILD_TYPE     := Release
WEB_PORT       ?= 8080
WEB_TARGET     := studio
NUM_JOBS       := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
CONFIG         ?= config/conf/studio.yaml
WIDTH          ?=
HEIGHT         ?=
TITLE          ?=
FULLSCREEN     ?=
FLOATING       ?=
BORDERLESS     ?=

# Use Clang with libc++ to match the Dawn dependency
CXX            := $(shell which clang++ 2>/dev/null || which c++ 2>/dev/null || echo c++)
ifeq ($(findstring clang,$(CXX)),clang)
	CXXFLAGS       := -stdlib=libc++
	LDFLAGS        := -stdlib=libc++
endif

.DEFAULT_GOAL := run
.PHONY: configure build rebuild clean install run test all \
	configure-web build-web web

# ARGS for native executable
ARGS           ?=

RUN_ARGS       := $(if $(CONFIG),--config $(CONFIG),)
RUN_ARGS       += $(if $(WIDTH),--width $(WIDTH),)
RUN_ARGS       += $(if $(HEIGHT),--height $(HEIGHT),)
RUN_ARGS       += $(if $(TITLE),--title "$(TITLE)",)
RUN_ARGS       += $(if $(FULLSCREEN),--fullscreen,)
RUN_ARGS       += $(if $(FLOATING),--floating,)
RUN_ARGS       += $(if $(BORDERLESS),--borderless,)
RUN_ARGS       += $(ARGS)

configure:
	@echo "→ Configuring native ($(BUILD_TYPE))..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DCMAKE_CXX_COMPILER=$(CXX) \
		-DCMAKE_CXX_FLAGS="$(CXXFLAGS)"

build: configure
	@echo "→ Building native..."
	@cmake --build $(BUILD_DIR) -j$(NUM_JOBS)
	@echo "✓ Native build complete"

run: build
	@echo "→ Running studio (native)..."
	@cd $(BUILD_DIR)/bin && ./studio $(RUN_ARGS)

test: build
	@echo "→ Running tests..."
	@ctest --test-dir $(BUILD_DIR) --output-on-failure

rebuild: clean build

debug:
	@$(MAKE) run BUILD_TYPE=Debug

release:
	@$(MAKE) run BUILD_TYPE=Release

all: build

configure-web:
	@echo "→ Configuring web ($(BUILD_TYPE))..."
	@mkdir -p $(WEB_BUILD_DIR)
	@cd $(WEB_BUILD_DIR) && emcmake cmake .. \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DEMDAWNWEBGPU_PORT="$(EMDAWNWEBGPU_PORT)"



build-web: configure-web
	@echo "→ Building web..."
	@cmake --build $(WEB_BUILD_DIR) -j$(NUM_JOBS)
	@cp web/favicon.svg $(WEB_BUILD_DIR)/favicon.svg 2>/dev/null || true
	@cp $(WEB_BUILD_DIR)/$(WEB_TARGET).html $(WEB_BUILD_DIR)/index.html
	@echo "✓ Web build complete"

web: build-web
	@echo "→ Running web (emrun)..."
	@emrun --no_browser --hostname localhost --port $(WEB_PORT) $(WEB_BUILD_DIR)
	@echo "Open: http://localhost:$(WEB_PORT)/"

clean:
	@rm -rf $(BUILD_DIR) $(WEB_BUILD_DIR) Testing/
