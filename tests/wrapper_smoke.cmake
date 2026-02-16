cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED AYM_WRAPPER OR AYM_WRAPPER STREQUAL "" OR NOT EXISTS "${AYM_WRAPPER}")
  message(FATAL_ERROR "AYM_WRAPPER no definido o no existe: '${AYM_WRAPPER}'")
endif()

if(NOT DEFINED AYM_COMPILER OR AYM_COMPILER STREQUAL "" OR NOT EXISTS "${AYM_COMPILER}")
  message(FATAL_ERROR "AYM_COMPILER no definido o no existe: '${AYM_COMPILER}'")
endif()

if(NOT DEFINED AYM_SMOKE_ROOT OR AYM_SMOKE_ROOT STREQUAL "")
  set(AYM_SMOKE_ROOT "${CMAKE_BINARY_DIR}/tmp/ctest_wrapper_smoke")
endif()

function(run_checked working_dir)
  set(cmd ${ARGN})
  execute_process(
    COMMAND ${cmd}
    WORKING_DIRECTORY "${working_dir}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0)
    string(JOIN " " command_text ${cmd})
    message(FATAL_ERROR
      "Fallo comando en wrapper smoke:\n"
      "  ${command_text}\n"
      "Directorio: ${working_dir}\n"
      "Exit code: ${rc}\n"
      "STDOUT:\n${out}\n"
      "STDERR:\n${err}\n")
  endif()
endfunction()

function(run_expected_fail working_dir)
  set(cmd ${ARGN})
  execute_process(
    COMMAND ${cmd}
    WORKING_DIRECTORY "${working_dir}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(rc EQUAL 0)
    string(JOIN " " command_text ${cmd})
    message(FATAL_ERROR
      "Se esperaba fallo en wrapper smoke:\n"
      "  ${command_text}\n"
      "Directorio: ${working_dir}\n"
      "STDOUT:\n${out}\n"
      "STDERR:\n${err}\n")
  endif()
endfunction()

set(ENV{AYM_COMPILER} "${AYM_COMPILER}")

file(REMOVE_RECURSE "${AYM_SMOKE_ROOT}")
file(MAKE_DIRECTORY "${AYM_SMOKE_ROOT}")

set(project_name "smoke_demo")
run_checked("${AYM_SMOKE_ROOT}" "${AYM_WRAPPER}" new "${project_name}" --path "${AYM_SMOKE_ROOT}")

set(project_dir "${AYM_SMOKE_ROOT}/${project_name}")
run_checked("${project_dir}" "${AYM_WRAPPER}" add math ^1.2.3)
run_expected_fail("${project_dir}" "${AYM_WRAPPER}" add io ^1.0.0 --frozen)
run_checked("${project_dir}" "${AYM_WRAPPER}" lock check)
run_checked("${project_dir}" "${AYM_WRAPPER}" lock sync --frozen)

file(MAKE_DIRECTORY "${project_dir}/.aym/repo/math/1.2.3/modules")
file(WRITE
  "${project_dir}/.aym/repo/math/1.2.3/modules/util.aym"
  "qallta\nlurawi desde_repo() : jakhuwi { kuttaya(42); }\ntukuya\n")

file(WRITE
  "${project_dir}/src/main.aym"
  "qallta\napnaq(\"math/util\");\nqillqa(desde_repo());\ntukuya\n")

run_checked("${project_dir}" "${AYM_WRAPPER}" build --check --doctor-fix)
run_checked("${project_dir}" "${AYM_WRAPPER}" cache sync --frozen)

file(MAKE_DIRECTORY "${project_dir}/tests")
configure_file("${project_dir}/src/main.aym" "${project_dir}/tests/smoke.aym" COPYONLY)
run_checked("${project_dir}" "${AYM_WRAPPER}" test --doctor-fix)

# Verifica modo --frozen: si manifest cambia, debe fallar sin regenerar lock.
file(WRITE
  "${project_dir}/aym.toml"
  "[package]\n"
  "name = \"smoke_demo\"\n"
  "version = \"0.1.0\"\n"
  "edition = \"2026\"\n\n"
  "[dependencies]\n"
  "math = \"^2.0.0\"\n")
run_expected_fail("${project_dir}" "${AYM_WRAPPER}" build --check --frozen)
run_expected_fail("${project_dir}" "${AYM_WRAPPER}" lock check)
run_expected_fail("${project_dir}" "${AYM_WRAPPER}" lock sync --frozen)
run_expected_fail("${project_dir}" "${AYM_WRAPPER}" cache sync --frozen)
run_checked("${project_dir}" "${AYM_WRAPPER}" lock sync)
run_checked("${project_dir}" "${AYM_WRAPPER}" lock check)
run_checked("${project_dir}" "${AYM_WRAPPER}" cache sync --frozen)

set(lock_path "${project_dir}/aym.lock")
if(NOT EXISTS "${lock_path}")
  message(FATAL_ERROR "No se genero lockfile en ${lock_path}")
endif()

file(READ "${lock_path}" lock_contents)
string(FIND "${lock_contents}" "manifest_checksum = \"fnv1a64:" manifest_checksum_pos)
if(manifest_checksum_pos EQUAL -1)
  message(FATAL_ERROR "aym.lock generado sin manifest_checksum valido")
endif()

string(FIND "${lock_contents}" "checksum = \"fnv1a64:" dep_checksum_pos)
if(dep_checksum_pos EQUAL -1)
  message(FATAL_ERROR "aym.lock generado sin checksum de dependencia valido")
endif()
