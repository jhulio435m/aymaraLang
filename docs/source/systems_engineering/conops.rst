Concepto de Operaciones (ConOps)
================================

1. Propósito
------------

Describir cómo los usuarios interactúan con AymaraLang y su compilador
``aymc`` en escenarios reales, incluyendo flujos operativos, roles y
restricciones.

2. Escenarios operativos
------------------------

2.1 Uso educativo en aula
~~~~~~~~~~~~~~~~~~~~~~~~~

1. Docente distribuye ejemplos ``.aym``.
2. Estudiantes editan el código en un editor de texto.
3. Ejecutan ``aymc`` para compilar y correr los binarios.
4. Se analiza salida para reforzar conceptos de programación.

2.2 Uso comunitario y cultural
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Creación de materiales educativos en aymara.
2. Publicación de ejemplos y ejercicios.
3. Ejecución en equipos modestos sin depender de conexión a internet.

3. Actores y responsabilidades
------------------------------

+-----------------------------------+-----------------------------------+
| Actor                             | Responsabilidad                   |
+===================================+===================================+
| Usuario final                     | Escribir y ejecutar programas     |
|                                   | ``.aym``                          |
+-----------------------------------+-----------------------------------+
| Docente                           | Diseñar ejercicios y guiar el     |
|                                   | aprendizaje                       |
+-----------------------------------+-----------------------------------+
| Desarrollador del compilador      | Mantener el compilador y          |
|                                   | documentación                     |
+-----------------------------------+-----------------------------------+

4. Supuestos
------------

- El entorno cuenta con compilador C++ y herramientas de
  ensamblado/enlace.
- El usuario dispone de acceso de lectura/escritura al sistema de
  archivos.

5. Limitaciones operativas
--------------------------

- El backend LLVM es opcional y puede no estar disponible.
- Soporte limitado a x86_64 en la generación nativa.

6. Criterios de éxito operacional
---------------------------------

- Compilación exitosa en menos de 2 segundos para ejemplos educativos.
- Ejecución consistente en Windows y Linux.
- Mensajes de error comprensibles para estudiantes.
