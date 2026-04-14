cmake_minimum_required(VERSION 3.15)
project(aymaraLang CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)

option(AYM_ENABLE_UNIT_TESTS "Compila pruebas unitarias C++ con GoogleTest" ON)
option(AYM_FETCH_GTEST "Descarga GoogleTest si no se encuentra instalado" ON)
option(AYM_ENABLE_COVERAGE "Habilita instrumentacion de cobertura (gcov/gcovr) en toolchains GNU/Clang" OFF)

set(AYM_VERSION_FILE "${CMAKE_SOURCE_DIR}/VERSION.txt")
if(EXISTS "${AYM_VERSION_FILE}")
  file(READ "${AYM_VERSION_FILE}" AYM_VERSION)
  string(STRIP "${AYM_VERSION}" AYM_VERSION)
else()
  set(AYM_VERSION "0.1.0")
endif()

if(MSVC)
  add_compile_options(/W4 /permissive-)
  add_compile_options($<$<CONFIG:Release>:/O2>)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
  add_compile_options($<$<CONFIG:Release>:-O2>)
endif()

if(AYM_ENABLE_COVERAGE)
  if(MSVC)
    message(FATAL_ERROR "AYM_ENABLE_COVERAGE no soportado con MSVC.")
  endif()
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "AYM_ENABLE_COVERAGE requiere GCC o Clang.")
  endif()
  add_compile_options(-O0 -g --coverage)
  add_link_options(--coverage)
endif()

file(GLOB_RECURSE AYM_SOURCES CONFIGURE_DEPENDS
  "compiler/*.cpp"
)

add_executable(aymc ${AYM_SOURCES})

set(AYM_WRAPPER_SOURCES
  "tools/aym/main.cpp"
  "compiler/utils/project_tool.cpp"
  "compiler/utils/project_manifest.cpp"
  "compiler/utils/semver.cpp"
)

add_executable(aym ${AYM_WRAPPER_SOURCES})
target_include_directories(aym PRIVATE ${CMAKE_SOURCE_DIR})

