# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)
project(Llamaar)

# Path to the llama.cpp submodule
set(LLAMA_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../llama-cpp)

# Build ONLY the core llama and ggml libraries
add_subdirectory(${LLAMA_CPP_DIR} llama_cpp_build)

# Compile our minimal JNI bridge
add_library(llamaar SHARED llama-bridge.cpp)

# Link everything together
target_link_libraries(llamaar llama log android)