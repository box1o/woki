-include scripts/emdawnwebgpu.env

BUILD_DIR      := build
WEB_BUILD_DIR  := build-web
BUILD_TYPE     := Release
WEB_PORT       ?= 8080
WEB_TARGET     := studio
NUM_JOBS       ?= $(shell nproc)
CONFIG         ?= config/conf/studio.yaml
PREFIX         ?= /usr/local
WOKI_EXTENSION_WITH_WASMTIME ?= ON
WASMTIME_PROVIDER ?= prebuilt
WOKI_WASMTIME_BUILD_FROM_SOURCE ?= OFF
ifneq ($(WOKI_WASMTIME_BUILD_FROM_SOURCE),OFF)
WASMTIME_PROVIDER := source
endif

# Use Clang with libc++ to match the Dawn dependency
CXX            := $(shell which clang++ 2>/dev/null || which c++ 2>/dev/null || echo c++)
ifeq ($(findstring clang,$(CXX)),clang)
	CXXFLAGS       := -stdlib=libc++
	LDFLAGS        := -stdlib=libc++
endif

.DEFAULT_GOAL := run
.PHONY: configure deps build rebuild clean install run test all wokiext wokiext-install extensions lsp \
	configure-web build-web web

RUN_ARGS       := $(if $(CONFIG),--config $(CONFIG),)

configure:
	@echo "→ Configuring native ($(BUILD_TYPE))..."
	@echo "  Wasmtime: $(WOKI_EXTENSION_WITH_WASMTIME) (provider=$(WASMTIME_PROVIDER))"
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DCMAKE_CXX_COMPILER=$(CXX) \
		-DCMAKE_CXX_FLAGS="$(CXXFLAGS)" \
		-DWOKI_EXTENSION_WITH_WASMTIME=$(WOKI_EXTENSION_WITH_WASMTIME) \
		-DWOKI_WASMTIME_PROVIDER=$(WASMTIME_PROVIDER)

deps: configure
	@echo "→ Fetching and building third-party dependencies..."
	@cmake --build $(BUILD_DIR) --target woki_thirdparty_deps -j$(NUM_JOBS)

lsp: configure
	@ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json
	@echo "✓ LSP: compile_commands.json -> $(BUILD_DIR)/compile_commands.json"

build: deps
	@echo "→ Building native..."
	@cmake --build $(BUILD_DIR) -j$(NUM_JOBS)
	@echo "✓ Native build complete"

run: build
	@echo "→ Running studio (native)..."
	@cd $(BUILD_DIR)/bin && ./studio $(RUN_ARGS)

test: build
	@echo "→ Running tests..."
	@ctest --test-dir $(BUILD_DIR) --output-on-failure

wokiext: deps
	@echo "→ Building wokiext..."
	@cmake --build $(BUILD_DIR) --target wokiext -j$(NUM_JOBS)
	@echo "✓ wokiext build complete"

wokiext-install: wokiext
	@echo "→ Installing wokiext to $(PREFIX)..."
	@cmake --install $(BUILD_DIR) --prefix $(PREFIX)

extensions: deps
	@echo "→ Building extensions..."
	@cmake --build $(BUILD_DIR) --target woki_extensions -j$(NUM_JOBS)
	@echo "✓ Extensions build complete"

install:
	@echo "→ Installing native to $(PREFIX)..."
	@cmake --install $(BUILD_DIR) --prefix $(PREFIX)
	@echo "✓ Install complete"

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
	@echo "✓ Web build complete"

web: build-web
	@echo "→ Running web (emrun)..."
	@emrun --no_browser --hostname localhost --port $(WEB_PORT) $(WEB_BUILD_DIR)
	@echo "Open: http://localhost:$(WEB_PORT)/"

clean:
	@echo "→ Cleaning build output..."
	@rm -rf $(BUILD_DIR) $(WEB_BUILD_DIR) Testing/
	@echo "✓ Clean complete"
