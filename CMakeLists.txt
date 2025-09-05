cmake_minimum_required(VERSION 3.15)
project(aymaraLang CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  add_compile_options(/W4 /permissive-)
  add_compile_options($<$<CONFIG:Release>:/O2>)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
  add_compile_options($<$<CONFIG:Release>:-O2>)
endif()

file(GLOB_RECURSE AYM_SOURCES CONFIGURE_DEPENDS
  "compiler/*.cpp"
)

add_executable(aymc ${AYM_SOURCES})

# Put binaries into bin/ inside the build directory
set_target_properties(aymc PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

