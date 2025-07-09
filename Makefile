# Makefile for the C++23 fire_n_go Project

# --- Compiler and Flags ---
# Use g++ as the C++ compiler.
CXX = g++

# Set the C++ standard and enable all common warnings.
# -g adds debugging symbols, which is useful for gdb and stack traces.
# -O2 is a standard optimization level for release builds.
CXXFLAGS = -std=c++23 -Wall -Wextra -g -O2

# --- Dependency Generation Flags ---
# -MMD generates dependency files (.d) for user headers.
# -MP creates phony targets for headers to prevent errors if a header is deleted.
DEPFLAGS = -MMD -MP


# --- Optional Features ---

# To enable debug-level logs, uncomment the following line.
# This is independent of the stack trace feature.
# CXXFLAGS += -DENABLE_DEBUG_LOGS=1

# To enable std::stacktrace on errors, set STACKTRACE to 1.
# Example: make STACKTRACE=1
# Note: The logger will only print stack traces if ENABLE_DEBUG_LOGS is also active.
STACKTRACE ?= 0


# --- Project Files ---
# List all your .cpp source files here.
SRCS = main.cpp fire_n_go.cpp

# Automatically generate object file and dependency file names.
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

# --- Executable Name ---
EXECUTABLE_NAME = test_fngo


# --- Platform-Specific Configuration ---
# This section automatically detects the OS and sets the appropriate
# compiler flags, commands, and executable names.

# Default to Linux/Unix settings.
LDFLAGS = -pthread
STACKTRACE_LDFLAG = -lstdc++_libbacktrace # For GCC 13 and older
EXECUTABLE = $(EXECUTABLE_NAME)
RM = rm -f

# Check if the OS is Windows NT.
# This is a common environment variable set on Windows systems.
ifeq ($(OS),Windows_NT)
    # Windows-specific settings (for MinGW-w64 or similar)
    LDFLAGS =
    # GCC 14+ on Windows uses -lstdc++exp for stacktrace support.
    STACKTRACE_LDFLAG = -lstdc++exp
    EXECUTABLE = $(EXECUTABLE_NAME).exe
    # Use the 'del' command to clean files on Windows.
    RM = del /Q
endif

# Add stacktrace flags only if the feature is enabled.
ifeq ($(STACKTRACE),1)
    CXXFLAGS += -DUSE_STACKTRACE=1
    LDFLAGS += $(STACKTRACE_LDFLAG)
endif


# --- Build Targets ---

# The default target, executed when you just type 'make'.
# It depends on the final executable.
all: $(EXECUTABLE)

# Rule to link the final executable from the object files.
# $@ is an automatic variable representing the target name (the executable).
# $^ is an automatic variable representing all prerequisites (the object files).
$(EXECUTABLE): $(OBJS)
	@echo "Linking executable: $@"
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build finished successfully."

# Include the generated dependency files. The '-' before include tells make
# to ignore errors if the file doesn't exist (e.g., on a clean build).
-include $(DEPS)

# Pattern rule to compile .cpp files into .o object files.
# This now includes the DEPFLAGS to generate dependency information.
# $< is an automatic variable representing the first prerequisite (the .cpp file).
%.o: %.cpp
	@echo "Compiling: $<"
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Target to run the compiled application.
run: all
	@echo "Running application..."
	./$(EXECUTABLE)

# Target to clean up the build directory.
# It now also removes the dependency files (.d).
clean:
	@echo "Cleaning up project files..."
	-$(RM) $(OBJS) $(DEPS)
	-$(RM) $(EXECUTABLE)
	@echo "Cleanup complete."

# Phony targets are ones that don't represent actual files.
# This prevents 'make' from getting confused if a file named 'clean' exists.
.PHONY: all clean run

