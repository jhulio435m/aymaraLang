cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED AYM_WRAPPER OR AYM_WRAPPER STREQUAL "" OR NOT EXISTS "${AYM_WRAPPER}")
  message(FATAL_ERROR "AYM_WRAPPER no definido o no existe: '${AYM_WRAPPER}'")
endif()

if(NOT DEFINED AYM_COMPILER OR AYM_COMPILER STREQUAL "" OR NOT EXISTS "${AYM_COMPILER}")
  message(FATAL_ERROR "AYM_COMPILER no definido o no existe: '${AYM_COMPILER}'")
endif()

if(NOT DEFINED AYM_LOCK_CONTRACT_ROOT OR AYM_LOCK_CONTRACT_ROOT STREQUAL "")
  set(AYM_LOCK_CONTRACT_ROOT "${CMAKE_BINARY_DIR}/tmp/ctest_lock_contract_smoke")
endif()

function(run_capture out_var err_var rc_var working_dir)
  set(cmd ${ARGN})
  execute_process(
    COMMAND ${cmd}
    WORKING_DIRECTORY "${working_dir}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  set(${out_var} "${out}" PARENT_SCOPE)
  set(${err_var} "${err}" PARENT_SCOPE)
  set(${rc_var} "${rc}" PARENT_SCOPE)
endfunction()

function(expect_success_with_text working_dir expected_text)
  set(cmd ${ARGN})
  run_capture(out err rc "${working_dir}" ${cmd})
  if(NOT rc EQUAL 0)
    string(JOIN " " command_text ${cmd})
    message(FATAL_ERROR
      "Comando debio funcionar y fallo:\n"
      "  ${command_text}\n"
      "Directorio: ${working_dir}\n"
      "Exit code: ${rc}\n"
      "STDOUT:\n${out}\n"
      "STDERR:\n${err}\n")
  endif()
  string(FIND "${out}${err}" "${expected_text}" found)
  if(found EQUAL -1)
    string(JOIN " " command_text ${cmd})
    message(FATAL_ERROR
      "Salida no contiene texto esperado.\n"
      "Comando: ${command_text}\n"
      "Esperado: ${expected_text}\n"
      "STDOUT:\n${out}\n"
      "STDERR:\n${err}\n")
  endif()
endfunction()

function(expect_failure_with_text working_dir expected_text)
  set(cmd ${ARGN})
  run_capture(out err rc "${working_dir}" ${cmd})
  if(rc EQUAL 0)
    string(JOIN " " command_text ${cmd})
    message(FATAL_ERROR
      "Comando debio fallar y retorno 0:\n"
      "  ${command_text}\n"
      "Directorio: ${working_dir}\n"
      "STDOUT:\n${out}\n"
      "STDERR:\n${err}\n")
  endif()
  string(FIND "${out}${err}" "${expected_text}" found)
  if(found EQUAL -1)
    string(JOIN " " command_text ${cmd})
    message(FATAL_ERROR
      "Fallo esperado, pero sin texto de contrato.\n"
      "Comando: ${command_text}\n"
      "Esperado: ${expected_text}\n"
      "STDOUT:\n${out}\n"
      "STDERR:\n${err}\n")
  endif()
endfunction()

set(ENV{AYM_COMPILER} "${AYM_COMPILER}")

file(REMOVE_RECURSE "${AYM_LOCK_CONTRACT_ROOT}")
file(MAKE_DIRECTORY "${AYM_LOCK_CONTRACT_ROOT}")

set(project_name "lock_contract_demo")
expect_success_with_text(
  "${AYM_LOCK_CONTRACT_ROOT}"
  "Proyecto creado"
  "${AYM_WRAPPER}" new "${project_name}" --path "${AYM_LOCK_CONTRACT_ROOT}")

set(project_dir "${AYM_LOCK_CONTRACT_ROOT}/${project_name}")

expect_success_with_text(
  "${project_dir}"
  "Dependencia actualizada: math = \"^1.2.3\""
  "${AYM_WRAPPER}" add math ^1.2.3)

expect_success_with_text(
  "${project_dir}"
  "Lockfile consistente. Dependencias: 1"
  "${AYM_WRAPPER}" lock check)

expect_success_with_text(
  "${project_dir}"
  "Modo frozen activo: lockfile no regenerado."
  "${AYM_WRAPPER}" lock sync --frozen)

expect_failure_with_text(
  "${project_dir}"
  "modo --frozen: 'add' no permite modificar dependencias."
  "${AYM_WRAPPER}" add io ^1.0.0 --frozen)

# Fuerza inconsistencia para validar contrato de error de lock check.
file(WRITE
  "${project_dir}/aym.toml"
  "[package]\n"
  "name = \"lock_contract_demo\"\n"
  "version = \"0.1.0\"\n"
  "edition = \"2026\"\n\n"
  "[dependencies]\n"
  "math = \"^2.0.0\"\n")

expect_failure_with_text(
  "${project_dir}"
  "AYM5007"
  "${AYM_WRAPPER}" lock check)

expect_failure_with_text(
  "${project_dir}"
  "modo --frozen"
  "${AYM_WRAPPER}" lock sync --frozen)

expect_success_with_text(
  "${project_dir}"
  "Lockfile sincronizado. Dependencias: 1"
  "${AYM_WRAPPER}" lock sync)

expect_success_with_text(
  "${project_dir}"
  "Lockfile consistente. Dependencias: 1"
  "${AYM_WRAPPER}" lock check)
