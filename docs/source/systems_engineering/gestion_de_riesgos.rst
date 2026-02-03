Plan de Gestión de Riesgos
==========================

1. Objetivo
-----------

Identificar, evaluar y mitigar riesgos técnicos y de proyecto asociados
al desarrollo y uso de AymaraLang.

2. Metodología
--------------

- Identificación de riesgos.
- Evaluación de probabilidad e impacto.
- Planes de mitigación y contingencia.

3. Registro de riesgos
----------------------

+-------------+----------------+--------------+-------------+--------------------+
| ID          | Riesgo         | Probabilidad | Impacto     | Mitigación         |
+=============+================+==============+=============+====================+
| R-01        | Dependencia de | Media        | Alta        | Documentar         |
|             | toolchain      |              |             | dependencias y     |
|             | externo        |              |             | validar en CI      |
+-------------+----------------+--------------+-------------+--------------------+
| R-02        | Backend LLVM   | Media        | Media       | Mantener backend   |
|             | incompleto     |              |             | NASM como          |
|             |                |              |             | principal          |
+-------------+----------------+--------------+-------------+--------------------+
| R-03        | Compatibilidad | Alta         | Media       | Definir roadmap    |
|             | limitada a     |              |             | multi-arquitectura |
|             | x86_64         |              |             |                    |
+-------------+----------------+--------------+-------------+--------------------+
| R-04        | Curva de       | Media        | Media       | Crear guías y      |
|             | aprendizaje de |              |             | ejemplos           |
|             | sintaxis       |              |             | accesibles         |
|             | aymara         |              |             |                    |
+-------------+----------------+--------------+-------------+--------------------+
| R-05        | Falta de       | Media        | Media       | Automatizar tests  |
|             | recursos para  |              |             | y usar runners     |
|             | pruebas en     |              |             | multi-OS           |
|             | Windows        |              |             |                    |
+-------------+----------------+--------------+-------------+--------------------+

4. Monitoreo
------------

- Revisiones trimestrales del registro de riesgos.
- Seguimiento de issues en GitHub.
