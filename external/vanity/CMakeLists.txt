cmake_minimum_required(VERSION 3.18)

# Находим CUDA toolkit (нужно для target CUDA::cudart)
find_package(CUDAToolkit REQUIRED)

# CPU-исходники VanitySearch
set(VANITY_CPU_SOURCES
    Int.cpp
    IntGroup.cpp
    IntMod.cpp
    Point.cpp
    SECP256K1.cpp
    Random.cpp
    Timer.cpp
    Base58.cpp
    Bech32.cpp
)

# Добавляем наш проверенный модуль хеширования
list(APPEND VANITY_CPU_SOURCES ${CMAKE_SOURCE_DIR}/src/hash.cpp)
list(APPEND VANITY_CPU_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/hash_wrappers.cpp)

# GPU-исходники VanitySearch
set(VANITY_GPU_SOURCES
    GPU/GPUEngine.cu
    GPU/GPUGenerate.cpp
)

# Создаём статическую библиотеку
add_library(vanity_gpu STATIC
    ${VANITY_CPU_SOURCES}
    ${VANITY_GPU_SOURCES}
)

# Указываем include-директории
target_include_directories(vanity_gpu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/GPU
    ${CMAKE_CURRENT_SOURCE_DIR}/hash
)

# Настройки CUDA - ИСПРАВЛЕНО: 86 вместо 50!
set_target_properties(vanity_gpu PROPERTIES
    CUDA_SEPARABLE_COMPILATION OFF
    CUDA_ARCHITECTURES 86
)

# Линкуем CUDA runtime
target_link_libraries(vanity_gpu
    PRIVATE CUDA::cudart
)