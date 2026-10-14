BUILD_DIR := build

.DEFAULT_GOAL := run
.PHONY: build run 

	
build:
	@echo "-Building the project-"
	@mkdir -p $(BUILD_DIR)
	@cmake -S . -B $(BUILD_DIR)
	@cmake --build $(BUILD_DIR)

run: build
	@echo "-Running the project-"
	@./$(BUILD_DIR)/studio/studio
