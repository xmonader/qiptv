# kiptv — Makefile wraps CMake and packaging. Primary developer interface.
BUILD_DIR := build

.PHONY: all build configure test lint run install validate deb deb-lint clean

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

# Validate the desktop entry and AppStream metadata (packaging prerequisites).
validate:
	@desktop-file-validate data/io.github.xmonader.kiptv.desktop && echo "desktop: OK"
	@appstreamcli validate --no-net data/io.github.xmonader.kiptv.metainfo.xml

# Build the Debian package -> ../kiptv_<version>_amd64.deb
deb:
	@command -v dpkg-buildpackage >/dev/null 2>&1 || { echo "dpkg-buildpackage not found; install dpkg-dev debhelper"; exit 1; }
	@dpkg-buildpackage -b -us -uc
	@echo "Built: $$(ls -1t ../kiptv_*_amd64.deb 2>/dev/null | head -1)"

# Run the Debian policy linter against the most recent build (optional tool).
deb-lint:
	@if command -v lintian >/dev/null 2>&1; then \
		lintian $$(ls -1t ../kiptv_*_amd64.deb 2>/dev/null | head -1); \
	else \
		echo "lintian not installed (apt install lintian)"; \
	fi

clean:
	@rm -rf $(BUILD_DIR) obj-*-linux-gnu \
		debian/kiptv debian/.debhelper debian/files \
		debian/*.substvars debian/*.debhelper.log debian/debhelper-build-stamp
