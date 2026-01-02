TARGET = ChessGame
BUILD_DIR = out
SRC_DIR = src
CPP_FILES := $(shell find $(SRC_DIR) -type f -name '*.cpp')
H_FILES := $(shell find $(SRC_DIR) -type f -name '*.h')

CMAKE_FLAGS = -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
MAKE_FLAGS = -j4

.PHONY: all
all: run

.PHONY: run
run: build-release
	cd $(BUILD_DIR) && ./$(TARGET)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# ------
# Build
# ------

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: build-debug
build-debug: $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Debug .. && $(MAKE) $(MAKE_FLAGS)

.PHONY: build-release
build-release: $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Release .. && $(MAKE) $(MAKE_FLAGS)

.PHONY: build-profile
build-profile: $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=RelWithDebInfo .. && $(MAKE) $(MAKE_FLAGS)

# ---------
# Formatting
# ---------

.PHONY: format
format:
	clang-format -i $(CPP_FILES) $(H_FILES)

.PHONY: tidy
tidy: $(BUILD_DIR)/Makefile
	cd $(BUILD_DIR) && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
	clang-tidy $(CPP_FILES) $(H_FILES) --fix -p $(BUILD_DIR) -- -x c++

# ---------
# Debugging
# ---------

.PHONY: run-debug
run-debug: build-debug
	cd $(BUILD_DIR) && gdb ./$(TARGET)

.PHONY: debug
debug: build-debug run-debug

.PHONY: valgrind
valgrind: build-debug
	cd $(BUILD_DIR) && valgrind --leak-check=full --error-limit=no ./$(TARGET) > valgrind_output.txt 2>&1

# ---------
# Profiling
# ---------

.PHONY: callgrind
callgrind: build-profile
	cd $(BUILD_DIR) && valgrind --tool=callgrind ./$(TARGET)

.PHONY: callgrind-view
callgrind-view:
	kcachegrind ./$(BUILD_DIR)/callgrind.out.*
