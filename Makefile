# Makefile for the C++23 fire_n_go Project
# This Makefile includes automatic dependency generation to correctly recompile
# files when headers are changed.

# --- Compiler and Flags ---
CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -g -O2

# --- Dependency Generation Flags ---
# -MMD: Generate dependency files (.d) for user headers.
# -MP:  Create phony targets for headers to prevent errors if a header is deleted.
DEPFLAGS = -MMD -MP


# --- Optional Features ---

# To enable debug-level logs, uncomment the following line.
# CXXFLAGS += -DFNGO_DEBUG_LOGS

# To enable std::stacktrace on errors, set STACKTRACE to 1.
# Example: make STACKTRACE=1
STACKTRACE ?= 0


# --- Project Files ---
SRCS = main.cpp fire_n_go.cpp
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d) # These are the dependency files we will generate.


# --- Executable Name ---
EXECUTABLE_NAME = test_fngo


# --- Platform-Specific Configuration ---
# Default to Linux/Unix settings.
LDFLAGS = -pthread
STACKTRACE_LDFLAG = -lstdc++_libbacktrace # For GCC 13 and older
EXECUTABLE = $(EXECUTABLE_NAME)
RM = rm -f

# Check if the OS is Windows NT.
ifeq ($(OS),Windows_NT)
    # Windows-specific settings
    LDFLAGS =
    # GCC 14+ on Windows uses -lstdc++exp for stacktrace support.
    STACKTRACE_LDFLAG = -lstdc++exp
    EXECUTABLE = $(EXECUTABLE_NAME).exe
    RM = del /Q
endif

# Add stacktrace flags only if the feature is enabled.
ifeq ($(STACKTRACE),1)
    CXXFLAGS += -DUSE_STACKTRACE=1
    LDFLAGS += $(STACKTRACE_LDFLAG)
endif


# --- Build Targets ---

# The default target.
all: $(EXECUTABLE)

# Rule to link the final executable.
$(EXECUTABLE): $(OBJS)
	@echo "Linking executable: $@"
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build finished successfully."

# Include the generated dependency files.
-include $(DEPS)

# Pattern rule to compile .cpp files into .o object files.
%.o: %.cpp
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Target to run the compiled application.
run: all
	@echo "Running application..."
	./$(EXECUTABLE)

# Target to clean up the build directory.
clean:
	@echo "Cleaning up project files..."
	-$(RM) $(OBJS) $(DEPS)
	-$(RM) $(EXECUTABLE)
	@echo "Cleanup complete."

# Phony targets are ones that don't represent actual files.
.PHONY: all clean run

