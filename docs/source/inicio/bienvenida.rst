AymaraLang — Documentación oficial
==================================

Bienvenido a la documentación oficial de **AymaraLang** (``aym``), un
lenguaje de programación inspirado en la lengua aymara y con un
compilador escrito en C++17. Aquí encontrarás desde una introducción
general hasta una referencia completa del lenguaje, la arquitectura del
compilador y las guías de instalación.

¿Por dónde empezar?
-------------------

- **Primera vez:** lee la :doc:`Visión general <inicio/vision_general>` y el apartado
  de :doc:`Primeros pasos <primeros_pasos/introduccion_al_lenguaje>`.
- **Aprendizaje práctico:** consulta la :doc:`Guía de
  características <lenguaje/guia_de_caracteristicas>`.
- **Referencia completa:** revisa la :doc:`Referencia del
  lenguaje <lenguaje/referencia_del_lenguaje>` y la :doc:`Gramática formal <lenguaje/gramatica_formal>`.
- **Desarrollo del compilador:** visita
  :doc:`Arquitectura <arquitectura/vision_general_del_sistema>` y :doc:`CLI y flujo de
  compilación <primeros_pasos/cli_y_flujo_de_compilacion>`.

Estructura de la documentación
------------------------------

1. **Visión general**: identidad, objetivos y conceptos clave.
2. **Lenguaje**: sintaxis base, ejemplos y guía de características.
3. **Referencia**: operadores, funciones integradas, palabras clave y
   gramática.
4. **Compilador**: arquitectura y flujo de compilación.
5. **Construcción e instalación**: pasos por plataforma y generación de
   instaladores.
6. **Proyecto**: estructura del repo, contribuciones y roadmap.
7. **Sistemas de ingeniería**: documentos de arquitectura, requisitos y
   validación.
8. **Recursos**: presentaciones y material complementario.

Construir la documentación con Sphinx
-------------------------------------

La documentación está preparada para **Sphinx** y usa archivos
reStructuredText (``.rst``).

.. code:: bash

   pip install sphinx
   sphinx-build -b html docs/source docs/build/html

Luego abre ``docs/build/html/index.html`` en tu navegador.

--------------

**Siguiente:** :doc:`Visión general <inicio/vision_general>`
