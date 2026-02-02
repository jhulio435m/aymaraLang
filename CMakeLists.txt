cmake_minimum_required(VERSION 3.15)
project(aymaraLang CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(AYM_ENABLE_LLVM "Enable LLVM backend" ON)
set(AYM_VERSION "0.1.0")

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

include(GNUInstallDirs)

install(TARGETS aymc
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY runtime/
  DESTINATION ${CMAKE_INSTALL_DATADIR}/aymaraLang/runtime
  FILES_MATCHING PATTERN "*.c" PATTERN "README.md")

if(AYM_ENABLE_LLVM)
  find_package(LLVM QUIET CONFIG)
  if(LLVM_FOUND)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    set(AYM_LLVM_COMPONENTS core support)
    llvm_map_components_to_libnames(AYM_LLVM_LIBS ${AYM_LLVM_COMPONENTS})
    target_include_directories(aymc PRIVATE ${LLVM_INCLUDE_DIRS})
    target_compile_definitions(aymc PRIVATE ${LLVM_DEFINITIONS} AYM_WITH_LLVM)
    target_link_libraries(aymc PRIVATE ${AYM_LLVM_LIBS})
  else()
    message(WARNING "LLVM solicitado pero no encontrado; compilando sin backend LLVM.")
  endif()
else()
  message(STATUS "LLVM backend deshabilitado mediante AYM_ENABLE_LLVM=OFF")
endif()

# Put binaries into bin/ inside the build directory
set_target_properties(aymc PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(CPACK_PACKAGE_NAME "AymaraLang")
set(CPACK_PACKAGE_VENDOR "AymaraLang")
set(CPACK_PACKAGE_VERSION ${AYM_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Compilador AymaraLang (aymc)")
set(CPACK_PACKAGE_CONTACT "AymaraLang")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "AymaraLang")

if(WIN32)
  set(CPACK_GENERATOR "NSIS")
  set(CPACK_NSIS_DISPLAY_NAME "AymaraLang")
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
  set(CPACK_NSIS_MODIFY_PATH ON)
  set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
  set(CPACK_NSIS_PACKAGE_NAME "AymaraLang")
  set(CPACK_PACKAGE_EXECUTABLES "aymc" "AymaraLang Compiler")
else()
  set(CPACK_GENERATOR "TGZ;DEB;RPM")
  set(CPACK_PACKAGE_FILE_NAME "AymaraLang-${AYM_VERSION}-${CMAKE_SYSTEM_NAME}")
endif()

if(APPLE)
  set(CPACK_GENERATOR "DragNDrop")
endif()

include(CPack)
