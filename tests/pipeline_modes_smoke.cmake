cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED AYM_COMPILER OR AYM_COMPILER STREQUAL "" OR NOT EXISTS "${AYM_COMPILER}")
  message(FATAL_ERROR "AYM_COMPILER no definido o no existe: '${AYM_COMPILER}'")
endif()

if(NOT DEFINED AYM_PIPELINE_ROOT OR AYM_PIPELINE_ROOT STREQUAL "")
  set(AYM_PIPELINE_ROOT "${CMAKE_BINARY_DIR}/tmp/ctest_pipeline_modes_smoke")
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

function(expect_success working_dir)
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
endfunction()

function(expect_success_contains working_dir expected_text)
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

function(expect_failure_contains working_dir expected_text)
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
      "Fallo esperado sin texto diagnostico.\n"
      "Comando: ${command_text}\n"
      "Esperado: ${expected_text}\n"
      "STDOUT:\n${out}\n"
      "STDERR:\n${err}\n")
  endif()
endfunction()

file(REMOVE_RECURSE "${AYM_PIPELINE_ROOT}")
file(MAKE_DIRECTORY "${AYM_PIPELINE_ROOT}")
file(MAKE_DIRECTORY "${AYM_PIPELINE_ROOT}/runtime")
configure_file("${CMAKE_CURRENT_LIST_DIR}/../runtime/runtime.c" "${AYM_PIPELINE_ROOT}/runtime/runtime.c" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../runtime/math.c" "${AYM_PIPELINE_ROOT}/runtime/math.c" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../runtime/runtime_arrays.c" "${AYM_PIPELINE_ROOT}/runtime/runtime_arrays.c" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../runtime/runtime_maps_strings.c" "${AYM_PIPELINE_ROOT}/runtime/runtime_maps_strings.c" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../runtime/runtime_exceptions.c" "${AYM_PIPELINE_ROOT}/runtime/runtime_exceptions.c" COPYONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/../runtime/runtime_gfx_linux.c" "${AYM_PIPELINE_ROOT}/runtime/runtime_gfx_linux.c" COPYONLY)

set(source_file "${AYM_PIPELINE_ROOT}/pipeline_modes.aym")
file(WRITE
  "${source_file}"
  "qallta\n"
  "qillqa(\"pipeline\");\n"
  "tukuya\n")

set(base_path "${AYM_PIPELINE_ROOT}/pipeline_modes")
set(asm_path "${base_path}.asm")
set(compile_json "${AYM_PIPELINE_ROOT}/pipeline_compile.json")
set(link_json "${AYM_PIPELINE_ROOT}/pipeline_link.json")
if(WIN32)
  set(obj_path "${base_path}.obj")
  set(exe_path "${base_path}.exe")
else()
  set(obj_path "${base_path}.o")
  set(exe_path "${base_path}")
endif()

expect_success_contains(
  "${AYM_PIPELINE_ROOT}"
  "[aymc] pipeline(ms):"
  "${AYM_COMPILER}" "--compile-only" "--time-pipeline" "--time-pipeline-json=${compile_json}" "--emit-asm" "-o" "${base_path}" "${source_file}")

if(NOT EXISTS "${asm_path}")
  message(FATAL_ERROR "No se genero ASM esperado en compile-only: ${asm_path}")
endif()
if(NOT EXISTS "${obj_path}")
  message(FATAL_ERROR "No se genero objeto esperado en compile-only: ${obj_path}")
endif()
if(EXISTS "${exe_path}")
  message(FATAL_ERROR "No deberia existir ejecutable tras compile-only: ${exe_path}")
endif()
if(NOT EXISTS "${compile_json}")
  message(FATAL_ERROR "No se genero metrics JSON en compile-only: ${compile_json}")
endif()
file(READ "${compile_json}" compile_json_content)
string(FIND "${compile_json_content}" "\"schema\": \"aymc.pipeline.v1\"" compile_has_schema)
if(compile_has_schema EQUAL -1)
  message(FATAL_ERROR "Metrics JSON compile-only sin schema esperado")
endif()
string(FIND "${compile_json_content}" "\"mode\": \"compile-only\"" compile_has_mode)
if(compile_has_mode EQUAL -1)
  message(FATAL_ERROR "Metrics JSON compile-only sin mode esperado")
endif()
string(FIND "${compile_json_content}" "\"timing_ms\"" compile_has_timing)
if(compile_has_timing EQUAL -1)
  message(FATAL_ERROR "Metrics JSON compile-only sin bloque timing_ms")
endif()
string(FIND "${compile_json_content}" "\"tool_timeout_ms\"" compile_has_timeout_field)
if(compile_has_timeout_field EQUAL -1)
  message(FATAL_ERROR "Metrics JSON compile-only sin campo tool_timeout_ms")
endif()
string(FIND "${compile_json_content}" "\"commands\"" compile_has_commands)
if(compile_has_commands EQUAL -1)
  message(FATAL_ERROR "Metrics JSON compile-only sin bloque commands")
endif()
string(FIND "${compile_json_content}" "\"phase_summary\"" compile_has_summary)
if(compile_has_summary EQUAL -1)
  message(FATAL_ERROR "Metrics JSON compile-only sin bloque phase_summary")
endif()
string(FIND "${compile_json_content}" "\"commands_total\": 1" compile_has_commands_total)
if(compile_has_commands_total EQUAL -1)
  message(FATAL_ERROR "Metrics JSON compile-only con commands_total inesperado")
endif()
string(FIND "${compile_json_content}" "\"ok\": 1" compile_has_ok_count)
if(compile_has_ok_count EQUAL -1)
  message(FATAL_ERROR "Metrics JSON compile-only con contador ok inesperado")
endif()
string(FIND "${compile_json_content}" "\"reason_ok\": 1" compile_has_reason_ok)
if(compile_has_reason_ok EQUAL -1)
  message(FATAL_ERROR "Metrics JSON compile-only con reason_ok inesperado")
endif()
string(FIND "${compile_json_content}" "\"stage\": \"ensamblado\"" compile_has_asm_stage)
if(compile_has_asm_stage EQUAL -1)
  message(FATAL_ERROR "Metrics JSON compile-only sin traza de etapa ensamblado")
endif()
string(FIND "${compile_json_content}" "\"exit_reason\": \"ok\"" compile_has_exit_reason_ok)
if(compile_has_exit_reason_ok EQUAL -1)
  message(FATAL_ERROR "Metrics JSON compile-only sin exit_reason=ok")
endif()
string(FIND "${compile_json_content}" "\"stdout\"" compile_has_stdout)
if(compile_has_stdout EQUAL -1)
  message(FATAL_ERROR "Metrics JSON compile-only sin campo stdout en commands")
endif()
string(FIND "${compile_json_content}" "\"stderr\"" compile_has_stderr)
if(compile_has_stderr EQUAL -1)
  message(FATAL_ERROR "Metrics JSON compile-only sin campo stderr en commands")
endif()

expect_success_contains(
  "${AYM_PIPELINE_ROOT}"
  "[aymc] pipeline(ms):"
  "${AYM_COMPILER}" "--link-only" "--time-pipeline" "--time-pipeline-json=${link_json}" "-o" "${base_path}")

if(NOT EXISTS "${exe_path}")
  message(FATAL_ERROR "No se genero ejecutable esperado en link-only: ${exe_path}")
endif()
if(NOT EXISTS "${link_json}")
  message(FATAL_ERROR "No se genero metrics JSON en link-only: ${link_json}")
endif()
file(READ "${link_json}" link_json_content)
string(FIND "${link_json_content}" "\"mode\": \"link-only\"" link_has_mode)
if(link_has_mode EQUAL -1)
  message(FATAL_ERROR "Metrics JSON link-only sin mode esperado")
endif()
string(FIND "${link_json_content}" "\"runtime\"" link_has_runtime)
if(link_has_runtime EQUAL -1)
  message(FATAL_ERROR "Metrics JSON link-only sin bloque runtime")
endif()
string(FIND "${link_json_content}" "\"commands\"" link_has_commands)
if(link_has_commands EQUAL -1)
  message(FATAL_ERROR "Metrics JSON link-only sin bloque commands")
endif()
string(FIND "${link_json_content}" "\"phase_summary\"" link_has_summary)
if(link_has_summary EQUAL -1)
  message(FATAL_ERROR "Metrics JSON link-only sin bloque phase_summary")
endif()
string(FIND "${link_json_content}" "\"exit_reason\": \"ok\"" link_has_exit_reason_ok)
if(link_has_exit_reason_ok EQUAL -1)
  message(FATAL_ERROR "Metrics JSON link-only sin exit_reason=ok")
endif()

file(REMOVE "${obj_path}")
set(fail_json "${AYM_PIPELINE_ROOT}/pipeline_fail.json")
expect_failure_contains(
  "${AYM_PIPELINE_ROOT}"
  "pre-link"
  "${AYM_COMPILER}" "--link-only" "--time-pipeline-json=${fail_json}" "-o" "${base_path}")

if(NOT EXISTS "${fail_json}")
  message(FATAL_ERROR "No se genero metrics JSON en fallo link-only: ${fail_json}")
endif()
file(READ "${fail_json}" fail_json_content)
string(FIND "${fail_json_content}" "\"success\": false" fail_has_failure)
if(fail_has_failure EQUAL -1)
  message(FATAL_ERROR "Metrics JSON de fallo sin success=false")
endif()
string(FIND "${fail_json_content}" "\"failed_stage\": \"pre-link\"" fail_has_stage)
if(fail_has_stage EQUAL -1)
  message(FATAL_ERROR "Metrics JSON de fallo sin failed_stage=pre-link")
endif()
string(FIND "${fail_json_content}" "\"status\": \"failed\"" fail_has_failed_status)
if(fail_has_failed_status EQUAL -1)
  message(FATAL_ERROR "Metrics JSON de fallo sin status=failed en commands")
endif()
string(FIND "${fail_json_content}" "\"commands_total\": 1" fail_has_commands_total)
if(fail_has_commands_total EQUAL -1)
  message(FATAL_ERROR "Metrics JSON de fallo con commands_total inesperado")
endif()
string(FIND "${fail_json_content}" "\"failed\": 1" fail_has_failed_count)
if(fail_has_failed_count EQUAL -1)
  message(FATAL_ERROR "Metrics JSON de fallo con contador failed inesperado")
endif()
string(FIND "${fail_json_content}" "\"reason_missing_input\": 1" fail_has_reason_missing_input)
if(fail_has_reason_missing_input EQUAL -1)
  message(FATAL_ERROR "Metrics JSON de fallo con reason_missing_input inesperado")
endif()
string(FIND "${fail_json_content}" "\"exit_reason\": \"missing_input\"" fail_has_exit_reason_missing_input)
if(fail_has_exit_reason_missing_input EQUAL -1)
  message(FATAL_ERROR "Metrics JSON de fallo sin exit_reason=missing_input")
endif()
string(FIND "${fail_json_content}" "\"stderr\"" fail_has_stderr)
if(fail_has_stderr EQUAL -1)
  message(FATAL_ERROR "Metrics JSON de fallo sin campo stderr en commands")
endif()
