BUILD_DIR     := build
WEB_BUILD_DIR := build-web
BUILD_TYPE    := Release
WEB_PORT      ?= 8080
PREFIX        ?= /usr/local

NUM_JOBS ?= $(shell getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

CXX := $(shell command -v clang++ 2>/dev/null || command -v c++ 2>/dev/null || echo c++)

ifeq ($(findstring clang,$(CXX)),clang)
	CXXFLAGS := -stdlib=libc++
endif

.DEFAULT_GOAL := run

.PHONY: \
	configure build run \
	configure-web build-web web web-run \
	install rebuild debug release all \
	format format-check clean


configure:
	@echo "→ Configuring native ($(BUILD_TYPE))..."
	@cmake -S . -B $(BUILD_DIR) -Wno-deprecated \
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
	@echo "→ Running studio..."
	@$(BUILD_DIR)/bin/studio


configure-web:
	@echo "→ Configuring web ($(BUILD_TYPE))..."
	@emcmake cmake -S . -B $(WEB_BUILD_DIR) -Wno-deprecated \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

build-web: configure-web
	@echo "→ Building web..."
	@cmake --build $(WEB_BUILD_DIR) --target studio -j$(NUM_JOBS)
	@echo "✓ Web build complete"

web: build-web

web-run: build-web
	@echo "→ http://localhost:$(WEB_PORT)/"
	@emrun --no_browser --port $(WEB_PORT) $(WEB_BUILD_DIR)/index.html


install: build
	@cmake --install $(BUILD_DIR) --prefix $(PREFIX)

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
	@echo "→ Cleaning..."
	@rm -rf $(BUILD_DIR) $(WEB_BUILD_DIR)
	@echo "✓ Clean complete"
