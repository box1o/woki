BUILD_DIR ?= build
BUILD_TYPE ?= Release
PREFIX ?= /usr/local
NUM_JOBS ?= $(shell nproc)

.DEFAULT_GOAL := build
.PHONY: configure build install clean test

configure:
	@cmake -B $(BUILD_DIR) -S .. \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX)

build: configure
	@cmake --build $(BUILD_DIR) --target wokiext -j$(NUM_JOBS)

install: build
	@cmake --install $(BUILD_DIR) --prefix $(PREFIX)

test: build
	@$(BUILD_DIR)/ext/wokiext --help

clean:
	@rm -rf $(BUILD_DIR)
