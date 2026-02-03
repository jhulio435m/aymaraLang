Plan de Verificación y Validación (V&V)
=======================================

1. Objetivo
-----------

Definir cómo se verificará y validará AymaraLang y su compilador para
asegurar cumplimiento de requisitos.

2. Alcance
----------

- Compilador ``aymc`` (lexer, parser, semantic, codegen).
- Backend LLVM experimental.
- Runtime mínimo.

3. Estrategia de V&V
--------------------

========== ==========================================
Tipo       Descripción
========== ==========================================
Inspección Revisión de código y documentación
Prueba     Ejecución de casos de prueba automatizados
Análisis   Comparación de resultados esperados
========== ==========================================

4. Matriz de verificación (resumen)
-----------------------------------

========= ========== ===================================
Requisito Método     Evidencia
========= ========== ===================================
FR-01     Prueba     Compila múltiples archivos ``.aym``
FR-05     Prueba     Binario ejecuta ejemplos
FR-07     Prueba     Generación de ``.ll``
NFR-03    Inspección Mensajes de error legibles
========= ========== ===================================

5. Criterios de aceptación
--------------------------

- Todos los ejemplos en ``samples/`` deben compilar y ejecutarse.
- ``make test`` debe pasar sin errores (cuando el entorno lo permita).

6. Artefactos de evidencia
--------------------------

- Reportes de ejecución de ``make test``.
- Logs de compilación y salida de ejemplos.