if(BUILD_TESTING AND AYM_ENABLE_UNIT_TESTS)
  file(GLOB_RECURSE AYM_UNIT_TEST_SOURCES CONFIGURE_DEPENDS
    "tests/unittests/*.cpp"
  )
  if(AYM_UNIT_TEST_SOURCES)
    set(AYM_CORE_SOURCES ${AYM_SOURCES})
    list(FILTER AYM_CORE_SOURCES EXCLUDE REGEX "compiler/main\\.cpp$")

    add_executable(aym_unit_tests ${AYM_CORE_SOURCES} ${AYM_UNIT_TEST_SOURCES})
    target_include_directories(aym_unit_tests PRIVATE ${CMAKE_SOURCE_DIR})

    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
      if(AYM_FETCH_GTEST)
        include(FetchContent)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_Declare(
          googletest
          DOWNLOAD_EXTRACT_TIMESTAMP TRUE
          URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        )
        FetchContent_MakeAvailable(googletest)
      else()
        message(FATAL_ERROR "GoogleTest no encontrado. Instala GTest o activa AYM_FETCH_GTEST=ON.")
      endif()
    endif()

    if(TARGET GTest::gtest)
      target_link_libraries(aym_unit_tests PRIVATE GTest::gtest)
    else()
      target_link_libraries(aym_unit_tests PRIVATE gtest)
    endif()

    add_test(NAME aym_unit_tests COMMAND aym_unit_tests)
    set_tests_properties(aym_unit_tests PROPERTIES
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
  endif()
endif()

if(BUILD_TESTING)
  add_test(
    NAME aym_wrapper_smoke
    COMMAND
      ${CMAKE_COMMAND}
      -DAYM_WRAPPER=$<TARGET_FILE:aym>
      -DAYM_COMPILER=$<TARGET_FILE:aymc>
      -DAYM_SMOKE_ROOT=${CMAKE_BINARY_DIR}/tmp/ctest_wrapper_smoke
      -P ${CMAKE_SOURCE_DIR}/tests/wrapper_smoke.cmake
  )
  set_tests_properties(aym_wrapper_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )

  add_test(
    NAME aym_diagnostics_smoke
    COMMAND
      ${CMAKE_COMMAND}
      -DAYM_COMPILER=$<TARGET_FILE:aymc>
      -DAYM_DIAG_ROOT=${CMAKE_BINARY_DIR}/tmp/ctest_diagnostics_smoke
      -P ${CMAKE_SOURCE_DIR}/tests/diagnostics_smoke.cmake
  )
  set_tests_properties(aym_diagnostics_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )

  add_test(
    NAME aym_lock_contract_smoke
    COMMAND
      ${CMAKE_COMMAND}
      -DAYM_WRAPPER=$<TARGET_FILE:aym>
      -DAYM_COMPILER=$<TARGET_FILE:aymc>
      -DAYM_LOCK_CONTRACT_ROOT=${CMAKE_BINARY_DIR}/tmp/ctest_lock_contract_smoke
      -P ${CMAKE_SOURCE_DIR}/tests/lock_contract_smoke.cmake
  )
  set_tests_properties(aym_lock_contract_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )

  add_test(
    NAME aym_pipeline_modes_smoke
    COMMAND
      ${CMAKE_COMMAND}
      -DAYM_COMPILER=$<TARGET_FILE:aymc>
      -DAYM_PIPELINE_ROOT=${CMAKE_BINARY_DIR}/tmp/ctest_pipeline_modes_smoke
      -P ${CMAKE_SOURCE_DIR}/tests/pipeline_modes_smoke.cmake
  )
  set_tests_properties(aym_pipeline_modes_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )

  add_test(
    NAME aym_backend_ir_smoke
    COMMAND
      ${CMAKE_COMMAND}
      -DAYM_COMPILER=$<TARGET_FILE:aymc>
      -DAYM_BACKEND_IR_ROOT=${CMAKE_BINARY_DIR}/tmp/ctest_backend_ir_smoke
      -P ${CMAKE_SOURCE_DIR}/tests/backend_ir_smoke.cmake
  )
  set_tests_properties(aym_backend_ir_smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )

  if(UNIX AND NOT WIN32)
    find_program(AYM_BASH_EXECUTABLE NAMES bash)
    if(AYM_BASH_EXECUTABLE)
      add_test(
        NAME aym_linux_gui_smoke
        COMMAND
          ${AYM_BASH_EXECUTABLE}
          ${CMAKE_SOURCE_DIR}/tests/linux_gui_smoke.sh
          $<TARGET_FILE:aymc>
      )
      set_tests_properties(aym_linux_gui_smoke PROPERTIES
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      )
    endif()
  endif()

  if(WIN32)
    find_program(AYM_PWSH_EXECUTABLE NAMES pwsh powershell)
    if(AYM_PWSH_EXECUTABLE)
      add_test(
        NAME aym_windows_installer_smoke
        COMMAND
          ${AYM_PWSH_EXECUTABLE}
          -File ${CMAKE_SOURCE_DIR}/tests/installer_windows_smoke.ps1
          -BuildDir ${CMAKE_BINARY_DIR}
          -Config $<CONFIG>
      )
      set_tests_properties(aym_windows_installer_smoke PROPERTIES
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      )
    endif()
  endif()
endif()

if(BUILD_TESTING AND AYM_ENABLE_COVERAGE)
  find_program(AYM_GCOVR_EXECUTABLE gcovr)
  if(AYM_GCOVR_EXECUTABLE)
    add_custom_target(coverage
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
      COMMAND ${AYM_GCOVR_EXECUTABLE}
              --root ${CMAKE_SOURCE_DIR}
              --object-directory ${CMAKE_BINARY_DIR}
              --exclude-directories ${CMAKE_BINARY_DIR}/_deps
              --exclude tests/.*
              --xml-pretty
              --output ${CMAKE_BINARY_DIR}/coverage/coverage.xml
              --html-details ${CMAKE_BINARY_DIR}/coverage/coverage.html
              --print-summary
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      USES_TERMINAL
      COMMENT "Ejecutando tests y generando reporte de cobertura en ${CMAKE_BINARY_DIR}/coverage")
  else()
    message(WARNING "gcovr no encontrado: no se generara target coverage.")
  endif()
endif()

include(GNUInstallDirs)

install(TARGETS aymc aym
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES README.md LICENSE installer/EULA.txt
  DESTINATION .
  OPTIONAL)

install(DIRECTORY runtime/
  DESTINATION ${CMAKE_INSTALL_DATADIR}/aymaraLang/runtime
  FILES_MATCHING PATTERN "*.c" PATTERN "README.md")

# Put binaries into bin/ inside the build directory
set_target_properties(aymc aym PROPERTIES
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
  set(CPACK_PACKAGE_EXECUTABLES
    "aym" "AymaraLang Wrapper"
    "aymc" "AymaraLang Compiler")
else()
  set(CPACK_GENERATOR "TGZ;DEB;RPM")
  set(CPACK_PACKAGE_FILE_NAME "AymaraLang-${AYM_VERSION}-${CMAKE_SYSTEM_NAME}")
endif()

if(APPLE)
  set(CPACK_GENERATOR "DragNDrop")
endif()

include(CPack)
