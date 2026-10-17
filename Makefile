BUILD_DIR := build
BUILD_DIR_WEB := build-web

.DEFAULT_GOAL := run
.PHONY: build run build-web run-web


# ================================================================================
# Native build
# ================================================================================

build:
	@echo "-Building the project-"
	@mkdir -p $(BUILD_DIR)
	@cmake -S . -B $(BUILD_DIR)
	@cmake --build $(BUILD_DIR)

run: build
	@echo "-Running the project-"
	@./$(BUILD_DIR)/studio/studio


# ================================================================================
# WEB emcmake build config
# ================================================================================

build-web:
	@echo "-Building web version-"
	@mkdir -p $(BUILD_DIR_WEB)
	@emcmake cmake -S . -B $(BUILD_DIR_WEB)
	@cmake --build $(BUILD_DIR_WEB)

run-web: build-web
	@echo "-Running web (emrun)... -"
	@emrun --no_browser --hostname localhost --port 8080 $(BUILD_DIR_WEB)/studio/studio.html
	@echo "Open: http://localhost:8080/studio.html"
