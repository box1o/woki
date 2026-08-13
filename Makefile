BUILD_DIR      := build
BUILD_TYPE     := Release
NUM_JOBS       ?= $(shell getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)
CONFIG         ?= config/conf/studio.yaml
PREFIX         ?= /usr/local

# Use Clang with libc++ to match the Dawn dependency
CXX            := $(shell command -v clang++ 2>/dev/null || command -v c++ 2>/dev/null || echo c++)
ifeq ($(findstring clang,$(CXX)),clang)
	CXXFLAGS       := -stdlib=libc++
	LDFLAGS        := -stdlib=libc++
endif

.DEFAULT_GOAL := run
.PHONY: configure build rebuild clean install run ll format format-check

RUN_ARGS       := $(if $(CONFIG),--config $(CONFIG),)

configure:
	@echo "→ Configuring native ($(BUILD_TYPE))..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -Wno-deprecated .. \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX) \
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

format:
	@python3 scripts/format.py

format-check:
	@python3 scripts/format.py --check

clean:
	@echo "→ Cleaning build output..."
	@rm -rf build build-* 
	@echo "✓ Clean complete"
