cmake_minimum_required(VERSION 3.16)

project(computacion_avanzada_cartoon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Homebrew instala SFML 2 y libomp como paquetes "keg-only". Estas rutas
# permiten que CMake los encuentre sin modificar variables globales del sistema.
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH
        "/opt/homebrew/opt/sfml@2"
        "/opt/homebrew/opt/libomp"
        "/usr/local/opt/sfml@2"
        "/usr/local/opt/libomp"
    )
endif()

# Las implementaciones originales usan la API de SFML 2.x.
find_package(SFML 2.5 COMPONENTS graphics REQUIRED)

if((DEFINED SFML_VERSION AND SFML_VERSION VERSION_GREATER_EQUAL "3.0") OR
   (DEFINED SFML_VERSION_MAJOR AND SFML_VERSION_MAJOR GREATER_EQUAL 3))
    message(FATAL_ERROR
        "Este proyecto requiere SFML 2.x. En Homebrew use: brew install sfml@2"
    )
endif()

find_package(MPI REQUIRED COMPONENTS CXX)
find_package(OpenMP COMPONENTS CXX)

# Apple Clang necesita indicar explícitamente el encabezado y la biblioteca
# de OpenMP. En Linux, normalmente alcanza con FindOpenMP de CMake.
if(NOT OpenMP_CXX_FOUND AND APPLE)
    find_path(OPENMP_INCLUDE_DIRECTORY omp.h
        HINTS
            "/opt/homebrew/opt/libomp/include"
            "/usr/local/opt/libomp/include"
        REQUIRED
    )

    find_library(OPENMP_LIBRARY omp
        HINTS
            "/opt/homebrew/opt/libomp/lib"
            "/usr/local/opt/libomp/lib"
        REQUIRED
    )

    add_library(OpenMP::OpenMP_CXX INTERFACE IMPORTED)
    target_compile_options(OpenMP::OpenMP_CXX INTERFACE
        -Xpreprocessor -fopenmp
    )
    target_include_directories(OpenMP::OpenMP_CXX INTERFACE
        "${OPENMP_INCLUDE_DIRECTORY}"
    )
    target_link_libraries(OpenMP::OpenMP_CXX INTERFACE
        "${OPENMP_LIBRARY}"
    )
elseif(NOT OpenMP_CXX_FOUND)
    message(FATAL_ERROR "No se encontró una instalación de OpenMP para C++")
endif()

add_executable(secuencial src/secuencial.cpp)
target_link_libraries(secuencial PRIVATE sfml-graphics)

add_executable(mpi src/mpi.cpp)
target_link_libraries(mpi PRIVATE sfml-graphics MPI::MPI_CXX)

add_executable(openmp src/openmp.cpp)
target_link_libraries(openmp PRIVATE sfml-graphics OpenMP::OpenMP_CXX)

add_executable(hibrido src/hibrido.cpp)
target_link_libraries(hibrido PRIVATE sfml-graphics MPI::MPI_CXX OpenMP::OpenMP_CXX)

