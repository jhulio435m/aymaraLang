cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED AYM_COMPILER OR AYM_COMPILER STREQUAL "" OR NOT EXISTS "${AYM_COMPILER}")
  message(FATAL_ERROR "AYM_COMPILER no definido o no existe: '${AYM_COMPILER}'")
endif()

if(NOT DEFINED AYM_BACKEND_IR_ROOT OR AYM_BACKEND_IR_ROOT STREQUAL "")
  set(AYM_BACKEND_IR_ROOT "${CMAKE_BINARY_DIR}/tmp/ctest_backend_ir_smoke")
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

file(REMOVE_RECURSE "${AYM_BACKEND_IR_ROOT}")
file(MAKE_DIRECTORY "${AYM_BACKEND_IR_ROOT}")

set(source_file "${AYM_BACKEND_IR_ROOT}/ir_smoke.aym")
file(WRITE
  "${source_file}"
  "qallta\n"
  "qillqa(\"ir-smoke\");\n"
  "tukuya\n")

set(output_base "${AYM_BACKEND_IR_ROOT}/ir_smoke")
set(ir_path "${output_base}.ir")
set(pipeline_json "${output_base}.pipeline.json")

expect_success(
  "${AYM_BACKEND_IR_ROOT}"
  "${AYM_COMPILER}" "--backend" "ir" "--time-pipeline-json=${pipeline_json}" "-o" "${output_base}" "${source_file}")

if(NOT EXISTS "${ir_path}")
  message(FATAL_ERROR "No se genero artefacto IR esperado: ${ir_path}")
endif()

if(NOT EXISTS "${pipeline_json}")
  message(FATAL_ERROR "No se genero pipeline JSON esperado: ${pipeline_json}")
endif()

file(READ "${pipeline_json}" pipeline_json_content)
string(FIND "${pipeline_json_content}" "\"backend\": \"ir\"" has_ir_backend)
if(has_ir_backend EQUAL -1)
  message(FATAL_ERROR "Pipeline JSON no reporta backend=ir")
endif()
string(FIND "${pipeline_json_content}" "\"tool_timeout_ms\"" has_timeout_field)
if(has_timeout_field EQUAL -1)
  message(FATAL_ERROR "Pipeline JSON backend ir sin campo tool_timeout_ms")
endif()
string(FIND "${pipeline_json_content}" "\"commands\"" has_commands)
if(has_commands EQUAL -1)
  message(FATAL_ERROR "Pipeline JSON backend ir sin bloque commands")
endif()
string(FIND "${pipeline_json_content}" "\"phase_summary\"" has_phase_summary)
if(has_phase_summary EQUAL -1)
  message(FATAL_ERROR "Pipeline JSON backend ir sin bloque phase_summary")
endif()
string(FIND "${pipeline_json_content}" "\"commands_total\": 1" has_commands_total)
if(has_commands_total EQUAL -1)
  message(FATAL_ERROR "Pipeline JSON backend ir con commands_total inesperado")
endif()
string(FIND "${pipeline_json_content}" "\"reason_ok\": 1" has_reason_ok)
if(has_reason_ok EQUAL -1)
  message(FATAL_ERROR "Pipeline JSON backend ir con reason_ok inesperado")
endif()
string(FIND "${pipeline_json_content}" "\"stage\": \"ir-gen\"" has_ir_gen_stage)
if(has_ir_gen_stage EQUAL -1)
  message(FATAL_ERROR "Pipeline JSON backend ir sin stage ir-gen")
endif()
string(FIND "${pipeline_json_content}" "\"exit_reason\": \"ok\"" has_exit_reason_ok)
if(has_exit_reason_ok EQUAL -1)
  message(FATAL_ERROR "Pipeline JSON backend ir sin exit_reason=ok")
endif()
string(FIND "${pipeline_json_content}" "\"stdout\"" has_stdout_field)
if(has_stdout_field EQUAL -1)
  message(FATAL_ERROR "Pipeline JSON backend ir sin campo stdout en commands")
endif()
string(FIND "${pipeline_json_content}" "\"stderr\"" has_stderr_field)
if(has_stderr_field EQUAL -1)
  message(FATAL_ERROR "Pipeline JSON backend ir sin campo stderr en commands")
endif()

expect_failure_contains(
  "${AYM_BACKEND_IR_ROOT}"
  "--link-only"
  "${AYM_COMPILER}" "--backend" "ir" "--link-only" "-o" "${output_base}")
