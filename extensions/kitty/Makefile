BUILD_DIR ?= build
BUILD_TYPE ?= Release
NUM_JOBS ?= $(shell nproc)

.DEFAULT_GOAL := build
.PHONY: configure build lsp verify bundle install clean

configure:
	@cmake -B $(BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	@cmake --build $(BUILD_DIR) -j$(NUM_JOBS)

lsp: configure
	@ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json
	@echo "✓ LSP: compile_commands.json -> $(BUILD_DIR)/compile_commands.json"

verify: build
	@wokiext verify .

bundle: verify
	@wokiext bundle .

install: bundle
	@wokiext install .

clean:
	@rm -rf $(BUILD_DIR) extension.wasm *.wokiext
