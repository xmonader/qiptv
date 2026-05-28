# kiptv — Makefile wraps CMake. Primary developer interface.
BUILD_DIR := build

.PHONY: all build configure test lint clean run install

all: build

configure:
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

build: configure
	@cmake --build $(BUILD_DIR) -j$(shell nproc)

test: build
	@ctest --test-dir $(BUILD_DIR) --output-on-failure

# Static analysis: clang-tidy over the compile database when available.
lint:
	@if command -v clang-tidy >/dev/null 2>&1 && [ -f $(BUILD_DIR)/compile_commands.json ]; then \
		find src -name '*.cpp' | xargs clang-tidy -p $(BUILD_DIR) 2>/dev/null || true; \
	else \
		echo "clang-tidy or compile_commands.json missing; run 'make build' first"; \
	fi

run: build
	@./$(BUILD_DIR)/bin/kiptv

install: build
	@cmake --install $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR)
