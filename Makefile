.PHONY: build test lint clean run

build:
	cargo build

test:
	cargo test

lint:
	cargo clippy -- -D warnings

clean:
	cargo clean

run:
	cargo run
