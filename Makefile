BUILD_DIR := bin
API_BINARY := $(BUILD_DIR)/woki-api
CLI_BINARY := $(BUILD_DIR)/woki
GO_PACKAGES := ./cmd/... ./cli/... ./internal/... ./pkg/... ./tests/...

MAKEFLAGS += --silent

.DEFAULT_GOAL := help

.PHONY: help run build api cli cli-run dev-api dev-web web-install web-lint web-build web-check infra-up infra-down fmt fmt-check vet test test-race test-shuffle integration check

help:
	@printf '%s\n' \
		'Woki development commands:' \
		'  make infra-up       Start PostgreSQL and Redis' \
		'  make run            Run the API' \
		'  make cli-run        Build and run the CLI' \
		'  make dev-api        Run the API for local development' \
		'  make dev-web        Run the Vite development server' \
		'  make build          Build API and CLI binaries into bin/' \
		'  make web-install    Install web dependencies' \
		'  make web-check      Lint and build the web application' \
		'  make check          Run formatting, Go, and web checks'

run:
	@go run -buildvcs=false ./cmd/api

api:
	@mkdir -p $(BUILD_DIR)
	@go build -buildvcs=false -trimpath -o $(API_BINARY) ./cmd/api

cli:
	@mkdir -p $(BUILD_DIR)
	@go build -buildvcs=false -trimpath -o $(CLI_BINARY) ./cli/cmd/woki

build: api cli

cli-run: cli
	@$(CLI_BINARY)

dev-api: run

dev-web:
	@npm --prefix web run dev

web-install:
	@npm --prefix web ci

web-lint:
	@npm --prefix web run lint

web-build:
	@npm --prefix web run build

web-check: web-lint web-build

infra-up:
	@docker compose up -d postgres redis

infra-down:
	@docker compose down

fmt:
	@gofmt -w $$(find . -name '*.go' -not -path './web/*')

fmt-check:
	@files="$$(gofmt -l $$(find . -name '*.go' -not -path './web/*'))"; \
	if [ -n "$$files" ]; then \
		printf '%s\n' "Go files need formatting:"; \
		printf '%s\n' "$$files"; \
		exit 1; \
	fi

vet:
	@go vet $(GO_PACKAGES)

test:
	@go test $(GO_PACKAGES)

test-race:
	@go test -race $(GO_PACKAGES)

test-shuffle:
	@go test -shuffle=on -count=5 $(GO_PACKAGES)

integration:
	@WOKI_TEST_INTEGRATION=1 go test -race -count=1 ./tests/integration

check: fmt-check vet test web-check
