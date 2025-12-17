TARGET = ChessGame
BUILD_DIR = out
SRC_DIR = src
CPP_FILES := $(shell find $(SRC_DIR) -type f -name '*.cpp')
H_FILES := $(shell find $(SRC_DIR) -type f -name '*.h')

CMAKE_FLAGS = -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
DEBUG_FLAGS = -DCMAKE_BUILD_TYPE=Debug \
              -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -O1"
RELEASE_FLAGS = -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_CXX_FLAGS="-O3 -march=native -DNDEBUG"
PROFILE_FLAGS = -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_CXX_FLAGS="-O2 -pg" \
                -DCMAKE_EXE_LINKER_FLAGS="-pg"
MAKE_FLAGS = -j4

.PHONY: all
all: run

# ------
# Build
# ------

$(BUILD_DIR)/Makefile: CMakeLists.txt | $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) $(RELEASE_FLAGS) ..

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: build
build: $(BUILD_DIR)/Makefile
	cd $(BUILD_DIR) && $(MAKE) $(MAKE_FLAGS)

.PHONY: run
run: build
	cd $(BUILD_DIR) && ./$(TARGET)

.PHONY: run-console
run-console: build
	cd $(BUILD_DIR) && ./$(TARGET) -nogui

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

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

.PHONY: build-debug
build-debug: $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) $(DEBUG_FLAGS) .. && $(MAKE) $(MAKE_FLAGS)

.PHONY: run-debug
run-debug: build-debug
	cd $(BUILD_DIR) && gdb ./$(TARGET)

.PHONY: debug
debug: build-debug run-debug

.PHONY: valgrind
valgrind: build
	cd $(BUILD_DIR) && valgrind --leak-check=full --error-limit=no ./$(TARGET) > valgrind_output.txt 2>&1

.PHONY: build-profile
build-debug: $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) $(PROFILE_FLAGS) .. && $(MAKE) $(MAKE_FLAGS)

.PHONY: callgrind
callgrind: build-profile
	cd $(BUILD_DIR) && valgrind --tool=callgrind ./$(TARGET)

.PHONY: callgrind-view
callgrind-view:
	kcachegrind ./$(BUILD_DIR)/callgrind.out.*
