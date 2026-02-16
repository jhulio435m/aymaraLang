cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED AYM_COMPILER OR AYM_COMPILER STREQUAL "" OR NOT EXISTS "${AYM_COMPILER}")
  message(FATAL_ERROR "AYM_COMPILER no definido o no existe: '${AYM_COMPILER}'")
endif()

if(NOT DEFINED AYM_DIAG_ROOT OR AYM_DIAG_ROOT STREQUAL "")
  set(AYM_DIAG_ROOT "${CMAKE_BINARY_DIR}/tmp/ctest_diagnostics_smoke")
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
      "Fallo comando en diagnostics smoke:\n"
      "  ${command_text}\n"
      "Directorio: ${working_dir}\n"
      "Exit code: ${rc}\n"
      "STDOUT:\n${out}\n"
      "STDERR:\n${err}\n")
  endif()
endfunction()

file(REMOVE_RECURSE "${AYM_DIAG_ROOT}")
file(MAKE_DIRECTORY "${AYM_DIAG_ROOT}")

set(project_dir "${AYM_DIAG_ROOT}/diag_demo")
file(MAKE_DIRECTORY "${project_dir}")
file(MAKE_DIRECTORY "${project_dir}/.aym/cache/math/1.2.3/modules")

file(WRITE
  "${project_dir}/src_main.aym"
  "qallta\napnaq(\"math/util\");\nqillqa(desde_cache());\ntukuya\n")

file(WRITE
  "${project_dir}/.aym/cache/math/1.2.3/modules/util.aym"
  "qallta\nlurawi desde_cache() : jakhuwi { kuttaya(7); }\ntukuya\n")

file(WRITE
  "${project_dir}/aym.toml"
  "[package]\n"
  "name = \"diag_demo\"\n"
  "version = \"0.1.0\"\n"
  "edition = \"2026\"\n\n"
  "[dependencies]\n"
  "math = \"^1.2.3\"\n")

run_checked("${project_dir}"
  "${AYM_COMPILER}" "--check-manifest" "--emit-lock" "--manifest" "${project_dir}/aym.toml")

# Introduce inconsistencia manifest-lock para forzar AYM4006 durante import por paquete.
file(WRITE
  "${project_dir}/aym.toml"
  "[package]\n"
  "name = \"diag_demo\"\n"
  "version = \"0.1.0\"\n"
  "edition = \"2026\"\n\n"
  "[dependencies]\n"
  "math = \"^2.0.0\"\n")

set(diag_json "${project_dir}/aym4006.diagnostics.json")
execute_process(
  COMMAND "${AYM_COMPILER}" "--check" "--diagnostics-json=${diag_json}" "${project_dir}/src_main.aym"
  WORKING_DIRECTORY "${project_dir}"
  RESULT_VARIABLE check_rc
  OUTPUT_VARIABLE check_out
  ERROR_VARIABLE check_err
)

if(check_rc EQUAL 0)
  message(FATAL_ERROR
    "Se esperaba fallo de compilacion para AYM4006, pero retorno 0.\n"
    "STDOUT:\n${check_out}\n"
    "STDERR:\n${check_err}\n")
endif()

if(NOT EXISTS "${diag_json}")
  message(FATAL_ERROR
    "No se genero diagnostics JSON esperado: ${diag_json}\n"
    "STDOUT:\n${check_out}\n"
    "STDERR:\n${check_err}\n")
endif()

file(READ "${diag_json}" diag_contents)

string(FIND "${diag_contents}" "\"code\": \"AYM4006\"" has_code)
if(has_code EQUAL -1)
  message(FATAL_ERROR
    "Diagnostics JSON no contiene AYM4006.\n"
    "Contenido:\n${diag_contents}\n")
endif()

string(FIND "${diag_contents}" "\"severity\": \"error\"" has_severity)
if(has_severity EQUAL -1)
  message(FATAL_ERROR
    "Diagnostics JSON no contiene severidad error.\n"
    "Contenido:\n${diag_contents}\n")
endif()

string(FIND "${diag_contents}" "Sincroniza y valida dependencias del proyecto" has_suggestion)
if(has_suggestion EQUAL -1)
  message(FATAL_ERROR
    "Diagnostics JSON no contiene sugerencia esperada para AYM4006.\n"
    "Contenido:\n${diag_contents}\n")
endif()
