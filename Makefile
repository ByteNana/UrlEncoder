BUILD_DIR := /tmp/UrlEncoder-build
SOURCE_DIR := $(shell pwd)

# clang-tidy needs help finding the standard library headers.
# On macOS, point at the Xcode SDK; on Linux, point at the GCC toolchain.
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  CLANG_TIDY_SYSROOT := --extra-arg=-isysroot --extra-arg=$(shell xcrun --show-sdk-path 2>/dev/null)
else
  CLANG_TIDY_SYSROOT := --extra-arg=--gcc-toolchain=/usr
endif

.PHONY: all build test examples check lint clean

all: build

build:
	cmake -S $(SOURCE_DIR) --preset default
	cmake --build $(BUILD_DIR)

test:
	cmake -S $(SOURCE_DIR) --preset test
	cmake --build $(BUILD_DIR)
	cd $(BUILD_DIR) && ctest --output-on-failure

examples:
	pio ci examples/basic/src/main.cpp --lib="." --board=esp32dev

check:
	find src examples -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror

lint:
	@test -f $(BUILD_DIR)/compile_commands.json || (echo "Run 'make build' first to generate compile_commands.json" && exit 1)
	find src -name "*.cpp" | xargs clang-tidy -p $(BUILD_DIR) $(CLANG_TIDY_SYSROOT)

clean:
	rm -rf $(BUILD_DIR)
