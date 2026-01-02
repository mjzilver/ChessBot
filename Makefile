TARGET = ChessGame
BUILD_DIR = out
SRC_DIR = src
CPP_FILES := $(shell find $(SRC_DIR) -type f -name '*.cpp')
H_FILES := $(shell find $(SRC_DIR) -type f -name '*.h')

CMAKE_FLAGS = -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
DEBUG_FLAGS   = -DCMAKE_BUILD_TYPE=Debug \
                -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -O1"
RELEASE_FLAGS = -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_CXX_FLAGS="-O3 -march=native -DNDEBUG"
PROFILE_FLAGS = -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_CXX_FLAGS="-O2 -pg" \
                -DCMAKE_EXE_LINKER_FLAGS="-pg"
MAKE_FLAGS = -j4

# ------------------------
# Build Setup
# ------------------------

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/Makefile: CMakeLists.txt | $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) ..

.PHONY: build
build: $(BUILD_DIR)/Makefile
	cd $(BUILD_DIR) && $(MAKE) $(MAKE_FLAGS)

.PHONY: build-debug
build-debug: $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) $(DEBUG_FLAGS) .. && $(MAKE) $(MAKE_FLAGS)

.PHONY: build-release
build-release: $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) $(RELEASE_FLAGS) .. && $(MAKE) $(MAKE_FLAGS)

.PHONY: build-profile
build-profile: $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) $(PROFILE_FLAGS) .. && $(MAKE) $(MAKE_FLAGS)

# ------------------------
# Run
# ------------------------

.PHONY: run
run: build-release
	cd $(BUILD_DIR) && ./$(TARGET)

.PHONY: run-console
run-console: build
	cd $(BUILD_DIR) && ./$(TARGET) -nogui

.PHONY: run-debug
run-debug: build-debug
	cd $(BUILD_DIR) && \
	ASAN_OPTIONS=detect_leaks=1:abort_on_error=0:symbolize=0:malloc_context_size=50:verbosity=0:log_path=asan.log \
	LSAN_OPTIONS=verbosity=0 \
	./$(TARGET)

.PHONY: debug
debug: run-debug

# ------------------------
# Clean
# ------------------------

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# ------------------------
# Formatting / Linting
# ------------------------

.PHONY: format
format:
	clang-format -i $(CPP_FILES) $(H_FILES)

.PHONY: tidy
tidy: $(BUILD_DIR)/Makefile
	cd $(BUILD_DIR) && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
	clang-tidy $(CPP_FILES) $(H_FILES) --fix -p $(BUILD_DIR) -- -x c++

# ------------------------
# Profiling / Valgrind
# ------------------------

.PHONY: valgrind
valgrind: build-debug
	cd $(BUILD_DIR) && valgrind --leak-check=full --error-limit=no ./$(TARGET) > valgrind_output.txt 2>&1

.PHONY: callgrind
callgrind: build-profile
	cd $(BUILD_DIR) && valgrind --tool=callgrind ./$(TARGET)

.PHONY: callgrind-view
callgrind-view:
	kcachegrind ./$(BUILD_DIR)/callgrind.out.*

# ------------------------
# ThreadSanitizer (TSAN)
# ------------------------

TSAN_FLAGS = -fsanitize=thread -fPIE -g
TSAN_LINKER_FLAGS = -fsanitize=thread

.PHONY: build-tsan
build-tsan: $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="$(TSAN_FLAGS)" -DCMAKE_LINKER_FLAGS="$(TSAN_LINKER_FLAGS)" .. && $(MAKE) $(MAKE_FLAGS)

.PHONY: tsan
tsan: build-tsan
	cd $(BUILD_DIR) && ./$(TARGET)

# ------------------------
# Perf profiling
# ------------------------

.PHONY: perf
perf: build-release
	cd $(BUILD_DIR) && perf stat -r 5 ./$(TARGET)

.PHONY: perf-record
perf-record: build-release
	cd $(BUILD_DIR) && perf record -o perf.data ./$(TARGET)

.PHONY: perf-report
perf-report:
	perf report -i $(BUILD_DIR)/perf.data
