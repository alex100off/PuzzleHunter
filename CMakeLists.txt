cmake_minimum_required(VERSION 3.18)

# Статическая линковка CUDA Runtime: exe не требует cudart64_*.dll
set(CMAKE_CUDA_RUNTIME_LIBRARY Static)

project(PuzzleHunter LANGUAGES CXX CUDA)

# Разрешаем использование новых версий компилятора MSVC (обход проверки версии)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -allow-unsupported-compiler")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)

# Поддержка нескольких поколений GPU + PTX для будущих (JIT)
set(CMAKE_CUDA_ARCHITECTURES 61 75 86 89)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -gencode arch=compute_86,code=compute_86")

# Подключаем зависимости
add_subdirectory(external/secp256k1)
add_subdirectory(external/vanity)

# ==========================================
# Основной проект
# ==========================================
add_executable(PuzzleHunter
  src/main.cpp
  src/gui.cpp
  src/bitcoin.cpp
  src/hash.cpp
  src/multiset_permutation.cpp
  src/targets.cpp
  src/cuda_batch.cu
  src/gpu_solver_v2.cpp
)

target_include_directories(PuzzleHunter PRIVATE external/secp256k1/include)

target_link_libraries(PuzzleHunter
    PRIVATE secp256k1
    PRIVATE vanity_gpu
)

# ==========================================
# Dear ImGui
# ==========================================
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_link_libraries(imgui PUBLIC d3d11.lib)
target_link_libraries(PuzzleHunter PRIVATE imgui)